# Smart Doorbell Rev B Enclosure Handoff

This document condenses the enclosure/mechanical decisions for the wireless
battery-powered doorbell build. Use it as the starting context before changing
the KiCad Rev B PCB outline, mounting holes, or component placement.

## Target Product Shape

- Apartment-door-mounted, Ring-style doorbell.
- Mounted outside an apartment door using renter-friendly Command strips.
- Corridor is well lit and has decent ventilation.
- Interface is vertical along the longest enclosure dimension.
- Front interface layout:
  - Camera in the upper half, centered.
  - Status LED/light pipe near the camera.
  - Optional PIR below/near the status LED.
  - Main illuminated button near the bottom, centered.
  - Charging LED near the bottom.
  - Speaker, mic, and USB-C cutouts around sides and bottom.

## Enclosure Decision

Use a black, solid-cover `DS-AG-0813`-style ABS enclosure:

- Selected outside size: `80 x 130 x 70 mm` (`W x H x D`), with no external
  mounting ears.
- The catalogued enclosure uses a `50 mm` deep base and `20 mm` lid. Mount the
  PCB to the deep base/back side; use the lid as the hallway-facing front panel
  for the button, camera, PIR, and light-pipe openings.
- This is a compact, vertical doorbell form factor. It is a better visual fit
  than the previously considered `158 x 90 x 60 mm` and `115 x 90 x 55 mm`
  project boxes while retaining useful depth for the button and battery.
- Do not design the PCB around the enclosure's lid-screw pattern or molded
  bosses. Use custom standoffs fixed to the deep base instead.

Why this enclosure:

- The old `100 x 68 x 50 mm` box was too tight.
- The 22 mm button plus battery plus PCB stack caused the lid not to close.
- User dry-fit showed that even with the PCB basically sitting on the 10 mm
  battery, the `100 x 68 x 50 mm` enclosure could not fully close.
- The `80 x 130 x 70 mm` enclosure keeps a much more doorbell-like front
  profile and has more total depth than the old box.
- The present PCB outline is shaped for rounded corners: it is wide through
  its middle and narrow at its USB-C end. Preserve that advantage for the
  first fit instead of converting it to a plain rectangle.

Depth math:

- Old `100 x 68 x 50 mm` box measured roughly:
  - deep base internal depth: `33 mm`
  - lid internal depth: `12 mm`
  - total usable internal depth: about `45 mm`
- The selected enclosure has a catalogued `50 mm` base plus a `20 mm` lid.
- Button internal protrusion when mounted correctly through the front lid is
  about `31 mm`.
- PCB stack on short standoffs:
  - `4 mm` standoff + `1.6 mm` PCB + `5 mm` tallest JST = `10.6 mm`
  - `6 mm` standoff + `1.6 mm` PCB + `5 mm` tallest JST = `12.6 mm`
- The available depth works only when the battery uses the side-wall/depth
  volume described below rather than stacking behind the button or PCB.

Catalog fit evidence for the standard `DS-AG-0813`:

