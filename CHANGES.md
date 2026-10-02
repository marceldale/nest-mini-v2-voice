# Changes relative to upstream

This design is a modified version of **[iMike78/nest-mini-drop-in-pcb](https://github.com/iMike78/nest-mini-drop-in-pcb)**.
The upstream design is licensed under the **CERN-OHL-S v2** (see `LICENCE`), and this
modified version is licensed under the same licence.

This file is the modification notice required by CERN-OHL-S v2 section 3.3(b).

Some entries point to the maintainer's working notes (`AENDERUNGEN.md` with its section numbers,
`AUDIT_REV_A2.md`, "the project notes"). **These notes are not published.** Each statement in this
file therefore carries its own evidence — datasheet page, source file and line, or a figure measured
on the design files in this repository — and the pointer only says where the work was logged.

| | |
|---|---|
| Upstream base | commit `7de6eec6` ("v2 hardware", 2026-05-16) |
| Modified by | marceldale, 2026-09-19 to 2026-10-02 |
| Tools | KiCad 9.0.9 (ERC/DRC/netlist via `kicad-cli`); ESPHome 2026.9.0 for the firmware configuration |
| Status | **Not yet manufactured or tested.** Every change below is verified on paper only: netlist, ERC, DRC and datasheets. None has been verified on hardware. |

## How each change was checked

For every change we compared the netlist before and after, ran ERC, and ran DRC with
schematic parity. The final state of the design is:

* **DRC: no errors, no unconnected pads, schematic parity clean.** 249 parts (including the
  unpopulated C110), 196 nets (after the decisions of 2 Oct 2026, section 11).
* **Eight warnings remain, all of them understood:**
  * Seven `hole_to_hole` at via pairs 0.500 mm apart, which leaves 0.200 mm between the
    0.300 mm holes against KiCad's 0.2495 mm setting. All seven pairs are **same-net**
    vias on `+14V` (3 pairs), `+5V`, `+3V3`, `ESP_3V3` and `PVDD` (one each), and JLCPCB's limit for same-net vias is exactly
    **0.2 mm**, so these are at the limit but inside it. They come from upstream.
  * One `connection_width` of 0.0864 mm in the `GND` zone on B.Cu, at 98.700 / 54.500 — a
    point **50.7 mm from the board centre**, so outside the 34.600 mm board outline. It is a
    neck in a part of the zone that the board edge cuts away; nothing is manufactured there.
* **No DRC exclusions.** The four that rev A1 needed are gone: the three
  `copper_edge_clearance` findings at the touch-electrode rings were solved by pulling the
  rings back from the hole edge (E5b.2), and `courtyards_overlap` J3/J4 went with J3 (E3).
* **No silkscreen warnings.** The 23 that rev A1 carried were worked off one by one in E5b.5.
* ERC reports **66** messages. **None of them is a circuit error** — they break down as:

  | Kind | Count | What it is |
  |---|---|---|
  | `power_pin_not_driven` | 21 | no `PWR_FLAG` on rails that are fed from a connector or a regulator whose output pin is not typed as a power output. Cosmetic. |
  | `unconnected_wire_endpoint` | 18 | wire stubs on the **root sheet**, left over from parts removed in E1, E5.1, E5.4 and E5.5. Cosmetic; they carry no net. |
  | `pin_to_pin` | 14 | 12 are "Unspecified meets Passive" from upstream symbols whose pins have no type. **Two are errors and both are deliberate**: U14 pins 26/23 (OUT_A+/OUT_A−) and 17/20 (OUT_B+/OUT_B−) are tied together because the TAS5805M runs in **PBTL** (datasheet figure 8-9) — the two bridges in parallel into one 4 Ω driver. That is the intended wiring, and ERC has no way to know it. |
  | `label_dangling` | 6 | labels with nothing on them: `XMOS_USB_P`/`XMOS_USB_N` (left from U2, removed in E1), `XL_DN0`/`XL_DN1` (left from R37/R38, E5.4), `SPI_CS_N`, `MIC_DATA_2`. |
  | `global_label_dangling` | 2 | `I2C2_SCL`, `I2C2_SDA` — a second I²C bus that this board does not use. |
  | `same_local_global_label` | 2 | `I2C_SCL` and `I2C_SDA` exist as both a local and a global label on the ESP32 sheet. Same net either way. |
  | `multiple_net_names` | 2 | `GNDA`/`DGND` at U14 (the amplifier has one ground reference, which is intended) and `I2S_DIN_SEC`/`I2S_DOUT` on the XMOS sheet. |
  | `pin_not_driven` | 1 | LED2's `DIN` is fed from LED1's `DOUT`, but LED1's symbol types its pins as *Unspecified* while LED2's types them properly. A symbol-library difference, not a wiring one. |

  The 26 messages in the first four rows that are **left-over artefacts** (6 dangling labels,
  2 dangling global labels, 18 wire stubs) sit on the root sheet and are worth cleaning up in
  the editor; they change nothing electrically. Everything else is either a property of the
  upstream symbols or, in the case of the two PBTL errors, the design working as intended.

---

## 1. Audio reference path and clocking (XU316 ↔ I²S)

**Why:** The firmware runs the ESP32 as I²S clock *secondary* and expects the XU316 to
generate the word clock and bit clock. On the board, however, the XU316 was not connected to
the I²S bus that feeds the amplifier. As a result, nobody generated the I²S clocks: no audio,
and no echo-cancellation reference.

**What:** We connected the XU316 to the I²S bus.

| Measure | Change | Commit |
|---|---|---|
| C1 | XU316 X1D01 (pin 10) → `I2S_LRCK` (tap at R29). The fan-out of pad 8 (VDDIO) and pad 9 (X1D00) was re-routed to make room. | `f6ecf19` (schematic), `09bdf3d`, `c01c038` (layout) |
| C2 | X1D10 (pin 13) → `I2S_BCLK` (tap at R30) | `64e04b1` |
| C3 | X1D34 (pin 53) → `I2S_DIN_ESP` (tap at R25). R25 was first set DNP, so ESP32 GPIO11 no longer drove the same net; in E5c-P5 **R25 was removed from the design altogether** and X1D34 (U10.53) now carries an explicit **no-connect**. **Superseded by E5d — see the note below.** | `bee3bd9`, `2d0813b` |
| C4 | X1D11 (pin 14) → `I2S_MCLK` | `1a7fdb9` |
| C6 | R23 and R26 were first set DNP, which separates ESP32 GPIO9 and GPIO12 from the MCLK node; in E5c-P5 **R23, R25 and R26 were removed from the design** — symbols, pads and stubs — because their pads walled in the east region. R24 (populated) stays, and so does R31 (X1D11 ↔ X0D36). No MCLK pin is used in the firmware. | `fed148e` |
| C7 | XU316 reset `XU316_RST` moved from ESP32 GPIO17 (U8.23) to **GPIO46** (U8.52), to match the routing. There was no via space for GPIO17 next to `AUDIO_PA_EN`. | `3dc22d3` |

**Correction to C3 (rev A2, E5d, 28.09.2026):** C3 gave the amplifier its I²S data from the
XU316 (X1D34 → `I2S_DIN_ESP` → U14.8/SDIN). That makes audio depend on the XU316 firmware
forwarding the stream. E5d moves U14.8 to `I2S_DIN_SEC` instead, so the amplifier now takes its
data straight from the ESP32:

    GPIO10 → R24 (0R, fitted) → I2S_DIN_SEC → U14.8 (SDIN)
                                             └→ R40 (0R, fitted) → X1D00 (XU316, AEC reference)

