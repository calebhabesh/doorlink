# Smart Doorbell Rev C ordered release

This directory records the isolated Rev C PCB/PCBA package submitted to JLCPCB
on 2026-07-26 as the minimum five-board prototype batch. It does not replace or
modify the immutable Rev B order recorded in `../ORDERED_RELEASE.md`.

## Exact ordered upload files

| Purpose | File | SHA-256 |
| --- | --- | --- |
| Fabrication | `production_files/GERBER-smart-doorbell.zip` | `7ebd19abdf2506559d982eb9ccf2855b6e97c4294295550f7871b72116f74491` |
| Assembly BOM | `production_files/BOM-smart-doorbell.csv` | `b8c32a878c7caabdf3b9f238a1eec08a2a92a1a69c329369d7b5a2bb62ae5043` |
| Placement | `production_files/CPL-smart-doorbell.csv` | `bb05144a3e20f71e30b5ff7071b05e1eb57f1dc3faacc909f88bdab1a21b15aa` |

Run `sha256sum -c ORDERED_RELEASE.sha256` from this directory whenever checking
the exact submitted package. Do not substitute files from the parent Rev B
paths or from `pcb/smart-doorbell/production/`.

These three hashes define the exact Rev C order identity. The tracked bytes and
hash manifest are immutable order evidence; any later regeneration is a new
release candidate.

## Source and generation provenance

- KiCad source title blocks: Smart Doorbell Project, revision C.
- Gerber generator: KiCad Pcbnew 10.0.5.
- Gerber generation timestamp: `2026-07-26T05:11:30-04:00`.
- Gerber project metadata: revision C.
- Git release tag: `pcb-rev-c-ordered-2026-07-26`.
- Project-local plugin database snapshot: `project.db`
  (`02549d60fb4e8063091f892665035068eef0c9bd039da6491a4b0a322385b46e`).
- Camera symbol library: `../../ov5640_pinout.kicad_sym`, referenced through
  `../../sym-lib-table`.
- Top-mask post-processing: `../../../../scripts/fix-jlcpcb-mask.sh`.

The exact source content used for this freeze was:

| Source | SHA-256 |
| --- | --- |
| `../../smart-doorbell.kicad_pcb` | `a29030c30a896c481777cf684eb9f4e7a3ffd209cc45e68b115e549563aee3d0` |
| `../../smart-doorbell.kicad_sch` | `32523098141959951881cd6337a0f85f3e38096cae66ab706157dfb62308d481` |
| `../../smart-doorbell.kicad_pro` | `f4404cd6d46fe735ec5cfc337edff98ec06ce8cfc56d595d8b6d1a6f49f1079d` |
| `../../ov5640_pinout.kicad_sym` | `064e62d8f5685142a9031123e77b1df5f2efce731c3e43bdf72688f9a9682b8a` |

All 108 PCB footprints are locked in this saved PCB source. Lock flags do not
change fabrication geometry; the regenerated package was independently
compared with a fresh KiCad export using the plugin's solder-mask subtraction
and routed-slot options.

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

## JLCPCB BOM and assembly-preview sign-off

Status: **signed off before order**.

- [x] The JLCPCB BOM-matching export contains the same 85 unique references as
      the ordered BOM and CPL.
- [x] All 38 matched rows retain the expected value, package, and JLCPCB/LCSC
      part number.
- [x] All placements are top-side; no bottom designators are present.
- [x] U9 pin 1 matches the PCB pin-1 marker and its 180-degree CPL orientation.
- [x] J3 pin 1 is at the `CAM24 / J3-1` end and the flex opening faces the
      `FLEX ENTRY` label.
- [x] U2, U4, U5, U6, U7, U8, D2, D3, J1-J4, and J6-J8 orientations match the
      Gerber overlay.
- [x] U1, MK1, R11, R21, R26, R31, R36, J5, J9, TP1-TP10, and H1-H4 are not
      placed.
- [x] BOOT1 and RESET1 use the intended B3U-1000P, C231329. The JLCPCB
      multiple-match warning was reviewed and accepted.
- [x] No substitutions, unresolved parts, bottom-side placements, or quantity
      changes remain.

| Field | Sign-off |
| --- | --- |
| Reviewer | Codex independent release audit; BOOT1/RESET1 confirmed by Caleb Habesh |
| Date | 2026-07-26 |
| Order timestamp (as supplied) | `2026-07-26 06:01:45` |
| PCB prototype ID | `Y9-6841583A` |
| PCBA ID | `SMT026072660566-6841583A` |
| Related web-order ID | `W2026072618014602` |
| Assembly quantity | Five top-side PCBAs |
| Preview evidence | `evidence/JLCPCB-assembly-preview-top.png` |
| BOM-match evidence | `evidence/JLCPCB-BOM-matching-2026-07-26.xlsx` |
| Evidence hashes | `ORDER_EVIDENCE.sha256` |
| Result | PASS — approved for the minimum Rev C prototype order |

The BOM evidence was downloaded from JLCPCB at
`2026-07-26 17:48:43`, identifies five assembled boards, and records an
estimated matched-parts total of `$57.3764`. Its 38 groups expand to the same
85 references as the ordered BOM/CPL. The source `JLCPCB Part #` column is
blank because the rows were selected by JLCPCB's matching system, but every
resulting matched part number was independently compared with the protected
assignments and passed.

## Post-order rule

Saving or editing the local KiCad project cannot alter the files already held
by JLCPCB. Do not regenerate, replace, or re-upload any ordered file. Any
future manufacturing output must use a new release directory, manifest, and
complete review.

This order establishes manufacturing-package and preview approval only. It
does not establish successful camera startup, OV5640 capture, current
consumption, charger/thermistor behavior, battery protection or polarity, RF
performance, audio performance, or enclosure fit. Perform the documented
current-limited first-article bring-up with a fresh known-good exact 357-V1
camera.
