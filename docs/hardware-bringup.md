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
| Speaker | Original tested unit: `8 ohm`, `1 W`, approximately `24 x 15 mm`; current enclosure target: one seller-described `4 ohm`, `3 W`, `2.0`-terminal speaker from a five-piece pack | New speaker and metal cover recorded 2026-08-10; dimensions, connector fit, loudness, temperature, and finished-enclosure response remain unvalidated |
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

J5 connects directly to the MCP73871 `THERM` charging-safety input. It is not
routed to an ESP32 ADC and firmware cannot report thermistor temperature. The
ESP32 can read only battery voltage, through the R7/R8 100 kOhm/100 kOhm
divider and C10 filter on GPIO1.

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
3. With the camera, speaker, external button, and PIR disconnected, connect the
   protected 1S LiPo with USB disconnected. Verify TP4 matches the cell voltage,
   TP5 follows the battery path, and TP6, TP7, TP8, and TP10 are regulated
   correctly.
4. Flash the bounded battery diagnostic, disconnect USB, attach secure meter
   leads at TP4/TP9, reset the board on battery-only power, and record the meter
   reading. The image takes 64 samples once and then stops sampling. Because
   the native USB console is unavailable during battery-only operation,
   reconnect USB after the measurement; the diagnostic repeats the retained
   result every five seconds without resampling. Do this only after the J5 NTC
   is installed, because reconnecting USB enables the hardware charger. Record
   the GPIO1 ADC result beside the TP4 reading. Calibrated millivolts are
   reported when eFuse calibration is available, with only the nominal 2:1
   divider ratio applied. Do not use this first comparison as a low-battery
   cutoff. Reset the board for each additional comparison point.
5. Disconnect the cell. Reconnect the current-limited 5 V source to TP3/TP9,
   using a limit that permits the approximately 213 mA programmed charge plus
   the measured system load; 400 mA is a reasonable initial ceiling after the
   USB-only check has passed. Reconnect the cell and observe the first few
   minutes of charging. The charge current is designed for approximately
   213 mA but will vary with cell state, input limit, and system load. Watch
   battery voltage and U2 temperature; stop for unexpected heating, smell,
   swelling, or unstable rails.
6. Record TP3-TP10, supply input current, battery voltage before/after, U2
   temperature if measurable, and D2 charge-indicator behavior. Supply current
   is not the same as battery charge current, and a nearly full cell may not
   enter constant-current charge.
7. Do not leave this first charge unattended. Remove input power first, then the
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

### Fresh Rev C article progress record

Bring-up moved to a fresh Rev C article on 2026-08-04 after the first
populated Rev C board was quarantined following intermittent U2/MK1 heating.
The earlier symptom is treated as an isolated contamination or assembly fault,
not as a cleared design issue on that board.

- The fresh board has hand-installed U1 (`ESP32-S3-WROOM-1-N16R8`) and MK1
  (`ICS-43434`). With USB as the only power source, TP6 measured `3.32 V`; U2
  and MK1 remained cool and USB remained stable without resets.
- The boot log detected 16 MiB DIO flash and 8 MiB octal PSRAM, and the PSRAM
  memory test passed.
- A microphone-only image ran for more than 60 seconds without I2S errors or
  disconnects. MK1 produced changing data in the expected left slot while the
  unused right slot remained zero, consistent with its grounded LR pin.
- A speaker-only image kept camera power, camera controls, microphone RX,
  Wi-Fi, MQTT, and sleep disabled. It completed 14 low-level 1 kHz tone bursts
  over approximately 32 seconds without I2S errors, resets, brownouts, or USB
  disconnects. The intentionally quiet tones were crisp and clearly audible;
  U4 and U2 remained cool. The board was then returned to the core-only image,
  which holds GPIO42/CAM_PWR_EN and AMP_EN low.
- With that core-only image running, the Rev C camera-off rails measured TP10
  `0.13 V`, TP7 `0 V`, TP8 `0 V`, and TP6 `3.3 V`. The small unloaded TP10
  residual did not indicate an enabled camera rail; both downstream regulator
  outputs remained fully off.
- A Wi-Fi-only image kept camera power, camera controls, amplifier, I2S, MQTT,
  gateway upload, and sleep disabled. It associated successfully, obtained a
  DHCP lease, and held a stable `-29 dBm` link through repeated status checks
  without disconnects, resets, brownouts, or heap loss. U1 and U2 remained
  cool. The board was then returned to the core-only image with RF disabled.
- With the camera disconnected, a rails-only image held RESET low, PWDN high,
  XCLK low, and the camera data pins high-impedance. A bounded five-second U9
  pulse produced TP10 `3.31 V`, TP7 `2.77 V`, and TP8 `1.50 V`; each rail rose
  cleanly and discharged after U9 was disabled. No instability was reported.
  Core-only firmware was restored after every pulse so GPIO42/CAM_PWR_EN could
  not re-enable the rails on reset.
- A new, previously unpowered `DCXYX-LZTKQJ-5M-357-V1 FF` camera was inserted
  lens-up/contact-down in the corrected Rev C J3. With all power removed, TP10,
  TP7, and TP8 each measured open circuit to TP9. Camera-attached standby tests
  then passed at 100 ms, one second, and five seconds with RESET low, PWDN high,
  XCLK low, and data pins high-impedance. Loaded five-second rail measurements
  were TP10 `3.30 V`, TP7 `2.77 V`, and TP8 `1.50 V`; U2, U6, U7, U8/L1, and
  the camera remained cool without whine, odour, USB instability, or other
  fault indication.
