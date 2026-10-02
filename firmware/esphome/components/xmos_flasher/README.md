<!--
SPDX-FileCopyrightText: 2026 marceldale
SPDX-License-Identifier: MIT
-->

# `xmos_flasher` — write the XU316 factory image from the ESP32

The XU316 boots from its own SPI flash U11 (W25Q32JVSSIQ), which is **empty as delivered**. Once an
image is in it, `voice_kit` keeps it current over I²C (DFU). This component fills an empty or damaged
U11 from the ESP32-S3, without a JTAG adapter: it writes `firmware/xmos/ffva_v1.3.1_factory.bin`
(286 720 bytes) over the bus switch U16, verifies it and restarts.

## Operation (Home Assistant)

1. Supply the board from 14 V (base plate or bench supply at TP18). On USB alone the write is refused
   (see "Guard" below).
2. Switch on **"XMOS-Flash freigeben"** (configuration entity). It switches itself off after 60 s.
3. Press **"XMOS-Flash schreiben"** within those 60 s. The wake word, the voice assistant and media
   playback are stopped first.
4. Watch **"XMOS-Flash Status"** (`connect` → `identify` → `erase` → `write` → `verify` → `done`) and
   **"XMOS-Flash Fortschritt"** (%). It takes about 15 s (erase 256 × 45 ms typical, 1 120 pages of
   about 1.5 ms, 1 MiB read back at 4 MHz); each `loop()` call works for up to 20 ms, so the write is
   not paced by ESPHome's 16 ms loop interval.
5. On success the ESP32 restarts after 3 s; the log must then show `[voice_kit] DFU version: 1.3.1`.
   On failure the status reads `failed: <stage> <reason> at 0x<address>`, the bus is released and the
   XU316 is let out of reset; nothing restarts. Pressing the button again repeats the whole write.

Nothing happens at start-up: the component only acts on the button.

## What it does on the bus

| Step | Action | Evidence |
|---|---|---|
| connect | GPIO46 high: Q2 pulls `RST_N` low (XU316 in reset) **and** U16 connects D± to its 2D side (`S` = 1, `OE` = GND), so GPIO39 reaches `QSPI_CS_N` and GPIO40 `QSPI_D1`. 10 ms settle. SPI2 is set up on GPIO3 (CLK), GPIO4 (MOSI = D0), GPIO40 (MISO), GPIO39 (CS), mode 0, 4 MHz. | Netlist: `XU316_RST` = U8.52 GPIO46, R47, U16.9 S, TP11; `QSPI_CLK` = U8.8, U11.6, U10.5; `QSPI_D0` = U8.9, U11.5, U10.59; `XMOS_SPI_CS_N` = U8.44, U16.8; `XMOS_SPI_MISO` = U8.45, U16.7; U16.3/4 = `QSPI_CS_N`/`QSPI_D1`. TS3USB221 table 7-1: OE low, S high → D = 2D. In reset the XU316's GPIOs are inputs with weak pull-downs (XU316 datasheet section 8). |
| identify | First 16 clocks of FFh (ends a continuous-read mode the XU316 may have left), wait while BUSY (an interrupted DFU erase/program), then 66h + 99h software reset (tRST 30 µs) — never during BUSY, the datasheet warns of data corruption. Then 9Fh must return **EF 40 16**; 35h is read and **QE must be 1** (the IQ part has QE fixed at 1; with QE = 0 `/HOLD` would be active and is pulled low by the XU316 in reset). SR1 BP0–2/TB/SEC and SR2 CMP must be 0 (otherwise erase/program would be ignored silently) — the status registers are never written, a protected flash is reported, not unlocked. | W25Q32JV datasheet: JEDEC ID table, QE bit S9 ("factory fixed default for … IQ"), section "Enable Reset (66h) and Reset Device (99h)", block-protect bits. |
| erase | 06h + 05h (WEL must be set), 20h for each 4 KiB sector of the **whole 1 MiB boot partition** (256 sectors), busy-polled with 05h, 1 s timeout per sector. The data partition above 1 MiB is not touched. | tSE 45 ms typ / 400 ms max. Boot partition 0x100000 as the image was built (`firmware/xmos/README.md`). |
| write | One 256-byte page per loop: 06h, 02h, busy-poll (10 ms timeout), then 03h read-back and byte compare. The last page is padded with FFh. | tPP 0.4 ms typ / 3 ms max; 03h up to fR = 50 MHz. |
| verify | 03h over the whole boot partition, 4 KiB per loop: MD5 of the first 286 720 bytes against `MD5SUMS`, and every byte after the image must be FFh (no stale upgrade image). | |
| release | SPI bus freed, **GPIO3/4/39/40 back to floating inputs first**, then GPIO46 low: U16 isolates CS/MISO and the XU316 boots from U11. On success the ESP32 restarts (`App.safe_reboot()`), so `voice_kit`, the microphones and the amplifier come up in their normal order. | |

