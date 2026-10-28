# Feature roadmap

The next presentation, game-feel, audio and arena-design work is defined in the
[polish and level-design plan](polish-plan.md); [TODO.md](../TODO.md) gives its
order and status. The art direction is Paint.exe Maximalism (crude, mouse-drawn
box-head figures), chosen on 2026-10-09 over the plan's original pixel art, and
its fiction is the [backstory](backstory.md): a grey, unsaved picture with one
palette slot left, which two team colours fight to fill. Gameplay style comes
first, UI after; the author accepted all five round-2 art choices on 2026-10-15
(polish plan §1). The rest of the direction is distinctive paint tools and redesigned
playable layouts, with a full replacement of all music and sound effects and
expressive feedback for actions such as jumping and landing. The target is a
good indie game's finish, including quiet details such as an occasional idle
muzzle drip, tool settling and environmental life. Start with one complete
reference match and review its feel in motion. That work is planned, apart from
the backstory's opening, respawn and reveal, which its "Building it" section
records as built, including the accepted pop, sigh and chair-creak sounds.
Existing live audio bindings now use the accepted replacement set; further cue
hooks and the gameplay mix remain open. The sections below record implemented
increments and their remaining acceptance checks.

## Match scope decisions (2026-10-15)

The author chose timed painting as the sole ruleset (`GAP#1.2`). Single Match,
Best of 3 and Best of 5 remain session formats for that ruleset. No further
mode or speculative `MatchRules` seam is planned; a future ruleset would need
its own product decision.