- A camera-only one-frame image detected the OV5640 at SCCB address `0x3c`
  with PID `0x5640`, started 10 MHz XCLK, and captured a `160x120`, 2,600-byte
  JPEG with valid start/end markers. Two initial `FB-OVF` notices preceded the
  successful frame. The frame buffer was returned, the driver deinitialized,
  and GPIO42 shut the camera rails off before the post-test heartbeat. The
  board was then restored to core-only firmware.
- A second one-frame diagnostic encoded its captured JPEG only after returning
  the frame buffer, deinitializing the driver, and disabling GPIO42/U9. The
  1,715-byte JPEG was reconstructed from 2,288 base64 characters with valid
  `FFD8`/`FFD9` markers. Visual inspection showed a coherent room scene, bright
  light, and person with plausible colour and no striping, torn geometry, or
  obvious data-bit corruption. One initial `FB-OVF` notice preceded this
  successful frame. Core-only firmware was restored after export.
- A follow-up `320x240` QVGA capture completed without an `FB-OVF` notice. The
  6,193-byte JPEG was reconstructed from 8,260 base64 characters with valid
  markers after camera shutdown. Visual inspection showed recognizable room
  objects, stable geometry, plausible colour, and no striping or data-line
  artifacts; a bright foreground light was normally overexposed. Core-only
  firmware was restored after the one-frame test.
- A `640x480` VGA capture also completed without overflow. Its 12,600-byte
  JPEG was reconstructed from 16,800 base64 characters after camera shutdown
  and showed a coherent, artifact-free room/person scene. The image was upside
  down in the current loose-module orientation; that is a later sensor `vflip`
  configuration issue, not evidence of a data-path fault. Core-only firmware
  was restored after the test.
- A later XGA test selected `1024x768`, 10 MHz XCLK, `vflip=1`, `hmirror=1`,
  and three discarded warm-up frames. Its 29,181-byte JPEG was coherent and
  free of the severe coloured column artifact described below. QVGA, VGA, and
  XGA therefore passed through the OV5640 driver's binned-readout path.
- The automatic JPEG buffer was too small for initial QXGA attempts. A
  diagnostic-only 2 MiB PSRAM JPEG buffer allowed structurally valid
  `1600x1200` UXGA and `2048x1536` QXGA exports, but both were covered by
  severe narrow coloured vertical columns. The artifact persisted with 20 MHz
  and 10 MHz XCLK, three warm-up frames, and a brighter scene. Maximum QSXGA
  and WQXGA attempts were stopped after timeouts and were not retried.
- A `1600x1200` internal OV5640 colour-bar capture at the same 10 MHz XCLK was
  clean. Because the pattern traversed the sensor JPEG encoder, 8-bit DVP bus,
  ESP32-S3 camera peripheral, PSRAM buffer, base64 export, and USB serial path,
  this strongly reduced the likelihood that those stages or their
  PCLK/HREF/VSYNC transfer caused the scene-dependent columns. Its small,
  highly compressible JPEG did not exercise the same transfer duration as the
  noisy real-scene frame.
- Review of the register tables found that Espressif changes sampling
  increments when leaving binned mode but does not apply the corresponding
  full-readout analog and black-level-correction recipe. A bounded UXGA test
  applied the established full-readout values `0x3618=0x04`, `0x3612=0x29`,
  `0x3708=0x21`, `0x3709=0x12`, `0x370c=0x00`, `0x4001=0x02`, and
  `0x4004=0x06`. The resulting 92,270-byte `1600x1200` JPEG was coherent and
  the severe coloured columns were gone. This confirms the mode-table delta
  as the UXGA fix on this article.
- QXGA was then recaptured with the same full-readout recipe, 10 MHz XCLK,
  three warm-up frames, orientation correction, and the 2 MiB JPEG buffer. The
  `2048x1536`, 488,692-byte JPEG had valid markers and coherent geometry, but
  the severe coloured vertical columns remained. The recipe is therefore not
  a QXGA fix. Before another ordinary QXGA scene capture, use a QXGA internal
  colour bar to test the higher-volume digital path specifically, or measure
  TP10/TP7/TP8 during the active QXGA readout.
- The follow-up QXGA internal-colour-bar test kept those same settings and
  produced a clean `2048x1536`, 81,165-byte JPEG with valid markers. There was
  no narrow-column corruption in the solid bars. This strongly validates the
  QXGA-rate DVP/PCLK/HREF/VSYNC transfer, ESP32-S3 capture, PSRAM, base64, and
  USB export. It does not identically stress a noisy real scene: the solid bars
  compressed to only 81,165 bytes, so a data-dependent or long-transfer fault
  was not eliminated solely by this result. The fixed scene-wide one-pixel
  columns nevertheless pointed primarily to sensor readout/BLC or active rail
  quality rather than JPEG-stream corruption.
