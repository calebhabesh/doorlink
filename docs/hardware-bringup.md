# Smart Doorbell hardware bring-up

This guide preserves the measured Rev B first-article record and defines the
separate Rev C first-power plan. Revision-specific population and rail behavior
must not be mixed: Rev B has the defective same-number J3 routing and an
always-powered camera rail, while Rev C has the physically mirrored J3 routing
and GPIO42-controlled camera power.

## Scope and hardware baseline

The receiving, rework, and measured-progress sections below apply only to the
delivered Rev B batch:

| Item | Identifier |
| --- | --- |
| JLCPCB web order | `W2026071513150300` |
| PCB order | `Y7-6841583A` |
| PCBA order | `SMT026071560509` |
| Received | 2026-07-23 |

The retained order contains five PCBs with five top-side economic PCBA
assemblies. The JLC assembly installed 84 top-side reference placements on each
board. These two parts were deliberately excluded and must be fitted by hand:

| Reference | Exact part | Method |
| --- | --- | --- |
| U1 | ESP32-S3-WROOM-1-N16R8 | Reflow/hot air |
| MK1 | ICS-43434 bottom-port I2S microphone | Reflow/hot air |

For the delivered Rev B boards only, do not populate the DNP or contingency
parts during this guide: R11, R21, R31, R37, U9, J5, or J9. In particular,
R11 and R21 keep J3 pads 24 and 23 isolated by default. Rev C deliberately
populates R37 and U9 and is covered in the dedicated section below.

### Materials cross-reference

The following status comes from the current repository records plus the latest
receipt report. "Recorded existing" means a prior project record identifies the
item, but it must still be found and inspected before use. It does not prove
that every accessory or connector is present.

| Material | Recorded specification | Status and use in this guide |
| --- | --- | --- |
| Rev B PCBA | Five boards from the retained JLC order above | Received; select one first article |
| Enclosure | Black solid-cover `DS-AG-0813`-style ABS, nominal `80 x 130 x 70 mm` | Received with the housing order; measure the actual clone before drilling |
| Adhesive Velcro | Thin battery-retention material | Received; dry-fit only, do not attach during electrical bring-up |
| PCB standoffs | M2.5 female-to-female, selected height `10 mm`, outside diameter ideally no more than `5 mm` | To order; existing `6 mm` parts are about `1 mm` shorter than the unused molded bosses; verify actual thread, height, screw engagement, and PCB clearance |
| U1 | `ESP32-S3-WROOM-1-N16R8`, previously purchased through AliExpress | Recorded purchased; verify the exact `N16R8` marking before reflow |
| MK1 | TDK InvenSense `ICS-43434`, five previously purchased through AliExpress | Recorded purchased; verify dry-pack condition, package marking, and pin 1 |
| Battery | 1S `103450`, nominal `3.7 V`, `50 x 34 x 10 mm`, JST-PH 2.0, advertised `2000 mAh` | Recorded existing; the record does not establish pack protection, so verify protection and polarity |
| Camera | Fixed-focus `DCXYX-LZTKQJ-5M-357-V1` OV5640, 24-pin 0.5 mm FPC | Recorded selected; leave disconnected in this guide |
| Button | 22 mm white-LED metal momentary button | Recorded existing; reserve its approximately `31 mm` internal depth during dry fit |
| Speaker | `8 ohm`, `1 W`, approximately `24 x 15 mm` oval speaker | Recorded existing; leave disconnected in this guide |
| Mic duct | Silicone tube, `2 mm ID x 4 mm OD`, 1 m | Recorded existing; use only for a non-destructive clearance check |
| Light-pipe stock | `LPA3.5-21.1 mm` sample, ten `3 x 100 mm` clear acrylic rods, ten black 3 mm clip bezels | Recorded purchased; do not cut until the measured lid spacing is known |
| Optional PIR | AM312, approximately 13 mm dome and 25 mm overall depth | Recorded existing; leave disconnected in this guide |
| Harness materials | JST-PH 2.0 pigtails including 4-pin 26 AWG, heat shrink | Recorded existing; not needed for rail validation |

Before starting, locate the exact U1, MK1, battery, and a suitable power lead.
The enclosure, Velcro, standoffs, camera, button, speaker, PIR, light pipes, and
wiring are not required to prove the bare PCBA power tree. Missing mechanical
consumables must not delay the electrical first-article test.

The records do **not** confirm receipt of every item in the mechanical shopping
list. Confirm these separately before permanent enclosure work: matching M2.5
screws and nylon washers, plastic-compatible structural epoxy, battery-side
fishpaper/Kapton insulation, flexible neutral-cure sealant, foam/EVA speaker
gasket, and any acoustic vent membrane.

## Equipment and safety

Prepare an ESD-safe work surface, magnification, a multimeter with insulated
probe tips, a regulated 5 V bench supply with adjustable current limit, and a
known-good protected one-cell LiPo. A thermal camera is useful but not required.
Also prepare secure miniature grabbers or soldered temporary power leads,
no-clean flux, suitable solder paste, hot air plus bottom preheat/hot plate as
needed, tweezers, IPA for board surfaces, and a timer. Use a USB-C power source
or USB power meter only after the controlled bench-supply check passes.

Do not hold loose multimeter probes on TP3 and TP9 while reaching for the supply
switch. Use mechanically secure, polarity-marked leads; TP3 is not protected
against a reversed bench connection. Keep USB, the battery, and the bench
supply disconnected whenever resistance mode is selected or soldering is in
progress.

