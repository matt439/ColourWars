# Movement Course

This is an experimental blockout for the movement work in
[the polish plan](polish-plan.md), available as **Movement Course** in the normal
stage selector. Choose **Single Match**, **1 Player (practice)** for solo trials.
Choose it explicitly for multiplayer trials too; `include_in_random: false`
keeps this experimental course out of **Random** matches.
It uses the existing timed match and paint rules; it is not yet the optional
untimed workshop. Existing arenas are unchanged.

The source is [movement_course.json](../game/content/levels/movement_course.json).
Open it directly in the [level editor](level-editor.md), save an experimental
copy, then rebuild to test in the game. The reused Turbulence icon and simple
existing blocks are placeholders. Music volume is zero; the required music key
still references the existing bank recipe. This is not replacement audio.

## Dimensions and route

Coordinates are world units, with positive Y downward. Surfaces below are their
top Y; spawn coordinates are the character's top-left. The current character is
52 wide and 120 high. Grey blocks are solid and paintable; pale blue platforms
allow jumping and dropping through. The dark background has no collision.

The main horizontal test surfaces are at Y=1000. Follow this route left to right,
then reverse it to compare direction and aim behaviour:

| Section | X range | Surface / purpose |
|---|---|---|
| Left spawn and runway | 0–800 | Y=1000; 800-unit approach for acceleration, stopping and reversal |
| Short gap | 800–980 | 180-unit gap; tap or hold, vary approach speed and takeoff position |
| Short landing / long takeoff | 980–1380 | Y=1000; 400-unit platform |
| Long gap | 1380–1780 | 400-unit gap; compare a jump from rest with a held running jump |
| Long landing / running takeoff | 1780–2480 | Y=1000; 700-unit runway; right edge also exercises late ledge input |
| Running gap | 2480–3040 | 560-unit gap; hold jump with a running approach; recover below a miss |
| Running landing | 3040–3980 | Y=1000; 940-unit platform, containing the low-ceiling station |
| Low step | 3980–4100 | Y=940; 60-unit rise, for short-jump comparison |
| Middle step | 4100–4320 | Y=820; 120-unit rise, for held jumps |
| High step | 4320–4540 | Y=660; 160-unit rise, then a flat 220-unit landing |
| Return steps | 4540–4760, 4760–4880 | Y=820, 940; the same 160/120 rises when approached from the right |
| Right spawn and runway | 4880–5680 | Y=1000; 60-unit rise to the first return step |

The low ceiling spans X=3460–3800, with its underside at Y=840. That leaves
160 units above the walking surface, or 40 above the standing character. Jump
under it to test immediate ceiling response, then leave its ends while holding
jump. It is intentionally an interrupted jump, not an ordinary route clearance.

The separate drop-through platform spans X=470–700 at Y=840, a 160-unit rise
above the left runway. Jump up through it, land, drop through, then jump again.
Check that drop input never turns into an extra buffered or grace jump.

## Recovery and spawns

A continuous floor at Y=1400 spans the whole course. Upper solid blocks are
40 units thick, leaving at least 360 units above this floor. It provides a safe
return lane below every missed jump and staircase. There are no pits or hazards.

Each gap contains two jump-through recovery steps, at Y=1260 and Y=1120:

| Gap | Recovery step X range | Rises back to the course |
|---|---|---|
| Short | 830–950 | 140, 140, then 120 units sideways to either upper edge |
| Long | 1510–1650 | 140, 140, then 120 units sideways to either upper edge |
| Running | 2700–2860 | 140, 140, then 120 units sideways to either upper edge |

Walk along the lower floor to a recovery pair, jump through the two steps, then
jump sideways onto a main platform. A miss may land on a recovery step directly.
The lower lane beneath the staircase returns to the running-gap recovery pair;
there is no required jump from the lower floor directly to the high staircase.

Team A's four spawns are X=100, 190, 280, 370; Team B's are X=5180, 5270, 5360,
5450. All use Y=875, putting their feet at Y=995 with a 5-unit initial settle to
the runway. Their full 52×120 rectangles fit inside the bounds, do not intersect
geometry or one another, and have 38 units between adjacent players. No ceiling
or drop platform is above a spawn. These are testing positions, not accepted
competitive spawn protection.

## Measured baseline and current evidence

The actual Player/Scene acceptance fixture produced identical Debug and Release
measurements on 2026-10-08. These are same-height jumps at a fixed timestep of
0.0166666 seconds and maximum horizontal speed of 800 world units per second:

| Input | Apex above launch | Time to land | Horizontal travel |
|---|---:|---:|---:|
| One-tick press, immediately released | 95 | 0.633331 s | 506.665 |
| Held jump | 207.472 | 1.01666 s | 813.33 |

These measured trajectories exclude acceleration from rest and input variation.
They are not guaranteed gap distances. The actual course Scene passed running
held jumps across the 180-, 400- and 560-unit gaps in both directions, with
horizontal braking beyond the destination lip. Continuous full-speed input can
overshoot a short landing and enter the next gap.

Course captures cover one through four player views at 720p, 1080p and QHD.
Spawn settling passed in the exercised one-to-four-player layouts. The retained
captures, simulation checks and their limits are recorded in
[the foundation evidence](polish-foundation.md#evidence-and-limits). They establish
a working baseline; they do not approve camera-motion readability, controller
feel or the visual treatment.

## Qualification still required

The launch, gravity and acceleration settings remain candidates until playtests.
The 60/120/160-unit staircase rises fit within the measured vertical envelope,
but their approaches and landings still need traversal trials. The old movement
note is not used as a specification.

- Measure from-rest travel and test varied takeoff positions and input durations.
  Repeat gaps using taps and partial controller input as well as held jumps.
- Traverse every recovery pair and staircase in both directions, from rest and
  at speed. Exercise course ceiling contact, platform drops, buffered landings
  and late ledge presses, including after pause, death and respawn.
- Compare keyboard/mouse with physical controllers, including partial stick
  input, stopping, reversal, aim retention and moving while firing.
- Review landing visibility and readability during actual camera movement and
  aim sweeps at the captured resolutions and player counts.
- Test repeated spawns, all five tools, exposed wall/ceiling painting and the
  rescue lane. This course is not a balanced competitive arena; paint area,
  combat pacing and spawn fairness have not been accepted.

Editor load/save and startup checks establish supported data and resource use.
They do not establish controller feel, route reliability or visual polish.