- Explicitly redoing OV5640 black-level calibration after the full-readout
  mode switch substantially reduced the original QXGA corruption. The bounded
  test applied the full-readout
  recipe, wrote `0x4003=0x88` to request BLC over eight frames, discarded ten
  settling frames, explicitly cleared `0x4003[7]`, and read the redo bit back
  as zero. Continuous BLC updates remained disabled (`0x4005=0x18`). The next
  `2048x1536`, 128,243-byte low-detail JPEG was coherent and no longer had the
  severe coloured columns. Mean absolute horizontal-neighbour RGB delta fell from
  `18.380` in the failed full-readout-only control to `2.001`; the 95th
  percentile fell from `60` to `9`. A separate test with continuous BLC
  enabled also passed (`1.618` mean, `8` at the 95th percentile), but the
  cleared one-shot result showed that continuous updates were unnecessary for
  that particular scene.
- A subsequent `2048x1536`, 310,057-byte capture containing a person exposed
  strong residual vertical fixed-pattern banding across the wall, face, and
  clothing. Its column/row high-frequency ratio was `15.18`, compared with
  `2.28` for the clean XGA reference. The earlier low-detail metric therefore
  overstated the result: the BLC sequence is a partial mitigation, not a QXGA
  fix. Do not accept the present QXGA image quality as normal or production
  ready.
- A fixed-control QXGA capture then disabled automatic exposure/gain, set the
  gain registers to minimum (`0x350a/0x350b=0x0000`), selected a 600-line
  exposure (`0x3500..0x3502=0x002580`), and fixed office white balance. The
  underexposed 113,487-byte image still showed vertical columns. Its high-pass
  per-column profile correlated `0.738` with the earlier automatic-gain subject
  frame despite the large brightness difference. The spatially recurring
  columns rule out high automatic gain as the root cause and strengthen the
  case for fixed sensor/readout/BLC offsets or fixed clock/power coupling.
- A covered-lens capture with those identical fixed controls produced a nearly
  uniform dark frame (mean `6.07/255`, standard deviation `0.71`). Its faint
  high-pass column signature still correlated `0.664` with the minimum-gain
  illuminated frame over the full width (`0.423` in the central crop), but its
  absolute column variation was small. The visible artifact therefore has a
  signal-dependent component, consistent with column/ADC gain mismatch or an
  incomplete full-readout/BLC configuration, rather than being solely a large
  additive dark offset. Fixed clock or rail coupling is not yet excluded.
- A brighter minimum-gain check increased the fixed exposure from 600 to 1,800
  lines while retaining fixed office white balance and all other QXGA settings.
  The column profile remained strongly repeatable: it correlated `0.821` with
  the 600-line image and `0.781` with the earlier automatic-control subject
  image. This rules out underexposure in the first minimum-gain test as the
  explanation for the recurring columns.
- A controlled back-to-back comparison then changed only BLC continuous-update
  bit `0x4005[1]`. Both images used minimum gain, 1,800-line exposure, fixed
  office white balance, the same stationary scene, an eight-frame BLC redo,
  and an explicitly cleared redo bit. On the central wall crop, changing
  `0x4005` from `0x18` to `0x1a` reduced adjacent-pixel luminance variation
  from `0.478` to `0.375` and reduced the high-pass column-profile standard
  deviation from `0.909` to `0.827` (approximately 9%). The two column
  profiles still correlated `0.976`, and the bands remained plainly visible.
  Continuous BLC is therefore a small mitigation, not the QXGA fix, and has
  not been enabled in the normal capture path on this evidence alone.
- The suspect camera was then replaced, with power removed, by a second OV5640
  module. Under the same minimum-gain, 1,800-line exposure, fixed-white-balance,
  frozen-BLC QXGA recipe, the alternate module produced a coherent subject
  image with dramatically less fixed-column structure. Comparable flat-wall
  crops measured a high-pass column standard deviation of `0.353` on the
  alternate module versus `0.909` on the original, while the column/row ratio
  fell from `8.00` to `2.31`. This sensor-to-sensor result identifies the first
  module as a substantial contributor; quarantine it rather than treating its
  QXGA output as representative of the Rev C PCB.
- Unrestricted automatic controls on the alternate module still amplified fine
  vertical noise in the dim, strongly backlit subject scene, although much less
  than the original module. On the same upper-left wall coordinates, the
  high-pass column metric fell from `6.340` on the original automatic-control
  subject image to `1.961` on the alternate module. A follow-up kept exposure
  and white balance automatic but fixed analog gain at approximately 2x
  (`0x350a/0x350b=0x001f`). The sensor selected an 885-line exposure and the
  resulting flat-region column metric fell to `0.504`, with a column/row ratio
  of `0.84`. This is the best practical QXGA result so far. It validates a
  low-gain exposure policy as a software mitigation on the good module, but
  longer exposure and subject-motion blur must be evaluated in realistic
  doorway lighting before adopting it as a production setting.
- A fixed-control alternate-module QXGA capture was repeated after moving the
  otherwise unchanged USB connection from a motherboard USB 2.0 header to a
  USB 3.2 header. The new 90,220-byte frame still showed the fine vertical
  pattern, so the header change did not cure the artifact. The framing changed
  between the saved USB-header captures, preventing a defensible numerical
  A/B amplitude claim; this test also does not exclude ripple generated after
  VBUS by the board power path and camera regulators.
