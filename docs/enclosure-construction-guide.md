# Doorlink Indoor Enclosure Construction Guide

This is the build-facing guide for fitting the assembled Rev C doorbell into
the black `DS-AG-0813`-style `80 x 130 x 70 mm` ABS enclosure. The finished
unit will be mounted to an apartment door in a well-lit, ventilated indoor
corridor. It is not intended for rain, direct weather, or outdoor service.

Read the [Rev B enclosure handoff](enclosure-rev-b-handoff.md) for the PCB,
battery, camera, button, microphone, and internal-volume decisions behind this
construction plan. Treat the measured parts and the physical Rev C PCB as the
machining source of truth. Marketplace dimensions are planning dimensions,
not final cut dimensions.

The enclosure loses any original ingress rating after it is machined. That is
acceptable for this indoor installation; weatherproof acoustic membranes,
rain testing, and outdoor drainage features are not required. Dust control,
strain relief, electrical insulation, acoustic separation, and safe LiPo
retention still matter. The corridor being ventilated does not require extra
vent holes in the enclosure.

## Confirmed Mechanical Inputs

| Item | Construction dimension or intent |
| --- | --- |
| Enclosure | Black solid-cover `80 x 130 x 70 mm` ABS, `DS-AG-0813` style |
| Hallway-facing front face | Approximately `77 x 125 mm`, with four existing corner closure holes |
| PCB | Rev C, approximately `59 x 91 mm`, mounted in the deep base |
| PCB hole pattern | Four `2.7 mm` M2.5 mounting holes on a `51.4 x 51.4 mm` pattern |
| PCB supports | Four `10 mm` M2.5 female-to-female standoffs, preferably no more than `5 mm` OD |
| Battery | `103450` LiPo, approximately `50 x 34 x 10 mm`, retained on the right interior wall |
| Camera | Fixed-focus 120-degree OV5640; circular lens base is approximately `7 mm` diameter, while the square camera head is approximately `8.5 mm` wide |
| Button | Selected `22 mm` version: M22 x 1 threaded barrel, approximately `25 mm` face diameter, `15 mm` threaded length, and `33 mm` overall depth |
| Speaker | One `4 ohm`, `3 W` oval speaker, approximately `25 x 32 mm`; received connector has been verified as `2.0`; front-lid mounted below the button and rotated landscape |
| Speaker grille | Cut from one supplied `100 mm` circular metal mesh sheet using aviation snips |
| PIR | AM312-style module; dome is approximately `13 mm` at its widest and tapers toward approximately `8 mm` |
| Light pipes | `3 mm` clear acrylic rods with `3 mm` black plastic panel lamp bases |
| Microphone duct | `2 mm` ID x `4 mm` OD silicone tube from its own `1.0-1.5 mm` exterior port to PCB microphone `MK1` |

Measure the received enclosure wall thickness, corner-hole centers, internal
ribs, screw columns, and every actual component with calipers before marking.
Retain a spare enclosure or make every new opening first in comparable scrap
ABS.

## Parts And Fasteners

Have the complete mechanical set on hand before cutting:

- four `10 mm` M2.5 female-to-female standoffs, OD preferably `<=5 mm`;
- four M2.5 pan-head machine screws; start with M2.5 x 6 mm and keep M2.5 x 4
  or 5 mm alternatives in case the measured standoff thread is shallow;
- four M2.5 nylon washers under the PCB screw heads;
- plastic-compatible structural epoxy or plastic bonder for attaching the
  standoff bases to the enclosure;
- the 22 mm button with its supplied panel gasket, nut, and terminal wiring;
- two `3 mm` clear acrylic light-pipe rods and two black `3 mm` plastic lamp
  bases, one for each intended indicator;
- adhesive-backed hook-and-loop or similar removable adhesive fastener for
  the battery, plus fishpaper/Kapton or a thin battery carrier;
- closed-cell foam/EVA for the speaker perimeter gasket;
- the cut metal speaker mesh, with all cut strands and sharp edges contained;
- neutral-cure electronics-safe silicone for flexible retention, acoustic
  sealing, and strain relief;
- a fine-tip hot-glue gun and hot-melt glue sticks for sealing and retaining
  the microphone tube at the PCB; and
- heat-shrink tubing, cable ties or tie mounts, and the verified JST-PH
  pigtails needed for internal wiring.

Aim for approximately `3-5 mm` of screw engagement in each standoff. A screw
must not bottom out before clamping the washer and PCB, and it must not protrude
through a bonded standoff into the enclosure wall. Do not substitute enclosure
closure screws for PCB screws.

Do not use silicone as the primary structural adhesive for the standoffs. It
is intentionally flexible. Use a structural bonder verified for both the ABS
case and the standoff material; many adhesives do not bond reliably to nylon.
Use neutral-cure silicone for parts that benefit from compliance. Avoid
vinegar-smelling acetic-cure silicone around electronics.

## Panel Layout

Use the `77 x 125 mm` front face in portrait orientation. Preserve generous
no-cut areas around all four existing corner holes, their internal screw
columns, the lid lip, and any gasket. Measure those keepouts rather than
assuming the four holes form a perfect rectangle.

Use this mechanical priority order; appearance does not override it:

1. Position the PCB so the USB-C receptacle is square to the underside opening
   and approximately flush with, or slightly recessed behind, the exterior
   enclosure surface.
2. Confirm PCB wall, boss, button, battery, and cable clearances, then bond the
   four standoffs so that PCB position cannot change.
3. Project the centers of PCB LEDs D3 and D2 perpendicular to the front panel.
   These two points are the fixed axes of the rigid light pipes.
4. Arrange the camera, optional PIR, button, and front speaker around those
   fixed axes and the four enclosure closure-column keepouts.

The resulting visual order should be:

1. Camera centered high on the lid, but no higher than its 70 mm flex can reach
   without a tight bend.
2. Optional PIR dome fitted below the camera as a future contingency.
3. D3 lamp base at its mechanically projected position.
4. Illuminated 22 mm button raised into the middle/lower-middle region and
   adjusted around D3 if necessary.
5. Front-facing speaker immediately below the button, rotated to put its
   approximately `32 mm` dimension left-to-right and `25 mm` dimension
   top-to-bottom.
6. D2 lamp base at its mechanically projected position near the bottom.

The speaker is now deliberately front-facing and below the button. The
microphone remains separate: it uses a `1.0-1.5 mm` pinhole on the left side
wall, in the deep-base/rear portion and at the same height as PCB microphone
MK1. Its silicone duct runs through the 10 mm under-PCB space.

Make full-size front templates of the speaker's complete `32 x 25 mm`
landscape envelope, terminal projection, grille patch, and actual diaphragm
opening. The outer speaker envelope is a clearance template, not the ABS cut
line.

## Microphone And Front-Speaker Placement Map

Use these orientation names throughout the build:

- **rear**: the deep-base wall mounted against the apartment door;
- **front**: the removable `77 x 125 mm` hallway-facing lid;
- **left/right**: as viewed by a visitor facing the installed doorbell; and
- **underside**: the short downward-facing wall containing USB-C only.

```text
LEFT-SIDE SECTION — vertical centerline through the enclosure

door / rear                                             hallway / front
     |                                                        |
     v                                                        v
     +--------------------------------------------------------+
     | rear wall                                              | front lid
     |    10 mm standoffs                                     |
     |       PCB                                               |
     |       |                                                 |
     |       | MK1                                             |
     |       o<==== short 2 mm-ID silicone mic duct            |
     |  side mic port                                   [SPK]  | front lid
     |  1.0-1.5 mm                                      faces  | opening
     +------ USB-C opening ------------------------------front--+
                         UNDERSIDE
```

```text
FRONT ENVELOPE — conceptual order only

     +----------------------------------+
     |             camera               |
     |               PIR                |
     |                         D3       |
     |           22 mm button            |
     |        +----------------+          |
     |        | front speaker  |          |
     |        | 32 mm x 25 mm  |          |
     |        +----------------+    D2    |
     +----------------------------------+
```

