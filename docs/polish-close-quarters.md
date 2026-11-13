# Close Quarters: first playable blockout

Status: first playable layout increment, 2026-10-20 (`POL#17`). The current
movement and weapon tuning stay unchanged. This increment opens movement
comparison; attended reference-arena acceptance and further layout iteration
remain in `POL#17`.

The [development order](polish-plan.md#development-order-2026-10-17) puts this
layout before movement tuning, followed by small level and movement adjustments.
The author approved the current gameplay art for this stage on 2026-10-20;
the new composition still needs review. The [arena requirements](polish-plan.md#design-requirements)
remain the acceptance criteria.

## Purpose and starting point

Make the existing wide, open Close Quarters stage into a connected contest with
an exposed main route, a lower bypass, an upper route and sheltered home rooms.
Retain the 6,000-unit world width and the approved Tool Props treatment so the
first change can be judged as a layout change.

The previous layout had one 3,000-unit scored slab at Y=4950, a 750-unit upper
slab at Y=4600, narrow access platforms and long unscored approaches. The nominal
scored area was 91,200 square world units: 72,000 on the main slab's top and
underside, and 19,200 on the upper slab. There was no scored lower walking lane.
Its tiny spawn pockets offered little choice in approaching the contest.

This pass keeps both sides mirrored. Home surfaces remain unscored; useful paint
is reached through the shared routes. The lower lane receives walkable paint so
the bypass has a reason to be used and the roller has a continuous scoring route.

## Geometry and routes

Coordinates are world units, with positive Y downward. A rectangle below is
`x, y, width, height`; route heights are the top of its support. Right-side rooms
mirror the left with `right_x = 6000 - left_x - width`.

| Shared geometry | Rectangle | Route purpose |
|---|---|---|
| Main deck | `1500, 4970, 3000, 50` | Broad central walking and painting route. |
| Lower floor | `1500, 5250, 3000, 1000` | Scored lower bypass; joins unscored outer floor at both ends. |
| Lower access platforms | `1200, 5110, 300, 10` and `4500, 5110, 300, 10` | Jump-through recovery from the lower floor to the main deck. |
| Upper access platforms | `2200, 4830, 300, 10` and `3500, 4830, 300, 10` | Jump-through approaches from both sides of the main deck. |
| Upper deck | `2500, 4690, 1000, 50` | Contestable height with approaches and an escape on either side. |
| Centre eraser | `2880, 4890, 240, 80` | Low solid cover interrupts the main lane's straight firing line. |

The main deck and lower floor reconnect around both ends of the slab. Both
lower access climbs are `5250 → 5110 → 4970`: two 140-unit rises, with 300-unit
landings. The lower passage has 230 units of clearance beneath the main deck.
It offers cover from the main and upper routes, while its end approaches remain
open to attack.

The upper route is `4970 → 4830 → 4690`, also two 140-unit rises in either
direction. Players can retreat or drop from either edge. The centre eraser is
an 80-unit rise, below the measured tap-jump apex. Its top has 150 units of
clearance beneath the upper deck; a standing 120-unit player fits. Its cover
cannot secure the lower bypass as well as the main route.

The old narrow vertical jump-through fences are removed. Solid cover is drawn
as solid terrain; jump-through access platforms retain the ruler treatment.

## Sheltered home rooms

Each room has a raised exit over a solid baffle and a lower exit beneath it.
Neither exit depends on controlling the other. All room and approach surfaces
are unscored.

| Left-side geometry | Rectangle | Purpose |
|---|---|---|
| Home floor | `0, 5000, 850, 10` | Jump-through support and drop exit. |
| Home roof | `0, 4700, 400, 40` | Solid overhead shelter above all four spawn positions. |
| Inner baffle | `850, 4740, 100, 300` | Solid screen between the home and central firing positions. |
| Inner upper step | `650, 4860, 200, 10` | Jump-through approach to the baffle. |
| Outer upper step | `950, 4860, 200, 10` | Jump-through landing and return approach. |
| Lower recovery platform | `350, 5140, 300, 10` | Return from the lower floor to the home floor. |
| Approach bridge | `1150, 4930, 350, 10` | Jump-through connection from the upper exit to the main deck. |

Left spawn positions are `(30, 4880)`, `(120, 4880)`, `(210, 4880)` and
`(300, 4880)`. Each 52-by-120 player begins with feet at Y=5000. Their mirrored
right-side X coordinates are 5918, 5828, 5738 and 5648, at the same Y. Adjacent
players have 38 units of separation. Each side retains four valid positions,
including configurations with fewer active players.

The upper exit climbs `5000 → 4860 → 4740`: rises of 140 and 120. The far
step, bridge and main deck descend through `4860 → 4930 → 4970`. Returning
from the main route needs a 40-unit hop to the bridge, then a 70-unit rise to
the outer step. No solid upper deck overlaps the baffle ascent.

Dropping through the home floor reaches the continuous lower floor. A drop
above the recovery platform can land there first; another drop or movement
off its edge continues downward. Dropping beside its footprint reaches the
lower route directly. The return climb is `5250 → 5140 → 5000`, with rises of
110 and 140. Its 130-unit clearance beneath the home floor keeps a standing
player clear of the one-way artwork. The baffle ends at Y=5040, leaving
210 units above the lower floor.
The roof leaves 260 units above the home floor. The approach bridge leaves
170 units above the adjacent lower access platform.

The baffle is taller than a direct held jump from the home floor, so its
paired steps make the raised route deliberate. The lower exit avoids that
ascent. Whether opponents can repeatedly contain both exits remains a match
question; the geometry alone does not establish spawn safety.

## Movement, tools and supply

The [current movement measurements](polish-movement-course.md#measured-baseline-and-current-evidence)
give a 95-unit tap apex and a 207.472-unit held apex. At full speed, the
same-height held jump travels 813.33 units in 1.01666 seconds; acceleration
from rest reduces that travel. The retained course trials qualify 180-unit
gaps across tested starts and hold times. This layout uses broad supports and
required climbs of at most 140 units rather than requiring those long jumps.

Current full-speed stopping distance is about 121 units. The 300-unit access
platforms provide room to release direction; the narrower baffle top is a
crossing between two steps, not a required stopping position. Required routes
must be exercised in both directions with the actual Player and Scene.

The sniper's straight, long-lived jet makes unobstructed firing lines a layout
concern. The home baffles and centre eraser supply physical occlusion. The upper
deck has two approaches and gives up the sheltered lower route. Nearby floor
and wall contacts should give the sprayer, mister and bucket useful encounters;
their actual firing arcs and exposure still need checks at current values.

The roller's fixed downward aim makes connected walking paint valuable. The
lower scored floor adds that opportunity without requiring ceiling paint. All
five tools must be exercised before making weapon-role claims or changing their
numbers under `POL#27`.

There are no supply pickups in current gameplay. Ammunition recovers after 0.5
seconds without a paid shot, at 0.4 tank per second: about three seconds from
empty to full on unpainted floor. Since paint underfoot shipped on 2026-11-05
(`POL#30`), own paint recovers 1.25 times as fast and enemy paint not at all.
The home rooms, baffle sides and lower bypass offer
places to stop firing and recover. Whether those pauses are too safe, too long
or easy to deny belongs in the pacing observations.

## Paint budget

Score exposed reachable faces, including ceilings and walls. The builder's
buried-face exclusion must remove contacts between the centre eraser and main
deck and between adjacent floor pieces. Ramps, if added in later iterations,
retain their existing paint-pass behaviour.

The central 3,000-unit floor is paintable on its top only; the two outer floor
pieces and home rooms remain unpainted. The main deck, upper deck and centre
eraser enable all four faces, with buried intervals removed by the real builder.
The eraser's bottom and the main-deck top beneath it must not count. A full
rectangular perimeter is not an exposed-area measurement.

| Region | Intended scoring role | Verified exposed area |
|---|---|---|
| Left shared approach, X=1500–2500 | Main-deck and lower-floor paint reached from either route. | 36,600 (26.85%). |
| Centre, X=2500–3500 | Upper deck, central main/lower routes and centre cover make this the largest single contested region. | 63,120 (46.30%). |
| Right shared approach, X=3500–4500 | Mirror of the left shared approach. | 36,600 (26.85%). |
| Home rooms and outer approaches | Shelter, orientation and return to play. | 0. |

Walkable top faces total **84,000 (61.62%)**; undersides 48,000 (35.21%);
vertical faces 4,320 (3.17%). Left and right budgets match. The centre eraser
removes 240×12 units from the main top; its own buried bottom scores nothing.
The floor joins and covered home surfaces add no hidden score. These measurements
reconcile per object with runtime tiles and partition the total by region.
They establish distribution, rather than equal opportunity during a match.

## Framing and presentation

Retain the 6,000-unit world frame. The overview spans X=0–6000 and
Y=4300–5350 so both rooms, the upper route and lower walking lane appear.
Camera bounds are `-50, 4300, 6100, 1050`; the opening starts at
`0, 4300, 1920, 1080`. Out-of-bounds remains `-100, -100, 6200, 6200`.
Current movement, weapon and presentation tuning are unchanged.

Fit the approved terrain kit to these collision shapes and re-check the
composition in one-, two-, three- and four-player views. Roofs, platforms,
cover and landing edges must remain recognisable when heavily painted.
The brush pot and tape roll move 20 units down with the main deck, preserving
their previous footing and native dimensions. Decorative art must not imply
collision.

## Verification and remaining acceptance

The first increment was validated on 2026-10-20 in the working tree based on
`938440e`; receipts retain the source hashes and dirty-tree state. Evidence root:
`out/validation/cq-blockout-20261020/0fb186ee-4deb-406d-acf7-35bb8c8cbbac/`.

- Debug and Release built under `/W4 /WX`. All 39 consumer cases passed in
  each preset (`acceptance-final/`). After the final recovery-platform and
  landmark fit, focused `cq_blockout` and `terrain` checks passed again in
  `acceptance-fitted/`, with fresh compile outputs and logs.
- The real Player/Scene case reconciles 136,320 exposed runtime area with
  object, region and face-direction calculations. It checks all eight spawns,
  two deaths/respawns each, held-input reset, driven lower departures, both
  representative upper departures/returns, and main/upper loops in both
  directions with +/-2 input ticks. Fourteen centred standing clearances
  include one-way artwork. The pre-fit overlapping recovery platform is
  retained as a failing negative control.
- All five tools make paid paint contact on the main deck. The four aimed
  tools also launch and sweep into the centre cover from both sides, with
  swept areas of 192 / 192 / 384 / 624 for sprayer / sniper / mister / bucket.
  This is bounded contact evidence, not effective-range or role balance.
- Both presets passed the focused D3D11 probe on the RTX 5080:
  `native-fitted/x64-debug/6e37327c46e64f52bbd29df970d676ff/result.json` and
  `native-fitted/x64-release/a26d092a334d4571bedea4a3e24c6a32/result.json`.
  Each records five 720p frames, eight native crops and 23 readbacks, including
  actual overview and StagePreview, with frozen pixels/state/RNG and the Debug
  replay fingerprint. Hidden captures establish rendering evidence only.
- Debug and Release startup passed with private saves, required audio,
  per-monitor-v2 DPI and graceful shutdown. Release's empty captured ammo log
  was checked by rerunning the same binary directly; its passing terminal
  output and exit code are retained alongside the acceptance receipt.

Route timings include deliberate checkpoint stops: core loops remain below
317 ticks (5.28 s), main loops take 517/519/521 ticks and upper loops
612/616/620. They are feasibility timings, not observed match pacing.

The first playable increment needs:

- Valid content and successful Debug/Release builds under the existing warnings.
- All eight spawns settling with full clearance, including repeated respawns.
- Actual Player/Scene travel from each home into the main, lower and upper
  routes, with return paths in both directions and no required inaccessible rise.
- Exposed-paint measurements from the actual authored stage, including floor
  joins and centre-cover clipping, plus left/right symmetry.
- Current-value checks for all five tools and captures that expose the new
  collision silhouettes at playing size.

The author subsequently clarified the order: [research comparable levels](level-design-research.md),
create an original [new map](polish-crossfeed.md), then tune movement and all
five weapons together there. This CQ increment is preserved as a comparison;
its passing routes do not establish that its long lanes, spacing, required
inputs or weapon scale are suitable. Further CQ redesign is not a substitute
for the requested new map.

Full `POL#17` acceptance still needs attended two- and four-player matches,
separate 1v1 and 2v2 qualification, three-player exercise, side swaps with
comparable loadouts and repeated respawns under pressure. Record time to first
contest and return from spawn, safe painting stretches, repeated chokes,
disengagement, upper-route counterplay and any tool or side advantage. Scripted
routes and bots can reveal failures; they do not establish comfort or balance.
Physical-pad acceptance (`ROAD#2`) and the reference-match finish review and
fresh-player observations (`POL#24`) were waived by the author on 2026-11-05
and not performed. The other arenas followed Crossfeed's acceptance under
`POL#21`, which the author closed on 2026-11-05.

## Arena art (`POL#16`, 2026-11-14)

### Brief

**Play identity.** A compact loop of connected rooms: two sheltered home bays,
a long main deck with a low central cover, an upper crossing and a lower bypass.
It is in Random, so it needs a place of its own rather than the generic
countryside it borrowed from the kit, which said nothing about rooms.

**Environment.** The paint workshop and supply warehouse of the
[initial brief](polish-plan.md#initial-arena-briefs), drawn as an interior:
a back wall under a roof, the sky seen only through high windows. Each home
room becomes a loading bay with a roller door; the central contest happens in
front of a paint tank fed by a pipe run; racks of tins stand where the
approach bridges meet the deck; the lower bypass is the workshop floor, with a
bench, leaning timber and drums.

**Props** (all new, all `cq_*`, in the recipe after the shared kit):
`cq_window`, `cq_door`, `cq_rack`, `cq_tank`, `cq_pipes`, `cq_bench`,
`cq_timber` and `cq_drums`. Each is a crude flat drawing on the cell grid in
the backdrop roles only (bg_far, bg_mid, bg_line, bg_detail, paper), outlined
in bg_line. Tins and drums carry blank bg_detail labels: no text, and no team
hue anywhere.

**Layering, far to near.** Paper; the works wall (`swatch_bg_far`) with a
bg_mid roof beam edged in bg_line above the camera's top; the six high
windows; the pipe run, then the tank in front of its drop; the two loading
doors; the two racks; the two timber stacks and two drum groups; the bench;
then the shared brush pot and tape roll, still last, at their accepted
footings.

**Material palette.** The Muted Day roles are unchanged. The arena's own mix
is its props plus one structure material: the centre cover changes from
`eraser` to `paint_tin` (legal for a paintable solid, already drawn on Press
Tower's 320x80 press head), so the central contest is fought over a paint tin.
The pencils, rulers and ground stay. Paintability comes from the faces, not
the material, so the scored area is unchanged.

**What stays from the shared kit.** The paper backing, the brush pot and the
tape roll (same rectangles), the airbrush shadows and the terrain materials.
The sun, six drifting clouds, two bird pairs, seven hills, the house and four
trees leave: an interior has no horizon. The window panes reuse their
language instead: a cloud, a gull and a hill line in miniature.

### What was built

Eight props in [the terrain recipe](../game/art/terrain/terrain.json),
described in the [kit's README](../game/art/terrain/README.md); the build
packs them on the landmark shelves and the atlas grows to 2048x1228 on this
branch. Close Quarters now has 24 decorations: paper, the works wall and roof
beam (three swatches), six windows at Y=4332, the pipe run (2388, 4300), the
tank on the upper deck (2872, 4370), the loading doors behind the home rooms
(420 and 5196, 4600), the racks standing on the approach bridges (1200 and
4512, 4546), timber in the left bay (960, 5026) and the right of the lower lane
(3740, 5026), drums under each home floor (40 and 5704, 5074), the bench in the
lower lane (1900, 5122), then the brush pot and tape roll. The `_a`/`_b` pairs
are exact mirrors. No collision geometry, spawn, station or bound moved.

Composition was iterated in 1P, 4P and the whole-arena time-lapse view at
seeds 7, 0 and 3. Shelf boards are kept off the heights of the real steps
beside them and crowded so no shelf top reads clean; the bench is low and
cluttered; nothing is lined in black. The roof beam sits above the camera's
top, so only the overview and time-lapse show it, as the edge of the building.

### Checks

The tests describe the new intended state rather than being loosened. The
terrain probe pins the new layering (landmarks at index 22 of 24 after the
bench) and the 2048x1228 atlas, and counts Close Quarters as an interior
alongside Press Tower: no clouds, no drift. The editor's drift-phase mirror
example moved to the Movement Course's identical `cloud_4`. The painter's
parallax prototype keeps its exact matrix and counts, with Close Quarters as a
zero-displacement interior control. Its cloud-drift contexts drop Close
Quarters and the 4P sky composition built around its `cloud_4`.

Debug acceptance passed for `terrain`, `cq_blockout`, `stage_selection`,
`level_format`, `paint`, `user_stages`, `debug_console`, `instant_replay`,
`match_records`, `team_selection`, `story_audio` and `menu_text`. Runtime
exposed paint stays **136,320**, as before the change. `check-level-editor.ps1`
and `check-painter.ps1 -CompileOnly` passed, as did the whitespace lint and
`check_doc_links.py`. The editor run's first attempt failed one native
keyboard-state check while game windows were open on the same desktop; it
passed on a rerun with no game running.

### Human checks still open

How it looks on a real GPU (the painter's landmark, parallax and drift
matrices were compiled, not run), how it plays (whether a rack shelf or the
bench ever reads as a step in a fight) and the author's view of the interior
direction and the paint-tin centre cover. The atlas height will change again
when the other arenas' props merge.