- The OV5640 automatic gain ceiling was then programmed directly through
  `0x3a18/0x3a19`, because esp32-camera 2.1.6 passes the generic
  `gainceiling_t` enum index to registers that require the OV5640 encoded gain.
  With automatic exposure and white balance, a raw `0x0020` (2x) ceiling,
  16 discarded convergence frames, JPEG quality 12, and the established QXGA
  full-readout/BLC sequence, the alternate module selected an 885-line
  exposure and reached gain `0x0020`. The valid 186,535-byte QXGA JPEG had
  substantially more plausible full-scene colour than the fixed-office-WB
  diagnostic, but colored fixed-column structure remained visible across the
  face and smooth wall. An otherwise identical 94,021-byte UXGA capture made
  the same residual structure visible; lowering resolution alone did not make
  this 2x low-light profile production ready.
- A follow-up used a raw `0x0010` (nominal 1x) ceiling with QXGA automatic
  exposure and white balance. The ceiling registers read back `0x0010`; the
  capture registers reported gain `0x0013` and the same 885-line exposure.
  The validated retry produced a complete 142,941-byte JPEG and automatic AWB
  gains `0x0563/0x0400/0x0699`. Its colour was no longer subject to the fixed
  office-WB green cast, but the dim, strongly backlit scene was underexposed
  and residual colored columns remained visible. This is not representative
  of the intended well-lit corridor and must be repeated there before choosing
  between the lower-noise 1x policy and the brighter 2x policy. The first 1x
  serial export lost 1,224 JPEG bytes and showed a horizontal tear; the export
  helper now rejects any base64 stream whose received length differs from the
  firmware-reported length instead of accepting marker-valid partial data.
- The low exposure was traced to stale OV5640 anti-flicker timing rather than
  insufficient USB-header power. At 10 MHz XCLK the active QXGA registers
  reported a 25 MHz sensor system clock, HTS `2844`, and VTS `1968`, while the
  inherited table still limited exposure to 984 lines and used a 295-line
  50 Hz band step. The observed 885 lines were exactly three of those stale
  steps. The setup layer now derives 50/60 Hz band steps and maximum band counts
  from the live PLL, HTS, and VTS registers, sets both AEC maximum-exposure
  register pairs, explicitly selects the configured mains frequency, and
  disables multi-frame night mode. A 60 Hz retest computed 73-line bands and a
  1,964-line ceiling; automatic exposure selected 1,898 lines at the same raw
  `0x0020` gain ceiling. At this timing that is approximately 216 ms instead of
  the former 101 ms. The valid 158,011-byte QXGA capture was substantially
  brighter and retained plausible automatic white balance. Its lamp and monitor
  highlights clipped, and faint fixed vertical structure remained, so realistic
  corridor testing must still balance face brightness against motion blur.
  Normal camera initialization now enables the same timing calculation for the
  configured 60 Hz mains frequency and programs the proven raw `0x0020`
  automatic-gain ceiling. Normal camera initialization now uses the validated
  10 MHz XCLK; it still needs a corridor capture, an 8 MHz comparison, and a
  moving-subject check.
- The partial correction currently lives in the tracked `ov5640_mode_fix`
  setup layer.
  It applies the analog/full-array values only to OV5640 modes that exceed the
  driver's binned `1280x960` envelope, requests a bounded eight-frame BLC redo,
  leaves continuous BLC updates off, and clears the redo request after the
  settling frames. Normal camera initialization also applies the proven
  `vflip=1`/`hmirror=1` orientation and discards the eight BLC settling frames
  before returning a usable camera.
- Diagnostic firmware now supports an internal colour bar, a manual DVP PCLK
  divider, timing/readout register logging, and the bounded full-readout
  recipe/BLC redo. The manual PCLK control was retained for diagnostics but did
  not address the artifact. The component's temporarily extended
  frame-get timeout was returned from 15 seconds to its original 4 seconds
  after these tests, and the board was returned to core-only firmware.
- R26 was confirmed absent and an external MF52D B3950 10 kOhm NTC was
  soldered to J5 and attached to the battery with Kapton tape. It measured
  approximately `7.6 kOhm` immediately after installation, inside the
  MCP73871's charge-permitted THERM resistance window.
- A bounded GPIO1 ADC image was built and flashed with ESP-IDF 6.0.1. It keeps
  the external camera, speaker, button, PIR, amplifier, Wi-Fi, MQTT, and sleep
  disabled, takes 64 ADC samples once, applies eFuse curve calibration and the
  nominal R7/R8 2:1 divider, releases the ADC resources, and then holds the
  safe GPIO state. With USB connected and no cell, it reported `4.140 V` on
  `+BATT`, consistent with the charger's no-cell output.
- Battery-only operation passed with the external peripherals disconnected.
  The measured rails were TP4 `4.00 V`, TP5 `3.99 V`, TP6 `3.31 V`, TP10
  `0.16 V` and decaying, TP7 `0 V`, and TP8 `0 V`. The camera gate therefore
  remained off while SYS and 3.3 V operated normally.
- Reconnecting USB with the protected cell and J5 NTC installed produced a
  solid D2 charge indication. U2 and the battery remained cool. Opening the
  native USB monitor reset the MCU and truncated the retained battery-only
  voltage line after raw average `2324`; the post-reset charging-state sample
  reported `4.028 V` while TP4 simultaneously measured `4.05 V`. The ADC was
  therefore `22 mV` low (`0.54%`) at this first calibration point. Do not bake
  a correction or production cutoff into firmware until a lower-voltage point
  is also measured.
