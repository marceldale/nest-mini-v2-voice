<!--
SPDX-FileCopyrightText: 2026 marceldale
SPDX-License-Identifier: MIT
-->

# Local copy of `tas58xx` with a start-up patch

`components/tas58xx/` is the ESPHome external component
[mrtoy-me/esphome-tas58xx](https://github.com/mrtoy-me/esphome-tas58xx) at commit
`89f29cfd687200e908a6459404abfbb513adb17d` (branch `main`, the newest state on 1 Oct 2026),
copied unchanged in one commit and patched in the next, so `git diff` between the two shows the
patch exactly. `nest-mini-v2-voice.yaml` loads it with `external_components: source: type: local`.

## Why

The TAS5805M on this board is wired for PBTL (outputs 23+26 and 17+20 paralleled). The original
start-up writes a register table (`tas58xx_minimal.h`) that ends with

```
{ 0x02, 0x00 },   // DEVICE_CTRL_1: BTL
{ 0x54, 0x00 },   // AGAIN: 0 dB
{ 0x03, 0x03 },   // DEVICE_CTRL_2: PLAY
```

and only afterwards `configure_registers_()` sets PBTL (`set_dac_mode_`, DEVICE_CTRL_1 bit 2) and
the analog gain (`set_analog_gain_`, −12 dB). For that short time the amplifier plays in BTL at
full analog gain into paralleled outputs. The TAS5805M datasheet (SLASEH5D, 7.5.3.1 step 4) asks
for the device to be set into Hi-Z, configured, and only then put into PLAY.

## The patch

| File | Original | Patched |
|---|---|---|
| `tas58xx_minimal.h` | `{ 0x03, 0x03 }` (PLAY) at the end of the table | `{ 0x03, 0x02 }` (Hi-Z) |
| `tas58xx.h` | `ControlState tas58xx_control_state_;` — never initialised (the comment says "initialised in setup", `setup()` does not) | `ControlState tas58xx_control_state_{CTRL_HI_Z};` |
| `tas58xx_minimal.h` | `{ 0x53, 0x00 }` — class-D loop bandwidth 80 kHz | `{ 0x53, 0x60 }` — 175 kHz (pin audit U14-B2, added 2 Oct 2026) |
| `tas58xx.cpp` `update()` | every fault is cleared on the next 1 s update, DC and over-current included | DC and over-current faults (CHAN_FAULT bits 0–3) are not cleared; nothing is cleared while one is present, the output stays off until the device restarts (added 5 Oct 2026, as in the ESPHome core component; SLASEH5D 7.5.3.3.1/2: a DC fault re-trips only after 570 ms) |

With both, `configure_registers_()` runs: `set_deep_sleep_off_()` (no write, state is Hi-Z) →
`set_modulation_scheme_()` → `set_dac_mode_(PBTL)` → `set_analog_gain_(−12 dB)` →
`set_state_(CTRL_PLAY)`, which now writes PLAY because the state differs. Without the
initialisation, a random start value could equal `CTRL_DEEP_SLEEP` (then `set_deep_sleep_off_`
writes PLAY before PBTL) or `CTRL_PLAY` (then PLAY would never be written).

**Why 175 kHz (U14-B2).** Register 0x02 is 0x04 here (FSW_SEL = 000 = 768 kHz, PBTL, BD). The
datasheet's field description of ANA_CTRL (table 7-27) says "With Fsw=768kHz, 175kHz bandwidth should
be selected for high audio performance", section 7.3.8.2 says "Same Fsw, Better THD+N performance
with higher BW", and table 7-3 allows 175 kHz for BD at 768 kHz (rule Fsw ≥ 3 × BW: 768 ≥ 525 kHz).
The datasheet's PBTL curves are measured with exactly this set-up (768 kHz, 10 µH / 0.68 µF, BD,
175 kHz), and its example start-up script writes `w 58 53 60`. The value is written while the device
is still in Hi-Z. On the sample: read back 0x53 (expect 0x60) and compare THD+N before/after.

Not changed: the undocumented register `0x46 = 0x01`, whose source comment says "SR 96KHz; for
48KHz use 0x11" while this bus runs at 48 kHz. TI lists the address as reserved; the effect has to
be checked on the sample (read back register 0x71 for clock errors) before touching it.

## Licence

The upstream repository has no licence file. Its README states that the component is based on
work by Andriy Malyshenko (sonocotta) licensed under GPL-3.0, so the copy is distributed here under
**GPL-3.0-or-later** (`LICENSES/GPL-3.0-or-later.txt`), with mrtoy-me and Andriy Malyshenko named as
authors and our two modifications marked with a dated notice at the top of each changed file
(GPL-3.0 section 5 a). The assignment is in `REUSE.toml`. This file (`PATCH.md`) is our own text, MIT.

The same change was submitted to the upstream repository as PR #4. Status: PR #4 closed; per maintainer's comment in mrtoy-me/esphome-tas58xx#4 the start-up order is covered by his development branch and by the ESPHome core driver (esphome/esphome#19971).