The diagrams are conceptual, not drilling templates. The USB-C position still
fixes the PCB first, and the PCB fixes D3/D2. The camera, PIR, wired button, and
lid-mounted speaker must fit around those axes. Keep the speaker landscape and
below the button. Move it a few millimetres left or right only as needed to
clear D2, a closure column, or its own terminal projection; do not move the
USB-aligned PCB.

Because the speaker is attached to the removable lid, provide a flexible
two-wire service loop from J6 and strain relief at the lid. The loop must let
the lid open far enough to unplug J6 without pulling the camera flex, mic duct,
or speaker terminals. The speaker body, mesh, gasket, and terminals must all
clear the button barrel and both rigid light pipes when the lid closes.

## Tentative Recommended Front-Face Layout

Use the following layout as the first full-size paper template. Camera, PIR,
and button coordinates are tentative. The displayed D3 and D2 coordinates are
only predictions of where the fixed PCB geometry will place them. Coordinates
are hole centers measured on the hallway-facing `77 x 125 mm` face, viewed
from the front:

- origin `(0, 0)`: top-left edge;
- `X`: increases toward the right edge;
- `Y`: increases toward the bottom edge; and
- enclosure centerline: `X = 38.5 mm`.

| Feature | Initial center estimate `(X, Y)` | Opening/visible size | Status |
| --- | --- | --- | --- |
| Camera lens | Approximately `(38.5, 30 mm)` | Start undersize; finish just over the measured approximately `7 mm` circular base | Tentative raised position; verify the 70 mm camera flex reaches before drilling |
| PIR dome | Approximately `(38.5, 45 mm)` | Start near `8 mm`; hand-fit to the taper beneath the approximately `13 mm` dome | Tentative; it uses a flexible pigtail and can move around fixed items |
| Status light pipe, D3 | Predicted near `(51.5, 65 mm)` | Hole determined by the actual black `3 mm` lamp base | Fixed by the final PCB/USB position; transfer the physical LED center before drilling |
| Main button | Planning position near `(33, 69 mm)` if the predicted D3 axis is correct | Approximately `22 mm` barrel hole and `25 mm` visible bezel | Raised and slightly left only to clear predicted D3; recenter it if the physical D3 transfer permits |
| Front speaker | Planning position near `(38.5, 98.5 mm)` | Landscape outer envelope approximately `32 x 25 mm`; acoustic cutout is smaller | Below the button; shift slightly only if required by physical D2, closure columns, or terminal clearance |
| Charge/status light pipe, D2 | Predicted near `(51.5, 118.5 mm)` | Hole determined by the actual black `3 mm` lamp base | Fixed by the final PCB/USB position; check the lower lip and screw column before committing |

The LED estimates come from the current KiCad positions and assume the
approximately `59 x 91 mm` PCB is centered laterally behind the `77 mm` face
and its bottom edge is aligned with the bottom for USB-C access. Under that
assumption, D3 projects to approximately `(51.7, 65.3 mm)` and D2 to
approximately `(51.7, 118.7 mm)`. These are predictions, not drilling
coordinates. The physical projection made after the USB and standoffs are
final is authoritative.

```text
                 TOP — 77 mm
        0                                  77
     0  +----------------------------------+
        |  O                            O  |  existing closure holes;
        |                                  |  measure their true centers
        |                C                 |  camera (~38.5, 30)
        |                                  |
        |                P                 |  PIR (~38.5, 45)
        |                      S           |  D3 status pipe (~51.5, 65)
        |             B                    |  22 mm button (~33, 69)
        |          +-------------+         |
        |          |   speaker   |         |  landscape (~38.5, 98.5)
        |          +-------------+         |
        |                      L           |  D2 lower pipe (~51.5, 118.5)
   125  |  O                            O  |
        +----------------------------------+
                 BOTTOM / UNDERSIDE

        C camera     P PIR       S status light
        B button     L charge/status light
```

The sketch is positional, not to scale, and the `O` marks do not assert corner
hole coordinates. Draw the actual hole and internal screw-column keepouts on
the template first. Maintain at least `5 mm` of intact front-face plastic
between finished openings where possible, in addition to the measured column
and lip keepouts.

The lower D2 light-pipe prediction is the main risk in this layout because it
is only about `6.5 mm` above the nominal bottom edge and now sits near the
front-speaker envelope. Do not move the correctly aligned PCB or angle the
rigid rod. First establish the true D2 axis, then verify clearance from the
speaker frame, grille, lamp-base flange, internal lip, and lower-right screw
column. If they conflict, adjust the movable speaker/button arrangement or the
lamp-base/front-panel mechanical method; USB alignment remains the controlling
datum.

Before drilling, tape the paper template to the enclosure, install the button,
camera, PIR, and their wire/flex mock-ups behind it, and hold the closed unit at
its intended door height. Confirm that the camera flex reaches without a sharp
bend, the button is comfortable to press, the PIR can see the corridor, and no
feature overlaps a closure column.

## Optional PIR Future Contingency

It is reasonable to mount the PIR now as a future contingency. The enclosure
is indoors, and the visible unused dome is acceptable for this build. Fitting
it during the initial machining also avoids disassembling a finished enclosure
and generating cutting debris near installed electronics later.

Mount the PIR mechanically to the front panel with a short pigtail rather than
rigidly plugging its three pins into PCB connector J7. Start with an
approximately `8 mm` opening and enlarge it gradually with a hand reamer or
round file to match the point where the panel meets the taper. Do not advance
the large step drill automatically to `10`, `12`, or `14 mm`, and do not jump
directly to the approximately `13 mm` maximum dome diameter. The final opening
depends on the received sensor, panel thickness, and retaining position.

Keep the entire approximately `13 mm` dome unobstructed from the corridor
side. Retain the sensor by its PCB or a small bracket, not by squeezing the
Fresnel dome. Small neutral-cure silicone dabs at the module PCB edges are
acceptable as secondary retention; keep adhesive off the dome and component
side.

Until PIR firmware is intentionally enabled, leave its J7 pigtail disconnected,
labeled, insulated, and secured inside the enclosure. This avoids needless
sensor power draw or an accidental firmware interaction. When it is enabled,
verify the actual pigtail order against both devices rather than relying on
wire colors:

- J7.1: `+3V3`
- J7.2: `PIR_SENSOR_OUT`
- J7.3: `GND`

## Camera Opening

The approximately `7 mm` measurement is the widest circular lens base, not the
width of the complete camera module. The approximately `8.5 mm` square camera
head and ribbon stay behind the front panel. Do not try to pass the complete
module through a 7 mm hole.

Drill below final size and ream/file the circular opening until the lens base
has non-binding clearance. A likely finished opening is only slightly larger
than the measured 7 mm part, but determine it from the received module rather
than adopting a nominal drill size. Keep the lens as close to the inner panel
surface as safely possible. With the camera live, inspect all four image
corners and gradually bevel the opening outward only if the panel causes
vignetting.

Use a removable bracket or compliant retainers for the camera head and provide
strain relief for the flex. Do not put silicone or other adhesive on the lens,
in its field of view, or on the FPC contacts. Check the live image in the
actual well-lit corridor and also under dimmer conditions. In particular,
check whether either light pipe or the button ring reflects into the lens.

## Button Opening

The selected part is the `22 mm` version shown in the size family, not the 16
or 19 mm version. Its M22 x 1 barrel calls for an approximately `22 mm` panel
opening; its front bezel is approximately `25 mm` and covers the cut edge.
Confirm these dimensions on the received button.

On the taped layout, it is useful to trace the complete approximately `25 mm`
bezel so its clearance from D3, the speaker, and the camera can be checked.
That traced circle is a **clearance envelope**, not the cut line. Mark its
center clearly, drill from that center, and stop at the measured approximately
`22 mm` barrel diameter.

Use a pilot and step drill, stopping before the next step enters the panel.
The button should pass through without being forced or rocking excessively.
Deburr by hand, keep the supplied face gasket, and tighten the retaining nut
only enough to prevent rotation. Excess torque can bow or crack the ABS.