Before every voltage measurement, confirm that the meter lead is in its
`V`/`ohms` jack, not the current jack. Never place a meter configured for
current directly across a rail. Do not probe the closely spaced live USB-C
contacts; use the test points instead.

On Rev B, the populated 10 kOhm R26 makes THERM see a nominal temperature but
does not measure the cell. On Rev C, R26 is DNP and J5 is only an external
thermistor solder-pad connection; no NTC is included in the JLCPCB BOM. Do not
charge a battery on Rev C until a suitable 10 kOhm-at-25-degrees-C NTC has been
wired to J5 and thermally attached to the cell. Any initial charge must be
attended, at room temperature, on a non-flammable surface, with a protected
cell whose datasheet permits the designed approximately 213 mA charge current.
Do not use a swollen, dented, punctured, deeply discharged, unprotected, or
polarity-unknown pouch cell.

## First-article record

Create a record for the selected board before changing it:

| Field | Record |
| --- | --- |
| Board label/serial | |
| Top and bottom photographs | |
| U1/MK1/DNP inspection result | |
| Ambient temperature | |
| Bench-supply model and settings | |
| Battery measured voltage and polarity | |
| Resistance readings before U1 | |
| Rail/current readings before U1 | |
| Resistance and rail/current readings after U1 | |
| Resistance and rail/current readings after MK1 | |
| Enclosure and standoff measurements | |

Use the same meter, range, probe polarity, and settling time for all resistance
comparisons:

| Rail | As received | After U1 | After MK1 |
| --- | --- | --- | --- |
| TP3 `+5V` to TP9 | | | |
| TP4 `+BATT` to TP9 | | | |
| TP5 `SYS` to TP9 | | | |
| TP6 `+3V3` to TP9 | | | |
| TP7 `+2V8` to TP9 | | | |
| TP8 `+1V5` to TP9 | | | |
| TP10 `CAM_3V3` to TP9 | | | |

Record powered results at each assembly stage:

| Measurement | As received | After U1 | After MK1 |
| --- | --- | --- | --- |
| Supply voltage/current limit | | | |
| Input current at 5 s / 60 s | | | |
| TP3 `+5V` | | | |
| TP4 `+BATT` | | | |
| TP5 `SYS` | | | |
| TP6 `+3V3` | | | |
| TP7 `+2V8` | | | |
| TP8 `+1V5` | | | |
| TP10 `CAM_3V3` | | | |
| Warmest part and temperature | | | |

Retain at least one untouched PCBA until the first article passes. If a reading
is suspicious but not a hard short, compare it with that untouched board before
rework.

## Test points

Use TP9 as the fixed ground reference. Attach a grabber to TP9 first and use one
hand-held insulated probe for the positive test point. These USB-only
expectations describe Rev B:

| Test point | Net | USB-only expectation |
| --- | --- | --- |
| TP3 | `+5V` / USB VBUS | Near the applied 5 V |
| TP4 | `+BATT` | Do not use as a pass/fail reading with no cell fitted |
| TP5 | `SYS` | Near the active power source; it is not the 3.3 V rail |
| TP6 | `+3V3` | About 3.3 V |
| TP7 | `+2V8` | About 2.8 V |
| TP8 | `+1V5` | About 1.5 V |
| TP9 | `GND` | Ground reference |
| TP10 | `CAM_3V3` | About 3.3 V through populated R36 (Rev B only) |

TP1 and TP2 are not supply test points. On Rev B they expose the contingency
nets at J3 pads 24 and 23; because Rev B is physically mirrored, those pad
numbers must not be called the installed camera-pin numbers. On Rev C, TP1
exposes camera pin 24 through physical J3 pad 1 and TP2 exposes camera pin 23
through physical J3 pad 2.

For the Rev B first powered check, use these as investigation bands rather than
production calibration limits: TP6 `3.20-3.40 V`, TP7 `2.70-2.90 V`, and TP8
`1.45-1.55 V`. TP10 should closely track TP6 because R36 is the populated
zero-ohm bypass.

On Rev C, R36 is DNP and U9 is the only path from `+3V3` to `CAM_3V3`.
GPIO42/R37 must default low. With U1 absent or firmware holding GPIO42 low,
TP10, TP7, and TP8 should be off; TP6 remains about 3.3 V. After controlled
camera-power enable, TP10 should be about 3.3 V, TP7 about 2.8 V, and TP8 about
1.5 V. A grossly wrong, oscillating, or collapsing rail is a failure; record
exact values rather than writing only "pass."

## 1. Receiving inspection and non-destructive fit check

1. Reconcile the shipping label with the retained order identifiers above.
   Do not mix this batch with the cancelled duplicate order.
2. Count the five PCBAs and check each board for shipping damage, bow, cracked
   MLCCs, damaged connectors, or loose parts. Keep each board in its ESD
   packaging except while inspecting it.
3. Label one board as the first article and photograph both sides before any
   rework. Confirm U1 and MK1 are empty. Confirm R11, R21, R31, R37, U9, J5,
   and J9 are unpopulated; bare TP1-TP10 and H1-H4 are intentional.
4. Under magnification, inspect factory placements and joints. Pay particular
   attention to U2, U4-U8, L1, J1-J4, J6-J8, polarized D2/D3, and small parts
   near the empty U1/MK1 lands. The protected per-reference assembly record is
   `pcb/smart-doorbell/production/Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv`.