The image is **embedded at compile time** (`image_file`, `md5_file` in the YAML). The build fails if
its MD5 does not match `MD5SUMS` or if it does not fit the boot partition. There is no download at
run time. Cost: firmware image 2 283 639 → 2 600 735 bytes (32.0 % of an 8 MB OTA slot).

## Limits

* No lock against a running `voice_kit` DFU update or against automations that restart the wake word
  during the write: the flash is not at risk (the XU316 is in reset, the ESP owns the bus), but a DFU in
  progress is aborted and audio components log errors until the restart.
* The SPI pins are given as plain numbers, so ESPHome does not flag a second use of GPIO3/4/39/40 in the
  YAML; keep them free (the YAML comment lists them as reserved).
* After a failure the wake word stays stopped until the next restart.

## Guard: not on USB alone

The button is refused unless the amplifier is configured and does **not** report PVDD under-voltage
(`id(tas5805m_dac).is_failed()`, binary sensor `endstufe_pvdd_uv`). PVDD exists only on the 14 V
rail (R66), so this is the "14 V present" test the board offers; there is no GPIO for it. The log names
the condition that failed. **Open until the sample:** with an empty U11 the XU316 does not run, and
`tas58xx` is then set up without I²S clocks — I²C works regardless, so the amplifier should not be
marked failed and should not report PVDD under-voltage on 14 V; the bring-up check of the first board (working notes, HANDGRIFFE 6.8) tests exactly this.

Load calculation for the record: electrically the write would also work on USB alone. The board
needs about 0.66 A on USB with peaks of about 0.8 A in normal operation (CHANGES.md, M17); during a
write the XU316 is in reset, audio is stopped, and U11 adds at most 25 mA (ICC4/ICC5). That stays
below U18 (1.5 A) and the U4 current limit (≥ 2.2 A). The guard therefore follows the operator's rule
(write on 14 V, as in the bring-up order), not a current limit; to allow USB, drop the lambda line.

## Origin and licence

Derived from **FutureProofHomes/Satellite1-ESPHome** (commit `8f8906c`, 2 Oct 2026), components
`memory_flasher` and `satellite1/memory_flasher` (`xmos_flashing.cpp/.h`) by Mischa Siekmann. That
repository is under the ESPHome licence: C/C++ files under **GPLv3**, Python under **MIT**. It does
not say "or any later version", so the C++ files here are marked **GPL-3.0-only** (not "or later")
and `__init__.py` **MIT**, both naming the original author.

Taken over: the stage machine run from `loop()` one bounded step at a time; page program with WEL
check, busy poll and read-back compare; padding of the last page with FFh; MD5 over the written image
against the expected value; JEDEC check before writing; image embedded as a PROGMEM array with a
compile-time MD5 check; progress weighting by time rather than bytes.

Changed for this board (the line-by-line comparison is in the project's working notes):

* **Bus access.** Satellite1 switches all lines through its own bus and stays an `spi` device of its
  hub. Here CLK and D0 are wired permanently to the XU316, so the SPI bus exists only during a write
  and the pins are returned to floating inputs before the XU316 leaves reset. One GPIO (46) does reset
  and switching together.
* **Erase range.** Satellite1 erases the image sectors plus one (the upgrade-header sector). Here the
  whole 1 MiB boot partition is erased and verified as FFh after the image, because the target is an
  empty *or damaged* flash.
* **Verification.** Satellite1 hashes the stream it writes and keeps a persistent record. Here the
  flash is read back after writing and the MD5 is taken from the flash itself, plus the FFh tail.
* **Not taken over:** remote (HTTP) images, full-chip erase, factory-reset action, the persistent
  resume record across reboots and the unique-ID check. A write interrupted by a power loss leaves an
  invalid image; the remedy is simply to press the button again (the XU316 then fails its version
  read, and `voice_kit` reports it).
* Read with 03h instead of 0Bh (fast read); 4 MHz default instead of the hub's SPI clock.

## Files

| File | Licence |
|---|---|
| `xmos_flasher.h`, `xmos_flasher.cpp` | GPL-3.0-only (derived) |
| `flash_plan.h` | GPL-3.0-only (constants and `static_assert` checks) |
| `__init__.py` | MIT (derived) |
| `README.md` | MIT |
| `../../tests/test_xmos_flasher.py` | MIT — host test against a simulated W25Q32JV |