The approximate `33 mm` overall depth and rear terminals require a cylindrical
keepout behind the front panel. Fit heat shrink to every soldered terminal and
confirm that the lid can close without a terminal touching the PCB, battery,
speaker, wire, or enclosure screw.

## Light Pipes And Black Lamp Bases

Use the two black `3 mm` plastic lamp bases as the finished front-panel bezels
for the clear acrylic rods. The advertised `3 mm` dimension describes the
light-pipe/LED size; it may not be the enclosure hole diameter. Measure the
barrel and retaining tabs of an actual base, then drill a scrap-ABS test hole
and confirm that it clips securely through the measured panel thickness.

The purchased `3 mm x 100 mm` rods are intentionally oversized. Cut each one
only after its lamp base, the PCB standoffs, and the final PCB position have
been established. Do not calculate the length from the enclosure's advertised
depth.

### Transfer The Required Length

Use the uncut rod itself as a depth gauge:

1. Disconnect USB and the battery. Install the PCB on its final standoffs and
   install the appropriate black lamp base in the front panel.
2. Choose one factory-finished end as the temporary measuring tip. After the
   rod is cut, this end will be flipped around and used as the visible exterior
   end.
3. Temporarily close the enclosure. Insert that measuring tip slowly through
   the lamp base from outside until it only just touches the center of the PCB
   LED. Use no pressure; a 0603 LED is not a mechanical stop.
4. Mark the rod exactly at the intended exterior finished plane, normally
   flush with or slightly proud of the black lamp base. A narrow ring of
   painter's tape makes the mark easier to see.
5. Withdraw the rod and measure the marked contact length. Make a final-length
   mark `0.25-0.5 mm` shorter, toward the end that touched the LED. This is the
   required no-preload clearance.
6. Make the first cut approximately `0.5 mm` beyond the final-length mark on
   its discard side so the retained section remains slightly long. Sawing and
   sanding remove the final excess.

Repeat this transfer separately for D3 and D2. Their rods may not be the same
length because the enclosure, PCB, lamp bases, and adhesive stack have
tolerances. If the uncut rod cannot reach an LED squarely through a proposed
lamp-base position, recheck the projected hole center and panel transfer instead
of forcing the rod to bend or moving the USB-aligned PCB.

### Cut And Finish The Acrylic Rod

The preferred cutter is a fine-tooth razor saw or jeweler's saw:

1. Wrap the cut area with painter's tape and draw a square line around the rod.
   Do not deeply score the acrylic with a knife; a score can start a crack.
2. Support the rod close to the cut in a small miter block, a `3 mm` drilled
   wood guide, or between padded wood jaws. Clamp it only tightly enough to
   prevent rolling. Do not clamp bare acrylic directly in a steel vise.
3. Saw with light, short strokes and keep the blade perpendicular. Let the
   teeth cut; twisting or pushing hard can chip the end.
4. Place `400` grit abrasive on a known-flat block and sand the cut end with
   small figure-eight motions while holding the rod square. Progress through
   approximately `800` and `1200` grit; `2000` grit or plastic polish is
   optional if more optical clarity is needed.
5. Remove only a small amount at a time, clean away abrasive dust, and measure
   the total length with calipers after each grit.

If a fine saw is unavailable, a present and undamaged Dremel 426 reinforced
cutoff wheel on the correct mandrel is an acceptable fallback. Secure the rod,
wear eye protection, use the lowest practical speed, and make several light
passes around the circumference instead of forcing the wheel straight through.
Stop if the acrylic softens, smears, or discolors. Leave extra length for the
same hand-sanding process. Prefer the reinforced 426 over the small brittle 420
discs for this job.

Never cut a light pipe with aviation snips, diagonal/side cutters, pliers, or a
large drill bit. Those tools can crush, craze, or launch the acrylic. Do not
flame-polish the end.

### Final Fit

Install the finished rod with the untouched factory end facing the corridor
and the hand-polished end facing the PCB LED. Close and tighten the enclosure
normally, then verify:

- approximately `0.25-0.5 mm` remains between the rod and LED;
- the rod does not bow the PCB, hold the lid open, or rattle;
- the visible end is flush or only slightly proud of the black lamp base;
- the intended LED is bright and centered with acceptable viewing angle; and
- light from D2 and D3 does not leak into the camera or the other light pipe.

If the pipe is too long, remove it and shorten only the LED-facing end on the
flat abrasive block. If it is too short, replace it from the remaining rod
stock rather than filling the optical gap with structural adhesive. Add a
short black sleeve around the interior portion if needed to reduce light bleed.
Retain the rod, if the lamp base does not hold it sufficiently, with a tiny dab
of neutral-cure silicone on its side near the base. Keep adhesive away from
both optical end faces.

Do not assume the purchased `21.1 mm` LPA3.5 part is the correct finished
length; the actual enclosure and standoff stack determine it.

## Microphone Side Port And Duct

MK1 is near the PCB's upper-left edge and is a bottom-port microphone. Its
acoustic inlet is the small PCB hole directly beneath the package, opening into
the space between the PCB and the enclosure's rear wall. A front-panel hole
aimed at the top of MK1 would not couple correctly.

Place the exterior microphone pinhole on the **left side wall**, not on the
front face or underside. It must remain in the deep-base portion so it stays
with the PCB when the lid is removed. Use these initial transfer estimates only
for planning:

- vertical center: approximately `67.5 mm` down from the top, based on the
  current bottom-aligned PCB estimate;
- depth: approximately level with the under-PCB duct; calculate its exact
  center from the actual PCB clearance and tube diameter as described below;
  and
- port diameter: start at `1.0 mm` and enlarge no further than approximately
  `1.5 mm` after an audio test.

The final port is determined from the bonded PCB position, just like the light
pipes. After the standoff adhesive has fully cured, install the PCB and transfer
the longitudinal position of MK1's actual PCB acoustic hole to the left wall
using common top and rear datum edges. Align to the small acoustic hole through
the PCB, not merely to the `MK1` package outline or silkscreen. The side port
may move slightly along the wall to avoid a rib, but do not place it higher on
the enclosure for appearance: every unnecessary offset lengthens or bends the
duct.

Account for the tube's outside radius when setting the port depth. Let `H` be
the measured distance from the **inside rear floor** of the enclosure to the
underside of the installed PCB. For the `4 mm` OD tube laid horizontally under
the PCB, use:

```text
tube and side-port center above inside rear floor = H - 2 mm
```

If `H` is actually `10 mm`, the nominal tube/port center is therefore about
`8 mm` above the inside rear floor, not `10 mm`. When laying out that point on
the outside wall from the exterior rear surface, add the measured rear-wall
thickness. Confirm the result with a short physical piece of tube: its top
should meet the PCB underside at MK1 without lifting the PCB, flattening the
tube, or requiring a sharp bend.

Use the existing `2 mm` ID x `4 mm` OD silicone tube as a side-tapped duct:

```text
left exterior
  -> 1.0-1.5 mm side-wall pinhole
  -> enclosure-side end of silicone tube remains open and is seated behind that pinhole
  -> short tube run rightward in the under-PCB space
  -> approximately 1 mm side window cut in the tube and aligned upward to MK1's PCB hole
  -> a short section of tube continues just past the window
  -> PCB-side end of the tube is completely sealed with hot glue
```

At the PCB end, first seal the open end of the tube completely with hot glue.
Cut an approximately `1 mm` window in the **side wall of the tube**, slightly
back from that sealed end; do not use the end opening as the microphone pickup.
Turn this side window upward and align it to the actual acoustic hole through
the PCB, not merely to the printed `MK1` outline. Enlarge the window only if
recorded speech is too attenuated.