5. Without installing screws, place the unpowered board in the received
   enclosure. Record inside width, height, base depth, lid depth, ribs/bosses,
   seam overlap, and the usable flat mounting area. Confirm that the roughly
   `59 x 91 mm` PCB fits and that the USB-C overhang is not forced against ABS.
6. Test the received standoff/screw stack with the four `2.7 mm` M2.5 mounting
   holes on their `51.4 x 51.4 mm` pattern. Record height, outside diameter,
   screw length, washer stack, and underside clearance. Do not force an
   incorrect thread and do not bond anything yet.
7. Temporarily place the battery on the right wall, button in the lower-centre
   lid volume, speaker at a side/bottom grille candidate, and mic tube under the
   upper-left MK1 area. Check volume only: do not flex the pouch, compress the
   tube enough to bow the PCB, peel adhesive backing, drill, or cut parts.

Photographs and measurements from this step turn the marketplace enclosure and
standoff descriptions into verified first-article dimensions.

## 2. Before soldering: test the assembled PCBA

1. With no USB, battery, camera, speaker, or external connector attached, use
   resistance mode from TP9 to TP3, TP4, TP5, TP6, TP7, TP8, and TP10. A
   capacitance-charging reading is normal. A stable near-zero-ohm reading or
   sustained continuity beep is not; stop and compare with another untouched
   board before applying power. Record the settled reading and probe polarity,
   because semiconductor paths can make the two directions differ.
2. Connect the bench supply while it is off: positive lead to TP3 and negative
   lead to TP9. Verify polarity at the attached leads with the meter before
   connecting them to the PCBA. This powers the same `+5V` net as USB without
   probing USB-C contacts.
3. Set 5.0 V and a 100 mA current limit, then enable the supply. A brief
   capacitor inrush is normal. If the supply remains in current limit, a part
   heats, there is an odour, or a rail is missing, turn it off immediately. If
   there is no fault indication but normal startup is limited by 100 mA, increase
   the limit cautiously to 250 mA and repeat.
4. Record input current after approximately 5 seconds and 60 seconds. There is
   no fixed pass current yet; repeatability and the absence of foldback or
   heating matter more than a guessed number.
5. Measure TP3, TP5, TP6, TP7, TP8, and TP10 against TP9. Record the readings.
   The regulated rails must be close to 3.3 V, 2.8 V, and 1.5 V as listed above.
   `SYS` follows the selected power source, so do not reject it merely because it
   is not 3.3 V.
6. Keep power applied for one minute while checking U2, U8, L1, U6, and U7 for
   abnormal heating. Disconnect power before moving clips or changing meter
   mode. Do not use a finger as the only temperature instrument around a
   suspected fault.
7. Remove the bench supply. Then perform one USB-C connector check without a
   battery: use a normal 5 V USB-C supply or a USB power meter, not loose meter
   probes in the receptacle. Confirm the same rail readings and no abnormal heat.

### Battery and charger check

Do this only after the USB-only test passes.

1. Inspect the pouch and verify that its protection circuit is documented or
   physically present. Measure its open-circuit voltage. If it is swollen,
   damaged, polarity-unknown, unexpectedly below its permitted discharge
   voltage, or protection cannot be established, do not connect it.
2. With all power removed, verify board and harness polarity independently:
   J1 pin 1 must have continuity to TP4 (`+BATT`) and J1 pin 2 to TP9 (`GND`).
   Verify which harness contact reaches battery positive. Do not trust wire
   colour or generic JST-PH polarity.
3. Connect the protected 1S LiPo with USB disconnected. Verify TP4 matches the
   cell voltage, TP5 follows the battery path, and TP6, TP7, TP8, and TP10 are
   regulated correctly.
4. Disconnect the cell. Reconnect the current-limited 5 V source to TP3/TP9,
   using a limit that permits the approximately 213 mA programmed charge plus
   the measured system load; 400 mA is a reasonable initial ceiling after the
   USB-only check has passed. Reconnect the cell and observe the first few
   minutes of charging. The charge current is designed for approximately
   213 mA but will vary with cell state, input limit, and system load. Watch
   battery voltage and U2 temperature; stop for unexpected heating, smell,
   swelling, or unstable rails.
5. Record TP3-TP10, supply input current, battery voltage before/after, U2
   temperature if measurable, and D2 charge-indicator behavior. Supply current
   is not the same as battery charge current, and a nearly full cell may not
   enter constant-current charge.
6. Do not leave this first charge unattended. Remove input power first, then the
   battery, before soldering either hand-installed component.

## 3. Solder U1: ESP32-S3-WROOM-1-N16R8

Use the exact N16R8 module: 16 MB flash and 8 MB PSRAM. Do not fit a look-alike
WROOM variant.

1. Read the module label under magnification and photograph proof of the exact
   `N16R8` variant. Handle the module with ESD precautions. Ensure USB and the
   LiPo are absent. Clean the U1 footprint with IPA, let it dry fully, and apply
   a small, even amount of solder paste to the castellated pads and segmented
   centre ground pad. Do not flood the pads.
2. Orient the module using physical landmarks, not the text alone: its antenna
   end must point into the large top-board antenna keep-out, and the module's
   pin-1 mark must align with the U1 footprint's pin-1 triangle at the upper
   left corner below the antenna end.
