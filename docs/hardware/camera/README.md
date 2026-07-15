# OV5640 357-V1 seller documentation

`DCXYX-LZTKQJ-5M-357-V1-seller-drawing.jpg` is the mechanical and pinout
drawing supplied by the AliExpress seller for the fixed-focus
`DCXYX-LZTKQJ-5M-357-V1` OV5640 camera module.

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
datasheet. Before powering a first article, physically verify pin 1 and the
exposed-contact side of the supplied ribbon. Pins 23 and 24 remain isolated by
the board's DNP contingency links for the fixed-focus module.