Solo remains practice (`GAP#1.3`), with coverage results and no opponent. The
accepted solo story treatment is unchanged: no person, opening or reveal, as
recorded in the [backstory](backstory.md#building-it). The original exclusion of
shipping AI opponents was superseded by the author's 2026-10-27 request below;
the existing Debug bots remain developer tools.

### AI bot players (ROAD#10)

**ROAD#10 - OPEN (requested 2026-10-27).** Add playable AI teammates and
opponents through normal match setup in Debug and Release, within the existing
four-player, two-team limit. Keep a bot-free solo-practice option available.

Start with three difficulty settings: **Easy, Normal and Hard**, selectable per
bot during setup. Difficulty should change reaction time, aim accuracy and
decision quality while using the same movement, weapons, ammunition and scoring
rules as human players.

The first implementation step is to define the human/bot roster and setup flow,
bot view allocation, team/weapon selection and difficulty tuning. Build on the
existing input and seeded Debug-bot experience where useful, preserving the
game's OOP design. Playable bots need to navigate platforms and recover when
stuck, seek paintable territory, engage opponents and manage paint reserves
across all five tools. The current scripted measurement bots do not establish
those capabilities.

Acceptance needs menu setup, mixed human/bot rosters, bot identity, pause and
disconnect handling, restarts and series continuity in both builds; repeatable
navigation/combat checks on the shipped arenas; and attended play showing that
the three difficulty settings feel distinct. Automated checks alone do not
establish difficulty or gameplay quality.

#### Roster foundation (2026-10-27)

The first bounded slice adds explicit human/bot seats and `BotDifficulty`
(`easy`, `normal`, `hard`) to `MenuLevelSettings` in both builds. Existing
seats default to human; P1 remains the human setup host. A bot occupies one of
the four participant seats and has no physical device slot. An unclaimed
human, including a Debug dummy, is still human: `num == -1` alone never means
bot. Human-to-bot conversion releases the device; bot-to-human conversion
requires a new claim. Changing difficulty keeps the team and weapon choices.
Fresh input claims preserve bot choices; shrinking discards removed seats,
and growth adds unclaimed humans. Copies retain all seat metadata, and the
existing balanced team shuffle includes bots.

This initial slice established the roster model. At that stage, normal setup
had no bot controls and construction, remembered-setup capture, labels and replay
writing rejected bot rosters. The metadata and runtime slices below now replace
those temporary guards.

Validation: Debug and Release builds pass, together with the six selected
consumer cases `bot_roster`, `settings`, `team_selection`, `remembered_setup`,
`debug_console` and `player_identity` in each build. The new case covers all
human/bot seat patterns within the four-participant limit, each difficulty,
sparse claims, conversions, reset/copy/shuffle/resize retention and invalid
requests without mutation. Unsupported consumers reject before resource access,
lossy capture or replay filesystem changes. Both real-bank isolated startup
checks pass. Evidence: `out/validation/road10-roster/`. These are roster and
regression checks; bot navigation, combat, views and attended difficulty
acceptance are still unimplemented/unqualified.

The integration contract for the next slices is:

- **Setup:** Player Count remains the total participant count. In Join, P1
  selects a non-host, unclaimed row and changes it between Human and Bot with
  Easy/Normal/Hard difficulty, defaulting a new bot to Normal. A joined human
  must leave before the host changes that row. Other devices claim only human
  rows; one human plus up to three bots needs no spare controllers. One-player
  practice stays bot-free. Keep the existing input actions: Up/Down selects a
  host-controlled row, Left/Right changes its choice, Confirm continues when
  every human is connected. Do not add gameplay bindings for setup.
- **Teams and tools:** humans retain their own rows. P1 uses Up/Down to switch
  between P1 and bot rows, Left/Right to choose each row's team or tool, and
  Confirm/Back to ready/unready that row. All rows must be ready, and a
  multiplayer roster must contain both teams. Random teams includes bots and
  clears every confirmation. Random weapons resolve per participant. Bot
  identity and difficulty must appear in setup and match/results labels.
- **Views:** only humans get split-screen panes, ordered by human seat. Roster
  indices remain stable: a bot in P2 does not rename P3 when P3 gets the second
  human pane. Introduce an explicit participant/view mapping through builders,
  cameras, scene views, HUDs, aim and overlays; changing only `screen_layout`
  to the human count is insufficient. All participants still count for paint,
  spawns, story, scoring and results. One human against bots is a versus match.
- **Lifetime and inputs:** the shipping bot driver produces ordinary
  `PlayerInputData` after physical-slot mapping. It must never emit menu/pause
  actions, request rumble or claim a controller. Pause/focus loss stops bot
  decisions with the simulation; disconnects concern participating humans.
  Rebuild driver state for restart/next round and retain the chosen roster and
  difficulties through the existing settings copies. Record the final bot
  inputs; replay replaces the driver. Save and replay formats must preserve
  controller kind/difficulty before their rejection guards are removed.
- **Difficulty:** add validated data in `tuning.json` for reaction intervals,
  aim error and decision/replanning quality when the driver consumes it. Use a
  seeded private random stream per bot. All difficulties obey normal movement,
  tool, reserve, health, respawn and scoring rules; Hard does not gain hidden
  enemies, extra paint or speed. Initial values remain provisional until the
  attended comparison establishes distinct, useful difficulties.

#### Saved identity and replay metadata (2026-10-27)

Remembered setups now preserve each participant's explicit controller kind and
bot difficulty, alongside the existing team and tool. Human seats restore
unclaimed; bots retain no device. Layout follows the human count while series,
teams and participant count still describe the whole match. Legacy entries
default to human/Normal, malformed fields recover independently, and P1 remains
the human host. Human difficulty is unused and saved canonically as Normal.
New captures and writes reject invalid metadata before changing the save.

Debug replay format **4** records the same controller/difficulty metadata and
human layout; its reader also accepts legacy human-only format 3. The version
change prevents an older reader from silently treating a bot as a human dummy.
Sparse human seats retain their roster indices. Recording still captures final
per-participant inputs after its inner filter, and playback replaces those
inputs; this is a codec/input-order check, not playable bot replay acceptance.
Invalid roster writes leave existing files and missing directories untouched.

The shared Join/Team/Weapon/Results label formatter now spells out
`P2 - Bot (Normal)` and its Easy/Hard variants using validated
`menu_text.labels` copy. Resource validation includes bot fonts, complete row
bounds and Results percentages. Debug status and paint overlays also distinguish
bot difficulty from an idle human with `num == -1`. Runtime world identity and
the human/view mapping remain part of the next slice.

At this stage, normal setup remained human-only and the level builder retained
its early rejection guard. The runtime slice below removes both guards together
with the playable setup/view/driver path.

Validation: Debug and Release builds pass, with the eight selected consumer
cases `bot_roster`, `remembered_setup`, `debug_console`, `menu_text`, `settings`,
`team_selection`, `player_identity` and `moving_structure` in each build.
The new checks cover all 15 legal human/bot patterns across three difficulty
variants, legacy/malformed metadata, sparse human gaps, final recorded input
ordering, bot labels and the retained runtime rejection. Native
`-RememberedSetupOnly` probes pass on hardware D3D11 in both builds, including
separate-process reload, restart/series regressions and an actual menu opening
a fresh human roster without changing the remembered bot save. Both real-bank
isolated startup checks pass. Evidence: `out/validation/road10-metadata/`.
These are selected metadata/regression checks, not the full acceptance suite,
bot gameplay qualification or attended difficulty acceptance.

#### Setup, human panes and shipping driver (2026-10-27)

Normal Join now lets P1 select an unclaimed non-host row with Up/Down and
change Human/Bot difficulty with Left/Right. A new bot defaults to Normal;
joined or reserved human rows must leave before conversion. P1 controls its
own and bot Team/Weapon rows, with separate row readiness; other humans retain
their own input. Bot-free solo practice remains available. Remembered bots
restore into normal setup with fresh human claims, and normally confirmed bot
matches save their resolved roster through the existing launch callback.

`Level::player` and `player_count` now describe stable roster participants.
Explicit `participant_for_view`/`view_for_participant` mappings give panes only
to humans. Scene views, follow/aim cameras, HUDs, overlays and the opening's
camera transition use that mapping. P3 keeps its number when a bot occupies P2.
Bots retain participant-based spawns, paint credit, audio identity, scoring and
results. Their world labels include the stable number and difficulty, with
validated `player_identity.bot_label_size` and `bot_label_gap` geometry.

The Release-capable `MatchBotDriver` supplies ordinary participant input after
physical-slot mapping and mouse aim, before the optional developer filter.
Each bot has a private seeded stream. Recording sees the final input; playback
bypasses the shipping driver. Rebuilds create fresh driver state while settings
copies preserve difficulty and choices. Bots have no physical slot, menu/pause
actions or motor destination. Pause, focus loss, reconnect waits and the resume
count-in stop decisions with simulation.

Initial policy values live in validated `tuning.json.bots`: reaction/replan
intervals, aim error and territory candidate count per difficulty, plus shared
sight range, reserve thresholds and stuck recovery delays. The driver samples
local visible unowned/enemy paint, responds to living enemies with conservative
structural-bounds line of sight, predicts a local aim point, and emits normal
jump/turn/drop/fire requests. The reserve guard stops firing until recovery;
all movement, ammunition, health, respawn and scoring still use normal game
rules. Hard gets no extra speed, supply or hidden enemy observations.

This is an initial local policy, not arena navigation or combat qualification.
It has no route graph; multi-level routes can still trap its reactive recovery.
Structural AABB sight is conservative around ramps/platforms. Its engagement
and lead estimates do not establish projectile hits with drag, inherited motion
or bounces. Snapshot memory is bounded by the current arena, but CPU/frame-pacing
impact has not been measured. Initial Easy/Normal/Hard values remain provisional.

Validation: Debug and Release builds pass, with the eleven selected consumer
cases `bot_driver`, `bot_runtime`, `bot_roster`, `remembered_setup`,
`debug_console`, `menu_text`, `settings`, `team_selection`, `player_identity`,
`moving_structure` and `gameplay_tuning` in each build. The new runtime case
constructs all 15 legal human/bot arrangements, retains sparse identity and
tool/team choices, checks unchanged human inputs and zero-time decisions, and
records/replays 120 actual bot-driven steps to an equal final Debug fingerprint.
The policy case checks all three difficulties and five tool observations,
reserve hysteresis, reaction/aim, death/respawn and jump/turn/drop requests;
these are policy checks, not successful arena-route or combat trials.

Native `-BotPlayersOnly` passes at 720p/1080p on hardware D3D11 in both builds.
It drives actual Join/Team/Weapon pages, bot-free solo, host and sparse-human
readiness, then an actual P1-human/P2-bot/P3-human/P4-bot match through normal
setup capture, Pause/Restart, focus count-in, human disconnect/reconnect and two
Results/series continuations. Frozen frames retain state and the two human
panes; inspected captures show the P2 Easy/P4 Hard world labels. Native
`-RememberedSetupOnly` passes in both builds, including normal remembered-bot
restoration and separate-process human reclaims. `-JoinPaperOnly` retains the
human setup/layout regressions at six sizes in both builds. Both real-bank
isolated startup checks pass. Evidence: `out/validation/road10-runtime/`, with
build logs at `out/road10-build-debug.log` and `out/road10-build-release.log`.
This is selected acceptance, not the full suite, physical-pad, performance or
attended difficulty qualification.

#### Bounded arena trials, recovery and combat approach (2026-10-27)

The opt-in `bot_trials` consumer case now runs the shipping driver through
120 natural-spawn trials: Crossfeed, Close Quarters, King of the Hill and
Turbulence, all five tools, both bot teams and three difficulties. Each trial
has a fixed seed and 600 active simulation steps. Ten separate Hard combat
trials start the bot and a stationary human opponent 700 units apart on the
mirrored Crossfeed lower floors. Those trials use initial placement only;
natural-spawn trials never inject positions, and neither mode uses live cheats.
The CSV records movement, jump/drop requests, blocked steps, paid fire,
reserves/recovery, owned paint, opponent health loss and attributed eliminations.

Two policy defects are fixed. Recovery now holds an escape direction before
sampling another territory goal, rather than immediately choosing the same
obstruction again. Positional progress also detects small oscillations even
when collision velocity is nonzero. Validated `bots.recovery_steps` (60),
`progress_window_steps` (60) and `progress_distance` (52 units) own the new
thresholds. Death clears the recovery; intentional combat stops do not trigger
it. Ordinary reaction timing can retain the escape direction briefly after the
holdoff ends. This remains local recovery, with no platform route graph.

Combat approach now pursues a living visible opponent outside firing range;
previously short-range tools ignored that opponent until already close enough.
It drops the pursuit on lost visibility after the normal reaction interval.
The range estimate also caps gravity-affected travel at its ideal same-height
ballistic reach, retaining the existing drag/lifetime limit. This prevents
Sprayer and Bucket from stopping at an impossible nominal range. Neither the
cap nor the existing aim lead is an exact trajectory solver.

The Release baseline at `74f271b2` produced opponent health loss in 6/10 local
combat trials. The updated policy produces health loss in **10/10**, with
owner-attributed eliminations in 8/10; Sprayer damages but does not eliminate
within this budget. All ten now require actual paid fire and opponent health
loss to pass. These are stationary local opponents, not opposed-match balance.

Natural-spawn paint results remain measurements, not route-success gates:

| Arena | Trials holding owned paint before | After |
|---|---|---|
| Crossfeed | 30/30 | 30/30 |
| Close Quarters | 20/30 | 19/30 |
| King of the Hill | 30/30 | 30/30 |
| Turbulence | 30/30 | 30/30 |

The eleven zero-paint results are all Close Quarters Team A: Easy/Normal with
every tool, plus Hard Roller. Recovery alone does not resolve that route and
territory failure; the decreased count is retained as evidence rather than
hidden behind a passing execution check. Turbulence's mean inward progress
increases from 1834 to 2180 units, but these trials do not prove required
platform landings, useful regional coverage or general navigation quality.

Validation: Debug and Release builds, the seven selected consumer cases
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_trials`, `bot_runtime`,
`gameplay_tuning` and `movement`, and both real-bank isolated startup checks
pass. The 130-trial CSVs are byte-identical between builds. Two selected
600-step final-input tapes reproduce public actor/projectile/paint state in
both builds, with complete fingerprints additionally checked in Debug. The
short recovery/range cases join the default suite; the matrix stays opt-in:
`scripts/run-acceptance.ps1 -Preset x64-debug -Cases bot_trials` (or Release).
Evidence: `out/validation/road10-trials/`; the earlier Release baseline is in
`out/validation/road10-trials-baseline/`. This is selected acceptance, not the
full suite, native bot presentation, physical-pad, performance or attended
difficulty qualification.

#### Close Quarters escape and separate standing goals (2026-10-27)

The Close Quarters Team A failure is reproduced by the retained trial CSV:
Easy/Normal bots stop at the spawn baffle, with no drop requests or owned
paint. Their visible paint samples provide no useful route out of the spawn;
the old fallback keeps jumping into the baffle. A paint-face contact point
also lacks the body clearance needed by a standing destination.

The shipping policy now selects paint aim and standing destinations
independently. Visible paint within the existing tool-range estimate can be
aimed at even when its standing destination is unavailable; more distant paint
can still inform movement. Standing candidates require a body-sized flat top,
clearance and a conservative direct body sweep. One-way supports are explicit;
ramps and rotated slabs retain their obstacle bounds but are not flat-top
candidates. Enemy visibility remains the existing conservative sight query.

When no standing goal is available on a one-way platform, the bot can choose a
known lower support and request an ordinary drop. It checks a newly landed
platform at its next reaction rather than waiting for the next paint replan.
Near a support boundary, the descent goal turns toward available floor space.
A deliberate drop clears held jump and suppresses a simultaneous takeoff.
These are local standing/descent checks, not a platform route graph or an
exact jump/fall trajectory solver; the direct sweep can reject a legitimate
route that needs an intervening jump.

At the same fixed seed and 600 active steps, natural-spawn trials now hold
paint in **30/30 Close Quarters configurations**, up from 19/30 at `47a70c43`.
Crossfeed, King of the Hill and Turbulence retain 30/30 each. Close Quarters
now requires firing and bot-owned paint in every tool/team/difficulty
configuration. The ten stationary local combat trials retain actual opponent
health loss in 10/10, with owner-attributed eliminations in 9/10. These counts
do not establish useful regional coverage or opposed balance.

The new default `bot_navigation` case separately qualifies 30 mirrored home
drops using the real shipped Close Quarters geometry. It places the bot once
at home-floor center X=500/5500; subsequent inputs come from the shipping
driver. Within 600 active steps, it requires a drop request while grounded on
`spawn_a_floor`/`spawn_b_floor`, a subsequent grounded contact on the named
recovery or lower bypass, then grounded contact and owned top-face paint on
`boundary_floor`. Landings require feet within 0.05 units and horizontal body
overlap with the named surface; they are transient contacts, not controlled
stops or centered landings. A visit to the recovery platform is optional.
The human starts at its opposite natural spawn and receives neutral input.

Validation: Debug and Release builds, the eight selected cases `bot_driver`,
`bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_trials`, `bot_runtime`,
`gameplay_tuning` and `movement`, and both real-bank isolated startup checks
pass. The 130-trial CSVs are byte-identical between builds. Three matrix tapes
(including the formerly failing Team A Normal Roller) and two mirrored home-drop
tapes reproduce public player/paint state and projectile count after 600
steps; Debug additionally checks complete fingerprints. Policy regressions
cover separate aim/standing targets, tool-range aim, unknown/obstructed
descents, held-jump cancellation and inward boundary escapes. The CSV now calls
the old misleading `paid_shots` column `firing_steps`: that flag includes
affordable held-fire presentation between launches; owned paint and opponent
damage establish actual normal-rule projectile contacts. Evidence:
`out/validation/road10-navigation/`. This is selected acceptance, not the full
suite, native presentation, physical-pad, performance or attended difficulty
qualification; CPU/frame-pacing cost remains unmeasured.

#### Adjacent support ascents and recovery returns (2026-10-27)

The shipping policy now explores adjacent raised rectangular supports. It
derives a conservative rise and approach budget from the normal held jump,
checks body clearance and the solid-body approach/rise/crossing/descent sweeps,
and prefers smaller rises among nearby candidates. A solid target uses a
takeoff beside its lip; a centered one-way target permits a vertical jump.
The selected route steers and brakes each simulation step, while initial
selection, aim and firing retain the difficulty's reaction policy. There are
no arena names or authored route hints in the driver.

An ascent can continue through an intermediate one-way support. Completed or
failed targets are excluded for the rest of that life. An attempt expires
after the existing progress-window plus recovery budget (120 steps at the
current values); normal stuck detection does not mistake intentional vertical
takeoff or braking for a blocked horizontal goal. A vanished target or newly
occupied landing cancels the attempt, and death clears its route and visited
supports. Initial one-way spawns retain the downward escape. After a deliberate
drop, exploration waits for grounded contact on a paintable top, so it cannot
reverse the escape or climb past the first scoring floor. Support observations
carry that paintable role separately from their collision geometry.

The opt-in `bot_ascents` case qualifies **90/90** actual-driver trials: three
independent starts, both sides, all five tools and all three difficulties.
Each has initial placement only and a 900-active-step budget:

- Home return: `boundary_floor_left/right` → `spawn_a/b_recovery` →
  `spawn_a/b_floor`, starting at X=500/5500.
- Lower deck: the same lower floor → `lower_left/right_platform` →
  `lower_paint`, starting at X=1300/4700.
- Upper deck: `lower_paint` → `upper_left/right_platform` → `upper_paint`,
  starting at X=2300/3700.

Each route requires ordered grounded contacts and at least two ordinary jump
presses. Contacts use the existing 0.05-unit feet-height tolerance and body
overlap; these are transient landings, not centered stops. The two deck routes
are separate starts, not one continuous lower-floor-to-summit qualification.
At the end, **59/60** deck trials retain bot-owned top-face paint. The remaining
Easy Team A Sprayer upper-deck trial reaches both endpoints but retains no
owned top paint. Paint is a measurement here, not a route pass condition;
regional usefulness and contested ownership remain open.

Validation: both builds and the nine selected cases `bot_driver`,
`bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`, `bot_trials`,
`bot_runtime`, `gameplay_tuning` and `movement` pass. The retained 30 home-drop
gates still require scoring-floor contact and owned floor paint. Natural-spawn
trials retain paint in 120/120 configurations; the ten stationary local combat
trials retain damage in 10/10 and attributed eliminations in 9/10. Six new
900-step final-input tapes reproduce public player/paint state and projectile
count in both builds, with complete fingerprints additionally checked in Debug.
Policy checks cover all difficulties, invalid support/clearance, initial
reaction, enemy priority, target disappearance, occupied landings, expiry,
failed-target exclusion and death/respawn reset. Both real-bank isolated startup
checks pass. Evidence: `out/validation/road10-ascents/`; reproduce with
`scripts/run-acceptance.ps1 -Preset x64-debug -Cases bot_ascents` (or Release).

This is static Close Quarters qualification for an adjacent-support heuristic,
not a general route graph or an exact trajectory solver. Intervening one-way
contacts can catch a jump, and the conservative sweeps can reject valid routes.
Moving supports, ramps, rotated slabs and later path obstruction beyond the
landing need separate qualification. The full suite, native presentation,
physical-pad, performance and attended difficulty acceptance were not run.

#### Other-arena local ascents and recovery returns (2026-10-27)

The opt-in `bot_arena_routes` case adds **150/150** actual-driver route gates
across Crossfeed, King of the Hill and Turbulence: five separate starts, both
sides, all five tools and all three difficulties. Each trial injects its initial
position only, then runs 900 active steps with ordinary bot inputs; the opposite
human starts at its natural spawn and receives neutral input.

| Arena / route | Named ordered supports | Starting center X (A / B) |
|---|---|---|
| Crossfeed home return | `boundary_floor_a/b` -> `spawn_a/b_floor` | 300 / 3300 |
| Crossfeed mixer | `centre_plinth` -> `upper_step_a/b` -> `centre_mixer` | 1700 / 1900 |
| King of the Hill terraces | `mountain_level_5` -> `mountain_level_6` -> `mountain_level_7` | 2000 / 4000 |
| Turbulence lower deck | `boundary_floor` -> `left_bottom_left_jump` / `right_bottom_right_jump` -> `left/right_lower_paint` | 825 / 5385 |
| Turbulence middle deck | `left/right_lower_paint` -> `left_mid_left_jump` / `right_mid_right_jump` -> `left/right_mid_paint` | 1190 / 4810 |

The gates retain the existing 0.05-unit feet-height tolerance and horizontal
body overlap, requiring ordered grounded contacts and one ordinary jump for
the home return or two for the other routes. These are transient landings,
not centered stops. Final owned top-face paint is measured separately:
**139/150** trials retain some on their finishing support. The eleven misses
and the retained Close Quarters deck miss remain territory-usefulness work;
successful traversal does not establish regional value or contested ownership.

The first Release probe passed only **66/150** routes and exposed three policy
defects, now fixed without arena-specific hints or geometry/tuning changes:

- The ascent envelope applied acceleration before the launch displacement.
  It now respects the normal launch tick and the actual number of held jump
  outputs. At current values this permits Turbulence's reachable 200-unit
  one-way steps; a 201-unit step remains outside the estimate.
- Alignment assumed continuous full-body floor beneath a jump crossing.
  Takeoff now keeps the body's center inside the known source floor while
  allowing ordinary lip overhang. Airborne steering crosses a one-way target
  immediately, or approaches a solid lip until the feet clear its top. Candidate
  ranking preserves smaller adjacent rises, adds the crossing distance beyond
  a body width, and breaks ties by walking approach. This supersedes the earlier
  beside-target takeoff and height-only ranking description above. The retained
  CQ gates caught and prevented skipping its lower scoring deck for the higher
  overlapping approach bridge.
- Support observations derived paintability from collision type. They now use
  the independent paintable object role, including Crossfeed's six scoring
  one-way platforms, so a scoring landing can end a deliberate descent's
  exploration holdoff. Ordinary non-scoring one-way supports retain that holdoff.

Validation: Debug and Release builds and the ten selected consumer cases
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`,
`bot_arena_routes`, `bot_trials`, `bot_runtime`, `gameplay_tuning` and `movement`
pass. The retained CQ checks pass all 90 ascents and 30 home drops, with deck
paint still 59/60. Natural-spawn trials retain paint in 120/120 configurations;
stationary local combat retains damage in 10/10 and attributed eliminations in
9/10. All ten new 900-step final-input tapes reproduce public player, projectile
and exact paint state in both builds, with complete fingerprints additionally
checked in Debug; the retained cases also keep their replay gates. New arena
measurements and the 130-trial CSV are identical between builds. Policy checks
cover the jump-height boundary, missing source floor, gap alignment/takeoff,
airborne crossing, local-step choice and scoring/non-scoring descent landings;
actual-object checks retain independent scoring and one-way roles. Both
real-bank isolated startup checks pass. Evidence:
`out/validation/road10-arena-routes/`; reproduce the new matrix with
`scripts/run-acceptance.ps1 -Preset x64-debug -Cases bot_arena_routes` (or Release).

These are separate local starts, not continuous ground-to-summit qualification
or a general route graph. King of the Hill's ramp approach and castle interior,
Turbulence's upper chain, moving supports, rotated slabs and later obstruction
remain unqualified. The horizontal budget still estimates whole-flight travel;
it does not solve how much crossing time remains above a solid lip. Finite
expiry and failed-target exclusion contain failed attempts. The full suite,
native bot presentation, physical-pad, performance and attended difficulty
acceptance were not run; initial Easy/Normal/Hard values remain provisional.

#### King of the Hill ramp and castle chains (2026-10-27)

The new opt-in `bot_hill_routes` case starts at three independent endpoints in
the shipped King of the Hill, with initial placement only. Each chain runs both
sides, all five tools and all three difficulties, for **90/90** route gates.
Every trial has a fixed 1800-active-step budget; the opposite human receives
neutral input at its natural spawn. No route names, destinations, resources or
subsequent placements enter the shipping policy.

| Chain | Ordered grounded contacts | Starting center X (A / B) |
|---|---|---|
| Ramp approach | `boundary_floor` -> authored east/west ramp -> `mountain_level_1_centre` -> `mountain_level_5` | 200 / 5800 |
| Summit and entrance | `mountain_level_7` -> levels 8, 9, 10, 11 -> `castle_entrance` | 2350 / 3650 |
| Castle interior | `castle_floor_left/right` -> `castle_platform_left/right` -> `castle_platform_mid` -> `castle_room_entrance` | 2775 / 3225 |

Flat contacts retain the 0.05-unit feet-height tolerance and body overlap.
Ramp contact additionally requires the actual ramp movement state and the
uphill body corner on the authored hypotenuse. The chains require at least
one, five and three ordinary jump presses respectively. Final required
contacts occur by step 211, 276 and 171 respectively across the matrix;
the simulation continues for the full budget. These remain separate starts,
not one continuous boundary-floor-to-castle-room qualification.

The unchanged policy at `88aa901a` passes only **37/90**: 23 ramp approaches,
13 summit/entrance chains and one castle-interior chain. The failures exposed
four arena-independent policy gaps, repaired without geometry, tuning or tool
changes:

- Upright ramps now have explicit bounds and uphill direction observations,
  separate from flat standing supports. A bot can approach a known toe, walk
  the hypotenuse using its ordinary supporting corner, and leave onto known
  crest support. The selected ramp's AABB is bypassed only for these checked
  slope paths; other solid bounds remain conservative barriers. Remaining path
  clearance, ramp presence and crest support are rechecked each step. A finite
  travel budget derives from path length and normal initial movement speed;
  expiry, obstruction or disappearance cancels the walk, and death clears its
  visited-ramp exclusion. Enemy engagement retains normal reaction priority.
- Short positive steps are eligible. The previous half-body-height minimum
  excluded the 50-unit step above the ramp crest.
- Ascent clearance checks a crossing just above the destination lip rather
  than demanding room for the entire maximum held-jump apex. Ordinary ceiling
  collision may shorten a jump after it has reached usable crossing height;
  insufficient landing body clearance still rejects it.
- One-step geometric lookahead prefers an available continuing ascent over
  a dead-end shelf, retaining local climb/gap/approach ranking within each
  group. Visited or failed supports cannot supply that continuation. Solid
  gap crossings carry normal run-up speed into takeoff inside the known
  source lip and steer across immediately, instead of braking to rest first.

Validation: Debug and Release builds and the eleven selected consumer cases
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`,
`bot_arena_routes`, `bot_hill_routes`, `bot_trials`, `bot_runtime`,
`gameplay_tuning` and `movement` pass. New direct policy checks cover mirrored
ramps, reaction/priority, obstruction/disappearance, finite expiry and
death/respawn, short steps, low-headroom acceptance/rejection, continuing-chain
selection and solid-gap run-up. Actual-object checks retain the authored ramp
bounds/directions and their separate obstacle/standing roles in Crossfeed and
King of the Hill. Six new 1800-step final-input tapes reproduce public player,
projectile count and exact paint state in both builds, with complete
fingerprints additionally checked in Debug. Both real-bank isolated startup
checks pass. New hill measurements and the 130-trial CSV are identical between
builds. Evidence: `out/validation/road10-hill/`; the unchanged Release
baseline is in `out/validation/road10-hill-baseline/`. Reproduce the new matrix
with `scripts/run-acceptance.ps1 -Preset x64-debug -Cases bot_hill_routes`
(or Release).

The retained 90 CQ ascents, 30 home drops and 150 other-arena route gates still
pass. CQ final deck-paint retention changes from 59/60 to **60/60**, while the
other-arena finishing-support measurement changes from 139/150 to **135/150**.
These paint counts are recorded separately from traversal success; retained
paint misses and regional usefulness remain open. Natural-spawn trials retain
paint in 120/120 configurations; stationary local combat retains damage in
10/10 and attributed eliminations in 9/10.

The clearance and crossing estimates remain local heuristics, not a general
route graph or trajectory solver. Arbitrary slopes, moving supports, rotated
slabs and opposed routes remain unqualified. The full suite, native bot
presentation, physical-pad, CPU/frame-pacing and attended difficulty acceptance
were not run; the lookahead's runtime cost has not been measured.

#### Turbulence upper crossings and continuous ascent (2026-10-27)

The opt-in `bot_turbulence_routes` case adds **90/90** actual-driver chains in
the shipped Turbulence arena: three starts, both sides, all five tools and all
three difficulties. Every trial uses one initial placement and a fixed
3600-active-step budget in a 120-second match. The opposite human receives
neutral input at its natural spawn. No route, goal, resource or subsequent
placement enters the shipping policy.

The shared upper chain requires ordered grounded contact with
`central_divider_ledge_paint` -> `upper_jump_mid` -> either `upper_left` or
`upper_right` -> either `upper_jump_left` or `upper_jump_right` ->
`upper_top_paint`. This qualifies travel from the divider through a side deck
to the top; it does not require a left-to-right shelf transfer.

| Chain | Starting center X (A / B) | Required floor-to-divider prefix | Minimum jump presses |
|---|---|---|---|
| Upper crossing | 2650 / 3350 | Starts on the divider paint ledge | 4 |
| Continuous outer-floor ascent | 825 / 5385 | `boundary_floor` -> `left_bottom_left_jump` / `right_bottom_right_jump` -> side ascent below | 10 |
| Local divider recovery | 2825 / 3175 | `boundary_floor` -> `left_bottom_right_jump` / `right_bottom_left_jump` -> side ascent below | 10 |

The side ascent continues through `left/right_lower_paint` -> either of that
side's mid jump steps -> `left/right_mid_paint` ->
`left/right_upper_mid_right_jump`, then the shared upper chain. Contacts retain
the 0.05-unit feet-height tolerance and horizontal body overlap. Continuous
means one uninterrupted simulation with no further placement; ordinary falls
and retries remain allowed. Divider recovery starts on the floor beside the
central obstruction, without a forced in-flight disruption. Final required
contacts occur by step 268, 697 and 671 respectively; every trial continues
for its full budget.

The unchanged Release policy at `553d335f` passes only **39/90**: 14 upper
crossings, eight outer ascents and 17 divider recoveries. Its supported walk
to takeoff incorrectly consumed the airborne jump-distance allowance. Walking
approach is now bounded independently by sight and checked floor clearance;
only airborne alignment consumes jump reach. Finite expiry includes approach
distance at normal initial movement speed, plus the existing jump/recovery
allowance. Candidate ranking charges walking distance beyond the old local
jump envelope, preserving nearby low-step priorities while preventing newly
admitted remote shelves from displacing useful local climbs. The policy adds
no arena names or route hints, and geometry, tuning and tools are unchanged.
A stalled full-sight approach can retain its attempt for up to 1080 steps
(18 seconds) at current values; later obstruction revalidation is unqualified.

Validation: Debug and Release builds and the twelve selected consumer cases
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`,
`bot_arena_routes`, `bot_hill_routes`, `bot_turbulence_routes`, `bot_trials`,
`bot_runtime`, `gameplay_tuning` and `movement` pass. Direct policy checks cover
mirrored long supported approaches, retained reaction delays, local/remote
candidate ranking, excessive walk/gap rejection and finite stalled expiry.
Six new 3600-step final-input tapes reproduce public players, projectile count
and exact paint state in both builds, with complete fingerprints additionally
checked in Debug. Both real-bank isolated startup checks pass. New route
measurements and the 130-trial CSV are identical between builds. Evidence:
`out/validation/road10-turbulence/`; unchanged Release baseline:
`out/validation/road10-turbulence-baseline/`. Reproduce the new matrix with
`scripts/run-acceptance.ps1 -Preset x64-debug -Cases bot_turbulence_routes`
(or Release).

Final owned top-face paint is measured separately: **82/90** new trials retain
some. The eight misses are Easy: three Team A upper crossings and five divider
recoveries across the two sides. The retained CQ gates pass all 90 ascents and
30 home drops, with deck paint **60/60**; all 150 other-arena routes and 90 hill
chains pass, with other-arena finishing-support paint **139/150**. Natural-spawn
trials retain paint in 120/120 configurations; stationary local combat retains
damage in 10/10 and attributed eliminations in 9/10. Traversal does not
establish regional value or contested ownership; paint misses remain open.

Moving and opposed local combat are qualified in the next slice below. Moving
supports, rotated slabs, arbitrary slopes and general navigation remain
unqualified. This route slice did not run the full suite, native bot presentation,
physical-pad or CPU/frame-pacing checks.

#### Moving opponents, opposed bots and local scoring paint (2026-10-27)

The opt-in `bot_combat_trials` case adds **60** fixed-seed Crossfeed trials:
both mirrored lower-floor starts, all five tools and all three difficulties,
with 1200 active steps per trial. Thirty have one shipping bot against an
ordinary human input script that shuttles and jumps without firing. The other
thirty have two opposed shipping bots; P1 remains a neutral human at its natural
spawn in that three-seat roster. Only the two combatants' initial positions are
placed. Death, respawn, resources and subsequent movement follow normal rules.
The human's short jumps can hit the authored ceiling, and a killed human's
ordinary respawn can take it outside the initial shuttle band.

Every bot must move, fire with normal reserves and retain owned paint; every
trial must produce opponent health loss. All thirty moving humans additionally
lose health while already moving. Damage, its first step and moving/airborne
context are observed health changes; only lethal contacts have owner attribution.
The CSV also records per-actor reserve recovery, owned area and observed
neutral/enemy tile transitions. These transitions are sampled once per step,
not total shots or every within-step paint exchange.

Combat aim now searches a finite free-flight prediction using the actual
weapon's muzzle offsets, inherited velocity, projectile gravity, horizontal
drag, velocity caps and lifetime. The old distance/speed lead omitted these
launch details and capped prediction at half a second. The new search evaluates
32 directions/refinements, each for at most 60 integration steps. It uses only
the currently visible enemy's observed position/velocity. Reaction intervals
and one authored difficulty-error draw per decision are unchanged. Sixteen
mirrored actual paid-shot centerlines pass within 0.26 units of stationary or
constant-velocity targets before difficulty error; separate checks cover close,
vertical, unreachable and coincident targets.

Territory replans now supplement the existing random movement candidates with
the nearest visible non-team top tile on the bot's current grounded scoring
support. This deterministic candidate supplies paint aim only; it adds no
standing/descent goal or random draws. Existing territory replanning/reaction
timing clears it, so short transient landings can still be missed. Geometry
checks exclude non-scoring supports, other faces and rotated tops. Paired policy
checks retain navigation and random streams; **30/30** actual-driver starts on
Turbulence's `upper_top_paint` buy shots and paint that top within 180 active steps.
These starts qualify local painting, not reaching that platform.

The unchanged Release baseline at `07fd8450` and the updated policy both damage
the designated opponent in 30/30 trials of each combat mode. Moving-human health
loss totals 27.714 -> 28.287; opposed-bot health loss totals 73.375 -> 70.639,
with attributed eliminations 40 -> 38. The updated opposed trials damage
59/60 bot actors: in one Normal Sniper trial P2 eliminates P3 without taking
damage. The gate allows that clean win rather than requiring retaliation.
Either opposed bot repaints enemy tiles in 27/30 trials in both versions; P2's
repaint-positive trials change from 24/30 to 25/30. These are local capability
checks and mixed measurements, not evidence of an overall balance improvement
or monotonic difficulty strength.

All **450** retained static route gates pass. Final finishing-support paint
remains a separate measurement: CQ deck paint stays 60/60, the other-arena
matrix changes 139/150 -> **144/150**, and the Turbulence upper/continuous matrix
changes 82/90 -> **79/90**. The eleven Turbulence misses and six other-arena
misses remain open; a local paint opportunity does not ensure painting during a
brief landing. Natural-spawn paint remains 120/120. The ten retained stationary
local combat trials still damage in 10/10, now with attributed eliminations
in 10/10 rather than 9/10.

Validation: Debug and Release builds and fourteen selected cases pass:
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`,
`bot_arena_routes`, `bot_hill_routes`, `bot_turbulence_routes`, `bot_trials`,
`bot_runtime`, `gameplay_tuning`, `movement`, `bot_territory` and
`bot_combat_trials`. Six new 1200-step final-input tapes reproduce public
players, projectile count and exact paint state in both builds, with complete
fingerprints additionally checked in Debug; retained replay gates also pass.
Both trial CSVs are byte-identical between builds. Both real-bank isolated
startup checks pass. Evidence: `out/validation/road10-combat/`; unchanged Release
baseline: `out/validation/road10-combat-baseline/`. Reproduce the new matrix with
`scripts/run-acceptance.ps1 -Preset x64-debug -Cases bot_combat_trials` (or Release).
The short `bot_territory` case joins the default suite.

The next slice below extends opposed trials to natural arena spawns and regional
ownership, and investigates the retained finishing-support paint misses.
Prediction assumes constant target velocity for one second,
ignores intervening cover/bounces and can prefer a slower arc; it does not prove
reachability. CPU/frame-pacing cost remains unmeasured, including the added
bounded flight search and local surface scan. The full suite, native bot
presentation and physical-pad checks were not run for this slice. Attended
difficulty distinction and usefulness remain the author's acceptance gate;
Easy/Normal/Hard values stay provisional and `ROAD#10` stays OPEN.

#### Natural-spawn teams, regional ownership and top-launch clearance (2026-10-27)

The opt-in `bot_team_trials` case adds **40** natural-spawn 2v2 roster trials:
Crossfeed, Close Quarters, King of the Hill and Turbulence, all five tools and
both mirrored team assignments. Each trial uses one neutral human and three
Normal bots for 1800 active steps. All participants use the same tool in each
trial. No positions, route hints or resource changes are injected. One side has
one active bot beside the neutral human; the other has two, so this qualifies
interactions and territory capability rather than fair opposed balance.

All **120/120** bot actors move more than 100 units, buy ordinary fire and retain
owned paint. Both teams retain paint outside their home band in **80/80** team
observations; these are repeatable gates. All 80 also retain some middle-band
paint, recorded separately as a measurement. The companion regional CSV reuses
the reviewed polygon-clipping measurement and reconciles exact runtime scores.
Crossfeed uses its existing bands; the other arenas use geometric X thirds of
their camera bounds. These regions describe geography, not authored strategic
roles or equal scoring opportunity. Outside-partition paint remains explicit.

| Arena | Matches with observed health loss | Matches with enemy repaint | Bot actors taking health loss |
|---|---|---|---|
| Crossfeed | 10/10 | 9/10 | 25/30 |
| Close Quarters | 10/10 | 9/10 | 27/30 |
| King of the Hill | 9/10 | 7/10 | 22/30 |
| Turbulence | 10/10 | 8/10 | 23/30 |

Owner-attributed bot-v-bot eliminations total **36** across **26/40** matches
(13 Crossfeed, nine Close Quarters, ten King of the Hill and four Turbulence).
Eight additional attributed deaths belong to the neutral human. Both teams'
middle-band holdings and the arena-wide enemy paint transitions are separate
observations: transitions do not yet carry regional attribution.

The main CSV records per-seat movement, requests, shooting state, paid-fire
steps, reserves, health loss, owner-attributed eliminations, enemy/ally owner
transitions and final regional ownership. Paid-fire steps mean an observed net
ammunition decrease; the weapon's shooting-state flag alone also remains true
while a Sniper is cooling down. Health loss is observed rather than attributed;
only lethal contacts carry source ownership. Paint transitions are sampled once
per active step, so they do not count all exchanges inside that step. These
outcomes do not establish team coordination, contested regional retention or
relative difficulty strength.

Finishing-support diagnostics now record grounded contact time, firing requests,
paid-fire steps and a bounded failure trace with aim, muzzle, reserves and tool
readiness. The unchanged Release baseline at `d2f7d6ee` confirms two distinct
limits: some direct downward aims launch below the platform top, and some Easy
upper landings last only two grounded steps while the bot is refilling or the
tool is cooling down. Ascent landing already forces a replan and reaction on the
next step; a general faster reaction was not the missing contract.

The deterministic current-support candidate now rejects a free-flight launch
whose muzzle plus projectile half-height can start below the scoring top over
the difficulty's complete authored aim-error envelope. A farther eligible tile
takes aim priority over a nearer random sample. Roller contact paint retains its
ordinary constrained pose. This changes aim eligibility/priority only: movement
goals, navigation random draws, reaction/replanning cadence, reserves and visible
enemy priority are retained. It does not predict later motion, cover or the full
paint trajectory.

All **450** retained static route gates still pass. CQ deck paint remains 60/60;
other-arena finishing paint remains **144/150**, with a different mix of misses;
Turbulence upper/continuous finishing paint improves **79/90 -> 83/90**. The
retained Normal Sniper landing on Crossfeed's mixer now also requires owned top
paint, reproducing a specific formerly below-floor launch. Six other-arena
misses and seven Easy Turbulence misses remain explicit; the latter all have
only two grounded finishing steps and no paid fire during them. A geometric
launch check cannot provide a missing reserve, cooldown interval or dwell time.
Natural-spawn paint remains 120/120, stationary local combat retains health loss
and attributed eliminations in 10/10, and all 60 moving/opposed local combat
gates remain satisfied.

Validation: Debug and Release builds and fifteen selected consumer cases pass:
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`,
`bot_arena_routes`, `bot_hill_routes`, `bot_turbulence_routes`, `bot_trials`,
`bot_runtime`, `gameplay_tuning`, `movement`, `bot_territory`, `bot_combat_trials`
and `bot_team_trials`. Four new 1800-step final-input tapes reproduce public
players, projectile count and exact paint in both builds, with full fingerprints
additionally checked in Debug; retained replay gates also pass. Both real-bank
isolated startup checks pass. All four trial CSVs (including regional ownership)
are byte-identical between builds. Evidence: `out/validation/road10-teams/`; unchanged
Release route diagnostics: `out/validation/road10-territory-baseline/`.
Reproduce the new matrix with `scripts/run-acceptance.ps1 -Preset x64-debug
-Cases bot_team_trials` (or Release).

The next slice below qualifies the finishing opportunity, mixed-tool and
independent-seed opposition, and regional attribution of paint transitions.
CPU/frame-pacing, moving-support routes,
the full suite, native bot presentation and physical-pad checks were not run for
this slice. Attended difficulty distinction and usefulness remain the author's
acceptance gate; values remain provisional and `ROAD#10` stays OPEN.

#### Finishing window, deliberate support shots and mixed-tool opposition (2026-10-27)

The thirteen retained finishing misses had two causes. The seven Easy
Turbulence misses landed on the lip of `upper_top_paint` with only 1.67 units
of overlap. On the step after landing, no further ascent was eligible, Easy's
eight samples usually supplied no standing goal, and the stale pre-climb
heading sent the bot outward. The ordinary ledge jump then left the top within
two grounded steps. Six of those bots were in reserve hysteresis (ammo
0.17-0.70 against a 0.80 resume), and one Bucket was cooling down.
`readiness=0` in the earlier traces means *ready*, not empty. The six other-arena
misses were all Sniper: its one paid shot per 60 steps was spent on a stale
airborne decision, a support point fixed at replan time and then walked into,
or a near tile the 110-unit muzzle had already passed.

**Finishing window.** An ascent landing on a paintable support marks the next
decision. If that decision chooses no further ascent or ramp, sees no enemy, is
grounded and not recovering, and visible in-range non-team top paint remains on
the support, the bot opens a finite window. The window lasts one regen delay
plus the steps to refill to `reserve_resume` (only when already refilling), one
tool cooldown and `bots.finish_dwell_steps` (30, provisional). During it the bot
steers within the fully supported span of that support, turned inward, and
suppresses jump, drop, descent, ramp and ascent selection. Each step requalifies
the support aim point for the policy's ordinary decision cadence; difficulty
reaction delays and the single aim-error draw per decision are unchanged. The
window closes on expiry, airborne state or recovery, which force the next
decision, or on a visible enemy, which retains the ordinary reaction delay.
Firing rules are unchanged: the reserve hysteresis, the immediate empty/refill
guard, affordability and cooldown all apply. The bot only stays long enough to
use them. Mid-chain landings do not open a window, so route contact timing is
unchanged except at chain ends.

**Deliberate support shots.** A tool whose `shoot_interval` is at least
`bots.deliberate_shot_interval` (0.75 s, provisional; only the Sniper today)
is deliberate. While grounded and ready, its driver requalifies the current
support target every step. The candidate must pass the existing launch-clearance
check and a straight-launch landing test. That test starts from the post-move
muzzle, centres the jet's collision box on it as `ProjectileJet` does, and
requires the swept box's first contact with the top to lie over the support.
With such a target and no visible enemy, the bot withholds a deliberate shot
whose actually fired angle, including its drawn error, would not land. Curved and
contact tools are not judged by that test. Intervening cover is not checked.
Enemy shots and all navigation, reserve and random-draw behaviour are unchanged.

| Measurement | Before (`07bd59d9`) | After |
|---|---|---|
| Turbulence final top paint | 83/90 | **90/90** (now a gate) |
| Other-arena finishing paint | 144/150 | **146/150** |
| CQ deck paint | 60/60 | 60/60 |
| Natural-spawn paint | 120/120 | 120/120 |
| Stationary local combat (damage / attributed eliminations) | 10/10, 10/10 | 10/10, 10/10 |

The four remaining other-arena misses are King of the Hill terrace Snipers on
Normal and Hard. Each passes the finish mid-chain for 15 grounded steps while its
one shot, spent during the climb, cools down. No window opens mid-chain, and the
landing gate cannot supply a cooldown; what a slow tool should spend during an
ascent stays open. Crossfeed's mixer Sniper landings on Easy/a and Hard/b are now
gated beside the retained Normal/b landing. Moving/opposed local combat still
satisfies all 60 gates; its measurements move in both directions (all-seat
health loss in moving-human trials 28.217 -> 27.380, opposed 71.952 -> 74.812, attributed opposed
eliminations 39 -> 40). They are not evidence of balance.

**Mixed tools, independent seeds and regional exchange.** `bot_team_trials` keeps
its 40 same-tool trials byte-identical apart from trial numbering, and adds
**120** mixed-tool trials: four arenas, five rotations, both sides and three
whole-match seeds (`41731`, `52807`, `61403`). Across the rotations each bot seat
holds every tool once, and the lone bot and both opponents always differ. The
neutral human holds a Sprayer and never fires. The seed is the `std::srand`
value that reaches stage draws and then the bot seed. Every one of the 40
configurations gives three pairwise-different final states, so the seeds reach
play.

A new exchange ledger attributes every sampled owner change to Measurement's
exact regional tile pieces, computed once per tile (the harness now also checks
scored-area bits each step). For every region it reconciles the ledger's final
holdings with Measurement's cells. It also requires each bot's regional
transitions to sum to its existing arena-wide neutral/enemy/ally areas. These are
the integrity gates; the conservation residual is bookkeeping. The new
`bot_team_trials_exchanges.csv` reports per region, team and seat: initial and
final holdings, neutral claims, enemy captures and losses, ally owner changes,
`contested_held` (final holding on tiles the enemy held at some sampled step),
first/last exchange steps and mutual exchange.

| Arena (mixed, 30 matches) | Middle mutual capture | Middle contested-held teams | Damage matches |
|---|---|---|---|
| Crossfeed | 28/30 | 57/60 | 30/30 |
| Close Quarters | 19/30 | 45/60 | 27/30 |
| King of the Hill | 18/30 | 43/60 | 27/30 |
| Turbulence | 11/30 | 35/60 | 20/30 |

Repeatable gates: all **360/360** mixed bot actors move, buy fire and retain paint;
all 40 seed configurations are distinct; and teams retain off-home paint in
**239/240** observations. The single exception is a named navigation repro:
Close Quarters, seed `61403`, human on team a, rotation 0. Its lone team-a
Sprayer travels 16.5k units inside a 1500-unit extent without leaving its home
band. Rotations 2 and 4 of the same seed formerly missed the same way and now
pass. Any other miss fails the case. Middle exchange, contested holding and
damage remain measurements. Sampling once per step hides within-step flips. The
one-bot side and the neutral human make these interaction and territory
capability checks, not fair opposed balance.

Validation: Debug and Release builds and the fifteen selected cases pass,
including the eight team-trial and retained replay gates. All five trial CSVs,
including the three team reports, are byte-identical between builds. Both real-bank
isolated startup checks pass. Evidence: `out/validation/road10-finish/`;
the unchanged-policy harness run is `out/validation/road10-mixed-harness/`.
Design diagnoses and an adversarial review were run first. The review's three
confirmed landing-test defects (box origin, swept contact and post-move launch)
are fixed above.

The next slice below fixes that loop, decides the slow tool's ascent spending and
qualifies middle retention under sustained opposition.

#### Home-band loop, slow-tool spending and sustained middle retention (2026-10-27)

**The Close Quarters `61403` loop.** A per-step trace of the lone team-a Sprayer
showed no stuck state the progress check could see. It spawns above the
non-scoring home floor (x 0-1500) and descends to it. With no sampled paint in
range, its decision aims along its heading, `(heading, 0.7)`, and fires. That
untargeted fire painted nothing that scores and emptied the reserve near x 1200.
The refill then retreated from a visible enemy about 1500 units away, back to
x 400, where the reserve recovered and the walk started again. The bot moved
continuously, so the 52-unit progress window never tripped, and it never reached
the scoring floor at x 1500.

Untargeted fire is now withheld while the bot stands on a known non-scoring
support. Fire at an engaged enemy, a support target or a sampled paint goal is
unchanged, as is untargeted fire on scoring ground, ramps and in the air. The
named trial now retains off-home paint, and the combat-trial CSV is unchanged by
this part. Across the 2v2 team trials, paint claimed per paid shot rises for every
tool against the unchanged-policy run (Sprayer 33.9 -> 36.1 area, Roller 27.0 ->
30.8, Bucket 520 -> 543, Mister 179 -> 188, Sniper 174 -> 187), with both parts
of this slice applied.

A second latent behaviour appeared in the same trace and is not changed here:
`seeking_after_descent_` clears only on a scoring landing. After the bot's drop to
that non-scoring floor it stayed set for the rest of the life, which disables every
ascent and ramp choice. Close Quarters lets the bot walk out, but an arena whose
only exit from a non-scoring floor is a climb would hold it there.

**What a slow tool spends during an ascent.** The four King of the Hill terrace
misses are geometric, not timing. The route's finish, `mountain_level_7`, is
exposed only as a 100-unit lip on each side, with `mountain_level_8` 100 units
above it. A Sniper's straight jet starts about 110 units from its centre, so from
the lip, or from the apex of the jump onto it, every launch that starts above the
top lands beyond the lip. Withholding fire for that landing, the first candidate
tried, made it worse (144/150): the bot then never fired at all on the lip.

The decision taken is instead: **a deliberate tool spends its shot only where a
straight launch from its current pose lands on a scoring top, and nowhere else
mid-chain.** The driver now qualifies a target for a ready deliberate tool when it
is grounded or airborne. A tile on the support underfoot keeps priority, as before.
Without one, it may take the nearest visible, in-range top on any scoring support
whose nominal straight launch passes the existing landing test. Each step the bot
re-aims at that target with the decision's single stored error draw, and fires
only if that exact angle lands. With no target during an ascent or ramp chain, it
holds fire. Visible enemies, reserves, cooldown, reaction cadence and the
navigation random stream are unchanged. The re-aim needs a decision made in
this life; before the first one, after a spawn, the bot holds as before rather
than firing ahead of its reaction delay with no drawn error.

Measured alone, this part raises Sniper paint per paid shot in the team trials
from 178.7 to 187.0 area, and its final owned paint from 374k to 384k. Other-arena finishing paint stays **146/150**: the
remaining misses are the same terrace Snipers (team a on all three difficulties,
team b on Hard), now classified as a geometric limit of that lip for this tool.
Turbulence final top paint remains 90/90, CQ deck paint 60/60 and natural-spawn
paint 120/120. Stationary local combat keeps damage and attributed eliminations
in 10/10, and all 60 moving/opposed gates hold. Their measurements move in both
directions (all-seat health loss in moving-human trials 27.380 -> 26.729,
opposed 74.812 -> 76.148, attributed opposed eliminations 40 -> 41).

**Sustained opposition.** `bot_team_trials` adds **40** sustained trials: one
Normal bot a side, a neutral human on each team (P4 claims an ordinary pad slot
and never acts), all four arenas, five tool rotations, both mirrored sides and
the three seeds across rotations, for **7200** active steps each. Every step it
samples each team's middle-band holding from the exchange ledger (new
`ExchangeLedger::held`). A new `bot_team_trials_middle.csv` records, for every
trial and team, the first holding step, held steps, losses, recaptures, the
longest gap, final-third holding and minimum, peak, and whether the enemy was
still capturing that team's middle tiles in the final third.

