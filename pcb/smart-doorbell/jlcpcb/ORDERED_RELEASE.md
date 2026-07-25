# Smart Doorbell Rev B ordered release

This directory records the exact PCB/PCBA package submitted to JLCPCB and paid
on 2026-07-15. The local files were independently rehashed after submission.

## Ordered upload files

| Purpose | File | SHA-256 |
| --- | --- | --- |
| Fabrication | `production_files/GERBER-smart-doorbell.zip` | `308147675567d52b2c969652221e89e74eb338f233dc546a0f6a8c3fbe2cefe4` |
| Assembly BOM | `production_files/BOM-smart-doorbell.csv` | `259df70355df98f8d69fe8e36467e864358dbc74648b01cfa6c947faa1909097` |
| Placement | `production_files/CPL-smart-doorbell.csv` | `4568b054b96dbd2f7c47d4aba105d7f1d151f0aa36835294cef4a8d51ea8b242` |

Run `sha256sum -c ORDERED_RELEASE.sha256` from this directory for a
machine-readable verification of all three files.

JLCPCB PCB order: `Y7-6841583A`. The signed-in PCBA order page displayed the
prefix `SMT026071560509`. The order contained five PCBs and five top-side
economic PCBA assemblies.

## Delivered hardware

The physical Smart Doorbell Rev B boards now in hand were received on
2026-07-23 from the following retained JLCPCB order:

| Item | Identifier |
| --- | --- |
| JLCPCB web order | `W2026071513150300` |
| PCB order | `Y7-6841583A` |
| PCBA order | `SMT026071560509` |

This is the batch whose final DFM placement review was completed before
shipment. It is the only valid physical baseline for Rev B bring-up. The later
duplicate order (`W2026071513290609`, PCB `Y8-6841583A`, PCBA
`SMT026071560541`) was cancelled and must not be used as a hardware reference.

## Source and generation provenance

| Item | Saved/generated timestamp (America/Toronto) |
| --- | --- |
| Schematic | 2026-07-14 19:24:03.678626984 -0400 |
| PCB | 2026-07-14 23:39:10.332831080 -0400 |
| CPL | 2026-07-14 23:39:10.802850493 -0400 |
| BOM | 2026-07-14 23:39:10.804061473 -0400 |
| Corrected top mask | 2026-07-14 23:39:11.101603061 -0400 |
| Fabrication ZIP | 2026-07-14 23:39:11.161865321 -0400 |

The package was generated with KiCad 10.0.4 and kicad-jlcpcb-tools. Project-local
LCSC assignments and placement corrections are in `project.db`.

The plugin omitted top-mask apertures for NPTH pads. Its configured
post-generation hook, `../../../scripts/fix-jlcpcb-mask.sh`, replaced the top
mask with a KiCad-native plot, verified the 0.5 mm MK1 acoustic aperture and all
four 2.7 mm mounting-hole apertures, rebuilt the 13-member ZIP, and tested the
embedded result.

The assembly package contains 37 BOM groups and 84 unique top-side placements.
Its per-reference LCSC assignments match
`../production/Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv`. The intended 23
hand-solder, DNP, test-point, and mechanical references are absent from both BOM
and CPL.

The saved DRC report has one known source-metadata parity warning: R37 has a
blank PCB `LCSC Part #` field while the schematic carries `C21190`. R37 is DNP
and is absent from both ordered assembly files, so this does not change the PCB
or PCBA package. The two excluded J2 silkscreen-to-edge warnings are intentional
for the USB-C connector overhang.

## Post-order rule

Saving or editing the local KiCad project cannot alter the files already held by
JLCPCB. It can, however, make these manufacturing outputs older than the source
files. Do not regenerate, replace, or re-upload an ordered file without treating
the result as a new release candidate and repeating the manufacturing review.

At the time of this snapshot, JLCPCB data preparation was still pending. Final
CAM/DFM files must be checked against this Rev B release before production-file
approval.

The corrected Rev C candidate is isolated under `rev-c/` with its own manifest
and hashes. None of those files replaces the three Rev B uploads above.
`sha256sum -c ORDERED_RELEASE.sha256` must continue to pass from this directory;
run the separate Rev C hash check only from `rev-c/`.