- After approximately five minutes of attended charging, TP3 measured
  `4.99 V`, TP4 `4.08 V`, TP5 `4.97 V`, and TP6 `3.31 V`. D2 remained solid
  and U2 remained cool. Compared with the simultaneous first-charge TP4
  reading, battery voltage rose by approximately `30 mV`; VBUS, SYS, and 3.3 V
  remained stable. No inline USB current meter was available, so input and
  battery charge current were not measured and remain open validation items.
- A dedicated battery-only integration image then performed one bounded QXGA
  capture/upload cycle. It copied the captured JPEG into owned PSRAM, returned
  the camera frame, deinitialized the driver, disabled GPIO42/U9, started Wi-Fi,
  uploaded one multipart event, stopped Wi-Fi, freed the JPEG copy, and entered
  a safe result hold. Audio, amplifier, MQTT client, and sleep remained
  disabled. The gateway health endpoint reported database, MQTT, storage, and
  gateway all `UP` before the test.
- The battery-powered cycle created gateway event `15` with event type
  `BATTERY_QXGA_BRINGUP`. The stored JPEG was complete and structurally valid:
  `2048x1536`, 184,999 bytes, baseline 8-bit sRGB. Doorlink displayed the image
  and the configured devices produced the expected doorbell notification/chime.
  Visual inspection showed coherent geometry, no tearing, and usable facial
  detail. Fine vertical structure remained visible in darker regions but was
  non-blocking. Bright lamp/monitor highlights clipped and the face was dark in
  this strongly backlit room, so corridor exposure and moving-subject tests
  remain necessary. The image was upside down only because the loose camera was
  physically dangling upside down; final rotation must be judged in its intended
  enclosure orientation.
- After the successful upload, D3 held solid and the shutdown readings were
  TP4 `3.99 V`, TP10 `0.10 V` and decaying, TP7 `0 V`, and TP8 `0 V`; U2, the
  camera, and battery remained cool. MK1 was unexpectedly very warm even though
  this image did not initialize I2S. Power was removed immediately. The
  integration result proves the camera/network/gateway path, but the board is
  not cleared for further powered testing until MK1 and the 3.3 V rail are
  checked unpowered. The ICS-43434 normally draws only sub-milliamp current, so
  perceptible heating is not normal.
- Review after the MK1 observation found that non-audio firmware disabled I2S
  but left GPIO4/WS and GPIO5/SCK floating. The ICS-43434 datasheet recommends
  stopping both clocks and pulling them to ground for standby. The shared core
  safe-state helper now explicitly drives WS, SCK, and speaker data low and
  makes microphone data an input with a pulldown. This closes a firmware safety
  gap but does not establish that floating clocks caused the heating; an
  assembly fault or damaged microphone remains possible.
- The operator then disclosed a recurring power-entry symptom that predates the
  battery integration test. Slowly or partially inserting either USB-C or the
  battery connector has sometimes produced a high-frequency whine perceived
  near U2. Removing power and reconnecting once or twice has previously made
  the sound disappear and allowed apparently normal operation. Because the
  symptom occurs with both input sources, the motherboard USB port or cable
  cannot be its sole cause. U2 is a linear charger; U8 is the switching
  converter and its MODE pin is strapped low for light-load power-save
  operation, so U8/L1 or a nearby ceramic capacitor is a more plausible
  acoustic source. Contact bounce or a slow input ramp can also make the
  always-enabled U8 repeatedly cross its startup threshold. Sound location was
  not instrumented, however, and this mechanism does not explain away the hot
  microphone. Stop partially engaging either connector and do not repower this
  article from an unrestricted source. The earlier TP6-to-TP9 readings of
  `OL` in one polarity and `4.81 kOhm` in the reverse polarity rule out a
  persistent hard short only; they do not clear a startup, oscillation, or
  powered semiconductor fault. The next powered test requires current limiting.
- A subsequent USB-only reconnection, with battery and camera absent, reproduced
  the startup symptom twice despite decisive connector insertion; power was
  removed promptly on each whine. The third connection started quietly and
  enumerated as the ESP32-S3 USB JTAG/serial device. The corrected core-only
  image then reported its safe-state heartbeat with WS/GPIO4 and SCK/GPIO5 held
  low. MK1 remained at ambient temperature during this quiet start. This is
  evidence against a persistent MK1 short and supports a startup-dependent
  fault, but the two immediately preceding whine events mean the power-entry
  issue remains reproducible and unresolved.

Battery-only operation, a five-minute attended USB charge, and the functional
camera/gateway/notification portion of a battery-only QXGA cycle have passed on
this article. Power was later resumed at the user's direction and MK1 remained
at ambient temperature, but the intermittent startup whine remains unresolved
and must stay in the bring-up record.

