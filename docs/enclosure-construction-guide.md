# Doorlink Enclosure Construction Guide

This is the build-facing guide for fitting the assembled Rev C doorbell into
the black `DS-AG-0813`-style `80 x 130 x 70 mm` ABS enclosure. Read the
[Rev B enclosure handoff](enclosure-rev-b-handoff.md) first for the PCB,
battery, standoff, camera, button, microphone, and internal-volume decisions
that led to this construction plan.

Do not treat marketplace dimensions as machining dimensions. Measure the
received enclosure, speaker, metal speaker cover, bezels, camera, and button
with calipers before marking the lid. Buy or retain a spare lid/enclosure and
make every new type of opening in scrap ABS first.

## Current Parts And Build Intent

- Enclosure: black solid-cover `80 x 130 x 70 mm` ABS,
  `DS-AG-0813`-style.
- PCB: routed Rev C board, approximately `59 x 91 mm`, mounted to the deep
  base on `10 mm` M2.5 standoffs.
- Battery: `103450` LiPo, approximately `50 x 34 x 10 mm`, mounted on the
  right interior wall without compression.
- Camera: fixed-focus, 120-degree OV5640 module on the front lid.
- Button: 22 mm illuminated metal momentary button on the lower front lid.
- Speaker: one speaker from the five-piece pack described by the seller as
  `4 ohm`, `3 W`, with a `2.0` terminal. The received connector and all
  physical dimensions still need to be verified; do not assume the seller's
  `2.0` description guarantees correct JST-PH latch, housing, pin order, or
  polarity for PCB connector J6.
- Speaker cover: outdoor metal grille/cover installed inside the lid, between
  the ABS opening and the speaker.
- Microphone: PCB-mounted ICS-43434 coupled to its own small exterior port by
  the documented silicone tube. It does not share the speaker opening.
- PIR: optional and disabled in the normal production firmware. Do not cut a
  PIR opening unless it will actually be installed.

Only one speaker is installed. The other four units are spares and useful for
bench comparison; they are not connected in parallel or series.

## Speaker Electrical Limit

The `4 ohm`, `3 W` marking is the speaker's nominal impedance and power
handling, not a promise of 3 W from this board. U4 is a MAX98357A powered from
the board's `/SYS` rail, and R32 straps its gain to 6 dB. Analog Devices rates
the amplifier for a 4-ohm load and advertises 3.2 W into 4 ohms specifically
at a 5 V supply. Doorlink's `/SYS` rail is not a dedicated fixed 5 V speaker
supply, so available unclipped output will vary with USB/battery state and be
lower than that headline condition.

This is still the appropriate higher-output speaker class to test with the
present board. Its larger radiating area and the enclosure coupling may sound
fuller than the old `24 x 15 mm`, `8 ohm`, `1 W` unit, but that remains a test
expectation rather than a validated result. Do not change R32 or add speakers
to chase more loudness. Set the practical maximum with digital level, clipping,
sound quality, current draw, and temperature tests.

J6 is a bridged Class-D output:

- J6.1 is `OUTP` and J6.2 is `OUTN`.
- Connect the speaker only across J6.1 and J6.2.
- Never connect either speaker lead, terminal, or metal grille to ground.
- Insulate both speaker terminals and keep the metal grille mechanically and
  electrically clear of them.