- Reference: [Switch Box mechanical document](https://images.100y.com.tw/pdf_file/DMCA.pdf),
  pages 5-6. Treat this as the geometry reference for the standard mould; the
  exact marketplace enclosure may be a compatible clone and still needs
  measurement.
- Published outside dimensions: `130 x 80 x 70 mm` (`H x W x D`).
- Published dimension table: `H1/H2/H3/H4 = 130/124/112/100 mm`,
  `W1/W2/W3 = 80/74/62 mm`, and `D1/D2 = 50/20 mm`; stated tolerance is
  `+/-1 mm`.
- The matching internal mounting plate is `116 x 68 x 1.6 mm`.
- The current roughly `59 x 91 mm` board fits within that mounting-plane
  envelope with about `9 mm` total width margin and `25 mm` total height
  margin. This supports the current outline, but the actual purchased case
  still requires a physical fit check for ribs, bosses, and molding variation.

## Current PCB Context

Current KiCad source of truth:

- PCB file: `pcb/smart-doorbell/smart-doorbell.kicad_pcb`
- Current outline bounding box is roughly `59 x 91 mm`:
  - x range: `107.25..166.25`
  - y range: `55.95..146.95`
- Current mounting holes are M2.5:
  - footprint: `MountingHole_2.7mm_M2.5`
  - positions: `(111.05, 71.425)`, `(162.45, 71.425)`,
    `(111.05, 122.825)`, `(162.45, 122.825)`
  - current pattern: `51.4 x 51.4 mm`
- Current mechanically relevant footprints:
  - `MK1` ICS-43434 mic at `(110.95, 89.442)`
  - `J6` speaker JST-PH 2-pin at `(115.55, 98)`
  - `J7` PIR JST-PH 3-pin at `(155.25, 86.45)`
  - `J8` button/LED JST-PH 4-pin at `(133.65, 122.1)`
  - `J1` battery JST-PH 2-pin at `(158.442, 114.371)`
  - `J2` USB-C receptacle at `(136.85, 143.45)`

Do not stretch or scale the PCB to the enclosure. The existing outline is
already within the documented mounting-plane envelope. The USB-C end tapers to
about `41 mm`, which is useful around the enclosure's rounded end corners.

## Camera Module Target and FPC Contingency

The selected camera is the `DCXYX-LZTKQJ-5M-357-V1 FF`: the 120-degree
distortion-free, fixed-focus OV5640 24-pin DVP module. Fixed focus is the
intended production choice; autofocus is not a requirement for this doorbell.

The first 22 connector positions use the eight-bit DVP/SCCB/power arrangement
documented in the KiCad source. The seller drawing now recorded in
`docs/hardware/camera/README.md` identifies position 23 as `AF_GND` and position
24 as `AFVDD`. Rev B keeps both contacts isolated by default, with test pads and
normally-unpopulated configurable links:

- `J3.23` -> test pad -> `0R`, DNP -> GND (`AF_GND` contingency).
- `J3.24` -> test pad -> `0R`, DNP -> `+2V8` (`AFVDD` contingency).

For the selected fixed-focus camera, both `0R` resistors remain unpopulated and
neither pin connects to power, ground, GPIO, or pull resistors. Populate them
only after a specific autofocus module's pinout confirms pin 23 is `AF_GND`
and pin 24 is `AFVDD`. This is a future AF contingency, not a requirement for
the fixed-focus build.

Before powering the camera, inspect the received module and verify its pin-1
and exposed-contact orientation against the seller drawing. R11 and R21 remain
DNP for the selected fixed-focus camera.

## Recommended PCB Mechanical Direction

Keep the PCB at its current envelope for the first enclosure fit:

- Width target: current `58-59 mm` maximum; do not grow it.
- Height target: current `91 mm`; do not grow it.
- Preserve the tapered/scalloped edge treatment, especially around the USB-C
  end. Do not replace it with a full rectangle before the physical fit test.
- Do not chase the enclosure boss pattern.
- Use custom internal standoffs attached to the enclosure floor.
- Prefer the current M2.5 mounting direction:
  - PCB holes: `2.7-2.8 mm` NPTH.
  - Hardware: M2.5 machine screws, washers, and short threaded standoffs.

Mounting concept:

- Attach custom standoffs to the ABS floor using epoxy/plastic bonder.
- Do not epoxy the PCB itself.
- Do not use RTV silicone as the primary structural mount; it is too flexible.
- Neutral-cure silicone/E6000 is fine for strain relief and non-structural
  retention.
- Avoid acetic/vinegar-cure silicone near electronics.

Standoff process:

1. Screw standoffs loosely to the PCB.
2. Put a small amount of plastic-compatible epoxy on the bottom of each
   standoff.
3. Place the PCB into the enclosure at the exact final position.
4. Let cure with the PCB acting as the alignment jig.
5. Remove the screws and lift the PCB out.
6. Add a small epoxy fillet around each standoff base if needed.
7. Reinstall the PCB with nylon washers.

Standoff height:

- `4 mm` is the compact target and may work because the silicone mic tube can
  compress.
- `6 mm` is mechanically safer, especially around the mic tube.
- The selected `70 mm` enclosure should tolerate either `4 mm` or `6 mm` when
  the battery uses the right-side L-shaped placement and the button keepout is
  kept clear.

Consider adding an extra support near the upper-left/mic area if the tube
preload causes board flex.

## Volume Allocation Inside Box

Do not stack everything in one depth column. Separate the mechanical volumes:

- Button barrel: front/lid area, lower center, must have a clear rear keepout.
- PCB: mounted flat to the back floor of the deep base on short standoffs.
- Battery: mounted to the right interior side wall in the forward portion of
  the deep base, not under the PCB and not behind the button.
- Speaker: wired/movable, mounted to bottom or side grille.
- Mic tube: short duct near the left side and `MK1`.
- Wiring gutters: use side/corner space.

The battery and PCB form an L-shaped volume: the PCB occupies the back plane;
the LiPo lies vertically on the right wall, with its `34 mm` dimension running
front-to-back. Keep the battery entirely on the deep-base side of the
lid/base seam. Never bond it to the lid or bridge the seam, because opening the
lid would flex the pouch and its leads.

## Battery

Existing battery:

- Type: `103450` LiPo.
- Dimensions: `50 x 34 x 10 mm`.
- Connector: JST-PH 2.0.
- Nominal voltage: `3.7 V`.
- Advertised capacity: `2000 mAh`.

Nothing is wrong with this battery. `103450` means approximately:

- `10 mm` thick
- `34 mm` wide
- `50 mm` long

Use it as the baseline/prototype battery. Capacity may be optimistic; treat it
as roughly `1500-2000 mAh` until measured.

Recommended placement:

- Mount battery to the right interior wall of the `80 x 130 x 70 mm` deep
  base. The right side keeps the left side free for the short `MK1` mic duct
  and is close to battery connector `J1`.
- Orientation:
  - `50 mm` dimension vertical.
  - `34 mm` dimension front-to-back.
  - `10 mm` thickness protruding inward from the side wall.
- Place it in the forward part of the deep base, away from the lower-centre
  button rear keepout and clear of the PCB's tallest components.

"Battery bay" means a reserved internal keepout/placement zone, not a separate
plastic part. The battery can sit against the ABS side wall, retained by
foam tape, adhesive Velcro, or a light strap.

Battery safety/mechanical notes:

- Do not clamp or compress the pouch.
- Use fishpaper/Kapton/plastic insulation between the LiPo and any PCB
  solder joints or hard edges.
- Do not rely on loose tape permanently.
- Bigger battery is okay only if thickness stays near `10 mm`; larger
  footprint is less risky than greater thickness.

## Button

Current button:

- 22 mm illuminated metal momentary button.
- White LED.
- LED voltage listing: `3-6 V` / `5 V`.
- Rear depth from listing: about `33 mm`.
- When mounted correctly through the front lid with face/gasket outside the
  enclosure, internal protrusion is about `31 mm`.
- Blue rear harness/socket makes total depth too long and should be removed.

Keep the current 22 mm button. The button is not the main problem if the
battery is side-mounted and the PCB does not stack behind it.

Button wiring:

- Use JST-PH 2.0 4-pin 26 AWG pigtail if it matches PCB connector `J8`.
- Solder pigtail wires directly to the button tabs.
- Use heat shrink over each soldered terminal.
- Do not friction-fit bare wires.
- Do not use the original 18 AWG harness wires unless necessary; they are
  mechanically bulky.

Button pinout:

- Red: NC, leave unused and insulated.
- Blue: NO, connect to doorbell signal.
- Black: C/common, connect to button common/GND side.
- Yellow: LED positive.
- Green: LED negative.

PCB `J8` pin mapping:

- `J8 pin 1`: `/DOORBELL_IN` -> button NO/blue.
- `J8 pin 2`: `GND` -> button C/black.
- `J8 pin 3`: button LED positive path via `R9=330` to `/BTN_LED` -> yellow.
- `J8 pin 4`: `GND` -> LED negative/green.

Verify JST pigtail wire order by continuity. Do not assume AliExpress color
order matches the PCB pin order.

## Microphone

Current mic:

- `MK1`: ICS-43434 I2S MEMS microphone.
- It is a bottom-port mic.
- The KiCad footprint includes a small non-plated acoustic hole through the
  PCB under the mic.
- Sound enters from the underside of the PCB, not from the top of the mic
  package.

Existing material:

- Silicone tubing: `2 mm ID x 4 mm OD`, 1 meter.

Accepted mechanical concept:

- Use the silicone tube as a side-tapped acoustic duct/manifold under the PCB.
- The tube should be mic-only. Do not share the speaker grille.
- Because `MK1` is already near the upper-left side of the PCB, the duct can be
  short.

Recommended mic duct:

```text
left side wall mic hole
-> open tube end against side-wall mic hole
-> short tube run under PCB near MK1
-> small upward-facing window/slit in tube wall aligned to PCB mic acoustic hole
-> tube continues 5-10 mm past MK1
-> far end sealed
```

Details:

- Exterior mic hole: about `1.0-1.5 mm`.
- Tube window under mic: start about `1 mm`, enlarge only if needed.
- Align the tube window to the PCB acoustic hole, not just the `MK1` package
  outline.
- Use tape only for dry fit.
- Final sealing/retention: neutral-cure silicone, E6000, or similar flexible
  adhesive.
- Do not let adhesive block the PCB acoustic hole or tube bore.
- Add PCB keepout around the under-board tube path.
- If using `4 mm` standoffs, the tube will be lightly compressed. This is
  acceptable if the PCB does not bow.
- `6 mm` standoffs give more comfortable clearance.

Avoid:

- Long curved mic tube paths.
- Routing mic to the speaker grille.
- Leaving the mic to listen only to the sealed enclosure interior.
- Moving the mic off-board unless the PCB-mounted solution fails.

## Speaker

Existing speaker:

- 8 ohm, 1 W, small oval speaker.
- Size: `24 x 15 mm`.
- Wired/movable, so it can be placed independently of the PCB.

Recommended placement:

- Mount to bottom or side grille.
- Keep away from mic grille/tube.
- Use foam/EVA gasket or adhesive speaker gasket to couple speaker to holes.
- Drill a grille pattern using small bits.

Electrical caution:

- Speaker connector `J6` is driven by the MAX98357A bridged output.
- Do not connect either speaker lead to ground.

## Light Pipe And LEDs

Existing light-pipe parts:

- LPA3.5 light guide column.
- Purchased size: `LPA3.5-21.1 mm`.
- Ten clear acrylic rods, `3 mm x 100 mm`.
- Ten black `3 mm` panel-mount clip LED bezels.

Likely issue:

- `21.1 mm` is a test/spare part, not a presumed final fit. With the PCB
  mounted to the rear/deep base on 6 mm standoffs, the likely LED-to-inner-lid
  distance is roughly `55-60 mm`; confirm this only with the actual enclosure
  and installed PCB.

Final light-pipe construction:

- Use the `3 mm` acrylic rods with the matching `3 mm` black panel-mount
  bezels. The bezel is the intentional, finished front-panel interface; do
  not glue a short LPA3.5 part to a rod.
- The bezel's nominal `3 mm` describes the LED/rod it accepts, not necessarily
  its required panel-hole diameter. Obtain the seller's dimension drawing or
  test one in scrap ABS before drilling the enclosure. Confirm its clip tabs
  work with the actual enclosure wall thickness.
- Cut the finished pipes slightly long and trim in small steps. Use a fine
  hobby/razor saw, then sand the end flat (`400 -> 800 -> 1200` grit). An
  X-Acto knife may score the material but is not the preferred cutter for a
  clean, square optical end.
- Leave about `0.25-0.5 mm` axial clearance rather than preloading the PCB LED
  or front lid.
- Insert the rod from the enclosure interior and retain its rear with a small
  black sleeve/stop plus a tiny neutral-cure silicone dab. Keep adhesive away
  from the visible optical end; do not use Loctite 404 for an optical joint.
- Keep the light pipe path vertical/straight where possible.

## PIR Sensor

Existing PIR:

- AM312 module.
- Dome/head diameter: about `13 mm`.
- Approximate depth from head to end of 3 pins: `25 mm`.

Recommendation:

- Treat PIR as optional/future.
- Do not rigidly plug the AM312 into the PCB if that creates mechanical stress.
- Prefer wiring it to `J7` with a short pigtail and mounting it to the front
  lid.
- Reserve front-panel keepout now if PIR is desired later.

PCB `J7` pin mapping:

- `J7 pin 1`: `+3V3`
- `J7 pin 2`: `PIR_SENSOR_OUT`
- `J7 pin 3`: `GND`

The default Rev B path is J7.2 `PIR_SENSOR_OUT` through populated R30 (1 kohm)
to `PIR_WAKE` on GPIO3. R29 (100 kohm) is the pulldown. GPIO42 is only the DNP
fallback through R31 and must not be treated as the default PIR input.

## USB-C

- USB-C connector is on bottom edge (`J2`).
- Keep bottom enclosure cutout aligned with USB-C.
- Do not place a standoff/boss directly below the USB-C port.
- This is one reason not to use the enclosure's bottom molded boss spacing for
  PCB mounting.

## Holes And Tools

Existing tools/materials:

- 4-32 mm HSS titanium-coated step drill bit for larger ABS holes.
- Smaller Ryobi drill bits for small IO/interface holes.
- Sandpaper for ABS rough edges.
- Heat shrink tubing.
- JST-PH 2.0 pigtails, including 4-pin 26 AWG.

Use:

- Step bit for button and larger clean ABS openings.
- Small bits for speaker grille, mic hole, charging/status LED hole, and pilot
  holes.
- Sandpaper/deburring after drilling.
- Heat shrink on soldered button terminals.

## Hardware Shopping List

Core mechanical:

- Black `DS-AG-0813`-style `80 x 130 x 70 mm` solid-cover ABS enclosure;
  preferably buy a spare.
- M2.5 threaded standoffs, `4 mm` and/or `6 mm` length, OD ideally `<=5 mm`.
- M2.5 machine screw assortment matched to standoff length.
- M2.5 nylon washers.
- Plastic-compatible epoxy or JB Weld Plastic Bonder.
- Optional E6000 or neutral-cure silicone for strain relief and acoustic seals.
- Fishpaper/Kapton/plastic insulation sheet.
- Thin foam tape, adhesive Velcro, or light battery-retention strap.
- Foam/EVA speaker gasket material.
- Optional acoustic vent membrane stickers for mic/speaker dust protection.

Optional replacements/upgrades:

- Bigger LiPo only if thickness remains around `10 mm`; avoid thick packs.

## KiCad Modification Approach

Before editing:

1. Start from a clean checkpoint or commit.
2. Keep `pcb/smart-doorbell/` as source of truth for routed nets.
3. Use current project guide pin mappings:
   - `AMP_EN`: GPIO44
   - `PIR_WAKE`: GPIO3 through populated R30
   - GPIO42: DNP PIR fallback through R31 / AUX contingency
   - USB native D-/D+: GPIO19/GPIO20 only

Recommended KiCad process:

1. Add mechanical reference geometry on a non-fabrication drawing layer:
   - `DS-AG-0813` enclosure mounting plane / inner envelope
   - front/lid plane
   - button rear keepout cylinder
   - side battery bay
   - mic tube path
   - speaker grille region
   - USB cutout region
2. Keep critical electrical layout stable at first:
   - ESP32-S3 module
   - camera FPC connector
   - USB-C connector
   - mic and local support parts
   - power/audio support passives
3. Keep the present outline for the first dry fit. Adjust it only when a
   measured enclosure interference requires a change.
4. Place mounting holes for custom standoffs, not molded enclosure bosses.
5. Preserve or improve support around the mic/tube area.
6. Resolve any routing damage after the mechanical geometry is set.
7. Re-run DRC/ERC.
8. Export a 1:1 paper/cardboard outline and physically test it inside the real
   enclosure with button, battery, speaker, and mic tube.

## Open Items For Next Session

- Measure the received black `80 x 130 x 70 mm` enclosure and confirm its
  internal ribs/bosses against the documented `DS-AG-0813` geometry (which
  permits `+/-1 mm`).
- Dry-fit the unmodified PCB, including its USB-C connector, before changing
  the outline or ordering another board.
- Confirm the final PCB position and USB-C bottom cutout.
- Decide final standoff height: `4 mm` compact vs `6 mm` safer.
- Dry-fit the right-wall L-shaped battery placement, the lower-centre button,
  speaker, and left-side mic tube together.
- Confirm whether to add a fifth support point near `MK1`.
- Treat the stock enclosure's ingress rating as void after drilling camera,
  button, USB-C, microphone, and speaker openings unless those openings receive
  appropriate gaskets or acoustic membranes.
