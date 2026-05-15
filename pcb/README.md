# Smart Doorbell Custom PCB

This directory contains the KiCad project files, schematics, and production files (Gerbers, CPL, BOM) for the Smart Doorbell custom PCB.

## Final Hardware & Assembly Reality

The files here track the design of the board. However, due to cost optimization and economy-level PCBA constraints with JLCPCB, the final assembled board differs slightly from the raw KiCad BOM:

### 1. MCU Module
*   **Designed For:** ESP32-S3-WROOM-1 or WROOM-2 series.
*   **Final Deployed Module:** `ESP32-S3-WROOM-1-N16R8`. This provides 16MB of Octal SPI Flash and 8MB of Octal SPI PSRAM, which is critical for buffering images from the OV5640 camera.
*   **Assembly Note:** This module is **hand-soldered** to the board after manufacturing to save on JLCPCB assembly costs.

### 2. Audio Input (Microphone)
*   **Module:** TDK InvenSense `ICS-43434` (I2S MEMS Microphone).
*   **Assembly Note:** Like the ESP32, this is excluded from the JLCPCB BOM and is **hand-soldered**.

### 3. Known Bring-up Risks
Because of the hand-soldered components and tight clearances, the following risks must be mitigated during the initial hardware bring-up (see `docs/hardware-bringup.md`):
*   **Solder Bridges:** Especially under the ESP32-S3 module and on the tiny footprint of the ICS-43434 microphone.
*   **Camera Signal Integrity:** The DVP interface from the OV5640 runs at high speeds. While the pinout was optimized for trace routing, line capacitance or noise could still impact image quality.
*   **Audio Noise:** The I2S traces for the microphone and the MAX98357A amplifier are susceptible to noise from the nearby Wi-Fi antenna and fast digital lines.