Fix the tube to the rear-facing underside of the PCB with hot glue, not tape.
Apply a small bead around the tube-to-PCB interface so the upward-facing side
window is acoustically sealed to the PCB hole, then use only the minimum extra
hot-glue dabs needed to stop the tube moving. Keep molten glue out of the tube
bore and MK1 acoustic hole, do not cover pads or test points, and do not use so
much glue that it bows the PCB or makes the board impractical to remove. Let the
glue cool before installing the PCB. The opposite, enclosure-side end of the
tube remains open: seat it directly behind the enclosure's microphone pinhole
and seal around its outside without blocking either the enclosure pinhole or
the tube bore.

Measure duct length along the acoustic path from the exterior side-wall port
to the upward-facing MK1 window. Aim for approximately `10-25 mm`. A direct
run up to about `30-40 mm` is acceptable if the enclosure geometry requires
it; avoid more than about `50 mm`. Do not coil the purchased one-metre tube or
store excess inside the enclosure. Long, narrow runs, tight bends, and
partially flattened sections reduce level and color speech even though the
doorbell operates half-duplex. Dry-fit with about `5 mm` of trimming allowance,
then cut the installed piece to the shortest relaxed route after the PCB
position is final.

Do **not** put the cut metal speaker mesh between the exterior pinhole and the
silicone tube. This indoor `1.0-1.5 mm` port does not need a metal grille, and
mesh can attenuate speech, rattle, shed sharp strands, or prevent the tube from
sealing against the wall. Seat the tube mouth directly behind the deburred
pinhole and seal only around its outside. If later testing demonstrates a real
dust problem, use a purpose-made acoustically transparent membrane only after
comparing recordings; do not improvise with the speaker mesh.

Keep the microphone duct entirely separate from the front speaker opening.
Do not terminate the tube in the speaker cavity, and do not let it touch the
speaker frame or grille; that would mechanically transmit vibration and worsen
speaker-to-microphone leakage.

## Front-Facing Speaker Below The Button

The speaker's approximately `25 x 32 mm` outside envelope is not automatically
the ABS cutout size. The opening follows the sound-producing diaphragm, while
the speaker frame, foam gasket, and mesh require an uncut perimeter land.
Measure the actual diaphragm and draw the opening from it. Rotate the speaker
so its approximately `32 mm` dimension runs left-to-right and `25 mm` dimension
runs top-to-bottom, then place the complete stack on the front lid below the
button using the placement map above.

For the taped first layout, draw three centered landscape outlines:

| Outline | Tentative size | Purpose |
| --- | --- | --- |
| Speaker outside envelope | `32 mm` wide x `25 mm` high | Clearance only; do not cut this full rectangle |
| Finished acoustic opening | Approximately `24 mm` wide x `17 mm` high | Rounded rectangle/oval exposing the diaphragm while retaining about `4 mm` of frame on each side |
| Initial rough opening | Approximately `22 mm` wide x `15 mm` high | First Dremel/file boundary; enlarge only after holding the actual speaker behind it |

For the current pencil layout, the inner rounded outline is the area to be
excavated. The surrounding approximately `32 x 25 mm` rounded rectangle marks
the speaker body and mounting clearance and must remain intact. Put an `X` in
the inner waste area before machining so the two outlines cannot be confused.

The approximately `24 x 17 mm` opening is a conservative estimate from the
seller drawing, not an authoritative received-part dimension. Give it rounded
corners of roughly `4 mm` radius or make it a smooth oval; sharp internal
corners add no useful acoustic area. Center it on the actual diaphragm, not
merely on the wire exit or outside plastic frame. Stop at the `22 x 15 mm`
rough opening, test the real speaker from behind, and file gradually toward the
finished line only where the diaphragm remains unobstructed.

For a finished opening near `24 x 17 mm`, start with a mesh patch around
`28 x 21 mm`. That supplies approximately `2 mm` overlap around the opening
while remaining inside the `32 x 25 mm` speaker envelope. Adjust both sizes to
the received diaphragm and flat mounting land. The mesh, gasket, and adhesive
must stay clear of the moving surround.

The stack from the corridor side toward the enclosure interior is:

```text
indoor corridor
  -> front-lid ABS wall and its acoustic opening
  -> cut metal mesh overlapping the opening from inside
  -> thin closed-cell foam/EVA perimeter gasket
  -> speaker face/frame
  -> speaker body, insulated terminals, and wiring
```

Before cutting, close the empty enclosure with cardboard templates representing
the button's full rear barrel, the speaker body and terminals, both rigid light
pipes, and the camera flex. The speaker may sit visually below the button while
its rear body is shifted a few millimetres laterally to clear D2 or a terminal.
Preserve at least approximately `3-5 mm` between the speaker's outer frame and
the button hardware, lamp-base bodies, and closure-column keepouts.

Cut the ABS opening first, then derive the grille patch from it. The mesh patch
should overlap intact ABS by at least `2-3 mm` around the complete opening,
while still fitting flat beneath the speaker frame/gasket or its separate
retaining bracket. Do not pre-cut the mesh to `25 x 32 mm` solely because that
is the speaker envelope. Cut an oversized rough blank from the `100 mm` mesh
circle, test it against the actual opening, and trim it to its final rounded
shape.

Wear gloves and eye protection when using aviation snips. Short offcuts and
wire strands are sharp and can spring away. After cutting, remove loose strands
and burrs, round every corner, and capture the entire cut perimeter under the
gasket, adhesive, or a folded edge so it cannot abrade wiring. Vacuum and wipe
the work area before bringing the PCB or battery back.

Prefer a removable clamp or small bracket for the speaker. If bonding the mesh,
apply a thin continuous bead of neutral-cure silicone only around its perimeter
and allow it to cure fully before electronics are installed. Keep adhesive and
gasket material off the diaphragm, suspension, acoustic opening, and any rear
speaker vent. The gasket should prevent front sound from leaking around the
frame and cancelling with rear sound. Add compliant retention so the grille
cannot buzz.

Because the speaker is lid-mounted, terminate its lead in the verified `2.0`
plug at J6 and leave a controlled service loop. Add flexible strain relief near
the speaker without gluing its terminals. When opening the enclosure, support
the lid and unplug J6 before allowing the lid to hang; neither the speaker lead
nor camera flex is a hinge or tether.

The speaker is marked `4 ohm`, `3 W`; this is its impedance and power-handling
class, not a promise that the board delivers 3 W. U4 is a MAX98357A powered
from `/SYS`, not a dedicated fixed 5 V rail. Set the usable maximum with
digital volume, clipping, sound-quality, current, and temperature tests.

J6 is a bridged Class-D output:

- J6.1 is `OUTP` and J6.2 is `OUTN`.
- Connect the one speaker only across J6.1 and J6.2.
- Never connect either speaker lead, terminal, or the metal mesh to ground.
- Keep both terminals insulated and mechanically clear of the grille.
- Although the received connector has been verified as `2.0`, confirm its
  housing/latch fit at J6, then verify pin order, polarity, and isolation from
  ground with a meter before power-up.

## Cutting Support And Tools

Sacrificial backing is a **temporary machining aid**, not a structural part of
the finished enclosure. The rigid ABS housing does not need a plywood liner or
permanent reinforcement after machining. Backing reduces local flex, exit-side
chipping, and the chance that a drill grabs as it breaks through; it does not
make an otherwise weak enclosure usable.

Direct backing is not mandatory for every cut. A small pilot, relief, or
`1.0-1.5 mm` microphone hole may be drilled without backing when the local wall
is already rigid, the enclosure is clamped so it cannot move, the bit is sharp,
and low pressure is used near breakthrough. For the `22 mm` step-drilled button
hole, use direct rigid backing whenever it can be fitted. For the camera, lamp
base, and PIR holes, backing is recommended but may be replaced by a padded jig
or clamps that support the wall within a few millimetres of the cut. For the
USB-C and speaker cutouts, use direct backing or support the panel close to the
cut on all practical sides; leave the powered rough cut inside the line and
finish it with hand files. If the ABS chatters, bows, or vibrates, stop and add
closer support rather than applying more tool pressure.

A silicone electronics work mat is not a cutting backer: it does not support
the plastic at breakthrough and can be grabbed, torn, or wrapped by a drill or
rotary cutter. It may remain under the rigid support only as bench protection,
well away from the cutting path.