The clocks are unchanged and still come from the XU316 (C1: X1D01 → `I2S_LRCK`,
C2: X1D10 → `I2S_BCLK`). After E5d, X1D34 (U10.53) drives no target. R25 was removed in E5c-P5,
and U10.53 carries an explicit no-connect. `I2S_DIN_SEC` keeps its name on purpose, because it
still describes its source (the ESP32's second I²S peripheral). The net `I2S_DIN_ESP` no longer
exists.

**Why X1D34 stays a no-connect (E5d-N, 30.09.2026):**

* **Voice PE:** X1D34 goes through **R73 (0R, fitted)** to the AIC3204 input `DIN/MFP1`. ESP32
  GPIO11 also reaches that net, through R97 (0R, fitted).
* **The Voice PE firmware does not use that input.** ESPHome sets up the AIC3204 so that it
  takes its playback data on `SCLK/MFP3`, which is `I2S_DIN_SEC` from GPIO10
  (`aic3204.cpp`, lines 40–43). The XU316 firmware (ffva v1.3.1) writes no playback data to
  X1D34 at all: in `voice-kit-xmos-firmware` v1.3.1 the only `rtos_i2s_tx()` call on the playback
  bus is commented out (`src/ffva/src/main.c`, line 208) and the TDM branch is disabled
  (`appconfI2S_TDM_ENABLED 0`), so the output port 1K on X1D34 (`NC-VOICE-KIT.xn`, lines 50–65)
  only ever sends an empty buffer.
* **The TAS5805M has no equivalent.** It has a single SDIN, which now sits on `I2S_DIN_SEC`. A
  DNP resistor from X1D34 would therefore serve no purpose.
* **An XMOS-fed amplifier ("path F")** would need a modified XU316 firmware *and* a layout change.

Logged in the working notes (AENDERUNGEN, sections 83a and 100).

**Evidence:**

* The XU316 ports X1D01, X1D10, X1D11 and X1D34 are the ones the Home Assistant Voice PE
  (same ESP32-S3 + XU316 combination) uses for this I²S bus.
* **C7 strapping:** ESP32-S3 datasheet v2.2, Tab. 3-1 and 3-3. GPIO46 has a weak pull-down,
  and R48 (470 kΩ to GND) holds it low at reset.

## 2. Power

| Measure | What | Why | Evidence | Commit |
|---|---|---|---|---|
| A1 | 0.9 V core supply of the XU316 changed from LDO **TPS7A0309 (U6, SOT-23-5)** to a buck converter **TPS62065DSGR** (WSON-8) with 1 µH (L8), divider 51 k / 100 k (R74/R75), 22 pF feed-forward (C100; 120 pF since audit fix M2). C13/C18/C20 are now 10 µF (C20 4.7 µF since M2). | The LDO is rated 200 mA (TPS7A03 datasheet), while the XU316 core draws about 225–300 mA (XU316 datasheet). The LDO would also dissipate about 0.5 W in a SOT-23-5. The Voice PE also uses a buck converter for this rail. | TPS62065 datasheet: Vout = 0.6 V × (1 + 51/100) = 0.906 V; XU316 VDD range 0.855–0.945 V | `daf86a4`, `3ff45d1`, `ec8229c`, `9600d92`, `2506c3c`, `9f279f2`, `3c7bb45` |
| A2 | DEF pin of both TPS62130 (U3, U7) to GND through 0 Ω (R7, R14; previously 100 k); PG pull-ups R10/R16 DNP (PG left open) | On U3, DEF and PG were pulled up to +14 V through 100 kΩ, but their absolute maximum is 7 V. DEF high also raised both outputs by 5 %, to 5.25 V and 3.43 V, leaving 0.17 V margin to the ESP32-S3's 3.6 V limit. PG is not used. | TPS62130 datasheet (TI SLVSAG7F) sections 7.1 and 8.3; after the change +5V = 5.00 V and +3V3 = 3.27 V | `7ac93e5` |
| — | Three GND vias for ground islands without a via to the plane (at C25/C70 and two on B.Cu); one dangling via on `LED1-DOUT` removed | Found by filling the zones: isolated copper | DRC on the filled copy shows no islands | `0dcb0e5` |
| A1b | **FB3 removed**: the 600 Ω ferrite bead between the converter output and `VDD` is gone, `+0V9` and `VDD` are now one net (named `VDD`), and the 1.47 mm gap is closed with a 0.80 mm track on F.Cu. C13/C18/C20 stay. This also puts the converter's feedback (R74) on the same net as the XU316's VDD pins. | The bead's DC resistance eats the core rail's margin. See the table below. | Worst-case VDD at 830 mA: 0.746 V with the bead, 0.829 V with a 0 Ω link, **0.871 V without** — against a lower limit of 0.855 V | this commit |

| E5.2 | **U5 changed from MIC5365-1.8 to TLV70018DCKR** (LCSC C133796). Type, value,
manufacturer and part number only — the footprint stays. | The MIC5365 had no stocked part number;
the TLV70018 is a current TI part in the same package and is pin-compatible. | TI TLV700 datasheet
(SLVSA00E): the **DCK** suffix is SC70-5, which is the footprint already in use — not the DBV
(SOT-23-5) variant. Pin order IN/GND/EN/NC/OUT is the same in both (Micrel MIC5365/6 Rev. 3.1, pin
description p. 5; TI TLV700 SLVSA00E, pin functions p. 4). U5 runs from `+3V3` with `EN` tied high
and feeds **`+1V8`** — the XU316's VDDIOB18 bank (U10 pins 17, 26 and 31), the JTAG reference on
J4.6 and TP4. The swap raises the rating from 150 to **200 mA**, lowers the minimum input from 2.5
to 2.0 V and gives 85 mV dropout at 100 mA; the 1 µF in and out stay as they are. The land pattern
was checked against TI drawing DCK0005A (4214834/G, 11/2024): pitch 0.650 mm and pad width
0.400 mm match to the hundredth. | `737b487` |

**L8, the inductor of that converter, checked against the manufacturer's datasheet.** The
design asks for 1 µH at **≥ 2 A** saturation. The ordered part is not the Murata the design
names: LCSC C395545 is the **Sunlord WPN201610U1R0MTY01**, and LCSC publishes no product page for
that number, so the figures had to come from the manufacturer. From the Sunlord WPN datasheet,
table *WPN201610U Series*:

| | |
|---|---|
| Saturation current `Isat` | **3.60 / 3.80 A** |
| RMS current `Irms` | 3.10 / 3.40 A |
| DC resistance | 0.05–0.06 Ω |
| Self-resonant frequency | 63 MHz |

**The ≥ 2 A requirement is met with margin.** `Irms` also covers the 1 A peak the XU316 core
draws under DSP load. The Murata DFE201610P-1R0M the design was dimensioned around has 3.1 A and
70 mΩ, so the ordered part is equal or better in every figure that matters here.

**Checked, no change needed:**

* **A3:** All input capacitors on +14 V / PVDD are rated ≥ 25 V. This is why C83 and C88 keep
  the 50 V part instead of being merged into the 16 V one — see section 7.
* **A5:** D1 is fixed as TPD1E10B06DPYR (C48260). The value field and the footprint name disagreed; both are the same X1SON-2 package.

### Why the ferrite bead in the core path had to go (A1b)

Both references — the Home Assistant Voice PE and the designer's Gen1 board — put a ferrite bead
between the 0.9 V converter and the XMOS core rail. **This fork does not**, and the reason is
arithmetic.

The load is the XU316-1024-QF60B-**C24**: I(VDD) typically **225 mA**, maximum **830 mA**
(XU316-1024-QF60B datasheet, power consumption section, row I24). VDD must stay within
0.9 V ± 5 %, i.e. **0.855 … 0.945 V**.

The supply is the TPS62065 in PWM mode (MODE tied to +5 V through 0 Ω). From **SLVS833E**
(March 2010, revised October 2020), table "OUTPUT", p. 5: `Vref` 600 mV, `VFB(PWM)`
−1.5 % … +1.5 %, load regulation −0.5 %/A. With R74 = 51 k and R75 = 100 k the nominal output is
0.6 V × (1 + 51/100) = **0.906 V**. Worst case adds ±0.675 % from the 1 % divider resistors
(sensitivity (R1/R2)/(1+R1/R2) = 0.338).

The bead was a **BLM18KG601SN1D**, 600 Ω @ 100 MHz, **DCR 150 mΩ**, rated 1.3 A (LCSC C85833,
retrieved 30 Sep 2026) — so the current rating was never the problem; the DC resistance was. The
copper from the converter to the XU316 pins adds 14.5 mΩ over 18.22 mm.

| | with the 600 Ω bead | with a 0 Ω link (≤ 50 mΩ) | **without FB3** |
|---|---|---|---|
| VDD at 225 mA (typical) | 0.848 V | 0.871 V | **0.882 V** |
| VDD at 830 mA (maximum) | 0.746 V | 0.829 V | **0.871 V** |
| margin to the 0.855 V limit at 830 mA | **−109 mV** | **−26 mV** | **+16 mV** |

Only removing the bead holds in every calculated case. A 0 Ω link — considered first — still falls
26 mV short at maximum current, and raising the divider is no way out either: at 0.93 V nominal the
upper tolerance reaches 0.950 V, past the 0.945 V limit. Tapping the feedback behind the bead would
regulate the drop away, but it puts roughly 1 µH in the control loop of a 3 MHz converter (a pole
near 50 kHz) and needs an 18 mm high-impedance sense line across the board.

Two honest caveats: the ±1.5 % is a datasheet limit over temperature and input voltage, typically
the error is near 0 %; and 830 mA is the XU316's absolute maximum with all tiles at full clock,
while the FFVA firmware stays well below it. The calculation is the worst case — but a design that
does not survive the worst case has no margin left.

What the bead used to do, decoupling the core rail from the converter's switching noise, is now left
to the output capacitors (C17 4.7 µF, C18 10 µF, C20 10 µF) and the VDD decoupling at the XU316
(nine capacitors). **The 0603 land pattern is gone**, so re-fitting a bead is no longer a
no-cost option — if bring-up shows noise on the core rail, the fix is a bead of a few mΩ in the
0.80 mm link, which means a layout change.

`TP5`, the test point on this rail, now reads `VDD` rather than `+0V9`.

**A4, back-feed into +14V — corrected:** an earlier version of this file said this happens
only when USB-C and the 14 V supply are connected at the same time. That is wrong. It already
happens on **USB alone**, which is the documented way to flash the board ("Disconnect
POWER_IN before USB flashing" on the silkscreen):

* With the barrel connector unplugged, U4 (AP22802) puts VBUS on +5V.
* +5V reaches the `SW` node of U3 (TPS62130) through L1. With `VIN` at 0 V, `SW` exceeds its
  absolute maximum of `VIN` + 0.3 V (TPS62130 datasheet SLVSAG7F, page 4).
* The body diode of the integrated high-side switch conducts and charges the whole +14V rail,
  including PVDD with about 340 µF.
* Once +14V rises, R8 also enables U3, which then runs at 100 % duty cycle and ties `VIN` to
  `SW`. The +14V rail and PVDD end up at about +5V.

**Solved in rev A2:** the ideal diode U18 (LM66100DCK) in the output of U3 blocks the path, and Q4
with R76/R77 keeps U4 disabled while +14V is present. Both supplies may now be connected at the
same time, and the +14V rail stays at 0 V while the board runs from USB. See "Back-feed from USB
into +14 V — solved in rev A2" below, which gives the evidence (working notes: A4, finding 20; E4).

## 3. Amplifier (TAS5805M)

| Measure | What | Why / evidence | Commit |
|---|---|---|---|
| B1 | `ADR/FAULT` of U14: R70 changed from 0 Ω to GNDA to **4.7 kΩ to DVDD** | Sets the I²C address, TAS5805M datasheet Table 7-5 | `8a6df77` |
| B2 | C78 at `VR_DIG` changed from 100 nF to **1 µF** | Datasheet Table 8-5 (PBTL with inductor filter) | `fbccc6c` |
| B3/B4 | Firmware only: PBTL mode, address 0x2C, `analog_gain: -12dB` | See section 8 | — |
| K1 (audit) | **PBTL output pairing corrected:** U14 pins 26 + 23 (OUT_A+/OUT_A−) now go to L6, pins 17 + 20 (OUT_B+/OUT_B−) to L7. Upstream paired A+ with B+ and A− with B−, which in PBTL mode shorts half-bridges switching in opposite phase. Schematic and tracks only, all on F.Cu; the bootstrap capacitors stay on their own OUT pin | TAS5805M datasheet Figure 8-9, "Mono (PBTL) System Application Schematic", p. 84 | see git log |
| M8 (audit) | Speaker path L6/L7 → C86/C87 → J5 and the joining tracks at L6/L7 widened from 0.5 to **0.8 mm**; the B.Cu bridge of `Net-(C86-Pad1)` now uses **three vias at each end** instead of one | IPC-2221: 0.8 mm × 35 µm carries 2.03 A at 10 K rise; the speaker draws 1.31 A RMS with `analog_gain: -12dB` and 2.26 A unthrottled (12.6 K rise) | see git log |

## 4. Microphones, touch and antenna

| Measure | What | Why / evidence | Commit |
|---|---|---|---|
| D2 | MK3, C73, R61 set to DNP; R64 stayed populated. | The third microphone is not used: its data line is re-used by C4. (An earlier version of this file said R64 keeps U12 `1OE` at a defined level. That is wrong: R64 sits between `VDD_MIC` and MK3 `SELECT`, and U12 pad 1 had no copper. Fixed by B6.) | `ad378c0` |
| E5.1 | **The whole MK3 branch is now gone**: MK3, C73, R61, R51 and R64 are removed from the schematic and the board, together with their wires, three GND symbols and the label `MIC_DATA_M_2`. TP23 (`VDD_MIC`) was moved onto the copper that remains. | Leaving a populated-but-pointless branch in the design was the last reason for the two unconnected items DRC reported, and R64 fed a `VDD_MIC` stub that went nowhere. Netlist: 245 → 242 parts, 200 → 198 nets, no other net or node changed. | `5513330` |
| B6 | U12 (SN74LVC125A) pin 1 `1OE` connected to **GND** in the schematic and on the board. Pad 1 lies fully in the B.Cu GND fill that also connects pads 4 and 7, so no track is needed. Channel 1 is now permanently enabled: `1Y` = `1A` = `MUTE_ON`, a defined level on `MIC_DATA_2`, which goes to no XU316 port. | Pad 1 had no copper, so `1OE` was an open CMOS input on a populated part. The SN74LVC125A has no internal pull resistors and requires defined input levels. Built the same way as `2OE` (pin 4), which is already on GND. Netlist diff: only U12.1 `MIC_DATA_M_2` → `GND`. ERC unchanged. DRC: no new violation, unconnected items 3 → 2. | `b74d48b` |
| E1 | MPR121 touch controller (U15) connected: electrode ELEC1 and `TOUCH_IRQ` routed; the designer's copper rings around the three screw holes (B.Cu) serve as electrodes; test points TP14–TP16 placed on the board. | The touch electrodes were not connected in the layout. The schematic needed no change. | `26fb018` |
| G4 | Antenna matching network L3/L4/C28 moved next to the antenna feed. The mismatched feed line shrank from 7.47 mm to 4.29 mm (the antenna keep-out prevents going closer). One fence via removed. | Before the change, 7.47 mm of mismatched line (about 39° at 2.45 GHz) lay between the antenna and its matching network; now 4.29 mm (about 22°). | `14cee7d` |
| — | Antenna keep-out allows pads; GND via plus fill-exclusion area at the C46/C47 neck (0.072 mm copper neck removed) | DRC `items_not_allowed` / `connection_width` | `18d52cd` |

**G4b, RF trace width (`9f4e9bd`):**

* **What changed:** only the widths; no part and no trace was moved.
  * `LNA_IN` (ESP32 pin 1 → C28) went from 0.2 to **0.3 mm**.
  * The antenna feed `AE1-Pad1` (AE1 → L3) went from 0.2 to **0.4 mm**.
* **Clearance to the GND pour:**
  * Smallest gap per net: 0.1275 mm before and after (net-class clearance 0.125 mm).
  * The pour changes only at these traces: F.Cu GND −0.565 mm².
* **Checks:** DRC with schematic parity shows no new violations; ERC and the netlist are
  unchanged.
* **Impedance:** On the ordered stack-up (section 6) the 0.2 mm traces were estimated at
  about 65–73 Ω, and 0.4 mm at 50 Ω. By that estimate the feed is now close to 50 Ω, and
  `LNA_IN` at 0.3 mm is still above 50 Ω.
* **Teardrops (`11a1853`):** regenerated in the KiCad editor for the whole board.
  * The three teardrops on the RF nets now match the new widths.
  * 410 teardrops were added on 75 other nets, at pads and vias on all four copper layers
    (+49 mm² copper). None of them is on the RF nets.
  * Clearance of the RF nets, including their teardrops, to the GND pour stays 0.1275 mm.
  * DRC with schematic parity shows no new violations.

### The LED data line now has a 5 V level shifter (finding 10)

**What changed (F7.3):** a single buffer **U19 (74AHCT1G125GW, SOT-353-5, LCSC C12495)** sits
between the ESP32-S3 and the first LED. `OE` is tied to GND, `VCC` runs from +5V with a 100 nF
capacitor **C104** (LCSC C1525) 2.00 mm from the VCC pad. The old net `ESP_WS2812` was split into
`LED_DATA_3V3` (U8.27 → U19.2) and `LED_DATA_5V` (U19.4 → LED1.3).

**Why.** The LEDs run from +5V, and the SK6805/SK6812 family specifies `VIH` as **0.7 × VDD**, so
**3.5 V** at a 5 V supply. The ESP32-S3 guarantees only `VOH` ≥ **0.8 × VDD = 2.64 V**. Driving
`DIN` straight from a 3.3 V GPIO is therefore outside the LED's specification — it usually works
because the real threshold sits lower, but nothing in either datasheet promises it, least of all
across temperature and part spread. The 74AHCT1G125 closes both ends of the chain:

| Stage | Value | Requirement | Margin |
|---|---|---|---|
| ESP32-S3 `VOH` min → U19 `A` | 2.64 V | AHCT `VIH` max **2.0 V** | +0.64 V |
| U19 `Y` `VOH` min → LED1 `DIN` | **4.4 V** (VCC 4.5 V, I_OH −4 mA) | SK6812D `VIH` 3.5 V | +0.9 V |

The AHCT family is the right one here precisely because its inputs are TTL-compatible (`VIH` 2.0 V
instead of the 0.7 × VCC = 3.5 V an AHC or LVC part at 5 V would demand), so a 3.3 V logic level
drives it with margin while the output swings to the full 5 V rail.

**Deviation from Gen1 and the Voice PE.** Both drive their LEDs directly from a 3.3 V GPIO and work.
That is evidence the arrangement often functions, not that it is within specification; this board
keeps the shifter because the cost is one SC-70 part and one capacitor, and the first LED is the one
whose colour fidelity a marginal level would degrade first.

**Routing.** The outer layers have no corridor between U19 and U8, which is why the original line
already ran on In1.Cu. So the inner-layer run was brought to U19 instead of being detoured on the
LED side: the old 27.15 mm diagonal on In1.Cu was replaced by 15.27 mm (`LED_DATA_3V3`, from the U8
side) and 20.75 mm (`LED_DATA_5V`, on to LED1), with one via each next to the pads (T1
116.762/84.450, T2 120.138/83.900) and 1.61 mm of new copper on B.Cu. The alternative — cutting the
diagonal at its closest point to U19 — was found and rejected: it needed 36.0 mm of extra B.Cu track
across the LED side.

**Placement.** U19 sits at **118.450/84.450 on B.Cu** (LED side), 8.26 mm north-west of LED2,
8.84 mm from the nearest screw-hole edge and 7.44 mm outside the ELEC0 touch ring. The operator
confirmed on the sample that the clearance there — next to the shielded Google part — takes the
1.1 mm package. Sketch: `pics/u19_lage.svg`, drawn as seen when looking at the LED side.

**Checks.** ERC 72 (unchanged). Netlist: 241 → **243** parts, 201 → **202** nets, and per-pin the
diff shows only the five lines above. DRC with schematic parity **33 violations, 2 unconnected,
0 parity problems** — the same as before the change. Copper graph: both new nets form one component
each, no dangling ends. Zone islands 0. Clearances of the new copper ≥ 0.127 mm. Teardrops are
unchanged at 1051; the five new vias get theirs when the board is next opened in the editor.

### Why LED1 and LED6 sit on the microphone side, and stay there

The two edge LEDs on the tabs (`SK6812D-EC3210R`, side-emitting) are on **F.Cu**, the microphone
side, while the four ring LEDs are on B.Cu. That looks inconsistent, and an early prototype photo of
the designer's suggested it might be an oversight — in that photo every LED was still on one side.

It is not an oversight. On a dismantled H2C the original's **volume LEDs also sit on the underside,
the microphone side, and emit sideways outward**, into the light guides beside the two touch areas.
LED1 and LED6 reproduce that. Their purpose is the volume indication: the firmware turns `left_led`
(index 0 = LED1) and `right_led` (index 5 = LED6) on while the corresponding touch area is held.
LED6 is also the link that carries the chain onward — its output goes through R80 to J6.5 and the
LED ring board — so removing the two would mean re-wiring the chain end and shifting every LED index
in the firmware.

**Orientation checked.** The light has to leave toward the board edge, not toward the middle. The
part is a side emitter (LCSC C2890041: "SMD-4P 3.2 × 1.0 × 1.08 mm", mounting **side view**), and its
footprint marks the emitting face: `Onju:LED-SMD_4P_SK6812D-EC3210R` carries a closed silkscreen
rectangle from (−1.000 / 0.681) to (1.000 / 1.100), running the full length of the part and lying
outside its 1.0 mm wide body; the top-emitting footprint `Onju:SK6805-EC20` used for the ring has
no silkscreen at all (the footprint keeps that name; the part in it is now the SK6812-EC20,
which has the same outline and the same pad positions). Solving the part-to-board transform from the measured pad positions — not from an
assumed rotation convention — and checking it against all four pads gives:

| | Rotation | Emitting face points | Part lies | Result |
|---|---|---|---|---|
| LED1 | −90° | west (−x) | west of centre | **outward** |
| LED6 | +90° | east (+x) | east of centre | **outward** |

Both are correct, and the two are exact mirror images of each other. The one thing not proven from a
document is that the silkscreen rectangle really is the lens: the package drawing in the datasheet is
an image, and its text could not be extracted. Lighting LED1 on the sample settles it in seconds.

### What the LED ring draws from +5V (rev A2)

With LED2–LED5 changed to the SK6812-EC20, all six LEDs draw the same **12 mA per colour**; the
SK6805 was a 5 mA part, so the ring used to be visibly dimmer than the two edge LEDs.

| Item | Was | Now |
|---|---|---|
| LED2–LED5 | 4 × 3 × 5 mA = 60 mA | 4 × 3 × 12 mA = **144 mA** |
| LED1, LED6 (SK6812D-EC3210R) | 72 mA | 72 mA |
| TAS5805M `DVDD` through U7 | not counted | + 13 mA |
| **+5V total** | about 550 mA typical, 700 mA peak | **about 647 mA typical, 800 mA peak** |

U18 (LM66100, 1.5 A) and U3 (TPS62130, 3 A) still have plenty of margin, and the drop across U18
stays at about 63 mV.

**One thing to know when you power it from USB:** at full brightness the board pulls up to 0.8 A
from VBUS plus converter losses, so a plain 500 mA charger will not do. The receptacle is wired for
the full current anyway (R1/R5 = 5.1 kΩ), so any supply that can deliver 1 A or more is fine.

## 5. Footprints and fabrication rules

| Measure | What | Why | Commit |
|---|---|---|---|
| F2 | J1 (USB-C) SMD pads: `solder_paste_margin` changed from −100 to 0 | The 24 pads would have received no solder paste | `ba1286a` |
| F3 | J1 shield pads renamed from `"0"` to SH1–SH4 on GND | The shield had no net. This removes 4 parity errors. | `1d4c356` |
| F4 | Three extra 0.85 mm Edge.Cuts circles over the microphone ports removed | The footprint already has the 0.5 mm port; TDK T3902 DS-000357 p. 18 recommends 0.5–1 mm | `8b8205a` |
| — | U6 now uses the KiCad standard footprint `Texas_DSG0008A` | Part of A1 | `3ff45d1` |
| V2 | **Thermal holes enlarged to 0.3 mm, pads to 0.6 mm**; positions unchanged. U14: 24 pads 0.2 / 0.3 mm → **0.3 / 0.6 mm**. U3, U7: 4 pads each 0.2 / 0.5 mm → **0.3 / 0.6 mm**. The annular ring is now **0.15 mm**, the JLCPCB minimum for plated component holes (multilayer, 1 oz). The smallest hole on the board is now 0.3 mm. | 0.2 mm holes need a special drill option and cost more. These holes are marked as component holes in the drill file, and for those JLCPCB requires a 0.15 mm annular ring. Ring to ring: U14 0.60 mm, U3/U7 0.58 mm. Ring to copper of other nets: F.Cu ≥ 0.169 mm (U3/U7 pad 1), In2 0.50 mm (+5V plane), In1/B.Cu ≥ 1.0 mm. DRC with parity shows no new violations; ERC and the netlist are unchanged. Fill changes only at U14, U3 and U7. | `9d27103`, `3fd705f` |
| V3 | **Three tap vias moved out of their pads**, positions of the parts unchanged. R23.2 (`I2S_MCLK`) 129.900 / 73.200 → **129.400 / 74.271**; R29.2 (`I2S_LRCK`) 126.700 / 75.500 → **126.880 / 75.341**; R30.2 (`I2S_BCLK`) 127.500 / 75.500 → **128.060 / 75.831**. Each gets a 0.2 mm stub from the pad centre, and the track that continues on the other layer follows to the new via (R23 on In2.Cu, R29 and R30 on B.Cu). Since E5c-P5 R23 no longer exists; its via at 129.400 / 74.271 stays and remains part of the `I2S_MCLK` path (X1D11 to R24/R31), the 0.2 mm stub went with the pad. The three old 0.15 mm stubs and nine teardrops at the old positions were removed. | These three vias came from this fork (C1, C2 and C4). Their rings overlapped the pad copper and their drilled holes sat inside the pads' solder-mask openings, which a Eurocircuits DFM run reported as "Bohrungen in SMD". Via edge to its own pad is now 0.134–0.454 mm, smallest clearance to other copper 0.132 mm. DRC with schematic parity is unchanged; netlist identical, stub test unchanged. Pairings "hole inside a mask opening": 31 → 28. | `f0d524f` |
| V1 | **Five vias moved out of small pads** and connected with a 0.2 mm stub from the pad centre. U6 pad 1 (GND) 136.500 / 78.200 → 137.370 / 78.070; U9 pad 4 (GND) 130.788 / 58.395 → 131.907 / 58.095; C13 pad 2 (GND) 136.000 / 82.800 → 135.330 / 82.633; C70 pad 2 (GND, bottom) 131.500 / 80.350 → 131.060 / 80.180; C100 pad 1 (+0V9) 133.000 / 82.400 → 133.600 / 82.800, with the B.Cu track from 132.0 / 82.4 extended to the new via. The old in-pad stubs of C13 and C100 and three via teardrops at the old positions were removed. | A via in a small pad needs resin filling and a copper cap. The U9 via came from upstream, the other four from this fork (A1, `9f279f2`; C70 in `0dcb0e5`). Checked against the current board: no via is left in any SMD pad. Via edge to its own pad is 0.127–0.595 mm, to copper of other nets ≥ 0.127 mm, and the four GND vias connect to the In1 GND plane. DRC with parity shows no new violations; ERC and the netlist are unchanged. All fill changes lie within 0.7 mm of an old or new via position. Vias in SMD pads: 31 → 26. | `07ece05` |
| — | L6/L7 footprint changed from `XAL4040-103MEC` to `MTQH404030S100MBT`: lands 1.6 × 4.1 mm at ±1.35 mm; tracks unchanged | Replacement inductor, see section 7 | `f4fd33f` |
| — | DRC severities `lib_footprint_issues` / `lib_footprint_mismatch` set to *ignore*; DRC exclusions set for the electrode rings and J3/J4 | The upstream footprint libraries are not in the repository | `18d52cd`, `6836fe7` |

**Checked, no change needed: inner annular ring (IAR).** A Eurocircuits check run on the rev A1
data reported: *"Der gemessene Wert für Innenlagenrestring (IAR) (0,000 mm) stimmt mit keiner
der verfügbaren Optionen überein."* We measured every one of the 438 holes against all copper
on In1 and In2 (zone fills, pads, vias, tracks, teardrops):

| | In1.Cu | In2.Cu |
|---|---|---|
| 397 vias (0.3 / 0.4 mm) | 0.05 mm | 0.05 mm |
| U14 thermal holes | 2.52 mm | 0.149 mm |
| U3 / U7 thermal holes | 0.204 mm | 0.149 mm |
| J1 (4 slots) | 0.394 mm | 0.200 mm |

* **No hole meets inner copper without a ring.** The smallest inner ring is 0.05 mm, at the
  vias, on both inner layers.
* The **non-plated** holes (MK1 and MK2 at 0.5 mm; MK3 and U1 are gone since E5.1 and E1) have no copper
  at all on the inner layers; the nearest copper is 0.127 mm away. They are in their own file
  `…-NPTH.drl`, which carries the Gerber attribute `NonPlated`.
* No pad and no via uses "remove unused layers"; every one keeps its ring on all four layers.
* **This is not a design error.** The reported 0.000 mm does not match any hole in the data.
  The most likely reason is that the non-plated holes were measured as if they were plated,
  which happens when the NPTH file is not recognised as non-plated on upload.

**Checked, no change needed: C13 and C24 (Eurocircuits DFM).** The same check run reported
*"Bohrungen in SMD"* at C13, C30 and C38, and *"Bauteil möglicherweise außerhalb der Kontur"*
at C24.

* **C13 is clear since V1.** The via sits at 135.330 / 82.633, outside the pad: 0.145 mm from
  the pad copper and 0.115 mm from its solder-mask opening; the drilled hole is 0.195 mm from
  the copper and 0.165 mm from the opening. In `production/` (before V1) the via was still
  inside the pad, which explains the message if that data set was checked.
* **C24 is well inside the outline.** Its pads and mask opening are 8.53 mm from the nearest
  `Edge.Cuts` element, its courtyard 8.23 mm. The part sits at 116.400 / 81.000 and the board
  spans 98.6 / 54.7 to 173.1 / 125.0, 3212.6 mm².
  * `Edge.Cuts` holds the outline plus **four inner cut-outs** (18.9 mm² at 145.9 / 72.6,
    10.8 mm² each at 107.0 / 87.2 and 161.0 / 87.2, 9.1 mm² at 134.1 / 114.2). The nearest one
    is the 8.23 mm away. A checker that takes one of these as the board outline reports most
    parts as outside; that would explain the message, but we could not confirm it.
* **C30 has no via in the pad.** The nearest via is 0.090 mm from the pad copper, its hole
  0.140 mm; both vias belong to the same net and come from upstream.
* **C38 / C39:** see "Via holes inside a pad's solder-mask opening" under the known issues.

### When ordering: the board needs 0.10 mm minimum track width

Two signals, `UART_TX` and `UART_RX`, use the net class **`UART-eng`** with a track width of
**0.100 mm**; everything else stays at 0.150 mm and the clearance is 0.127 mm throughout. The
narrow sections are what makes the expansion connector J6 reachable at all (see "A track ran
lengthwise under the whole pad row of J6" under the known issues). A rule file
`nest-mini-v2-drop-in-pcb.kicad_dru` relaxes `track_width` and `connection_width` for this net
class only, so the DRC checks the rest of the board against 0.127 mm as before.

**JLCPCB fabricates this without a surcharge**, both pages retrieved 30 Sep 2026:

* `jlcpcb.com/capabilities/pcb-capabilities` — multilayer, 1 oz outer copper:
  "0.09 / 0.09 mm (3.5 / 3.5 mil)".
* `jlcpcb.com/help/article/in-what-cases-will-there-be-charged-extra` — extra cost only "if the
  trace width or spacing of multi-layer boards is 3.0-3.5mil" (20 % of the order amount for
  4–8 layers). **0.10 mm is 3.94 mil**, above that threshold; the margin to it is 0.011 mm.

**With any other fabricator, check this first.** A house that quotes 4 mil (0.1016 mm) as its
minimum would reject or surcharge these two tracks. Should that happen, the fix is not to widen
them — the channel to J6 does not fit two 0.15 mm tracks — but either to widen the channel (move
the ELEC2 electrode ring or the screw hole at 162.815 / 89.076) or to accept one of the two UART
signals staying unconnected, as rev A2 stood before this change.

### When ordering: the XMOS flash U11 stays at 4 MB (W25Q32JVSSIQ)

U11 is a **W25Q32JVSSIQ**, 32 Mbit = **4 MB**, LCSC C179173, 8-pin SOIC 208-mil. An 8 MB part was
considered and rejected. The reason is worth recording, because the three boards this design takes
its cues from do not agree with each other:

| Board | Flash at the XU316 | Size | Source |
|---|---|---|---|
| **Gen1** (`home-mini-DiP-v1`) | W25Q32JVSSIQ | **4 MB** | U9 on the XMOS sheet; `pruefung/gen1.net` puts U9 on `/XMOS/QSPI_CLK`, `QSPI_CS_N` and `QSPI_D0…D3`, so it is the XU316's boot flash (in Gen1 the XU316 is U8) |
| Satellite1 rev 6.1 | W25Q64JVSSIQ | 8 MB | `FutureProofHomes/Satellite1-Hardware`, `hat/rev6.1hatSCH.pdf`, U18, next to U17 `XU316-1024-QF60B-I32` |
| Voice PE v1.0 | ZB25VQ128DSJG | 16 MB | `home_assistant_voice_pe_schematic_v1.0_241009.pdf`, U15 |
| **this board, rev A2** | **W25Q32JVSSIQ** | **4 MB** | — |

**Why 4 MB and not more.** Two independent pieces of evidence point the same way:

* **The firmware says 4 MB.** The FFVA firmware this board runs describes its own target in
  `src/ffva/bsp_config/NC-VOICE-KIT/NC-VOICE-KIT.xn` (`esphome/voice-kit-xmos-firmware`, tag
  `v1.3.1`):

      <Device NodeId="0" Tile="0" Class="SQIFlash" Name="bootFlash" Type="S25FL116K"
              PageSize="256" SectorSize="4096" NumPages="16384">

  16384 pages × 256 bytes = **4 194 304 bytes = 4.00 MiB**. That is the geometry `xflash` is given
  when the image is built, and the boot partition the build system asks for is 0x100000 = 1 MiB. So
  the firmware is not merely *able* to live in 4 MB — 4 MB is what its target description declares,
  even though the Voice PE hardware it was written for physically carries 16 MB.
* **Gen1 proves it in hardware.** Gen1 runs this firmware family on exactly this 4 MB part, and
  Gen1 is built and shipped. For the 8 MB part there is no such proof on an XU316 of *this* variant:
  Satellite1 uses it, but with an `XU316-1024-QF60B-I32`, a different order grade.

Going to 8 MB would therefore have bought a margin nothing asks for, at about $0.55 more per piece,
and would have replaced a part proven in hardware with one that is not. If more room is ever
needed — a second firmware image, a larger data partition — `W25Q64JVSSIQ` (LCSC C179171) is the
drop-in: same package code `SS`, same `IQ` order suffix (QE bit factory-fixed to 1, so IO2/IO3 are
quad pins from power-up), tVSL 20 µs, Fast Read Quad I/O `EBh` (Winbond W25Q64JV, Rev J,
2018-03-27). Only the device ID changes with the capacity, so a boot image that hard-codes it must be
rebuilt.

**Measured.** The factory image was built with XTC Tools 15.3.1 and is **286 720 bytes
(280.0 KiB)**:

    xflash --factory ffva_v1.3.1.xe --boot-partition-size 0x100000 -o ffva_v1.3.1_factory.bin

That is **27.3 % of the 1 MiB boot partition and 6.8 % of the 4 MiB flash**, which settles the
capacity question by measurement and not by inference. The image comes out byte-for-byte identical
whether `--target-file NC-VOICE-KIT.xn` is given or not, whether the XN names `S25FL116K` or
`W25Q32JV`, and whether the boot partition is declared as 0x100000, 0x80000 or left out — the target
description is compiled into the `.xe`, and the boot-partition size is metadata rather than padding.

**The toolchain knows our part by name.** `W25Q32JV` is one of the 15 devices natively supported by
`libquadflash` (`target/include/QuadSpecEnum.h`, `fl_QuadFlashId`: `WINBOND_W25Q32JV = 13`, together
with W25Q16JV, W25Q64JV, W25Q128JV, three Spansion and eight ISSI parts; also listed in the tools
documentation under "List of devices natively supported by libquadflash"). So no `--spi-spec` file is
needed and nothing depends on SFDP auto-detection — `--noinq` exists to suppress the device query,
and `--spi-spec` to describe a part that is not on the list. Neither applies here.

Writing it to the board still needs the XTAG4 on J4; `xflash --list-devices` reports "No Available
Devices Found" until one is attached.

**The image is in the repository:** `firmware/xmos/ffva_v1.3.1_factory.bin`, with its checksums in
`MD5SUMS`, the exact build command and provenance in `firmware/xmos/README.md`, and a copy of the
XMOS Public Licence v1 alongside it. That licence covers compiled code only under its clause 2.2, so
the directory carries the source notice its clause 2.3 requires (where the source is, that nothing was
modified) and the clause 2.2.2 condition is met because the code runs on an XU316. Nothing in the
firmware was changed — the release `.xe` was only repackaged by `xflash`, and since that build is
deterministic the file can be reproduced from the public release asset at any time.

## 6. Board and silkscreen

* **Stack-up set to JLCPCB JLC04161H-7628** (`ba55b9f`). The board is 1.6 mm, 4 layers:
  prepreg 0.2104 mm (εr 4.4), core 1.065 mm (εr 4.6), inner copper 0.5 oz. The previous
  values were the KiCad defaults. The surface finish is not set in the project and is chosen
  at order time.
* **Silkscreen** (`6e97dd7`, and E5b.5 in `433136d` / `18f93b1`):
  * The upstream "MiciMike" logo and board titles were removed from both sides.
  * "CERN-OHL-S-2.0" and the USB flashing note were kept.
  * B.Silkscreen now carries: *"based on MiciMike Nest Mini v2 by iMike78 · CERN-OHL-S 2.0 ·
    modified"*, plus **`rev-a2` and the date**, once on each side.
  * **All 23 silkscreen DRC warnings are gone** (E5b.5). They were worked off one at a time and
    each step is logged: pin-1 and polarity marks were **moved**, never deleted, and only inside
    the sector of their present bearing from the part centre, so they still say what they said
    (U14, MK1, X1, POWER_IN1, U11); outline segments were **shortened from the offending end** in
    0.05 mm steps until clear, so they stay recognisable as an outline (SW1 twice, SW2 twice, L8);
    reference fields were offset; five orphaned digits on B.Silkscreen ("4", "1", "10", "1", "9")
    were removed — they were the pin numbers of J3 and TP10, which E3 took out, so they labelled
    nothing.
  * **Pin-1 marks** are present on J1, J4, J5, J6 and POWER_IN1. There are **no pin numbers** next
    to the connectors: at 0.5 mm pitch they would not be legible, so the pinouts live in the
    README instead.

### Test points (rev A2)

Rev A1 had eleven test points, several of them labelled only "TestPoint". Rev A2 brings the count
to **eighteen**, all bare copper with nothing to populate, and gives every one a short name on the
silkscreen next to the pad (the full net name does not fit on a 68 mm board):

| New | Net | Why |
|---|---|---|
| TP17 | `+5V` | **behind** the ideal diode U18 — together with TP3 in front of it, this is the pair that shows whether U18 works: about 55…65 mV of difference under load, and TP3 near 0 V on USB alone |
| TP18 | `+14V` | the rail from the base plate, at the input side |
| TP19 | `ESP_RST` | reset of the ESP32-S3 |
| TP20 | `ESP_BOOT` | boot mode of the ESP32-S3 |
| TP21, TP22 | `GND` | a reference next to U8 and one next to U14, so a probe does not need a long return path |
| TP23 | `VDD_MIC` | the 3.3 V microphone supply from U13, on the copper that remained after the MK3 branch came out |

The short names are **in the value field**, not in an extra text item, so there is one source for
them and schematic parity stays intact. TP3 was renamed `5V_SW` to match its net. The three touch
electrodes TP14–TP16 carry **no** silkscreen: they sit inside the electrode rings on the LED side,
where any lettering would land on copper. The full table with positions and expected readings is in
the README. I²C and SPI test points were considered and dropped — the buses are reachable at the
parts themselves, and the board has no room to spare.

### Touch electrode rings pulled back from the hole edge (E5b.2)

The three copper rings around the screw holes serve as the touch electrodes. In rev A1 they ran up
to the hole edge, which produced three `copper_edge_clearance` findings that had to be **excluded**
from DRC with a stated reason. Rev A2 pulls the inner edge of each ring back to **at least 0.25 mm**
from the hole edge. The electrode area is otherwise unchanged, so the capacitance seen by the MPR121
barely moves; the three exclusions are gone, and DRC now passes these rings without being told to
ignore them (`38aad27`).

### The land pattern of L6 and L7 against the manufacturer's datasheet (E5b.3)

L6 and L7 are the output inductors of the amplifier, changed in rev A2 from the Coilcraft
XAL4040-103MEC to the MetalLions MTQH404030S100MBT (C51883185) for cost. The land pattern had been
taken from the LCSC footprint, which is not a source. It was checked against the MetalLions
datasheet and found correct; the silkscreen outline around both parts was cleaned up at the same
time (`77439a9`).

The DC resistance matters here because it sits in the speaker path: the datasheet gives **92 mΩ
typical and 110 mΩ maximum** for the S100 variant. An earlier reading of 190/220 mΩ was a
transcription artefact — `pdftotext` had shifted the columns of the MetalLions catalogue.
* **Zone refills** (`3c7bb45`, `c4c0cf8`, `6cf4fc1`, `6836fe7`, `5e09946`, `289454b`,
  plus earlier ones in C and B) change fill polygons only.

## 7. Bill of materials

All populated parts carry an LCSC part number: **84 unique parts, 221 placements**, 814 solder
joints, 153 parts on F.Cu and 68 on B.Cu. Twenty-one items are marked DNP: the eighteen test
points, the J4 pad field and the two PG pull-ups R10/R16.

**Six gaps were found and closed in E7**, all of them in parts that rev A2 itself added — they
had their part number in the wrong field, or not at all, so the BOM and CPL exporters did not see
them. Each number was checked on its LCSC product page (1 Oct 2026) and against the footprint in
the layout:

| Part | What was wrong | Now |
|---|---|---|
| **R80** (100 R, LED data line to the ring board) | `LCSC Part #` held **C25744**, which is a **10 kΩ** resistor, and the MPN field said `0402WGF1002TCE` — also 10 kΩ. The value field said 100 R. | **C25076** (`0402WGF1000TCE`, 100 Ω ±1 %, 0402). Had this gone to fabrication, R80 would have been fitted with 10 kΩ and the data line to the ring board would not have worked. |
| **U18** (ideal diode) | no part number in any field | **C2869734** (LM66100DCKR, TI, SC-70-6) |
| **U19** (level shifter) | number only in a field named `LCSC`, which no exporter reads | **C12495** (74AHCT1G125GW,125, Nexperia, SOT-353) |
| **Q4** (USB lock-out) | number only in `Supplier Part` | **C383201** (LBSS138WT1G, LRC, SC-70; 50 V, 200 mA, V(GS(th)) max 1.5 V — the value the divider R76/R77 was dimensioned against) |
| **C104** (U19 decoupling) | number only in a field named `LCSC` | **C1525** (CL05B104KO5NNNC, 100 nF 16 V 0402; it sits on +5V, so 16 V is ample) |
| **R79** (10 k) | correct in the schematic, missing in the layout | **C25744** in both |

Changes compared with `7de6eec6`:

| Parts | Change | Why | Commit |
|---|---|---|---|
| U1, U2, U11, U14, C74–C77, C80, C82, C90, C91, R2–R4, R6, R66, R67 | Part numbers added (no number upstream) | Needed for assembly; each number was checked on the LCSC product page against value, package and rating | `7a514f0`, `a5ecf2b` |
| C86, C87 | Part number added: C107133 (680 nF 50 V X7R 0805) | The design asks for 100 V, which does not exist in 0805. 50 V is 3.5 × PVDD (decision) | `18da934` |
| R68, R69 | Changed from C17168 (0402) to **C17477** (0805 jumper, 2 A) | Package mismatch; these are the GND–GNDA links carrying amplifier return current | `309fdba` |
| R13 | Changed from C3016445 (4 in stock) to **C861392** (YAGEO 40 kΩ 0.1 %) | Availability. U3 output unchanged: 0.8 V × (1 + 210 049.9 / 40 000) = 5.001 V | `3fdd26e` |
| LED2–LED5 | Changed from C2890036 (**SK6805-EC20**) to **C2909058** (**SK6812-EC20**) | Availability (115 in stock against 37 174) and equal brightness across all six LEDs. Both types are 2 × 2 × 0.65 mm with the pads in the same places, so the footprint, the placement and the CPL rotation are untouched — only the part number changes. Costs 84 mA more on +5V, see below | this commit |
| L6, L7 | Changed from Coilcraft XAL4040-103MEC (about 8.6 $ each) to **MetalLions MTQH404030S100MBT** (C51883185): 10 µH, Isat 4.9 A, 92 mΩ | Cost. The firmware limit `analog_gain ≤ −12 dB` keeps the peak current at about 1.85 A. The land pattern was taken from the LCSC footprint; **the manufacturer datasheet still needs to be verified.** | `f4fd33f` |
| C1/C83/C88, C6/C17/C80/C90, C12/C64, C79/C81/C84/C85, R8/R9/R15/R41/R75, R73 | Replaced by JLCPCB *basic* parts of the same value and package, with at least the same voltage and equal or better tolerance. C6/C17/C80/C90 and C12/C64 change from X7R 10 V to **X5R 16 V**. | Fewer extended-part setup fees | `0f547c6` |
| Values | C13 2.2 → 10 µF, C18 4.7 → 10 µF, C20 100 nF → 10 µF, C78 100 nF → 1 µF, R7/R14 100 k → 0 Ω, R70 0 Ω → 4.7 k; new: L8, R74, R75, C100 | Sections 2 and 3 | see above |
| DNP | R10, R16 (the PG pull-ups). MK3, C73 and R61 were DNP in rev A1 and are now **removed** (E5.1) | Sections 1, 2 and 4 | see above |
| Removed | R23, R25, R26 (were DNP), J3, TP10, **J2**, **U1/U2 and their parts** (E1), **MK3/C73/R61/R51/R64** (E5.1), **R37/R38** (E5.4), **C2** (E5.5) | Sections 1, 2 and 4. C2 was the 100 nF decoupling capacitor of U2's VCC; U2 went out with the USB switch-over in E1, so C2 had been sitting on a net with nothing to decouple. The part count is now **242** with **198** nets. | see above |

**Kept deliberately:**

* **J1** (Molex 1054500101, C134092). No stocked USB-C receptacle fits the footprint.
  C134092 is a stocked part, but JLCPCB flags it as "process difficult" in the BOM check,
  where it has to be confirmed by hand.
* **L1/L2** (Bourns SRP4020TA on a Cenker footprint), as fabricated by the upstream designer
  since 2025.
* **C83 and C88 keep C307331** (Samsung CL05B104KB54PNC, 100 nF **50 V** X7R 0402) instead of
  being merged into C1525, the part used for the other 41 100 nF capacitors. C1525 is
  CL05B104KO5NNNC and rated **16 V**, while C83 and C88 sit on `PVDD` / `GNDA` — the
  TAS5805M supply, fed from the +14 V input through R66. 14 V on a 16 V part is 88 % of the
  rating; it breaks rule A3 above ("all input capacitors on +14 V / PVDD are rated ≥ 25 V"),
  the condition under which these two were moved to a basic part in the first place ("at least
  the same voltage"), and the decision taken for C86/C87 on the same rail (50 V, "3.5 × PVDD").
  All 41 C1525 sit on rails of 5 V or less — these two are the only 100 nF 0402 above 5 V in
  the design, which is why the second part number exists. Merging them would have saved **0.8
  cents** per board and one BOM line, and **no setup fee**: both parts are JLCPCB *basic*
  parts.

## 8. Firmware configuration (`nest-mini-v2-voice.yaml`, new file)

This file is derived from upstream `MiciMike.yaml`, which is unchanged in this repository.

* **Wi-Fi:** DHCP instead of a fixed IP. All credentials are read via `!secret` from
  `secrets.yaml`, which is not in the repository; `secrets.example.yaml` lists every key the YAML
  uses (`ssid`, `wifi_pass`, `api_key`) with placeholders — copy it to `secrets.yaml` and fill it in.
* **Touch:** `esp32_touch` replaced by the **MPR121** component (address 0x5A, channels 0/1/2).
* **I²S:**
  * `i2s_dout_pin` GPIO11 → **GPIO10**, which goes to the XU316 via R24 (see C3).
  * The ESP32 is `secondary` on both buses.
  * No MCLK pin.
* **Amplifier:** `tas58xx` in PBTL mode, address 0x2C, `enable_pin: GPIO18`,
  `analog_gain: -12dB`.
  * This gain limit **must not be raised**. The TAS5805M analog-gain scale refers to 29.5 V
    (datasheet Table 7-2). At −12 dB the peak output is 7.41 V, which gives about 1.85 A into
    4 Ω. The limit protects L6/L7 from saturation and also protects the 40 mm driver.
* **XU316 reset:** `reset_pin: GPIO46` (see C7).
* **Mute:** mute detect on GPIO1, not inverted.
* **Other:**
  * `rgb_order` replaced by `channel_colors` for ESPHome 2026.
  * `${device_name}` and `${friendly_name}` substitutions defined.
  * Five light effects were called but never defined. They are now defined: `pulse`, `slow_pulse`, `"Volume Display"` (taken from the Voice PE template), `show_volume` mapped to it, and `listening_ww` as the built-in `flicker`. The last one is an interpretation, not taken from a source.
  * Entity names translated to German.
* **Pin-audit fixes (1–2 Oct 2026), before the first power-up:**
  * **K-B1, sample rate:** the two resamplers and both media-player pipelines ran at 44 100 Hz while
    the I²S speaker is fixed at 48 000 Hz. As an I²S secondary it cannot change its rate and rejects
    the stream (`i2s_audio_speaker_standard.cpp`, lines 368–372, "Incompatible stream settings") —
    there would have been no sound. All four are now 48 000 Hz, as in the Voice PE.
  * **K-B2, LED effects:** the effects came from the Voice PE's 12-LED ring and wrote `it[0..11]`
    into a 4-LED partition; ESPHome does not check the index (`partition/light_partition.h`), so they
    wrote past the buffer. Every effect now works on `N = it.size()`, and `voice_assistant_leds` and
    `led_ring` span all six LEDs (index 0–5, left to right). Mic mute shows on the two outer LEDs,
    volume zero on the two middle ones.
  * **K-B3, LED timing:** `chipset: SK6812` gives T1H = 600 ns; the SK6812-EC20 needs ≥ 0.70 µs and
    the SK6812D-EC3210R 0.65–1.00 µs. Custom timing now: 300/900 ns for a 0, 800/400 ns for a 1.
  * **K-B4, amplifier start-up order:** `tas58xx` put the TAS5805M into PLAY (in BTL, at 0 dB) before
    it set PBTL and −12 dB. It is now loaded from a local, patched copy
    (`firmware/esphome/components/tas58xx`, upstream `89f29cf` plus two lines, see
    `firmware/esphome/PATCH.md`): the register table ends in Hi-Z, PLAY follows PBTL and the gain.
    The copy is GPL-3.0-or-later (© mrtoy-me, based on work by Andriy Malyshenko); the change is
    also prepared as a pull request upstream.
  * **K-B5, start-up after the XU316 reset:** `setup_priority: 790` for the amplifier, below
    `voice_kit` (799), so PDN, configuration and PLAY follow the XU316 reset; the TAS5805M waits in
    Hi-Z for the clocks (datasheet 7.3.4). **The transition to PLAY is coupled to the XU316 being
    ready** (2 Oct 2026, review point B5-B2): ESPHome's `Application::setup()` (2026.9.0,
    `core/application.cpp` lines 64–65 and 78–102) sets components up in priority order and waits after
    each one until its `can_proceed()` is true; `VoiceKit::can_proceed()` (`voice_kit.h` line 127) is
    true only once the XU316 firmware version has been read over I²C and matches (about 3 s after the
    reset, or after a DFU update). The amplifier, at priority 790, is therefore configured only when
    FFVA is running and driving BCLK/LRCLK; until then R71 holds /PDN low and the outputs are
    high-impedance. That is the order of datasheet 7.5.3.1 (PDN, 5 ms, stable clocks, Hi-Z, DSP,
    PLAY). Only if `voice_kit` fails (no XU316) does set-up continue without clocks; the device then
    stays in clock-error Hi-Z (7.3.4). Checked at bring-up: register 0x71 = 0 after start, sound,
    and the `tas58xx` set-up logged after `[voice_kit] DFU version`.
  * **Amplifier faults:** the ADR/FAULT pin is not wired to the ESP32, so the fault registers are read
    over I²C with the `tas58xx` binary sensors (any fault, over-current and DC fault per channel, PVDD
    under/over-voltage, over-temperature warning and shutdown).
  * **Comments corrected:** the PLAY order, the `tas58xx` licence note (its README names GPL-3.0 code
    as the basis), `mixer_mode` (only written when EQ entities exist), and a note that GPIO3/GPIO4
    must stay unused because they share the XU316 QSPI bus.
* **Decisions of 2 Oct 2026 (commits `d1a3583`, `c6b3fb3`; see "Every open point decided"):**
  * **K-B8, sounds pinned:** the 16 sound files load from Voice PE commit `d4e6fa43` instead of the
    moving `raw/dev` branch. ESPHome checks no hash for `files:`; the commit fixes the content (all 16
    byte-identical with the earlier build).
  * **K-B9, models pinned:** `hey_jarvis`, `hey_mycroft` and the VAD model load from
    `esphome/micro-wake-word-models` at commit `40ff33f`. `okay_nabu` and `stop` exist only as release
    assets that GitHub reports as not immutable; their target sha256 values are noted in the YAML.
  * **K-B10:** `min_version: 2026.9.0` — the version every fix above was checked with (as in the
    Voice PE).
  * **K-B14:** the voice assistant takes microphone channels 0 and 1, as the Voice PE does; Home
    Assistant can then stream channel 1 (AEC + IC + NS, no AGC) to STT services that ask for it.
    Cost: 16 384 bytes of PSRAM.
  * **U8-B6:** a comment lists the GPIOs that are wired on the board but deliberately unused
    (GPIO3/4 on the XMOS flash bus, GPIO39/40 behind U16, GPIO9/11/12/48 on J6).
  * **U8-B12 / M13:** `psram: enable_ecc: true` is prepared as a commented-out option (85 °C instead
    of 65 °C ambient, 7.5 instead of 8 MB usable), and a diagnostic `internal_temperature` sensor
    feeds the temperature measurement M-T at bring-up.
  * **U14-B2:** the `tas58xx` register table sets **0x53 = 0x60**, class-D loop bandwidth 175 kHz for
    768 kHz switching (SLASEH5D table 7-27), written in Hi-Z before PLAY (`firmware/esphome/PATCH.md`).
* **Build check:** compiles with ESPHome 2026.9.0 / ESP-IDF 5.5.5 (after the decisions of 2 Oct 2026:
  RAM 43.0 %, flash 28.1 % of one 8 MB OTA slot, image 2 283 639 bytes); warnings only from
  third-party code and the deliberate strapping pin GPIO46.

## 9. Production data (`production/`)

This folder is new. See `README.md`, section "This fork".

* **`production/jlcpcb/`**
  * `gerber_jlcpcb.zip`: 4 copper layers, masks, paste, silkscreen, outline, Excellon PTH/NPTH.
  * `BOM_JLCPCB.csv`: 83 rows, one row per LCSC number.
  * `CPL_JLCPCB.csv`: 221 placements.
  * `cpl_corrections.csv`: the 19 corrections.
  * `ROTATION_CHECK.md`: the target state for the preview check.

  The CPL follows the Fabrication Toolkit logic. 19 rows are corrected against the EasyEDA
  footprints JLCPCB uses: U14, U6, U3, U7, C89, U4, U5, U12, U13, U15, Q2, U2, J5 and U1 for
  rotation, and J1, X1, LED1, LED6 and POWER_IN1 for position.
* **`production/pcbway/`**
  * `BOM_PCBWay.xlsx` / `.csv`: manufacturer and MPN.
  * `positions_kicad.csv`: plain KiCad export (`--exclude-dnp`), the 221 populated parts; no
    JLCPCB corrections are applied.
  * `assembly_drawing.pdf`: top view, and bottom view mirrored.
  * `ASSEMBLY_FORM_COUNTS.md`: figures for the assembly form.

**Rev A1 production data** (after B6, G4b, the teardrop update, V1, V2 and V3) is in a separate folder,
**`fertigung_rev_a1/`**. The files in `production/` are left as they were, for the state
before B6.

* `fertigung_rev_a1/jlcpcb/gerber_jlcpcb.zip` was generated from the zone-filled board with the
  same options as before. Compared with `production/`:
  * **All four copper layers** change, because of the board-wide teardrops, the moved vias
    (V1, V3) and the larger thermal holes (V2).
  * **F.Cu** also changes at the widened RF traces (G4b).
  * **B.Cu** also changes around U12, where pad 1 now joins the GND fill (B6).
  * The PTH drill file changes, because eight vias moved (V1, V3) and 32 thermal holes are now
    0.3 mm (V2). Masks, paste, silkscreen and outline are identical apart from the date
    lines.
* `fertigung_rev_a1/pcbway/positions_kicad.csv` was regenerated (`--exclude-dnp`); its
  content is identical.
* BOMs, the JLCPCB CPL with its corrections, the rotation check, the assembly drawing and the
  form counts are copied unchanged. No part and no position changed.

## 10. Audit fixes before ordering (rev A2, 1 Oct 2026)

An independent audit of the rev A2 end state (`AUDIT_REV_A2.md`, working notes) found two
critical and several medium issues. They are fixed here, one commit each. K1 (PBTL output pairing)
is listed in section 3, K2 (J4 pin-out) under "J4 is now a pad field" below.

| Audit | What | Why / evidence | Commit |
|---|---|---|---|
| M1 | **C105, C106 (100 nF) added from AVIN to GND at U3 and U7**, on B.Cu right under each converter, each reached through one via next to the AVIN pin (pin 10). C105 is a 50 V part (`+14V`, rule A3), C106 16 V (`+5V`). There is no room on F.Cu: the courtyards of U3/C3 leave 0.26 mm, of U7/C14 0.02 mm. | TPS62130 datasheet (SLVSAG7F) p. 16: "it is required to place a capacitance of 0.1 μF from AVIN to AGND, to avoid potential noise coupling" | see git log |
| M2 | **U6 (TPS62065): C100 22 pF → 120 pF** (FH 0402CG121J500NT, C0G, LCSC C40059) and **C20 10 µF → 4.7 µF** (Samsung CL21A475KAQNNNE, LCSC C1779, same 0805 footprint). | TPS62065 datasheet (SLVS833E): feed-forward zero f = 1/(2π·R1·Cff) = 25 kHz (eq. 3/4) → 125 pF; with 22 pF the zero sat at 142 kHz, with 120 pF at 26 kHz. Effective output capacitance must stay within 4.5–22 µF (table 7.3): VDD carried 22.3 µF (24.3 µF at +10 %), now 17.0 µF (18.7 µF). | see git log |
| M3 | **C107 (10 µF X5R, 0603, LCSC C19702) added on `VDDIO`** of the XU316, on F.Cu at 143.0 / 65.94, 2.4 mm from VDDIO pin U10.38, tied to C56.1 and through its via to the VDDIO ring on In2. | XU316 datasheet p. 27 asks for a bulk capacitor of at least 10 µF per supply; `VDDIO` had only 100 nF parts behind the 120 Ω bead FB4, the nearest larger capacitor (C61) was 42 mm away. | see git log |
| M11 | **C108, C109 (100 nF) added at the VDD pins of MK1 and MK2**, behind the 100 Ω series resistors R62/R63 (which now form a 15.9 kHz low-pass with them). Both on F.Cu: the back side around the microphone holes is kept clear (board-edge bulge r 3.8 mm, probably for the gasket). For C109 the `VDD_MIC` track to R63 was re-routed around the new part and a GND stitching via moved from 169.6 / 93.5 to 169.0 / 94.9. | T3902 datasheet p. 15: "Decouple the VDD pin … with a 0.1 µF capacitor … as close to VDD as the PCB layout allows"; until now the only capacitors (C71/C72) sat before R62/R63. | see git log |
| M14 | **5 V main path widened:** `+5V_SW` from L1 to U18 (VIN) and `+5V` from U18 (VOUT) to the vias now 0.8 mm instead of 0.3 mm, and **four vias instead of one** into the `+5V` plane on In2. | IPC-2221: 0.3 mm × 35 µm carries 1.0 A at 10 K rise against a load of up to 1.7 A by datasheet limits; now 2.0 A (track) and 3.0–3.8 A (vias). The limit is the load switch U18 (LM66100, 1.5 A continuous). | see git log |
| M15 | **XU316 core feed (`VDD`) on In2 widened from 0.5 to 0.9 mm**, and all three `VDD` vias at the converter now join it on In2 (before only one). | IPC-2221 (inner layer, 0.5 oz): 0.395 A at 10 K before, 0.605 A now; at the XU316's budgetary 830 mA the rise drops from 54 K to 20.5 K, the drop from 9.4 to 5.2 mV. Wider does not fit through the gap in the `VDDIO` ring. Typical core current (225 mA) gives 1.1 K. | see git log |
| M16 | **USB VBUS branch from J1 to FB1 widened**: the two In2 tracks from 0.25 to 0.6 mm (they merge into one band of about 1.2 mm), the B.Cu link to FB1 likewise. | IPC-2221 (inner layer, 0.5 oz): 0.48 A at 10 K rise before, 0.90 A now; at 0.8 A the rise drops from 32 K to 7.6 K. | see git log |
| M18 | **L1/L2 land pattern matched to the ordered part.** New footprint `Inductor_SMD:L_Bourns_SRP4020TA` (pads 1.5 × 2.4 mm at ±1.85 mm, outer 5.2 mm) replaces `L_Cenker_CKCS4020`, whose pads ended under the body of the Bourns part. Field `Saturation` corrected from 4.2 A to 6 A. Test point TP17 (unpopulated) moved up 0.35 mm to clear the larger courtyard. | Bourns SRP4020TA datasheet, "Recommended Layout" (5.2 / 2.2 / 2.4 mm) and product table (2R2M: Isat 6.0 A). The matching Cenker part (C5291811) is weaker (3.4 A, ±30 %). | see git log |

## 11. Pin-audit fixes before ordering (rev A2, 1 Oct 2026)

A second independent audit went through every pin of U8 (ESP32-S3), U10 (XU316) and U14 (TAS5805M)
and through all function chains (pin audit, working notes `PIN_AUDIT_REV_A2.md`). It found no
critical hardware issue. The changes below were decided before ordering, one commit each; the
rows from U10-B1 on were added on 2 Oct 2026, when every remaining point was decided (see "Every open
point decided").

| Pin audit | What | Why / evidence | Commit |
|---|---|---|---|
| U8-B2 → H1 | **Y1 (40 MHz crystal) replaced by a ±10 ppm part:** JLYE Y201640MDBCX, LCSC C49158179 (3 014 in stock on 1 Oct 2026), same SMD2016-4P package and pin-out (1/3 crystal, 2/4 GND), same load capacitance 9 pF, so C26/C27 (12 pF) stay. The `Tolerance` field said "10ppm" although the fitted part (Yajingxin TAXM40M4ZCBCDT2T, C424431) is ±20 ppm; corrected. | Espressif ESP32-S3 Hardware Design Guidelines 1.3.5: "the accuracy of the selected crystal should be within ±10 ppm". JLYE datasheet (LCSC): tolerance ±10 ppm at 25 °C, stability ±20 ppm over −40…+85 °C, ESR ≤ 60 Ω (31–48 MHz), C0 ≤ 3 pF, drive ≤ 100 µW. Old part per LCSC: ±20 ppm / ±20 ppm. | see git log |
| K-B6 → H2 | **R72 (REXT of the MPR121) 200 kΩ → 75 kΩ 1 %**, UNI-ROYAL 0402WGF7502TCE, LCSC C25798 (JLCPCB "preferred extended", no setup fee; 45 660 in stock on 1 Oct 2026). Same 0402 footprint, value change only. | MPR121 datasheet pin 7: "Connect a 75 kΩ 1% resistor to VSS to set internal reference current". With 200 kΩ all charge currents scale by 0.375; ESPHome's `mpr121` uses fixed charge settings (no auto-configuration), and the pin audit's estimate put a finger touch at 2–8 counts against `touch_threshold: 12`. See "REXT at the MPR121" below. | see git log |
| U8-B7, K-B13 → H3 | **R81 (10 kΩ, 0402, LCSC C25744) added from `LED_DATA_3V3` to GND** at the input of the level shifter U19 (74AHCT1G125), on B.Cu at 116.1 / 84.9 (90°), pad 1 tied by a 0.66 mm track to the existing via at U19 pin 2, pad 2 in the GND fill. | Until the ESP32 configures GPIO21 (reset, ROM boot), the pin floats, and U19 then passes an undefined level to six SK6812 that sit on +5 V permanently — random colours or a lit ring at power-up. The 74AHCT1G125 input has no internal pull (datasheet: input leakage only); 10 kΩ pulls it low, and GPIO21 drives 3.3 V into it at 0.33 mA. | see git log |
| U10-B2 → H4 | **C17 (output of the 1.8 V LDO U5) 4.7 µF → 10 µF**, Samsung CL10A106KP8NNNC, X5R 10 V, LCSC C19702 (JLCPCB basic part), same 0603 footprint. `+1V8` feeds VDDIOB18 and USB_VDD18 of the XU316 and the JTAG reference on J4. | XU316 datasheet (XM014429B) section 13, p. 27: a bulk capacitor of at least 10 µF on each supply; `+1V8` had 4.7 µF as its largest part. The TLV70018 is stable with any effective output capacitance ≥ 0.1 µF and ESR < 200 mΩ (TLV700 datasheet 8.2.2.1), no upper limit; charging 10 µF at the 220 mA minimum current limit takes about 80 µs. | see git log |
| U8-B4 → H5 | **No external pull-down on GPIO45 — note only, no room.** Pin 51 sits between the `XU316_RST` escape (0.175 mm from the pad end) and the crystal tracks on F.Cu; on B.Cu right underneath is U16 with a GND pad. A resistor would need the crystal area re-routed. GPIO45 stays unconnected and relies on its internal weak pull-down (45 kΩ typ.), which keeps VDD_SPI at 3.3 V. | ESP32-S3 datasheet table 3-1: the strapping value is set by the internal weak pull-down "if the pins are not connected to any circuit" (GPIO45 → 0); table 3-4: GPIO45 = 0 → VDD_SPI 3.3 V from VDD3P3_RTC; table 5-4: R_PD 45 kΩ. GPIO45 has no copper at all, so nothing can couple in. Fallback without hardware: the eFuses `VDD_SPI_FORCE` = 1 and `VDD_SPI_TIEH` = 1 fix 3.3 V regardless of GPIO45 (table 3-4) — one-time, only if a sample ever shows the wrong flash voltage. | note only |
| U10-B3 → H6 | **15 vias added in the exposed pads of U10 (XU316):** 11 in the GND paddle (now **16**), one in each of the four VDD paddles (now **4 each**). All 0.4/0.3 mm through vias like the existing ones, at least 0.7 mm centre-to-centre (hole-to-hole ≥ 0.4 mm, rule 0.25 mm), placed by DRC against everything on the other three layers (one candidate at 136.45 / 63.17 hit C51 on F.Cu and was moved to 137.85 / 65.32). | XU316 datasheet (XM014429B) section 13.4: "16 vias in a 4 x 4 grid" in the ground paddle and four per VDD paddle; there were 5 and 3. The five existing GND vias (a cross) were kept, so the 16 are not a regular grid. Copper check after filling: `VDD` on In2 keeps **one island** and its outer bands are unchanged (all new GND vias lie inside the area spanned by the five old ones); only the interior gets perforated (31.7 → 24.9 mm²). GND In1 2790.8 → 2788.8 mm², one island; GND F.Cu 2196.4 → 2194.9 mm², 21 → 21 islands. | see git log |
| U10-B1 (and M19) | **C63 100 nF → 1 µF** (Samsung CL05A105KA5NQNC, X5R 25 V, LCSC C52923, already in the BOM), same 0402 footprint — the reset RC of the XU316 (R49 10 k to `+1V8`) goes from τ = 1 ms to 10 ms. Value change only, no copper. | XU316 datasheet section 13, p. 27: VDDIO must be valid before `RST_N` is released. Calculated before: boot start at 3.6–4.8 ms with `+3V3` at 2.44–3.03 V (limit 2.97 V), violated in 5 of 6 corners; now release at 8.4–13.7 ms, `+3V3` stable by 4.94 ms. The `voice_kit` reset pulse (1 ms through Q2) still discharges C63 in ≤ 10 µs. | `5a8c1ce` |
| C98 | **C98 (VREG of the MPR121) 1 µF → 0.1 µF** (Samsung CL05B104KO5NNNC, X7R 16 V, LCSC C1525, already in the BOM). | MPR121 datasheet pin 5: "Connect a 0.1 μF bypass cap to VSS"; section Power Supply: "a separate 0.1 μF decoupling ceramic capacitor on VREG". | `5a8c1ce` |
| U8-B1 | **C110 added as an unpopulated (DNP) 0201 footprint**: shunt trap on `LNA_IN`, 1.0 pF C0G (Murata GRM0335C1H1R0BA01D, LCSC C85893), F.Cu 119.15 / 74.40, 180°, pad 1 with a 0.2 mm stub to `LNA_IN`, pad 2 to a new GND via at 118.35 / 74.40. `LNA_IN` itself unchanged. | Today's matching network is kept (mismatch loss 0.55–0.80 dB; a CLC would gain 0.14 dB) but does not attenuate the second harmonic (+0.3 dB relative); with C110 about −24 dB. Fitted only after the VNA and harmonic measurements — see "Radio approval". Netlist 248 → 249 parts, DRC 8 → 8, parity 0. | `852b458` |
| U14-B2 | **Firmware: register 0x53 = 0x60** (class-D loop bandwidth 80 → 175 kHz) in the `tas58xx` table. | SLASEH5D table 7-27: "With Fsw=768kHz, 175kHz bandwidth should be selected for high audio performance"; Fsw ≥ 3 × BW holds (768 ≥ 525 kHz). Patch documented in `firmware/esphome/PATCH.md`. | `c6b3fb3` |
| M6b | **MPN field of R68/R69** corrected from `0402WGF0000TCE` to `0805W8F0000T5E` (schematic and layout). | The part (LCSC C17477) and the BOMs were already right; only the field named a 0402 part. | `c6b3fb3` |
| K-B8, K-B9, K-B10, K-B14, U8-B6, U8-B12 | **Firmware:** sounds and wake-word models pinned, `min_version`, two microphone channels, GPIO reserve comment, PSRAM-ECC option and temperature sensor. | See section 8, "Decisions of 2 Oct 2026". `esphome config` valid, compile successful. | `d1a3583` |

## Bring-up order: power it from a bench supply first

Decision of 1 Oct 2026. The first time this board is powered, it is **not** connected to the base
plate.

1. **Nothing connected.** Resistance check from each rail to ground, board unpowered: `+14V`,
   `+5V`, `+5V_SW`, `+3V3`, `ESP_3V3`, `+1V8`, `VDD`, `VDD_MIC`, `PVDD`. A short here is cheaper
   to find now than after 14 V has been applied.
2. **Bench supply on TP18 and a ground test point**, current limit set low — start at 100 mA.
   TP18 is `+14V` at 143.90 / 109.43; use TP21 (120.58 / 64.74) or TP22 (128.74 / 104.08) for
   ground. Bring the voltage up slowly and watch the current. The board draws its quiescent
   current through U3 and U7; the LEDs and the amplifier are off until firmware runs.
3. **Measure the rails in order:** TP18 `+14V` → TP3 `+5V_SW` → TP17 `+5V` → TP1/TP6 `+3V3`
   → TP4 `+1V8` → TP5 `VDD` (0.9 V) → TP23 `VDD_MIC` (3.3 V). Each one has to be right before
   the next matters. **TP3 against TP17** is the pair that shows whether the ideal diode U18
   conducts: about 55–65 mV of difference under load.
4. **USB next**, still without the base plate. On USB alone TP3 should read near 0 V while TP17
   carries about 4.9 V — that is Q4 having locked out U4's path into the 14 V rail. Then plug
   both in, in either order, and confirm nothing changes.
5. **The base plate last.** By then every rail, the ideal diode and the USB lock-out have been
   seen working, and the only new thing the base plate brings is the cable itself.

Why this order: a bench supply has a current limit, the base plate does not. Everything that can
be learned at 100 mA should be learned before 14 V arrives from a supply that will happily deliver
amps into a fault.

**Measurement plan.** Every point that could not be decided on paper (result MEASURE AT BRING-UP in
"Every open point decided") has a written procedure — what, with which instrument, target, limit, and
what follows from a deviation — in the maintainer's working notes (`HANDGRIFFE.md`, section
"Inbetriebnahme — Messvorschriften", in German, not published), in the order above: resistance check →
bench supply → rails → USB → 14 V → firmware → audio → radio → long run and temperature. The limits
that matter most:

| Step | Measurement | Target | Limit / action |
|---|---|---|---|
| Rails | XU316 power-on, 4 channels (`+3V3` TP1, `+1V8` TP4, VDD TP5, `RST_N` J4.10) | when `+1V8` passes 1.62 V, `+3V3` ≤ 1.98 V; `RST_N` released only after `+3V3` ≥ 2.97 V | otherwise check C63 |
| Rails | `ESP_3V3` before any eFuse is burned | ≤ 3.30 V | above → do not burn |
| USB | USB alone | speaker silent, `tas58xx` PVDD fault or set-up error in the log; mics, LEDs, Wi-Fi work | expected, not a defect |
| 14 V | `+14V` at TP18 when plugging in; under full load | peak ≤ 17 V; ≥ 8.5 V at full volume | never ≥ 20 V (U3 absolute maximum): do not use that base/adapter |
| 14 V | 14 V removed during playback | output Hi-Z once `+14V` ≤ 4.2 V, no spike > 1 V at the speaker | |
| Firmware | XU316 boot after reset | first stable LRCK < 2.5 s; log `[voice_kit] DFU version: 1.3.1` | > 3 s → `voice_kit` timing |
| Firmware | TAS5805M registers | 0x53 = 0x60, 0x37 = 0x09, 0x38 = 0x40, 0x68 = 0x03, 0x71 = 0x00 | 0x46 A/B test 0x01 / 0x11 |
| Audio | `I2S_BCLK`/`I2S_LRCK` at U14 pins 7/6, ≥ 500 MHz scope, spring ground | peak ≤ 3.8 V, undershoot ≥ −0.5 V | otherwise edge measure in XU316 firmware or a fit change |
| Audio | OUT_A+ at C74 at 1 W and 6.9 W | peak ≤ 18 V, undershoot ≥ −1 V | otherwise 100 nF at PVDD pins 27/28 |
| Audio | AVDD at C80 | ≥ 4.75 V within 5 ms of PDN↑, ripple < 50 mVpp | otherwise C80 → 1 µF |
| Audio | LRCK frequency; speaker R_DC at J5 | 48 000 Hz ± 1.9 Hz; 2.8–3.6 Ω | R_DC ≥ 1.5 Ω |
| Audio | Touch (MPR121 counts) with music at full volume | noise < 6 counts, touch ≥ 24 counts | raise thresholds per channel |
| Radio | VNA at C28 (MV-3); harmonics (MV-5); RSSI (MV-4) | \|Γ\| against 35 Ω ≤ −10 dB over 2.400–2.484 GHz; harmonics ≤ −30 dBm ERP; RSSI at most 6 dB below a reference board, ≥ −67 dBm at `output_power: 8.5dB` | MV-5 fails → fit C110 and re-tune ("Radio approval") |
| Long run | Air 2–3 mm above U8 in the closed housing, ≥ 2 h at full volume | ≤ 53 °C at 23 °C room | 65 °C − (35 °C − room); above 53 °C → `enable_ecc: true` |
| Long run | Amplifier faults after 30 min full load | GLOBAL_FAULT1 / CHAN_FAULT = 0 | |

## Before you order: the seven things to get right

Each of these has its own section with the evidence; this is the short list to hand to a
fabricator or to check in the order form. Points 2 and 6 are **order options that have to be
selected**, and point 7 is **a remark to enter in the order form**, not just things to check.

| # | What | Why it matters |
|---|---|---|
| 1 | **0.10 mm minimum track width** must be fabricable | `UART_TX` and `UART_RX` use the net class `UART-eng` at 0.100 mm. JLCPCB does 0.09 mm on a 4-layer board without a surcharge; a house quoting 4 mil (0.1016 mm) would reject or surcharge them. See "When ordering: the board needs 0.10 mm minimum track width". |
| 2 | **Order the vias filled: JLCPCB option *Epoxy Filled & Capped*** | The board vias reach into **23 pads (58 vias, all same-net)**, measured after the pin-audit fixes of 2 Oct 2026: the via centre lies inside 14 pads — J6.11, R79.2, R80.1 (signal), C99.1, U11.6, L1.2 (supply), C47.2 and C60.2 (GND, above the U10 paddle; two of the vias added in H6), and the large pads of U8 (9 vias) and U10 (32 vias) — and in 9 more pads part of the drilled hole reaches into the solder-mask opening (C65.1, J1.B6, J4.4, J4.6, LED2.1, TP21, TP22, U19.2, U19.4). **Do not order these untented and unfilled.** In reflow an open via inside a signal pad wicks solder out of the joint and down the barrel; the joint is left starved, and on a 0.30 mm FPC pad like J6.11 there is nothing to spare. Filling and plating over gives a flat, closed pad surface — JLCPCB's own description is "filled with epoxy resin or copper paste and then plated over to achieve an opaque and smooth finish", and it "forms flat surface after plugging, suitable for BGA and SMT holes". Our vias are 0.30 mm drill / 0.40 mm diameter, inside the 0.15–0.55 mm range the process supports and well under the 0.5 mm above which JLCPCB accepts no complaints about incomplete filling. See "Vias inside pads" and "Vias in the large pads of U8 and U10". |
| 3 | **U11 must be an `IQ` part, not `IM`** | `W25Q32JVSSIQ` (LCSC C179173). The `IQ` order suffix has the QE bit **fixed to 1 at the factory**, so IO2 and IO3 work as data lines out of the box — which is what the XU316 needs to boot from quad SPI. The `IM` variant trades that for a `/RESET` pin and does not have QE fixed. Same package code `SS` (8-pin SOIC 208 mil) for both, so the footprint will not tell them apart. See "When ordering: the XMOS flash U11 stays at 4 MB". |
| 4 | **Seven via pairs sit at the 0.2 mm hole-to-hole limit** | 0.500 mm centre to centre with 0.300 mm drills leaves 0.200 mm of material. JLCPCB's limit for **same-net** vias is exactly 0.2 mm (`jlcpcb.com/capabilities/pcb-capabilities`), and all seven pairs are same-net vias on `+14V` (3 pairs), `+5V`, `+3V3`, `ESP_3V3` and `PVDD` (one each). It is inside the limit with nothing to spare, so a house with a 0.25 mm rule will flag it. They come from upstream. |
| 5 | **J1 needs confirming by hand** | The USB-C receptacle (Molex 1054500101, LCSC C134092) is stocked but JLCPCB flags it "process difficult" in the BOM check, where it has to be confirmed manually. No stocked alternative fits the footprint. |

| 6 | **Surface finish: ENIG**, not HASL | Decision of 1 Oct 2026. Five things on this board want a flat, planar finish: the **0.40 mm pitch** of U8, U10, U15 and U6, where HASL's uneven solder domes bridge; the **QFN thermal pads**, which need to sit flat; the **J4 spring-contact pad field**, where an XTAG adapter presses onto bare pads and HASL's bumps give unreliable contact; the **eighteen test points**, measured with a probe tip; and the **three touch electrodes** on B.Cu, which are large bare-copper rings that have to stay planar and corrosion-free for the MPR121 to see a stable capacitance. ENIG gives all five; HASL gives none of them. |
| 7 | **Order remark: `Y1 orientation per silkscreen pin-1 corner mark (top-left).`** | EasyEDA has no model for Y1 (C49158179, "Component not found", 2 Oct 2026), so the assembly preview shows only a placeholder and the part is placed by hand. A 90° error puts the crystal on the ground pads. The silkscreen corner mark and pad 1 (`XTAL_N`) are both at the top left, measured from the board. See "Y1 has no model in the JLCPCB preview". |

Beyond those seven: the stack-up is set to **JLCPCB JLC04161H-7628** (1.6 mm, 4 layers, 0.5 oz
inner copper).

**What the two options cost is not published.** JLCPCB states the technical terms for both on
`jlcpcb.com/capabilities/pcb-capabilities` and `jlcpcb.com/help/article/pcb-via-covering`, and the
surcharge list on `jlcpcb.com/help/article/in-what-cases-will-there-be-charged-extra` names neither
a via-filling fee nor an ENIG base price (all three retrieved 1 Oct 2026). Both amounts come from
the interactive quote tool, which needs an account — so **no figure is given here rather than a
guessed one**. Two things *are* established:

* Via filling is free and standard on 6-layer and above; on a **4-layer** board it is charged.
* The ENIG **area surcharge** does not apply to this board. JLCPCB charges "$0.8992/m² … for
  every 1% for the portion exceeding 30%" of exposed copper. Measured from the board: the solder
  mask openings are **393.0 mm² on F.Mask (473 pads) and 174.3 mm² on B.Mask (272 pads)**, together
  567.3 mm² against 7 423.1 mm² of board area over both sides — **7.64 %**. Every via on the board
  is mask-covered, checked rather than assumed: they contribute **0.0 mm² in 0 openings**. (The
  vias that sit inside pads are enclosed by those pads' openings and already counted there.)
  7.64 % is far below the 30 % threshold, so only the ENIG base price applies.

### What two boards cost

Standard PCBA, assembled on both sides, **two boards** (decision of the operator, 1 Oct 2026).
Component prices and classifications from JLCPCB's own parts API, retrieved **1 Oct 2026**; the
full table of all **86** lines is in `pruefung/e7_jlcpcb_2026-10-01.csv`. Quantities are
`max(need + loss allowance, minimum order)` per line, which is how JLCPCB bills — at two boards
that minimum dominates for most passives, so the component cost does not halve against five.

| Item | Cost | How it is made up |
|---|---|---|
| Setup fee | 51.12 $ | double-sided Standard PCBA, independent of quantity |
| Stencil | 16.42 $ | double-sided, independent of quantity |
| Feeder loading | **131.58 $** | 86 part numbers × 1.53 $ — Standard charges this for basic and extended alike, and it does not shrink with the order |
| Solder joints | 2.60 $ | 814 joints × 2 boards × 0.0016 $ |
| Components | 102.63 $ | 34 basic, 52 extended |
| **Assembly total** | **304.36 $** | **152.18 $ per board** |
| Bare boards | **not retrieved** | 4 layers, 69.2 × 69.2 mm outline, 2 pieces. JLCPCB publishes no price list for this — it comes from the interactive quote tool, so it is **not stated here rather than guessed**. |

Most expensive lines: U10 (XU316) 26.24 $, MK1/MK2 6.20 $, U8 (ESP32-S3R8) 6.11 $, U15 (MPR121)
5.38 $, AE1 (antenna) 5.12 $, U9 (W25Q128) 5.10 $, U3/U7 (TPS62130) 3.69 $, the eight 22 µF
3.52 $.

**Where the money goes at this quantity:** the three quantity-independent items — setup, stencil
and feeder loading — come to **199.12 $, almost two thirds of the total**. The feeder loading alone
(131.58 $) exceeds the entire component cost. Per board the fixed share is 99.56 $, which is why
two boards cost 152.18 $ each where five cost 82.84 $ each. Reducing the number of distinct part
numbers is the one lever that matters here — which is why the 100 nF consolidation was examined at
all. It turned out not to be available: see section 7 on C83/C88.

**The number of part numbers was 84 in an earlier version of this table and is 86.** The earlier
figure was counted before E7 corrected J6 and R80: both carried `C25744`, the number of a 10 kΩ
resistor, which was already in the list, so two numbers were hidden behind one. Three numbers were
also missing from the price file altogether — `C133796` (U5, from E5.2), `C25778` (R77) and
`C424659` (J6) — and one was stale, `C626087`, the MIC5365 that E5.2 replaced. All four fixed; 86
numbers now match the schematic, the layout and the generated BOM.

For comparison, at larger quantities the fixed share stops dominating: the component cost per board
falls to about 36.84 $ at 30 boards and 30.02 $ at 100 (figures from section 34.4 of the working
log, on the 24 Sep price basis).

## Open points before or at bring-up

### POWER_IN1: the pinout is now confirmed against a second source

The ten-way FFC to the base plate is wired **pad 1 and 3–6 to GND, pad 2 to MUTE, pads 7–10 to
+14V**, with the two shield tabs unconnected. That assignment came from the designer's prototype and
was long carried here as unverified: `upstream/PCB_connections.xlsx` covers J2, J3, J4 and the test
points but **not** this connector (checked again on 1 Oct 2026 by reading the sheet out in full),
the two prototype photos show the orange flat cable but not its conductors, and none of the 18
upstream discussions touches connector pinouts.

**It is now confirmed from an independent design.** [Onju Voice](https://github.com/justLV/onju-voice)
by Justin Alvey is a replacement board for the *same device* — its README says "just make sure you
get the 2nd gen" — so it meets the same base-plate connector. Its schematic
(`hardware/Onju-Home.SchDoc`, branch `master`, Altium, 796 672 bytes, retrieved 1 Oct 2026) has a
ten-way connector **J1** whose nets were read out of the file by following the wires from each pin:

| Pin | Onju Voice J1 | this board | |
|---|---|---|---|
| 1 | `GND` | `GND` | same |
| 2 | `MUTE` | `MUTE` | same |
| 3 | `GND` | `GND` | same |
| 4 | `GND` | `GND` | same |
| 5 | `GND` | `GND` | same |
| 6 | `GND` | `GND` | same |
| 7 | `14V` | `+14V` | same |
| 8 | `14V` | `+14V` | same |
| 9 | `14V` | `+14V` | same |
| 10 | `14V` | `+14V` | same |

**All ten pins agree.** Two independent designs for the same device arrive at the same assignment,
which is as good as this gets without a meter on an original. The second ten-way connector in that
schematic, J3, carries `CHIP_PU`, `GPIO0`, `TXD0`, `RXD0` and `VBUART` — that is its programming
header, not the base plate.

The pin-1 mark on our board is the 0.4 mm silkscreen circle at 143.560/117.651, 1.30 mm above
pad 1. If you still want to meter it, the quick version: continuity from pads 1, 3, 4, 5, 6 to the
barrel jack's outer contact; the mute slider moves exactly pad 2; pads 7–10 read about 14 V.


* **Physical checks:**
  * Touch electrodes against the enclosure touch zones (E2).
  * POWER_IN1 pin-out: **confirmed** against the Onju Voice schematic, all ten pins (see
    below). Metering it on an original is optional now, not required.
* **Documents still to verify:**
  * Land pattern of L6/L7 against the MetalLions datasheet.
  * Row pitch of the U1 fixing pads (2.0 mm in the footprint, about 2.1 mm in the Nidec
    drawing).
* **Done in rev A2:** the A4 back-feed is blocked by U18 and Q4 (E4). (The RF trace width was
  carried out in G4b, `9f4e9bd`.)
* **The XMOS flash (U11) is empty as delivered.** The board has to be programmed once before
  the XU316 will run. Over-the-air and I²C updates from the ESP32 only work once a factory
  image is in the flash. The two ways in are JTAG at J4 with an XTAG adapter, or a test clip
  on U11. With an XTAG, `xflash` loads a flash loader over JTAG into the XU316's RAM and writes U11
  from there (XTC Tools Guide XM-014363-PC v15.3, section 5.1.7); the third way, writing U11 from the
  ESP32 through U16, is wired but has no firmware yet (see "QSPI bus isolation uses one switch" below).
* **Assembly preview:** check the Y1 orientation in the JLCPCB preview; there is no EasyEDA
  footprint to compare against.

## Checked against Gen1, deliberately left as they are

Three findings from the review list turned out to be either the designer's own choice or
solvable in firmware. They are recorded here so nobody re-opens them without the evidence.

### REXT at the MPR121: was 200 kΩ, now the datasheet's 75 kΩ (finding 4, changed in H2)

R72 (200 k) sits between GND and `Net-(U15-REXT)` at U15 pin 7. The MPR121 datasheet says
"External Resistor – Connect a 75 kΩ 1% resistor to VSS to set internal reference current", so the
internal reference current is a factor 2.67 smaller than the datasheet's nominal, which shifts
electrode charge time and sensitivity.

**Gen1 uses the same value.** In the Gen1 netlist (`pruefung/gen1.net`) the MPR121 is U2, and
`Net-(U2-REXT)` consists of U2.7 and **R2 = 200k**. The touch block of this fork was taken from
Gen1 on purpose (BAUTEILE.md 2.6), and Gen1 works. The MPR121's charge current and charge time are
register-configurable, so the effect is compensated in firmware if it ever shows up.
**No change.** Worth measuring on the sample: touch sensitivity of the three electrodes.

**Audit 1 Oct 2026 (M10), kept as a note:** the independent audit flagged this again as a deviation from the datasheet (medium). It stays at 200 kΩ for rev A2, for the reasons above. If touch is unreliable on the sample, the fix is a value change only: R72 → 75 kΩ 1 %.

**Changed after the pin audit (1 Oct 2026, H2):** R72 is now 75 kΩ 1 % (C25798). ESPHome's `mpr121` component sets fixed charge current and time and does no auto-configuration, so the firmware compensation assumed above is not available without a custom component; the pin audit estimated a finger touch at 2–8 counts against the threshold of 12 with 200 kΩ. Gen1 may tune its registers differently; this fork follows the datasheet.

### No pull-up on the MPR121 interrupt (finding 11)

`TOUCH_IRQ` consists of exactly two nodes, U15.1 (`IRQ`, an open-collector output) and U8.7
(GPIO2). **Gen1 is identical** — there, `/ESP32-S3R8/TOUCH_IRQ` is U2.1 and U6.7, also without a
resistor.

The ESPHome `mpr121` hub component has **no interrupt-pin option at all** (esphome.io, retrieved
30 Sep 2026: the hub takes only `address`, `id`, `touch_debounce`, `release_debounce`,
`touch_threshold`, `release_threshold`), so the line is not read; the component polls over I²C.
What remains is that GPIO2 would float. `nest-mini-v2-voice.yaml` therefore declares an internal GPIO
binary sensor on GPIO2 with `pullup: true`, which switches in the ESP32-S3's internal weak pull-up
of **typically 45 kΩ** (ESP32-S3 Series Datasheet v2.2, table 5-4 "DC Characteristics", p. 65).
**No hardware change.**

### Q3 drives the microphone LDO's enable inversely (finding 5)

Q3 (BC847B in SOT-23) has pin 1 = base, and physically **pin 2 = emitter, pin 3 = collector**, but
the symbol `Transistor_BJT:Q_NPN_BCE` assigns pin 2 as the collector. In the layout pin 2 sits on
`Net-(Q3-C)` together with R54 (470 kΩ to +5 V) and **U13 pin 3**, the `EN` of the microphone LDO,
while pin 3 goes to GND. The transistor therefore works inverted.

**It still works, and it is within specification.** The load is only **10.6 µA** (5 V through
470 kΩ), which an inverse current gain of 2 to 5 covers many times over, and the saturation voltage
in inverse operation stays below 0.1 V, so `EN` is pulled well under its 0.35 V threshold
(TLV733P, SBVS235C, `VEN(LO)` max 0.35 V). What is left is the base-emitter reverse voltage: with
`MUTE_ON` at 0 V the emitter sits at +5 V through R54, so **V_EB = 5 V against a rated V_EBO of
6 V** — inside the limit, but with only 1 V of margin, and reverse-biased junctions get leaky and
noisy near that figure. The leakage that matters would have to exceed the 10.6 µA the pull-up
supplies before `EN` misbehaves, which is orders of magnitude away.

**Two fixes were measured and both are too expensive for rev A2:**

* **Swapping pins 2 and 3.** The symbol has to change from `Q_NPN_BCE` to `Q_NPN_BEC` *and* the
  drawn wires at pins 2/3 have to be exchanged, otherwise the emitter ends up on the load; in the
  layout the 3.1 mm B.Cu track from pad 2 (139.218/95.450) has to move to pad 3
  (137.343/94.500), which today has no track of its own but is tied to the ground plane.
  Estimated 1.5 to 2 hours with the evidence runs.
* **Moving R54 from +5 V to +3V3**, which would bring V_EB down to 3.3 V. This was tried and
  **rolled back**. The schematic side is easy, but the layout is not: at R54 the In2.Cu plane
  carries **+5V**, and the nearest +3V3 fill is 7.00 mm away (135.718/86.550), the nearest +3V3
  copper of any kind 9.35 mm. The router found a path of **13.03 mm with 3 vias** — and worse, pad
  R54.2 sits only **0.045 mm** from the +5V tracks running past it (needed: 0.127 mm), so giving the
  pad a different net produces two clearance violations that can only be cleared by moving the +5V
  track itself. It also cut a new island out of the ground plane. DRC went from 33/2/0 to 35/3/0, so
  the change was reverted.

**Decided 2 Oct 2026: kept as it is (VERIFIED NOT NEEDED).** The BC847B datasheet (LCSC C20069135,
p. 1) specifies **I_EBO ≤ 0.1 µA at V_EB = 5.0 V** — exactly this operating point. Through R54 the
enable stays at 4.95 V at 25 °C and 4.25 V at 65 °C (the ESP32's ambient limit), against V_EN(HI) 0.9 V
of the TLV733P. Swapping pins 2 and 3 would only trade I_EBO for I_CBO, which is specified at the same
≤ 0.1 µA. See "Every open point decided".

### No power-good gating for the XMOS reset (finding 21)

`RST_N` has a 10 kΩ pull-up to `+1V8` (R49) — the right domain, `RST_N` belongs to VDDIOB18 — plus
100 nF (C63) and Q2, driven from GPIO46 through R47/R48. The Voice PE schematic carries the explicit
note "XMOS RST must after VDD and VDDIOB_18 Power Good." and implements it with `3V3_PG`, a Schmitt
buffer and two transistors. Here the release depends on the RC time and on the ESP32.

**Left as it is, because the firmware re-resets the part anyway.** GPIO46 has an internal pull-down
(ESP32-S3 datasheet v2.2, table 3-1) and externally only R47 plus R48 (470 k to GND), so during
power-up it reads 0, Q2 stays off and `RST_N` rises with R49/C63 — about 1 ms after `+1V8`. That
first release is indeed not gated on VDD. But the ESPHome `voice_kit` component drives
`reset_pin: GPIO46` in its own setup, at priority `HARDWARE - 1` = **799.0**
(`esphome/home-assistant-voice-pe`, `components/voice_kit/voice_kit.h`; the constants are in
`esphome/core/component.h`) — that is, long after every rail has settled and the ESP32 itself has
booted. So the XU316 gets a clean reset from the firmware regardless of what happened at
power-on.

**To confirm on the sample:** power-up behaviour — does the XU316 come up on the first try, and does
it still come up if the supply is ramped slowly? A sporadic failure here would point straight back
to this finding.

**Audit 1 Oct 2026 (M19), kept as a note:** the XU316 datasheet (section 13, pp. 27–28, figure 17) asks either to hold `RST_N` until VDDIOL/R/T are valid, or to have VDDIOL/R/T valid by the time VDD and VDDIOB18 are; and the 3.3 V supply should not rise above 1.98 V while VDDIOB18 is off. Here VDD (U6) is valid about 0.5 ms after `+5V`, `+1V8` (U5, fed from `+3V3`) starts when `+3V3` passes about 1.9 V — by then `+3V3` is at about 1.97 V, right at the 1.98 V limit — and VDDIO reaches 2.97 V only about 1.6 ms later, while `RST_N` already follows `+1V8` with τ = R49·C63 = 1 ms. Neither condition is met with margin. The firmware reset (GPIO46) repairs the boot, not the sequencing. **Fixed in rev A2 (U10-B1, `5a8c1ce`): C63 is now 1 µF**, so τ = R49·C63 = 10 ms. The calculation puts the release of `RST_N` at 8.4–13.7 ms, while `+3V3` is stable by 4.94 ms at the latest (soft start 4.63–5.43 ms); before, the XU316 started to boot at 3.6–4.8 ms with `+3V3` at only 2.44–3.03 V. The 1 ms `voice_kit` pulse still discharges 1 µF through Q2 in ≤ 10 µs. The second condition (3.3 V supply ≤ 1.98 V while VDDIOB18 is off) is not touched by C63 and has only about 20 mV of calculated margin, so it is measured at bring-up: four channels at power-on — `+3V3` (TP1), `+1V8` (TP4), VDD (TP5) and `RST_N` (J4.10); when `+1V8` passes 1.62 V, `+3V3` must be ≤ 1.98 V, and `RST_N` must cross its threshold only after `+3V3` ≥ 2.97 V.

## Every open point decided (rev A2 is the production state)

**Rev A2 is the production state; there is no rev B.** Every point that earlier versions of this
file deferred — from the audit of 1 Oct 2026 (working notes, `AUDIT_REV_A2.md`), its grouped low
list, and the pin audit (working notes, `PIN_AUDIT_REV_A2.md`) — was decided on 2 Oct 2026 by four
fresh reviewers working on the design files at `88b7c04` / `2c536a8` (working notes, AENDERUNGEN 149
and 150). Each point has one of four results:

| Result | Meaning |
|---|---|
| **IMPLEMENTED** | changed in rev A2; the commit is named |
| **NOT PLACEABLE** | there is no room without re-routing the ESP32 fan-out; a bring-up measurement decides whether it matters |
| **VERIFIED NOT NEEDED** | a calculation against a datasheet limit shows no measurable effect |
| **MEASURE AT BRING-UP** | cannot be decided on paper; what to measure, target and limit are in the bring-up plan (see "Bring-up order" above) |

Five statements in the earlier list were wrong and are corrected below: the USB pair impedance
(about 117 Ω, not 106 Ω), the current in the `+14V` feed to U3 (about 0.34 A, not 1.1 A), the
connection of C83 (M7), the source resistors of `MIC_CLK_M`, and the net next to `ELEC1` (M9).

### Audit of 1 Oct 2026 — medium points

| Point | Finding | Result | Evidence |
|---|---|---|---|
| M4 | Fast clocks on B.Cu over split supply areas on In2; In1 GND slotted by 151.2 mm of signal tracks | MEASURE AT BRING-UP (MV-1) | Differential-mode estimate (Ott, 3 m, pessimistic return path): every clock that runs continuously stays below CISPR 32 class B with margin (I2S_MCLK +7.7 dB, I2S_BCLK +18.2 dB, MIC_CLK_M +15.2 dB); only `QSPI_CLK`, which runs at boot and DFU only, may reach the limit (−6.8 dB pessimistic, −3.9 dB optimistic). Cable common mode cannot be calculated. Re-routing In1 would touch the ESP32 fan-out. |
| M5 | I²S and I²C cross the GND/GNDA split at x ≈ 129.1–131.1 mm | VERIFIED NOT NEEDED | Nearest bridge R69/R68 is 7.0–12.7 mm away. Detour 3.4–21 nH at 10.9 mA charge current (BCLK, 5.4 pF, 1.64 ns) gives 23–140 mV ground bounce against ≈ 0.6 V noise margin of the TAS5805M inputs (V_IL 0.99 V, V_IH 2.31 V). A third bridge would close another loop for class-D current. |
| M6 | Class-D return only through R68/R69 (0805 0 Ω); rating undocumented | VERIFIED NOT NEEDED | UNI-ROYAL 0805 jumper: < 50 mΩ, **2 A** rated, 5 A overload (datasheet V.3, p. 5, table 7). Return current 1.23 A peak at −12 dB into 4 Ω, **≤ 1.58 A** for a 3.1 Ω driver — below 2 A even if all of it flows through one link; drop ≤ 79 mV. |
| M6b | MPN field of R68/R69 named a 0402 part | IMPLEMENTED `c6b3fb3` | MPN now `0805W8F0000T5E` (LCSC C17477); the BOMs were already right. |
| M7 | C83 (100 nF) 3.58 mm from PVDD pin 28 | VERIFIED NOT NEEDED; output check at bring-up | Correction: C83 hangs on **the same 0.5 mm F.Cu track as C79/C81** (4.9 mm), not only on vias and In2. Loop ≈ 2.3 nH (TI example ≈ 0.85 nH); L·di/dt at 2.1 A / 5 ns ≈ 0.97 V; even with twofold ringing **≤ 15.9 V** against 30 V PVDD and 32 V output absolute maximum (SLASEH5D 6.1). Bring-up: OUT_A+ peak ≤ 18 V. |
| M9 | `ELEC1` runs through the class-D area (via 0.148 mm from OUT_A+) | MEASURE AT BRING-UP (MV-2) | Correction: there is no `+14V` sense line there — the net between U14 and L6/L7 is **`MUTE_ON`** (C67 100 nF on it, coupled ≈ 1.4 µV). Estimate for ELEC1 after the MPR121 filters ≈ 0.6 counts rms against release threshold 6 — an order of magnitude only, the electrode capacitance is unknown. Re-routing would touch the amplifier output. |
| M12 | AE1 pad 2 sits 1.44 mm beside the ground area | MEASURE AT BRING-UP (MV-3, MV-4) | Johanson 2450AT18A100E: the matching values "on client's PCB will be different"; detuning is not calculable without an EM simulation, and the board outline is fixed. See "Radio approval" below. |
| M13 | ESP32-S3R8 rated to 65 °C ambient | MEASURE AT BRING-UP (M-T); firmware prepared in `d1a3583` | Datasheet table 1-1, note 3: with PSRAM ECC 85 °C (usable PSRAM −1/16). The YAML carries `enable_ecc: true` commented out and an `internal_temperature` sensor. |
| M17 | On USB alone the board needs ≈ 0.66 A (peaks ≈ 0.8 A); CC is not evaluated | VERIFIED NOT NEEDED | Operation is on the 14 V base; USB is service only, and the amplifier has no supply on USB. U4 current limit ≥ 2.2 A, U18 1.5 A. The rule "use a source of at least 1 A" stays (see "What the LED ring draws"). |
| M19 | XU316 `RST_N` released before VDDIO is valid | IMPLEMENTED `5a8c1ce` | See U10-B1 below (C63 1 µF). |
| Q3 | Q3 (BC847B) works inverted, V_EB = 5 V against V_EBO 6 V; pin swap considered | VERIFIED NOT NEEDED | BC847B datasheet (LCSC C20069135, p. 1): **I_EBO ≤ 0.1 µA at V_EB = 5.0 V** — exactly this operating point. Through R54 470 k the enable of U13 stays at 4.95 V (25 °C) / 4.25 V (65 °C) against V_EN(HI) 0.9 V (TLV733P). A pin swap only replaces I_EBO by I_CBO (also ≤ 0.1 µA). |

### Audit of 1 Oct 2026 — low points (grouped)

| Group | Point | Result | Evidence |
|---|---|---|---|
| Schematic | 21 `power_pin_not_driven`, no PWR_FLAGs | VERIFIED NOT NEEDED | Every net has a real source (regulator or connector, listed per net in the working notes); U14 AVDD/VR_DIG are internal regulator outputs that must carry only their capacitors (SLASEH5D pin table). |
| Schematic | Duplicate net names (I2S_DOUT/I2S_DIN_SEC, I2C local/global, hidden DGND) | VERIFIED NOT NEEDED | Netlist: one net each (`I2S_DIN_SEC` = R24.2, R40.2, U14.8; one `I2C_SDA`, one `I2C_SCL`); U14 DGND on GNDA as in the datasheet example (p. 84). |
| Parts | C29 (12 pF at GPIO0) | VERIFIED NOT NEEDED | τ = 10 kΩ · 12 pF = 120 ns, settled after 0.6 µs; the strap is latched ≥ 11.5 ms after supply. |
| Parts | 0 Ω pairs R22+R42, R27+R43 | VERIFIED NOT NEEDED | Assembly option ESP32 ↔ XU316; 2 × ≤ 50 mΩ has no effect at logic levels. |
| Parts | U12 channel 1 drives `MIC_DATA_2` without load | VERIFIED NOT NEEDED | 1OE = GND, 1A = `MUTE_ON`: all inputs defined, output unloaded. |
| Parts | R65 47 k on `VDD_MIC` beside the TLV733P discharge | VERIFIED NOT NEEDED | 70 µA (0.23 mW), no functional effect either way. |
| Parts | TP14–TP16 double the touch rings | VERIFIED NOT NEEDED | Only nodes besides U15 on ELEC0–2; they enlarge the electrodes marginally. |
| Pins | U18 ST (pin 5) open | VERIFIED NOT NEEDED | Open-drain output only ("Connect to GND if not required", SLVSEZ8A), leakage 20 nA; it does not act on the CE comparator. |
| Pins | Free ESP32 GPIOs without pull | VERIFIED NOT NEEDED | Estimate ≤ 100 µA per pin × 12 = ≤ 1.2 mA (no datasheet figure; HWG 1.3.10 gives none) = 0.2 % of the board current; no load, no strap. |
| Pins | XU316 X0D40–43 enabled as inputs, open | VERIFIED NOT NEEDED | Values used only in `gpio_test()`, which `main` never calls; leakage 12–317 nA (XU316 DS 14.3); the Voice PE leaves them open as well. |
| Pins | VBUS_DETEC 100 k instead of 4.7 k | VERIFIED NOT NEEDED | X1D09 is not declared in `NC-VOICE-KIT.xn` and unused by FFVA. |
| Pins | Microphone VOL up to 0.97 V against V_IL 0.8 V of U12 | VERIFIED NOT NEEDED | VOL ≤ 0.3·VDD is specified at 0.5 mA load; here the load is a CMOS input plus R52 47 k to GND → VOL ≈ 10 mV. |
| Layout | Segments shorter than 0.05 mm | VERIFIED NOT NEEDED | Recounted: 212 of 2 723. Same-net copper merges in the Gerber; DRC clean. |
| Layout | Collinear overlapping segments | VERIFIED NOT NEEDED | Same-net overlap merges in the Gerber; DRC without finding. |
| Layout | Detours > 3× on SPI_HD and XMOS_SPI_MISO | VERIFIED NOT NEEDED | SPI_HD 25.4 mm against 9.2–12.1 mm for the other flash lines → skew ≈ 0.09 ns at a 12.5 ns period. |
| Layout | Seven orphaned teardrops | VERIFIED NOT NEEDED | Six on `ESP_WS2812` fill **0.000 mm²**; the seventh (+3V3, 0.105 mm²) belongs to the via it sits on. |
| Ground/EMC | Few GND vias at MK1 (8.4 mm) | VERIFIED NOT NEEDED | ≈ 1.1 nH × 4 mA data edge → 0.44 mV. |
| Ground/EMC | R68 2.1 mm from U13's ground | VERIFIED NOT NEEDED | ≈ 0.3 mV at 1 A → ≈ −147 dBFS at the microphones (PSR −97 dBFS per 100 mVpp). |
| Ground/EMC | PDM lines cross U6's switch node | VERIFIED NOT NEEDED | In1 GND and In2 +3V3 lie between the two layers at the crossing. |
| Ground/EMC | Clocks parallel to data and to ELEC0 | VERIFIED NOT NEEDED | Near-end crosstalk 16–38 mV against ≥ 0.6 V margin; ELEC0 ≈ 0.9 counts rms (checked again in MV-2). |
| Ground/EMC | Microphone return currents over supply areas | VERIFIED NOT NEEDED | `MIC_DATA_M_1` +25.6 dB below CISPR 32 B (pessimistic estimate). |
| Ground/EMC | `VDD_MIC` loop ≈ 116 mm² | VERIFIED NOT NEEDED | R62/R63 100 Ω + 100 nF: −34 dB at 768 kHz → ≈ 0.1 mV at the microphone. |
| Ground/EMC | USB pair with 45–47 % ground reference | VERIFIED NOT NEEDED | Full speed, edges ≥ 4 ns; 32 mm is electrically short (< 1/6 of the rise length). |
| USB/clocks | USB pair impedance | VERIFIED NOT NEEDED | Correction: **≈ 117 Ω** (2-D field solution with GND on F.Cu above; **136 Ω** without), not 106 Ω. Reflection bound Γ·2T_d/t_r = 0.20 × 0.13 = **2.6 %** of the swing; FS receiver sensitivity 200 mV. Optional M-USB. |
| USB/clocks | No differential-pair rule | VERIFIED NOT NEEDED | Length difference 0.06 mm (≈ 0.8 mm with the connector side) → 6 ps against 83 ns bit time. |
| USB/clocks | 3–4 vias without return vias | VERIFIED NOT NEEDED | Return detour ≈ 1 nH → 0.55 Ω against 90 Ω at the 87.5 MHz knee. |
| USB/clocks | R2/R3 at J1 instead of at the chip | VERIFIED NOT NEEDED | = U8-B10 below. |
| USB/clocks | BCLK/LRCK ≈ 52 mm without source resistor | VERIFIED NOT NEEDED; fastest corner MEASURE AT BRING-UP | = U10-B8 below. |
| USB/clocks | `MIC_CLK_M` 76 mm "without source series resistors" | VERIFIED NOT NEEDED | Correction: the statement was wrong. `MIC_CLK_M` has **R39 100 Ω** at the XU316 (3.1 mm from the pin) and **R56 100 Ω** at the buffer U12, then a star to MK1 32.1 mm / MK2 43.9 mm; 100 Ω > Z0, so the line is over-damped. |
| Supply | `+14V` without fuse or TVS | MEASURE AT BRING-UP (MV-6) | Plug-in surge into C89 220 µF: 14.3 V at 0.128 Ω / 1.5 µH; 19.2 V only at 0.05 Ω. A fuse depends on the adapter's current limit, which has no datasheet here. |
| Supply | U3 only 3 V below its 17 V limit | MEASURE AT BRING-UP (MV-6) | SLVSAG7F: V_IN recommended 3–17 V, absolute maximum 20 V; the margin depends only on the adapter. |
| Supply | `+14V` feed to U3 0.3 mm | VERIFIED NOT NEEDED | Correction: it carries only U3's input current, 5 V · 0.8 A / (13.5 V · 0.88) ≈ **0.34 A** → **0.8 K** (IPC-2221), 3.1 mV — not 1.1 A / 12 K; PVDD is fed through R66. |
| Supply | ESP32 VDD3P3 track 0.20 mm | VERIFIED NOT NEEDED | 12.5 mΩ × 340 mA = 4.3 mV against 300 mV margin (3.3 → 3.0 V); 1.7 K. |
| Supply | Loop and FB routing of U6 | VERIFIED NOT NEEDED | C13 2.15 mm, L8 2.48 mm, FB parts 2.9–4.6 mm; no datasheet value exceeded. |
| Supply | No thermal vias in U6's pad | VERIFIED NOT NEEDED | R_θJA 65.3 K/W: 2.9 K at 0.225 A; at the XU316 maximum 0.83 A 16 K (33 K at double R_θJA) → T_J ≤ 98 °C at 65 °C ambient. |
| Supply | AP22802 "not recommended for new design" | VERIFIED NOT NEEDED | Works as drawn; LCSC C211404 in stock (14 426 on 2 Oct 2026). |
| Supply | VDD3P3_CPU limit when burning eFuses | IMPLEMENTED `d1a3583` | Note under "Do not burn `EFUSE_STRAP_JTAG_SEL`": burn only at `ESP_3V3` ≤ 3.30 V. |
| Fabrication | GND island on B.Cu with a 0.086 mm neck | VERIFIED NOT NEEDED | If the neck breaks, both parts keep their own GND vias (2 and 1). |
| Fabrication | Few visible references, small silkscreen texts | VERIFIED NOT NEEDED | Recounted: 25 visible references, 34 texts below 1.0 mm / 0.15 mm; assembly works from the CPL. |
| Firmware | `tas58xx` has no licence file | IMPLEMENTED `2c536a8` | Patched copy distributed under GPL-3.0-or-later, both authors named, modified files marked; the patch is also prepared as a pull request upstream. |

### Pin audit of 1 Oct 2026 — ESP32-S3 (U8)

| Point | Finding | Result | Evidence |
|---|---|---|---|
| U8-B1 | No CLC network at pin 1 (HWG 1.3.6) | VERIFIED NOT NEEDED (matching); harmonic trap C110 IMPLEMENTED as DNP `852b458` | Today's network C28/L4/L3 against the 35 Ω point: mismatch loss **0.55–0.80 dB**; a CLC for 50 → 35 Ω would gain **0.14 dB**. The T network can be tuned by values alone (≤ −19.7 dB calculated). A full CLC at the pin is not placeable without touching the fan-out. Second harmonic: today +0.3 dB relative (no attenuation) → C110 footprint, see "Radio approval". |
| U8-B3 | No series part in `XTAL_P` | NOT PLACEABLE; MEASURE AT BRING-UP (M-RF) | HWG 1.3.5 gives no level; crystal harmonics n = 60/61/62 fall on 2400/2440/2480 MHz. Search for a 0402 place on F.Cu (x 118–121 / y 69–73.5): **0** places. |
| U8-B4 | GPIO45 without external pull-down | VERIFIED NOT NEEDED (H5, `21cd850`) | Pin 51 has no copper; internal 45 kΩ × I_IH ≤ 50 nA = 2.3 mV ≪ V_IL 0.825 V → VDD_SPI 3.3 V. |
| U8-B5 | No 0 Ω footprints in the flash lines | VERIFIED NOT NEEDED | Lines 9.3–25.4 mm against a critical length of 55 mm (t_r 0.65 ns; 28 mm even at drive strength 3) — lumped, nothing to damp. EMI share is covered by M-RF. |
| U8-B6 | GPIO3/4/9/11/12/39/40/48 used in hardware, absent from the YAML | IMPLEMENTED `d1a3583` | Reserve comment in the YAML; `esphome config` valid. |
| U8-B7 | GPIO21 → U19 without pull | IMPLEMENTED (H3, `3d75dfc`) | R81 10 k to GND, 0.33 mA load for GPIO21. |
| U8-B8 | 60 µs high glitch on GPIO18 (`AUDIO_PA_EN`) at power-up | VERIFIED NOT NEEDED | After a /PDN pulse DEVICE_CTRL_2 resets to 0x10 = deep sleep (SLASEH5D 7.6.1.3): the output stage does not switch, no pop. |
| U8-B9 | 60 µs low glitch on GPIO3/4 (`QSPI_CLK`/`QSPI_D0`) | VERIFIED NOT NEEDED | Worst-case contention ≤ 39 mA for 60 µs (7.7 µJ); `voice_kit` resets the XU316 again (1 ms pulse) long after the glitch. |
| U8-B10 | R2/R3 (22 Ω) at J1, 22 mm from the chip | VERIFIED NOT NEEDED | Line ≈ 37 mm = 0.26 ns against a critical length of 283 mm (FS edge ≥ 4 ns); reflection ≤ 2.6 % of the swing (≈ 86 mV) against 0.8 V margin. At the connector the resistors are even the better match. Optional M-USB. |
| U8-B11 | RC on `CHIP_PU` 10 mm from pin 4 | VERIFIED NOT NEEDED | t_STBL = 11.5 ms ≥ 50 µs (230-fold); worst-case coupling 14 mV against 2.48 V needed to reset. |
| U8-B12 | ESP32-S3R8 to 65 °C ambient without PSRAM ECC | MEASURE AT BRING-UP (M-T); firmware prepared `d1a3583` | Same as M13. |

### Pin audit of 1 Oct 2026 — XU316 (U10) and TAS5805M (U14)

| Point | Finding | Result | Evidence |
|---|---|---|---|
| U10-B1 | RST_N releases the XU316 before VDDIO and the flash supply are valid | IMPLEMENTED `5a8c1ce` | **C63 100 nF → 1 µF** (CL05A105KA5NQNC, LCSC C52923, same footprint). Before: boot start at 3.6–4.8 ms with `+3V3` at only 2.44–3.03 V (XU316 DS ch. 13: VDDIO ≥ 2.97 V before reset release) — violated in 5 of 6 corners. Now RST_N released at 8.4–13.7 ms, `+3V3` stable by 4.94 ms. The `voice_kit` pulse still discharges C63 in ≤ 10 µs. Bring-up: 4-channel power-on check. |
| C98 | MPR121 VREG capacitor 1 µF | IMPLEMENTED `5a8c1ce` | **C98 → 0.1 µF** (CL05B104KO5NNNC, LCSC C1525): MPR121 datasheet pin 5, "Connect a 0.1 μF bypass cap to VSS". |
| U10-B2 | `+1V8` bulk only 4.7 µF | IMPLEMENTED (H4, `2414d77`) | C17 10 µF. |
| U10-B3 | Too few paddle vias | IMPLEMENTED (H6, `5a23232`) | 16 GND vias, 4 per VDD paddle. |
| U10-B4 | Signal pads without toe extension | MEASURE AT BRING-UP | Terminal fully covered (heel +0.055, toe +0.005 mm); IPC-7351B is a recommendation, the effect is not calculable; extending is impossible at 6 of 60 pads. X-ray of the first delivery. |
| U10-B5 | Crystal X1: ESR and drive level; C48/C54 20 pF instead of 22 pF | VERIFIED NOT NEEDED (ESR, load); drive level MEASURE AT BRING-UP | SJK 7F24000E12UCG: ESR 60 Ω max, C_L 12 pF — meets XU316 DS 7.3. 1 pF difference ≈ 10 ppm; the XU316 is the I²S clock master, nothing needs an absolute reference. Drive 28–126 µW calculated, no maximum given by SJK. |
| U10-B6 | X0D40–43 enabled as inputs, open | VERIFIED NOT NEEDED | See the low list above. |
| U10-B7 | ESP32 GPIO3/4 permanently on `QSPI_CLK`/`QSPI_D0` | VERIFIED NOT NEEDED | A low glitch cannot change the boot mode (X0D04 must be 0); `voice_kit` re-boots the XU316 after every ESP32 start; YAML note in place. |
| U10-B8 | `I2S_BCLK`/`I2S_LRCK` ≈ 52 mm without source resistor | VERIFIED NOT NEEDED; fastest corner MEASURE AT BRING-UP | The Voice PE has **no** damping resistor there either: X1D01/X1D10 go through R70/R71 = **0 Ω** plus 12 pF to GND. Line simulation (Z0 ≈ 72 Ω): typical edge gives **3.31–3.36 V** at the TAS5805M against the 3.8 V limit (DVDD + 0.5 V, SLASEH5D 6.1); no double clocking in any corner (ring-back ≥ 2.86 V > V_IH 2.31 V). The fastest datasheet edge (0.92 ns) would give 3.92–4.03 V → measured with a scope at bring-up. |
| U14-B1 | PLAY before PBTL/−12 dB | IMPLEMENTED `dab4c7e` | tas58xx patch (K-B4), see section 8. |
| U14-B2 | Loop bandwidth 80 kHz at 768 kHz switching | IMPLEMENTED `c6b3fb3` | Register **0x53 = 0x60** (175 kHz): SLASEH5D table 7-27, "With Fsw=768kHz, 175kHz bandwidth should be selected"; Fsw ≥ 3 × BW holds (768 ≥ 525 kHz). Patch in `firmware/esphome/PATCH.md`. Bring-up: read back 0x53. |
| U14-B3 | `mixer_mode: LEFT` without effect | IMPLEMENTED `dab4c7e` | YAML comment corrected (K-B7); PBTL uses the left frame, the ESP32 sends mono into both slots. |
| U14-B4 | Undocumented registers (0x46 = 0x01 and others) | MEASURE AT BRING-UP | Reserved addresses (SLASEH5D table 7-6). Read back 0x37 = 0x09, 0x38 = 0x40, 0x68 = 0x03, 0x71 = 0x00; A/B test 0x46 = 0x01 / 0x11. |
| U14-B5 | DSP released before the XU316 reset | IMPLEMENTED `dab4c7e` | `setup_priority: 790` below `voice_kit` (799). |
| U14-B6 | AVDD 4.7 µF instead of 1 µF (figure 8-9) | MEASURE AT BRING-UP | Whether 4.7 µF affects the internal regulator is not specified. AVDD at C80 ≥ 4.75 V within 5 ms of PDN↑, ripple < 50 mVpp. |
| U14-B7 | DC bias of the 22 µF / 25 V 0805 parts unknown | VERIFIED NOT NEEDED | Bulk requirement met by C89 alone (220 µF); even at 25 % remaining capacitance the ripple is 62 mV (0.4 % of 14 V). |
| U14-B8 | 4 Ω speaker not verified | VERIFIED NOT NEEDED for R_DC ≥ 1.5 Ω; MEASURE AT BRING-UP | Limit from the inductor I_sat 4.5 A: R_min = 1.43 Ω (datasheet OCE limit 0.74 Ω). Measure R_DC at J5: 2.8–3.6 Ω. |
| U14-B9 | Current rating of J5 | VERIFIED NOT NEEDED | TE 1775443-2: 3 A per contact against ≤ 1.6 A rms, 2.26 A peak. |
| U14-B10 | `tas58xx_control_state_` uninitialised | IMPLEMENTED `dab4c7e` | `{CTRL_HI_Z}` (see `PATCH.md`). |
| U14-B11 | Amplifier faults not visible | IMPLEMENTED `dab4c7e` | `tas58xx` binary sensors (section 8). |

### Pin audit of 1 Oct 2026 — firmware and schematic (K, FW)

| Point | Finding | Result | Evidence |
|---|---|---|---|
| K-B7 | `mixer_mode` comment described an effect that does not occur | IMPLEMENTED `dab4c7e` | Comment corrected. |
| K-B8 | 16 sound files from `raw/dev` | IMPLEMENTED `d1a3583` | Pinned to Voice PE commit `d4e6fa43`; ESPHome checks no hash for `files:`, the commit fixes the content (all 16 byte-identical with the earlier build). |
| K-B9 | Wake-word/VAD models unpinned | IMPLEMENTED `d1a3583` | `hey_jarvis`, `hey_mycroft`, `vad` pinned to micro-wake-word-models `40ff33f`; `okay_nabu`/`stop` exist only as release assets (GitHub `"immutable": false`) → target sha256 noted in the YAML. |
| K-B10 | No `min_version` | IMPLEMENTED `d1a3583` | `min_version: 2026.9.0`, as in the Voice PE. |
| K-B11 | XU316 power-up sequence tight | VERIFIED NOT NEEDED (and C63 changed anyway under U10-B1) | `voice_kit` resets the XU316 on every ESP32 start (1 ms against T(RST) min 5 µs) with all rails settled. |
| K-B12 | No PDN shutdown when the 14 V disappear | VERIFIED NOT NEEDED; MEASURE AT BRING-UP | The TAS5805M goes to Hi-Z at PVDD < 4.2 V while DVDD stays regulated down to `+14V` ≈ 3.5 V — output off before DVDD, no damage; at most a pop. |
| K-B13 | `LED_DATA_3V3` without pull-down | IMPLEMENTED (H3, `3d75dfc`) | R81. |
| K-B14 | Voice assistant used microphone channel 0 only | IMPLEMENTED `d1a3583` | Channels 0 and 1, as in the Voice PE; Home Assistant can stream channel 1 to STT services that disable AGC/NS. Cost 16 384 B PSRAM. |
| K-B15a | No `wifi: on_connect` | VERIFIED NOT NEEDED | The API server drops all clients on Wi-Fi loss; `on_client_disconnected` already calls `control_leds`. |
| K-B15b | `output_power: 8.5dB` | MEASURE AT BRING-UP | Kept; RSSI ≥ −67 dBm keeps 8.5 dB. |
| K-B16 | 11 of 16 sound files unused | VERIFIED NOT NEEDED | Linker map: discarded input sections — **0 B** of flash. |
| K-B17 | tas58xx licence | IMPLEMENTED `2c536a8`; README sentence `d1a3583` | GPL-3.0-or-later; README no longer says "no C++ components". |
| K-B18 | USB alone: speaker silent, undocumented | IMPLEMENTED `d1a3583`; MEASURE AT BRING-UP | README note; PVDD comes only from `+14V` through R66. |
| K-B19 | Rise and current limit of the base's 14 V unknown | MEASURE AT BRING-UP | No source. |
| K-B20 | Level and polarity of MUTE (POWER_IN1.2) unknown | MEASURE AT BRING-UP | No source. |
| K-B21 | Touch electrode capacitance unknown | MEASURE AT BRING-UP | No source. |
| K-B22 | XU316 boot time from U11 unknown | MEASURE AT BRING-UP | XU316 DS gives only T(INIT) 290–480 µs. |
| K-B23 | PDM edge setting of lib_mic_array not visible | MEASURE AT BRING-UP | Submodules missing; MK1 SELECT = VDD, MK2 SELECT = GND (netlist). |
| K-B24 | Register 0x46 = 0x01 undocumented | MEASURE AT BRING-UP | Not in the SLASEH5D register map (7.6). |
| K-B25 | `on_boot` without `control_leds` and 10 min fallback | VERIFIED NOT NEEDED | Own start-up indication; the fallback would only change colours on local events and fight the running `led_ring` effect. |
| K-B26 | Load on J6 depends on the unknown counterpart | MEASURE AT BRING-UP | I²C rise ≤ 300 ns. |
| K-B27 | XMOS README asked for a `voice_kit` text sensor that does not exist | IMPLEMENTED `d1a3583` | Proof is now the log line `[voice_kit] DFU version: 1.3.1`. |
| FW-B1 | `framework: version: recommended` | VERIFIED NOT NEEDED | Under ESPHome 2026.9.0 `recommended` = ESP-IDF 5.5.5 (`esp32/__init__.py` l. 966–969); reproducibility comes from `min_version` (K-B10). |

**Count** (105 rows above): IMPLEMENTED 24 · VERIFIED NOT NEEDED 55 · MEASURE AT BRING-UP 18 as the sole
result. Eight rows carry two: U8-B3 NOT PLACEABLE with measurement M-RF; U8-B1 VERIFIED NOT NEEDED with
C110 IMPLEMENTED as DNP; U10-B5, U10-B8 (and its low-list twin), U14-B8, K-B12 and K-B18 a decision plus a
bring-up check.

### Radio approval: C110 is fitted only after a VNA measurement

The pin audit asked for a CLC network at the ESP32's RF pin (U8-B1). The decision: **the matching
network stays** (C28 1.2 pF series, L4 2.7 nH shunt, L3 3.3 nH series; mismatch loss 0.55–0.80 dB,
a CLC would gain 0.14 dB). What today's network does not do is attenuate the **second harmonic**
(calculated +0.3 dB relative to the carrier; the ESP32-S3 Hardware Design Guidelines ask for S21 <
−35 dB at 4.8/7.2 GHz). The harmonic level of the chip itself is not specified, so the emission
cannot be calculated.

* **C110** — shunt trap on `LNA_IN`, 1.0 pF C0G 0201 (Murata GRM0335C1H1R0BA01D, LCSC C85893), on
  F.Cu at 119.15 / 74.40, 180°, pad 1 with a 0.2 mm stub to the `LNA_IN` track, pad 2 to a GND via
  at 118.35 / 74.40 (commit `852b458`). Calculated with ≈ 1.05 nH stub inductance: second harmonic
  −24 dB relative instead of +0.3 dB. **It is placed as a DNP footprint and not fitted.** `LNA_IN`
  itself is unchanged (5.95 mm, 0.30 mm); the unfitted stub is < 1 % of the guided wavelength.
* **Fit only after measurement:** VNA match (MV-3: |Γ| against 35 Ω ≤ −10 dB over 2.400–2.484 GHz)
  and harmonics (MV-5: ≤ −30 dBm ERP per ETSI EN 300 328, i.e. ≥ 50 dB below a 20 dBm carrier).
  If MV-5 fails, fit C110, re-tune C28/L4/L3 on the VNA (EVB starting point 2.4 pF / 2.0 nH /
  4.7 nH), and repeat MV-5 (second harmonic ≥ 20 dB better than without C110) and MV-4 (RSSI not
  worse).
* **Radio approval applies to the fitted state.** Whatever is decided — C110 fitted or not, and the
  final C28/L4/L3 values — the radio tests for approval have to be run on exactly that populated
  board. Changing the RF fit after the tests means testing again.

## Known issues and notes (rev A2)

### Distance between the crystal Y1 and the ESP32

Espressif's hardware design guidelines for the ESP32-S3 ask for two things that pull in opposite
directions on this board: the crystal should sit at least 2.0 mm away from the chip
("PCB Layout Design > Crystal"), and the antenna should be kept away from the crystal
("RF" and "Typical Layout Problems"). Y1 sits between the ESP32 and the chip antenna, so every
millimetre gained on one side is lost on the other. Measured pad edge to pad edge:

| | crystal to chip | crystal to antenna |
|---|---|---|
| rev A1 | 1.00 mm | 1.77 mm |
| rev A2 | 1.30 mm | 1.47 mm |
| Gen1 board (same chip, same antenna type) | 1.30 mm | 4.18 mm |
| guideline | >= 2.0 mm | "keep away", no figure given |

**Done in rev A2:** Y1 was moved 0.30 mm away from the chip. That reaches the Gen1 figure of
1.30 mm and costs 0.30 mm towards the antenna (1.77 -> 1.47 mm). Four ground stitching vias
(0.4 mm pad, 0.3 mm drill) were added between the crystal and the antenna feed, all of them
outside the antenna keepout area. The load capacitor C27 stayed where it is: an existing ground
via at 117.30/67.10 mm blocks the place it would have moved to, so the track from Y1 pin 1 to
C27 takes up the 0.24 mm offset with a 45 degree bend and is 0.08 mm longer than before. `XTAL_P` is now 3.61 mm and `XTAL_N` 6.86 mm long, both on F.Cu
without vias, with a load capacitor at each end and an unbroken ground plane on In1.Cu under the
crystal. Going the full 2.0 mm would push the crystal to 0.72 mm from the antenna, which the
guidelines advise against. **Decided 2 Oct 2026: kept as it is.** The pin audit found no free place
for a series part in `XTAL_P` either (U8-B3, 0 places for a 0402); whether the crystal harmonics at
2400/2440/2480 MHz reach the receiver is measured at bring-up (M-RF: throughput on channels 6 and 13
at least 70 % of channels 1/11 at equal RSSI; MV-4: RSSI against a reference board). If it fails, the
access point is set to channel 1 or 11.

### Do not burn `EFUSE_STRAP_JTAG_SEL` (ESP32-S3)

Rev A2 uses **GPIO3** and **GPIO39 to GPIO42** as an SPI bus from the ESP32 to the XMOS flash
(see "Switching the XMOS flash between the two processors"). Both groups are listed as priority 3
pins in the ESP32-S3 datasheet: GPIO3 is a strapping pin for the JTAG signal source
(section 3.4), GPIO39 to GPIO42 are the JTAG pins MTCK, MTDO, MTDI and MTMS (section 2.3.4).

In the factory state this is not a conflict. All three relevant eFuses (`EFUSE_DIS_PAD_JTAG`,
`EFUSE_DIS_USB_JTAG`, `EFUSE_STRAP_JTAG_SEL`) are 0, JTAG runs through the USB-Serial/JTAG
controller and GPIO3 is "ignored" (table 3-5). Table 3-1 lists GPIO3 as *floating*, and the
datasheet states it "does not have any internal pull resistors".

**Therefore: never burn `EFUSE_STRAP_JTAG_SEL` on these boards.** eFuses are one-time
programmable. Burning it would make GPIO3 a strapping input that must not float and would move
JTAG onto GPIO39 to GPIO42 — both conflict with the SPI bus. Debug the ESP32 over USB instead.

If an eFuse ever has to be burned (for example the VDD_SPI fallback noted under U8-B4/H5), measure
`ESP_3V3` first: the ESP32-S3 datasheet (table 5-2, note 3) limits VDD3P3_CPU to 3.3 V while writing
eFuses, and the TPS62130 rail is 3.19–3.36 V (nominal 3.265 V). Burn only if the measured value is
≤ 3.30 V.

### J4 is now a pad field on the microphone side

Upstream J4 was a 2x5 footprint on the LED side with 1.70 mm pads at 2.54 mm pitch, pin 4 on
`+5V` and pin 6 on `VDDIO` (3.3 V). Rev A2 moves it to the **microphone side** as a pad field
with **0.80 mm pads at 1.27 mm pitch** — the pitch of the XMOS xSYS2 connector
(XA-XTAG4 datasheet XM-014675-DS), footprint `Onju:xSYS2_2x05_P1.27mm_Pads`, SMD, no drilled
holes.

**Final position (E3b): 137.070 / 56.730** (rotated 90° at E3b, −90° since K2 below), with C103 at 141.370 / 56.095. An earlier
intermediate position put the pad field inside the band y 73 … 90, where it blocked every route
through the east region; moving it to the microphone side shortened the five JTAG lines from
**150.41 mm to 22.89 mm** (RST_N 5.53, TMS 4.72, TDI 4.02, TDO 4.25, TCK 4.37 mm, one via each)
and opened the band — flooding from U8.17 grew from 110.8 mm² to 2 211.2 mm². `+1V8` from J4.6 via
C103 to C45.1 is 10.94 mm with no via. A GND stitching via at 136.200 / 57.300 that ended up under
the new pad J4.3 was moved to 140.600 / 57.300 (working notes: AENDERUNGEN, sections 97 and 98).
Re-measured in the board file on 1 Oct 2026, after K2, the signal tracks are TMS 4.72, TCK 4.37,
TDO 4.25 and TDI 4.23 mm with one via each.

**Pin-out corrected in the audit fixes (1 Oct 2026, K2).** Until then pin 4 was left unconnected,
pin 6 carried `+1V8`, and the signals sat on the odd pins — the xSYS2 pin-out rotated by 180°, but
with VREF in the wrong place: an XTAG4 plugged on 1:1 did not work, and plugged on rotated it would
have shorted `+1V8` to `GND`. J4 now follows the **xSYS2 JTAG-only header** of the XU316 datasheet
(appendix G.3, figure 25): **1 VREF (`+1V8`), 2 TMS, 4 TCK, 6 TDO, 8 TDI, 10 RST_N, 3/5/7/9 GND**.
The fix is a 180° rotation of the footprint (now −90°), the tenth pad renamed from `MP` to `10`,
and new nets only on the row at y = 56.095: pin 1 `+1V8`, pins 5 and 7 `GND`. The five JTAG
signals keep their pads and tracks. C103 (100 nF) sits 0.6 mm from pin 1; the silkscreen dot of the
footprint marks pin 1. The schematic symbol is now `Connector_Generic:Conn_02x05_Odd_Even`.
The JTAG group of the XU316 runs at VDDIOB18 = 1.8 V and the XTAG4 takes its reference voltage from
the target ("pin 1 to VDDIOB18 (with a decoupler)").

That removes the risk of feeding 5 V or 3.3 V into the adapter's level shifters. J3 ("XTAG DEV",
xLINK) and TP10 are removed, and so are **R37 and R38** (22 R): they were the series resistors of
the xLINK pair that went to J3, so with J3 gone they had nothing on the other side. U10 pins 36 and
37 now carry a `no_connect` marker in the schematic (E5.4, `0390f3a`). The pad field is not populated (DNP); it is meant for spring contacts or a hand-soldered
header.

### `AUDIO_PA_EN` crosses the QSPI bus on an inner layer (rev A2)

Rev A2 adds a bus switch (U16) that splits the QSPI bus between the XU316 and the flash, so the ESP32
can reach the flash as well. It sits west of the bus, and the corridor between its pads and the bus
was occupied by `AUDIO_PA_EN`, which ran vertically on the LED side at x = 120.1 from y = 73.8 to
78.7.

`AUDIO_PA_EN` now crosses **underneath** the bus on **In2.Cu**: LED side up to (126.200/72.700), a new
via at (126.100/72.700), then In2.Cu to (124.200/74.600) and (124.200/80.600), where the existing via
takes it back to the microphone side. The net gets shorter (58.05 → 50.49 mm, 16 → 14 segments, two →
three vias) and the corridor is free.

The price is a 8.69 mm long, 0.404 mm wide slot in two power planes on In2.Cu. It was measured before
and after:

| Plane | Effect |
|---|---|
| `ESP_3V3` on In2.Cu | 173.04 → 165.46 mm², **still a single island**. The paths from the supply island (L5, C36–C40) to *every* supply pin of U8 are unchanged (5.50 / 9.16 / 9.02 mm), and so is the narrowest point (1.000 mm). Only two small loads run around: the pull-up supply at R33 (6.08 → 14.26 mm) and the area near (132.40/76.44) (11.45 → 18.60 mm) |
| `+3V3` on In2.Cu | 228.83 → 227.16 mm² (−0.7 %), **islands 3 → 3**, all paths unchanged. This plane is touched because the `ESP_3V3` fill ends at y = 77.8 while the bus has to be crossed at y = 78.105 — there is no route that avoids it (checked on both outer layers, with a southern entry point, and with In2.Cu blocked south of y = 77.5) |
| `GND` on In1.Cu | 753.89 → 753.38 mm² — the new drill hole only, **no slot**, islands 2 → 2 |
| `GND` on B.Cu | 535.84 → **543.06 mm²**: the freed space is taken by the ground fill, islands 23 → 22 |

Every supply pin of U8 keeps its decoupling capacitors on the outer layers with its own vias
(pads 55/56 ← C31, C32; pad 46 ← C33; pads 20/29 ← C30, C34, C35), so the decoupling does not depend
on the slotted area at all.

**Decided 2 Oct 2026: kept as it is.** Running the QSPI lines through the switches in series (as
Satellite1 does) would avoid the crossing, but it means re-routing inside the ESP32 fan-out, and no
measurable effect of the crossing has been shown: `ESP_3V3` stays one island, every supply path of U8
is unchanged, and the decoupling does not depend on the slotted area. Emission is checked at bring-up
(MV-1).

### QSPI bus isolation uses one switch, and `QSPI_CLK` runs in the ground plane (rev A2)

Rev A2 lets the ESP32 write the XU316's flash, so that the board can be programmed for the first
time without a tool — once the ESP32-side flasher exists. **It does not exist yet** (audit A1.2h);
until then the first image goes in over J4 with an XTAG4. Satellite1 switches all four bus lines; here only **two** are switched, because the
single west–east corridor past the flash carries two tracks and a second switch would block it:

| Line | rev A2 | ESP32 pin | stub |
|---|---|---|---|
| `QSPI_CS_N` | switched by U16 (2D ↔ D) | GPIO39 | 2.42 mm |
| `QSPI_D1` (MISO) | switched by U16 | GPIO40 | 6.50 mm |
| `QSPI_D0` (MOSI) | permanently connected | GPIO4 | 13.72 mm |
| `QSPI_CLK` | permanently connected | GPIO3 | 7.95 mm |

U16 is a TS3USB221E (LCSC C129313, alternative C128396); 1D is left open, `S` is driven by GPIO46 and
`OE` is tied to GND. C101 (100 nF) decouples the supply and sits at (123.600/72.000).

**Supply: U16 and C101 run from `ESP_3V3`, not from `VDDIO`.** Both nets are the same 3.3 V out of
`+3V3`, each behind its own 120 Ω ferrite (FB2 for `ESP_3V3`, FB4 for `VDDIO`, both
BLM18PG121SN1D: 2 A rated, 50 mΩ DCR max), and neither can be switched off. `VDDIO` would have been
the obvious choice — it is what the bus and the flash run on — but it has no route to this spot: the
corridor between U8, the bus and the crystal keepout carries either the three signal tracks or a
supply line, not both. Four routing attempts were made (working notes, AENDERUNGEN 63.3); the shortest
`VDDIO` feed was 18.97 mm and left `XMOS_SPI_CS_N`, `XMOS_SPI_MISO` and `XU316_RST` unroutable. The
`ESP_3V3` fill on In2.Cu lies directly under U16, so one via at (123.100/72.650), 0.65 mm from
C101, feeds the switch instead.

The switch tolerates this, per SCDS220M (November 2024):

* Its I/O pins are rated **absolutely, not relative to VCC** — recommended 0 to 5.5 V (section 5.3)
  and absolute maximum −0.5 to 7 V (5.1), with the note that they are 5.5 V tolerant over the whole
  range. Signals at `VDDIO` level on a switch powered from `ESP_3V3` are therefore within spec by a
  wide margin; in operation the two rails differ by at most the drop across their ferrites, tens of
  millivolts.
* With **VCC = 0 V** and up to 3.6 V on the I/O pins, I(OFF) is at most **±2 µA** (5.5), so the
  power-up order of the two branches does not matter.
* The one pin that *is* limited to VCC is the control input: V(IH) max = VCC (5.3). It is driven by
  GPIO46 of the ESP32 — which after this change shares the supply net with U16's VCC, so the limit
  holds by construction. With VCC on `VDDIO` it would have depended on which rail sat higher.
* I(CC) is at most **30 µA** (1 µA with `OE` high, 5.5). Against the ferrite's 50 mΩ that is a drop
  below a microvolt, and against its 2 A rating it is no load at all.

U16 pins 5 and 6 and C101 pin 2 need no dedicated ground via: all three sit inside the same ground
fill on B.Cu, which carries 23 ground vias in this area — the nearest is 1.02 mm from C101 pin 2,
2.58 mm from U16 pin 6 and 2.71 mm from U16 pin 5, and each one reaches the ground plane on In1.Cu.

**Because `QSPI_D0` and `QSPI_CLK` stay connected,** firmware must keep GPIO3 and GPIO4 as inputs
whenever the XU316 talks to its own flash; otherwise the ESP32 would drive a running bus. The XU316 is
held in reset by `voice_kit` after the ESP32 boots, so that window is well defined. All four stubs were
checked against the round-trip criterion (≤ t_r/3 … t_r/6): the limit is 15 mm and every stub is below
it.

`QSPI_CLK` reaches the bus on **In1.Cu, the ground plane** — microphone side to (124.900/72.700), via,
6.33 mm on In1.Cu to (130.050/75.550), via, then LED side to the flash pad. Routing it on In2.Cu would
have been easier to draw but is much worse electrically; both were measured:

| | `QSPI_CLK` on In2.Cu | `QSPI_CLK` on In1.Cu (chosen) |
|---|---|---|
| stub length | 8.33 mm | **7.95 mm** |
| slotted plane | `ESP_3V3`, **cut in two** (165.46 → 156.77 mm², islands 1 → 2) | `GND`, **stays whole** (753.38 → 747.73 mm², islands 2 → 2) |
| fast nets crossing the slot on the layer that references the slotted plane | **6** — `QSPI_CS_N`, `QSPI_D1`, `QSPI_D2`, `QSPI_D0`, `I2S_BCLK`, `I2S_LRCK`, all on B.Cu | **0** |
| return-current detour inside the reference plane | **none available** — the two sides of the slot end up in different islands | 4.90 mm (`I2C_SCL`) and 5.83 mm (`Net-(U8-GPIO7)`, the ESP32 side of `I2S_LRCK` behind R29) |
| `ESP_3V3` on In2.Cu | cut in two | 165.46 → 158.92 mm², **islands 1 → 1** (drill holes only) |
| DRC | one extra unconnected item (`ESP_3V3`, between two teardrops on F.Cu) | unchanged |

In this stack-up (F.Cu — 0.2104 mm prepreg — In1.Cu — 1.1 mm core — In2.Cu — 0.2104 mm prepreg — B.Cu)
F.Cu references In1 and B.Cu references In2, so the choice of inner layer decides which nets are
affected. The usual rule against slotting a ground plane aims at slots that cut the plane and force
long detours; here the plane stays whole and only two nets at or below 400 kHz cross it. No power net
comes near: the switching nodes of U3, U6 and U7 are 6.2 mm or further away, the class-D outputs of
U14 are 26 mm away, and the separate analogue ground `GNDA` spans y 94.9…118.3 while the slot lies at
y 72.7…75.6.

**These figures come from the bus specifications (I²C fast mode, I²S at 48 kHz), not from measurements
on a built board.**

While routing, `QSPI_D0` on B.Cu cut a 3.56 mm² island out of the ground fill that contained **U11.4,
the flash's ground pin, with no via in it** — the flash ground would have been left floating. A ground
via at (124.950/75.050), 1.54 mm from the pad, fixes it.

The flash keeps its `W25Q32JVSSIQ`: the ESP32 can set the QE bit itself, so flash types without a
factory-set QE bit are usable as well.

**Decided 2 Oct 2026: kept as it is.** Switching all four lines in series would need the corridor
that is not there (see above); the one slot is in GND on In1, which stays whole, and only two nets at
or below 400 kHz cross it. `QSPI_CLK` runs only at boot and during DFU; the rough emission estimate
for it is the only one that reaches the CISPR 32 class B limit, so it is measured at bring-up in a
boot loop (MV-1, audit point M4). The firmware rule stays: GPIO3 and GPIO4 are never driven (YAML note
U10-B7).

### U4 (AP22802AW5-7) is not recommended for new designs

Every page of the AP22802 data sheet (Diodes Incorporated) carries the note
"NOT RECOMMENDED FOR NEW DESIGN - USE AP22811". The part is still listed and in stock at the
assembly house, and it works as drawn: `EN` is active high (AP22802**A**; the B suffix is active
low), V(IH) >= 1.5 V, V(IL) <= 0.5 V, reverse leakage with the switch disabled 0.01 uA typical.

**Decided 2 Oct 2026: kept (VERIFIED NOT NEEDED).** The function is unchanged and the part is stocked
(LCSC C211404, 14 426 in stock on 2 Oct 2026). Should it ever become unavailable, the successor AP22811
has to be checked for pin assignment, enable polarity and current limit before it replaces U4.

### Naming of the two board sides

Throughout this file, **F.Cu** is the microphone side (MK1/MK2, U8, U14) and **B.Cu** is the LED
side (LED1-LED6, U10, U11, U15). In the enclosure the LED side faces up.

### Vias in the large pads of U8 and U10

26 vias sit in the large pads:

* U8: 9, in the exposed pad;
* U10: 17, 5 in the exposed pad and 12 in the four VDD bars.

All of them come from upstream. The five vias in small pads were moved out (V1). (An
earlier version of this file gave the counts as U8 10 and U10 16; the correct counts are
9 and 17. The total of 31 before V1 was correct.)

These pads have no solder mask over the vias, so the holes are open. Solder paste lies
over all 17 holes in U10 and over 4 of the 9 holes in U8. The vias must therefore be
ordered resin-filled and copper-capped (see README, "Vias in pads").

**Kept as it is; no measurable effect shown.** The vias are ordered filled and capped ("Before you
order", point 2), which closes the holes before the paste is printed; splitting the paste apertures
was not examined further. The X-ray of the first delivery (pin-audit point U10-B4) shows the joints.

### Via holes inside a pad's solder-mask opening

Two SMD pads have a via whose ring overlaps the pad copper and whose **drilled hole lies
inside the pad's solder-mask opening** (pad plus 0.03 mm, at J1 0.051 mm):

| Pad | Net | Via |
|---|---|---|
| C65.1 | +5V | 135.542 / 92.500 |
| J1.B6 | USB_D_P | 152.000 / 66.400 |

Both come from upstream. Two more pads, C38.1 and C39.1, share one upstream via at
121.300 / 77.285: its ring touches both pads, but the hole stays 0.004 and 0.016 mm outside
the openings.

* **The ordered via covering takes care of these.** All vias are ordered resin-filled and
  copper-capped (see README), so the holes are closed and plated over before assembly.
* Checked against all 815 SMD pads and 397 vias: together with the 26 vias in the large pads
  of U8 and U10 these are the only holes that reach into a mask opening, 28 pairings in all.
  Counting also holes that reach only partly into an opening (audit, 1 Oct 2026), seven more pads
  join the list: J4.4, J4.6, LED2.1, TP21, TP22, U19.2 and U19.4. The filled and capped vias cover them as well.
* The three cases from this fork (R23, R29, R30) were fixed by V3.

### Via annular ring is at the minimum

All 397 vias are 0.3 mm hole / 0.4 mm diameter, which gives a 0.05 mm ring on all four
layers. That is JLCPCB's absolute minimum ("via diameter should be 0.1 mm larger than the
via hole size"); the preferred value is 0.15 mm larger, that is a 0.075 mm ring. The size
comes from the upstream design and is within the ordered JLCPCB option, so it is kept as it
is.

**Kept as it is; no measurable effect shown.** 0.4 mm vias are within the ordered JLCPCB option;
going to 0.45 mm would touch all vias and every clearance on the board.

### U12 channel 1 is wired wrong

This error comes from upstream. On U12 (SN74LVC125A), pin 1 (`1OE`) is on
`MIC_DATA_M_2`, the MK3 data line, and pin 2 (`1A`) is on `MUTE_ON`, so data and enable
are swapped. Channel 3 of the same part is wired correctly.

* **It has no functional effect.** Since B6, `1OE` is on GND, so the channel is permanently
  enabled and `1Y` follows `MUTE_ON`. `1Y` drives `MIC_DATA_2`, and after C4 that net goes to
  no XU316 port. All inputs are at defined levels.
* The open `1OE` input found earlier is **fixed** (B6, `b74d48b`).
* Since E5.1 the MK3 branch is removed altogether, so the swap now describes a channel with
  nothing on either side of it. **Decided 2 Oct 2026: kept (VERIFIED NOT NEEDED)** — 1OE is on GND,
  every input sits at a defined level and the output has no load.

### The third microphone MK3 — removed in rev A2 (was a known issue)

Upstream has a footprint and a branch for a third microphone that is not used: after C4,
X1D11 carries `I2S_MCLK`, so there is no XU316 port left for it. In rev A1 the branch was
set to DNP, which left DRC reporting two unconnected items (MK3 pad 4 on `MIC_CLK_M`, and a
`VDD_MIC` stub at C73/R64).

**Rev A2 removes the branch** — MK3, C73, R61, R51, R64 and the label `MIC_DATA_M_2` (E5.1,
`5513330`). The two unconnected items are gone with it, and the non-plated 0.5 mm hole for
MK3 is gone from the drill file. If a third microphone is ever wanted, it needs a free XU316
port first, which means giving up something else — see the port table in section 1.

### Back-feed from USB into +14 V — solved in rev A2 (was A4)

The problem: +5V back-fed into +14V **on USB alone**, not only when both supplies were connected.
With the barrel connector unplugged, +5V reached the `SW` node of U3 through L1, the body diode of
the high-side switch conducted, and the whole +14V rail including PVDD was charged (TPS62130,
SLVSAG7F, page 4).

**The operating rule "never connect USB-C and the 14 V supply at the same time" no longer
applies.** Rev A2 solves it in hardware, with two parts:

**1. An ideal diode in the output of U3.** U18 (LM66100DCK, SC-70-6, LCSC C2869734) sits between
the L1 node and the +5V rail. The node in front of it is the new net `+5V_SW`; behind it the rail
keeps the name `+5V`.

* `CE` is tied to `VOUT`, which is what the datasheet asks for reverse-current blocking
  (SLVSEZ8A, section 5 pin functions and section 8.3.2). Turn-off threshold V(OFF) =
  V(CE) − V(IN) is 0 / 35 / **80 mV**.
* On USB alone, `+5V` sits about 4.9 V from U4 while `+5V_SW` is near 0 V, so U18 switches off and
  nothing reaches U3. Reverse current through the switched-off U4 is at most 1 µA (AP22802).
* On the 14 V supply, U18 conducts. The drop is 63 mV at 0.8 A with the typical 79 mΩ, 113 mV with
  the maximum 141 mΩ — `+5V` stays at 4.94 V or 4.89 V, well above every load's minimum (LED
  3.7 V, regulators from 2.5 V).
* **`VOS` of U3 and its feedback network (R11 with C9, R12, R13) stay in front of the diode**,
  together with the output capacitors C10 and C11 and the test point TP3. If they sat behind it,
  U3 would regulate the voltage after the diode and, on USB, would see externally fed voltage at
  the feedback pin of an unpowered regulator — the TPS62130 datasheet does not say that is
  allowed. The price is that the 60 to 115 mV are not regulated out, which does not matter here.
* **U7 (the 3.3 V regulator) sits behind the diode.** That is essential: U7 feeds the ESP32 over
  FB2 and the XMOS side over FB4, so on USB alone it has to run from what U4 supplies. In the
  chip layout U7 hung on the L1 node, in front of the diode — that had to be separated.

**2. A lock on the USB switch.** Q4 (LBSS138WT1G) pulls `USB_EN` to ground as long as +14V is
present, so U4 cannot feed the rail at all: R76 (1 MΩ) and R77 (330 kΩ) divide +14V down to
V(GS) = 3.47 V, safely above V(GS(th)) max 1.5 V, at a quiescent current of 10.5 µA. `EN` then
sits far below V(IL) 0.5 V. Without +14V the gate is at 0 V, Q4 blocks, and R9 pulls `EN` to
V(BUS), above V(IH) 1.5 V — U4 is free.

When the 14 V supply is added while USB is already connected, the gate follows through
R76 ∥ R77 = 248 kΩ with a time constant of about 12 µs. U4 may stay enabled for that long, but no
current can flow back into U3 because U18 is off while `+5V` sits above `+5V_SW`.

* **Bring-up:** both supplies may now be used, also at the same time. The 14 V supply wins.
* Programming the XMOS over JTAG (J4) needs no USB connection and is unaffected.
* **Test points:** TP3 is on `+5V_SW`, a second one on `+5V` behind U18. Expected difference under
  load 55…65 mV at the typical R(DSon), and TP3 near 0 V on USB alone.
* Nothing outstanding here; the SS34 blocking diode proposed earlier is no longer needed.

### Y1 has no model in the JLCPCB preview

EasyEDA has no footprint for C424431 (40 MHz, 2016, 4 pins), so the JLCPCB assembly preview
shows nothing to check Y1 against. On a 4-pin crystal a 90° error swaps the crystal pins
with the ground pins. Pin 1 (`XTAL_N`) must be at the top left (see
`production/jlcpcb/ROTATION_CHECK.md`). Kept as it is: the orientation is checked by hand in the
assembly preview against the target table.

Since the pin-audit fix H1 (2 Oct 2026) Y1 is the JLYE Y201640MDBCX (C49158179). EasyEDA has no
model for that number either ("Component not found", checked 2 Oct 2026), and the JLCPCB preview
shows only a placeholder. Pin 1 (`XTAL_N`) stays top left, at the corner with the silkscreen corner
mark (measured from the board: mark 117.03–119.57 / 67.83–70.77 mm, pad 1 at 117.75 / 68.60). The
order form carries the remark **"Y1 orientation per silkscreen pin-1 corner mark (top-left)."**

### Four rotations corrected after the JLCPCB preview (2 Oct 2026)

The order was stopped when the assembly preview showed U18 with pin 1 top left instead of bottom
left (input and output swapped) and U16 with pin 1 bottom right instead of bottom left. Every
polarised or multi-pin part — 42 rows, not just the two reported — was then recomputed against
the EasyEDA footprint of its LCSC number (JLCEDA/EasyEDA Official Library, https://lceda.cn/,
https://easyeda.com; tool `fertigung/werkzeug/easyeda_abgleich.py`). Four rotations were wrong
and are corrected in `fertigung/werkzeug/jlc_korrektur.csv`:

| Part | LCSC | Side | CPL before | CPL now | Match |
|---|---|---|---|---|---|
| U18 | C2869734 | top | 270° | **0°** | 6/6 pads in order; EasyEDA rows 0.21 mm further out, symmetric |
| U16 | C129313 | bottom | 90° | **0°** | 10/10 pads, max. 0.04 mm |
| U5 | C133796 | top | 0° | **270°** | 5/5 pads, max. 0.13 mm |
| Q4 | C383201 | top | 0° | **180°** | 3/3 pads, max. 0.03 mm |

The other 38 rows agree. R35 has a new part number, C100510 (LIZ Elec CR0402FF6800G, 680 Ω 1 %),
replacing C25130 at order time. Details: `fertigung/DREHLAGEN_PRUEFEN.md`.

### Footprint origins not at the pad centre — fixed in rev A2

In rev A1 four footprints had their KiCad origin away from the centre of the pads, and the CPL
file had to be patched by hand for each of them:

| Part | Offset in rev A1 | Now |
|---|---|---|
| X1 | 3.0 mm (EasyEDA origin = body centre) | origin at the body centre, which is what the assembler references |
| J5 | 0.4 mm | origin at the pad centre |
| J1 | 0.355 mm (pad centre including the shield tabs) | marked as an SMD footprint, so the exporter uses its origin |
| U1 | 0.55 mm | the part is gone (E1) |

**Rev A2 corrects the footprint instances instead of the export** (E5.3, `a24ebfb`). The pads did
not move: their coordinates are bit-identical before and after. The CPL exporter
`fertigung/werkzeug/ft_cpl.py` now takes the origin for `FP_SMD` footprints and the centre of the
pad bounding box otherwise, so no hand correction is left. The correction table
`fertigung/werkzeug/jlc_korrektur.csv` is kept, versioned and empty of entries for these four
parts; if a part listed in it is missing from the board, the exporter says so and skips it
(`4ec7b50`).

### A track ran lengthwise under the whole pad row of J6 (rev A2, fixed)

`J6` is the 12-pin 0.5 mm FPC connector (HRS FH34SRJ-12S-0.5SH) added in rev A2. Its pads are
0.80 × 0.30 mm, front side only, 0.5 mm pitch.

In an intermediate state of rev A2 the `IR_RX` track on the back side ran at x 159.1 straight
under all twelve pads — 0.025 mm from the pad centres, where `via diameter/2 + clearance`
= 0.2 + 0.127 = 0.327 mm is needed for a via in a pad. That made **every** signal pad of the
connector impossible to via, and it walled in pins 6, 7 and 10 so that they could not be
connected at all. The descent was moved east of the pad row (x 159.45 … 159.85, 6.28 mm, no
via); the smallest distance from a foreign back-side track to a pad centre is now 0.325 mm.

**Rule for any further work on this connector:** no track may run lengthwise
under a fine-pitch pad row on the opposite layer — crossing is fine. Two vias in adjacent pads
of a 0.5 mm pitch row are impossible in any case (0.577 mm needed, so they must be offset by at
least 0.288 mm in x).

**All twelve pins are connected in rev A2** — but only through a narrower net class. The story is
worth keeping, because it says what the board's geometry allows.

The whole J6 area is reached through a **single channel** on the back side, in the south-east around
165.5 / 86.0. Flooding on B.Cu from pin 6 outwards (working notes, AENDERUNGEN 99.4 and 101.3) reaches
49.8 mm2 for a 0.15 mm track and collapses to 8.1 mm2 at 0.35 mm: the maximin width of the channel is
**0.298 mm** with this fork's own tracks in place, and **0.345 mm** with both freshly routed nets
(`IR_TX`, `UART_RX`) taken out of the obstacle model. Two 0.15 mm tracks side by side need
2 x 0.15 + 0.127 = **0.427 mm**. So the channel carries one 0.15 mm track, not two - and that is not
because of this fork's copper:

* with the own tracks in place, the narrow point is the diagonal 165.50 / 86.00 -> 166.00 / 86.50,
  formed by the `/XMOS/MIC_CLK_M` track on B.Cu (0.279 mm away) and the `IR_TX` via at
  165.500 / 86.700 (0.315 mm away); clear width 0.593 mm where 0.681 mm would be needed;
* take those away and the next narrow point appears, limited by the **copper ring of touch electrode
  `Net-(U15-ELEC2)`** - centre 162.810 / 89.100, radius 2.500 mm, width 1.500 mm, so it occupies the
  annulus r 1.75 ... 3.25 mm - at 0.301 mm, together with the board outline. The ring is one of the
  three touch electrodes (see E1) and comes from the upstream design.

Moving the `IR_TX` via does not help either: the direction in which clearance to `MIC_CLK_M` grows
runs straight into that ring. At 0.05 mm offset there is already no room for a via, and from 0.40 mm
the via would sit inside it. Of 1 259 positions with enough clearance, none has room.

**What does work is a narrower track.** `UART_TX` and `UART_RX` were given the net class
**`UART-eng`** (track 0.100 mm, clearance unchanged at 0.127 mm) and re-routed. Two 0.10 mm tracks
side by side need 0.327 mm of substitute width at 0.127 mm clearance - less than the 0.345 mm the
channel carries. Measured: two 0.10 mm tracks get through (18 098 grid nodes, y down to 85.70), one
0.10 mm plus one 0.15 mm does not (3 146 nodes, y only to 90.35).

JLCPCB fabricates this without a surcharge. Both pages retrieved 30.09.2026:
`jlcpcb.com/capabilities/pcb-capabilities` gives "0.09 / 0.09 mm (3.5 / 3.5 mil)" for multilayer
boards with 1 oz outer copper, and `jlcpcb.com/help/article/in-what-cases-will-there-be-charged-extra`
charges extra only "if the trace width or spacing of multi-layer boards is 3.0-3.5mil" (20 % for
4-8 layers). **0.10 mm is 3.94 mil**, above that threshold - the margin is 0.011 mm, so anyone who
does not want to cut it that fine uses 0.102 mm (exactly 4.0 mil).

Net classes alone are not enough: the **board constraints** `min_track_width` and `min_connection`
are 0.127 mm, and a net class does not override them. With 0.10 mm tracks the DRC first reported 348
violations (199 `track_width`, 116 `connection_width`). Rather than lowering the constraints for the
whole board, a rule file **`nest-mini-v2-drop-in-pcb.kicad_dru`** relaxes them for this one net class
only. That brought the count back to 33.

Only the narrow stretches are 0.10 mm wide. Every segment with room for 0.15 mm was set to 0.15 mm,
measured segment by segment: `UART_RX` carries 28.48 mm at 0.10 and 30.43 mm at 0.15 mm, `UART_TX`
49.11 mm at 0.10 and 43.17 mm at 0.15 mm.

`UART_RX` ends up **shorter** than in the 0.15 mm version - 58.91 mm instead of 69.07 mm, 4 vias, no
via in a pad. `UART_TX` is 92.28 mm with 8 vias, from pad U8.17 south through the J4 area, then east
and through the channel.

**Kept as it is; no measurable effect shown.** Widening the channel (moving the ELEC2 ring or the
screw hole at 162.815 / 89.076) would let both signals keep 0.15 mm, but 0.10 mm is fabricated without
surcharge and the UART lines are slow; the load on J6 is checked at bring-up (K-B26).


### Vias inside pads (rev A2)

`J6` is the 12-pin 0.5 mm FPC connector (HRS FH34SRJ-12S-0.5SH) added in rev A2. Its pads are
0.80 x 0.30 mm, front side only.

Three **signal** nets are reached through a via inside a pad (all of them same-net — no foreign net
in any pad):

| Pad | Net | Via | Drill / diameter | Note |
|---|---|---|---|---|
| J6.11 | `IR_RX` | 159.100 / 91.550 | 0.3 / 0.4 mm | FPC pad, 0.30 mm wide |
| R79.2 | `IR_RX` | 155.350 / 96.650 | 0.3 / 0.4 mm | 0402 pad |
| R80.1 | `/LEDs/LED6_DOUT` | 154.450 / 90.700 | 0.3 / 0.4 mm | 0402 pad |

For J6.11 there is no legal place for the via next to the pad. Measured over the whole pad axis
in 0.05 mm steps, the free positions are 158.825 ... 159.125 mm - all of them inside the pad.

Pin 6 (`UART_TX`) **is** routed and its via sits **outside** the pad, at 158.350 / 93.900, 0.375 mm
clear of the pad edge. That only became possible with the narrower `UART-eng` class: at 0.15 mm every
free position along the pad axis was inside the pad, at 0.10 mm a raster search over 2.5 mm found
seven usable places outside it. The via was then placed by forbidding vias over the pad
(x 158.398 ... 159.852 / y 93.554 ... 94.508) and letting the router pick its own spot; the detour
costs 1.50 mm of track (90.78 -> 92.28 mm).

**J4.3 was fixed** (this pad is J4.8 since the pin-out correction K2). `/XMOS/TDI` first reached the XU316 through a via at 135.850 / 57.600, inside
the 0.80 x 0.80 mm SMD pad J4.3. The net was re-routed with vias forbidden over the whole J4 pad
field plus clearance (x 133.803 … 140.337 / y 55.368 … 58.092): the via now sits at
**136.100 / 58.100**, 0.335 mm clear of the pad edge, and the track grew from 4.02 mm to 4.23 mm
(5 segments instead of 3, F.Cu 0.874 / B.Cu 3.354 mm). DRC, ERC and the netlist are unchanged.

* **The ordered via covering takes care of all three.** All vias are ordered resin-filled and
  copper-capped (see README, "Vias in pads"), so the holes are closed and plated over before
  assembly. Without that option the solder would be drawn into the hole, which a 0.30 mm FPC pad
  tolerates badly.
* Measured again after the audit fixes (script `via_in_pad.py`, which also counts a drilled hole that
  only partly reaches into a mask opening): board vias reach into **23 pads (58 vias, all same-net)**, measured after the pin-audit fixes of 2 Oct 2026: the via centre lies inside 14 pads — J6.11, R79.2, R80.1 (signal), C99.1, U11.6, L1.2 (supply), C47.2 and C60.2 (GND, above the U10 paddle; two of the vias added in H6), and the large pads of U8 (9 vias) and U10 (32 vias) — and in 9 more pads part of the drilled hole reaches into the solder-mask opening (C65.1, J1.B6, J4.4, J4.6, LED2.1, TP21, TP22, U19.2, U19.4). L2.2 is no longer on the list: its
  via was moved out of the larger pad of the new L2 footprint (audit fix M18).

**Kept as it is; no measurable effect shown.** Every via in a pad is same-net and covered by the
ordered filled and capped vias; widening the south-east channel was not pursued.
