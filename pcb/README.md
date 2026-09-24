# Doorlink PCB

The routed [KiCad schematic](smart-doorbell/smart-doorbell.kicad_sch) and [PCB](smart-doorbell/smart-doorbell.kicad_pcb) are the source for Rev C connections, footprints, and population intent. The Rev C design uses an ESP32-S3-WROOM-1-N16R8, an OV5640 24-pin camera connector, MCP73871 battery charging and power path, TPS63802 3.3 V regulation, and a GPIO-controlled camera power switch.

## Manufacturing files

The [Rev C ordered release](smart-doorbell/jlcpcb/rev-c/ORDERED_RELEASE.md) identifies the exact Gerber, JLCPCB BOM, and placement files submitted for the assembled prototype. Its BOM contains the 85 JLCPCB-populated references. U1 (ESP32-S3) and MK1 (microphone) were added after PCB assembly; several optional footprints remain unpopulated. The [protected assignment record](smart-doorbell/production/Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv) retains per-reference LCSC assignments inherited from Rev B.

The [Rev B ordered release](smart-doorbell/jlcpcb/ORDERED_RELEASE.md) is historical. Its camera connector wiring differs from Rev C, so use the Rev C package for the photographed board. Do not regenerate or overwrite either ordered package.

The Rev C charger expects a suitable external 10 kΩ battery thermistor at J5; no thermistor is included in the PCB assembly BOM. See the [bring-up record](../docs/hardware-bringup.md) for the prototype checks and [board pin map](../main/board_pins.h) for firmware assignments.

Hardware design and manufacturing files in this directory: © 2026 Caleb Habesh, licensed under [CERN-OHL-P-2.0](LICENSE).