3. Reflow with hot air using controlled, even heating. The module manufacturer
   specifies one lead-free reflow with a 235--250 degrees C peak for 30--70 s;
   use that as the component limit rather than repeatedly reheating the module.
4. Allow the board to cool naturally. Under magnification, inspect every
   visible castellated joint and check alignment. The hidden centre ground joint
   cannot be visually certified after placement; its confidence comes from
   paste control, a proper thermal process, alignment, and the electrical
   checks. Inspect the antenna keep-out and do not place copper tape, wiring,
   standoffs, the battery, or other metal over the antenna region.
5. With power removed, repeat the resistance checks from TP9 to TP3, TP5, TP6,
   TP7, TP8, and TP10 and compare them with the before-U1 record. Repeat the
   5 V smoke test at 100 mA. If no fault is evident but U1 startup causes
   foldback, raise the ceiling in controlled steps, up to 500 mA for this
   no-peripheral test. Record current at 5 seconds and 60 seconds. Stop if U1 or
   any regulator heats abnormally or any regulated rail collapses.
6. Do not interpret USB enumeration or the behavior of an unflashed module as a
   hardware pass/fail in this phase.

## 4. Solder MK1: ICS-43434 microphone

MK1 is a bottom-port microphone. Its acoustic opening must sit directly above
the board's 0.5 mm MK1 acoustic bore. Keep paste, flux, adhesive, tape, and
cleaning liquid out of that bore; the microphone cannot work through an
obstructed port.

1. Verify the part marking and pin-1 convention against the ICS-43434 package
   drawing. Ensure all power sources are absent. Apply minimal paste to the six
   MK1 pads. Align the microphone's pin-1 mark to the triangular pin-1 marker on
   the footprint. Do not infer orientation from package shape alone.
2. Reflow with low airflow and even heating so the small package is not blown
   sideways. Do not direct the air jet through the acoustic bore or repeatedly
   reheat the microphone.
3. After cooling, inspect all six joints under magnification and confirm that
   the bore remains open from the board underside.
4. With power removed, compare resistance from TP6 to TP9 with the pre-MK1
   record and inspect for bridges between adjacent MK1 pads. Repeat the
   current-limited 5 V smoke test; confirm TP6 remains close to 3.3 V and input
   current has not changed unexpectedly.

## 5. Post-solder mechanical dry fit

After the board is fully cool, cleaned, and unpowered:

1. Reinstall only the temporary standoff/screw stack. Confirm no metal hardware
   can touch pads or traces and the PCB does not bow.
2. Recheck the proposed left-side `2 x 4 mm` mic tube path. Its window must
   eventually align to the 0.5 mm PCB acoustic bore, not merely the MK1 package.
3. Recheck the right-wall `50 x 34 x 10 mm` battery volume with insulation and
   Velcro thickness included. Keep the pouch entirely on the deep-base side of
   the seam and out of the antenna keep-out.
4. Check that the 22 mm button's approximately 31 mm rear keep-out, camera FPC,
   USB-C cable approach, speaker, wiring bends, and light-pipe lines do not
   intersect the PCB or battery.
5. Dry-fit and photograph the stack with the selected `10 mm` standoffs.
   Confirm that they clear the unused molded bosses with tolerance, the mic
   tube is not crushed, the PCB remains flat, and the lid closes without the
   button, camera FPC, wiring, or other front-mounted parts touching the PCB.

Do not epoxy standoffs, apply Velcro, cut rods, or drill the enclosure yet.
Camera, speaker, microphone, button, LED, PIR, and USB flashing tests can still
change the best opening locations. Any holes will also invalidate the stock
enclosure's ingress claim unless they are properly sealed.

## 6. Hardware-only exit criteria

Stop here and begin software bring-up only when all of the following are true:

- The received batch and first-article photographs are recorded.
- The enclosure and received standoff dimensions are measured, and all
  unconfirmed accessories are listed rather than assumed present.
- U1 and MK1 are correctly oriented and cleanly soldered.
- USB-only and battery-only tests have stable TP6, TP7, TP8, and TP10 rails.
- USB plus battery charging was observed briefly and safely.
- No rail is shorted, no component overheats, and the current-limited supply did
  not remain in foldback.
- The MK1 acoustic bore is visibly unobstructed.
- Before/after resistance, voltage, current, and fit measurements are retained.

At this point, leave the camera and other external peripherals disconnected
until the software bring-up procedure begins with USB enumeration and flashing.

## First-article progress record

The first Rev B article reached the USB-only software checkpoint on 2026-07-25.
This is a partial record, not completion of the battery/charger or peripheral
exit criteria above.

- One of the five PCBAs was visually inspected with no obvious defect. The
  intended DNP references were empty, and the board plus loose standoffs
  dry-fitted successfully in the enclosure.
- Before hand assembly, TP3, TP4, TP5, TP6, TP7, TP8, and TP10 all measured
  open circuit to TP9 in resistance mode; the ground-contact check measured
  `0 ohm`.
- The as-received USB-only rails were TP3 `5.06 V`, TP4 `4.15 V`, TP5
  `5.06 V`, TP6 `3.33 V`, TP7 `2.81 V`, TP8 `1.50 V`, and TP10 `3.30 V`.