MAX98357A reference: [Analog Devices product page and data sheet](https://www.analog.com/en/products/max98357a.html).

## Front-Panel Arrangement

Use the existing vertical layout as the starting point:

1. Camera centered in the upper half.
2. Status light pipe near the camera.
3. Optional PIR only if the feature is deliberately fitted.
4. Illuminated 22 mm button centered toward the bottom.
5. Charging/status light pipe where it remains visible.

Put the speaker opening on a lower side or bottom face if the actual parts and
wiring permit it. A downward-facing opening sheds rain better and reduces the
chance of direct water entry. If the speaker must face forward, place it where
the metal cover has a complete perimeter overlap and where a hydrophobic
acoustic membrane can be added without fouling the button or camera.

Keep the microphone opening on the opposite side of the enclosure when
possible. It must use its own `1.0-1.5 mm` port and short tube; sharing the
speaker cavity/opening worsens feedback and speaker-to-mic leakage.

## Speaker Stack

The intended stack, viewed from the hallway toward the interior, is:

```text
hallway
  -> ABS lid with an opening
  -> metal speaker cover/grille, overlapping the opening from inside
  -> optional hydrophobic acoustic membrane
  -> closed-cell foam/EVA perimeter gasket
  -> speaker face
  -> speaker body and insulated terminals inside the enclosure
```

This implements the requested `speaker -> metal cover -> ABS lid` order when
read from the enclosure interior outward. The metal cover is impact and debris
protection; it is not automatically a water seal.

Before cutting, measure and record:

- speaker body width, height, depth, and terminal projection;
- the sound-producing diaphragm/aperture, excluding its frame;
- metal cover outside dimensions, open area, thickness, mounting holes, and
  usable bonding flange;
- lid wall thickness and curvature at the proposed location; and
- total speaker-stack depth plus wire bend radius.

Make the ABS opening smaller than the metal cover so the cover overlaps solid
plastic on every side. Keep the opening and gasket clear of the moving
diaphragm. Do not make the lid opening larger than the grille's perforated area
or so small that the ABS masks a meaningful portion of the speaker aperture.
Use the actual parts to establish the line; no reliable cutout dimension can
be inferred from the seller photo.

Prefer screws and a removable clamp/bracket if the cover provides mounting
holes. Otherwise bond only the cover's perimeter to scuffed, cleaned ABS with
a plastic-compatible adhesive. Use a continuous thin gasket between speaker
frame and cover so front sound cannot leak around the frame and cancel with
rear sound. Do not put adhesive on the diaphragm, suspension, acoustic holes,
or speaker vent. Add small compliant pads or thread locking appropriate to the
fastener so the metal cover cannot buzz.

If outdoor exposure is credible, add a purpose-made hydrophobic acoustic vent
membrane behind the metal cover. A metal grille alone does not make the speaker
waterproof. Treat the enclosure's original ingress rating as void after the
first hole is made and validate the finished assembly accordingly.

## Best Tools For The Openings

A Dremel 4000 is a capable and versatile choice, especially for the speaker
opening and USB-C slot, but it is not the best single tool for every hole. The
most controlled workflow uses a drill/step bit for circles, a guided rotary
cutter for non-round openings, and hand files for the final fit.

Recommended kit:

- variable-speed drill or drill press;
- sharp `4-32 mm` step drill for the 22 mm button and other larger round holes;
- sharp small twist bits for pilots, the microphone port, and corner relief;
- Dremel 4000 with a Dremel 561 or 561HP multipurpose spiral cutting bit;
- Dremel 565 multipurpose cutting guide to control depth and keep the tool
  supported on the panel;
- flat, half-round, and needle files;
- deburring tool or a sharp hand scraper;
- `220`, `400`, and finer abrasive paper for non-optical edges;
- painter's tape, calipers, square, center punch/awl, clamps, and sacrificial
  plywood backing; and
- eye protection and dust collection/ventilation.

Dremel specifies its 561/561HP bit for plastic, recommends the 565 guide, and
lists `15,000-20,000 RPM` for plastic. Start at the low end on scrap, take light
passes, and pause if the ABS smears or melts instead of forming chips. The
Dremel 4000's `5,000-35,000 RPM` range is more than adequate. A less expensive
variable-speed rotary tool can do this one enclosure, but the 4000 is a good
purchase if it will be used for later fitting, deburring, and revisions.

Manufacturer references:

- [Dremel 4000 specifications](https://www.dremel.com/us/en/p/4000-2-32-f0134000fb)
- [Dremel 561 multipurpose cutting bit](https://www.dremel.com/us/en/p/561-26150561ac)
- [Dremel 565 multipurpose cutting guide](https://www.dremel.com/us/en/p/565-26150565ac)

Do not freehand a visible opening to its final line with a cutoff wheel. Rotary
tools wander, and melted ABS produces a raised, uneven edge. Cut inside the
waste line and file to the finished dimension.

## Opening Method By Feature

| Feature | Preferred roughing method | Finish and fit requirement |
| --- | --- | --- |
| 22 mm button | Pilot hole, then step drill from the visible face with the lid backed by plywood | Stop at 22 mm, deburr by hand, and enlarge only enough for a slip fit; retain the button's panel gasket |
| Camera | Step drill for a circular opening; files/reamer for final clearance | Live-test the 120-degree field of view before final sizing; the lid or window must not vignette the corners |
| Speaker | Drill relief holes inside the marked waste, then use the guided 561 bit | File to a smooth rounded opening hidden beneath the grille overlap; preserve a continuous sealing land |
| USB-C | Small corner holes, guided 561 cut between them | Needle-file to a cable-tested rounded rectangle and provide a bottom-facing plug/cover or gasket |
| Microphone | Sharp `1.0-1.5 mm` twist bit | Deburr without enlarging; align to the silicone duct and cover with an acoustic membrane if used outdoors |
| Light-pipe bezel | Pilot and step/twist drill to the measured bezel requirement | Test the actual clip in scrap ABS; `3 mm` is the rod size and may not be the panel-hole size |
| Optional PIR | Step drill only after the actual dome and retaining method are measured | Keep plastic/cover outside the sensor's field and seal its perimeter |

For the wide-angle camera, place the lens as close to its protected exterior
window/opening as safely possible and use an outward bevel if the live preview
shows vignetting. A UV-stable optical window and perimeter gasket are preferable
to leaving the lens exposed. Test the chosen window for reflections from the
status and button LEDs in darkness before bonding it.

## Marking And Cutting Procedure

1. Remove the lid from the enclosure. Remove every electronic part, the LiPo,
   gasket, and loose metal item from the work area. Never machine the enclosure
   with the battery or PCB installed.
2. Measure the actual enclosure interior, lid thickness, seam, ribs, screw
   columns, and curved regions. Mark a no-cut perimeter around the lid seal and
   screw columns.
3. Dry-fit the PCB on loose standoffs, right-wall battery, button barrel,
   camera, speaker stack, mic tube, light pipes, and all cable bends. Confirm
   that the lid closes without pressure on the LiPo or camera flex.
4. Cover both faces of the cutting region with painter's tape. Establish one
   centerline and fixed datum edges; transfer a full-scale paper template to
   the visible face. Check it against the real parts from both sides.
5. Make the entire pattern in scrap ABS or the spare lid. Fit the parts and
   correct the template before touching the final lid.
6. Clamp the final lid gently to flat sacrificial plywood with the visible face
   upward. Do not distort the lid or clamp across its sealing lip.
7. Center-punch lightly. Drill small pilots first. Let sharp bits cut without
   heavy pressure; clear chips frequently.
8. Make circular holes with the step drill. Approach the final step slowly and
   stop before the next diameter enters the panel.
9. For the speaker and USB-C openings, drill relief/corner holes inside the
   waste, use the 565 guide and 561 bit to remain inside the line, then finish
   to the line with files. Keep the tool moving and allow the ABS to cool.
10. Remove tape, scrape/deburr both faces by hand, and wash away all plastic
    chips. Do not use a flame to polish ABS edges.
11. Trial-fit every interface without adhesive. A part should seat without
    forcing or bending the panel. Confirm button travel, cable insertion,
    camera view, grille overlap, and speaker diaphragm clearance.

## Internal Assembly Order

1. Finish all machining and clean the enclosure completely.
2. Install the camera window/gasket, button gasket and nut, light-pipe bezels,
   speaker cover, acoustic membranes, and USB-C protection.
3. Mount the speaker behind its grille with a closed-cell perimeter gasket.
   Insulate the terminals and provide flexible strain relief to its two-wire
   lead.
4. Fit the mic tube to the PCB acoustic hole and its separate exterior port.
   Seal the far tube end as described in the handoff.
5. Bond aligned `10 mm` M2.5 standoffs to the deep base using the PCB only as
   a removable alignment jig. Do not epoxy the PCB.
6. Install the PCB with nylon washers. Keep the USB-C connector aligned and
   leave enough service loop to open the enclosure without pulling a cable.
7. Add fishpaper/Kapton between the PCB and LiPo zone. Retain the battery on
   the right interior wall with a non-compressing removable strap, Velcro, or
   appropriate foam tape. Never bridge the lid/base seam with the battery.
8. Route the speaker wires away from the camera flex and antenna, and route the
   mic duct away from the speaker. Prevent every wire from reaching the button
   terminals, screw threads, or lid seal.
9. Complete the unpowered continuity checks before connecting the battery.
10. Close the enclosure gradually while watching all internal parts. Stop if
    the lid applies pressure to the battery, PCB, camera flex, speaker, or
    button terminals.

Use neutral-cure electronics-safe silicone for flexible seals and strain
relief. Avoid vinegar-smelling acetic-cure silicone near electronics. Use a
plastic-compatible structural bonder for standoffs and grille mounts when
mechanical fasteners are unavailable. Keep all adhesive out of optical and
acoustic paths.

## Acceptance Checks Before Outdoor Use

- Verify the J6 plug housing and pin positions by inspection and continuity;
  neither speaker lead may have continuity to ground or the metal cover.
- Begin speaker playback at a low digital level. Increase in small steps using
  representative chimes and speech while listening for clipping, buzzing,
  grille rattle, and air leaks. Check U4, the speaker, and the power path for
  abnormal heating, and repeat on battery and USB power.
- Record and listen to visitor speech through the completed microphone duct
  with the speaker silent. Then test the half-duplex interaction for acoustic
  leakage; do not infer full-duplex capability.
- View a live camera frame in bright and dark conditions. Check all four
  corners for vignetting and check for LED reflections in the camera window.
- Verify button travel and ring-LED visibility without flexing the lid.
- Insert the real USB-C cable and confirm the plug shell does not lever against
  the ABS opening.
- With electronics and battery removed, use dry tissue inside the closed
  enclosure for a gentle rain-direction test. Do not submerge it or use a
  pressure jet. Correct leaks and let the enclosure dry completely before
  reinstalling electronics.
- Confirm the lid gasket is continuous, the enclosure closes without force,
  and mounting strips or brackets do not cover drainage paths. Add a secondary
  safety tether if a fall could injure someone or damage the LiPo.

Record final cutout dimensions, part positions, photographs, adhesive/gasket
materials, and the maximum validated playback setting in this file or a linked
as-built record. Until those checks pass, the enclosure and improved speaker
remain a construction plan, not an outdoor- or loudness-validated assembly.