The next firmware milestone was implemented without flashing it to the article.
`app_main.cpp` now provides the C++ entry point and selects an explicit
`DoorbellController` production mode. The controller classifies GPIO2 EXT0
button and GPIO3 EXT1 PIR wakes, runs the proven QXGA capture into an RAII-owned
PSRAM copy, shuts down GPIO42/U9 before starting Wi-Fi, uploads a bounded event,
fully deinitializes Wi-Fi, and enters deep sleep. Held-button and latched-PIR
states are excluded for one sleep cycle with a 30-second recovery timer to avoid
rapid wake loops. The default build remains the core-only safe image. Separate
safe and production ESP-IDF 6.0.1 builds pass. On 2026-08-05, an isolated
wake-only image validated active-low GPIO2 EXT0 button wake and return to deep
sleep repeatedly with the camera, PIR, speaker, Wi-Fi, MQTT, I2S, and amplifier
disabled. D3 and the GPIO48 ring acknowledged each press. Skipping repeated
image validation only on deep-sleep wake improved the response, and an
RTC-fast wake stub made the acknowledgement subjectively much snappier by
driving both LEDs before the bootloader. No instrumented latency measurement
was made. With the AM312 subsequently connected, its startup activity settled
and deliberate motion repeatedly produced the diagnostic's three-pulse GPIO3
EXT1 wake indication before returning to deep sleep. The combined production
button event was then flashed and exercised with the known-good OV5640 fitted
and PIR, battery, and speaker absent. One press produced a `2048x1536`,
`131797`-byte JPEG, received HTTP `200`, triggered the configured device
notification, released Wi-Fi and camera resources, disabled GPIO42/U9, and
returned to deep sleep. The resulting image appeared on the Doorlink site with
correct orientation when the camera was held upright and framed a portrait.
Device logging showed Wi-Fi had an IP at `17.91 s`, the
upload took approximately `0.93 s`, and deep-sleep entry occurred at `18.89 s`
after wake. This is a latency baseline, not an optimized result; add explicit
camera-versus-association timing before changing the proven capture path.

The first production PIR attempt did validate GPIO3 wake, QXGA capture, camera
shutdown, Wi-Fi connection, retry handling, and return to deep sleep. It did
not complete the event: the `180521`-byte JPEG received HTTP `401` on both
upload attempts. A non-event empty POST from the Arch host using the firmware's
configured key also returned `401`, as did the no-key control. The ignored
Arch `.env` and `main/config.h` keys match each other, so the running Pi
gateway's effective `GATEWAY_API_KEY` must be reconciled with the firmware
before repeating this test. Read-only inspection over the configured `ssh rpi`
alias established that the gateway had restarted at 18:47 and loaded a
different key from its Pi `.env`; the file and running-process values matched.
The ignored local `main/config.h` was synchronized without logging the secret,
an empty authenticated probe passed the API-key filter, and production was
rebuilt and reflashed. The retry completed a `PIR_MOTION` event with a
`2048x1536`, `138608`-byte JPEG and HTTP `200` on its first attempt. Camera and
Wi-Fi resources were released, both wake sources were rearmed, and deep sleep
began at `21.74 s`. Doorlink display and notification observation for this PIR
event were subsequently confirmed by the user. Both arrived with noticeable
delay. The firmware now instruments early association, trigger acknowledgement,
RF teardown, BLC, AEC/AWB, final capture, reconnect, and upload durations. An
idempotent fast-trigger flow and an isolated GPIO48 fade diagnostic build
successfully. On 2026-08-05 the fast-trigger production image was flashed over
native USB and validated on Rev C. The first button and PIR trials reached the
gateway in `1.50-1.70 s`, but exposed an `ESP_ERR_NOT_SUPPORTED` return after
otherwise successful Wi-Fi teardown. ESP-IDF 6.0.1 documents
`esp_netif_deinit()` as unsupported once its process-global lwIP task exists;
the firmware was incorrectly treating that expected result as an RF teardown
failure and abandoning capture. The unsupported global deinit call was removed
while retaining destruction of the Wi-Fi driver, station netif, event handlers,
and default event loop.

After rebuilding and reflashing, two PIR events and one button event completed
the fast-trigger path. A fully traced PIR event associated at `1.14 s`, received
early-trigger HTTP `200` at `1.56 s`, stopped RF at `1.69 s`, completed QXGA
capture at `17.81 s`, uploaded a `146713`-byte JPEG with HTTP `200` at `20.40 s`,
disabled GPIO42/U9, rearmed both wakes, and returned to deep sleep. The button
event received early-trigger HTTP `200` at `1.40 s`, stopped RF at `1.45 s`, and
persisted one `2048x1536`, `107095`-byte JPEG under the same event ID. Gateway
telemetry reported the button event and `-31 dBm` RSSI; the chime log contained
the early invocation and no second invocation during image upload. The isolated
GPIO48 diagnostic subsequently validated a nonblocking fade-in on a true EXT0
button wake. After user review, the production animation was finalized as a
`1.8 s` fade in, `2.5 s` full-bright hold, and `1.8 s` fade out. D3 remains the
immediate RTC-stub acknowledgement; GPIO48 no longer flashes full before its
fade-in.

Nearby movement during the initial tests showed that leaving GPIO3 armed made
PIR events appear interleaved with manual button tests and caused apparently
random chime webhook invocations. Production was changed to arm GPIO2 only by
default. PIR remains proven and available through the explicit
`CONFIG_SMART_DOORBELL_ENABLE_PIR_EVENTS` option and the isolated wake
diagnostic. Button re-arm now also requires a stable HIGH level for `100 ms`.