- U1, an `ESP32-S3-WROOM-1-N16R8`, was hand-installed with Sn63/Pb37. Its
  optional exposed ground pad was not soldered; the perimeter joints were
  visually inspected as good. MK1 was then installed with hot air; its
  alignment, attachment, and acoustic bore were visually inspected as good.
  The post-installation resistance checks passed.
- With unflashed U1 running, TP7 rose to `3.12 V` but returned to `2.81 V`
  while RESET1 was held. This was treated as GPIO-state backfeeding through
  the unloaded camera control networks.
- Native USB enumerated as Espressif `303a:4001` on `/dev/ttyACM0`.
- A core-only ESP-IDF 6.0.1 image was flashed with 16 MB DIO flash at 80 MHz
  and 8 MB octal PSRAM at 80 MHz. The boot log reported ESP32-S3 revision 0.2,
  a detected 16 MiB flash, an 8 MiB PSRAM device, and `SPI SRAM memory test
  OK`. The application-level N16R8 check passed.
- The core-only image disables camera, I2S, amplifier, Wi-Fi, MQTT, and deep
  sleep. It holds camera XCLK (GPIO12), PWDN (GPIO21), and RESET (GPIO40) low,
  leaves the remaining camera pins as floating inputs, and emits a one-second
  safe-state heartbeat.
- With that image running from USB and no battery or external peripheral
  connected, TP6 measured `3.32 V`, TP7 `2.81 V`, TP8 `1.50 V`, and TP10
  `3.32 V`. No suspicious heat, smell, unstable reading, or other behavior
  was observed. TP7 returning to its baseline supports the GPIO-backfeeding
  diagnosis.

Input current, instrumented component temperature, LiPo protection and
polarity, battery-only operation, and charging remain unverified. Camera and
permanent enclosure wiring also remain untested.

### Camera power attempt

Camera bring-up stopped at the power gate on 2026-07-25. Photographs confirmed
that the received flex is marked `DCXYX-LZTKQJ-5M-357-V1 FF`, has its exposed
contacts on the rear/shield side, and was inserted into bottom-contact J3 with
the lens and printed side up. With power removed and the camera connected,
TP6, TP7, TP8, and TP10 each settled at open circuit to TP9 rather than showing
a hard short.

Two brief USB-only trials produced an audible high-frequency whine, solid D2,
and rapid heating at U2:

1. The first trial used the original empty-connector idle state with camera
   PWDN low, RESET low, and XCLK low.
2. The second trial used a verified camera-attached idle image with PWDN high,
   RESET low, XCLK low, amplifier off, and a normal one-second heartbeat.

USB was removed promptly in both cases. Because the symptom persisted with
PWDN asserted, it must not be treated as a camera-initialization software
problem. Do not power the camera again from unrestricted USB. Resume only with
a camera-disconnected current baseline and a regulated 5 V bench supply at
TP3/TP9 with a controlled current limit. Direct camera reconnection remains
prohibited until the physical pin-order mismatch documented below is corrected
and verified unpowered.

After the second attempt, the camera was removed and allowed to cool. The J3
and flex contacts appeared undamaged, the camera itself had not heated, and U2
was the observed hot component. With all power and the camera removed, TP3,
TP4, TP5, TP6, TP7, TP8, and TP10 again each settled at open circuit to TP9.
This rules out a persistent hard short but does not clear the camera or its
powered load behavior. A direct attempt to measure resistance between the
camera's fine pin 15 `DGND` and pin 23 `AF_GND` contacts appeared open circuit,
but reliable probe contact could not be established; treat that result as
inconclusive, not as a confirmed camera measurement.

#### Camera read-only diagnosis

The 2026-07-25 seller drawing, received-camera photographs, routed netlist, PCB
footprint, and Hirose connector documentation identify a physical pin-order
mismatch:

- The flex has exposed contacts on its rear/shield side. In the seller
  drawing's contact-side view with the camera body above the tail, pin 1 is at
  the right and pin 24 is at the left. Viewed from the lens/printed side, the
  order is mirrored and pin 1 is at the left.
- J3 is the bottom-contact `FH12-24S-0.5SH(55)`. The routed footprint places
  J3 pin 1 at the right near J7 and J3 pin 24 at the left near MK1 when the
  component side is viewed upright.
- The tested insertion had the lens/printed side up and contacts down. It
  therefore mated camera pin `n` to J3 pin `25 - n`, not pin `n` to pin `n`.
  Camera pin 1 reached J3 pin 24 and camera pin 24 reached J3 pin 1.

The defective Rev B schematic net assignments were an exact same-number pinout
match to the seller drawing:

| J3 pin | Routed board net | Seller camera function |
| --- | --- | --- |
| 1 | NC | NC |
| 2 | `GND` | `AGND` |
| 3 | `CAM_SDA` | `SIO_D` |
| 4 | `+2V8` | `AVDD` |
| 5 | `CAM_SCL` | `SIO_C` |
| 6 | `CAM_RST_2V8` | `RESET` |
| 7 | `CAM_VSYNC` | `VSYNC` |
| 8 | `CAM_PWDN_2V8` | `PWDN` |
| 9 | `CAM_HREF` | `HREF` |
| 10 | `+1V5` | `DVDD` |
| 11 | `+2V8` | `DOVDD` |
| 12 | `CAM_D7` | `Y9` |
| 13 | `CAM_XCLK_2V8` | `XCLK1` |
| 14 | `CAM_D6` | `Y8` |
| 15 | `GND` | `DGND` |
| 16 | `CAM_D5` | `Y7` |
| 17 | `CAM_PCLK` | `PCLK` |
| 18 | `CAM_D4` | `Y6` |
| 19 | `CAM_D0` | `Y2` |
| 20 | `CAM_D3` | `Y5` |
| 21 | `CAM_D1` | `Y3` |
| 22 | `CAM_D2` | `Y4` |
| 23 | isolated by DNP R21 | `AF_GND` |
| 24 | isolated by DNP R11 | `AFVDD` |

