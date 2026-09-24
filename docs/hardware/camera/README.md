# OV5640 357-V1 seller documentation

The pinout below is transcribed from a mechanical drawing supplied by the
AliExpress seller for the fixed-focus `DCXYX-LZTKQJ-5M-357-V1` OV5640 camera
module. The seller's original image is not redistributed here.

The drawing identifies the 24 contacts as:

| Pin | Signal | Pin | Signal |
| --- | --- | --- | --- |
| 1 | NC | 13 | XCLK1 |
| 2 | AGND | 14 | Y8 |
| 3 | SIO_D | 15 | DGND |
| 4 | AVDD | 16 | Y7 |
| 5 | SIO_C | 17 | PCLK |
| 6 | RESET | 18 | Y6 |
| 7 | VSYNC | 19 | Y2 |
| 8 | PWDN | 20 | Y5 |
| 9 | HREF | 21 | Y3 |
| 10 | DVDD | 22 | Y4 |
| 11 | DOVDD | 23 | AF_GND |
| 12 | Y9 | 24 | AFVDD |

The seller drawing lists AVDD as 2.8 V, DOVDD as 1.8/2.8 V, and DVDD as
1.5 V. It also depicts a 24-contact, 0.5 mm-pitch FPC tail.

This is seller-provided evidence rather than a manufacturer-controlled
datasheet. The received module has now been compared with the drawing, but that
does not make the module a universal OV5640 mechanical standard. Every future
module must still be checked for physical pin 1, exposed-contact side, all
24 functions, and rail voltages. Pins 23 and 24 remain isolated by the board's
DNP contingency links for the fixed-focus module.

## Received first-article orientation

Front and rear photographs of the received camera were inspected on
2026-07-25. The flex marking reads `DCXYX-LZTKQJ-5M-357-V1 FF`, and the
24 exposed contacts are on the rear/shield side, opposite the lens. This
matches the seller drawing.

Both PCB revisions use J3 `FH12-24S-0.5SH(55)`, a horizontal bottom-contact
connector. With the PCB component side facing up, its antenna at the top, and
USB-C at the bottom:

- approach J3 from the antenna/U1 side;
- keep the camera lens facing up, away from the PCB;
- keep the exposed flex contacts facing down, toward the PCB;
- J3 pin 1 is the right-hand end near J7, and J3 pin 24 is the left-hand end
  near MK1.

This orientation mates the contact surfaces, but it does **not** preserve pin
order. The seller drawing's bottom/contact-side view has the camera body above
the tail, pin 1 at the right, and pin 24 at the left. Viewed from the
lens/printed side, that order is mirrored: pin 1 is at the left. Therefore the
photographed lens-up/contact-down insertion maps camera pin `n` to J3 pin
`25 - n`: camera pin 1 reaches J3 pin 24, and camera pin 24 reaches J3 pin 1.
The camera advertisement's enlarged tail drawing also visibly labels pin 24 at
the left and pin 1 at the right in the depicted contact-side view; it is
additional evidence of the same orientation, not evidence that the lens-side
orders match.

## Rev C physical mapping

Rev C keeps J3 in the mechanically correct Rev B orientation and corrects the
electrical nets on its physical pads. The footprint pad numbers remain honest:
camera pin `n` mates with physical J3 pad `25 - n`.

| Camera pin | Function | Physical J3 pad | Rev C board net |
| ---: | --- | ---: | --- |
| 1 | NC | 24 | NC |
| 2 | AGND | 23 | `GND` |
| 3 | SIO_D | 22 | `CAM_SDA` |
| 4 | AVDD | 21 | `+2V8` |
| 5 | SIO_C | 20 | `CAM_SCL` |
| 6 | RESET | 19 | `CAM_RST_2V8` |
| 7 | VSYNC | 18 | `CAM_VSYNC` |
| 8 | PWDN | 17 | `CAM_PWDN_2V8` |
| 9 | HREF | 16 | `CAM_HREF` |
| 10 | DVDD | 15 | `+1V5` |
| 11 | DOVDD | 14 | `+2V8` |
| 12 | Y9 / D7 | 13 | `CAM_D7` |
| 13 | XCLK1 | 12 | `CAM_XCLK_2V8` |
| 14 | Y8 / D6 | 11 | `CAM_D6` |
| 15 | DGND | 10 | `GND` |
| 16 | Y7 / D5 | 9 | `CAM_D5` |
| 17 | PCLK | 8 | `CAM_PCLK` |
| 18 | Y6 / D4 | 7 | `CAM_D4` |
| 19 | Y2 / D0 | 6 | `CAM_D0` |
| 20 | Y5 / D3 | 5 | `CAM_D3` |
| 21 | Y3 / D1 | 4 | `CAM_D1` |
| 22 | Y4 / D2 | 3 | `CAM_D2` |
| 23 | AF_GND | 2 | Isolated contingency net through DNP R21 |
| 24 | AFVDD | 1 | Isolated contingency net through DNP R11 |

The top silkscreen labels the ends `CAM1 / J3-24` and `CAM24 / J3-1`. The
project-local `ov5640_pinout.kicad_sym` library gives every J3 symbol pin both
the camera-pin identity and physical pad number. Run
`python3 scripts/verify-camera-interface.py` from the repository root to export
a fresh schematic netlist and enforce the complete mirrored map, contingency
links, level networks, and Rev C camera load switch.

Do not insert or power this camera directly in Rev B J3 again. A simple
lens-side flip is not a correction because J3 is bottom-contact and requires
the exposed flex contacts on the bottom. Recovering Rev B requires a verified
24-way crossover/interposer. Rev C needs no J3 rotation or flex twist, but its
first powered camera test still requires a known-good camera and the staged,
current-limited plan in `docs/hardware-bringup.md`.

The Rev C KiCad correction was submitted as the minimum five-board prototype
order on 2026-07-26 after its actual JLCPCB BOM match and top-side assembly
preview passed. Its exact upload files, hashes, and preview evidence are
recorded in `pcb/smart-doorbell/jlcpcb/rev-c/ORDERED_RELEASE.md`. Preserve both
immutable ordered releases: Rev B as fault history and Rev C as the corrected
first-article package. Insert or remove any flex only with every power source
disconnected, and keep R11 and R21 DNP for this fixed-focus module.