Because the enclosure has lips and deep walls, a flat sheet under the whole
part may not contact the cutting location. Fit a scrap plywood block inside
the lid or base so it bears directly beneath the marked area. Clamp the ABS
gently against that block without flattening a molded lip or distorting the
panel. Scrap MDF or a smooth hardwood block is also acceptable if it is rigid,
flat, and expendable. Never hold a small backing block by hand behind a powered
bit. Where no block can contact the cut, use padded clamps or a fitted cradle
to restrain the housing close to the work instead.

Available and recommended tools:

- Dremel 4000-2/30, `1.6 A`, corded variable-speed rotary tool;
- Dremel 561 multipurpose spiral-cutting bit from the kit, after confirming it
  is still present and undamaged;
- the available Ryobi variable-speed drill for the large step bit and ordinary
  twist bits, or a drill press if available;
- the available RYOBI A981951 `195-Piece Drill and Driver Bit Set`; use its
  black-oxide twist drills for the ABS work identified below, not similarly
  sized hex driver, masonry, or brad-point bits;
- the available `4-32 mm` HSS titanium-coated step drill for the 22 mm button
  and other larger circles;
- a separate sharp metric `1.0 mm` twist bit for the initial microphone port;
  optional `1.2 mm` and `1.5 mm` bits are also separate purchases if audio
  testing later justifies enlargement;
- fine-tooth razor saw or jeweler's saw, ideally with a small miter block or
  padded wood guide for the `3 mm` acrylic rods;
- flat, half-round, and needle files;
- deburring tool or sharp hand scraper;
- `220`, `400`, `800`, and `1200` grit abrasive paper;
- aviation snips for the loose metal mesh only;
- white china marker/grease pencil for direct marks on black ABS;
- low-tack painter's tape plus a `0.5 mm` mechanical pencil or fine technical
  marker for precise layout lines; and
- calipers, square, awl, clamps, rigid sacrificial plywood, eye protection,
  gloves, and dust collection.

Use gloves only while handling and cutting the loose metal mesh; do not wear
loose gloves near a rotating drill, step bit, or Dremel accessory.

### RYOBI 195-Piece Set Sizes Used Here

The available drill/driver set is RYOBI model `A981951`. The same 195-piece
package is also sold under the `A981952QP` listing. Its black-oxide twist drills
are the set's general-purpose bits intended for plastic. The relevant small
sizes documented for this set are:

| Case label | Metric diameter | Enclosure use |
| --- | ---: | --- |
| `1/16 in` | `1.59 mm` | Smallest twist drill in this set; **too large** for the microphone's `1.0-1.5 mm` limit |
| `5/64 in` | `1.98 mm` | Standard pilot for the step drill and tight USB-C relief holes |
| `3/32 in` | `2.38 mm` | Optional intermediate relief size when the drawn waste area permits it |
| `7/64 in` | `2.78 mm` | Speaker relief holes and roomier USB-C relief holes |
| `1/8 in` | `3.18 mm` | Optional larger relief hole only when it remains safely inside the waste line |
| `9/64 in` | `3.57 mm` | Candidate undersize lamp-base hole after measuring the base |
| `5/32 in` | `3.97 mm` | Candidate undersize lamp-base hole after measuring the base |
| `11/64 in` | `4.37 mm` | Candidate undersize lamp-base hole after measuring the base |
| `3/16 in` | `4.76 mm` | Candidate undersize lamp-base hole after measuring the base |
| `1/4 in` | `6.35 mm` | Available, but the step drill stopped at `6 mm` is preferred for the camera opening |
| `5/16 in` | `7.94 mm` | Available, but the step drill stopped at `8 mm` is preferred for the PIR opening |

Read the size stamped beside the **black-oxide twist-drill** slot in the case
before fitting the bit. The set also contains hex driver bits labeled with
fractional sizes; those are for driving hex-socket fasteners and do not drill
holes. For a measured lamp-base clip-body diameter, subtract `0.5 mm`, then
choose the largest listed twist bit no greater than that result. Drill a scrap
test and hand-ream to the snap fit.

The A981951 set does not replace the separate `4-32 mm` step drill. Use the
step drill for the camera, PIR, and 22 mm button because its gradual enlargement
is easier to control in a thin ABS panel than a large conventional twist bit.
It also does not contain a suitably small microphone bit: `1/16 in` equals
`1.5875 mm`, already beyond the specified `1.5 mm` maximum. Obtain a true
`1.0 mm` bit and preferably turn it in a pin vise; do not substitute `1/16 in`.

### Recommended Bits And Cutters

The sizes below are starting sizes for the measured parts, not permission to
skip the scrap-ABS trial. Use sharp HSS bits intended for metal/plastic. Do not
use masonry bits, spade bits, or a hole saw on this enclosure. A dull bit that
rubs or melts the ABS should be replaced rather than pushed harder.

| Operation | Recommended bit or cutter | Where to stop |
| --- | --- | --- |
| General pilot for the step drill | Set's black-oxide `5/64 in` (`1.98 mm`) twist bit in the Ryobi drill | Through the wall at low speed; the pilot remains smaller than the step bit's `4 mm` first step |
| 22 mm button | `4-32 mm` HSS step bit in the Ryobi drill, after the `5/64 in` pilot | Stop immediately after the `22 mm` step clears the visible face |
| Camera lens | Set's `5/64 in` pilot, then the step bit only to `6 mm`; hand reamer or round needle file afterward | Ream gradually to a non-binding fit slightly over the measured approximately `7 mm` lens base |
| PIR neck | Set's `5/64 in` pilot, then the step bit only to `8 mm`; hand reamer or round file afterward | Stop powered enlargement at `8 mm` and hand-fit the actual tapered neck |
| Black lamp-base hole | Set's `5/64 in` pilot, then the largest available black-oxide twist bit at least `0.5 mm` below the measured clip-body diameter; hand reamer afterward | Select from the conversion table, then ream to the scrap-tested snap fit; do not assume the advertised `3 mm` rod size is the hole size |
| Microphone port | Separate sharp metric `1.0 mm` twist bit, preferably in a pin vise; separate `1.2 mm` and then `1.5 mm` bits only after audio tests | Start at `1.0 mm`; the set's `1/16 in` (`1.59 mm`) bit is too large and must not be used |
| Speaker relief pattern | Set's black-oxide `7/64 in` (`2.78 mm`) twist bit in the Ryobi drill, then Dremel 561 (`3.2 mm`/`1/8 in`) spiral-cutting bit | Keep every relief hole and the 561 path inside the rough waste line; finish with files |
| USB-C corner/relief pattern | Set's black-oxide `5/64 in` (`1.98 mm`) or `7/64 in` (`2.78 mm`) twist bit, selected to preserve the marked corner radius, then Dremel 561 (`3.2 mm`/`1/8 in`) | Keep the powered cut inside the line and finish to the actual plug with needle/flat files |
| ABS edge deburring | Sharp hand scraper or single-flute deburring tool; a countersink may be turned lightly **by hand only** | Remove the burr without forming a visible chamfer or enlarging the opening |
| Acrylic light-pipe fallback | Dremel 426 reinforced cutoff wheel on its correct mandrel | Leave approximately `0.5 mm` extra and finish the LED-facing end on flat abrasive paper |

The Dremel 407 sanding drum may remove a small amount from an inaccessible
rough edge at low speed, but it is not a sizing bit. Grinding stones and the
small brittle 420 cutoff discs are not recommended for the enclosure openings.

### Marking The Black ABS

Manually laying out the features on the received enclosure is useful and is
the final check against the paper template. For direct white marking, use a
**white china marker/grease pencil** intended for glass, plastic, or other
non-porous surfaces. A pull-string china marker is convenient; expose only a
short tip and shape it to a narrow chisel point on scrap abrasive paper. An
ordinary white artist's colored pencil generally will not leave a consistent,
precise line on smooth ABS.