The tested reversed insertion instead created these critical mismatches:

| Camera pin/function | Landed on | Consequence |
| --- | --- | --- |
| 2 `AGND` | J3.23, isolated | Analog ground not connected as intended |
| 4 `AVDD` | J3.21, `CAM_D1` | Analog supply not powered |
| 10 `DVDD` | J3.15, `GND` | Camera core supply driven to board ground |
| 11 `DOVDD` | J3.14, `CAM_D6` | I/O supply not powered |
| 15 `DGND` | J3.10, `+1V5` | Camera digital ground driven to 1.5 V |
| 21 `Y3` | J3.4, `+2V8` | Camera data output driven from the 2.8 V rail |
| 14 `Y8` | J3.11, `+2V8` | Camera data output driven from the 2.8 V rail |
| 23 `AF_GND` | J3.2, `GND` | AF ground connected despite fixed-focus DNP intent |
| 24 `AFVDD` | J3.1, NC | AF supply remains unpowered |

This explains why asserting PWDN did not remove the fault: PWDN itself was not
connected to J3.8 in the tested orientation, and the fault was rail/ground and
I/O misapplication rather than camera initialization.

The camera rails are not firmware-switched on the assembled Rev B board.
Populated R36 directly joins `+3V3` to `CAM_3V3`; DNP U9 is only a contingency
load switch. U6 is an always-fed 1.5 V LDO and U7 is an always-fed 2.8 V LDO.
U8 is an always-enabled 3.3 V buck-boost converter from `SYS`, with its enable
pulled from `SYS` through R24. Thus the reversed flex applies the bad rail
mapping immediately whenever the board is powered.

U2 sits in series from USB `+5V` to `SYS`, so all downstream camera-fault
current passes through its internal system power path before U8 and the camera
LDOs. Rev B straps MCP73871 SEL low and PROG2 high for nominal USB 500 mA
selection, but the MCP73871 input-current control prioritizes the system load
and does not actively limit current demanded by that load. This makes U2
heating under a downstream overload plausible. The audible whine is more
likely an overloaded/cycling U8/L1 or the USB source than U2, which is not a
switching converter. The observed hot-part location was not instrumented, so
U6, U8/L1, and U2 must all be measured during any later controlled test.

D2 is connected only to the MCP73871 active-low `STAT1` output; it is not an
overcurrent indicator. Solid D2 proves that STAT1 was low, which is compatible
with an active charge state or a temperature/timer fault. STAT2 is unconnected,
so D2 alone cannot distinguish those states. The normal no-camera flashing is
consistent with repeated no-battery/charge-state transitions but has not been
instrumented.

The leading fault is therefore a PCB-to-flex physical pin-order incompatibility,
not camera firmware, PWDN state, or unrestricted-USB capacity. A static
soldering short is less likely because the disconnected rails were nominal
before the attempts and all listed rail-to-ground checks returned to open
circuit afterward. The camera and regulators are not cleared: powered reverse
bias and I/O injection can conduct without leaving a measurable unpowered hard
short, and either brief attempt may have damaged the camera.

Do not reconnect this flex directly to J3, even with current limiting. Before
any powered camera test:

1. Obtain a regulated 5.0 V bench supply with adjustable current limit or an
   inline USB current meter. An inline meter is sufficient only for the
   camera-disconnected baseline; use an adjustable current-limited supply for
   the first corrected-camera powered test. Keep the battery and all other
   peripherals absent.
2. Establish a camera-disconnected baseline at a 100 mA limit from TP3/TP9.
   If normal core-only startup reaches the limit without rail collapse,
   increase only enough to pass the measured baseline, initially no higher than
   250 mA. Record input current, TP3, TP5, TP6, TP7, TP8, TP10, and measured
   temperatures at U2, U6, U7, and U8/L1.
3. Provide a non-destructive 24-way crossover/interposer or a corrected PCB
   connection. With all power absent, prove all 24 paths, including camera
   pins 2 and 15 to board ground, pin 4 to 2.8 V, pin 10 to 1.5 V, pin 11 to
   2.8 V, and pins 23/24 isolated. Do not accept visual orientation alone.
4. With a corrected pin map, PWDN asserted, RESET low, and XCLK low, set the
   initial supply ceiling to the measured no-camera baseline plus 50 mA and no
   more than 250 mA. Apply power for no more than one second. Stop immediately
   for current-limit operation, more than 5% rail collapse, whine, odor, or
   measurable temperature rise.
5. Only after the one-second check is stable, repeat for five seconds while
   measuring TP5, TP6, TP7, TP8, and TP10. Do not release RESET or start XCLK
   until the corrected standby rail/current test passes.

No direct-camera powered step is authorized until the pin-order correction and
the camera-disconnected current baseline are both proven.

#### Rev B disposition and PCB reorder gate

