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
existing blocks are placeholders. The stage deliberately has no music: its
`sound` block names the replacement bank only. Existing action cues play;
footstep and landing audio hooks remain in `POL#23`.

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
They are not guaranteed gap distances; the route trials below measure those.

Course captures cover one through four player views at 720p, 1080p and QHD.
The retained captures, simulation checks and their limits are recorded in
[the foundation evidence](polish-foundation.md#evidence-and-limits) and
[the refinement increment](polish-refine.md). They establish a working baseline;
they do not approve camera-motion readability, controller feel or the visual
treatment.

## Retained route trials

The `movement_course` consumer acceptance case runs these trials on the authored
geometry. It runs the real Player through Level's update order in a fresh scene for
each trial, and Debug and Release produce identical logs. It asserts the intended
outcomes and records everything else as `MEASURE` lines, since the tuning is still
a candidate. Two passes were loosened to match the authored geometry: a held
leftward jump over the short gap may land on the drop-through platform, and a held
jump at a 60-unit rise may land one step higher.

### Gaps

Outcomes by jump hold, in ticks: F is the far platform, G the lower floor,
H a high recovery step and D the drop-through platform. No trial took off twice,
returned to its takeoff platform or died.

| Gap | Start | 1 | 6 | 12 | 21 | 30 | Held |
|---|---|---|---|---|---|---|---|
| Short 180, rightward | Any start | F | F | F | F | F | F |
| Short 180, leftward | Any start | F | F | F | D | D | D |
| Long 400 | From rest at the lip | G | G | G | F | F | F |
| Long 400 | 800, 120 before the lip | G | F | F | F | F | F |
| Long 400 | 800, 70 or 18 before, or a grace press | F | F | F | F | F | F |
| Running 560, rightward | From rest at the lip | H | H | G | G | G | G |
| Running 560, leftward | From rest at the lip | H | G | G | G | G | G |
| Running 560 | 800, 120 before the lip | G | G | G | F | F | F |
| Running 560 | 800, 70 or 18 before the lip | G | G | F | F | F | F |
| Running 560 | Grace press, 1 tick after leaving | G | F | F | F | F | F |

A held full-speed jump rises 207.5 and is airborne for 60 ticks. Released 20 units
past the far lip, the player stops 171–177 units beyond it.

### Running, stick input and braking

- **From rest:** full speed (800) after 48 ticks and 361.9 units, leaving 286 units
  of the left runway and 208 of the right. Release stops the player in 20 ticks
  over 121 units.
- **Stick magnitude:** raw values of 0.55, 0.65, 0.75, 0.9 and 1.0 move at 0.1, 0.3,
  0.5, 0.8 and 1.0 after the deadzone. They reach full speed after 528, 173, 102,
  62 and 48 ticks; 0.45 does not move. Speed rises with magnitude at every tick.
  The trial copies `PlayerInput`'s private pad mapping, because the engine's
  gamepad reader cannot be fed raw values.
- **Reversal (superseded baseline):** a partial opposite stick used to brake far
  more slowly than release. From 800, −0.1 took 142 ticks over 934 units and
  −1.0 took 15 ticks over 87. The initial decisions below replace that behaviour.
- **Analog drop:** a raw value of 0.79 stays on the platform and 0.81 drops.

### Recovery, staircases, ceiling and drop

- **Recovery pairs:** all three climb low step, high step, main platform, from
  both approaches to both exits. The rescue lane is the Y=1400 floor with these
  steps, and it reaches the running-gap pair without a jump from under the
  staircase and from the right runway. Holding direction through the whole exit
  jump from the short pair slides off the 400-unit short landing.
- **Staircases:** a held jump clears every rise in both directions, from rest
  and at speed. A tap clears only the 60-unit rises. A held jump at a 60-unit
  rise lands one step higher. Walking off a 120-wide step from its edge skips it
  and drops 180 to the runway.
- **Low ceiling:** a middle jump rises exactly the 40-unit clearance, bonks on
  tick 4 and lands on tick 16, with one takeoff and one landing cue. A press
  during the bonk launches on the landing only within the buffer. Running under
  it, a press within 60 units of either end clears the block; earlier presses
  bonk. The ceiling grants no ledge grace.
- **Drop-through:** the drop takes 20 ticks. A held jump or a fresh press
  produces no extra jump; a press three ticks before landing buffers one launch.
  A normal jump works afterwards.

### Timing after transitions, spawns, aim and paint

- **Ledge and buffer:** on the real 2480 lip, a press 1–5 ticks after leaving
  jumps and 6 or more does not. A press 0–7 ticks before the next landing launches
  on it. The windows are identical with no transition, after a pause and after
  death and respawn.
- **Spawns:** all eight authored spawns settle with feet at Y=1000 after three
  deaths each, with no carried input.
- **Aim:** keyboard/mouse aim holds through moving, reversing and firing, because
  the cursor direction is sent every tick. The baseline snapped released or
  deadzoned pad aim to facing; the initial decisions below replace that behaviour.
- **Paint:** every aimed tool paints the low ceiling's underside and a staircase
  wall face, each from range by flying projectiles. The roller paints the wall by
  contact and cannot reach the ceiling with its fixed aim. Bucket paint on the
  ceiling from range is small (48 units). The course's exposed paint area is
  fixed at 194,880.

## Initial movement decisions (2026-10-15)

`POL#4` is implemented at the author's request to get the behaviour in now and
tune it as the rest of the game develops. These are the current defaults; a
physical-controller session is not a prerequisite for using them.

- **Braking:** opposite input applies the full release deceleration plus
  acceleration scaled by stick magnitude. A slight reverse input therefore
  stops at least as firmly as release. After reaching zero, the remainder of
  that tick accelerates in the requested direction; braking cannot propel the
  player backwards. Held input reverses continuously without a forced stop tick.
  `player.x_deacceleration`, `player.x_acceleration` and `player.max_velocity.x`
  in `tuning.json` remain the tuning controls.
- **Released aim:** the four aimed tools retain their last explicit direction
  through deadzoned/released aim, movement, facing changes, firing, pause and
  respawn for the rest of the match. A new aim replaces it immediately. Until
  the first explicit aim, they follow facing. The roller keeps its existing
  forward/down contact angle and follows facing.
- **Short recovery landing:** accept sliding off the 400-unit platform when
  steering through the entire exit jump. Releasing direction over the platform
  remains the way to stop there; the recovery floor catches an overshoot.
- **120-wide step walk-off:** accept skipping the narrow intermediate step and
  landing 180 units lower on the runway. Descending does not snap to each tread.
- **60-unit rise overshoot:** accept a held jump reaching the next higher step;
  a tap is the short-step input. Jump height and steering remain under the
  player's control, with no automatic shortening at a step.
- **Leftward short gap:** accept the jump-through platform as the landing for
  longer holds. Shorter holds reach the runway; both are valid course routes.

The retained route case now asserts these choices, including symmetric braking
and aim retention for all five tools. The feedback timing/intensity decision
(`POL#5`) is recorded in [movement feedback](polish-feedback.md).

### Implementation checks

Debug and Release builds and all eleven consumer acceptance cases passed on
2026-10-15. The route logs match between configurations. From speed 800, both
directions now give these stopping measurements (at the fixed timestep above):

| Input after deadzone | Ticks to zero or reversal | Forward travel |
|---|---:|---:|
| Release | 20 | 121.4 |
| Opposite 0.1 | 19 | 117.0 |
| Opposite 0.3 | 18 | 108.9 |
| Opposite 0.5 | 17 | 101.8 |
| Opposite 0.8 | 15 | 92.7 |
| Opposite 1.0 | 15 | 87.3 |

The checks require stronger opposite input to stop no later or farther than
weaker input or release. Reverse speed on the crossing tick is bounded by
input-scaled acceleration; ninety further ticks must build reverse speed
without oscillation or exceeding the cap. All four aimed tools retain and
replace aim while firing after release and across an actual death/respawn.
The roller retains its fixed angle. The four accepted course outcomes above
are asserted. Tap/held jump rise remains 95/207.472 world units.

The one/four-player feedback visual probes also passed in both configurations
at 720p, 1080p and QHD, including moving cameras, aim sweeps and paused redraws.
Debug startup and extracted Release startup passed with isolated saves and
graceful shutdown. The 49-file Release ZIP contains the exact revised
presentation data and the accepted audio bank. Evidence is under
`out/acceptance/<preset>/` and `out/validation/pol4-pol5/`. These are scripted
checks, with physical-controller feel still to be reviewed under `POL#3`.

## Qualification still required

The launch, gravity and acceleration settings remain candidates until playtests.
The old movement note is not used as a specification.

- Compare keyboard/mouse with physical controllers, including partial stick
  input, stopping, reversal, aim retention and moving while firing. There is no
  physical controller on the development machine yet.
- Review landing visibility and readability during actual camera movement and
  aim sweeps at the captured resolutions and player counts. The feedback on/off
  captures in [the refinement increment](polish-refine.md) are prepared for that
  review.
- Compare the initial decisions above in play and adjust the existing tuning
  values if needed. These choices are implemented; later feel review does not
  block further game implementation.
- This course is not a balanced competitive arena. Paint area, combat pacing and
  spawn fairness have not been accepted.
