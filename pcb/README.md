# Smart Doorbell Custom PCB

This directory contains the KiCad project files, schematics, and production files (Gerbers, CPL, BOM) for the Smart Doorbell custom PCB.

## Final Hardware & Assembly Reality

The files here track the design of the board. However, due to cost optimization and economy-level PCBA constraints with JLCPCB, the final assembled board differs slightly from the raw KiCad BOM:

### 1. MCU Module
*   **Designed For:** ESP32-S3-WROOM-1 or WROOM-2 series.
*   **Final Deployed Module:** `ESP32-S3-WROOM-1-N16R8`. This provides 16MB of Octal SPI Flash and 8MB of Octal SPI PSRAM, which is critical for buffering images from the OV5640 camera.
*   **Assembly Note:** This module is **hand-soldered** to the board after manufacturing to save on JLCPCB assembly costs.
*   **Sourcing:** Purchased via AliExpress (C$7.69 each).

### 2. Audio Input (Microphone)
*   **Module:** TDK InvenSense `ICS-43434` (I2S MEMS Microphone).
*   **Assembly Note:** Like the ESP32, this is excluded from the JLCPCB BOM and is **hand-soldered**.
*   **Sourcing:** Purchased via AliExpress (5pcs for C$11.78).

### 3. Bill of Materials (JLCPCB Assembly)

The following components are populated directly by JLCPCB during manufacturing:

| Part / Value                | Designators                                     | Package / Footprint                               | JLCPCB Part # |
|:----------------------------|:------------------------------------------------|:--------------------------------------------------|:--------------|
| **IC / Active**             |                                                 |                                                   |               |
| MAX98357A (Audio Amp)       | U4                                              | TQFN-16-1EP_3x3mm                                 | C910544       |
| AP2112K-3.3 (3.3V LDO)      | U3                                              | SOT-23-5                                          | C23380830     |
| XC6206P282MR (2.8V LDO)     | U7                                              | SOT-23-3                                          | C347374       |
| XC6206P152MR (1.5V LDO)     | U6                                              | SOT-23-3                                          | C424701       |
| MCP73831-2-OT (Charger)     | U2                                              | SOT-23-5                                          | C424093       |
| SRV05-4 (ESD Protection)    | U5                                              | SOT-23-6                                          | C7420376      |
| **Connectors & Switches**   |                                                 |                                                   |               |
| USB_C_Receptacle_16P        | J2                                              | HRO_TYPE-C-31-M-12                                | C165948       |
| FPC Connector (24-pin 0.5)  | J3                                              | Hirose_FH12-24S-0.5SH                             | C202112       |
| JST PH 2-pin (Battery/Spk)  | J1, J6                                          | JST_PH_B2B-PH-K                                   | C131337       |
| JST PH 3-pin                | J7                                              | JST_PH_B3B-PH-K                                   | C131339       |
| JST PH 4-pin                | J8                                              | JST_PH_B4B-PH-K                                   | C131334       |
| Pin Header (1x06)           | J4                                              | PinHeader_1x06_P2.54mm_Vertical                   | C37208        |
| Tactile Switch (SPST)       | BOOT1, RESET1                                   | SW_SPST_B3U-1000P                                 | C231329       |
| **Passives (0603)**         |                                                 |                                                   |               |
| 10uF Capacitor              | C1, C13, C20, C25, C26, C27, C3                 | 0603                                              | C19702        |
| 1uF Capacitor               | C14, C15, C16, C17, C18                         | 0603                                              | C15849        |
| 0.1uF Capacitor             | C10, C19, C2, C21, C22, C23, C5, C8, C9         | 0603                                              | C14663        |
| 100k Resistor               | R4, R7, R8                                      | 0603                                              | C25803        |
| 10k Resistor                | R1, R5                                          | 0603                                              | C25804        |
| 5.1k Resistor               | R10, R6                                         | 0603                                              | C23186        |
| 4.7k Resistor               | R13, R14                                        | 0603                                              | C23162        |
| 2k Resistor                 | R12, R2                                         | 0603                                              | C22975        |
| 1k Resistor                 | R3                                              | 0603                                              | C21190        |
| 330Ω Resistor               | R9                                              | 0603                                              | C23138        |
| LED                         | D2, D3                                          | 0603                                              | C2290         |

### 4. Known Bring-up Risks
Because of the hand-soldered components and tight clearances, the following risks must be mitigated during the initial hardware bring-up (see `docs/hardware-bringup.md`):
*   **Solder Bridges:** Especially under the ESP32-S3 module and on the tiny footprint of the ICS-43434 microphone.
*   **Camera Signal Integrity:** The DVP interface from the OV5640 runs at high speeds. While the pinout was optimized for trace routing, line capacitance or noise could still impact image quality.
*   **Audio Noise:** The I2S traces for the microphone and the MAX98357A amplifier are susceptible to noise from the nearby Wi-Fi antenna and fast digital lines.