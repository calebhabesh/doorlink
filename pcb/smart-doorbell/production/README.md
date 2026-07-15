# PCB release artifacts

## Ordered Rev B release

The PCB/PCBA order submitted on 2026-07-15 used the kicad-jlcpcb-tools outputs:

- `../jlcpcb/production_files/GERBER-smart-doorbell.zip`
- `../jlcpcb/production_files/BOM-smart-doorbell.csv`
- `../jlcpcb/production_files/CPL-smart-doorbell.csv`

Their immutable ordered hashes and provenance are recorded in
`../jlcpcb/ORDERED_RELEASE.md`. The project-local part and placement corrections
are stored in `../jlcpcb/project.db`. The fabrication ZIP includes the native
F.Mask correction applied and verified by `../../../scripts/fix-jlcpcb-mask.sh`.

Do not regenerate or re-upload this package merely because KiCad is saved after
the order. Any future upload must be regenerated from the then-current sources
and pass a new release review.

## Legacy comparison evidence

`Smart_Doorbell_Project_B.zip` and its adjacent Fabrication Toolkit exports are
retained only as previously reviewed comparison evidence. They are not the files
used for the 2026-07-15 order.

`Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv` remains the protected,
authoritative per-reference LCSC assignment record. The ordered plugin BOM was
verified to match it exactly. The external `bom-JLCPCB Assembly Order.xls`
contains an earlier MCP73831/AP2112 topology and is not a valid upload BOM for
this revision.

## Assembly status

The assembly exports intentionally exclude these hand-solder/DNP parts:

- U1 (ESP32-S3-WROOM-1-N16R8), hand solder
- MK1 (ICS-43434), hand solder
- R11 and R21, DNP camera contingency links
- R31, DNP GPIO42 PIR fallback
- U9 and R37, DNP camera-power-switch contingency
- J5 and J9, unpopulated hand-solder/expansion connections
- TP1 through TP10, bare test points
- H1 through H4, mechanical mounting holes

## Camera-power contingency assembly rule

The default assembly keeps the existing always-powered camera-regulator input
path:

- Populate R36 (0 ohm bypass).
- Populate R38 (100 kohm `CAM_PWR_EN` pulldown).
- Do not populate U9 or R37.
- Do not populate R31; the PIR uses GPIO3 through R30.

To test GPIO42-controlled camera power gating in the future:

- Remove R36 before enabling the switched path.
- Populate U9 (TPS22919DCK) and R37 (1 kohm).
- Keep R31 DNP so GPIO42 is not connected to the PIR output.

R31 and R37 must never be populated simultaneously. R31 assigns GPIO42 to the
PIR fallback, while R37 assigns GPIO42 to `CAM_PWR_EN`. Populating both would
couple the PIR output to the camera-switch enable signal.

The BOM explicitly targets the timer-disabled `MCP73871T-2AAI/ML`. Do not
substitute the six-hour-timer `MCP73871T-2CCI/ML` at the present 213 mA charge
current.

This package does not replace the final JLC assembly preview or first-article
qualification. Before ordering, verify every resolved JLC/LCSC part, package,
quantity, side and rotation against the protected BOM and placement preview.

The TPS63802 input loop and local MCP73871 VBAT bypass are implemented layout
features, not pending relayout tasks. First-article testing must still validate
regulator startup/load steps, ripple and heating, charger operation, battery
voltage sag and protection behavior, the exact 357-V1 FF camera contact side and
pin orientation, and enclosure fit. The fixed THERM resistor provides no real
cell-temperature monitoring, so charging is restricted to controlled conditions
within the battery's documented 0--45 degrees C charging range.
