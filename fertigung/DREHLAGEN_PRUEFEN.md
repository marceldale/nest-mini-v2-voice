# Drehlagen und Positionen fuer die JLCPCB-Vorschau — Soll-Zustand

Stand: **02.10.2026**, nach der JLCPCB-Vorschau berichtigt (U18, U16, U5, Q4; Y1-Bemerkung; R35).
Zuvor: 01.10.2026, erzeugt aus `CPL_JLCPCB.csv` und der Platine (rev A2 Endstand).
Die Fassung vom 21.09.2026 ist ersetzt; sie nannte U1, U2, C1, C2, R37, R38, R51, R64
und FB3, die es nicht mehr gibt, und kannte die in E2.3, E4, E5c und F7.3 neu
eingefuegten Bauteile nicht.

**Grundlage:** `ft_cpl.py` (Logik des Fabrication Toolkit) plus
`werkzeug/jlc_korrektur.csv` (19 Zeilen). Die Korrekturen stammen aus einem Abgleich mit
dem EasyEDA-Footprint der jeweiligen LCSC-Nummer; dieser Footprint bestimmt JLCPCBs
Nullage und Mittelpunkt.

* **Konvention**, an den laut Vorschau richtigen Teilen bestaetigt:
  * oben: CPL-Drehung = Drehung des EasyEDA-Footprints
  * unten: CPL-Drehung = 360° − Drehung des gespiegelten EasyEDA-Footprints
  * Bestaetigt oben an U8 und U9, unten an U10, U11, Q1, Q3 und X1; am 02.10.2026 an allen
    42 gepolten und mehrpoligen Teilen nachgerechnet (Abschnitt „Berichtigung").
* **Pin 1** ist die Lage von Pad 1 relativ zur Padmitte, in der Ansicht der Vorschau:
  oben in Draufsicht, unten in Unteransicht — also seitenverkehrt zur Draufsicht.
  Die Werte unten sind aus unserem Footprint gerechnet.
* In der Vorschau pruefen: rosa Punkt an der angegebenen Ecke; Bestueckungsdruck-Marke
  an derselben Ecke; das Gehaeuse deckt alle Pads.

## Berichtigung nach der JLCPCB-Vorschau vom 02.10.2026

Die Bestellung wurde angehalten: die Vorschau zeigte U18 mit Pin 1 oben links statt unten links
(VIN/VOUT vertauscht), U16 mit Pin 1 unten rechts statt unten links und Y1 nur als Platzhalter.
Daraufhin wurde **jedes gepolte oder mehrpolige Bauteil** (42 Zeilen, Tabelle unten) gegen den
EasyEDA-Footprint seiner LCSC-Nummer nachgerechnet, nicht nur die drei gemeldeten: Footprint laden,
an die CPL-Mitte legen (unten in x gespiegelt), in 45°-Schritten drehen und gleichnamige Pads
zaehlen (`werkzeug/easyeda_abgleich.py`, Daten `pruefung/easyeda_pads_2026-10-02.json`).

**Vier Drehungen waren falsch**, alle vier jetzt in `werkzeug/jlc_korrektur.csv` (19 Zeilen):

| Bauteil | LCSC | Seite | CPL bisher | CPL jetzt | Abgleich mit dem EasyEDA-Footprint | Folge der alten Drehung |
|---|---|---|---|---|---|---|
| **U18** | C2869734 | oben | 270° | **0°** | 6/6 Pads in gleicher Reihenfolge; Pin 1 unten links (`/Power/+5V_SW`). EasyEDA setzt die Padreihen bei ±1,10 mm, unser Footprint bei ±0,89 mm — 0,21 mm je Reihe, symmetrisch nach aussen, die Anschluesse liegen auf den Pads | Gehaeuse quer, VIN/VOUT vertauscht (Meldung aus der Vorschau) |
| **U16** | C129313 | unten | 90° | **0°** | 10/10 Pads, max. 0,04 mm; Pin 1 in der Vorschau (Unteransicht) unten links | Pin 1 unten rechts (Meldung aus der Vorschau) |
| **U5** | C133796 | oben | 0° | **270°** | 5/5 Pads, max. 0,13 mm; EasyEDA hat Pin 1 unten rechts (Endung `-BR`), unser Footprint unten links | Eingangs- und Ausgangsseite quer; die alte Korrekturzeile „unverdreht deckungsgleich" war falsch |
| **Q4** | C383201 | oben | 0° | **180°** | 3/3 Pads, max. 0,03 mm; Pin 1 (`USB_EN_GATE`) oben links | Gate/Source auf der Drain-Seite; Q1 und Q2 mit derselben Nummer waren richtig, weil fuer sie eine andere Regel greift |

Alle uebrigen 38 Zeilen stimmen mit dem EasyEDA-Footprint. Nicht als Zaehlung, sondern im
Einzelnen belegt sind:

* **J5:** EasyEDA zerlegt jeden der beiden 7-mm-Kontaktstreifen in zwei Pads (1+4, 2+3); bei 225°
  liegen 1 und 4 auf unserem Streifen 1, 2 und 3 auf Streifen 2 — richtig.
* **D6:** 1→1 und 2→2 bei 270°, je 0,32 mm Versatz (anderer Padabstand), richtig.
* **U3/U7:** 16/17 — das Waermepad ist bei uns in Teilflaechen zerlegt; alle Anschlusspads 0,04 mm.
* **MK1/MK2:** 4/8 — Massering wie unter „Einzelhinweise".
* **SW1/SW2:** 2/4 — Taster, 180°-symmetrisch, Pads 1/2 und 3/4 paarweise verbunden.

**Y1 (C49158179):** EasyEDA liefert fuer die Nummer kein Bauteil („Component not found",
abgefragt 02.10.2026); die Vorschau zeigt nur einen Platzhalter, und der Bestuecker legt das
Teil von Hand auf. Die Drehung in der CPL (270°) bleibt; massgeblich ist die Bestueckungsdruck-Marke:
der Winkel im Bestueckungsdruck sitzt an der Ecke **oben links**, und dort liegt Pad 1
(`XTAL_N`) — aus der Platine nachgemessen (Winkel 117,03…119,57 / 67,83…70,77 mm, Pad 1 bei
117,75 / 68,60). **Bemerkung fuer JLCPCB im Bestellformular:**

> Y1 orientation per silkscreen pin-1 corner mark (top-left).

**R35:** neue Nummer **C100510** (LIZ Elec CR0402FF6800G, 680 Ω 1 % 0402) statt C25130, Ersatz auf
Angabe des Betreibers. Ungepolt; Zeile in der Tabelle des Pin-Audits.

Quelle der Footprint-Daten: JLCEDA/EasyEDA Official Library (https://lceda.cn/,
https://easyeda.com), abgerufen am 02.10.2026 ueber
`easyeda.com/api/products/<LCSC>/components`.

## JLCPCB-DFM-Rückfrage LED-Polarität (05.10.2026) — Ansichtskonvention und Sollage

**Ansicht in JLCPCBs DFM-Bildern:** Die Unterseite wird **links-rechts gespiegelt** gezeigt, so dass
der Bestückungsdruck lesbar ist: **J1 oben links**, auf der LED-Seite liegt **LED5 links, LED2 rechts**.
Das ist dieselbe Ansicht wie KiCads „Flip board view“ (Spiegelung um die senkrechte Achse) und wie die
Spalte „Pin 1 in der Vorschau“ dieser Datei für Bauteile auf der Unterseite. Die Oberseite wird
ungespiegelt gezeigt (J1 oben rechts).

**Befund (Betreiber, DFM-Bild zu SMT026100262223):** Das JLCPCB-Modell zeigte bei LED2–LED5 „+“ und den
Pin-1-Punkt **unten links**, „−“ oben rechts, DI oben links, DO unten rechts. In dieser Ansicht liegt
unser Pad 1 (+5V) aber **oben rechts** → Modell **um 180° falsch**. LED1 war richtig. Der Betreiber hat
JLCPCB um eine 180°-Drehung von LED2–LED5 und ein neues Bild gebeten (05.10.2026). **Ergebnis:** JLCPCB hat
LED2–LED5 um 180° korrigiert; neues Bild vom 05.10.2026 zeigt +5V oben rechts in der gespiegelten
Unteransicht — vom Betreiber freigegeben. LED1 bestätigt, **LED6 nicht ausdrücklich bestätigt** (Sichtprüfung
bei der Inbetriebnahme, Arbeitsnotizen, nicht veröffentlicht: HANDGRIFFE Schritt 0.7). Fertigung freigegeben, Fertigstellung ca. 14.10.2026.

**Merkregel für künftige Vorschauen bei JLCPCB:** Oberseite ungespiegelt (J1 oben rechts); Unterseite
links-rechts gespiegelt (J1 oben links). Die Spalte „Pin 1 in der Vorschau“ dieser Datei gilt genau in dieser
Ansicht. Ein Modell, dessen Pin-1-Punkt an einer anderen Ecke liegt, ist zu beanstanden — auch wenn der
EasyEDA-Abgleich nummerngleich passt (LED2–LED5, 05.10.2026).
Widerspruch zum EasyEDA-Abgleich vom 02.10.2026: dort passen bei CPL 90° alle 4 Pads des EasyEDA-Footprints
von C2909058 nummerngleich auf unsere Pads. JLCPCBs Bestückmodell weicht also vom EasyEDA-Footprint ab
(Pin-1-Lage), oder die Nummerierung im EasyEDA-Footprint passt nicht zum Gehäuse — ungeklärt. Für eine
Folgefertigung: Arbeitsnotizen (nicht veröffentlicht), HANDGRIFFE Abschnitt 6, F4.

**Sollage je LED in JLCPCBs Ansicht**

| Bauteil | Seite / Ansicht | +5V | GND | DIN | DOUT | Gehäusemerkmal |
|---|---|---|---|---|---|---|
| LED2, LED3, LED4, LED5 (SK6812-EC20, Pin 1 = VDD) | unten, gespiegelt (J1 oben links) | **oben rechts** (Pad 1) | unten links (Pad 3) | unten rechts (Pad 4) | oben links (Pad 2) | Kerbe auf der Gehäuseunterseite an der Kante +5V/DOUT, also **oben**; IC-Chip an der Kante GND/DIN (unten) |
| LED1 (SK6812D-EC3210R, westliche Lasche) | oben, ungespiegelt (J1 oben rechts) | 2. Pad von Süden (Pad 4) | 2. Pad von Norden (Pad 1) | Südende (Pad 3) | Nordende (Pad 2) | Linse nach Westen (Platinenkante); kein Punkt im Datenblatt |
| LED6 (SK6812D-EC3210R, östliche Lasche) | oben, ungespiegelt | 2. Pad von Norden (Pad 4) | 2. Pad von Süden (Pad 1) | Nordende (Pad 3) | Südende (Pad 2) | Linse nach Osten (Platinenkante) |

Reihenfolge auf der LED-Seite in dieser Ansicht von links nach rechts: LED5, LED4, LED3, LED2; der DO jeder
LED zeigt zum DI der links benachbarten. Sollbild: `pruefung/jlcpcb_led_sollage_2026-10-05.png` (dort in der
anderen Spiegelung, oben/unten — Pad-Netze gleich).

## Was nach rev A1 neu zu pruefen ist

Fuer diese **15** Bauteile hat sich die LCSC-Nummer geaendert oder das Bauteil ist neu.
JLCPCB hinterlegt dann einen anderen EasyEDA-Footprint, und der bestimmt die Nullage.
**Nur diese Zeilen muss der Betreiber in der Vorschau durchsehen** — alles andere war
in rev A1 abgeglichen und ist seither unveraendert.

| Bauteil | Seite | Drehung | Mitte X / Y | Pin 1 (Netz) | warum neu |
|---|---|---|---|---|---|
| **J1** | oben | 135° | 152.9000 / -65.8000 | unten (GND) | in E5.3 als FP_SMD gekennzeichnet, Position daher aus dem Ursprung |
| **J6** | oben | 270° | 157.9750 / -93.7810 | unten rechts (GND) | neu in E5c; Nummer in E7 von C25744/C324723 auf C424659 berichtigt |
| **Q4** | oben | 180° | 157.0000 / -83.9000 | oben links (/Power/USB_EN_GATE) | neu in E4; Nummer C383201 erst in E7 eingetragen; Drehung am 02.10.2026 von 0° berichtigt |
| **R76** | oben | 0° | 156.4000 / -81.8000 | links (+14V) | Nummer in E7 von C17168 (0 Ohm!) auf C26083 (1 M) berichtigt |
| **R77** | oben | 0° | 157.1000 / -80.8000 | links (/Power/USB_EN_GATE) | Nummer in E7 von C17168 (0 Ohm!) auf C25778 (330 k) berichtigt |
| **R80** | oben | 0° | 154.7000 / -90.6900 | links (/LEDs/LED6_DOUT) | Nummer in E7 von C25744 (10 k!) auf C25076 (100 R) berichtigt |
| **U5** | oben | 270° | 141.0000 / -75.9650 | unten links (+3V3) | Typ MIC5365-1.8 -> TLV70018DCKR (C626087 -> C133796), E5.2; Drehung am 02.10.2026 von 0° berichtigt |
| **U18** | oben | 0° | 124.8000 / -106.7000 | unten links (/Power/+5V_SW) | neu in E4; Nummer C2869734 erst in E7 eingetragen; Drehung am 02.10.2026 von 270° berichtigt |
| **LED2** | unten | 90° | 125.2624 / -89.1135 | oben rechts (+5V) | Typ SK6805-EC20 -> SK6812-EC20 (C2890036 -> C2909058), E6 |
| **LED3** | unten | 90° | 132.2500 / -89.1000 | oben rechts (+5V) | Typ SK6805-EC20 -> SK6812-EC20 (C2890036 -> C2909058), E6 |
| **LED4** | unten | 90° | 139.2000 / -89.1000 | oben rechts (+5V) | Typ SK6805-EC20 -> SK6812-EC20 (C2890036 -> C2909058), E6 |
| **LED5** | unten | 90° | 146.1000 / -89.1000 | oben rechts (+5V) | Typ SK6805-EC20 -> SK6812-EC20 (C2890036 -> C2909058), E6 |
| **U16** | unten | 0° | 121.5000 / -70.2500 | unten links (unconnected-(U16-D1+-Pad1)) | neu in E2.3; Drehung am 02.10.2026 von 90° berichtigt |
| **U19** | unten | 0° | 118.4500 / -84.4500 | unten rechts (GND) | neu in F7.3; Nummer C12495 erst in E7 eingetragen |
| **X1** | unten | 270° | 133.0000 / -59.5000 | oben links (Net-(U10-XIN)) | Ursprung in E5.3 auf das Bezugszentrum gesetzt |

Bei den vier Ring-LEDs ist die Erwartung, dass sich **nichts** aendert: beide LED-Typen
haben die Anschluesse an denselben Stellen (Datenblaetter SK6805-EC20-001 Rev. A1 und
SK6812-EC20-001 Rev. A/2, je Seite 4), und die CPL-Werte sind vor und nach dem Wechsel
gleich. Zu pruefen ist allein, ob JLCPCBs Footprint fuer C2909058 dieselbe Nullage hat
wie der fuer C2890036. **Ergebnis 05.10.2026: nein** — JLCPCBs Modell lag um 180° falsch (Abschnitt
„JLCPCB-DFM-Rückfrage LED-Polarität“).

## Nach den Audit-Fixes vom 01.10.2026 zusaetzlich zu pruefen

Die CPL ist aus dem Stand nach den Audit-Fixes neu erzeugt. **Neun** Zeilen kommen hinzu. Alle sind
ungepolt (Kondensatoren, Spulen); in der Vorschau zu pruefen ist nur, dass das Gehaeuse mittig auf
beiden Pads sitzt und nichts versetzt ist.

| Bauteil | Seite | Drehung | Mitte X / Y | LCSC | warum neu |
|---|---|---|---|---|---|
| **L1** | oben | 90° | 121.0125 / -109.5615 | C719179 | neuer Footprint `L_Bourns_SRP4020TA` (Audit M18): Pads 1,5 x 2,4 mm bei ±1,85 mm statt Cenker-Landmuster; LCSC-Nummer unveraendert |
| **L2** | oben | 90° | 112.0000 / -98.0500 | C719179 | wie L1 |
| **C100** | oben | 0° | 133.6000 / -82.3000 | C40059 | neue Nummer (120 pF statt 22 pF, Audit M2) |
| **C20** | oben | 0° | 137.5896 / -75.1115 | C1779 | neue Nummer (4,7 µF statt 10 µF, Audit M2) |
| **C105** | unten | 270° | 124.3000 / -115.9800 | C307331 | neu (Audit M1), 0402 auf B.Cu unter U3 |
| **C106** | unten | 270° | 115.7500 / -106.4300 | C1525 | neu (Audit M1), 0402 auf B.Cu unter U7 |
| **C107** | oben | 0° | 143.0000 / -65.9400 | C19702 | neu (Audit M3), 0603 |
| **C108** | oben | 315° | 105.6000 / -85.1000 | C1525 | neu (Audit M11), 0402 schraeg neben MK1 |
| **C109** | oben | 0° | 168.6000 / -92.2000 | C1525 | neu (Audit M11), 0402 neben MK2 |

J4 (xSYS2-Padfeld, Audit K2) ist DNP und steht nicht in der CPL.

## Nach dem Pin-Audit vom 02.10.2026 zusaetzlich zu pruefen

Die CPL ist aus dem Stand nach H1–H6 und den Entscheidungen vom 02.10.2026 neu erzeugt. **Sieben** Zeilen kommen hinzu (C110 ist DNP und steht nicht in der CPL). Y1 ist das einzige
Teil, bei dem die Drehung zaehlt: um 90° verdreht laegen die Quarzanschluesse (Pins 1/3) auf den
GND-Pads (2/4). Um 180° verdreht ist gleichwertig (der Quarz ist symmetrisch).

| Bauteil | Seite | Drehung | Mitte X / Y | LCSC | warum neu |
|---|---|---|---|---|---|
| **Y1** | oben | 270° | 118.3000 / -69.3000 | C49158179 | neue Nummer (JLYE Y201640MDBCX, ±10 ppm, Pin-Audit H1); Pin 1 oben links (Net-(U8-XTAL_N)), Pin 3 zu XTAL_P — JLYE-Datenblatt „Connection“: 1/3 Quarz, 2/4 GND. **Kein EasyEDA-Modell**, Bemerkung an JLCPCB: „Y1 orientation per silkscreen pin-1 corner mark (top-left)“ |
| **R72** | unten | 180° | 139.8100 / -82.1000 | C25798 | neue Nummer (75 kΩ statt 200 kΩ, H2) |
| **C17** | oben | 180° | 138.9700 / -73.1150 | C19702 | neue Nummer (10 µF statt 4,7 µF, H4); dieselbe Nummer wie C13/C18/C107 |
| **R81** | unten | 270° | 116.1000 / -84.9000 | C25744 | neu (H3), 0402 auf B.Cu neben U19 |
| **C63** | unten | 0° | 122.7550 / -60.7500 | C52923 | neue Nummer (1 µF statt 100 nF, U10-B1) |
| **C98** | unten | 180° | 139.8000 / -83.1000 | C1525 | neue Nummer (100 nF statt 1 µF, MPR121-Datenblatt) |
| **R35** | unten | 90° | 131.6000 / -60.6000 | C100510 | neue Nummer (LIZ Elec CR0402FF6800G, 680 Ω 1 %, statt C25130), Ersatz bei der Bestellung 02.10.2026 |

## Alle gepolten und mehrpoligen Bauteile

Spalte „Abgleich": Herkunft der Drehung. Zusaetzlich sind **alle** Zeilen am 02.10.2026 gegen
den EasyEDA-Footprint nachgerechnet (Abschnitt „Berichtigung").

| Bauteil | Seite | Drehung | Mitte X / Y (mm) | Pin 1 in der Vorschau (Netz) | Abgleich |
|---|---|---|---|---|---|
| AE1 | oben | 45° | 113.7747 / -66.9253 | unten links (Net-(AE1-Pad1)) | rev A1 |
| C89 | oben | 315° | 155.5934 / -110.5934 | oben links (PVDD) | rev A1, korrigiert |
| D3 | oben | 45° | 149.9606 / -69.5323 | unten links (GND) | rev A1 |
| D4 | oben | 45° | 149.2606 / -68.8323 | unten links (GND) | rev A1 |
| D5 | oben | 0° | 114.4502 / -81.4710 | links (ESP_3V3) | rev A1 |
| J1 | oben | 135° | 152.9000 / -65.8000 | unten (GND) | **NEU PRUEFEN** |
| J5 | oben | 225° | 160.8106 / -105.6288 | unten rechts (Net-(C86-Pad1)) | rev A1, korrigiert |
| J6 | oben | 270° | 157.9750 / -93.7810 | unten rechts (GND) | **NEU PRUEFEN** |
| LED1 | oben | 270° | 100.5202 / -89.0869 | oben rechts (GND) | rev A1, korrigiert |
| LED6 | oben | 90° | 171.1100 / -89.0668 | unten links (GND) | rev A1, korrigiert |
| MK1 | oben | 225° | 104.1227 / -82.3469 | rechts (Net-(MK1-DATA)) | rev A1 |
| MK2 | oben | 45° | 167.4438 / -96.0810 | links (Net-(MK2-DATA)) | rev A1 |
| POWER_IN1 | oben | 0° | 141.3104 / -120.2010 | oben rechts (GND) | rev A1, korrigiert |
| Q4 | oben | 180° | 157.0000 / -83.9000 | oben links (/Power/USB_EN_GATE) | **NEU PRUEFEN**, EasyEDA 02.10., korrigiert |
| R76 | oben | 0° | 156.4000 / -81.8000 | links (+14V) | **NEU PRUEFEN** |
| R77 | oben | 0° | 157.1000 / -80.8000 | links (/Power/USB_EN_GATE) | **NEU PRUEFEN** |
| R80 | oben | 0° | 154.7000 / -90.6900 | links (/LEDs/LED6_DOUT) | **NEU PRUEFEN** |
| SW1 | oben | 270° | 115.3000 / -85.0000 | oben rechts (ESP_RST) | rev A1 |
| SW2 | oben | 270° | 119.0200 / -85.0000 | oben rechts (ESP_BOOT) | rev A1 |
| U3 | oben | 270° | 122.0500 / -114.5625 | oben links (Net-(L1-Pad1)) | rev A1, korrigiert |
| U4 | oben | 0° | 153.4000 / -84.7000 | unten links (+5V) | rev A1, korrigiert |
| U5 | oben | 270° | 141.0000 / -75.9650 | unten links (+3V3) | **NEU PRUEFEN**, EasyEDA 02.10., korrigiert |
| U6 | oben | 180° | 135.7000 / -78.8600 | oben rechts (GND) | rev A1, korrigiert |
| U7 | oben | 270° | 112.9800 / -103.2300 | oben links (Net-(L2-Pad1)) | rev A1, korrigiert |
| U8 | oben | 90° | 124.4000 / -70.0000 | unten links (Net-(U8-LNA_IN)) | rev A1 |
| U9 | oben | 90° | 127.2000 / -60.3000 | unten rechts (/ESP32-S3R8/SPI_CS0) | rev A1 |
| U14 | oben | 270° | 136.2000 / -103.3000 | oben links (GNDA) | rev A1, korrigiert |
| U18 | oben | 0° | 124.8000 / -106.7000 | unten links (/Power/+5V_SW) | **NEU PRUEFEN**, EasyEDA 02.10., korrigiert |
| Y1 | oben | 270° | 118.3000 / -69.3000 | oben links (Net-(U8-XTAL_N)) | **NEU PRUEFEN** (neue Nummer C49158179, H1; kein EasyEDA-Modell, nach Bestueckungsdruck) |
| D1 | unten | 225° | 152.2606 / -64.3323 | oben rechts (GND) | rev A1 |
| D6 | unten | 270° | 138.4000 / -76.2000 | oben (Net-(U12-2Y)) | rev A1 |
| LED2 | unten | 90° | 125.2624 / -89.1135 | oben rechts (+5V) | **NEU PRUEFEN** |
| LED3 | unten | 90° | 132.2500 / -89.1000 | oben rechts (+5V) | **NEU PRUEFEN** |
| LED4 | unten | 90° | 139.2000 / -89.1000 | oben rechts (+5V) | **NEU PRUEFEN** |
| LED5 | unten | 90° | 146.1000 / -89.1000 | oben rechts (+5V) | **NEU PRUEFEN** |
| Q1 | unten | 180° | 137.3000 / -56.6000 | oben links (VBUS) | rev A1 |
| Q2 | unten | 270° | 123.0000 / -63.0000 | unten links (Net-(Q2-G)) | rev A1, korrigiert |
| Q3 | unten | 180° | 138.2800 / -94.5000 | oben links (Net-(Q3-B)) | rev A1 |
| U10 | unten | 180° | 137.8749 / -64.6003 | unten rechts (/XMOS/QSPI_D1) | rev A1 |
| U11 | unten | 90° | 127.2000 / -76.2000 | unten rechts (/XMOS/QSPI_CS_N) | rev A1 |
| U12 | unten | 180° | 135.0000 / -76.5000 | oben rechts (GND) | rev A1, korrigiert |
| U13 | unten | 180° | 132.9625 / -94.5000 | oben links (+5V) | rev A1, korrigiert |
| U15 | unten | 90° | 142.9000 / -83.6000 | unten rechts (/ESP32-S3R8/TOUCH_IRQ) | rev A1, korrigiert |
| U16 | unten | 0° | 121.5000 / -70.2500 | unten links (unconnected-(U16-D1+-Pad1)) | **NEU PRUEFEN**, EasyEDA 02.10., korrigiert |
| U19 | unten | 0° | 118.4500 / -84.4500 | unten rechts (GND) | **NEU PRUEFEN** |
| X1 | unten | 270° | 133.0000 / -59.5000 | oben links (Net-(U10-XIN)) | **NEU PRUEFEN** |

## Ungepolt oder 180°-symmetrisch

Widerstaende, Kondensatoren, Spulen und die beiden Taster. Hier ist die Drehung ohne
Bedeutung; zweipolige ungepolte Teile und SW1/SW2 zeigen gegenueber dem
EasyEDA-Footprint teils 180° Unterschied, was nicht geaendert wurde.

| Footprint | Bauteile |
|---|---|
| `BLM18PG121SN1D` | FB2 90°, FB4 90°, FB5 0° |
| `C_0402_1005Metric` | C5 180°, C16 180°, C24 90°, C26 0°, C27 180°, C28 270°, C29 180°, C30 90° (unten), C31 0°, C32 0°, C33 90° (unten), C34 90° (unten), C35 90° (unten), C36 0°, C37 180°, C38 90°, C39 90°, C40 90°, C41 0°, C42 90°, C43 90°, C44 270°, C45 0°, C46 0°, C47 180°, C48 90° (unten), C49 180°, C50 0°, C51 0°, C52 180°, C53 180°, C54 0° (unten), C55 0°, C56 0°, C57 270°, C58 180° (unten), C59 180°, C60 0°, C61 180° (unten), C62 180° (unten), C63 0° (unten), C65 180° (unten), C66 0° (unten), C67 0°, C68 270° (unten), C69 270° (unten), C70 270° (unten), C71 225°, C72 45°, C78 180°, C83 90°, C88 0°, C92 90°, C93 180° (unten), C94 180° (unten), C95 180° (unten), C96 180° (unten), C97 270°, C98 180° (unten), C99 0° (unten), C100 0°, C101 180° (unten), C103 0°, C104 90° (unten) |
| `C_0603_1608Metric` | C6 90°, C7 90°, C8 270°, C9 270°, C12 270°, C13 270°, C17 180°, C18 180°, C21 270°, C25 270°, C64 180° (unten), C74 180°, C75 180°, C76 180°, C77 180°, C80 270°, C82 270°, C90 90°, C91 90° |
| `C_0805_2012Metric` | C3 270°, C4 270°, C10 180°, C11 270°, C14 270°, C15 270°, C19 0°, C20 0°, C22 270°, C23 270°, C79 90°, C81 90°, C84 270°, C85 270°, C86 90°, C87 270° |
| `L_0402_1005Metric` | L3 270°, L4 90°, L5 270° |
| `L_Cenker_CKCS4020` | L1 90°, L2 90° |
| `L_Murata_DFE201610P` | L8 270° |
| `MTQH404030S100MBT` | L6 180°, L7 180° |
| `R_0402_1005Metric` | R1 315°, R2 225° (unten), R3 225° (unten), R5 135°, R7 180°, R8 180°, R9 180°, R14 180°, R15 180°, R20 0°, R21 0°, R22 180°, R24 0°, R27 0°, R28 0°, R29 270°, R30 270°, R31 180°, R32 90°, R33 180°, R34 180° (unten), R35 90° (unten), R36 180° (unten), R39 180° (unten), R40 0° (unten), R41 180° (unten), R42 0° (unten), R43 180° (unten), R44 180° (unten), R45 270° (unten), R46 270° (unten), R47 0° (unten), R48 180° (unten), R49 0° (unten), R50 180° (unten), R52 90° (unten), R53 180° (unten), R54 90° (unten), R55 180°, R56 90° (unten), R57 0°, R58 270° (unten), R59 135°, R60 315°, R62 45°, R63 315°, R65 90° (unten), R70 0°, R71 90°, R72 180° (unten), R73 180° (unten), R74 90°, R75 0°, R79 0° |
| `R_0603_1608Metric` | FB1 225° (unten), R11 0°, R12 270°, R13 90°, R17 0°, R18 270°, R19 90°, R67 180° (unten) |
| `R_0805_2012Metric` | R68 270°, R69 90° |
| `R_1206_3216Metric` | R66 90° |

## Einzelhinweise

* **Y1:** Auch fuer die neue Nummer C49158179 liefert EasyEDA keinen Footprint ("Component not
  found", 02.10.2026; vorher ebenso fuer C424431); **nicht abgleichbar**. Bei einem 4-Pin-Quarz
  vertauscht eine 90°-Drehung Quarz- und Masseanschluesse. Pin 1 (XTAL_N) muss oben links liegen,
  an der Ecke mit dem Winkel im Bestueckungsdruck. Bemerkung an JLCPCB: „Y1 orientation per
  silkscreen pin-1 corner mark (top-left)".
* **MK1/MK2:** EasyEDA teilt den Massering in vier Flaechen, unser Footprint hat einen
  Ring mit zwei Pads. Die Signalpads und die Schallöffnung decken sich; in rev A1 war
  das in der Vorschau richtig.
* **C89** (Polymer-Elko): Pad 1 ist **PVDD**, also der Plus-Anschluss. Die
  180°-Korrektur der Fabrication-Toolkit-Regel `^C_Elec_` ist zurueckgenommen.
* **J1** (USB-C): die vier Schirmlaschen sitzen in durchkontaktierten Langloechern.
  Pruefen, dass das Gehaeuse auf allen 24 Signalpads und den vier Laschen liegt.

Erzeugt mit `scratchpad/e6/h1_drehlagen.py` aus CPL und Platine; Berichtigung vom 02.10.2026
von Hand nach `werkzeug/easyeda_abgleich.py`, jede geaenderte Zahl dort nachgerechnet.
