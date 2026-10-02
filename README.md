# Open voice satellite board for the Google Nest Mini (2nd gen)

A drop-in replacement board that turns a Google Nest Mini (2nd gen) into a fully local voice
assistant for Home Assistant. Started from the open design by
[iMike78](https://github.com/iMike78/nest-mini-drop-in-pcb) (nest-mini-drop-in-pcb,
CERN-OHL-S v2) and since then reworked, verified and extended into its own board. Everything
that was changed, and why, is in [CHANGES.md](CHANGES.md); the people and projects this one
stands on are named under [Credits](#credits).

> [!WARNING]
> **Status (2 Oct 2026): rev A2 ordered (2 boards, JLCPCB, 2 Oct 2026).** Every change has been
> verified on paper only — netlist, ERC, DRC with schematic parity, datasheets, firmware source.
> The rotations of all 31 changed or new parts were checked in JLCPCB's assembly preview, and the
> board was ordered with the two fixed options (via filling, ENIG). **Nothing has been checked on
> hardware yet.** Until bring-up results are published here, **do not manufacture from these
> files.** Watch the repository for updates.

> [!NOTE]
> This status line changes again when the boards have been brought up. The steps are tracked in
> the roadmap below; the order itself is in [CHANGES.md](CHANGES.md#rev-a2-ordered-2-oct-2026).

![Status](https://img.shields.io/badge/status-rev%20A2%20ordered-yellow)
![Licence](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-blue)

## What it is

The Google Nest Mini (2nd gen) is a well-built little speaker: a good 40 mm driver, a
clean enclosure, two microphones and a touch surface. What ties it to Google is only the
circuit board inside.

This project replaces that board. You open the Nest Mini, take out Google's electronics
and put this board in its place. Speaker, housing, base plate and touch surface stay as
they are. What you get is a fully local voice assistant for Home Assistant: wake word,
speech and echo cancellation run on the device (ESP32-S3 + XMOS XU316), nothing leaves
your network, and the speaker still plays music and announcements.

Under the hood it follows the Home Assistant Voice Preview Edition: same processor
pair, same audio architecture, same ESPHome firmware base — fitted into the Nest Mini's
size, power and mounting constraints.

**What you need:** a Nest Mini (2nd gen), this board (assembled), a Home Assistant
installation and a USB-C cable for the ESP32. The XMOS needs its first image over JTAG (J4, an
XTAG4 adapter) until the ESP32-side flasher is written — see Highlights. If you
run it from USB rather than from the Nest Mini's own 14 V base plate, use a supply that can give
**1 A** — at full LED brightness the board draws up to 0.8 A, so a plain 500 mA charger is not
enough. On USB alone the amplifier has no supply — PVDD comes only from the 14 V rail (R66) — so
the speaker stays silent while microphones, LEDs and Wi-Fi work; the log then shows a PVDD
under-voltage fault or a `tas58xx` set-up error. That is expected, not a defect.

**What it is not:** a finished consumer product. It is an open hardware design at
prototype stage (see status above). No affiliation with Google.

## Highlights

* **XMOS flash reachable from the ESP32 — hardware only, so far.** A bus switch (U16, after the
  Satellite1 principle) lets the ESP32 write the XMOS flash directly. **The ESP32 software for
  that is not written yet.** Until it exists, the first XMOS image goes in over JTAG: J4 is an
  xSYS2 pad field, an XTAG4 plugs on 1:1. Later updates run over I²C (DFU) from the ESP32.

* **Audio path as on Voice PE.** ESP32 feeds the amplifier and the echo reference on one
  line; XU316 is the clock master. Works with the unmodified Voice PE XMOS firmware.

* **Power without operating rules.** Upstream, USB-C and the 14 V supply must never be
  connected at the same time, and even USB alone back-feeds the 14 V rail through the
  buck converter (a known issue). This board adds an ideal diode (LM66100) behind the
  5 V converter and a small switch that disables the USB supply while 14 V is present.
  Result: plug in USB, 14 V or both, in any order, without risk to the board or the host
  PC.

* **XU316 core on a buck converter.** Upstream feeds the 0.9 V core from a 200 mA linear
  regulator dropping 2.4 V, while the XU316 draws about 300 mA typical and up to 1 A under
  DSP load: the regulator would overheat and the XMOS would brown out. This board uses the
  TPS62065 with 2 A. Also: touch controller wired, antenna matching at the antenna, mic-3 path removed, EasyEDA-checked
  footprints, every part with an LCSC number.

* **Expansion connector (12-pin FPC)** for an add-on ring board. We wanted more than a
  voice satellite: presence detection (radar over UART), temperature/humidity (I²C), IR
  transmit and receive for controlling TV or air conditioning, and an LED ring. None of
  this fits inside the Nest Mini housing (the mounting plate sits flush on the board), so
  the connector stays unpopulated there and the board works as a plain Nest Mini
  replacement. In a custom enclosure it connects to a ring board — LEDs, IR emitters and
  receivers, radar module — that is planned as the next project. The main board carries
  the signals, the ring board carries the sensors.

## Roadmap

- [x] Design review against upstream, Voice PE and Gen1 ([CHANGES.md](CHANGES.md))
- [x] Rev A2 schematic and layout: XMOS flash via ESP32, Voice PE audio path, power without
      operating rules, expansion connector routed
- [x] Rev A2 silkscreen: pin-1 marks, test point labels, revision and date; no DRC warnings left
- [x] Independent audit of rev A2; critical and layout-relevant findings fixed (CHANGES.md section 10)
- [x] Rev A2 manufacturing files regenerated after the audit fixes (`fertigung/`, 1 Oct 2026)
- [x] Rotation check of the 31 changed or new parts in the fabricator's assembly preview
      (JLCPCB preview, 2 Oct 2026; U18, U16, U5, Q4 corrected in the CPL beforehand, Y1 by order remark)
- [ ] ESP32-side flasher for the XMOS image (until then: first image over J4 with an XTAG4)
- [x] Rev A2 ordered: 2 assembled boards (5 bare PCBs), JLCPCB, 2 Oct 2026
- [ ] Bring-up: resistance check, power rails, XMOS flash via ESP32
- [ ] Audio, microphones, echo cancellation, touch, LEDs
- [ ] Wi-Fi sensitivity against a reference device
- [ ] Firmware release (ESPHome YAML + XMOS factory image)
- [ ] Ring board and enclosure files
- [ ] Certification plan
- [ ] Crowd Supply campaign

## Trademarks

Google and Google Nest Mini are trademarks of Google LLC. This project is not affiliated with,
endorsed by or sponsored by Google. Home Assistant and ESPHome are projects of the Open Home
Foundation / Nabu Casa. XMOS and xcore are trademarks of XMOS Ltd. All trademarks belong to
their respective owners and are used here only to say what this board is compatible with.

## Licences

| What | Licence |
|---|---|
| Hardware — schematic, layout, our footprints, manufacturing data | **CERN-OHL-S v2** |
| ESPHome YAML, Python tools, documentation | **MIT** |
| XMOS firmware image under `firmware/xmos/` | **XMOS Public Licence v1** — not ours |
| ESPHome component `tas58xx` under `firmware/esphome/components/` | **GPL-3.0-or-later** — © mrtoy-me, based on work by Andriy Malyshenko (sonocotta); patched by us (start-up order, loop bandwidth), see [`firmware/esphome/PATCH.md`](firmware/esphome/PATCH.md) |
| KiCad library excerpts in `lib_lokal/` | **CC-BY-SA-4.0**, © KiCad Libraries Contributors |
| Parts of `lib_lokal/` (vendor footprints and symbols) | **terms not established** — see below |

Full texts are in [`LICENSES/`](LICENSES/). Files that can carry a comment header do carry an
`SPDX-License-Identifier`; for KiCad documents, Gerbers and binaries the assignment is in
[`REUSE.toml`](REUSE.toml), following the [REUSE](https://reuse.software) specification.

**One honest gap**, written up in
[`LICENSES/LicenseRef-Vendor-Model-Terms-Not-Established.txt`](LICENSES/LicenseRef-Vendor-Model-Terms-Not-Established.txt):

* The vendor 3D models the board references are **not** part of this repository: we hold no
  licence document for any of them. The 3D view therefore shows those parts without a body;
  fabrication and assembly do not need them.
* Parts of `lib_lokal/` have a similar problem: ten of the 23 footprints in `Onju.pretty` carry a
  SnapEDA or EasyEDA note, `New_Library.kicad_sym` is SnapEDA, and the Seeed and reSpeaker symbols
  come from Seeed's project. They arrived with the upstream design and we pass them on unchanged.
  This directory **cannot** be left out — without it the design does not open.

**`lib_lokal/` and the two library tables are versioned on purpose.** The design references every
symbol and footprint through `${KIPRJMOD}/lib_lokal/`, so a clone without them opens with empty
placeholders. The directory holds trimmed excerpts, not whole libraries: `Device.kicad_sym` is
17 kB here against 2 374 kB in a KiCad 9.0.9 installation. KiCad's library licence grants an
exception for designs that *use* the libraries but states that it does not apply when they are
redistributed as a collection — which is what this directory does — so those files keep
CC-BY-SA-4.0 with attribution.

The only C++ code in this repository is the patched copy of the ESPHome component `tas58xx` under
`firmware/esphome/components/` (GPL-3.0-or-later, see the table above); any further ESPHome
components would be GPL-3.0-or-later as well.
CERN-OHL-S v2 is a **strongly reciprocal** licence: if you make and distribute a board from
these files, or from a modified version of them, you have to make the complete source of your
version available under the same licence. See
[`LICENSES/CERN-OHL-S-2.0.txt`](LICENSES/CERN-OHL-S-2.0.txt), sections 3.2 and 4.

---

# For reference

Everything below is the technical detail — useful once you have a board in your hand, and not
needed to understand what the project is.

## Test points

Eighteen pads, all bare copper, nothing to populate. Sixteen are on the microphone side; the touch
electrodes are on the LED side. The silkscreen carries a short name next to each pad — the full net
name would not fit on a 68 mm board.

| Marking | Ref | Net | Side | Position | What you should see |
|---|---|---|---|---|---|
| `3V3` | TP1 | `+3V3` | microphone side | 163.00 / 74.50 | 3.3 V |
| `5V_SW` | TP3 | `/Power/+5V_SW` | microphone side | 121.40 / 102.00 | about 5 V, **before** the ideal diode U18 |
| `1V8` | TP4 | `+1V8` | microphone side | 145.70 / 71.50 | 1.8 V |
| `VDD` | TP5 | `VDD` | microphone side | 130.40 / 81.60 | 0.9 V |
| `3V3` | TP6 | `+3V3` | microphone side | 107.40 / 94.60 | 3.3 V |
| `XRST` | TP11 | `XU316_RST` | LED side | 126.10 / 66.50 | logic; high = XU316 held in reset |
| `MUTE` | TP12 | `MUTE` | microphone side | 140.90 / 116.80 | logic; follows the mute slider |
| `PA_EN` | TP13 | `AUDIO_PA_EN` | microphone side | 130.70 / 109.10 | logic; high = amplifier enabled |
| *(no marking)* | TP14 | `ELEC2` | LED side | 159.20 / 89.90 | touch electrode, right |
| *(no marking)* | TP15 | `ELEC1` | LED side | 135.82 / 119.53 | touch electrode, centre |
| *(no marking)* | TP16 | `ELEC0` | LED side | 114.20 / 88.90 | touch electrode, left |
| `5V` | TP17 | `+5V` | microphone side | 122.42 / 105.90 | about 5 V, **after** U18 — 55…65 mV below TP3 under load |
| `14V` | TP18 | `+14V` | microphone side | 143.90 / 109.43 | 14 V from the base plate |
| `RST` | TP19 | `ESP_RST` | microphone side | 118.58 / 79.30 | logic; low = ESP32 in reset |
| `BOOT` | TP20 | `ESP_BOOT` | microphone side | 121.84 / 81.75 | logic; low at power-up = boot mode |
| `GND` | TP21 | `GND` | microphone side | 120.58 / 64.74 | 0 V reference, next to U8 |
| `GND` | TP22 | `GND` | microphone side | 128.74 / 104.08 | 0 V reference, next to U14 |
| `VMIC` | TP23 | `VDD_MIC` | microphone side | 105.10 / 88.35 | 3.3 V microphone supply |

The three touch points carry no silkscreen: they sit inside the electrode rings on the LED side,
where any lettering would end up on copper. Their names are `T_R`, `T_C` and `T_L` in the schematic.

**The pair TP3 / TP17 is the one to measure first.** TP3 sits before the ideal diode U18, TP17
after it. Under load the difference should be about 55…65 mV; in USB-only operation TP3 should read
roughly 0 V, because the 14 V rail is then dead and the diode blocks the path back into it.

## Connectors

The board carries a designator and a pin-1 dot for every connector; the pinouts are here rather than
on the silkscreen, where they would not be legible at 0.5 mm pitch.

### J6 — ribbon cable to the LED ring board, 12 way, 0.5 mm pitch

| Pin | Net | | Pin | Net |
|---|---|---|---|---|
| 1 | `GND` | | 7 | `UART_RX` |
| 2 | `+5V` | | 8 | `I2C_SDA` |
| 3 | `+5V` | | 9 | `I2C_SCL` |
| 4 | `GND` | | 10 | `IR_TX` |
| 5 | `LED_DOUT` | | 11 | `IR_RX` |
| 6 | `UART_TX` | | 12 | `+3V3` |

Pin 1 is the one nearest the dot. `LED_DOUT` continues the LED chain from LED6 through R80, so the
ring board's first LED is index 6 as far as the firmware is concerned.

### POWER_IN1 — flat flex to the base plate, 10 way, 0.5 mm pitch

| Pin | Net | | Pin | Net |
|---|---|---|---|---|
| 1 | `GND` | | 6 | `GND` |
| 2 | `MUTE` | | 7 | `+14V` |
| 3 | `GND` | | 8 | `+14V` |
| 4 | `GND` | | 9 | `+14V` |
| 5 | `GND` | | 10 | `+14V` |

**Confirmed pin for pin against an independent design:** the schematic of
[Onju Voice](https://github.com/justLV/onju-voice) (`hardware/Onju-Home.SchDoc`, a replacement
board for the same Nest Mini 2nd gen) has the same ten-way connector as J1, and all ten pins
agree — 1 and 3–6 `GND`, 2 `MUTE`, 7–10 `14V`. Metering it on an original is optional; the quick
check is in `CHANGES.md` ("POWER_IN1: the pinout is now confirmed against a second source").

### J4 — XMOS JTAG pad field, 2×5, 1.27 mm pitch, unpopulated

The pin-out is the **xSYS2 JTAG-only header** of the XU316 datasheet (appendix G.3, figure 25), so
an XTAG4 plugs on **1:1** — pin 1 to pin 1. Pin 1 is marked by the silkscreen dot.

| Pin | Net | | Pin | Net |
|---|---|---|---|---|
| 1 | `+1V8` (VREF) | | 2 | `TMS` |
| 3 | `GND` | | 4 | `TCK` |
| 5 | `GND` | | 6 | `TDO` |
| 7 | `GND` | | 8 | `TDI` |
| 9 | `GND` | | 10 | `RST_N` |

VREF is 1.8 V on pin 1, with 100 nF (C103) right next to it. There is no 5 V and no 3.3 V on this
field. Up to the audit fixes of 1 Oct 2026 the field carried a rotated, non-standard pin-out with
VREF on pin 6; a board made from an older revision needs an adapter.


## Before bring-up

> [!CAUTION]
> **Do not burn `EFUSE_STRAP_JTAG_SEL` on the ESP32-S3.** Rev A2 uses GPIO3 and GPIO39 to
> GPIO42 as an SPI bus to the XMOS flash. In the factory state JTAG runs over the
> USB-Serial/JTAG controller and those pins are free; burning the eFuse switches JTAG to
> those pins and breaks the XMOS flash path. Details in
> [CHANGES.md](CHANGES.md#do-not-burn-efuse_strap_jtag_sel-esp32-s3).

## Manufacturing files

**`fertigung/` holds the rev A2 set**, generated after the audit fixes on 1 Oct 2026: Gerbers and
Excellon drill files, `gerber_jlcpcb.zip` for upload, BOM and CPL for JLCPCB, BOM and assembly
drawing for PCBWay. [`fertigung/ABLAUF.md`](fertigung/ABLAUF.md) documents the commands that
produce it and the checks each one prints — nothing in that directory is edited by hand.
**It is not released**: the board has never been fabricated, and two checks are still open (see
below).

Before ordering, these are the points to watch; the full list with the evidence is under
["Before you order" in CHANGES.md](CHANGES.md#before-you-order-the-seven-things-to-get-right):

* **0.10 mm minimum track width** for the two UART signals — fabricable at JLCPCB without a
  surcharge, but not everywhere.
* **Via-in-pad:** board vias reach into 23 pads (58 vias, all same-net), among them three signal
  pads and the large pads of U8 and U10. Order the vias **Epoxy Filled & Capped**.
* **U11 must be the `IQ` variant** of the W25Q32JV, not `IM` — the footprint cannot tell them
  apart, and only `IQ` has the QE bit set at the factory.
* Seven same-net via pairs sit exactly at the **0.2 mm hole-to-hole limit**.
* **J1** (Molex 1054500101, C134092) is stocked but flagged “process difficult” in the BOM
  check — confirm it by hand.
* Stack-up: 4 layers, 1.6 mm, **JLC04161H-7628**; surface finish chosen at order time.

Placement corrections by hand are **no longer needed**: rev A1 shipped a CPL with 19 of them, and
rev A2 fixes the footprint origins themselves instead (X1, J5, J1), so the CPL exporter
writes a correct CPL directly; the remaining nineteen corrections are applied automatically.

**Two things are still open**, both for the operator and both listed in
[`fertigung/DREHLAGEN_PRUEFEN.md`](fertigung/DREHLAGEN_PRUEFEN.md):

1. **31 parts need checking in JLCPCB's assembly preview** — those whose LCSC number changed
   since rev A1 (U5, LED2–LED5, J6, R76, R77, R80, X1, J1) or that are new (U16, U18, U19, Q4),
   nine from the audit fixes (L1, L2 with a new footprint; C20, C100 with new numbers;
   C105–C109 new, all unpolarised), and seven from the pin audit and the order (Y1 with a new number — the one
   where rotation matters, a 90° error puts the crystal on the ground pads, and it has no model in
   the preview, so the order carries the remark "Y1 orientation per silkscreen pin-1 corner mark
   (top-left)"; R72, C17, C63, C98, R35 with new numbers; R81 new). The first preview on 2 Oct 2026
   found U18 and U16 turned; a recomputation of all 42 polarised parts against the fabricator's
   footprints corrected four rotations (U18, U16, U5, Q4) — see `CHANGES.md`. C110 is not fitted and not in the CPL.
   For them the fabricator's own footprint decides the zero orientation, and that has not been
   compared yet. Everything else was checked in rev A1 and has not moved since.
2. **Two order options are fixed for this board** and have to be selected because they are not in
   the Gerbers: via covering ***Epoxy Filled & Capped*** (vias reach into 23 pads) and surface
   finish **ENIG** — decided, not optional (0.40 mm pitch, QFN thermal pads, the J4
   spring-contact field, test points and touch electrodes). Both are in "Before you order" in
   `CHANGES.md`.

The upstream design and its own documents are at
[iMike78/nest-mini-drop-in-pcb](https://github.com/iMike78/nest-mini-drop-in-pcb).

## Credits

This board exists because other people published theirs first.

* **[iMike78](https://github.com/iMike78/nest-mini-drop-in-pcb)** — *nest-mini-drop-in-pcb*,
  the design this one forks. The board outline, the mounting geometry, the microphone and touch
  arrangement and a great deal of the layout are the designer's work.
* **[Onju Voice](https://github.com/justLV/onju-voice)** (Justin Alvey) — the first drop-in
  board for the Nest Mini, and the origin of the `Onju` footprint library this project still uses.
* **[Home Assistant Voice PE](https://github.com/esphome/home-assistant-voice-pe)**
  (Nabu Casa / Open Home Foundation) — the audio architecture this board follows: the same
  ESP32-S3 + XMOS XU316 pair, the same clocking, and the XMOS firmware it runs.
* **[Satellite1](https://github.com/FutureProofHomes/Satellite1-ESPHome)** (FutureProofHomes)
  — the method for flashing the XMOS from the ESP32, which is what makes this board
  programmable without a debugger.
* **[ESPHome](https://esphome.io) and [Home Assistant](https://www.home-assistant.io)** — the
  firmware and the platform all of it runs on.

## How to follow or contribute

Star or watch the repository. **Issues and Discussions are handled here, in this repository:**
Discussions for questions, Issues for errors in the design or the documentation. Hardware
support starts after bring-up. Reference thread: Home Assistant community, topic 860001.