| Arena (sustained 1v1, 20 teams) | Enemy still capturing in final third | Held every final-third step | Lowest final-third share of peak |
|---|---|---|---|
| Crossfeed | 20/20 | 20/20 | 0.099 |
| Close Quarters | 19/20 | 20/20 | 0.199 |
| King of the Hill | 19/20 | 20/20 | 0.177 |
| Turbulence | 7/20 | 20/20 | 0.298 |

Repeatable gates: all 80 sustained bots move, buy fire and retain paint, and
every sustained team holds middle paint at every final-third step. The share of
peak is a measurement. A share floor of a tenth was tried and dropped: the
review fix below moved one Crossfeed team from 0.18 to 0.099 without it ever
losing the middle. These qualify retention capability under continued enemy
capture between equal bots; they are not balance or difficulty evidence.

**Gate reclassification.** The old mixed-tool gate required every team to retain
off-home paint at the final sample, with one named exception. That conflated
navigation with what an outnumbered side keeps. After the loop fix the one-bot
Turbulence side on seed `61403`, human b, rotation 0 holds none at the end. Its
Sprayer reached and painted the middle, died near step 1590 and lost that paint to
two opponents. The case now separates three gates:

- every bot's body leaves its home band (560/560 across all three matrices);
- two-bot teams retain off-home paint (120/120);
- the formerly named Close Quarters loop must retain it.

One-bot final retention is a measurement (119/120).

Validation: Debug and Release builds and the fifteen selected cases pass:
`bot_driver`, `bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_ascents`,
`bot_arena_routes`, `bot_hill_routes`, `bot_turbulence_routes`, `bot_trials`,
`bot_runtime`, `gameplay_tuning`, `movement`, `bot_territory`, `bot_combat_trials`
and `bot_team_trials`.
Nine final-input replays reproduce public players, projectile count and exact
paint state, including one 7200-step sustained tape, with complete fingerprints
additionally checked in Debug. All six trial CSVs, including the new middle
report, are byte-identical between builds, and both real-bank isolated startup
checks pass. Evidence: `out/validation/road10-loop/`;
the unchanged-policy baseline run is `out/validation/road10-loop-baseline/`. An
adversarial review (policy, harness and determinism lenses, each finding
verified independently) confirmed one defect, fixed above: the slow-tool re-aim
could fire before a life's first decision with a zero or stale error draw. Two
other findings were refuted and the harness lens found none.

**Next implementation:** decide whether `seeking_after_descent_` should clear on
any completed descent, and whether a refilling bot should retreat from a visible
enemy well outside that enemy's reach. Both appeared in the loop trace. Measure
the CPU cost of the per-step support scans, which now also run for airborne ready
Snipers. Moving-support routes, the full suite, native bot presentation and
physical-pad checks were not run. Attended difficulty distinction and usefulness
remain the author's acceptance gate; values remain provisional and `ROAD#10`
stays OPEN.

#### Descent latch, refill retreat, scan cost and moving supports (2026-10-27)

**The descent latch.** Clearing `seeking_after_descent_` on every completed
descent was tried first and rejected: the Close Quarters home-drop bot then
climbed straight back away from the scored bypass (`bot_navigation`, Easy
Sprayer team a, no scoring landing). The decision taken: the latch still clears
on a scoring landing, and also on the **second** stuck turn after a completed
non-scoring descent. One dead end still walks out as before; a floor blocked both
ways, whose only exit may be a climb, makes ascents eligible again once recovery
ends. `bot_driver` checks that one turn holds the latch and the second releases it.