Two clean, sequential, button-only production events then completed after the
previous cycle had returned to deep sleep. Event
`5c7f18d29e2387bd8f81e393c5bba249` received early-trigger HTTP `200` at
`1.83 s`, stopped RF at `1.95 s`, completed a `2048x1536`, `152126`-byte JPEG
upload at `19.86 s`, and logged `PIR production wake disabled` before sleep.
Event `1529017cecf3129dd9aebd25506105e8` independently received HTTP `200` at
`1.60 s`, stopped RF at `1.67 s`, uploaded a `151562`-byte QXGA JPEG at
`19.82 s`, and returned to button-only deep sleep. The Pi stored exactly one
row and one image for each ID and logged exactly one early chime invocation per
press; neither upload duplicated the chime.

Input-current measurement, a longer charge observation, and active/deep-sleep
current remain pending. Camera bring-up is sufficiently
complete to continue hardware validation: the
alternate module's remaining faint vertical structure is not a blocker for a
doorbell still image. Keep the first OV5640 module quarantined. QXGA
`2048x1536` at 10 MHz is now the production-still candidate; compare it with
8 MHz under actual corridor lighting and include a moving person before
freezing exposure policy. The approximately 216 ms result belongs to the
10 MHz diagnostic and motion blur remains unqualified. QSXGA `2560x1920` is
deferred because prior attempts timed out and its buffer, upload, energy, and
latency costs are not yet justified. If an
oscilloscope becomes available, compare
TP10/TP7/TP8 during VGA real-scene, QXGA colour-bar, and QXGA real-scene
operation.

After the USB-powered production button and PIR event paths passed, an initial
full-production battery-only test was paused when MK1 became hot under J1 battery power.
Assembly review on 2026-08-06 identified that paste had been applied to the five rectangular
signal/power pads but omitted from the circular annulus around the acoustic opening.
That annulus is MK1 pin 3 (`GND`). The KiCad footprint's paste layer provides four curved
apertures on the ground annulus while keeping the central 0.5 mm sound hole clear, matching
the microphone datasheet's stencil pattern. Without pin 3 grounded, MK1 suffered floating ground
latch-up during the LiPo battery startup voltage ramp.

On 2026-08-07, MK1 was re-attached using Sn42/Bi58 low-temperature solder paste applied to both
the five rectangular pads and the four ground annulus sections, keeping the center 0.5 mm acoustic
bore clear.

Subsequent testing confirmed complete resolution:
1. **USB Diagnostic Test**: 10-second clock-off standby phase completed cleanly. Live 24-bit PCM audio reported `left status=ACTIVE` with quiet `avg_abs` of 1,635-2,679 and speech/tap peak `avg_abs` of 294,892 (peak range 1,239,366). 0 I2S errors, 0 resets, 0 USB instability, and MK1 remained at room temperature with zero high-frequency whine.
2. **USB Production Event Test**: Single button press on GPIO2 triggered non-blocking LED ring fade (GPIO48), early Wi-Fi chime notification in ~1.4 s (HTTP 200), QXGA 2048x1536 OV5640 JPEG capture/upload, and clean return to deep sleep.
3. **Battery Power Test**: Connecting LiPo battery J1 directly produced zero heating across MK1 and U8/L1 (100% room temperature), zero high-frequency whine, and normal battery operation. The high-frequency switching whine occurred concurrently with MK1 heating during ungrounded latch-up; grounding Pin 3 resolved both symptoms in initial testing. The link between ungrounded MK1 latch-up current draw and U8/L1 switching noise is recorded as the primary suspected mechanism, subject to long-term monitoring across extended battery operational cycles. MK1 battery testing is cleared.

On 2026-08-10, the integrated production audio path was exercised end to end.
The uploaded visitor greeting played from the dashboard's **Play Audio** control
with subjectively good microphone quality, and a dashboard **Push to Talk**
reply played successfully through the doorbell speaker. The test also exposed
a noticeable delay between the button press and the start of visitor speech
capture. A first scheduling change overlapped the full local chime with camera
initialization/capture. A count test after flashing that build captured speech
starting around "4", confirming roughly four seconds of remaining button-to-mic
latency. The follow-up firmware now shortens the local acknowledgement chime at
roughly 400 ms, switches the shared half-duplex I2S bus to the microphone, and
records while early notification and camera work proceed. This second latency
change passes the ESP-IDF 6.0.1 production build and still requires a
flashed-board measurement.

The first immediate-greeting board run also exposed two follow-up issues.
Repeated button presses could claim the shared audio bus and defer chimes until
microphone capture ended, making local feedback feel unresponsive and risking
speaker noise around visitor audio. Chime playback is now explicitly suppressed
for the complete visitor-capture window: an active chime is stopped, and new or
retriggered button-chime requests are dropped rather than queued. Debounced
represses still restart the GPIO48 ring animation for immediate silent feedback.
Those presses are also excluded from the button mailbox, preventing a press
made during the silent window from starting a delayed follow-up recording.
Only the initial `DOORBELL_PRESS` cycle records a visitor greeting; later
`DOORBELL_REPRESS` cycles preserve local chime/retrigger behavior and cannot
open another speaker-suppression window.

