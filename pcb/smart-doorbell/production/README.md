# PCB release artifacts

The routed KiCad project is the electrical and population source of truth.
`Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv` remains the protected,
authoritative per-reference LCSC assignment record for all references inherited
by Rev C.

The external `bom-JLCPCB Assembly Order.xls` describes the obsolete
MCP73831/AP2112 topology. It is not an upload BOM and may only be consulted for
substitution candidates after every current requirement has been reverified.

## Ordered Rev B release

The exact PCB/PCBA package submitted on 2026-07-15 remains at:

- `../jlcpcb/production_files/GERBER-smart-doorbell.zip`
- `../jlcpcb/production_files/BOM-smart-doorbell.csv`
- `../jlcpcb/production_files/CPL-smart-doorbell.csv`

Its immutable hashes, order identifiers, and provenance are recorded in
`../jlcpcb/ORDERED_RELEASE.md`. Run `sha256sum -c ORDERED_RELEASE.sha256` from
`../jlcpcb/` before using Rev B as comparison evidence.

Rev B contains the known physical J3 camera-pin reversal. Never reorder it and
do not insert the selected lens-up/contact-down camera directly into a Rev B
board. Rev B populated R36, left U9/R37 DNP, and used populated fixed R26 in
place of real cell-temperature sensing; these facts describe only the ordered
historical batch.

## Ordered Rev C release

Rev C has a separate ordered-release directory:

- `../jlcpcb/rev-c/production_files/GERBER-smart-doorbell.zip`
- `../jlcpcb/rev-c/production_files/BOM-smart-doorbell.csv`
- `../jlcpcb/rev-c/production_files/CPL-smart-doorbell.csv`

Exact hashes, generation provenance, local verification results, and the
signed-off JLCPCB BOM/preview evidence are recorded in
`../jlcpcb/rev-c/ORDERED_RELEASE.md`. Run
`sha256sum -c ORDERED_RELEASE.sha256` from that directory whenever verifying
the submitted bytes.

Rev C contains 85 populated references in both BOM and CPL. Existing protected
Rev B references retain their exact value/LCSC assignment. The intentional
population delta is:

- add/populate C35 (`1u`, C15849);
- add/populate R37 (`1k`, C21190);
- add/populate U9 (`TPS22919DCKR`, C2149796);
- remove/DNP R26 and R36.

U9 is the only populated path from `+3V3` to `CAM_3V3`. R38 and the TPS22919
internal pulldown keep it default-off; GPIO42 reaches `CAM_PWR_EN` through
populated R37. R31 must remain DNP so GPIO42 is not also connected to the PIR
fallback. QOD is intentionally open.

Rev C intentionally excludes these references from both BOM and CPL:

- U1 (`ESP32-S3-WROOM-1-N16R8`) and MK1 (`ICS-43434`), hand solder;
- R11 and R21, DNP camera contingency links;
- R26, DNP fixed THERM resistor;
- R31, DNP GPIO42 PIR fallback;
- R36, DNP camera-power bypass;
- J5 and J9, unpopulated hand-solder/expansion connections;
- TP1 through TP10, bare test points;
- H1 through H4, mechanical mounting holes.

J5 is only a connection for an external thermistor. No NTC is present in the
Rev C JLCPCB BOM. Do not charge a battery until a suitable
10 kOhm-at-25-degrees-C NTC is wired to J5 and thermally attached to the cell.

The BOM explicitly targets timer-disabled `MCP73871T-2AAI/ML`; do not substitute
the six-hour-timer `MCP73871T-2CCI/ML` at the present approximately 213 mA
charge current.

## Legacy comparison evidence

`Smart_Doorbell_Project_B.zip` and its adjacent Fabrication Toolkit exports are
retained only as previously reviewed comparison evidence. They were not the
files submitted for the 2026-07-15 order.

Neither a local CPL inspection nor this manifest replaces JLCPCB's rendered
assembly preview. The Rev C order passed that external gate before submission:
all 85 top-side placements, every DNP, package, side, and rotation were checked,
with special attention to U9 pin 1 at the corrected 180-degree CPL rotation.
The retained evidence and sign-off are part of the ordered Rev C release.