The camera advertisement drawing itself visibly labels the depicted tail ends
as pin 24 at the left and pin 1 at the right. That evidence is consistent with
the seller pinout table and received-camera photographs. The drawing is a view
of the exposed-contact/rear side; using those labels as though they were shown
from the lens side caused the physical mirror error.

This 24-pin OV5640 module arrangement appears to be a common module convention,
but it is not a pinout or flex-orientation standard imposed by the OV5640
sensor. Do not assume that another listing is either compatible or reversed
from this one. A replacement module is acceptable only if a numbered mechanical
drawing, exposed-contact side, rail voltages, and all 24 functions independently
prove the required physical mating map.

There is no firmware fix or simple drop-in rework for the populated Rev B J3
interface. Rev C is the separately identified production correction. It keeps
the connector location and orientation, routes camera pin `n` to physical J3
pad `25 - n`, and labels both ends with camera and J3 identities. A
purpose-built, fully continuity-tested 24-way crossover could still recover a
Rev B board, but no such interposer has been designed. Do not attempt a 24-wire
bodge, cut traces, force the flex upside down, or treat a
same-side/opposite-side FFC description as proof of electrical order.

The in-repository Rev C gates now require all of the following:

1. Both KiCad title blocks and the generated Gerber project metadata identify
   revision C.
2. `python3 scripts/verify-camera-interface.py` passes the complete physical
   map, DNP contingency links, level networks, and U9 power circuit.
3. ERC and schematic-parity DRC contain no real violations beyond the recorded
   intentional exclusions.
4. The Rev C BOM and CPL contain the same 85 unique populated references. The
   protected Rev B references keep their exact value/LCSC assignments; the
   intentional population delta is added C35/R37/U9 and removed R26/R36.
5. The Rev C Gerber ZIP matches its generated directory, passes archive
   integrity and mask-aperture checks, and is recorded with the exact BOM and
   CPL hashes in `pcb/smart-doorbell/jlcpcb/rev-c/ORDERED_RELEASE.md`.
6. The actual JLCPCB assembly preview is checked for 85 placements, all DNPs,
   and pin-1 orientation, especially U9 at the corrected 180-degree CPL
   rotation. This is an external upload-preview gate and cannot be replaced by
   a local KiCad or CSV inspection.

The first five gates are reproducible locally. Gate 6 passed against the actual
JLCPCB BOM match and top-side preview before the minimum five-board Rev C
prototype order was submitted on 2026-07-26; its evidence is retained in the
ordered release directory. Corrected files do not establish that the
twice-stressed Rev B camera or power path is undamaged.

## Rev C first-power and camera sequence

1. With no battery, camera, or U1 installed, inspect the population and perform
   resistance/continuity checks. Prove all 24 camera paths against the table in
   `docs/hardware/camera/README.md`.
2. Apply regulated 5.0 V at TP3/TP9 with a 50-100 mA current limit. Verify
   `SYS` and `+3V3`. With U9 default-off through R38, its internal pulldown, and
   GPIO42 low/uninstalled, `CAM_3V3`, `+2V8`, and `+1V5` must remain off.
3. After installing the exact U1, use radio-disabled test firmware and a
   conservative limit. With no camera, hold RESET low, PWDN high, XCLK inactive,
   and camera data pins high-impedance; enable GPIO42/U9 and verify TP10 at
   about 3.3 V, TP7 at about 2.8 V, TP8 at about 1.5 V, temperatures, and
   shutdown discharge.
4. Power off fully and insert a known-good camera lens-up/contact-down. Do not
   reuse the stressed camera for the initial test.
5. Repeat the safe-control state, enable U9, verify rails and current, then let
   the camera driver release RESET/PWDN and start XCLK. Stop immediately for
   current limiting, rail sag, whine, odour, or rapid heating. Firmware defines
   `CAM_PWR_EN_PIN` as GPIO42 and performs this order in
   `main/camera_power.c`.
6. Test battery charging separately only after a suitable external
   10 kOhm-at-25-degrees-C NTC is wired to J5 and thermally attached to the
   cell. R26 is DNP and no thermistor is included in the Rev C JLCPCB BOM.

## Rev B peripheral progress record

### MK1 I2S microphone test

MK1 passed an initial electrical and acoustic-response test on 2026-07-25 with
the camera, battery, speaker, button, and PIR disconnected. A microphone-only
image kept the amplifier disabled and configured I2S RX at 16 kHz with 32-bit
stereo frames: WS on GPIO4, SCK on GPIO5, and microphone SD on GPIO6.

The left slot, selected by MK1's grounded LR input, continuously reported
changing samples while the unused right slot remained all zero. Taps near the
MK1 acoustic bore produced captured 24-bit sample ranges of approximately
`14,019,064` (`-7,490,168` to `6,528,896`) and `9,945,958`
(`-4,042,580` to `5,903,378`), well above the ambient response and without
reaching the signed 24-bit limits. This confirms working I2S clocks/data, the
expected channel selection, and a strong acoustic response. It does not yet
constitute a saved or quality-assessed visitor-audio recording.

### J6 speaker test

The J6 speaker path passed a conservative first test on 2026-07-25 with an
`8 ohm`, `1 W` speaker connected directly across OUTP and OUTN. A speaker-only
image kept the camera and microphone RX disabled, drove the shared I2S clocks
at 16 kHz, and sent a 1 kHz triangle-wave burst at only `512/32767` amplitude
for approximately 320 ms every 2.28 seconds. The firmware reported repeated
complete bursts without an I2S write error.

