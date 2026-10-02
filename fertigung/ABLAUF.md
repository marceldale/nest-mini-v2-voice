<!--
SPDX-FileCopyrightText: 2026 marceldale
SPDX-License-Identifier: MIT
-->

# How this directory is generated

Everything in `fertigung/` is produced from the KiCad files by the scripts in `werkzeug/`.
Nothing here is edited by hand. Run the steps in this order from the project root; each one
overwrites its own output.

## Step zero: check the BOM fields

```bash
python fertigung/werkzeug/bom_pruefen.py .
```

**Run this before generating anything.** It exists because E7 found ten wrong or missing part
numbers, and none of them was visible to ERC, DRC or the schematic-parity check — those look at
nets, not at order fields. The worst of them would have fitted R76 and R77, the divider that sets
Q4's gate voltage, with **0 Ω links**, which is a short from +14 V to ground.

It checks six things: schematic against layout for every component (number and value); no
populated part without a number; no number hiding in a field no exporter reads (`LCSC`,
`Supplier Part`); one number under two different values; resistor-style MPNs or descriptions on
parts that are not resistors; and, for the Uniroyal `0402WGFxxxxTCE` numbers, the encoded value
against the value field. It exits non-zero if it finds anything.

One known-good exception is built in: `C221897` sits on SW1 and SW2 with the value fields "Reset"
and "BOOT" — the same button, labelled differently.

What it cannot do is check a number against its LCSC product page. It prints the list of all 86
numbers with their URLs and the components that use each one; that part is handwork, and it is
where the J6, R80, R76 and R77 errors were actually settled.

```bash
python fertigung/werkzeug/fertigung.py nest-mini-v2-drop-in-pcb.kicad_pcb nest-mini-v2-drop-in-pcb.kicad_sch fertigung
```

That one command produces the JLCPCB set: Gerbers and Excellon drill files in `gerber/`
(plus drill maps as PDF), `gerber_jlcpcb.zip` for upload, `BOM_JLCPCB.csv` and
`CPL_JLCPCB.csv`. It prints three things worth reading:

* **BOM zusammengefasst** — part numbers used by more than one value or footprint. Two are
  expected: `C383201` (Q1, Q2, Q4 — the same MOSFET on two footprint variants of SC-70-3)
  and `C221897` (SW1, SW2 — the same button, labelled Reset and BOOT). Anything else means
  a part number is on the wrong component; that is how the J6 and R80 errors were found.
* **ohne LCSC** — components with no part number. Must be empty.
* **in CPL, nicht in BOM / in BOM, nicht in CPL** — both must be empty.

Then the PCBWay set:

```bash
python fertigung/werkzeug/bom_einzeln.py . fertigung/werkzeug/bom_einzeln.csv
python fertigung/werkzeug/pcbway_bom.py fertigung/werkzeug/bom_einzeln.csv
python fertigung/werkzeug/pcbway_xlsx.py fertigung/werkzeug/bom_einzeln.csv fertigung/pcbway/BOM_PCBWay.xlsx
```

`pcbway_bom.py` fetches manufacturer, MPN, description and package for every part number from
JLCPCB's parts API and **compares the MPN against the schematic**. Two differences are known
and deliberate — the part number names what is actually ordered, the MPN field what the design
asked for:

| Part | Schematic | Ordered |
|---|---|---|
| J5 | TE 1775443-2 | TE 5-1775443-2 (C5162845), variant of the same series |
| D6 | RB521S30T1G | RB521S30T1G-DW (C27636108), packaging variant |

Any **third** difference is a finding, not noise. (Until the audit fixes of 1 Oct 2026 there were
four: L8 and C100 now carry the ordered part in the schematic — L8 Sunlord WPN201610U1R0MTY01,
Isat 3.0 A min / 3.5 A typ per the Sunlord catalogue; C100 is FH 0402CG121J500NT, 120 pF.)

The assembly drawing needs KiCad's own Python, because it plots through pcbnew:

```bash
"C:/Program Files/KiCad/9.0/bin/python.exe" fertigung/werkzeug/bestueckzeichnung.py nest-mini-v2-drop-in-pcb.kicad_pcb <scratch>/kopie.kicad_pcb
"C:/Program Files/KiCad/9.0/bin/python.exe" fertigung/werkzeug/bestueck_plot.py <scratch>/kopie.kicad_pcb <scratch>/plot
```

The two resulting PDFs are joined into `pcbway/Bestueckungszeichnung.pdf`. `bestueckzeichnung.py`
works on a **copy** — it adds designators, outlines, pin-1 dots and polarity marks to the Fab
layers and removes DNP parts, and none of that belongs in the design file.

Finally `pcbway/Positionen_KiCad.csv`, the plain KiCad position export, as a cross-check against
the CPL:

```bash
kicad-cli pcb export pos --format csv --units mm --side both --exclude-dnp --output fertigung/pcbway/Positionen_KiCad.csv nest-mini-v2-drop-in-pcb.kicad_pcb
```

## Before handing anything to a fabricator

1. **Run `bom_pruefen.py` and read the LCSC pages it lists.** See above.
2. Zones must be filled and the fill must be current. Check it, do not assume it: fill the zones
   in a copy and compare the filled areas against the saved ones. For the set generated after the
   production state (2 Oct 2026, after C110): 11 582.99 mm² over 15 zone layers (13 zones), saved and
   refilled equal. One zone (`+14V` on In2) can alternate between 147.28 and 147.32 mm² from one fill
   to the next — a fill-iteration effect, 0.03 %; the Gerbers are made from the saved state. (After the audit
   fixes of 1 Oct 2026 the total was 11 596.11 mm²; H6 perforates the `VDD` area on In2 under U10.)
3. Read `DREHLAGEN_PRUEFEN.md` and work through the **15 + 9 + 6 new rows** (rev A1 → A2, the
   audit fixes and the pin-audit fixes; Y1 is the one where rotation matters) in JLCPCB's
   assembly preview. Those are the parts whose LCSC number changed since rev A1, or that are new;
   for them the fabricator's own footprint decides the zero orientation, and nobody has compared
   it yet.
4. Read "Before you order" in `CHANGES.md` — six points, including the 0.10 mm track width and
   the `IQ` variant of U11.
5. **Select the two order options.** They are not in the Gerbers; nobody will ask twice:
   * **Via covering: *Epoxy Filled & Capped*.** The board vias reach into **23 pads (58 vias, all same-net)**, measured after the pin-audit fixes of 2 Oct 2026: the via centre lies inside 14 pads — J6.11, R79.2, R80.1 (signal), C99.1, U11.6, L1.2 (supply), C47.2 and C60.2 (GND, on F.Cu above the U10 paddle, two of the H6 vias), and the large pads of U8 (9 vias) and U10 (32 vias: 16 GND, 4 in each VDD paddle) — and in 9 more pads part of the drilled hole reaches into the solder-mask opening (C65.1, J1.B6, J4.4, J4.6, LED2.1, TP21, TP22, U19.2, U19.4).
     An open via wicks solder out of the joint in reflow. Our vias are 0.30 mm drill / 0.40 mm
     diameter, inside the 0.15–0.55 mm range the process supports.
   * **Surface finish: ENIG.** 0.40 mm pitch on four parts, QFN thermal pads, the J4 spring-contact
     pad field, eighteen test points and three touch electrodes all want a planar finish. Exposed
     copper is 7.64 % of the board area, well under the 30 % above which JLCPCB adds an area
     surcharge.
