# Smart Doorbell Rev C release candidate

This directory is the isolated Rev C PCB/PCBA upload candidate generated on
2026-07-25. It does not replace or modify the immutable Rev B order recorded in
`../ORDERED_RELEASE.md`.

## Exact upload files

| Purpose | File | SHA-256 |
| --- | --- | --- |
| Fabrication | `production_files/GERBER-smart-doorbell.zip` | `bd347d7fe29fca29ce14ad9221bd4397a655ddf2f940d71f7b1310913f32c426` |
| Assembly BOM | `production_files/BOM-smart-doorbell.csv` | `b8c32a878c7caabdf3b9f238a1eec08a2a92a1a69c329369d7b5a2bb62ae5043` |
| Placement | `production_files/CPL-smart-doorbell.csv` | `bb05144a3e20f71e30b5ff7071b05e1eb57f1dc3faacc909f88bdab1a21b15aa` |

Run `sha256sum -c RELEASE_CANDIDATE.sha256` from this directory immediately
before upload. Do not substitute files from the parent Rev B paths or from
`pcb/smart-doorbell/production/`.

## Source and generation provenance

- KiCad source title blocks: Smart Doorbell Project, revision C.
- Gerber generator: KiCad Pcbnew 10.0.5.
- Gerber project metadata: revision C.
- Project-local plugin database snapshot: `project.db`
  (`a4a1a420526bb2f69f5979c5d6959dfdd5853a01b4290034f544c0a668ba4b01`).
- Camera symbol library: `../../ov5640_pinout.kicad_sym`, referenced through
  `../../sym-lib-table`.
- Top-mask post-processing: `../../../../scripts/fix-jlcpcb-mask.sh`.

The 13-member fabrication ZIP contains files byte-for-byte identical to the
adjacent `gerber/` directory. The native top mask contains the 0.5 mm MK1
opening and all four 2.7 mm mounting-hole openings, including the corrected
lower-hole Y coordinate of 128.825 mm.

## Assembly record

The BOM and CPL contain the same 85 unique top-side references. All inherited
Rev B references retain the exact value/LCSC assignment recorded by
`../../production/Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv`.

The intentional population delta from that protected record is:

| Action | Reference | Value / part | LCSC |
| --- | --- | --- | --- |
| Add | C35 | 1 uF | C15849 |
| Add | R37 | 1 kOhm | C21190 |
| Add | U9 | TPS22919DCKR | C2149796 |
| Remove / DNP | R26 | 10 kOhm fixed THERM resistor | C15401 |
| Remove / DNP | R36 | 0 ohm camera-power bypass | C21189 |

U9 is top-side with a CPL rotation of 180 degrees. The generated top copper and
paste each contain all six U9 pad apertures. R31 remains DNP, so populated R37
dedicates GPIO42 to `CAM_PWR_EN`. R11 and R21 remain DNP camera contingency
links. U1 and MK1 remain hand-soldered.

J5 is only an external thermistor connection. No NTC is included in this BOM;
do not charge a battery until a suitable 10 kOhm-at-25-degrees-C NTC is wired
to J5 and thermally attached to the cell.

## Reproducible local gates

Run from the repository root:

```bash
python3 scripts/verify-camera-interface.py
python3 scripts/verify-rev-c-release.py

kicad-cli sch erc --severity-all --exit-code-violations \
  --output /tmp/smart-doorbell-rev-c-erc.rpt \
  pcb/smart-doorbell/smart-doorbell.kicad_sch

kicad-cli pcb drc --schematic-parity --all-track-errors \
  --severity-all --exit-code-violations \
  --output /tmp/smart-doorbell-rev-c-drc.rpt \
  pcb/smart-doorbell/smart-doorbell.kicad_pcb
```

The ERC report may include only the intentionally excluded MAX98357A exposed-pad
warning. The DRC report may include only the two intentionally excluded USB-C
silkscreen warnings. Any other violation blocks upload.

## External JLCPCB gate

Status: **not signed off**.

After uploading the exact three hashed files, inspect the actual JLCPCB
assembly preview before payment and record:

- [ ] 85 total placements are shown.
- [ ] U9 pin 1 matches the PCB pin-1 marker and its 180-degree CPL orientation.
- [ ] U2, U4, U5, U6, U7, U8, D2, D3, J1-J4, and J6-J8 orientations are
      unchanged and correct.
- [ ] U1, MK1, R11, R21, R26, R31, R36, J5, J9, TP1-TP10, and H1-H4 are not
      placed.
- [ ] No unexpected substitutions, unresolved parts, bottom-side placements,
      or quantity changes are present.
- [ ] Preview screenshots/order reference and reviewer/date are recorded below.

| Field | Sign-off |
| --- | --- |
| Reviewer | |
| Date | |
| JLCPCB quote/order reference | |
| Preview evidence path | |
| Result | |

Do not mark this candidate ordered or pay until every checkbox passes.
