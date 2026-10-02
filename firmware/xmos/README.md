# XMOS XU316 factory flash image

`ffva_v1.3.1_factory.bin` is the factory boot image for **U10 (XU316-1024-QF60B-C24)** and its
boot flash **U11 (W25Q32JVSSIQ, 4 MB)**. It contains the XTC boot loader and the factory
application, ready to be written to the SPI flash.

## Source notice (XMOS Public Licence v1, clause 2.3)

This file is compiled code. **The source code of the software it is built from is available under
the terms of the XMOS Public Licence v1** — a copy of which is in this directory as
`LICENCE-XMOS-PUBLIC-LICENCE-v1.rst` — from:

* <https://github.com/esphome/voice-kit-xmos-firmware> — the FFVA firmware for the Home Assistant
  Voice Preview Edition, tag **`v1.3.1`** (released 2024-09-17)

Nothing in the firmware was changed here. The `.xe` release artefact was repackaged into a flash
image by `xflash`; no source file was modified, added or removed.

Clause 2.2.2 of that licence permits commercial distribution only while the software is used on a
device designed, licensed or developed by XMOS. That is the case here: it runs on the XU316.

## Provenance

| | |
|---|---|
| Built from | `ffva_v1.3.1.xe`, 5 779 572 bytes, MD5 `7233589645bce7f7289c47136c0792ae` |
| Downloaded from | <https://github.com/esphome/voice-kit-xmos-firmware/releases/download/v1.3.1/ffva_v1.3.1.xe> |
| Tool | XMOS XTC Tools **15.3.1**, `xflash` build 61-0e6d8350 (2024-07-04) |
| Built on | 2026-09-30 |

## How this file was produced

```
xflash --factory ffva_v1.3.1.xe --boot-partition-size 0x100000 -o ffva_v1.3.1_factory.bin
```

The build is deterministic and the result does not depend on the target description: the same
286 720 bytes come out with `--target-file NC-VOICE-KIT.xn`, with an XN whose flash `Type` is changed
from `S25FL116K` to `W25Q32JV`, and with `--boot-partition-size` set to `0x80000` or omitted
entirely. The target description is compiled into the `.xe`, and the boot-partition size is metadata
rather than padding. So this file can be reproduced from the public release asset at any time, and
the checksums in `MD5SUMS` are the check.

## Size against the flash

| | Bytes | Share |
|---|---|---|
| This image | 286 720 (280.0 KiB) | — |
| Boot partition (`0x100000`) | 1 048 576 | 27.3 % used |
| U11 flash (W25Q32JVSSIQ) | 4 194 304 | **6.8 % used** |
| Left for the data partition | 3 145 728 (3 MiB) | — |

## Writing it

This image is for an external programmer or for assembly-time programming. **Over JTAG you do not
need it** — `xflash` writes directly from the `.xe`:

```
xflash --quad-spi-clock 50MHz --factory ffva_v1.3.1.xe --boot-partition-size 0x100000
```

The XTAG4 goes onto the J4 pad field **1:1**: J4 follows the xSYS2 JTAG-only pin-out of the XU316
datasheet (appendix G.3) — 1 VREF (`+1V8`), 2 TMS, 4 TCK, 6 TDO, 8 TDI, 10 RST_N, 3/5/7/9 GND; pin 1
is marked by the silkscreen dot. The pad
field is unpopulated — solder or use spring contacts. `xflash --list-devices` must show the adapter
and the device first. Afterwards power-cycle the board: the ESPHome log has to show
`[voice_kit] DFU version: 1.3.1` (about 3 s after the ESP32 resets the XU316) and must not show
`Communication with Voice Kit failed` before the first-time programming counts as done. From then on
updates go over DFU via I²C from the ESP32.

None of the XTC tools is on the search path on the design machine; load the environment per call:

```
cmd /c "cd /d <dir> && "C:\Program Files\XMOS\XTC\15.3.1\SetEnv.bat" && xflash …"
```

`W25Q32JV` is one of the 15 flash devices natively supported by `libquadflash`
(`target/include/QuadSpecEnum.h`, `WINBOND_W25Q32JV = 13`), so no `--spi-spec` file is needed and
nothing depends on SFDP auto-detection.

One thing to watch on the first write: the firmware's own target description sets
`QE_REGISTER = flash_qe_location_status_reg_0` and leaves `QE_BIT` commented out. Our part carries the
`IQ` order suffix, where the QE bit is factory-fixed to 1, so it should not be needed. If `xflash`
complains about the QE bit, set it in a local XN.