The quiet beeps were clearly audible without high-frequency whine or harsh
distortion. U4 and U2 both remained cool. This passes the initial J6, U4,
GPIO7 data, GPIO4/5 clock, and GPIO44 enable-path check. OUTP and OUTN remain
a bridged output: neither terminal may ever be grounded. Higher-level playback
and subjective enclosure-volume testing remain deferred.

### J8 button and ring-LED test

The J8 external button and LED paths passed on 2026-07-25. With the PCB upright
and USB-C at the bottom, the installed four-wire pigtail mapped left-to-right
as black, red, white, and yellow. The pigtail was attached directly to the
button terminals as follows:

- J8.1 black to button NO
- J8.2 red to button common
- J8.3 white to button LED positive
- J8.4 yellow to button LED negative

Before connection, the button's NO-to-common resistance measured open when
released and `0 ohm` when pressed. A button-only image kept camera, I2S,
amplifier, Wi-Fi, MQTT, and sleep disabled. GPIO48 toggled the ring LED visibly
on and off once per second, confirming the LED path and polarity. GPIO2
initially read released/high and logged distinct active-low press and release
transitions during repeated manual presses. The temporary terminal joins were
individually insulated with electrical tape; they are not final enclosure
wiring and still require soldered, heat-shrunk terminations after mechanical
routing is proven.

### J7 AM312 PIR test

The optional AM312 and the default Rev B PIR path passed on 2026-07-25. J7 was
wired as pin 1 `3V3`, pin 2 `PIR_SENSOR_OUT`, and pin 3 ground. The pigtail
colors at J7 were yellow, black, and red respectively. At the sensor, those
were connected by female Dupont sockets to the module pins marked `+`, `OUT`,
and `-`; this avoids relying on mirrored seller views for pin orientation.

A PIR-only image kept camera, I2S, amplifier, Wi-Fi, MQTT, and sleep disabled.
It monitored the populated R30 route from J7.2 to GPIO3 and mirrored the input
onto the onboard status LED. After startup stabilization, the log repeatedly
showed clean `IDLE/LOW` to `MOTION/HIGH` to `IDLE/LOW` cycles, including a
deliberate hand-wave test from approximately 30--60 cm. The visible status LED
tracked motion. This confirms the AM312, J7 supply/signal/ground wiring, R29
pulldown, populated R30 path, and GPIO3 input. GPIO42 was not used. The
temporary spliced harness is not final enclosure wiring.

After the peripheral tests, all external peripherals and the battery were
removed and the board was reflashed with the repository's core-only default.
The final boot again reported 16 MiB DIO flash, 8 MiB octal PSRAM, and
`N16R8 CHECK PASS`. It reported `Camera-attached idle=no`, held camera XCLK,
PWDN, and RESET low, and produced a stable safe-state heartbeat. This is the
firmware state left on the first article.

## Rev B next-phase handoff

The next guide should proceed in this order so a failed subsystem does not
obscure basic board health:

1. Enumerate native USB, enter download mode with BOOT1/RESET1 as needed, and
   flash a minimal Rev B image using the exact N16R8 flash/PSRAM configuration.
2. Recheck idle rails and current with firmware running.
3. Treat the Rev B camera interface as a known physical pin-order mismatch.
   Use a received Rev C board or provide a verified crossover; do not reconnect
   the camera directly to Rev B J3. Establish a camera-disconnected current
   baseline before any corrected, current-limited camera test.
4. Capture and listen to a real MK1 audio recording before sealing its duct;
   the initial I2S electrical and acoustic-response test has passed.
5. Validate higher-level speaker playback after enclosure positioning; the
   initial low-level J6 test has passed. J6 is a bridged output, so neither
   speaker lead may be grounded.
6. Finalize the tested J8 button/LED wiring only after enclosure routing is
   proven; its initial input and ring-LED tests have passed.
7. Finalize the tested optional PIR harness only after enclosure routing is
   proven. Its J7.2-to-GPIO3 path through populated R30 has passed; GPIO42
   remains the DNP fallback, not the Rev B default.
8. Only after peripheral positions are proven should the enclosure be drilled
   and the standoffs, Velcro, ducts, gaskets, and light pipes be finalized.

## References

- [Ordered release record](../pcb/smart-doorbell/jlcpcb/ORDERED_RELEASE.md)
- [Ordered Rev C release](../pcb/smart-doorbell/jlcpcb/rev-c/ORDERED_RELEASE.md)
- [Protected Rev B assembly BOM](../pcb/smart-doorbell/production/Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv)
- [Rev B enclosure handoff](enclosure-rev-b-handoff.md)
- [OV5640 seller drawing record](hardware/camera/README.md)
- [Hirose FH12-24S-0.5SH(55) product and drawing](https://www.hirose.com/product/p/CL0586-0521-0-55)
- [MCP73871 datasheet](https://www.microchip.com/content/dam/mchp/documents/APID/ProductDocuments/DataSheets/MCP73871-Data-Sheet-DS20002090F.pdf)
- [TPS63802 datasheet](https://www.ti.com/lit/ds/symlink/tps63802.pdf)
- [ESP32-S3-WROOM-1 datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [ICS-43434 product page](https://www.invensense.tdk.com/en-us/products/microphone/ics-43434)