**The refill retreat.** A refilling bot now backs away from a visible enemy only
within that enemy's own engagement distance plus `bots.refill_retreat_margin`.
The observation carries each enemy's engagement distance with its visible tool.
The first value, 200, failed the same-tool `bot_team_trials` gate: in Close
Quarters, the lone Roller facing two Rollers (trial 55) died twice and kept no
off-home paint. With the old unconditional retreat it kept 9848. 400 still failed
it; 600 and 1000 passed team and combat trials. **600** (about 0.75 s of an
enemy's full run) is the provisional value. A refill lasts about 2.1 s, in which a
running enemy covers more than the 1600-unit sight distance, so the margin is
not derived from the refill.

**Refill off enemy paint.** The default-off paint-underfoot trial
([POL#30](polish-plan.md#experimental-underfoot-prototype)) stops ammunition
recovery on enemy paint, and a refilling bot walks on toward its next paint
goal, so it could stay empty. `bots.refill_off_enemy_paint` (shipped on, inert
while the trial is off) sends a refilling bot back to the last grounded centre
it held off enemy paint and holds it still off it until the reserve resumes.
A threat retreat still wins. The observation carries the trial's state and
whether this step's support is enemy paint; `bot_driver` checks hold, return
and the no-trial and toggle-off controls.

**Baseline comparison.** Release, against `b3380db` (no changes in this slice), in
the six trial and route cases:

| Measure | Before | After |
|---|---|---|
| 2v2 paint claimed per paid shot (Sprayer / Roller / Bucket / Mister / Sniper) | 36.1 / 30.8 / 543 / 188 / 187 | 36.0 / 29.9 / 540 / 189 / 190 |
| Moving-human trials: all-seat health loss, attributed eliminations | 26.729, 12 | 26.924, 13 |
| Opposed trials: all-seat health loss, attributed eliminations | 76.148, 41 | 75.913, 46 |
| Other-arena finishing paint / Turbulence top / route gates | 146/150, 90/90, all | unchanged |
| Bots leaving the home band (same-tool, mixed, sustained) | 120/120, 360/360, 80/80 | unchanged |
| Mixed one-bot off-home retention | 119/120 | 119/120 |

`bot_trials.csv` is byte-identical. Sustained 1v1 middle retention:

| Arena | Enemy still capturing in final third | Held every final-third step | Lowest final-third share of peak |
|---|---|---|---|
| Crossfeed | 20/20 (was 20/20) | 20/20 | 0.176 (was 0.099) |
| Close Quarters | 20/20 (was 19/20) | 20/20 | 0.153 (was 0.199) |
| King of the Hill | 19/20 | 20/20 | 0.177 (unchanged) |
| Turbulence | 7/20 | 20/20 | 0.298 (unchanged) |

**Scan cost.** New measure-only case `bot_scan_cost`: 225 trials covering 1-3
bots beside one idle human, every shipped arena, five tool rotations and three
difficulties, 1800 active steps each (405,000 steps). A
filter wrapping an identical `MatchBotDriver` times its `filter_seats` call apart
from the rest of `Level::update`. Each step is classed by the driver's own
predicate: idle, support scan only (a ready deliberate tool or finishing window,
over `paint_surfaces()`), or replan (territory sampling, with any scan). The
only gate is that the case completes and every bot moves (450/450). Release on
this PC (9950X3D):

| Arena (81,000 steps) | Whole step mean / p99 µs | `filter_seats` mean / p99 µs | Rest of update mean / p99 µs | Idle / scan-only / replan step mean µs | `paint_surfaces()` copy µs (surfaces) |
|---|---|---|---|---|---|
| Crossfeed | 94 / 631 | 68 / 604 | 26 / 112 | 6 / 304 / 252 | 55 (2590) |
| Close Quarters | 69 / 637 | 43 / 614 | 26 / 101 | 6 / 349 / 218 | 40 (2838) |
| King of the Hill | 168 / 1232 | 118 / 1183 | 49 / 244 | 7 / 558 / 377 | 99 (4275) |
| Turbulence | 143 / 1141 | 106 / 1098 | 37 / 151 | 5 / 632 / 371 | 80 (5180) |
| Press Tower | 103 / 783 | 78 / 762 | 25 / 105 | 6 / 374 / 284 | 54 (2924) |
| All | 115 / 1015 | 83 / 976 | 33 / 158 | 6 / 447 / 304 | 68 |

81% of steps are idle (no scan, about 6 µs). Scan-only steps are 14% and
replans 6%. The `filter_seats` p99 is 6% of the 16.7 ms frame (7% on King of the
Hill and Turbulence). Cost grows with bot count: Crossfeed's whole-step mean is
62, 88 and 132 µs for one, two and three bots.

The scan, not the sampling, is the larger per-step cost: a scan-only step
costs more than a replan step because every candidate surface runs the landing
test and a sight check. Even its p99 is under a tenth of the 16.7 ms frame. Single-step maxima
(up to 34 ms) fall on isolated steps and move between runs: three Release runs
put them in different trials. The PC was shared with other work (17-54% load),
so they are scheduling noise, not a bound. A `paint_surfaces()` copy per scan step
(40-99 µs) is the obvious first saving if the scan ever needs one.

**Moving supports.** `MatchBotSupport` now carries a stable identity, the
level object's position in its fixed target list (zero, for fixture supports and
ramps, still compares bounds). Ascent targets, visited supports and the finishing
support compare by it. An ascent and a finishing window refresh their support's
bounds each step, and the finishing hold moves with its support. No shipped arena
has a moving support, so their results are unchanged.

The new opt-in case `bot_mover_routes` runs on the explicit
`turbulence_mover_trial`: both sides, five tools, three difficulties, 2400
steps. It has two starts: *reach*, under the side's mover on its upper approach,
and *ride*, placed on the mover:

| Start | Landed on mover | Painted mover top | Left it again |
|---|---|---|---|
| reach, before identity | 5/30 (3-20 steps) | 16/30 | 5/5 |
| reach, with identity | 11/30 (2-197 steps) | 18/30 | 11/11 |
| ride, before identity | 30/30 | 27/30 | 30/30 |
| ride, with identity | 30/30 | 24/30 | 30/30 |

Before the fix an ascent to a mover aborted on its first step, so every reach
landing was incidental. The case gates that a riding bot starts grounded on its
mover, and that every bot which lands leaves it again for other ground (no bot
stuck). Reaching and painting the mover are measurements. Riding bots leave
after their first decision, since nothing holds them; whether a bot should stay
and paint a mover is not decided.

**Native presentation.** `check-painter.ps1 -BotPlayersOnly` passes in Debug. It
now also saves Team Select with bot rows, the Results page and a one-pane match
(P1 human, P2 Normal, P3 Hard) at 720p and 1080p. Evidence:
`out/validation/road10-native/`. Join, Team, Weapon and Results bot labels fit and
read clearly at both sizes. World labels in the one-pane view are small at 720p
(about 12 px). They are drawn in world space, so a bot at the pane edge has its
label clipped with its body (P3 in the capture). Clamping is not decided.

Validation: Debug and Release builds pass, as do the fifteen cases above plus
`bot_mover_routes` in both builds, `bot_scan_cost` in Release, and both real-bank
isolated startup checks. Evidence: `out/validation/road10-latch/` (the margin-200
failure in `road10-latch-margin200/`, baseline in `road10-latch-baseline/`),
`road10-mover-before/`, `road10-latch-scan/` and `road10-native/`.

**Next implementation:** attended difficulty comparison and usefulness remain the
author's gate. Decide whether a bot should hold and paint a mover and whether
world labels should clamp to the pane. Physical-pad checks were not run. Values
remain provisional and `ROAD#10` stays OPEN.

#### Mover painting and pane-clamped bot labels (2026-10-27)

The author decided both open questions: bots paint moving platforms, and in-match
bot labels stay inside the pane.

**Mover hold.** `MatchBotSupport` marks translating supports. A bot grounded on a
moving paintable support opens the ordinary finishing window on it, and reopens
it on expiry, for at most `bots.mover_hold_steps` (600, provisional) per ride. It
needs no visible enemy, no recovery and no climb. While a bot rides, the driver
scans the ridden support's top paint every step. The hold applies only while the
driver has qualified a shot onto that mover's own unpainted top
(`support_paint` on the ridden support's bounds), not merely while some
unpainted top is in sight. The first version held on `support_open` alone.
`bot_mover_routes` then failed its "every lander leaves" gate in both builds: a
Hard Sniper (reach, team b) landed at step 1910 with 3792 of the top painted and
stood there to the 2400-step end, firing at a sliver no shot reached. Requiring
a qualified shot fixed it without changing the gate or the trial budget.
`bot_driver` checks that a moving support holds a bot for the full hold and then
releases it, and that a static platform with the same paint does not hold it.

**Labels.** A bot's world badge and label shift together into the pane's visible
world rectangle, so a bot at or past the pane edge keeps its identity readable.

**Mover routes.** Release and Debug give identical measures:

| Start | Landed on mover | Painted mover top | Longest ride (median / max steps) | Mean peak top paint | Left it again |
|---|---|---|---|---|---|
| reach, before hold | 11/30 | 18/30 | 34 / 197 | 819 | 11/11 |
| reach, with hold | 11/30 | 18/30 | 491 / 646 | 1051 | 11/11 |
| ride, before hold | 30/30 | 24/30 | 10 / 79 | 853 | 30/30 |
| ride, with hold | 30/30 | 26/30 | 92 / 625 | 2261 | 30/30 |

Twelve riding bots and five reaching bots now hold for 550 steps or more, and four
riders paint the whole top (none before). A ride can run past 600 steps by up to
46 because the bot still needs its reaction time and must walk off. No bot died.

Validation: Debug and Release builds pass, as do `bot_driver`, `bot_runtime`,
`bot_recovery`, `bot_ballistics`, `bot_navigation`, `bot_territory`,
`bot_mover_routes`, `bot_team_trials`, `bot_arena_routes`,
`bot_turbulence_routes`, `bot_hill_routes`, `bot_trials`, `bot_combat_trials`,
`player_identity`, `paint`, `paint_surfaces`, `paint_underfoot`,
`moving_structure` and `turbulence_movers` in both builds. No shipped arena has a
mover: `bot_trials.csv`, `bot_combat_trials.csv` and the four `bot_team_trials`
CSVs are byte-identical to master `bbb6c0a` (Release) in both builds.
`check-painter.ps1 -BotPlayersOnly` passes in Debug. In its one-pane 720p and
1080p captures, P3's badge and full "P3 - Bot (Hard)" label sit inside the pane,
where the earlier capture clipped them. Evidence:
`out/validation/road10-mover-paint/` (the failing first hold in
`road10-mover-paint-v1/`, the master baseline in `road10-mover-paint-master/`).

**Next implementation:** attended difficulty comparison and usefulness remain the
author's gate. Physical-pad checks were not run. Values remain provisional and
`ROAD#10` stays OPEN.

### Balanced random teams

**GAP#1.5 - implemented (2026-10-15).** In team setup, any joined player may
release **X** on a pad or **R** on the keyboard to choose Random teams. The
shuffle assigns 1v1 for two players, 2v1 for three, and 2v2 for four; either
side may receive the extra player in a three-player roster. A repeated random
result is allowed. Manual Left/Right selection remains available.

The shuffled choices remain provisional until everyone confirms. A shuffle
clears every prior confirmation and discards simultaneous confirmation or
cancellation; buttons held across the shuffle must be released before a new
press can act. Missing controllers and inactive setup block the action, as
they block ordinary selection. Solo practice has no shuffle action.

Only team assignments change: players, device slots, weapons, stage, duration,
session length and screen layout stay in place. A series keeps its committed
teams through rounds and restarts; Random teams is available only in setup.

Validation: Debug and Release builds and all 24 consumer acceptance cases pass.
The new CPU case checks 128 seeds per multiplayer roster size, both three-player
majority sides, refusal of incomplete/solo rosters, non-team settings and armed
keyboard releases. The real-page probe checks every joined requester, discarded
simultaneous confirmation, full readiness reset and required reconfirmation,
manual choices, Weapon Select return, held actions, conflicting buttons, missing
controllers and focus return. Native 720p/1080p captures verify that four player
rows and the complete X/R legend fit; solo return omits the unavailable action.
A committed shuffled two-player roster also survives ordinary Pause > Restart,
Results/End and two best-of-three rounds (draw replays use the shared next-round
path). Public player team/device/weapon observations remain fixed throughout.
Isolated real-bank startup and graceful shutdown pass in both builds. The normal
Debug `-MainMenuOnly` probe also passes with the real input backend.

Run `scripts/check-painter.ps1 -Preset x64-debug -TeamSetupOnly` (or
`x64-release`) from an x64 VS Developer shell. Only that probe binary substitutes
a scripted `GamepadReader` backend; the game and other painter modes retain
XInput. Evidence is under `out/validation/random-teams/`. These driven checks do
not close the physical-pad session (`ROAD#2`) or human readability/listening gates.

### Handicaps

**ROAD#5 - DECISION (reopened by the author 2026-10-27).** It was deferred on
2026-10-15 until couch sessions (`ROAD#2`) showed a need; with bots now offering
Easy, Normal and Hard, the author asked for it to be built.

**Mechanic (chosen by the author 2026-10-27).** Each seat, human or
bot, has a handicap level: Off, +1 or +2, shown as **Boost**. A boost makes the
player take less damage from every hit and refill paint faster. Score stays
painted surface at every level; there is no paint or score multiplier, so a
boosted player still has to cover the ground themselves. The two numbers per
level are `tuning.json` `handicaps` (`none`, `plus_one`, `plus_two`):

| level | `damage_taken` | `paint_refill` | in practice |
|---|---|---|---|
| Off | 1.0 | 1.0 | the unchanged game |
| +1 | 0.8 | 1.25 | survives a quarter more paint |
| +2 | 0.65 | 1.5 | survives about half as much again |

`none` must be exactly neutral, `damage_taken` is in (0, 1], `paint_refill` is
at least 1, and each level must boost at least as much as the one below it.
The refill boost multiplies paint underfoot rather than replacing it. Both are
applied by `Player`, so a boosted bot gets the same help as a boosted human
and its refill dwell estimate counts the boost.

**Setup.** Weapon Select shows each row's level under its label. X / R, the
Team Select random-teams button, cycles that row Off, +1, +2 while the row is
unready; P1 reaches bot rows through the existing host-row routing. The
`weapon` and `bot_weapon` prompts name the button. The level belongs to the
seat: it survives bot/human conversion, difficulty changes, resizes, random
teams, fresh claims and restarts or the next round of a series.

**Persistence.** The remembered setup and Debug replays write `"handicap"` on
a seat only when it has one, so setups and replays without handicaps keep
their bytes. Absent or malformed saved values recover to Off; a replay with
an unknown level is refused.

**Validation.** The `handicap` case checks the tuning shape and refusals,
the roster field, the Weapon Select copy and prompts, bit-exact damage at
every level (Off unchanged), the refill multiplier and respawn retention;
`remembered_setup` round-trips and recovers handicaps. Presentation
validation fits every level's text under every Weapon Select label.

**Still the author's:** judge in attended play whether +1 and +2 are worth
having and the values feel fair against Easy/Normal/Hard bots. The author
picked one Boost for health and refill over health-only, refill-only or
two-way levels; the values remain provisional until that play.

## First increment: explicit joining and reconnect pause

Implemented:

- Player count leads to Join Players. The selecting device claims P1; other
  devices explicitly claim remaining seats. P1 confirms the completed roster.
- A device cannot claim two players, and joining stops at the selected count
  (at most four). Leaving frees a seat without renumbering other players.
- Setup and gameplay show player number and device. Gameplay identity has a
  dark backing to stay readable over the level art.
- A participating controller disconnect pauses the simulation and match audio.
  The pause menu names missing players and disables Resume and Restart until
  all return. Reconnection requires an explicit resume; spare devices cannot
  take over a player. If all participants disconnect, any available device can
  navigate Quit and its confirmation.

Identity is bound to an XInput slot for the match: paint, scores, the seat's
jump binding, stick profile and rumble preference are keyed on it. A controller
that comes back in a different slot reclaims its player (`ROAD#4`, 2026-10-27):
while the pause waits, Start on a pad that no seat reads hands it the first
missing seat in roster order, and the seat reads that pad from then on, through
restarts, until the match is left. XInput cannot tell a returning controller
from a spare one, so the press is the claim; the prompt bar says "Reconnect,
then Start". The HUD and the missing list name the pad the seat now uses, and
the seat's "back" cue plays. Setup never moves a seat. A pad back in its own
slot needs no press, as before. The rule and its edge cases are checked by
`menu_navigation` (`check_seat_reclaim`); unplugging a real pad into another
slot is part of `ROAD#2`.

The [shared resume count-in](polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds)
(`POL#18`, 2026-10-15) now holds the whole live match for two seconds after
explicit Resume or focus return. Returning a pad still leaves the menu open.
Movement/aim carry through; held fire/drop-through and stale jump/pause edges
are rearmed through release. Physical-controller comfort remains in `ROAD#2`.

Validation: both normal Debug and Release builds; a temporary native roster
probe for duplicate claims, capacity, stable seats, reset and labels; and a
temporary build using scripted gamepad states at the existing reader seam for
join/leave, reserved disconnected seats, two- and four-player setup, HUD identity,
disconnect pause, explicit resume and the all-disconnected quit path. The
engine pin and engine source are unchanged. Physical controller unplug/replug
and audible suspension still need a hardware check.

## Second increment: palette accessibility

Implemented:

- Options > Team palette offers Random (the default), blue/orange, pink/green
  and blue/yellow, with the A/B assignment written beside each fixed pair.
  Apply saves the choice; leaving without Apply discards changes.
- Fixed palettes apply to new matches and restarts. Older saves and unknown
  or malformed palette fields fall back to Random, preserving valid display
  settings. Random draws from the authored `team_palette.pairs` list.
- Each gameplay HUD spells out Team A or Team B below the player/device label,
  using the existing white text and dark backing. Results retain their A/B
  markers and labelled percentages, including the one-team practice result.

Validation: normal Debug and Release builds; a temporary native probe using
the real game objects, fonts, assets and D3D11 renderer for option navigation,
wrapping, Apply/discard, save round-trips and palette fallback; a separate
process restoring the saved choice; and all three fixed palettes across all
four layouts with both team assignments. The probe checks HUD/menu bounds,
player/result colours, solo result visibility and the rematch route, and
captures gameplay, options and results for visual inspection. The engine pin
and source are unchanged.

### Team palette content (GAP#2.2, 2026-10-15)

`presentation.json` now owns `team_palette`: a `colours` array of named colours
(`name`, `colour`, using the existing named/hex colour reader) and a `pairs`
array (`key`, `label`, `team_a`, `team_b`). Each team field references a named
colour. The shipped five colours, three pairs and A/B assignments are unchanged.
Match building, fixed weapon previews and Options labels read the same block.
The default `TeamColour` value is neutral grey; match builders supply the
selected authored pair explicitly.

The fixed-preference enum and its three save keys remain stable. Every file must
include those keys once; changing array order does not change saved selections.
Extra keyed pairs join Random through a content edit, without a rebuild. Making
one of those extra pairs a fixed Options choice still requires adding its enum
and save-key mapping. The random roll uses the array length. A random weapon
preview stays neutral and consumes no draw.

Startup rejects duplicate colour names or pair keys, empty identifiers/labels,
unknown colour references, malformed colours, missing fixed preferences and a
pair keyed `random`, with file/field context. The `team_palette` consumer case
checks the original values, edits and reordered fixed preferences, expansion to
a fourth random pair, and invalid-content guards.

Validation: Debug and Release consumer acceptance suites and isolated Debug
startup passed. These are data/construction checks; no new visual or colour
accessibility qualification is claimed.

## Third increment: release package

Implemented:

- The Release `package` target produces `ColourWars-windows-x64.zip` with an
  explicit runtime asset list checked against the manifest. It includes the
  required DirectXTK, XAudio2Redist and release MSVC DLLs, plus licence notices.
- Packages exclude source art, source audio, tools, saves and private legacy
  audio. Since the 2026-10-15 runtime migration they include the validated,
  distributable replacement bank. The optional-bank silent fallback remains
  supported. Debug packaging is rejected.
- The title screen shows the game revision and build configuration.
  `build-info.txt` records full game/engine revisions and backends. Metadata
  refreshes on each build, including local-change markers.
- The executable carries a VERSIONINFO resource stamped from the root
  `project(VERSION)`, the number CPack also uses; `check-startup.ps1` fails an
  executable that is not stamped with it. CI keeps the inspected, started Release
  ZIP as the `ColourWars-windows-x64` run artifact for 14 days. No tag or release
  job exists yet.

Original package validation: Debug and Release builds; ZIP extraction into a temporary directory
outside the checkout; all 16 packaged assets matched against their sources; no
wave bank, source audio, content tools or debug symbols; launch with an isolated
save and a minimal system-only PATH; and inspection confirming the process loads
DirectXTK, XAudio2Redist and MSVC DLLs from the extracted package. The title-screen
identifier was visually checked. CI now builds and inspects the Release ZIP and
uses it for the existing startup smoke check. A clean Windows machine still
needs a separate deployment check. The engine pin and source are unchanged.
The original validation predates replacement audio; current migration checks
are recorded in [audio inventory §9](audio-inventory.md#9-runtime-migration-and-remaining-integration).

## Fourth increment: pause controls/settings

Implemented:

- Options and Pause open the same Controls / audio page, with a persistent
  controller and keyboard/mouse reference. Returning preserves the parent
  page, including pending display/palette selections, and never resumes a match.
- **Pad sticks** opens separate movement and radial aim deadzones for Pad 1-4,
  with a live raw-input/response preview. The profiles follow XInput slots,
  independently of player/view numbers. Old saves inherit authored movement
  and their former global sensitivity (70%, 60%, 50%, 35%, 20%) until edited.
  Mouse cursor aim and fixed roller aim keep their existing behavior.
- Separate music and SFX volume scale the authored mix from 0-100%, including
  menu clicks, results, paint, player and weapon sounds. Apply affects menu
  music immediately. Quiet arena music under pause picks it up on the next
  active update; hard-paused voices pick it up before resuming. Subsequent
  sounds use the new gain when played.
- Apply saves the shared preferences; the Pad sticks child applies its profiles
  separately without losing pending parent changes. Back discards unapplied
  changes. Older or malformed fields fall back independently. A live match reads
  the saved preferences, and new matches and restarts use them too. Settings
  retain pause-menu input ownership and the disconnected-player fallback.

Validation: normal Debug and Release builds and a Release startup with an
isolated save; a temporary native probe using real game objects, assets and
D3D11 rendering, with the engine's existing null audio backend for recorded
volume assertions. Checked Apply/discard, parent-page preservation, limits,
old/malformed saves, separate-process restoration, live stick input, music/SFX
gains, all five weapons, and pause/settings/resume/restart with 1-4 players.
Scripted menu input checked ownership, participant/spare fallback and reconnect
guards. Captured the shared page in all four layouts and checked widget bounds
and screenshots. Physical controller feel and audible balance still need a
hardware check. The engine pin and source are unchanged.

## Fifth increment: sessions and paint contribution

Implemented:

- Mode Select's Match length row chooses Default or 1, 2, 4, 6 or 8 minutes
  with Left/Right (confirm also cycles). Default follows `match.duration`;
  the other choices come from `match.duration_choices` in `tuning.json`.
  This is the active paint clock only: opening, countdown and reveal keep
  their own timings. The choice survives setup navigation, restarts, rematches
  and every round of a series, for solo practice as well as multiplayer.
  Debug recordings retain the exact override (replay format 2); recordings
  from older builds remain unsupported. This closes gap-audit 1.4's remaining
  duration work on 2026-10-15. Debug and Release consumer acceptance passed;
  the Debug real-Level check runs a 1.25-second override through countdown
  and into the reveal, and the replay codec preserves a fractional override
  exactly. Debug startup passed with an isolated save and graceful shutdown.
  A physical-controller menu session remains part of ROAD#2.
- Mode Select offers Single Match, Best of 3 and Best of 5. A series needs
  2-4 players and ends when Team A or B reaches two or three wins. Single Match
  keeps the one-player practice path and the existing team scoring.
- Results show the round, cumulative A/B wins, the target and the series winner.
  Draws award no win; Replay Draw starts another round with the same score.
  Draws can extend a series beyond three or five rounds.
- Next Round keeps the roster, teams, weapons and stage, with fresh paint and
  timers. Pause > Restart replays only the current round and keeps earlier wins.
  Team/weapon/stage changes return after the series ends. Play Series Again
  resets the score; quitting discards the session. Sessions are not saved.
- Every result lists each player's device, team and surface still held as a
  percentage of the arena's total paintable area. Capturing a neutral or enemy
  tile assigns its full area to the projectile's owner; enemy repaint transfers
  that credit, and friendly repaint retains the previous owner's credit.
  Repeated hits never accumulate extra area. Respawns and reconnects retain
  ownership through the match's fixed device slots. Device-less debug players
  instead keep separate ownership by view, including in the results and paint overlay.
- Contribution uses the same tiles and areas as team scoring. Individual
  percentages use the whole arena, including unpainted surface; the competitive
  team bars still show each team's share of painted surface. Displayed values
  are truncated to one decimal place, so their sum may differ slightly.

Validation: normal Debug and Release builds; a temporary native probe using
real game objects, assets and D3D11 rendering with scripted controller states.
Checked both series lengths, draws, duplicate result delivery, both winners,
neutral/friendly/enemy paint and recapture, all five projectile kinds, sparse
controller slots and keyboard ownership, mode selection, and 1-4 player result
layouts. Exercised next round, pause/restart, disconnect/resume, clinch, rematch
and quit. Checked visible text bounds/overlap and inspected rendered results
and end menus. A normal Release startup used an isolated save. Physical
multiplayer play still needs a hardware check. The engine pin and source are
unchanged.

## Review follow-up: ammunition, coverage and release checks

Implemented after the 2026-10-08 review:

- Each shot requires its full authored ammunition cost, with a small tolerance
  for floating-point rounding. Holding fire while empty waits for sufficient
  regeneration; the last paid shot still gets its sound and visual feedback.
- Level construction clips paintable faces against adjacent solid rectangles.
  Buried surface contributes neither tiles nor scoring area, while partial
  exposed edges remain paintable. Ramps retain their deliberate paint pass-through.
- Display/palette Apply validates the whole selection and saves once. Both
  settings screens distinguish saved preferences from changes active only for
  the current run when writing fails; the old saved file remains intact.
- Native consumer acceptance cases are retained under `scripts/acceptance/` and
  run in both CI presets through `scripts/run-acceptance.ps1`. The standalone
  startup script checks isolated saves, the actual game window, diagnostics and
  graceful shutdown. Application errors fail CI; Release's exact missing-GPU
  diagnostic is the only permitted infrastructure skip.

Validation: Debug and Release configure/build and all retained acceptance cases
passed under `/W4 /WX`. The rebuilt Release ZIP passed content/DLL inspection
and startup with a system-only PATH, unrelated working directory and isolated
save; Debug startup also passed. Both closed gracefully. Malformed content,
crashes and misleading adapter-error messages were rejected by the startup
script; its exact-adapter skip was checked separately. The engine is unchanged.

The next acceptance work requires hardware and is still outstanding:

1. Run the extracted Release ZIP on a clean Windows 10/11 x64 machine.
2. Play two- and four-player sessions across the three arenas and five weapons,
   checking ammo exhaustion/recovery, paint coverage, readable HUDs and controls.
3. Exercise joining, disconnect/reconnect during setup and play, pause/settings,
   focus changes, fullscreen/resize, series draws, rematches and return to menu.
4. Complete the [replacement-audio acceptance](polish-plan.md#5-replace-the-entire-audio-set)
   as that work lands: new music and effects, music/SFX gains, muting, suspension,
   resumed weapon loops and the multiplayer mix. Also unplug headphones and
   switch the default output mid-match. The pinned engine does not recover a
   lost device ([ledger](review/engine-reports.md#what-is-deliberately-not-here)),
   so record what is heard. The ZIP now includes the
   replacement bank; qualify actual playback from the extracted release and the
   missing-bank silent fallback separately.
5. Relaunch and confirm saved settings; record device models, display layout,
   build revision and observed failures before choosing the next feature increment.
6. Leave a pad-only series running past the configured display-idle timer and
   observe display behaviour, then normal idle behaviour after leaving the match.
   Observe screen-saver and user-requested Sleep behaviour separately; `ROAD#6`
   changes neither policy. Bring one non-XInput pad (DualSense, DualShock or
   Switch Pro) without Steam Input, so the [XInput-only limit](#input-devices-and-seats)
   is observed rather than assumed.

## Couch-session gaps (2026-10-24 review)

A 2026-10-24 review checked the current code against the frozen gap audit and
found its first-class list almost entirely implemented. What follows is what it
found untracked that belongs to this roadmap. Spawn protection went to
[`POL#17`](polish-crossfeed.md), the unborrowed Splatoon mechanics to
[`POL#30`](polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30), and
audio-device loss to the [ledger](review/engine-reports.md#what-is-deliberately-not-here).

### Display sleep during a match (`ROAD#6`)

**Implemented (2026-10-24).** Each `GameLevel` owns one `DisplayAwake`
for its match/series lifetime. Construction attempts
`ES_CONTINUOUS | ES_DISPLAY_REQUIRED` once; destruction sends
`ES_CONTINUOUS` only after successful acquisition. Acquisition failure is
nonfatal with no retry; clearing is one nonthrowing best-effort attempt.
Pause, focus, restart and round transitions retain the same owner rather than
refreshing the request per frame. Pause/settings/results overlays retain it
until the `GameLevel` exits.

This is a display-idle request only: no `ES_SYSTEM_REQUIRED`,
`ES_AWAYMODE_REQUIRED` or screen-saver policy changes, and no override of
user-requested Sleep or lid/power-button settings. The
[client adapter](../game/display_awake.cpp) is the exhibit for the
[parked shell-ownership ask](review/engine-reports.md#what-is-deliberately-not-here).

Validation: Debug/Release configure, game/editor builds and engine include
checks passed, as did the focused `display_awake`, `debug_console` and
`story_audio` CPU cases in both presets. The injected CPU helper checks
ownership/failure paths and synthetic containing-owner unwind; it proves no
actual `GameLevel` constructor/init failure. Native recording passed nine
lifecycle cases per preset, covering eight actual `GameLevel` constructions,
five inits and one restart. Its fifteen events are recorded boolean intent,
not observed Win32 flags or OS state; the probe makes zero real display
requests. Scripted failed clearing proves an attempt and nonthrowing teardown,
not a successful physical OS clear.

From an x64 VS Developer shell, run `scripts/run-acceptance.ps1 -Preset x64-debug`
(the suite includes the CPU case), and `scripts/check-display-awake.ps1 -Preset x64-debug`
for the isolated lifecycle probe; repeat with `x64-release`. Qualified
build/CPU/native receipts are under
`out/validation/todo-loop-20261024/road6-prep/`. Physical pad-only display-timer
and separate screen-saver observations remain item 6 above in `ROAD#2`.
The existing `0bd8f45` Release ZIP predates ROAD6; no replacement package was
produced for this increment.

### Per-seat accessibility toggles (`ROAD#7`)

**Implemented 2026-10-27; a look on 1-4P panes with pads remains (`ROAD#2`).**
View shake, the aim marker and Reduce effects were single `PlaySettings` fields
that applied to every pane, so on a shared screen a motion-sensitive player
could not opt out alone. Each pad slot now has its own **Off / Shared / On**
override for all three, following the stick-profile and rumble pattern.

- **Shared** is the default and follows the Options switch, so an older save
  behaves exactly as before. The Pad sticks page shows it as `Shared (On)` or
  `Shared (Off)` so the effective value is never hidden.
- **Reduce effects is both.** The Options switch still governs everything on
  the shared screen: menus, the intro, the countdown, the reveal and world
  effects every pane draws (muzzle flash, figure motion, drips, structures). A
  pad's override governs only what that pad's player sees: its own view shake,
  its pane's HUD pulses, and the badge emphasis in its pane, including bot
  badges drawn there. A pad's Reduce effects still suppresses its own shake
  while keeping its separate View shake choice.
- The keyboard seat, bots and unknown slots always follow the shared switches.
- Edited on the Pad sticks page (rows 4-6, after Rumble), committed by its
  Apply for the edited slots only; the Controls page's Apply keeps them. Saved
  as `pad_comfort`: four `{"view_shake", "aim_marker", "reduce_effects"}`
  records holding `"off"`, `"shared"` or `"on"`, each field recovering to
  Shared independently.

CPU acceptance: `settings` (resolution, slot isolation, recovery, canonical
writing), `view_shake` (per-slot shake, Shared fallback, keyboard/bot),
`aim_marker` (sparse-slot panes) and `player_identity` (own versus other
viewer's badge emphasis). The painter fixtures cover the longer page.

### More team palette pairs (`ROAD#8`)

**Done 2026-10-27; the author accepted the three new pairs after seeing them.** Six pairs
ship. The three new ones are fixed Options choices (an enum entry and save key
each) as well as members of the Random draw, so each one widens the
colour-vision route around the roll:

| key | Team A | Team B | worst pair difference | nearest surrounding |
|---|---|---|---|---|
| `purple_gold` | purple `7B1FA2` | gold `FFC20A` | 43.6 (tritan) | 18.9 |
| `red_azure` | red `E0201B` | azure `1E90FF` | 50.9 (normal) | 18.5 |
| `magenta_forest` | magenta `FF00FF` | forest `269900` | 55.6 (deutan) | 23.2 |

**The bar.** The `team_palette` case simulates protan, deutan and tritan vision
with the Machado, Oliveira and Fernandes (2009) full-severity matrices in
linear sRGB and measures CIEDE2000. A pair passes when its two teams stay at
least 40 apart under normal and all three simulations, and each team colour
stays at least 15 from every surface colour the levels author (read from
`game/content/levels/`) and from the HUD health bar. Every shipped pair must
pass except the two legacy pairs, which are kept as they shipped and listed by
name: `pink_green` (27.3 under deutan, and its green is 5.9 from the health bar
under tritan) and `blue_yellow` (yellow is 10.3 from white). The exemption
list fails if either one starts passing, so a recolour has to take it off.
`blue_orange` passes (58.1, 22.9).

A content-only extra pair still joins Random without a rebuild, and now has to
pass the same bar. [The preview sheet](images/road8/team-pairs-colour-vision.png)
shows every pair on a Gainsboro surface under each simulation. Nothing here
qualifies the colours on screen: the opt-in visual cases that capture palettes
(`paint_fx_visual`, `painter_visual`, the console's opening art check) still
name the original three.

### Controller rumble (`ROAD#9`)

**Local implementation qualified, 2026-10-26; physical feel remains a decision.** In a four-pad couch game rumble is the only private
per-player channel: the screen and speakers are shared
([gap audit F6](review/gap-audit.md)). The engine's `GamepadReader` exposes no
vibration, so this is a client feature that also produces the exhibit for an
engine report.

1. **Client slice.** Short, distinct pulses for being hit, being eliminated,
   a hard landing and a dry trigger, read from `tuning.json`. A per-seat On/Off
   beside the stick settings, saved with them, default On. Bots, replays and the
   keyboard seat never rumble. Pause, focus loss and the end of a match stop
   every motor. The call is a direct `XInputSetState` on the seat's slot,
   isolated in one file because it bypasses the engine's reader.
2. **Report.** That one file is the exhibit for `GAP#F6`'s ask,
   `Gamepads::set_vibration` with the `state()` slot contract, drafted in the
   [ledger](review/engine-reports.md). Filing needs the author's authorisation.

Acceptance is a CPU case over a fake vibration sink: which events pulse which
seat, the per-seat setting and every stop condition. Feeling it needs pads
(`ROAD#2`).

The local work has bounded stopping points; each leaves a reviewable increment.

**Pulse policy implemented, 2026-10-26.** `Rumble` borrows a nonthrowing sink
and accepts caller-supplied profiles with normalized motor amplitudes and
durations from zero to one second. Each event has its own timer; overlaps take
the maximum per motor, and a repeated event refreshes only its own timer.
Physical-slot and session gates cancel pulses instead of suspending them, so
reconnect, re-enable and resume cannot revive old feedback. Explicit stop and
destruction request zero for all four physical pads.

The `rumble` fake-sink case passed in Debug and Release, covering all four
events, overlap/expiry/repetition, sparse slots, keyboard/bot/replay/disabled
and disconnected suppression, every stop condition and invalid-input atomicity.
Receipts are under `out/validation/overnight-20261026/` (`runtime` and `release`).
The policy test values are not authored cue tuning or a physical-feel result.

**Data and event intents implemented, 2026-10-26.** `PlaySettings.pad_rumble`
holds four default-On slot flags; the save writes `pad_rumble` as four
`{"enabled": bool}` records, recovering missing/malformed entries independently.
Four named pulse records now live in `tuning.json` with contextual finite/range
validation. Their values are provisional until physical-pad review.

Actual nonfatal damage, the once-only elimination transition, the existing hard
landing decision and the rate-limited empty-trigger warning emit coalescing
feedback flags. Player merges its weapon's flags; Level drains them with each
player's physical input slot. Placement/respawn and explicit discard clear them.
There is no hardware call in those gameplay objects; GameLevel output follows below.

Debug/Release `rumble`, `settings` and `gameplay_tuning` pass, including actual
callers, sparse slots, frozen/spawn/cooldown/immune controls, save recovery and
failure, and exact gameplay/RNG/Debug-fingerprint invariance while draining.
`combat_feedback`, `weapon_feedback` and `movement_contact` also pass in both
presets. Phase-2 receipts are under `out/validation/overnight-20261026/phase2/`.

| slice | deliverable and acceptance |
|---|---|
| Report filing | [Report 25](review/engine-reports.md#25-no-public-controller-vibration-output--draft-with-isolated-adapter-exhibit) is prepared against published engine master `c03a6f6`. Filing needs separate author authorization and a fresh upstream recheck. |

**Owner, settings page and native adapter qualified.**
Pad sticks now has a per-slot Rumble row, with pending/Apply/Back semantics.
It merges only explicitly edited rumble slots; the suspended Controls parent
preserves the child's committed rumble flags alongside its stick settings.
Authored help/status and row/font/bounds validation include that setting.

GameLevel owns the sink and policy, ages pulses once per outer update and
consumes every completed simulation step's events by physical input slot.
Focus, coverage, pause/count-in, Debug pause/overlay, automation through
`drives_slot`, connection and active-match state gate output. Stops discard
pending events; rebuild stops the old policy before Level teardown and adopts
the current authored profile. Time-up stops at the transition out of active
play, before the later Results page. Sink lifetime covers policy destruction
and initialization/rebuild unwind.

`game/rumble_xinput.cpp` isolates SDK types, normalized-to-16-bit conversion,
`XInputSetState`, failure handling and the transport-level lifetime stop. It
links SDK `Xinput.lib`, matching the current DirectXTK reader's XINPUT1_4 import.
The injected overload checks conversion/invalid-input/failure/stop intent without
calling the native transport. Failures are nonfatal, without retries or log spam.

Every painter mode, the separate display-awake lifecycle harness and the
frame-pacing harness omit that native object and supply the recording default
factory, including indirect GameMenu-to-GameLevel construction. `RumbleOnly`
adds actual owner lifecycle/automation checks and a settings-page witness with
isolated saves. The replay gate witness deliberately injects a damage event
and stops before its next checkpoint; it establishes output suppression, not
new replay determinism.

Both game builds, `rumble_adapter`, `rumble`, `settings`, `stick_settings`,
`menu_presentation`, `gameplay_tuning` and isolated system-path startup pass in
Debug and Release. Release `menu_text` passes. Both-preset `RumbleOnly` runs
passed: Debug records 15 owner cases and Release 11, with two sinks constructed
and destroyed in each. Each also passes eight actual settings-page cases,
including sparse owner isolation, pending/Apply/Back, save failure/retry and
parent/edited-slot merges. Their fake factories record zero native motor calls.
The separate display-awake lifecycle probe also passes both presets with the
native rumble object omitted. The retained settings images were visually
inspected. Phase-5 receipts are under `out/validation/overnight-20261026/phase5/`.
The affected Debug Controls/Sticks/percentage matrices pass with six Sticks
rows and the corresponding 144-update percentage budget. A stale child-Apply
navigation fixture was corrected without changing production focus behavior.
The frame-pacing harness compiles with its fake factory; no performance capture
was run. The metadata audit narrows RumbleOnly to its actual three-player sparse
roster and default Random palette selection, with no palette comparison.

Do not close the physical feel review with fake-sink results. The native hook
does not require a pad to build, and the implementation does not claim
that its four candidate pulses feel distinct or comfortable.

### Input devices and seats

Known limits, recorded so a later gap list does not file them again. The first
now has a row.

- **XInput only.** DualShock, DualSense and Switch Pro pads work only through
  Steam Input or a wrapper. The engine's reader seam is one function wide
  (`engine/input/gamepad_reader.h`), so a GameInput backend is Labrador work and
  would need an exhibit before it is a report. `ROAD#2` observes the limit
  (item 6 above). The author asked for it to be tracked on 2026-10-27
  (`ROAD#23`, [comparable-game gaps](#comparable-game-gaps-2026-10-27)).
- **One keyboard seat.** That seat aims with the mouse, so a second keyboard
  player needs a keyboard-only aim model. That is a design question, not a
  missing binding. It gets a row only if the author asks for two players on one
  keyboard.

## Comparable-game gaps (2026-10-27)

A 2026-10-27 comparison against Splatoon, TowerFall, INVERSUS, Super Smash
Bros. and Paint Rumble listed what those games have and ColourWars lacks.
Online play and new game modes were out of scope. The author asked for every
gap rated medium value or above to be tracked; the rows are `ROAD#11` to
`ROAD#24`. Value is how much a couch group would notice the feature; the size
is a first slice, before attended review. None is scoped yet.

Two gaps were already tracked and get no new row: a special charged by
painting is `POL#30`, and more arenas are the current aim (`POL#21`,
`POL#31`).

### Combat and tools

- **Sub weapons (`ROAD#11`).** Splatoon pairs each main weapon with a sub
  (bombs, Splash Wall, Sprinkler) that costs a large slice of ink. `Player`
  already takes a secondary weapon (always `grenade_std`) and ignores it.
  A sub changes the reserve and balance game, so it waits for `POL#27`; bots
  (`ROAD#10`) would need to use it.
- **Results awards (`ROAD#12`, done 2026-10-27).** Splatoon 3 medals and
  TowerFall awards give each player a few call-outs after a match. See
  [Results awards](#results-awards-road12) below.
- **More tool archetypes (`ROAD#13`).** Splatoon's brush, blaster, splatling,
  dualies and brella have no counterpart among the five tools; a fast-moving
  painter or a shield tool would differ most. Each new tool needs art, audio
  and bot ballistics, and waits for `POL#27`. A new subclass suits the OOP
  rule.
- **Map pickups (`ROAD#14`).** TowerFall chests, Paint Rumble power-ups and
  Smash items. Needs a match toggle and bot awareness, and waits for `POL#27`.

### Stages

- **Paint-activated stage objects (`ROAD#15`).** Splatoon's propeller lifts
  rise while inked, inkrails open for the team that inks their bulb, and
  sponges grow. The [horizontal movement capability](#horizontal-movement-capability-2026-10-26)
  supplies the motion; paint ownership driving it is new. Arena use follows
  `POL#21`, and bots must ride the objects. The first slice, the
  [paint ferry](#paint-ferry-road15-first-slice), is on an explicit trial stage.
- **Player-made stages without rebuilding (`ROAD#16`).** A stage saved by the
  editor played only after a manifest edit and a rebuild. First slice done
  2026-10-28: a per-user stage folder, each file validated on its own, a
  broken one refused without stopping startup; manifest stages stay
  never-optional. The editor's Save to My Stages, a rescan as Stage Select
  opens and a README note for players followed the same day. Attended play
  and two author calls remain. See [User stages](#user-stages-road16) below.

### Awareness and presentation

- **Overview map (`ROAD#17`, decision).** Splatoon's map shows ink and
  teammates. The three-player layout leaves the fourth quadrant unused, which
  makes it the natural place. A map shows territory mid-match, which bends the
  withheld-score decision (gap audit 1.1), so the author decides first.
- **Off-screen teammate and threat markers (`ROAD#18`).** Splatoon shows
  allies through walls. Relates to the `POL#3` rear-threat review and the
  `ROAD#10` label-clamping question.
- **End-of-round instant replay (`ROAD#19`, done 2026-10-27).** TowerFall
  replays each round's final seconds and can save a GIF. The End menu now
  offers Watch Replay; see [Instant replay](#instant-replay-road19) below.
  Saving a clip was not taken up.

### Sessions and setup

- **Re-pick between series rounds (`ROAD#20`).** Smash re-picks each game
  and TowerFall rotates levels. Done 2026-10-27: an unfinished series' End
  menu offers Change Weapons and Change Level under Next Round. They open the
  setup's own Weapon and Stage Select above the match, in the End menu's
  frame, so the series ledger survives. Weapon Select carries each seat's
  Boost, so handicaps re-pick too; Back from either page returns to the End
  menu, and the Stage confirmation plays the next round with the new picks.
  Teams, the roster and bot difficulty stay locked, because the ledger counts
  team wins and a seat swap would change who is winning. The re-picks last for
  the rest of the series only; the setup the main menu remembers is the one
  that started it. Done 2026-10-28: a Series stage rule on the Mode page (Same
  stage, Rotate, Loser's pick); see [Series stage rule](#series-stage-rule-road20)
  below. Left: the author plays the rules and says which they like.
- **Player profiles (`ROAD#21`, done 2026-10-27).** Smash name tags carry a
  person's controls, rumble and records. Deadzones, rumble, binds and
  accessibility toggles followed the pad slot (`POL#18`, `ROAD#7`, `ROAD#9`,
  `GAP#4.2`), so swapping controllers swapped settings. A pad can now wear a
  named profile that carries them. See [Player profiles](#player-profiles-road21)
  below.
- **Match variants (`ROAD#22`, decision).** TowerFall variants, Smash rules
  and INVERSUS custom rules: low gravity, endless paint, tool roulette. Scoring
  stays painted surface and many are tuning override sets, but the match scope
  decisions rule out a speculative `MatchRules` seam. The author decides
  whether variants count as modes.

### Input

- **Non-XInput controllers (`ROAD#23`).** INVERSUS and most Steam releases
  accept PlayStation and Switch Pro pads. The reader is Labrador's (see
  [Input devices and seats](#input-devices-and-seats)), so the engine part is a
  report with an exhibit; Steam Input covers it if the game ships on Steam.
- **Pad aim assist (`ROAD#24`).** One mouse player can join pad players, and
  mouse aim is much more precise. Relates to `POL#3`, `POL#18` and the
  `ROAD#5` handicaps.

### Not tracked, by choice

The author left out the gaps rated low or low-to-medium value: respawn travel
to a teammate (Splatoon Super Jump), automatic catch-up for the team behind
(TowerFall autobalance), a last-minute music change, quick signals ("This
way!"), a lifetime stats screen, and cosmetics or unlocks. New modes
(Splatoon's objective modes, Tricolor and Salmon Run, free-for-all painting,
TowerFall's elimination and co-op modes, INVERSUS arcade, Smash stock
battles) stay excluded by the [match scope decisions](#match-scope-decisions-2026-10-15).
A later gap list should read these absences as intended.


### Results awards (`ROAD#12`)

**Done 2026-10-27.** The results page gives each player at most one call-out,
right-aligned to the score bar on that player's contribution row and revealed
with the other details. There are four awards, in this order:

1. **Most paint laid**: the most area captured over the match, from neutral or
   from the other team. Friendly repaint lays nothing.
2. **Most retaken**: the most area recaptured from the other team.
3. **Best airtime**: the longest flight that ended in a landing. A fall to
   elimination never lands, so it never counts.
4. **Most splats**: the most eliminations credited to the player's projectiles.

Each award goes to every player tied for the lead with a nonzero tally, unless
that player already shows an earlier award. It is never passed down to a
runner-up, so a player who leads everything shows only Most paint laid and
nobody gets the rest. A lone player gets no award. Values are not shown.

The order is the design. Paint comes first and splats last, so a splat is
named only for a player who led nothing else, and no count of eliminations
appears anywhere. That is the answer to the splat-tally lever in
[`POL#30`](polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30).

The tallies are passive. Paintable structures add a tile's area to the
capturing owner when it changes colour (`PaintCredit`, reported with
`paint_total()`), the player banks its longest landed flight, and splats reuse
the existing lethal-projectile credit. Level gathers them into `LevelEndInfo`
by paint owner, and `LevelEndInfo::awards` picks the call-outs. The award copy is
`menu_text.results.award_*` in `presentation.json`; validation requires each to
fit the Results panel at the end of every row. A row whose own text is long
enough to reach the award keeps its text, and the page leaves that award out;
the shipped copy is short enough that this never happens.

The `results_awards` acceptance case checks the choice, the capture credit, the
airtime of a real jump on a floor, the default copy, and that every shipped
award clears the widest possible row by at least 24 units (28 at present, for
Most paint laid). `paint` checks credit on a real structure.

### Paint ferry (`ROAD#15`, first slice)

**2026-10-27.** The first paint-activated stage object is the **paint ferry**:
a [horizontal mover](#horizontal-movement-capability-2026-10-26) that travels
only while one team holds more of its own paint, toward that team's heading.
Team A's heading is authored (`team_a_heads`); team B's paint always pushes the
other way. On a tie, including an unpainted ferry, it holds still, and it stops
at either end of its travel. It moves at a cycling mover's leg speed, so the
existing admission and displacement budget is unchanged.

Paint in ColourWars stays where it lands, so a ferry is a tug of war rather
than Splatoon's ink-to-power lift: the team that owns it rides it forward, and
the other team takes it back by painting over it. Riders, carry, paint,
shadows and projectile sweeps are the mover's own; each step reads the paint
held before the step, so a shot that lands this step steers the next one.

It ships on one explicit-only stage, **Turbulence Yard - Ferry Trial**
(`turbulence_ferry_trial`): the Mover Trial with both movers turned into
ferries, team A heading right. Each starts at its own team's end, so painting
your own ferry carries you toward the middle. Arena use still follows
`POL#21`. The format is in
[Horizontal paintable motion](level-format.md#horizontal-paintable-motion).

Bots need no new policy. A ferry is a moving support, so a bot that rides one
holds there to paint it, which drives it. The `paint_ferry` acceptance case
checks the motion rules directly, then plays three whole 120-second matches
with three bots on the trial stage and holds every step of both ferries to the
paint held before it. In a Linux build of the same objects, ferries moved in
all three matches and bots rode a moving ferry in two of them (121 to 136
steps). Riding is a measurement, not a requirement.

**Editor drive checks (2026-10-28).** The level editor authors a ferry in
the inspector: `motion/drive` (shown as `cycle` when absent) and
`motion/team_a_heads`. Switching a mover to `paint` adds team A heading right
unless it has a heading, and switching to `cycle` or clearing the drive drops
it. The drive rule itself (`decode_drive`, `validate_drive`) moved into the
engine-free `game/horizontal_motion_contract.h`, which the game's loader and
the editor both call, so the editor refuses at load and on every edit exactly
the drives the game would refuse, in the game's words. See
[Moving-structure authoring](level-editor.md#moving-structure-authoring-gapd5).
The palette has no ferry entry of its own; a ferry is a moving paintable whose
drive is switched to paint.

Still open in `ROAD#15`: lifts and other vertical motion (the mover is
horizontal only), team-gated rails and growing sponges, ferry art that shows
its heading (the editor draws a ferry like any mover), and attended play. The
stage-select painter checks (`check-painter.ps1 -StagePaperOnly`,
`-StageCountsOnly`, `-StageSummaryOnly`) passed with the tenth stage on MattPC
in Debug and Release (2026-10-28).

### Instant replay (`ROAD#19`)

**Done 2026-10-27.** After a timed round, the End menu offers **Watch Replay**
on row 1, under the continuation, and the setup actions and Exit move down a
row. It plays the round's last `match.replay` seconds (8 in `tuning.json`)
with a REPLAY card at the top of the screen, then opens the End menu again over
the moment time ran out. Confirm, Back or Start ends it early. The panes are the
round's own: the same human panes, HUDs, clock and paint. The arena music and
the round's sounds play, and pads do not rumble. Watch it as often as you like.
Untimed practice has no End menu and so no replay.

**How.** It is the Debug record/play idea (`DBG#2`) in Release, held in memory.
Each round draws its own seed from the running `rand()` sequence just before
it is built, and `RoundTape` (`game/round_tape.h`) keeps that seed, a copy of
the setup (a re-pick page can change the match's own and then be backed out
of), every participant's final input on every logic step, and where the round was
suspended (which clears jump buffers) or resumed (which re-reads held jumps).
`GameLevel` records through `RoundTapeRecorder`, the seat filter every live
update passes through, behind the Debug filter when there is one. Watch Replay
destroys the finished level and builds it again from the seed, with no intro
and the countdown skipped. It runs the steps before the replay's window with
the sound groups at zero, up to 12 ms of steps a frame, which draws the round
at speed under the REPLAY card. Then it plays the window at the normal rate.
The rebuilt level's bots do not run; every seat comes from the tape. The tape
is the same build, machine and step, within one round, so nothing is written
to disk and no format is versioned.

The Debug tools can change a round between steps. Place, respawn and kill
spoil the tape, and so does changing a cheat after the first step. A spoiled
tape is not offered, and the End menu then has its usual rows. A live `tune`
edit or a suspend injected by `play` is not seen. A replay of such a round may
differ from what was played, which is acceptable for a developer build.

The End paper grew from 820 to 900 units tall, with rows 118 apart from y 300
(previously 125 from 330), so six rows fit with the item font's descenders.
Validation checks Watch Replay on row 1, the setup actions on both their rows,
and Exit on rows 1 and 5.

On MattPC (2026-10-27), `check-painter.ps1` with `-EndMenuOnly` (which runs the End
headings and series re-pick checks too), `-RoundHeadingOnly`, `-SeriesScoreOnly` and
`-SessionResultOnly` passed in Debug and Release. `-EndContinuationOnly` failed because its
fixed comparison crop sat at the old row position. The crop now starts 30 units higher with the
rows, and `-EndContinuationOnly` passed in Debug and Release on MattPC (2026-10-28).

The `instant_replay` acceptance case plays a 20 s round on each of the five
arenas and the Ferry Trial, whose movers follow the paint, as `GameLevel` plays it: a full intro, three shipping bots, a scripted
pad human and a pause with a held jump. It then rebuilds the round as Watch
Replay does and requires every taped step to match: player boxes, states, team
colours and projectile count, then the final paint. Leaving out the pause's
gaps diverges on Press Tower, and a wrong seed diverges at the first step. The
case also checks the tape rules.

### Player profiles (`ROAD#21`)

**Done 2026-10-27.** A profile is a name of up to ten characters (A-Z, 0-9 and
spaces) carrying everything a person sets for their pad: both stick deadzones,
rumble, the three comfort switches (View shake, Aim marker, Reduce effects),
the pad buttons and a Boost. There are at most eight, saved in a `"profiles"`
section; the save is version 4. A damaged entry keeps the fields that read and
falls back for the rest, and an entry with no usable name, or a name already
taken, is dropped.

A pad wears at most one profile and a profile is worn by at most one pad. While
a pad wears one, the pad-slot settings every reader already uses (`Save::play_settings`,
`Save::input_bindings`) show the profile's values for that slot. Pad sticks and
Pad buttons edit the worn profile rather than the slot, and their Pad row
names it. The slot's own settings stay saved underneath and come back when the
profile comes off. Wearing lasts for the run and is not saved, because a pad
slot is not a person: the next session starts with every pad bare. A seat that
reclaims its player after a disconnect (`ROAD#4`) keeps its identity slot, so
the profile it wore comes back with it. The keyboard and mouse seat has one set
of settings already and wears nothing.

Profiles live under Options, Controls, Profiles. The page chooses a profile,
renames it with the pad (Up and Down change the letter, Left and Right move,
A keeps, B cancels), sets its Boost, chooses which pad wears it, makes a new
one and deletes one (press twice). A new profile copies the settings of the
pad that pressed, so a person who has already tuned a slot keeps that tuning.
Every change saves at once, so the page has no Apply. On Join, X or R cycles a
joined pad through the profiles no other pad is wearing; the row shows the
profile's name, and the seat takes the profile's Boost. A menu-started match
writes each human pad seat's Boost back to its profile, so a handicap chosen
on Weapon Select is there next time. Records stay per stage, not per person.

The `player_profiles` acceptance case checks the names, the overlay and its
write-through, cycling, rename, delete, Boost carry-back and recovery from a
damaged section. `presentation.json` validation fits the page's copy and the
widest worn name on Join; the Pad rows scale it to fit like any setting value.

On MattPC (2026-10-28) the painter checks the Controls page and its children
touch (`check-painter.ps1 -ControlsMenuOnly`, `-MenuMotionOnly`,
`-PausedOptionsOnly`, `-KeyboardJumpOnly`, `-RumbleOnly`, `-PercentageTextOnly`,
`-StickSettingsOnly`, plus `-MenuTextOnly`) pass in Debug and Release. The first
run found one real defect: the heading at y 54 is 115 tall and reached 3 px into
the first row's focus-card envelope on Controls and Pad sticks, so the rows now
start at y 186 (was 180). Startup validation checks the heading only against the
focus padding, which is why it passed. The rest were stale probe fixtures: the
pinned prompt list lacked the two profile prompts and Join's `X/R Profile`, a
Home fixture still drew the card POL#37 removed, the percentage probe's exact
update count grew by the Profiles row's nine updates (180 to 189), and two
probe resets wrote through the working copy `play_settings()` now returns
instead of saving.

Still open: play with real pads.

### User stages (`ROAD#16`)

**Scope (2026-10-28).** A player who saves a level with the editor should be
able to play it by starting the game, with no manifest edit, CMake list or
rebuild. The slice is the smallest that a player can use: a per-user folder
the game reads at startup, the same checks a shipped stage gets, and a refusal
that costs the player one stage rather than the game. Shipped stages keep the
engine's content-root rule (beside the executable, named by the manifest) and
stay never-optional: one the manifest names and the disk lacks still fails
startup.

**Done 2026-10-28.** The folder is `%LOCALAPPDATA%\ColourWars\stages\`,
beside the save (`directory_consts::user_stage_directory`; the
executable-relative `save/stages` when there is no `LOCALAPPDATA`, as for the
screenshots and replays). The game does not create it. After the manifest
loads, `load_user_stages` (`game/stage_loading.h`) reads every `*.json` directly
in it, in file-name order, and checks each on its own:

- the file name is the stage's id: 1 to 64 ASCII letters, digits, `_` or `-`,
  not starting with `-` (the registration helper's rule, so the file can
  later be registered unchanged);
- no stage already has that id, compared without case: a user stage cannot
  replace a shipped one or another user stage;
- the level format and structure (`LevelLoadedInfo::validate`) and a drawable
  `display_name`, which is how the manifest's `level` kind reads a shipped
  stage (`read_stage_definition`, now shared by both);
- the per-stage halves of `validate_game_content`, split out as
  `validate_stage_objects` (built objects and resources, tunnelling, match
  sounds) and `validate_stage_text` (name and lesson fonts);
- the sound groups (`sound_groups::check_candidate`): a stage may not play as
  music a sound the game plays as an effect;
- the arena counter (`ARENA 11 / 11`) still fits with one more stage.

A file that fails any of these is left out and prints one line on stderr,
`user stage refused: <file>: <reason>`, where the reason is the same
diagnostic a shipped stage would fail startup with. Startup goes on. A missing
or empty folder says nothing; a file where the folder should be is one line.
Other files and sub-folders are ignored. The accepted stages then pass the
whole-list `validate_game_content` and join the sound groups with the shipped
ones; because each passed the same checks first, that cannot fail on them.

User stages follow the shipped ones in Stage Select, in file-name order, and
count in its arena counter. The briefing card marks one with its own hint,
"Built by you." over "Confirm to lock your choice." (`menu_text.stage.user_hint`),
in the browse hint's place; nothing else on the page moves, so the Stage
painter fixtures are unchanged. `+stage <name>` starts one from the Debug
command line.

Records and replay work with them, because both are keyed by the stage id: a
finished round records under the file name, Stage Select and Results show its
best, and the remembered setup restores it while the file is there (falling
back to the first stage, as for a retired stage, when it is not). Instant
replay rebuilds the round from the loaded definition in memory. The Debug
`record`/`play` files name the stage; playing one needs the file in the folder,
and a file edited since the recording plays its new geometry, which the replay
does not detect.

Provisional decisions, open for the author:

- **Random.** User stages are never Random candidates, whatever their
  `include_in_random` says; the Random hint's count is unchanged. An unfinished
  stage turning up in a party's Random pick seemed worse than having to choose
  it. Honouring an explicit `"include_in_random": true` would be a one-line
  change.
- **Names and records.** The id is the bare file name, the same namespace as
  the shipped stages, so a user stage keeps its records when it is later
  registered. The cost is that a future shipped stage with the same name
  refuses the user stage and inherits its records.
- **Marker.** The hint is the only marker. A mark on the name or preview would
  move the page and its painter fixtures.

The `user_stages` acceptance case points `LOCALAPPDATA` at a scratch folder
before it asks for the directory, and scans scratch folders beside its own
executable (one with a non-ASCII name): a missing, empty and non-folder path;
two valid stages, one asking for Random; and broken JSON, a level without
spawns, a missing sprite frame, a music/effect conflict, a case-only collision
with Crossfeed, two bad file names, an undrawable name, a text file and a
`.json` folder. It requires the valid two in order after the shipped ten,
marked and outside Random, each bad file refused in one line naming the
cause, the shipped Crossfeed untouched, and the whole list then passing
`validate_game_content` and `sound_groups::assign`. A second scan checks a
collision between two user stages. `check-startup.ps1 -UserStageFixture` starts
the real game with one valid and one broken user stage in its isolated
profile: the window comes up, and stderr carries exactly the one refusal line;
CI runs it on both presets after the other startup checks. On MattPC
(2026-10-28, Debug) the Stage painter checks (`-StagePaperOnly`,
`-StageCountsOnly`, `-StageSummaryOnly`) passed unchanged. They needed a local,
uncommitted fix first: the probe's `menu_text_checks.h` still lists 19 prompts
after `ROAD#21` added the two Profiles prompts, so no painter check compiles
on master until that list gains them.
A Debug launch with `+stage my_arena +players 2 +bot all paint` played a user
copy of Close Quarters from an isolated profile.

**Done 2026-10-28 (follow-ups).** Three more pieces landed:

- **The editor saves straight to the folder.** **File > Save to My Stages...**
  (Ctrl+M) opens a save dialog in the folder. It refuses a name the game
  would refuse, using the same code: the name rule, the case-blind
  comparison and the folder's place under `LOCALAPPDATA` moved into
  `game/user_stage_contract.h`. That header uses the standard library only
  and is included by the game's scan, `directory_consts` and the editor, as
  `horizontal_motion_contract.h` is. Shipped names come from the source
  manifest (`ContentCatalog::stages`). The editor asks before it replaces
  one of the player's own stages, creates the folder only on this command
  (removing it again if a cancelled or refused save leaves it empty), and
  never touches the save. See
  [Play the level](level-editor.md#play-the-level).
- **A rescan without a restart.** `UserStageFolder` (`game/stage_loading.h`)
  owns the folder for the session. `wWinMain` declares it above the
  `Application`, attaches it after the manifest and runs the startup scan
  through it. `GameMenu` takes it as an optional second argument and lends it
  to its `MenuContext` as a rescan hook. `MainMenuStageSelect::init` calls the
  hook as the page opens from the main menu. A match's `MenuContext` never
  has the hook, and the page also skips it between rounds, so the list holds
  still while a match or series could be using it. A rescan builds the new
  list beside the live one: the manifest stages unchanged, then the folder's
  stages in file-name order. Each new or edited file is checked as at
  startup and replaces its registry entry. The sound groups are then
  reassigned over the new list (it cannot fail on stages that each passed
  `check_candidate`, and if it does the session keeps only the shipped
  stages). Only refusals the previous scan did not print reach stderr, so
  reopening the page stays quiet. A stage whose file vanished or now fails
  leaves the list. Stage Select's `slot_of` already falls back to the first
  slot for a remembered or current stage that is gone. The old definition
  stays in the registry, unreachable from the list, because
  `labrador::Registry` has no remove. That costs memory only; see the draft
  report in [engine reports](review/engine-reports.md). Probes that build a
  `GameMenu` without the folder keep their fixed lists. The Debug `reload`
  command still rereads only tuning and presentation.
- **A note for players.** The release ZIP ships the root `README.md`
  (`cmake/release_package.cmake`). Its new "Your own stages" section names
  the folder and the name rule, says when the game reads the folder and
  where a refusal is printed, and points to Save to My Stages, noting that
  the editor is not in the ZIP.

The `user_stages` case now also runs a session: an unattached folder (every
probe's) does nothing. A startup scan loads one stage. A rescan adds a new
stage that sorts first, rereads an edited one (both the list and the registry
show the new name), and refuses a broken file. A rescan with no change gives
the same list. A rescan after one file is deleted and the other broken
returns to the shipped ten, leaving Crossfeed untouched and the whole list
passing `validate_game_content`. The editor's document checks
(`check-level-editor.ps1`) cover Save to My Stages under a scratch
`LOCALAPPDATA` ([Acceptance checks](level-editor.md#acceptance-checks)).

Provisional decisions, open for the author:

- **When to rescan.** Only as Stage Select opens from the main menu. It is the
  one screen that shows the list, and nothing there holds a definition. A
  file watcher or a Debug command would add moving parts for no extra
  player-visible effect.
- **The editor's shortcut** is Ctrl+M (for "My Stages"). Ctrl+Shift+S was
  taken.

Still open: attended play of a real player-made stage (a human gate), and the
Random and marker calls above.

### Series stage rule (`ROAD#20`)

**Done 2026-10-28.** The Mode page has a Series stage row under Match length,
an option row like it: Left and Right (or Select) cycle **Same stage**,
**Rotate** and **Loser's pick**. It decides which stage a series' next round
plays. Same stage is the default and is what every series did before; a single
match has no next round, so the row does nothing there. The rule is saved with
the remembered setup (`"last_match.stage_rule"`, written only when it is not
Same; save version 5). A save without it, or with a spelling this build does not
know, reads as Same, so old saves play as they did. The row's value, Stage
Select's picking heading and the Mode prompt are `presentation.json` text with
startup fit checks ([Mode labels](menu-presentation.md#mode-and-player-count-labels)).

What each rule does, decided on the round that just finished
(`game/series_stage_rule.h`):

- **A draw replays its stage under every rule.** The End menu already calls it
  Replay Draw, and a round that settles nothing should be played again on
  the same ground.
- **Same stage** keeps the stage.
- **Rotate** moves on to the next stage of the Random pool, in manifest
  order, wrapping. It never rotates *into* a stage left out of Random: those
  are trials and comparisons that a person chooses on purpose. A series
  started on one plays it once and then joins the pool at the first pool
  stage after it in the manifest. With an empty pool the stage stays.
- **Loser's pick** gives the next stage to the team that lost. Next Round
  opens Stage Select in the End menu's frame, under the heading *Team A picks
  the arena* (or B), and only that team's seats can browse, lock or back out;
  the other seats' input is ignored. Back returns to the End menu. The page
  opens on the stage just played. A losing team made only of bots cannot pick,
  so the stage rotates as under Rotate instead.
- **Play Series Again** after a completed series follows the rule too, since
  the last round was decisive; a single match's Rematch never moves.
- **Change Level by hand wins.** Change Level (and Change Weapons, which goes
  on to Stage Select) still opens the full Stage Select for everyone, and the
  stage confirmed there is played as chosen: the Stage page returns the new
  End action `play_chosen_stage`, which the rule never moves. Under Rotate the
  page opens on the stage the rotation would play next, so confirming it at
  once keeps the rotation going.

Between rounds the rule changes only the series' own copy of the settings, so
the setup the main menu remembers is still the one that started the series.

The `series_stage_rule` acceptance case covers each rule's next stage, draws,
explicit-only stages, bot-only losers, single matches, Play Series Again, a
stage chosen by hand overriding every rule, the remembered-setup round trip,
old and malformed saves, and the menu text. The painter series re-pick check
(`check-painter.ps1 -EndMenuOnly`) drives the real End menu and Stage Select
under each rule, including the restricted pick, and the setup text check
(`-SetupPaperOnly`) draws the new Mode row. On MattPC (2026-10-28, Debug)
`-EndMenuOnly`, `-SetupPaperOnly`, `-DurationTextOnly`, `-SetupMotionOnly`,
`-RememberedSetupOnly`, `-MainMenuOnly`, `-HomeReturnFocusOnly` and
`-StagePaperOnly` pass. `-MenuTextOnly` had stopped compiling when `ROAD#21`
added two prompts; its prompt list is brought up to date, and it now runs but
fails first on Home's Credits label, a page this change does not touch.

Provisional calls the author should confirm in play:

- which rule is the better default once tried (Same stays the default so
  nothing changes until then);
- whether Rotate should include the series' starting stage when it is
  explicit-only, rather than leaving it after one round;
- whether a draw under Loser's pick should hand the pick to the team that lost
  the previous decisive round instead of replaying;
- whether a bot-only loser should pick for itself (a random pool stage) rather
  than rotate.

The practice hint moved beside Single Match, and Mode's row spacing went from
100 to 96, to make room for the row; that is an art-pass call too.

## Menu navigation follow-up (GAP#4.4)

2026-10-15: pad sticks, d-pads, arrow keys and WASD now move on engagement and
repeat while held. `menu_style.navigation` in `presentation.json` sets the
first delay (0.4 s) and later interval (0.1 s); both must be finite and positive.
The game consumes Labrador's `DirectionRepeat`, with at most one move per update
and no catch-up burst after a hitch. Stick engagement is 0.8 and release is 0.6,
independent of gameplay deadzones. Conflicting directions require neutral before
navigation resumes; aliases for the same keyboard direction count as one.

Each device has its own repeat and neutral gate. Opening or returning to a page,
resetting input ownership, reconnecting a pad or regaining keyboard focus clears
navigation ownership. A control held across that boundary must return to neutral
before moving the new page. Navigation advances once in `MenuPage::update`;
repeated reads, including roster-order reads, return the same direction.
Confirm, Back and Start retain their existing armed release behavior.

The CPU acceptance case `menu_navigation` covers held-page entry, press/repeat
cadence, long frames, stick hysteresis, conflicting directions, keyboard focus,
repeatable reads and unchanged confirmation ownership. Physical-pad feel remains
part of ROAD#2; synthetic state checks do not close that session.

Validation: `scripts/run-acceptance.ps1 -Preset x64-debug` and `x64-release`
both passed, including the new case and Release developer-tool exclusion.
`scripts/check-startup.ps1` passed for both executables with isolated saves and
graceful shutdown; evidence is under `out/validation/menu-navigation-debug/`
and `out/validation/menu-navigation-release/`.

## Stage records and the sectioned save (GAP#6.2)

2026-10-17: every finished round now counts toward its stage's record, and the
save that holds it is built from sections and carries a version key.

**Save layout.** `Save` reads and writes a list of `SaveSection` objects
(`game/save_section.h`) against one root object; each section's read is caught
on its own, so a malformed section falls back to its defaults and leaves the
others intact. The display and play preferences stay flat root keys exactly as
before, so every existing save loads unchanged and needs no migration.
`"save_version": 3` is written first (version 1 introduced records, version 2
remembered setup, and version 3 independent input bindings). A missing key is
version 0, the same flat layout.
A higher version still loads every key this build knows and prints one
stderr line; the next write drops what it does not know. That is accepted for a
local save. New sections use nested keys: `"records"`, the last-match
setup's `"last_match"` (`GAP#6.1`), and `"input_bindings"` (`GAP#4.2`).

**Records** (`game/match_records.h`) are an array of `{ "stage": ... }`
objects, keyed by the manifest name, because the engine's JSON reader has no
member iteration. Each holds versus matches played, wins by side (A/B), draws
and the best winning share, plus practice matches played and the best
coverage. Malformed entries are skipped one by one, counts are kept at 0 or
more, percentages are clamped to 0-100, and a repeated stage keeps its first
entry. Records for stages the manifest no longer lists are kept.

Defaults chosen without an author call, recorded here so they can be revisited:
- **Metric:** the winning team's share of painted surface
  (`LevelEndInfo::team_*_percentage`), the number the results bar shows. Solo
  practice (one team) keeps a separate best coverage of the arena.
- **Records are keyed by stage only, not by match duration.** A shorter custom
  duration can set a best.
- A best moves only when the new value is higher **at the results' one-decimal
  display** (`percentage_tenths`), so an invisible gain is never called a new
  best. The first result on a stage is a new best. An unpainted practice run
  sets no best, and a draw counts but sets and beats no best.
- Every round of a series counts, including draw replays.

**Write point.** `GameLevel::open_results_menu` records the round once, before
the results open: one small synchronous write, the same cost as a settings
Apply. A failed write keeps the record for the rest of the run and reports
only on stderr. A match where a Debug bot or replay drives any seat
(`DebugDirector::drives_slot`) records nothing; that is inert in Release. A
Debug `+stage` session played from the keyboard does record, like any match.

**Shown.** The results page adds one line on the contribution heading's row,
right-aligned to the score bar and revealed with the other details: `NEW BEST
68.4%`, or `STAGE BEST 72.1%` when the result did not beat it (a draw shows the
best if one exists). Stage Select shows the shown stage's record on the
navigation row, right-aligned to the preview: `BEST WIN 68.4% / 12 PLAYED` for
a versus roster, `BEST COVER 37.0% / 2 PLAYED` for solo practice, or just the
count when only draws were played. It is hidden for Random and for a stage
not yet played in that kind of match, and shrinks rather than reach the arena
counter. Both reuse the existing fonts and ink (`results_menu.contribution_font`,
the 24 pt menu face via `stage_select.record_font`); the positions are
`results_menu.record_y` and `stage_select.record_y`, and the labels are
`menu_text.records`. No artwork was added; the later UI art pass may move both.

Validation: the CPU case `match_records` covers display-precision bests, side,
draw and practice tallies, the results-line text, version 0/1/newer saves,
per-section and per-entry recovery, bounded values, a retired stage's record
and an exact round trip; `settings`, `stick_settings` and `display_trial` pass
unchanged. `menu_text` covers the new copy. The painter probe's results and
Stage paper matrices check the line's placement, reveal timing, hiding and
shrinking at 720p and 1080p. The Team setup matrix's real best-of-three series
records its two drawn rounds once each and reloads them from disk; it and the
Resume matrix, whose matches also finish, now save into nested profiles so the
probe's shared save fixture stays byte-identical.

## Remembered match setup (`GAP#6.1`)

The save's independent `"last_match"` section stores the resolved choices of a
normally started match. Its seat-ordered `players` array holds team and concrete
weapon keys and supplies the player count; the other fields are stage, layout,
best-of length and an optional duration override. Omitted duration keeps using
the tuning default. Random weapons and stages resolve before capture;
remembering Random intent would be separate work.

`GameMenu` restores these choices once on launch, after content has supplied the
stage list. Mode and Count focus start at the remembered choices; Team, Weapon
and Stage still require confirmation. Count starts fresh device claims,
including when the remembered devices are missing. Device numbers, readiness,
the active round, scores and progress never enter the saved model. Every choice
remains editable, and returning from a match retains the menu's current
in-memory choices.

The normal Stage confirmation carries a distinct match-start result from a
developer launch. Capture occurs after the first successful level build, once
bot/replay ownership is known, and excludes developer starts and any driven
seat. Restart, rematch and later series rounds do not replace the remembered
setup. Recording a manually confirmed, human-controlled match can remember
those choices; recording does not drive its inputs.

Missing or malformed sections recover independently of display/play preferences
and records. Unsupported weapons use Sprayer; invalid teams recover to a valid
roster; solo keeps either valid team and uses best-of-one. Layout follows the
count, and invalid duration uses the tuning default. A removed stage falls back
to the first manifest entry without consuming randomness; an explicitly chosen
stage excluded from Random remains selectable. An unusable player count drops
only this section. Capture validates the resolved model before writing. The
candidate save replaces the prior file before becoming the remembered value in
memory; a failed write reports on stderr and leaves the prior file and active
match choices usable. Replacement requires serialization, stdio flush, stream
error and close checks to pass; close success alone cannot establish that an
earlier write succeeded.

Validation on 2026-10-18: both `/W4 /WX` game builds and all 37 CPU acceptance
cases passed. `remembered_setup` covers all counts and five tools, exact string
keys, either solo team, legacy/current/newer saves, section isolation, malformed
fields, removed/excluded stages, fresh claims, RNG preservation, strict capture
and actual staging/replacement failures with unchanged memory/bytes and retry.
Display trials retain remembered choices through Keep/Revert and concurrent
changes. A private anonymous-pipe exhibit reproduced a stream error with a
successful close; it supports the writer guard without claiming injected disk
faults in `Save`.

```powershell
./scripts/check-painter.ps1 -Preset x64-debug -RememberedSetupOnly
./scripts/check-painter.ps1 -Preset x64-release -RememberedSetupOnly
```

Both focused checks passed through actual `GameMenu` callbacks. A second hidden
process reloaded the same isolated save and matched independently seeded
Mode/Count/Team/Weapon/Stage pages, with reversed device order, missing/reconnected
pad, fresh joins and confirmations, editable choices and cancellation. Normal
save timing, Pause Restart and two drawn series rounds passed; Debug also checked
developer-script/sparse-bot record/replay starts and an explicitly claimed bot
  on a normal menu start. Debug's existing Setup Paper and Setup Motion regressions
  passed at their display sizes. A follow-up audit corrected the original Release
  omission record: Setup Paper completed its native checks (117 text and 24 motion
  CSV rows, final PASS output and empty stderr), but its wrapper was interrupted
  during contact-sheet generation before writing a final receipt. That is native
  completion evidence, not a passed wrapper result; the original receipts remain
  unchanged. Release Setup Motion remains unrun. Its extra three-resolution
  Count/Team/Weapon animation, readiness, interruption and resize checks contain
  no Release-specific branch or additional save path. Functional completion rests
  on both-preset all-count CPU recovery/transaction checks and the actual Release
  writer/relaunch flow, with Debug covering those shared rendering/lifecycle
  regressions. The broader Release motion matrix remains an explicit limitation,
  not a missing persistence acceptance gate. Both games
also passed isolated published-bank startup, per-monitor v2 DPI awareness and
graceful shutdown with a system-only PATH. Evidence and compact relaunch crops
  are under `out/validation/remembered-setup/`, including the supplementary
  `release-coverage-audit-*.json` receipt. Physical-pad acceptance remains
`ROAD#2`; these checks use scripted input and establish no performance result.

## Action bindings (`GAP#4.2`)

**Default evaluators and keyboard Jump rebinding are implemented and validated.**
**Keyboard Jump and per-slot pad Jump/Shoot/Pause rebinding are implemented (`GAP#4.2`).**
The pad defaults are unchanged (A / LT Jump, RT Shoot, Start Pause). Keyboard
actions other than Jump remain unscoped; the pad contract is
[below](#pad-buttons-slice-gap42), after the face-button slice it replaced.
[Action bindings](../game/action_bindings.h) name the existing gameplay actions
and own typed keyboard, mouse, pad-button and trigger defaults, alongside the
menu action and direction bindings. Pure evaluators consume engine snapshots;
both `PlayerInput` and `MenuInput` use them immediately. There is no new service
or input provider, and today's controls are retained.

Aliases combine each source's held, press and release result independently. In
particular, crossing the jump trigger while A remains held still produces a jump
press. Trigger thresholds remain live tuning with strict `>` comparisons.
Keyboard/mouse focus, mouse button epochs and pad connection retain the engine's
edge suppression. Menu keyboard prior-held reconstruction still happens per key
before combining aliases, and the menu reader retains its arming, conflict,
reconnect/focus-reset and repeated-read rules. Its unbound keyboard Pause action
stays neutral; Escape still means Back in menus.

Stick response, per-slot preferences, raw stick reporting, the strict stick-Y
drop comparison and camera-dependent mouse aim stay in the gameplay reader.
Menu hysteresis, neutral gates and direction repeat stay in their existing
classes, including D-pad navigation while stick navigation is disabled for the
calibration page. The developer bind warning retains its existing seven gameplay
keys, including the existing omission of Enter.

Acceptance: compare default action output with the existing readers, including
opposing/aliased keys, thresholds, held entry, reconnect, focus return and
simultaneous inputs. Real-page checks retain joining, selection, pause and
confirmation behavior. Synthetic devices establish mapping behavior; physical-pad
comfort remains `ROAD#2`.

Validation (2026-10-19): both builds and all 38 CPU cases per preset pass. The
new headless case compares literal frozen-reader oracles with pure evaluations
and actual `PlayerInput`/`MenuInput` instances using scripted pad snapshots.
It covers independent alias edges, focus/connection/mouse epochs, asymmetric
live trigger thresholds and adjacent float boundaries, held entry, repeated
reads, menu chords/reset/reconnect, direction aliases, strict drop thresholds,
raw sticks, sparse roster extraction and live per-slot preferences.

`ConnectionTextOnly` and `TeamFocusOnly` pass on both presets: 347 comparisons,
362 manual page updates and 16 compact crops per preset. All 32 crops match the
previously accepted captures byte for byte. These checks drive actual Join,
Count, Team and Weapon routes plus a directly constructed Pause page, interrupted
Restart confirmation and fresh Back Resume. They do not test native
`PlayerInput`-to-`GameLevel` pause opening or affirmative Restart/Quit; gameplay
mapping and release edges are covered by the independent CPU case. Both
isolated system-path startups pass with the published bank and per-monitor-v2
DPI awareness. Production and oracle reviews found no remaining issue. Evidence
is under `out/validation/default-actions/`; these hidden checks establish no
physical-pad comfort, attended readability or performance acceptance.

**Keyboard Jump slice implemented and validated (2026-10-20).** Controls /
audio has a Keyboard Jump child, independent saved preferences and an effective
mapping consumed by the actual `PlayerInput`. The child reuses the existing
controls widgets; Controls / audio now has nine rows.

The nine-row layout uses 74-pixel spacing. Keyboard Jump and the Controls/Sticks
Apply labels fit uniformly within the padded row and action column; the child
fits its four actions the same way. Resource validation measures those fitted
bounds with the actual glyphs, including the shipped font's taller capitals.

The finite key domain is the physical `labrador::Key` positions A-Z and Space,
with stable lowercase save names `a`-`z` and `space`, independent of enum
ordinals and typed characters. A/D/S conflict with other gameplay actions and
are refused, leaving 24 choices. W/R remain valid despite their menu aliases.
Space is the default; choosing J replaces only gameplay Jump. Menu Proceed
keeps Space/Enter, and opening skip keeps Space/Escape/Enter. Escape remains
gameplay Pause and F1 remains the gameplay debug control. Modifier chords,
mouse and pad sources are outside this slice; other unsupported keys leave the
pending choice unchanged. Grave/F10/F11 remain reserved developer keys.

Capture owns raw keyboard input before menu navigation or developer shortcuts
run. It waits for every named keyboard key, including the opening control, to
return to neutral, then accepts one eligible press and its release. Simultaneous
keys or another key before that release reject the gesture until a new neutral
input. Escape or the existing owner-aware pad Back cancels capture after keys
drain; disconnected-owner fallback retains the existing menu rules. Focus loss
cancels the gesture without applying it, and return requires fresh neutral
input. Menu action ownership and direction repeat reset on capture entry/exit
and child closure. A captured Space cannot also Apply; confirmation requires a
fresh gesture, and terminal capture samples cannot accept a second time.

In Debug, capture drains reserved-key presses and the Back+Start chord before
the director can open a console/menu or execute queued page work. Queued
scripts, match requests, goto, screenshots, reload content and armed
record/replay requests are discarded with one notice. Active recording/replay,
binds and current settings remain intact. A held chord stays consumed until
both buttons release.

The child edits a pending choice. Apply activates and saves it; Reset restores
pending Space, and Back discards unapplied edits. Save version 3 stores a
separate `input_bindings` member and section, so applying the Controls parent's
copied `PlaySettings` cannot overwrite it. Missing legacy data uses Space.
Unknown names, wrong types, out-of-domain keys and conflicts reset only this
section, preserving display, play, records and remembered setup. Its read
boundary translates validation exceptions into the section recovery path.
If writing fails, the choice remains active, the previous disk file survives,
and the page reports the failure and allows retry. Back after Apply retains
the active choice.

`PlayerInput` borrows the saved preference and rebuilds its effective mapping
only when the key changes. Press edges still come from the engine's continuous
snapshots, so changing to an already-held key does not manufacture a press.
The existing paused-match resume gate also consumes held actions. Replay
continues to store post-mapping `PlayerInputData`; its format is unchanged.

Controls help substitutes the effective key into its captured
`menu_text.help.controls` template. Returning from the child refreshes that
widget before it is shown. The match-opening keyboard reminder also refreshes
before a paused opening becomes visible, while retaining its captured copy,
fonts and geometry rather than adopting unrelated presentation edits. Resource
validation covers all 27 physical-key labels in the actual help/reminder fonts
and fitting areas, including child current/pending/release copy and the optional
developer warning.

Developer collision warnings work in both directions: a bare command bind on
the effective Jump key warns, and a pending/applied key shows a warning when a
bare debug bind already exists. The existing seven default-key warnings remain,
including Space's fixed opening alias and the deliberate omission of Enter.
These collisions remain warnings, and the query is inert in Release.

**Validation (2026-10-20).** Both game/editor presets build under `/W4 /WX`.
All 42 consumer cases per preset have successful final receipts, reconciled as
84 cases. The new CPU checks cover all 27 key names/labels, 24 allowed choices,
fixed menu/opening/pad aliases, actual saved-preference readers, migration and
malformed-section isolation, gesture ownership, held input, focus, repeated
reads, failed-write/retry and unrelated settings. The actual reader and resume
gate retain an already-held new Jump key without manufacturing a press.
Menu-text checks include actual shipped glyph metrics and exact synthetic
boundaries for the uniformly fitted Keyboard Jump/Apply labels.

`KeyboardJumpOnly` passes in both presets at hidden 720p/1080p: Debug checks
44 independently fitted help/status pairs over 406 locally fed menu updates;
Release checks 42 over 402. Each checks ten fresh Save reloads and two actual
paused GameLevel opening reminders. The real Controls child exercises
Apply/Back/Reset, failed-write/retry, independent pending parent audio, effective
help and paused-opening refresh with captured text/layout and frozen simulation,
RNG and available Debug fingerprints. All 88 shared PPMs match across presets.
The ownership leaf checks eleven common observations per preset and eleven
additional Debug observations for queued work, reserved shortcuts, the pad chord
and F10 held through Keyboard. Actual OS focus delivery is outside that
probe. All ten affected legacy UI runs pass: Controls (including Pad Sticks),
percentages, paused options, menu motion and Settings in both presets.

Separate writer and reader OS processes pass in both presets using a retained
private profile: J saves through the targeted setter and loads through actual
Save/PlayerInput, preserving unrelated preferences and save bytes. These are
acceptance-executable process round trips, not game-window relaunches. Both
isolated game startups pass with the published bank, per-monitor-v2 DPI,
system-only PATH and graceful shutdown.

An earlier restricted Release CPU `settings` run failed saving recovered stick
preferences. The unchanged executable passed in a fresh private directory with
explicit filesystem access; the original failure is retained and its cause
remains undiagnosed. Earlier font-fit/test-fixture failures are retained too.
Evidence is under `out/validation/crossfeed-20261020/keyboard-jump-v1/`, including
`consumer-reconciliation.json`, `native-reconciliation.json`,
`ui-reconciliation.json`, `settings-retry/` and `relaunch-final/`. These hidden
implementation checks establish no human comfort/readability, physical-pad,
listening, balance, hardware-baseline or performance acceptance.

### Pad Jump face-button slice (`GAP#4.2`)

**Scoped pad slice implemented and qualified, 2026-10-26.** Each XInput slot
can choose A, B, X or Y as its gameplay Jump face button, default A. LT remains an
independent Jump source with the existing live threshold and alias-edge rules.
Use a discrete choice rather than a new pad-capture interaction. The existing
Pad 1-4 selector and pending/Apply/Back flow establish the menu pattern; retain
the approved presentation treatment.

Profiles belong to XInput slots, not player/view indices or physical-device
identity. Only gameplay Jump changes: menu A/B/X, intro skipping, Start pause,
Back debug, RT shooting, sticks and analog drop retain their current mappings.
A face button's menu role is not a gameplay binding conflict. There is no
unbound-Jump option or trigger reassignment in this increment.

**Model/save/evaluator implemented, 2026-10-26.** `InputBindings.pad_jump`
holds four physical-slot choices and writes canonical `a`/`b`/`x`/`y` tokens
under the existing version-3 `input_bindings` section. Missing/malformed slots
recover independently to A while valid keyboard and other pad choices survive.
The actual `PlayerInput` caches each slot's effective binding, retaining LT and
source-level engine edges when a saved choice changes during held input.
Keyboard Jump Apply merges its key into the latest preferences so it cannot
overwrite newer pad choices.

Debug/Release game builds and `input_bindings`, `action_bindings`,
`keyboard_jump_capture`, `settings`, `remembered_setup` and `resume_countdown`
passed. Coverage includes all choices/slots, sparse slots, LT alias edges,
held adoption, suspend/reconnect/resume, local save recovery, failed writes,
reset and unrelated preferences. Receipts are under
`out/validation/overnight-20261026/` (`input` and `release`).

**Page/help and mapped replay qualified.** Controls now opens `PadJumpMenu`,
with physical-slot and face-button selectors, Reset this pad, Apply and Back.
Pending choices survive slot changes. Apply merges only edited slots into the
latest preferences; keyboard and untouched slots survive. Reset is pending for
the selected slot, and Back discards unapplied changes. Default selection follows
the pausing physical slot, otherwise the first connected pad, otherwise Pad 1.

Shared help names the per-pad choice and LT instead of promising A everywhere.
The opening reminder names the actual sparse roster's buttons and refreshes
while paused without ticking the Level. Menus still use A/B/X and intro skip
still uses A/B/Start. The ten-row Controls layout uses the existing Lato font,
64-unit spacing and valid 8-unit focus padding; no art-rule guard is relaxed.

`check-painter.ps1 -KeyboardJumpOnly` now checks both binding pages. Debug and
Release passed at 720p/1080p: 32 physical-slot/face Apply/reloads, 65 independent
native help/status glyph pairs, pending merge/reset/discard, controlled save
failure/retry, parent audio preferences, connected-owner isolation and selected-B
menu Back. Two actual paused page routes qualify reminder refresh/reversal with
unchanged Level fingerprints, and mixed four-pad reminders fit their cards.
The retained entry/reminder images were visually inspected; this is automated
native rendering and synthetic input, not an attended pad session.

Debug additionally records 360 actual-reader updates for reordered slots through
the production post-mapping recorder/file/player path. Persisted alternate face
choices change live input on 111 frames; replay from decoded settings preserves
every seat stream, fingerprint, six checkpoints, final state and terminal RNG.
Release explicitly excludes that developer replay path. The Release `menu_text`
case and both-preset `menu_presentation` checks also pass. Phase-3 native and
CPU receipts are under `out/validation/overnight-20261026/phase3/`.

No new engine input API or rebinding artwork was needed. Synthetic devices
establish mapping behavior. Physical-pad comfort
remains `ROAD#2`, and attended readability, balance and hardware acceptance stay
open.

The [pad buttons slice](#pad-buttons-slice-gap42) below generalises this one and
replaces its page.

### Pad buttons slice (`GAP#4.2`)

**Implemented 2026-10-27; replaces the Pad Jump page.** Each XInput slot has a
`PadProfile`: Jump, Shoot and Pause, each with a required primary control and an
optional alternate. The domain is eleven controls: A, B, X, Y, LB, RB, LT, RT, LS
and RS (pressing a stick in) and Start. Back stays the debug control and the
D-pad stays menu-only. The defaults are the controls the game always had, so an
untouched pad behaves exactly as before: A / LT Jump, RT Shoot, Start Pause.

One control does one action per pad: a profile with an unbound action, an
alternate equal to its primary, or a control used twice is invalid, and
`Save::set_input_bindings_and_save` refuses it before mutation. Movement, aim and
the stick-down drop stay on the sticks. Menus keep A/B/X/Start and the intro skip
keeps A/B/Start whatever the profile says, as Jump's face choice did before. A
trigger keeps its strict live threshold: the shoot threshold when it shoots, the
jump threshold when it jumps or pauses.

`effective_action_bindings(preferences, slot)` writes the profile into Jump,
Shoot and Pause's pad sources; `PlayerInput` rebuilds a slot's copy only when
its profile changes, so adopting an already held control still makes no edge.
Replay keeps storing post-mapping input, so recordings are unaffected.

**Page.** Controls / audio opens **Pad Buttons** in the Pad Jump row's place:
Pad, Jump, Jump (also), Shoot, Shoot (also), Pause, Pause (also), Reset this pad,
Apply, Back, on the same ten-row Controls layout. Left/Right cycles a row through
the domain (alternates add None first) and skips any control another row of the
same pad already uses, with a status saying so, so a pending profile is always
valid. A discrete choice was kept over a press-to-capture gesture, as for the
face slice. Pending profiles survive slot changes; Apply merges only edited
slots into the latest preferences; Reset is pending for the selected pad; Back
discards. Default selection still follows the pausing slot, then the first
connected pad. The match-opening reminder names each slot's saved controls,
in two columns for a shared profile and one line per pad when they differ.

**Save.** `input_bindings` gains a `pads` array of four objects, each with
`jump`, `shoot` and `pause` arrays of one or two canonical tokens (`"a"`, `"lb"`,
`"lt"`, `"ls"`, `"start"`, ...). It stays save version 3. A slot that fails to
parse or validate recovers to the defaults alone; a malformed array recovers
every slot without touching the keyboard key. When `pads` is absent, the older
`pad_jump` face array migrates per slot into Jump's primary.

**Checks.** `input_bindings` covers every control on every entry of every slot
against an independent oracle, the conflict query, invalid profiles, tokens,
thresholds, the actual reader (sparse slots, remap, reconnect, resume, and moved
Shoot/Pause with Start keeping its intro and menu roles), legacy migration,
per-slot recovery and failed writes. `menu_text` checks the copy and every
control label in the actual OptionRow. `check-painter.ps1 -KeyboardJumpOnly`
ports the Pad Jump native matrix to the new page, adding a skipped-conflict
Shoot Apply. Physical-pad comfort remains `ROAD#2`, and rebinding artwork belongs
to the later UI pass.

**Remaining scope: UNSCOPED.** Keyboard actions other than Jump (move, drop,
shoot, pause) still need their own contract.

Labrador declined the action-evaluation request (engine report 21, formerly
`GAP#F2`) in `197aff1`: evaluating a binding table is the action map, which its
boundary table gives to the game, and its clients already combine sources by
rules that disagree. The evaluator stays client code; see the
[closed entry](review/engine-reports.md#21-digital-action-evaluation-remains-client-code--declined-197aff1).
Pad Buttons is now a real pad-rebinding caller, but it chose discrete cycling
over capture, so a pad-capture request still has no caller.

## Per-player menu focus (`GAP#D1`)

**Implemented and evaluated (2026-10-19).** Team Select now
registers its three icons per row in one private `FocusGroup`, with exactly one
cursor per roster seat. Input slots still map through the joined roster: keyboard
slot 4 is not a fifth cursor, and sparse/reordered devices retain their own rows.
The focused pointer supplies the provisional team choice; readiness remains
separate. Final confirmation is still the only point that commits choices.

Left/Right keeps the existing discrete A/none/B order and endpoint limits through
explicit `set_focused` calls. It does not use the geometric candidate walker:
valid artwork origins can shift physical icon bounds without changing the team
order. Registered choice validation, cursor storage and derived painting fit
this page without navigation proxies, an engine change or a shared page base.

Focused and unfocused entries use the existing live unselected colour. A ready
seat disables only its chosen entry, retaining its cursor and deriving the
existing selected colour even while another cursor changes. Visual refresh
re-enables entries before applying current readiness. The colour snapshot updates
at the same refresh points as before; changing presentation data alone does not
recolour a frozen draw. Atlas/frame pairs, copy, fonts, paper and motion retain
their existing captured behavior. The inherited single-cursor group stays empty
because the paper uses it for a shared focus card/arrow.

Pre-frame Back handling, early shuffle priority, held/reconnect action resets,
repeated confirmation and endpoint sounds, team requirements, cancellation,
choice motion and RNG consumption are retained.

Weapon Select retains its existing value adjustment. Each seat owns one reused
`MenuIcon`; `cycle_weapons` changes a six-value enum and explicitly starts choice
motion, while Random resolves only after final confirmation. A cursor pinned to
that icon cannot represent those values. Walking the registered widgets would
move between seats, and a separate single-entry group per seat would never move.
Readiness also paints the weapon name, and repeated confirmation still plays its
sound, so disabled-entry activation would not replace that flow. These are
concrete differences in [the page](../game/states/main_menu_weapon_select.cpp),
not an engine defect: [FocusGroup](../external/labrador/engine/ui/focus.h)
explicitly leaves Left/Right value adjustment to the client, as `OptionRow`
already demonstrates. No proxy widgets or shared page base are needed. Team's
registered choices supply the useful per-player cursor client that this finding
was missing.

Validation: Debug and Release builds and all 37 CPU acceptance cases per preset
pass. The compact `TeamFocusOnly` native check passes 258 raster comparisons over
226 page updates per preset, including all roster counts, sparse/reordered device
slots, readiness retention, motion, input priorities, reconnect, final commit into
the actual Weapon page and an unready return. Captured artwork/text/fonts and
live-colour refresh boundaries are exercised, along with valid nonzero sprite
origins that reverse the physical A/B order. Each preset produces eight compact
icon-strip crops; the crops and CSV are byte-identical across presets. Both
isolated system-path startups pass with the published audio bank and per-monitor
DPI awareness. Evidence is under `out/validation/team-focus-cursors/`; a probe-only
camera API compile correction and its failed logs are retained. This establishes
no physical-pad, attended readability or performance acceptance.

## Other demonstration gaps

`GAP#D2` is **DEFERRED** until an accepted page requires a widget or page
abstraction beyond the existing `OptionRow`; a list of unused toggle, slider,
progress or modal types is not an implementation task.

`GAP#D5` is **complete for the selected bounded capability** (2026-10-26).
The [qualification below](#horizontal-movement-capability-2026-10-26) covers
horizontal movers and mirrored fixed scored slabs through runtime/editor
construction, collision/support, local paint, public routes/replay and native
composition. Their useful placements remain explicit-only Turbulence
comparisons; attended selection and balance are `POL#21`. Arbitrary moving
rotation is outside this contract. Crossfeed and Press Tower stay static.

### Horizontal movement capability (2026-10-26)

The first `GAP#D5` increment is implemented: a manually constructed
`TranslatingStructurePaintable` follows an axis-aligned horizontal triangular
path. A `Level` admits its fixed-lifetime movers before the first frame, gives
them stable IDs, prepares motion before input/state mutation and commits it
with the active physics step. Zero-duration updates preserve motion history and
do not reuse a previous contact sweep. Unsupported time steps, nonfinite rates,
unsafe relative displacement and closing support corridors are refused.

Confirmed solid and one-way top contacts carry a rider without adding personal
velocity or tool travel. Jump, drop, walk-off, death and placement release that
context. Paint, credit and scored areas remain attached to local faces; world
queries, scoring samples, art, splash, runoff, culling and the composed shadow
follow the current pose. Flying paint uses chronological relative sweeps across
reversals; a Roller retains its published launch volume, multiple victims and
short lifetime while painting each moving pair once. Existing static paths keep
their original arithmetic.

Optional moving state participates in Debug fingerprints and replay format 3.
The first qualified record/play factory was a fresh, manually constructed seeded
Level; public stage-loader qualification is recorded below. Class layouts remain the same
in Debug and Release.

Qualification is retained under `out/validation/todo-loop-20261025`:

- Debug game/editor and include checks, with 20 targeted native cases; Release
  game/editor and include checks, with 19 cases. `moving_structure` includes
  actual-object motion, paint, rider/lifecycle, projectile and bounds controls;
  its recorder/player and strict-codec checks run in Debug only.
- The exact old compatibility source, freshly linked to the new Debug objects,
  reproduces both complete legacy replay JSON files. A separate old-API trace
  reproduces all three complete CSVs over 20 Crossfeed/Press Tower cases,
  24,000 directed updates, both teams and all five tools, including terminal RNG.
- One hidden D3D11 hardware probe on the RTX 5080 passes 26 retained captures
  and 42 pane visits: 1/2/4 panes, ordinary/miniature poses, reversals, world-cull
  controls, runoff contrast and camera restoration after an exception. Independent
  retained-RGB checks recompute every frame hash and inspect the exact inventory.
  These are fixture cameras, not gameplay framing or comfort acceptance.

`moving-native-v9`, `moving-followups-v1`, `moving-device-v2` and
`moving-device-review-v1` hold those receipts. Failed fixture/launch observations
are preserved. The full Crossfeed balance case reached its 180-second bound
without a failure diagnostic and is **not qualified**; it is not repeated here.
The full Press Tower balance case was not run for this increment. Their bounded
static identity controls above pass; this is no new balance or performance claim.

**Runtime JSON and object construction implemented, 2026-10-26.** Optional
`StructurePaintable.motion` loads the existing horizontal trajectory through
the production object builder; missing motion retains the static constructor.
The [format contract](level-format.md#horizontal-paintable-motion) records the
field limits and the separate scene/player admission checks. Movers no longer
act as permanent static paint/material occluders; matches and stage previews
compose translated shadows from their actual mover owners. Envelope-aware
editor authoring is recorded below.

Debug/Release game/editor builds, `level_format`, `moving_structure`,
`paint_surfaces`, `terrain`, `stage_selection` and editor document acceptance
passed. Tests cover malformed records, authored phase, deterministic fresh object
construction, admission refusal, moving shadow ownership and static occlusion;
that increment did not exercise a public `LevelBuilder` replay rebuild. Receipts are
under `out/validation/overnight-20261026/` (`runtime`, `release`, editor logs).

**Public rebuild/replay qualified, 2026-10-26.** The `moving_structure` case
now writes complete stages and rereads them into fresh registries through
`LevelLoadedInfo` and `LevelBuilder`. Both presets qualify authored phases,
mover-only IDs, disk rebuilds, zero-step history and stage-named unsafe admission.
Debug additionally records/serializes/reads/rebuilds solid and one-way stages:
360 exact updates each, six checkpoints each, final state and terminal RNG.
The traces prove actual carry, paid local-face paint/credit, jump release,
one-way drop release and both reversals. Rebuild uses the decoded stage, setup,
seed and step; it does not reuse the recording's Level or manual factory.

Receipts are under `out/validation/overnight-20261026/phase2/` (`runtime-v2`,
`release`, and the printed task-owned replay receipt path). This qualifies
production `ReplayRecorder`/`ReplayPlayer` with the public builder; DebugDirector
command/restart adapters and native/human acceptance are separate. The existing
legacy-byte/hash checks also pass. Release excludes the developer replay path.

**Editor authoring and round trips qualified, 2026-10-26.** The
[editor contract](level-editor.md#moving-structure-authoring-gapd5) now supports
solid and one-way movers, anchor/property edits, safe duplication, undo/redo
and saved round trips while preserving unknown metadata and rectangle/TLBR
precedence. Source `player.size` supplies the complete standing corridor; mover
documents require an explicit tuning-file context. The editor and runtime share
an engine-free field/envelope contract with the existing arithmetic and messages.
Actual-player rate/displacement checks remain in `Level`.

The canvas shows the anchor, initial-phase body, complete travel and expanded
standing corridor. Motion paint is counted once in anchor space; changing phase
only translates scoring-guide locations, including across fractional float
boundaries. The actual editor model is included in `moving_structure` acceptance:
its saved stage loads through `LevelBuilder` with local paint/material checks,
and Debug records/rebuilds 180 exact updates with carry, paid local-face credit
and terminal RNG. Document checks cover both collision kinds, transactional
geometry edits, duplication, metadata, changed source sizes and scoring.

Current-source editor builds/document checks and `moving_structure` passed in
Debug and Release after the fractional-area correction. Debug includes the
180-update editor-save replay and 720-update solid/one-way public replays;
Release explicitly excludes developer replay. Receipts are under
`out/validation/overnight-20261026/phase4/` (`editor-*` and `mover/`).
The continuous editor strip sum and gameplay's float tile aggregation have
separate rounding; qualification checks phase invariance within each and
reconciles the published tile areas. This implementation does not close native
composition, attended editor/play, physical-pad or Turbulence acceptance.

**Fixed scored-slab core qualified in Debug and Release, 2026-10-26.**
`FixedSlabPaintable` adds one fixed 160-horizontal/80-rise footprint with
24-unit normal thickness, mirrored left/right. It retains the game's
`Structure`/`PaintableObject` hierarchy. The separate Player support path uses
the real top segment; caps and underside stay solid. Actual Scene courses
cover driven joins, stops, jumping, both contact orders, repeated uphill
ceiling stops without creep, reversal and recovery. Legacy static arithmetic
remains on its existing paths.

Flying paint selects the actual first impact using four-axis swept SAT for
the current linearly changing projectile boxes. Empty AABB corners, diffusion,
launch departure versus crossing, and enemy/cover ordering are qualified.
Paint ownership and credit stay in local tiles; published world quads carry
their authoritative local areas. A shared checked canonical adjacency helper
prevents OBB reconstruction drift from creating buried-face score, while
retaining real authored gaps. The editor exposes the
[same bounded contract](level-editor.md#fixed-scored-slab-authoring-gapd5),
with transactional edits, save/reopen and reciprocal polygon face clipping.

Both game/editor builds, the full editor document/hidden-window checks and
`fixed_slab` pass. Per preset, the case includes 18 actual Scene courses over 3,684 updates,
560 independent SAT probes, real projectile/credit controls, fresh public
1/4-player loads over 386 updates and seven paired editor/runtime refusals.
The isolated 24-object trial preserves all 22 mover-trial objects and remains
unregistered. Twelve focused existing cases also pass in both presets: movement/contact,
paint surfaces/underfoot, movers, blocked shots, tunnelling, terrain, format,
stage selection, lethal provenance and ammunition. Receipts are under
`out/validation/overnight-20261026/phase11/` (`slab-v4`, `regression`) and the
editor receipts linked above. New slab CPU receipts are `slab-v4/x64-debug/fixed-slab-29548`
and `regression/x64-release/fixed-slab-35444`.

**Native composition qualified, 2026-10-26.** The fixed-slab probe passes both
presets: 29 frames, 71 pane crops, 59 readbacks and 3,080 updates across all
split layouts, including four fresh wet-paint controls. Its visual base bands
remove measured primer pinholes while retaining every scoring tile and owner.
The [arena record](polish-turbulence.md#why-the-new-geometry-earns-its-place)
owns the exact inventory, six interior-pixel controls, frozen-state checks and
receipts. These hidden captures do not establish attended readability or value.

**Local public Roller role qualified, 2026-10-26.** Both presets pass eight
fresh public input-driven routes comparing the fixed slabs with the retained
composite. The 3,824 updates and 320 paid firing updates establish partial
diagonal-top paint with unchanged tuning, supported endpoints and natural
refill. Directional paint and airborne-contact differences are measurements;
the [arena record](polish-turbulence.md#why-the-new-geometry-earns-its-place)
owns their exact areas and limits. Fresh public replay remains separate.

**Fresh public replay qualified, 2026-10-26 (Debug).** Production recording,
codec and playback pass four mirrored uphill/downhill pairs from fresh decoded
public disk rebuilds. Every active fingerprint and complete paint/credit sample
matches. The first 180 countdown updates have no input-filter callback, so each
record/rebuild pair compares those cold priming observations separately before
its actual tape. The 1,192 tape updates, sixteen checkpoints, four malformed
codec controls and one 240-update wrong-checkpoint control use 4,064 actual
Level updates. The [arena record](polish-turbulence.md#why-the-new-geometry-earns-its-place)
owns the exact scope and receipts; stock DebugDirector cold playback and
cross-build determinism remain outside this proof. Release excludes this mode.

**Bounded capability work complete.** Runtime/editor construction, local paint,
support/projectile contacts, public driven routes/replay and native composition
now qualify the selected horizontal movers and mirrored static scored slabs.
This finishes `GAP#D5`; arena pressure and geometry selection remain `POL#21`.

The explicit-only `turbulence_mover_trial` now uses the authored pair beside
the preserved static arena. Both-preset route and native composition checks,
Debug seeded public rebuilds and the Release local five-tool comparison are
recorded in the [arena owner](polish-turbulence.md#why-the-new-geometry-earns-its-place).
The concrete added role is a Sniper crossover-top paint angle at all four
sampled phases; the other tools retain different static/moving tradeoffs.

Arbitrary or combined moving/rotating bodies remain outside the implemented
contract. Preserve the existing hierarchy and static arithmetic. The
[Turbulence blockout work](polish-plan.md#turbulence-blockout-work-pol21) owns
useful placements and route/side-swap qualification; attended arena acceptance
remains a separate gate. The mover comparison stays out of Random. `POL#21`
stays open; human/device gates and provisional weapon balance stay
unchanged.

## Presentation rate (`GAP#L9`)

**UNSCOPED.** `SIMULATION_HZ` deliberately fixes gameplay at 60 Hz; changing
`ApplicationOptions::target_fps` changes that step, not just presentation. The
pinned D3D11 backend presents with sync interval 1 and exposes no client vsync
switch. A presentation-rate setting must first identify its engine seam and
supported policy, separately from simulation and any interpolation work.

Inspect the current engine API and master before proposing a client setting or
report. Define its persistence and visible acceptance on the intended refresh
rates before implementation; this row does not supply a ready engine change.