The two paragraphs above record the intermediate firmware tested on 2026-08-10.
They were superseded later that day by the locked session behavior: the first
chime plays completely and continuing to hold records after it. The subsequent
interaction update preserves that non-interruptible first chime while restoring
immediate onboard acknowledgement for represses whenever I2S is idle. A repress
while a local chime is already audible rewinds the live PCM stream without
tearing down I2S. A repress held for 1,200 ms yields interruptible repress audio
to the visitor microphone; an arriving homeowner WAV may also interrupt it,
and repress chimes are skipped rather than queued during active dialogue. The
one-second minimum, 15-second cap, and this multi-press arbitration require
validation on the assembled board.

On 2026-08-15, integrated real-world testing found that the enclosure-target
4-ohm/3-watt speaker caused the local chime to stop partway through the alert
workflow and the board indication to go dark. Reinstalling the previously
tested 8-ohm speaker restored the expected workflow. This A/B result implicates
the increased speaker current and its overlap with camera startup, but does not
by itself distinguish supply droop, an ESP32 reset, or MAX98357A protection.
Firmware now defaults all production speaker output to 6 dB digital
attenuation, applies a 25 ms start/end ramp, and prevents camera power-up until
the active chime has fully drained and the amplifier is muted. The early remote
notification is still sent before this wait. Reset-reason logging was also
added. These mitigations do not modify the immutable PCB releases and require a
flashed-board A/B retest with the 4-ohm speaker before that speaker is accepted
for production.

The 2026-08-16 experimental follow-up keeps the full 6 dB chime ahead of camera
startup, then permits the 18 dB/250 ms acknowledgement throughout RF shutdown,
U9 rail startup, and camera capture. Repeated presses may restart that prefix.
When AMP_EN is already active, camera power-up verifies that the chime player
owns it in the bounded overlap profile and waits for the 25 ms ramp plus a 5 ms
guard before enabling U9. Any other AMP_EN owner still aborts camera startup.
This is a software power-limiting experiment, not evidence that full chime and
camera loads are safe together. Validate `SYS`, reset reason, amplifier
temperature, camera completion, and image integrity with the 4-ohm speaker.
Whole-home alert receipts are keyed per physical press, while the Home
Assistant calls use a 750 ms leading/trailing coalescing window; retries remain
idempotent for each `pressId`.

The same run reported a FreeRTOS stack overflow in task `main` at the start of multipart
upload. The production controller was still using the configured 3,584-byte
ESP-IDF main-task stack. It now runs in a dedicated 16 KiB `doorbell_ctrl` task,
with the controller object and large audio buffers retained in static storage;
stack high-water telemetry is logged immediately before and after upload. Both
follow-up changes pass the ESP-IDF 6.0.1 production build and require a flashed
board retest.

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

Build the isolated diagnostic with the production application disabled:

```bash
source /home/ethioking/.espressif/v6.0.1/esp-idf/export.sh
idf.py -B build-mic -D SDKCONFIG=sdkconfig.mic \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.mic.defaults' build
```

For the current Rev C battery-start investigation, disconnect J1 and all
external peripherals before flashing or running this image. Power only from a
fully inserted USB-C connector. The expected output is changing `left`
statistics with `status=ACTIVE` and an all-zero unused `right` slot. Speak and
tap near the board's MK1 acoustic bore while watching `range` and `avg_abs`;
both should rise substantially over the quiet baseline. This functional check
does not clear MK1 or the 3.3 V power path for battery operation.

The diagnostic first holds WS and SCK low for ten seconds before starting I2S.
During this standby observation, touch-check MK1 and listen for whine; remove
USB immediately for warming, whine, resets, or odour. Continue the acoustic
test only if the board remains quiet and MK1 stays at ambient temperature.

On 2026-08-06, the current Rev C article completed this USB-only diagnostic.
The ten-second clock-low standby phase completed without a reset or USB fault.
After I2S started, the expected left slot remained continuously `ACTIVE` and
the unused right slot remained exactly `ALL_ZERO`. Quiet intervals fell to
approximately `2,000-8,000` average absolute magnitude with sample ranges near
`14,000-31,000`; speaking/tapping produced an average absolute magnitude up to
`280,612` and a sample range up to `1,117,054`. The roughly one-minute run had
no I2S errors, resets, or USB instability. This proves that MK1 still has a
working USB-powered electrical path and strong acoustic response. Operator
confirmation of temperature and audible whine during the run must be recorded
separately. The board was then flashed back to the core-only safe image, which
holds WS, SCK, and speaker data low and microphone data as an input with a
pulldown. This result does not clear battery operation or validate the hidden
pin-3 ground joint.

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

On 2026-08-05, J6 was reconnected alongside the camera, permanent button, and
covered PIR while running the USB-powered production image. With the battery
absent, the speaker remained silent and U4, MK1, U2, U8/L1, and the speaker all
remained cool during the integrated idle/deep-sleep check. This validates the
production safe state with the speaker physically fitted; production playback
is still not implemented.

A subsequent USB-powered production button event also completed normally with
the speaker connected: the early ring acknowledgement, camera capture, upload,
Doorlink path, and notification worked, while J6 remained silent as intended.
This passes integration of every fitted external peripheral except battery
power; it does not imply production audio playback.

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
transitions during repeated manual presses. The permanent four-pin JST/button
harness used for the 2026-08-05 wake test retained this same mapping. The
button's unused red NC lead remains isolated.

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