The preferred precision workflow is still to cover the cutting area with one
smooth layer of low-tack painter's tape and draw on the tape with a sharp
`0.5 mm` mechanical pencil or fine technical marker. This gives a thinner line
than a grease pencil, prevents old corrections from remaining on the visible
ABS, and protects the surface from light tool scratches. Use the china marker
directly on uncovered side walls or curved areas where tape will not lie flat.

Use this order:

1. Clean dust and skin oil from the enclosure with a plastic-safe method and
   let it dry. Test the chosen tape and marker on an inconspicuous interior
   area first.
2. Apply tape without wrinkles or overlapping edges through a drilling center.
   Do not stretch the tape because it can relax and move a mark.
3. Mark the measured top-left datum, vertical centerline, enclosure seam,
   closure-column keepouts, USB-aligned PCB outline, and physically projected
   D2/D3 centers.
4. Transfer the camera, PIR, button, mic, speaker, and USB outlines from their
   full-size templates. Trace the actual bezels, grille, and plug shell as a
   second check, but do not use an outside part outline as the cut line unless
   the guide specifically calls for it.
5. Draw both the finished line and a separate rough-cut line `1-2 mm` inside it
   for Dremel work. Mark waste areas with an `X` so the wrong side is not
   removed.
6. Check every center and keepout from the same datum edges with calipers and a
   square. Lightly prick only confirmed drill centers with an awl; do not score
   long lines into the ABS with a knife or metal scriber.
7. Photograph the completed layout with a ruler in frame, close the empty
   enclosure for one last visual check, then machine through the tape.

A fine white or silver water-based paint marker can also mark painter's tape,
but it is not required. Avoid broad paint pens for drill centers, and do not
apply solvent-heavy paint markers directly to the finished ABS without a scrap
test. Grease-pencil marks are for visibility, not measurement: always choose
the cut center from the thin transferred line or awl point rather than from the
edge of a thick white stroke.

The missing A576 sanding/grinding guide and 678 circle/straight-edge guide are
not required for this enclosure. The 565 multipurpose cutting guide shown in
the manufacturer reference below is a different optional attachment and is
not part of the pictured 4000-2/30 contents. It can improve depth control, but
this build does not depend on buying it.

Without a 565 guide, drill closely spaced relief holes well inside the speaker
or USB waste line. Use the 561 only to join those holes and remove central
waste, keeping the bit perpendicular and at least `1-2 mm` inside the final
line. Complete the shape with hand files. The kit's 407 sanding drum and sanding
bands may rough an inaccessible interior edge at low speed, but they remove
material quickly and are not finish tools. Avoid the grinding stones on ABS,
and reserve the cutoff wheels for coarse straight relief cuts where an
overshoot cannot reach the finished outline.

For ABS, start rotary cutting near the low end of the tool manufacturer's
recommended plastic speed and make light passes. Pause if the plastic smears
or melts instead of producing chips. Do not freehand a visible finished edge
with a cutoff wheel. Cut inside the waste line and file to the line.

The `149 g`, `4-32 mm` step bit must be used only in the Ryobi drill or a drill
press. It is too large and heavy for the Dremel 4000's collet, bearings, and
handheld control; never attempt to adapt it to the rotary tool. Seat its shank
fully in a properly sized Ryobi chuck and tighten every chuck position if the
chuck uses a key. Test it at low speed away from the work and do not use it if
the tip or cutting steps visibly wobble. Select the Ryobi's low-speed/high-
torque gear if it has one, disable hammer mode, and use ordinary drilling mode.

For the `22 mm` button hole, clamp the ABS to direct rigid backing so both hands
can control the Ryobi. For smaller round holes, use the support decision above.
Make the `5/64 in` (`1.98 mm`) pilot, begin the step bit at low trigger speed,
keep the drill perpendicular, and let it cut without leaning on it. Identify
the `22 mm` step before starting and stop as soon as that step has passed
through the button opening; the next step will make the hole oversized. The
Dremel remains reserved for the 561 spiral bit, sanding drum, and small
cutoff-wheel work described above.

Manufacturer references:

- [Dremel 4000 specifications](https://www.dremel.com/us/en/p/4000-2-32-f0134000fb)
- [Dremel 561 multipurpose cutting bit](https://www.dremel.com/us/en/p/561-26150561ac)
- [Dremel 565 multipurpose cutting guide](https://www.dremel.com/us/en/p/565-26150565ac)
- [RYOBI A981951 195-piece set identification and intended materials](https://www.homedepot.ca/product/ryobi-drill-and-driver-bit-set-195-piece/1001042704)
- [RYOBI A981952QP detailed set contents](https://www.retailmarket.net/products/ryobi-a981952qp-195-piece-drilling-and-driving-kit-for-wood-plastic-metal-and-masonry-work/)

## Opening Method By Feature

| Feature | Tools | Support | Final-fit requirement |
| --- | --- | --- | --- |
| 22 mm button | Ryobi, set's black-oxide `5/64 in` (`1.98 mm`) pilot, then `4-32 mm` step bit; hand scraper for deburring | Direct rigid backing strongly recommended | Drill from the visible face, stop at 22 mm, retain the supplied gasket, and enlarge only enough for a free, non-rocking fit |
| Camera | Ryobi, set's `5/64 in` pilot, step bit stopped at `6 mm`, then hand reamer/round needle file | Direct backing recommended; a close padded jig is acceptable | Clear the measured approximately 7 mm circular base without binding; live-test and bevel only enough to eliminate vignetting |
| Speaker | Ryobi and set's black-oxide `7/64 in` (`2.78 mm`) relief bit; Dremel 4000 with 561 bit; half-round/flat files | Backing or close panel support recommended | File to the measured diaphragm outline while preserving a continuous mesh/gasket overlap land |
| USB-C | Ryobi and set's black-oxide `5/64 in` (`1.98 mm`) or `7/64 in` (`2.78 mm`) corner/relief bit; Dremel 4000 with 561 bit; needle/flat files | Backing or a fitted cradle supporting the wall close to the cut recommended | File to a cable-tested rounded rectangle; leave clearance for the complete plug shell and strain relief |
| Microphone | Pin vise preferred, or Ryobi, with a separate sharp metric `1.0 mm` bit; separate `1.2/1.5 mm` bits only if testing requires enlargement | Optional if the wall is rigidly clamped and cannot flex | Deburr without enlarging and align it to the separate silicone duct; do not use the set's `1/16 in` bit |
| Black lamp base | Ryobi with the set's `5/64 in` pilot and a measured undersize black-oxide finish bit selected from the conversion table, then hand reamer | Recommended; may be omitted for a rigidly clamped panel and sharp small bit | Validate the actual clip body and panel thickness in scrap; `3 mm` is not necessarily the panel-hole size |
| PIR contingency | Ryobi with the set's `5/64 in` pilot and step bit stopped at `8 mm`, then hand reamer/round file | Direct backing recommended; a close padded jig is acceptable | Fit the actual tapered neck without squeezing it; do not jump directly to the approximately `13 mm` maximum dome size |

## Marking And Cutting Sequence

1. **Tools: correct hand screwdrivers and a parts tray.** Remove the front lid,
   PCB, camera, LiPo, speaker, loose fasteners, and every wire before machining.
2. **Tools: calipers, small square, ruler, painter's tape, and `0.5 mm` pencil.**
   Measure the `77 x 125 mm` front face, its four corner holes, the internal
   screw columns, wall thickness, lips, ribs, and curved areas. Draw explicit
   no-cut keepouts around them.
3. **Tools: M2.5 hand screwdriver, calipers, square, and the actual USB cable.**
   Dry-fit the PCB on loose standoffs with the cable inserted. Move the PCB only
   until USB-C is square, approximately flush or slightly recessed, and free of
   side load. Confirm PCB-to-wall, boss, battery, and button clearances.
4. **Tools: Ryobi drill with the set's black-oxide `5/64 in` (`1.98 mm`) or
   `7/64 in` (`2.78 mm`) relief bit, Dremel 4000 with 561 bit, needle and flat
   files, clamps, and rigid backing or a support cradle.** Mark and machine the
   USB-C opening first with all electronics removed. Keep the powered cut
   inside the line, deburr it, and repeat the cable fit until the final PCB
   position is known.
5. **Tools: M2.5 hand screwdriver, calipers, adhesive mixing stick/applicator,
   masking material, and timer.** Bond and fully cure the four standoffs at that
   exact PCB position using the procedure below. Reinstall the PCB and confirm
   that USB alignment did not move. Do not later move the PCB for appearance.
6. **Tools: calipers, square, ruler, paper template, and sharp pencil.** Project
   the physical centers of D3 and D2 perpendicular to the front lid. Measure
   from common datum edges, mark both axes, and verify them independently.
7. **Tools: calipers, ruler, pencil, and a short `4 mm` OD tube offcut.** With
   the cured PCB installed, transfer MK1's actual acoustic-hole station to the
   left side. Measure clearance `H`, mark the port center at `H - 2 mm` from the
   inside rear floor, and verify it with the tube offcut before drilling.
8. **Tools: full-size paper/cardboard templates, scissors, painter's tape,
   calipers, and square.** Arrange the camera, PIR, button, and landscape speaker
   around the fixed light-pipe axes and closure-column keepouts. Dry-fit every
   component, flex, pigtail, tube, and cable bend; close without force.
9. **Tools: painter's tape, `0.5 mm` pencil or fine marker, calipers, square, and
   awl.** Cover both faces of each remaining cutting area, transfer the template
   to the visible face, and check it against real parts from both sides. Lightly
   prick confirmed centers, then remove every electronic and wired part.
10. **Tools: the same drill, Dremel bits, files, and reamers specified in the
    feature table.** Make the complete remaining pattern in scrap ABS or a spare
    enclosure first. Correct the template before cutting the final enclosure.
11. **Tools: fitted scrap plywood/MDF/hardwood block or padded support jig, plus
    clamps.** Directly back the button hole and other cuts where practical. For
    cuts where backing is optional, confirm the clamped wall cannot flex. Keep
    the silicone work mat out of every powered tool path.
12. **Tools: awl, Ryobi drill, the set's sharp black-oxide `5/64 in` pilot bit
    and `7/64 in` relief bit, plus a separate metric `1.0 mm` microphone bit.**
    Mark centers lightly, use the bit assigned in the table, drill at low
    speed, reduce pressure near breakthrough, and clear chips frequently.
13. **Tools: Ryobi drill and `4-32 mm` step bit.** Drill round holes from the
    visible face at low speed. Approach the final step slowly and stop before
    the next diameter enters the ABS; never put the step bit in the Dremel.
14. **Tools: Ryobi drill and the set's black-oxide `7/64 in` (`2.78 mm`)
    relief bit, Dremel 4000 with 561 bit, then half-round and flat files.** Make
    speaker relief holes inside the waste, join them while remaining `1-2 mm`
    inside the line, and hand-file to final shape. A Dremel 565 guide is
    optional.
15. **Tools: aviation snips, gloves, eye protection, flat file, and pliers for
    folding or containing edges.** Cut and finish the speaker mesh away from all
    electronics. Collect every offcut and loose strand.
16. **Tools: hand scraper/deburring tool, needle files, abrasive paper, vacuum,
    and soft brush.** Remove tape, deburr both faces by hand, and remove all
    plastic and metal debris. Do not flame-polish ABS.
17. **Tools: actual components, actual USB plug, calipers, hand screwdrivers,
    and inspection light.** Trial-fit every interface without adhesive. Confirm
    button travel, camera/PIR view, light-pipe alignment, lamp retention, grille
    overlap, speaker clearance, USB insertion, and closure-screw access.

For the current build, the standoffs were already aligned and bonded while the
USB-C opening was still only marked. Do not disturb or machine near them during
the adhesive's fixture or full-cure interval. After full cure, remove the PCB
and every electronic part, support the wall with fitted plywood or a close-
support cradle, cut the USB opening undersize, and file it to the actual cable
plug. Reinstall the PCB to verify strain-free insertion; do not shift the cured
standoffs to compensate for the opening.

## PCB Standoffs And USB-C Position

The photographed placement is the primary mechanical datum: the PCB is flat
on four standoffs in the deep base, and the USB-C receptacle is aligned with
the underside edge for easy charging and firmware access. Establish this
position before placing any front-panel component. Once the standoffs are
bonded, the two rigid light-pipe axes are consequences of this position and
must not be adjusted independently.

- The PCB must be supported only by its four standoffs. It should not rest on
  the enclosure edge, a molded boss, or the USB-C receptacle.
- Leave approximately `0.5-1 mm` clearance from PCB edges to enclosure walls
  wherever practical so molding variation and vibration do not wedge the
  board.
- Align the USB-C receptacle with the opening, but keep its front edge flush
  with or slightly recessed behind the enclosure's exterior surface. A
  protruding connector is easier to strike and lever off the PCB.
- Size the opening using the actual charging/data cable, including the plug's
  molded shell. The plug must insert straight without the ABS pushing it up,
  down, or sideways.
- Do not place a standoff or adhesive fillet beneath the USB opening. Repeated
  cable insertion loads must be carried by the mounted PCB without flexing it.
- Confirm that all four enclosure closure screws remain accessible with the
  PCB installed.
- Do not shift the PCB for a more symmetrical front-panel arrangement after
  the USB position is correct. Camera, PIR, and button connections provide the
  placement flexibility; the rigid D2 and D3 light pipes do not.

To install the standoffs:

1. **Tools: calipers or depth gauge, M2.5 screws, and hand screwdriver.** Verify
   screw length and approximately `3-5 mm` thread engagement in all four
   standoffs away from the enclosure. Confirm that no screw bottoms out.
2. **Tools: M2.5 hand screwdriver.** Attach the standoffs loosely to the PCB
   with nylon washers and screws. Do not use a powered driver.
3. **Tools: painter's tape or protective film, disposable mixing card, mixing
   stick, and small adhesive applicator.** Cover nearby PCB areas and place only
   a small amount of plastic-compatible bonder on each standoff base. Follow
   the adhesive instructions and wear any gloves they specify.
4. **Tools: calipers, square, actual USB cable, and temporary low-tack tape if
   needed.** Position the PCB at its exact final USB and wall clearances. Use it
   only as a temporary alignment jig while the standoffs set.
5. **Tools: timer and M2.5 hand screwdriver.** Observe the stated fixture and
   full-cure times. Remove the screws and PCB only when doing so cannot shift
   the standoffs. If the PCB must remain as the jig, keep epoxy off it and apply
   no cable, cutting, or side load while curing.
6. **Tools: small adhesive applicator.** After removing the PCB, allow any
   remaining full-cure interval to pass. Add a small fillet only if the adhesive
   instructions and clearances permit it.
7. **Tools: M2.5 hand screwdriver and inspection light.** Reinstall the PCB and
   washers. Tighten evenly just past snug; do not bow the PCB or strip the
   bonded standoffs.

The edge-accessible USB arrangement does make charging and firmware updates
easier. Its success criterion is a receptacle approximately flush with or
slightly recessed behind the exterior, plus strain-free insertion. The PCB
itself must not be physically pressed against the enclosure wall.

## Battery Adhesive Fastener

Retain the `103450` LiPo vertically on the right interior wall with its `50 mm`
dimension vertical, `34 mm` dimension front-to-back, and `10 mm` thickness
projecting inward. Keep it entirely in the deep-base half so opening the lid
does not pull on the pouch or leads.

Use a removable adhesive-backed hook-and-loop fastener or a light strap with
adhesive anchors. Prefer bonding the removable fastener to a thin fishpaper or
plastic carrier around the battery rather than applying an aggressive adhesive
directly to the bare pouch. Provide a pull tab and remove the battery by
separating the fastener, not by prying or sharply bending the cell.

Before applying adhesive, clean the enclosure contact area according to the
fastener manufacturer's instructions and let it dry. Use enough area to resist
the door's repeated acceleration without compressing the pouch. Add a thin
non-compressing pad only where needed to prevent rattle. Do not:

- clamp, bend, puncture, or tightly wrap the pouch;
- place hard PCB solder joints, mesh edges, screw tips, or button terminals
  against it;
- cover a swollen or damaged cell to force it to fit;
- bridge the lid/base seam with the battery or its fastener; or
- trap the battery lead so opening or servicing the enclosure pulls on it.

Keep fishpaper/Kapton between the battery zone and conductive or sharp parts,
and leave a service loop to J1.

## Ventilation And Closed-Enclosure Thermal Check

Do not add dedicated ventilation holes to this indoor enclosure unless closed-
enclosure testing identifies a real thermal problem. The speaker opening,
microphone pinhole, USB-C cutout, button, and lamp-base interfaces already make
the machined case non-hermetic. More holes primarily admit dust and insects,
weaken the ABS, and can create another acoustic path between the front speaker
and microphone.

Bottom-only holes produce little useful convection without a corresponding
upper outlet. They also do not make a damaged or failing LiPo safe. The battery
must instead be protected from puncture and compression, retained without
covering damage or swelling, and operated within the cell and charger
manufacturers' limits.

After assembly, run a representative worst-case closed-enclosure check that
includes charging from a low state, Wi-Fi and camera activity, speaker
playback, and several consecutive doorbell events. Stop for abnormal heating,
odor, swelling, resets, or charging faults. If measurements reveal excessive
temperature, investigate charging current, duty cycle, component dissipation,
and battery placement before considering vents. Add vents only from measured
evidence and keep any eventual opening away from the battery pouch, mic duct,
speaker cavity, USB-C receptacle, and exposed electronics.

## Internal Assembly Order

The USB opening and standoffs are established earlier so the rigid light-pipe
axes can be transferred before the other front holes are cut. After all
remaining machining is complete:

1. **Tools: vacuum, soft brush, inspection light, and lint-free wipe.** Remove
   every plastic chip, metal strand, and abrasive particle, including debris
   around the bonded standoffs. Keep electronics and the LiPo away until the
   work area is clean.
2. **Tools: inspection light, M2.5 hand screwdriver, and actual USB cable.**
   Inspect the four `10 mm` standoff bonds, reinstall the PCB, and verify the
   final USB-C position once more.
3. **Tools: button nut wrench or correctly sized spanner, small hand
   screwdrivers, tweezers, labels, and heat-shrink/heat gun where required.**
   Install the camera and PIR retention, button gasket/nut, and black lamp
   bases. Tighten the button only enough to prevent rotation. Label, insulate,
   and secure the disconnected PIR pigtail.
4. **Tools: painter's tape, calipers, fine-tooth razor/jeweler's saw, padded
   miter block, flat abrasive block, and `400/800/1200` grit paper.** Transfer,
   cut, finish, and fit each light pipe using its dedicated procedure above.
   Remove the PCB before applying any retention silicone.
5. **Tools: scissors or craft knife for the closed-cell gasket, small bracket
   hand tools or silicone applicator, heat-shrink and heat gun, and tweezers.**
   Install the front-lid mesh, gasket, and landscape speaker. Insulate its
   terminals and add the flexible J6 service loop and strain relief. Do not cut
   mesh during final assembly.
6. **Tools: sharp scissors or tube cutter, fine pointed cutter, tweezers, and a
   hot-glue gun with a fine applicator.** Seal the PCB-side tube end with hot
   glue, cut the side window slightly back from that end, align and hot-glue the
   window to the acoustic hole under MK1, and leave the enclosure-side end open
   behind the side-wall pinhole. Seal around both interfaces without blocking
   the tube bore, MK1 hole, or enclosure pinhole, and do not bow the PCB.
7. **Tools: M2.5 hand screwdriver.** Install the PCB with four nylon washers and
   correctly sized M2.5 screws. Tighten evenly just past snug.
8. **Tools: scissors, plastic-safe cleaning wipe specified by the fastener
   maker, ruler, and hand roller or firm finger pressure.** Attach the battery's
   removable fastener/carrier to the right wall, add insulation, and retain the
   battery without compression. Observe the fastener's dwell time.
9. **Tools: tweezers, flush cutters, and cable ties or adhesive tie mounts.**
   Route the speaker wires away from the camera flex and ESP32 antenna. Keep the
   mic tube away from the speaker and every cable away from sharp mesh, button
   terminals, screw threads, and the lid lip. Cut only tie tails, never wiring.
10. **Tools: digital multimeter in continuity/resistance mode.** With all power
    absent, perform the continuity and short checks, including isolation of both
    speaker leads and the metal mesh from ground. Then connect the battery.
11. **Tools: inspection light and the correct enclosure hand screwdriver.**
    Close the enclosure gradually while watching every internal part. Stop if
    the lid presses on the LiPo, PCB, camera flex, speaker, light pipes, or
    button terminals; tighten the four closure screws evenly by hand.

## Mounting To The Apartment Door

Apply renter-safe adhesive mounting strips or another approved removable
fastener to the flat rear of the deep base, not across the removable lid or
its four closure screws. Follow the fastener's surface-cleaning, load, dwell,
and removal instructions. Use multiple separated strips so the enclosure
cannot pivot when the button is pressed or a USB cable is inserted.

Use a tape measure and pencil or removable tape to set the height, a small
level to keep the housing vertical, the surface-cleaning wipes specified by
the mounting-strip maker, and firm hand pressure or a small hand roller to seat
the strips. Do not drill the apartment door unless the owner has approved a
specific mechanical fastener and the door construction has been verified.

Test Wi-Fi/Bluetooth performance with the closed unit held at the exact final
location before committing the door adhesive. A metal apartment door can
affect the ESP32 antenna even though the enclosure is plastic. If reception is
poor, adjust placement or add a non-metallic stand-off layer before making
electrical or PCB changes. Use a discreet secondary safety tether if permitted
and if a falling enclosure could strike someone or damage the LiPo.

## Acceptance Checks

- Inspect the four standoffs for full bonding and verify that the PCB is not
  touching an enclosure edge or boss.
- Insert the real USB-C cable repeatedly. Its shell must not bind on the ABS,
  and the PCB or receptacle must not visibly flex.
- Verify the speaker plug pin positions and continuity. Neither speaker lead
  may have continuity to ground or the metal grille.
- Start playback at low digital volume and increase in small steps. Listen for
  clipping, grille buzz, and air leaks; check U4, the speaker, and power path
  for abnormal heating on both battery and USB power.
- Record visitor speech through the completed mic duct with the speaker silent,
  then test half-duplex operation for acoustic leakage. Do not infer
  full-duplex capability.
- Inspect a live camera frame in the actual corridor. Check all four corners
  for vignetting and check for button/light-pipe reflections.
- Confirm that the contingency PIR dome has an unobstructed view, cannot move
  when the door closes, and has its unused J7 pigtail insulated and secured.
  Perform a functional detection test only after PIR firmware is enabled.
- Verify button travel and ring illumination without panel flex.
- Confirm both lamp bases remain clipped in place and neither rod preloads the
  PCB when the four enclosure screws are tightened.
- Tug wiring and the battery fastener gently. Nothing should approach the
  speaker mesh, button tabs, closure screws, or camera flex, and the battery
  must not shift when the closed enclosure is handled like a door-mounted
  product.
- Run a representative charging and operating cycle with the enclosure closed
  and confirm there is no abnormal temperature rise, odor, swelling, or reset.
- Press the installed button repeatedly and connect/disconnect USB while the
  enclosure is mounted. The mounting strips must not peel, rock, or creep.

Record the final hole diameters, speaker-opening outline, mesh dimensions,
feature center coordinates from the `77 x 125 mm` face, standoff screw length,
adhesives, photographs, and maximum validated playback setting in this file or
a linked as-built record. Until those values are measured on the completed
unit, dimensions identified as approximate remain construction starting points.
