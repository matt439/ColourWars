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
copy, then rebuild to test in the game. Its reused Turbulence `icon_frame` is
legacy metadata that no screen draws; Stage Select uses the live arena preview
([`ED#6` retired](level-editor.md#movement-course-icon-ed6-retired)). The stage
deliberately has no music: its `sound` block names the replacement bank only.
Action cues, including footsteps and landings, use the accepted replacement set;
optional detail and in-game mix listening remain in `POL#23`.

## Dimensions and route

Coordinates are world units, with positive Y downward. Surfaces below are their
top Y; spawn coordinates are the character's top-left. The current character is
52 wide and 120 high. Grey blocks are solid and paintable; pale blue platforms
allow jumping and dropping through. The dark background has no collision.

The main horizontal test surfaces are at Y=1000. Follow this route left to right,
then reverse it to compare direction and aim behaviour:

| Section | X range | Surface / purpose |
|---|---|---|
| Left spawn and runway | 0-830 | Y=1000; 830-unit approach for acceleration, stopping and reversal |
| Short gap | 830-950 | 120-unit gap; a tap from rest reaches the far runway |
| Short landing / long takeoff | 950-1440 | Y=1000; 490-unit platform |
| Long gap | 1440-1720 | 280-unit gap; a held jump from rest crosses, a tap does not |
| Long landing / running takeoff | 1720-2560 | Y=1000; 840-unit runway; right edge also exercises late ledge input |
| Running gap | 2560-3000 | 440-unit gap; use a running approach and hold; recover below a miss |
| Running landing | 3000-3980 | Y=1000; 980-unit platform, containing the low-ceiling station |
| Low step | 3980-4100 | Y=940; 60-unit rise, for short-jump comparison |
| Middle step | 4100-4320 | Y=820; 120-unit rise, for held jumps |
| High step | 4320-4540 | Y=660; 160-unit rise, then a flat 220-unit landing |
| Return steps | 4540-4760, 4760-4880 | Y=820, 940; the same 160/120 rises when approached from the right |
| Right spawn and runway | 4880-5680 | Y=1000; 60-unit rise to the first return step |

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

The provisional 2026-10-20 global compatibility profile, **P3**, uses gravity **3000**,
launch velocity **-730**, held-jump acceleration **-2250** and maximum hold time **0.22 s**.
Horizontal acceleration, braking, maximum speed, buffer and ledge grace are unchanged.
The actual Player/Scene Debug fixture measured these same-height jumps at a fixed
0.0166666-second timestep and maximum horizontal speed of 800 world units per second:

| Input | Apex above launch | Time to land | Horizontal travel |
|---|---:|---:|---:|
| One-tick press, immediately released | 95 | 0.516665 s | 413.332 |
| Held jump | 207.375 | 0.799997 s | 639.997 |

Receipt: `out/validation/crossfeed-20261020/compatibility-v1/candidates/compatibility-coupled-movement.out`.
The editor guide records the same actual measurements in
[`editor/movement-baseline.json`](../editor/movement-baseline.json).
These trajectories exclude acceleration from rest and input variation; they are not
guaranteed gap distances. The route trials below measure the authored geometry.

For comparison, the **2026-10-08 historical baseline**, measured identically in Debug
and Release, was tap 95 / 0.633331 s / 506.665 travel and held 207.472 / 1.01666 s /
813.33 travel. P3 keeps the tap height and almost the same held apex while shortening
tap airtime by about 0.117 s and held airtime by about 0.217 s. It addresses the
measured airborne duration; the author's **too-high held jump remains unresolved**.

The earlier P1/P2 lower-jump profile, 3000/-730/-1800/0.22, measured tap 95 and held
177.5, with held airtime 0.749997 s and travel 599.997. It works in Crossfeed's
160-unit recovery scheme, but cannot clear thirteen sampled 200-unit climbs in
the existing arenas. It is retained as an experiment rather than a global default.
P3 restores the reach those authored climbs require: all fifteen legacy probes
pass with P0 and P3, while P2 fails the thirteen 200-unit climbs. The existing arena
JSON is preserved. Final active Debug/Release consumer checks, Crossfeed native
captures and Release package checks pass; see the
[final P3 record](polish-crossfeed.md#selected-p3-profile-2026-10-20).
Course camera/comfort qualification remains separate below.
The three-profile receipts are in
`out/validation/crossfeed-20261020/compatibility-v1/legacy-candidates/movement-results.json`.

Live tuning applies globally. Reducing the held apex further needs coordinated
route work or a separately scoped movement-profile feature, followed by playtests.

The current candidate passes 80 ramp walking routes and 12 slope-roof approaches
and escapes. A 2600-gravity comparison also passed that movement fixture; a faster
3600-gravity comparison stalled at a leftward quarter-input ramp toe. That profile
is excluded because its seam qualification failed, not because anyone judged its
comfort. These checks record runtime qualification; comfort still needs play.

The course was re-authored during the lower-jump comparison: its old 180/400/560
gaps are now 120/280/440. Extending four main platforms preserves the spawns, low ceiling,
staircase and return floor. The 440-unit gap is centred on its recovery pair at
X=2780, giving equal 220-unit distances from the pair's centre to the upper edges.
The 140/140/120 recovery rises remain after actual route qualification. Debug
`goto` stations follow the revised takeoffs: `short_gap` X=738, `long_gap` X=1348
and `running_gap` X=1740; these are character top-left coordinates.

Earlier course captures cover 1-4P at 720p, 1080p and QHD in
[the foundation evidence](polish-foundation.md#evidence-and-limits) and
[the refinement increment](polish-refine.md). They predate this jump and gap revision.
They do not qualify revised landing visibility, camera motion, controller comfort
or competitive balance.

## Route trials (2026-10-20 provisional profile)

The `movement_course` consumer case builds a fresh scene from the authored geometry
for every trial and runs the real Player through Level's update order. The P3 tables
below come from the complete Debug observation at
`out/validation/crossfeed-20261020/compatibility-v1/candidates/compatibility-coupled-movement_course.out`
(with `movement_course-results.json` and the same-named `.err` beside it).
That run retained the fundamental traversal, collision,
recovery, spawn, input and five-tool checks, while observing three old geometry-specific
expectations: the short gap's upper landing, continuous recovery overshoot and
skipping a stair tread. The active case now asserts the new route roles described
below. Both active Debug and Release suites now pass these requirements;
the [final P3 record](polish-crossfeed.md#selected-p3-profile-2026-10-20)
maps their retained receipts.

### Gaps

Outcomes by held ticks: F is the far main platform, G the lower floor, H a high
recovery step and D the drop-through platform. Each trial took off exactly once;
none returned to its takeoff platform or died. Unqualified rows apply both ways.

| Gap | Start | 1 | 6 | 12 | 21 | 30 | Held |
|---|---|---|---|---|---|---|---|
| Short 120, rightward | Any start | F | F | F | F | F | F |
| Short 120, leftward | Rest at the lip | F | F | F | F | F | F |
| Short 120, leftward | Any running start or grace +1 | F | F | D | D | D | D |
| Long 280 | Rest at the lip | H | G | F | F | F | F |
| Long 280 | Any running start or grace +1 | F | F | F | F | F | F |
| Running 440 | Rest at the lip | H | H | G | G | G | G |
| Running 440 | 800, 120 or 70 before the lip | G | G | F | F | F | F |
| Running 440 | 800, 18 before the lip | G | F | F | F | F | F |
| Running 440 | Grace press, 1 tick after leaving | F | F | F | F | F | F |

The requirements are the route roles, rather than every sampled hold threshold:
a short tap from rest crosses; the long gap distinguishes a rest tap from a held
jump; the running gap requires a run-up for a fully held jump. Running held trials
land and brake beyond every far lip. A leftward moving jump can choose the upper
drop platform, which is not reached from rest with this profile. Intermediate hold
thresholds remain measurements because takeoff position changes them.

A full same-height held jump lasts 48 simulation updates (the route report's
zero-based landing index is 47). The earliest running-gap press, 120 before the
lip, lands with its centre 54 units beyond the far lip and stops 175.4 beyond it.
Other held running trials stop roughly 170.7-175.4 beyond their far lip under the
fixture's braking rule.
The early-press margin is a limitation to review with real inputs, not a
claim that all takeoff timing is comfortable. Grace +1 can cross the running gap
with a tap because the player has already travelled past the lip before pressing.

### Running, stick input and braking

- **From rest:** full speed (800) after 48 ticks and 361.9 units, leaving 316.1
  units of the left runway and 208.1 of the right in the measured spawn trials.
  Release stops the player in 20 ticks over 121.4 units.
- **Stick magnitude:** raw values 0.55, 0.65, 0.75, 0.9 and 1.0 map to movement
  0.1, 0.3, 0.5, 0.8 and 1.0 after the deadzone. They reach full speed after
  528, 173, 102, 62 and 48 ticks; 0.45 does not move. Speed is monotonic with
  magnitude. This fixture copies `PlayerInput`'s private pad mapping because
  the concrete engine gamepad reader cannot accept injected raw values.
- **Reversal:** partial opposite input brakes at least as firmly as release;
  the historical slower partial-stick braking was replaced on 2026-10-15.
  The unchanged measured stopping table is retained below.
- **Analog drop:** raw Y=0.79 stays on the platform and 0.81 drops.

### Recovery, staircases, ceiling and drop

- **Recovery:** all three pairs climb low step, high step and main platform,
  from both approaches to both exits. Both controlled steering and steering
  throughout the exit jump now stop on the target, including the wider short
  landing. The lower lane returns from beneath the staircase and the right
  runway to either exit of the running-gap pair without a required floor jump.
- **Staircases:** held jumps clear the 60/120/160 rises both ways from rest and
  at speed. Taps clear only 60. Tap and held inputs both land and stop on that
  first tread. Even with P3's restored apex, its shorter airtime does not produce
  the old combined 180-unit skip in these trials. Walking off
  the middle tread still skips the 120-wide lower tread and drops 180 to the
  runway. Running from the high step reaches the runway without a jump.
- **Low ceiling:** the middle jump rises exactly 40 units, bonks at zero-based
  tick 3 and lands at tick 13, with one takeoff and one landing cue. Presses
  0-7 ticks before landing buffer a launch; 8-9 do not. Running presses 60
  units before either far end still bonk; presses 20 or 4 before it clear.
  Those clear rightward trials land on `short_step`, while leftward trials
  land on `running_landing`. These differences follow the course geometry.
  The ceiling grants no ledge grace.
- **Drop-through:** the drop takes 17 ticks. Holding jump or pressing on the
  following tick produces no extra jump; a press three ticks before landing
  buffers one launch. A normal jump works afterwards.

### Timing after transitions, spawns, aim and paint

- **Ledge and buffer:** at the actual X=2560 lip, presses 1-5 ticks after leaving
  jump and 6 or more do not. Presses 0-7 ticks before the next landing launch on
  it; 8 does not. The next landing is `running_gap_recovery_high`. Windows match
  without a transition, after pause and after death/respawn.
- **Spawns:** all eight settle with feet at Y=1000 after three deaths each,
  without carried input.
- **Aim:** all four aimed tools retain and replace released/deadzoned pad aim
  through movement, reversal, firing, pause and respawn. Keyboard/mouse sends
  cursor aim every tick. The roller keeps its fixed forward/down direction.
- **Paint:** each aimed tool paints the ceiling underside and staircase face
  from outside cover using actual flying projectiles; the roller paints the
  wall by contact and cannot reach the ceiling. Bucket underside paint from
  outside remains small (48 scored units). The course's expected exposed paint
  area is **202,080**, up 7,200 from the old geometry: 300 added platform-width
  units contribute exposed top and underside strips at thickness 12. Final
  runtime/editor coverage confirmation belongs to the Debug/Release batch.

## Initial movement decisions (2026-10-15)

`POL#4` was implemented at the author's request to get the behaviour in and
tune it as the rest of the game develops. This dated record preserves those
decisions; the 2026-10-20 route trials above supersede geometry-specific outcomes.
Physical-controller review remains open.

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
- **Short recovery landing (superseded 2026-10-20):** the old 400-unit platform
  accepted a slide onto the recovery floor when steering throughout the exit.
  The current shorter airtime and 490-unit landing stop on the target instead.
- **120-wide step walk-off:** accept skipping the narrow intermediate step and
  landing 180 units lower on the runway. Descending does not snap to each tread.
- **60-unit rise overshoot (superseded 2026-10-20):** the old held jump accepted
  reaching the next step. The current envelope lands on the 60-unit tread with
  either input; no automatic shortening is used.
- **Leftward short gap:** the upper jump-through landing remains a valid choice
  for moving jumps. The 2026-10-20 table records its dependence on approach and
  hold; from rest the current profile lands on the runway.

The route case retains symmetric braking and aim retention for all five tools,
with the current geometry contracts stated above. The feedback timing/intensity decision
(`POL#5`) is recorded in [movement feedback](polish-feedback.md).

## Keyboard/mouse playtest (2026-10-17)

The author's first `POL#3` session, on keyboard and mouse: **the jump is a bit too
floaty, and it goes too high.** Braking, aim and the other initial decisions above
drew no comment. **Nothing is locked in from this:** jump constants depend on
level design and art, so they stay provisional and are retuned as those develop.
The author clarified the order on 2026-10-20: research comparable combat levels,
create an original new map (`POL#17`) using the current values, then tune movement
and all five weapons together (`POL#3`, `POL#27`) against that map and this course.
The [research criteria](level-design-research.md) and [Crossfeed brief](polish-crossfeed.md)
own that comparison. The CQ experiment is preserved. The current
gameplay style is approved for this stage; movement values and art remain open to iteration
([development order](polish-plan.md#development-order-2026-10-17)).

The current Crossfeed comparison has four repeatable Debug scripts. Launch with
`ColourWars.exe +exec crossfeed_baseline`, then use `exec crossfeed_movement`,
`exec crossfeed_lower` and `exec crossfeed_coupled` in the console to compare on the same map:

- [`crossfeed_baseline`](../scripts/debug/crossfeed_baseline.cfg) selects P0:
  the historical 2000-gravity jump, Sniper cost 0.25, Mister cost 0.01 and mist lifetime 5 s.
- [`crossfeed_movement`](../scripts/debug/crossfeed_movement.cfg) selects P1:
  the experimental 3000/-730/-1800/0.22 lower jump with those original tool values.
- [`crossfeed_lower`](../scripts/debug/crossfeed_lower.cfg) retains P2:
  that lower jump, Sniper cost 0.30, Mister cost 0.02 and mist lifetime 2 s.
- [`crossfeed_coupled`](../scripts/debug/crossfeed_coupled.cfg) selects P3:
  the P3 compatibility jump, 3000/-730/-2250/0.22, with the same coupled tool values.

Each enters a one-player Crossfeed match, resets tuning to loaded file values,
then explicitly sets all seven comparison fields. **These are global live edits,
not per-stage profiles.** P1/P2 cannot clear the legacy 200-unit climbs: use
`tune.reset tuning` before entering another arena, or select `crossfeed_coupled`
to restore the P3 comparison. `tune.diff tuning` inspects live edits;
`tune.reset tuning` restores loaded defaults. Saving requires an explicit
`tune.save tuning` command and changes the shared startup values. These scripts
support attended comparison; they do not establish comfort or combat acceptance.

The four original movement-only scripts in `scripts/debug/` retain the 2026-10-17 comparisons:
`exec jump_baseline`, `jump_mid`, `jump_low` and `jump_snappy`. In particular,
`jump_baseline` selects the old 2000-gravity profile, and `jump_mid` uses launch
-700 and hold acceleration -1800 rather than the current -730/-2250; none of these
four original scripts names the current profile exactly.
`tune.save tuning` writes a candidate when deliberately chosen. The older envelopes
below describe same-height jumps from the real integrator at 60 Hz; the horizontal
figure is airtime at 800 units per second.

| Script | gravity | launch | hold accel | hold time | Held apex | Airborne | Tap apex | Run travel |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `jump_baseline` | 2000 | −600 | −1300 | 0.35 | 207 | 1.02 s | 95 | 813 |
| `jump_mid` | 3000 | −700 | −1800 | 0.22 | 166 | 0.72 s | 88 | 573 |
| `jump_low` | 3200 | −650 | −2200 | 0.20 | 152 | 0.68 s | 72 | 547 |
| `jump_snappy` | 3600 | −750 | −2400 | 0.18 | 163 | 0.65 s | 85 | 520 |

The course moved with the jump on 2026-10-20: gaps were shortened as recorded
above, while the 140-unit recovery rises survived actual trials. Future changes
must likewise remeasure the route case and reauthor any unreliable required route.

### Implementation checks

Debug and Release builds and all eleven consumer acceptance cases passed on
2026-10-15. The route logs match between configurations. From speed 800, both
directions gave these stopping measurements, unchanged in the 2026-10-20 observation:

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
The roller retains its fixed angle. At that date the course used the old
95/207.472 tap/held apex; the 2026-10-20 table above records the current profile.

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
- Compare the current profile and input decisions in play and adjust the existing
  tuning values if needed. These choices are implemented; later feel review does not
  block further game implementation.
- The shorter airtime is measured, but P3 retains the old held apex to preserve
  the existing 200-unit climbs. The author's height feedback remains open; P2 is
  a lower-jump comparison, with no per-stage isolation in the implementation.
- This course is not a balanced competitive arena. Paint area, combat pacing and
  spawn fairness have not been accepted.
