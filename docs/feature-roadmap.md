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

Match options are not modes (the author's 2026-11-03 decision, `ROAD#22`). An
option that keeps the win condition, most painted surface when the clock ends,
is a match option inside the one ruleset, as Match length, the series formats,
the [Series stage rule](#series-stage-rule-road20) and Boost handicaps already
are. The `ROAD#14` pickups toggle and any later variant inherit this ruling. It
does not bring back the `MatchRules` seam: an option adjusts values or loadout
and adds no end condition or result path. Reopen if the 2026-10-15 decision is
read as one fixed set of match rules rather than one way to win; `ROAD#14`'s
toggle would then need its own ruling.

### AI bot players (ROAD#10)

**ROAD#10 - accepted (the author, 2026-11-05).** Setup, the shipping
driver, mover painting and pane-clamped labels are integrated, and the author
accepted the Easy/Normal/Hard values as they stand; they are no longer
provisional. The attended difficulty comparison and the physical-pad checks
were waived by the author on 2026-11-05 and were not performed. The dated
implementation records below retain their original checkpoints, including the
"provisional" and "OPEN" wording each had at the time.

**Original request (2026-10-27).** Add playable AI teammates and
opponents through normal match setup in Debug and Release, within the existing
four-player, two-team limit. Keep a bot-free solo-practice option available.

Start with three difficulty settings: **Easy, Normal and Hard**, selectable per
bot during setup. Difficulty should change reaction time, aim accuracy and
decision quality while using the same movement, weapons, ammunition and scoring
rules as human players.

The first implementation step was to define the human/bot roster and setup flow,
bot view allocation, team/weapon selection and difficulty tuning. Build on the
existing input and seeded Debug-bot experience where useful, preserving the
game's OOP design. Playable bots need to navigate platforms and recover when
stuck, seek paintable territory, engage opponents and manage paint reserves
across all five tools. Scripted measurement bots alone do not establish
those capabilities.

Acceptance needs menu setup, mixed human/bot rosters, bot identity, pause and
disconnect handling, restarts and series continuity in both builds; repeatable
navigation/combat checks on the shipped arenas; and attended play showing that
the three difficulty settings feel distinct. Automated checks alone do not
establish difficulty or gameplay quality.

#### Roster foundation (2026-10-27)

The first bounded slice adds explicit human/bot seats and `BotDifficulty`
(`easy`, `normal`, `hard`) to `MenuLevelSettings` (now `MatchSetup`) in both builds. Existing
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
  attended comparison establishes distinct, useful difficulties. In a Debug
  build that comparison starts from a launch line, for example
  `+stage crossfeed +players 2 +seat 2 bot hard`
  ([`DBG#17`](debug-console-record.md#roster-statements-for-bots-teams-and-boost-dbg17)).

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
(Since `HYG#35` the format is 5, which keeps this metadata and refuses 3 and 4;
`HYG#39` moved it to 6.)
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
neutral human holds a Sprayer and never fires. The seed was the `std::srand`
value whose first draw became the bot seed. Since `HYG#39` the round seed is
the shipping bots' seed itself, so the probes now name the bot seeds those gave,
`5242`, `8644` and `3946`, and the bots are bit-identical; the `61403` notes
below are about seed `3946`'s matches. Every one of the 40 configurations gives
three pairwise-different final states, so the seeds reach play.

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

**Refill off enemy paint.** Paint underfoot
([POL#30](polish-plan.md#experimental-underfoot-prototype); a default-off trial
when this was written, shipped on since 2026-11-05) stops ammunition
recovery on enemy paint, and a refilling bot walks on toward its next paint
goal, so it could stay empty. `bots.refill_off_enemy_paint` (shipped on, and
active now that underfoot ships on) sent a refilling bot back to the last grounded centre
it held off enemy paint and held it still off it until the reserve resumed.
A threat retreat still won. The observation carried the trial's state and
whether this step's support was enemy paint; `bot_driver` checked hold, return
and the no-trial and toggle-off controls. The 2026-11-05 paragraph below
replaces the hold.

**Paint underfoot in the bots (2026-11-05).** With underfoot shipped on, the
shipping driver failed `bot_combat_trials`, `bot_turbulence_routes`,
`bot_mover_routes` and `bot_team_trials` (all four pass with it off). The bots
now model it rather than the probes switching it off. `MatchBotLocomotion`
(`game/ai/match_bot_locomotion.h`) mirrors `Player::update_movement` from
`tuning.player().paint_underfoot`: a footing (none, own, enemy) scales the four
ground rates, and airborne steps use the base rates. The
observation carries this step's footing; braking (`steer`), a standing
launch's speed limit and jump reach, the approach stop (now starting a
stopping distance plus one held decision's travel early inside range) read
it; the first jump's reach takes the slower of this footing and unpainted
ground, since the takeoff need not share this step's paint. A finish window's refill estimate keeps the base rate, a sufficient
minimum since own paint only speeds the refill. The refill rule no longer freezes a
bot off enemy paint: it keeps walking and refilling, holding only where
`BotWorld::top_paint_team` reports enemy paint a stride or a braking distance
ahead, and that filter applies to a threat retreat too. On enemy paint it
makes for the nearest ground along its floor that isn't, then the remembered
ground. A crossing ascent still rising with its feet below a solid lip holds
beside the lip's face, as a plain ascent already did. A mover ride's hold, and
any finishing window on it, ends early once the support's unpainted tile count
has not fallen for a progress window (or two shots of a slow tool) of steps the
bot could fire, counted afresh when its reserve resumes. No probe
threshold changed and none runs underfoot off; `bot_driver`'s refill check
now pins walk-on, hold-before-enemy-paint and the refuge choice. Walk and ramp
budgets keep the base starting speed, which already far undercounts a real
walk. The four cases pass in Release with the shipped tuning; the closest
margin is a Mover reach trial (Easy Sprayer, team b) that lands at step 2010
and leaves at 2368 of 2400.

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

**Remaining acceptance (readiness audit, 2026-10-28):** attended difficulty
comparison and usefulness remain the author's gate. Physical-pad checks were
not run. Mover painting and pane-clamped labels are implemented above. The
author accepted the values on 2026-11-05 and waived the attended comparison
and physical-pad checks, which were not performed.

#### Fixed scored slabs for bots (`ROAD#38`)

**Measured; no segment landed (2026-10-30).** When filed, bots treated the fixed
scored slabs as solid boxes: `Level::bot_supports` leaves out
`structure_fixed_slab`, `Level::bot_ramps` takes only ramps, and the traversal
sweeps keep slab bounds as conservative barriers
(`game/ai/match_bot_navigation.cpp`, `match_bot_ramp.cpp`). That is unchanged.
Only `turbulence_slab_trial` has fixed slabs (explicit-only when this was
written; in Random since the author's 2026-11-05 `POL#21` call); it is the mover
trial plus one slab per side bridging the upper approach (top 880) to the
crossover (top 800).

The opt-in case `bot_slab_routes`, built like `bot_mover_routes`, runs the
actual driver for 2400 steps per trial on both stages: an approach start (placed
on the upper approach, as the mover case's reach route) and an ordinary spawn
start, both teams, five tools and three difficulties, 120 trials. It counts
steps grounded on a slab's top line (a ramp state with the uphill corner on the
line, as `Player` supports it), owned slab-top paint, and the first arrival on
the bot's own crossover, noting whether a slab step came first. On the mover
trial the same slab windows must read zero; nothing else is gated.

| Stage, start | Trials on a slab | Slab steps | Slab top painted | Crossover arrivals | Via a slab |
|---|---|---|---|---|---|
| slab trial, approach | 21/30 | 525 | 19/30 | 20/30 | 11/30 |
| slab trial, spawn | 17/30 | 342 | 18/30 | 11/30 | 8/30 |
| mover trial, approach | 0/30 | 0 | 0/30 | 30/30 | 0/30 |
| mover trial, spawn | 0/30 | 0 | 0/30 | 21/30 | 0/30 |

Identical in Debug and Release. The counts are not near zero: ordinary walking
carries bots onto and up the slabs (38/60 trials), they paint the slab tops
(37/60), and 19 of 31 crossover arrivals came over a slab. By the rule this
item was scoped with, no slab-aware segment was added, and bot code is
unchanged. The cost the slabs do show is fewer crossover arrivals than the
mover trial's empty gap: 31/60 against 51/60.

An unlanded experiment, recorded for whoever revisits it: offering each slab's
top line to `choose_ramp` as a `MatchBotRamp` (bounds spanning the top line,
with the slab's own AABB as the one obstacle its path may disregard) raised
slab-trial crossover arrivals to 30/30 (approach) and 25/30 (spawn), 55/60.
It was reverted because the scope did not call for it. If `POL#21` keeps the
slabs, that is the measured starting point.

Evidence: `out/validation/road38/` (per-trial `MEASURE` lines in each preset's
`bot_slab_routes-run.log`). This slice did not put slabs into Random (the
author's 2026-11-05 `POL#21` call did), change difficulty values or close the
attended `ROAD#10` gate. The slab trial
can now join `POL#42`'s comparisons.

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

**ROAD#5 - accepted (the author, 2026-11-05).** Reopened by the author on
2026-10-27 after being deferred on 2026-10-15 until couch sessions (`ROAD#2`)
showed a need; with bots now offering Easy, Normal and Hard, the author asked
for it to be built. On 2026-11-05 the author accepted +1 and +2 at the values
below without the attended session.

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
every level (Off unchanged), the refill multiplier and respawn retention,
and Boost on paint underfoot (below);
`remembered_setup` round-trips and recovers handicaps. Presentation
validation fits every level's text under every Weapon Select label.

**Accepted, 2026-11-05.** The author picked one Boost for health and refill
over health-only, refill-only or two-way levels, and accepted +1 and +2 at the
values above; they are no longer provisional. The attended play against
Easy/Normal/Hard bots was waived by the author on 2026-11-05 and was not
performed. A Debug build can still start that session without the menus:
`ColourWars.exe +exec review_bots_boost` plans a seeded Crossfeed with Easy +2,
Normal +1 and Hard bots, and `seat <n> bot|boost|team` sets any other roster
([`DBG#17`](debug-console-record.md#roster-statements-for-bots-teams-and-boost-dbg17)).

#### Boost on paint underfoot (`ROAD#39`)

**Covered (2026-10-29); checks only.** `Player::ammo_regen_multiplier`
multiplies the underfoot multiplier by the level's `paint_refill`
(`game/objects/player.cpp`), which is what "multiplies paint underfoot rather
than replacing it" above means. Paint underfoot has shipped on since the
author's 2026-11-05 `POL#30` call (`tuning.json` `paint_underfoot.enabled:
true`, at the `POL#36` amounts), so the `handicap` case requires that and
exercises the combination both ways, through the `paint_underfoot` case's
fixture (`scripts/acceptance/paint_underfoot/fixtures.h`):

- **Underfoot switched off (`World::configure(false)`):** on unpainted, own and
  enemy floor, every level's multiplier is that level's `paint_refill` alone,
  bit for bit.
- **Shipped tuning, underfoot on at its shipped amounts:** standing on actual floor
  paint, with the support consumed through real contacts, own paint gives
  `own_ammo_regen_multiplier` × `paint_refill` and enemy paint gives
  `enemy_ammo_regen_multiplier` × `paint_refill` bit-exact at every level;
  enemy 0 stays positive zero at +2, and Off on own paint equals the own
  multiplier unchanged. Unpainted floor, a fatally hit player and a dead
  player on own or enemy paint give `paint_refill` alone.
- **The bot's observation:** a Hard sprayer bot placed riding Team A's mover
  on `turbulence_mover_trial`, driven by the actual `MatchBotDriver` at
  Level's input boundary, opens a ride finishing window whose budget counts
  the refill at the observation's `paint_refill`
  (`MatchBot::open_finish`). Its first refilling window lasts 158, 138 and
  125 steps at Off, +1 and +2, under a disabled copy and the shipped tuning
  alike; the case requires each length to match its own level's
  `paint_refill` and no other level's, so the driver must observe the seat's
  Boost.

Validation: `handicap` and `paint_underfoot` pass in Debug and Release. No
tuning, content or game source changed in this slice; the 2026-11-05 switch to
shipping underfoot on (`POL#30`) moved the case's off half to a disabled copy.
The observation still carries the handicap's `paint_refill` alone, as
`MatchBotObservation` documents, and the author kept it so on 2026-11-13
(`REV#23`, closed) in the code's own words (`game/ai/match_bot.cpp`): at the
base rate with the seat's handicap, "own paint underfoot only shortens the real
refill, and the bot paints over any enemy paint on the top it holds, so the
window stays a sufficient minimum."

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
slot is part of `ROAD#2` (waived 2026-11-05, not performed).

The [shared resume count-in](polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds)
(`POL#18`, 2026-10-15) now holds the whole live match for two seconds after
explicit Resume or focus return. Returning a pad still leaves the menu open.
Movement/aim carry through; held fire/drop-through and stale jump/pause edges
are rearmed through release. Physical-controller comfort was `ROAD#2`'s,
waived 2026-11-05 and not performed.

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
and save-key mapping. The random roll uses the array length, and since `REV#14`
redraws when it lands on a pair whose `in_random` is false
([`ROAD#8`](#more-team-palette-pairs-road8)). A random weapon
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

The author selected docs and packaging first for release preparation on
2026-11-11. [Preparing a release](releasing.md) owns the current scope, the clean
candidate procedure and the author calls still open before publication.

Implemented:

- The Release `package` target produces `ColourWars-<version>-windows-x64.zip` with an
  explicit runtime asset list checked against the manifest. It includes the
  required DirectXTK, XAudio2Redist and release MSVC DLLs, plus licence notices.
  Since `FTR#6` (2026-11-10), the archive opens directly on the executable and
  `HOW-TO-PLAY.txt`; there is no extra enclosing folder or developer README.
  The version is the root `project(VERSION)`, and release notes are player instructions.
- Packages exclude source art, source audio, tools, saves and private legacy
  audio. Since the 2026-10-15 runtime migration they include the validated,
  distributable replacement bank. The optional-bank silent fallback remains
  supported. Debug packaging is rejected.
- The title screen shows the game revision and build configuration.
  `build-info.txt` records full game/engine revisions and backends. Metadata
  refreshes on each build, including local-change markers.
- The executable carries a VERSIONINFO resource stamped from the root
  `project(VERSION)`, the number CPack also uses; `check-startup.ps1` fails an
  executable that is not stamped with it. A pull request's full CI run keeps the
  inspected, started Release ZIP as the `ColourWars-windows-x64` run artifact for
  14 days. CI's `publish` job turns a matching `v*` tag into a GitHub Release.

Original package validation: Debug and Release builds; ZIP extraction into a temporary directory
outside the checkout; all 16 packaged assets matched against their sources; no
wave bank, source audio, content tools or debug symbols; launch with an isolated
save and a minimal system-only PATH; and inspection confirming the process loads
DirectXTK, XAudio2Redist and MSVC DLLs from the extracted package. The title-screen
identifier was visually checked. CI now builds and inspects the Release ZIP and
uses it for the existing startup smoke check. The separate deployment check on
a clean Windows machine (`ROAD#1`) was waived by the author on 2026-11-05 and
was not performed. The engine pin and source are unchanged.
The original validation predates replacement audio; current migration checks
are recorded in [audio inventory §9](audio-inventory.md#9-runtime-migration-and-remaining-integration).

### Windows Sandbox run of the Release ZIP (`ROAD#37`)

**Done 2026-10-30; rerun 2026-11-12 (`REL#2`).** A Windows Sandbox run proves that the ZIP loads and runs
on a clean OS. It does not prove that the game runs on a player's PC or GPU.
The clean-PC run of `ROAD#1` was waived by the author on 2026-11-05 and was not
performed, so that remains unproven; no CI job runs a sandbox.

**Labrador's run, recorded here.** Labrador's `PI-14` (master
`docs/product/BACKLOG.md:294`, at `b96bad6`) records a 2026-10-28 Windows
Sandbox run of this repository's CI Release ZIP (ColourWars `26a249d`, engine
`1e60fb2` = v0.2.0). The sandbox was Windows 11 Enterprise 10.0.26100, with no
Visual C++ runtime in System32 and no Visual Studio. The engine's
`tools/consumer_check/sandbox.py` set it up, and a script drove the run. The
sandbox's GPU was the host's RTX 5080 behind a paravirtual driver (guest driver
10.0.26100.1150, host driver 32.0.16.1692), so the game drew instead of
refusing. Synthetic key presses reached a keyboard-seat match and its pause
menu. Closing the window exited with code 0. The limits Labrador states
(`BACKLOG.md:2102`): a sandbox proves the loader and runtime requirements on a
clean OS, not a target GPU, and Release has no WARP fallback. Not checked: a
match on a physical pad, the Radeon iGPU (Labrador `SM-01`) and a
non-paravirtual GPU.

**The driver.** `scripts/sandbox-release.ps1 [-Zip <zip>] [-VGpu
Default|Enable|Disable] [-Run | -HostTrial]` uses
`out/build/x64-release/packages/ColourWars-<version>-windows-x64.zip` unless `-Zip` names
another. It writes a run folder under `out/validation/sandbox/` containing:

- `package/`: the ZIP and its extraction, mapped into the sandbox read-only.
- `results/`: holds a copy of `scripts/sandbox/logon.ps1` and is mapped
  writable.
- `clean-machine.wsb`: networking is off, and the sandbox runs the logon script
  at logon.
- `host.json`: the host's ZIP SHA-256 and, after a run, the verdict.

The logon script is self-contained and runs under Windows PowerShell 5.1. It
starts `ColourWars.exe` with `LOCALAPPDATA` set to the results folder, waits
for `ColourWarsWindowClass`, and captures the sandbox screen to `screen.png`.
It then closes the game with `WM_CLOSE`, as `check-startup.ps1` does, and
shuts the sandbox down. Before shutdown it writes `receipt.json`, which
records:

- the ZIP's SHA-256, hashed inside the sandbox;
- `build-info.txt`;
- the exit code;
- the game's stderr, stdout and `last-error.log`;
- the Windows caption, edition and build;
- whether `vcruntime140.dll`, `vcruntime140_1.dll` and `msvcp140.dll` are in
  System32;
- the display adapters.

The Release build has no `--smoke-test` (Labrador's script runs one), so the
logon script checks for the window, the save and a clean exit.

`-Run` starts `WindowsSandbox.exe` and waits for the receipt. It then checks
that the receipt's hash equals the host's and closes the leftover client
window. `-HostTrial` runs the same logon script on the host. It skips the
screenshot, because a host screenshot would capture the user's other windows,
and it skips the shutdown.

`scripts/check-sandbox-release.ps1` checks the generator without starting a
sandbox, using a synthetic ZIP. It checks:

- the `.wsb` XML: its elements, read-only and writable mappings, networking and
  logon command;
- XML escaping and relative paths;
- the copied ZIP and its extraction;
- that the logon script parses;
- refusals of bad input.

With `-Zip` the checker also runs a host trial of a real ZIP. The game cannot
open content through paths of 260 characters or more, so `-HostTrial` refuses
a deep `-OutputDirectory`.

**This machine's runs (MattPC, 2026-10-30).** All runs used a local Release ZIP
built from `74df941` (engine `1e60fb2`, SHA-256
`0C55115F...8555B9083`).

- **`-Run`:** the receipt's hash matched the host's. The guest was Windows 11
  Enterprise 24H2 26100.9711, with none of the three runtime DLLs in System32.
  Its adapters were an "NVIDIA GeForce RTX 5080" on the Microsoft paravirtual
  driver 10.0.26100.1150, plus the Remote Display Adapter. The title screen
  drew, showing "Build 74df941891da / Release". The save was created, stderr was
  empty, and `WM_CLOSE` exited with code 0.
- **`-Run -VGpu Disable`:** the same result. The paravirtual adapter was still
  present, so the missing-adapter refusal that Labrador describes did not
  happen. The receipt's `refused-no-hardware-adapter` path remains unexercised.
- **`-HostTrial`:** passed under both Windows PowerShell 5.1 and pwsh 7.6.9.

The first `-Run` produced no receipt. Windows PowerShell 5.1 leaves
`PSScriptRoot` empty in parameter defaults, so the script failed before
writing anything. The fix sets the default in the script body, and the host
trial now leaves `-Results` at that default as the sandbox does.

These runs did not include a match, any input, audio or a physical pad. They
also did not cover a target or non-paravirtual GPU.

**The `REL#2` run (MattPC, 2026-11-12).** `-Run` on the `REL#1` candidate ZIP
(ColourWars `ccfe9b7`, engine `e7fc50e`, `v0.3.0` plus nine commits, SHA-256
`B53BCC12...D453EFD99407E180B90E0426CD5E3`) passed. The receipt's hash matched
the host's. The guest was Windows 11 Enterprise 24H2 26100.9974, with none of
the three runtime DLLs in System32. Its adapters were the RTX 5080 on the
paravirtual driver 10.0.26100.1150, plus the Remote Display Adapter. The title
screen drew with "v0.1.0 / Build ccfe9b7ee877 / Release", the save was created,
stderr and `last-error.log` were empty, and `WM_CLOSE` exited 0. This is the
run Labrador's `PI-14` waited for at `v0.3.0`. It had the same limits as the
earlier runs: no match, input, audio, physical pad or target GPU.

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
  A physical-controller menu session was part of ROAD#2 (waived 2026-11-05, not performed).
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

The acceptance work below needed hardware. The author waived all of it on
2026-11-05 and **none of it was performed**: the clean-machine run (item 1,
`ROAD#1`) and the physical-pad sessions (items 2-6, `ROAD#2`) are closed as
waived, untested, not passed. The list stays as the record of what a
hardware session would check:

1. Run the extracted Release ZIP on a clean Windows 10/11 x64 machine.
2. Play two- and four-player sessions across the three arenas and five weapons,
   checking ammo exhaustion/recovery, paint coverage, readable HUDs and controls.
   In a match that mixes the mouse seat with pads, note whether pad seats
   consistently lose fights to it (story B8 and `ROAD#24`'s trigger in
   [design-process.md](design-process.md#open-points-for-the-author)).
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
5. Relaunch and confirm saved settings; record device models, which device the
   host played on (persona P1's hypothesis), display layout,
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
and separate screen-saver observations were item 6 above in `ROAD#2`, waived
on 2026-11-05 and not performed.
The existing `0bd8f45` Release ZIP predates ROAD6; no replacement package was
produced for this increment.

### Per-seat accessibility toggles (`ROAD#7`)

**Implemented 2026-10-27; accepted 2026-11-05.** The look on 1-4P panes with
pads was waived by the author on 2026-11-05 and was not performed.
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

### Toggle fire (`REV#31`)

**Implemented 2026-11-12; not played.** Shoot was hold-only on every device. The
2026-11-10 project review
([REV#31](review/2026-11-10-project-review.md#input-accommodations-beyond-rebinding-rev31))
found hold-to-toggle the one input accommodation with a clear case, so each pad
now has a **Toggle fire** row: Off (the default) holds to fire as before; On,
a Shoot press starts firing and the next press stops it, with nothing held.

- **Per pad, Off/On.** It rides in the `ROAD#7` record (`SeatComfort`) and so
  takes its save, profile carry, edited-slot Apply and worn-profile write-through
  for free, but it has no Shared value. The Shared pattern needs an Options
  switch to follow, and Controls / audio has no free row (eleven rows end
  above its status line). The keyboard seat therefore holds to fire.
- **Page.** Pad sticks row 7, after Reduce effects; Apply and Back move to
  rows 8 and 9 (`settings_rows::sticks_toggle_fire`, `sticks_apply`,
  `sticks_back`). Copy is `menu_text.items.toggle_fire` with the shared On/Off
  values, and startup validation fits the label and both values.
- **The latch** lives in `PlayerInput`, so recorded rounds and replays carry
  the resolved Shoot and need no format change. Every chosen Shoot control,
  alternates included, flips it on its press edge; a repeated read of one frame
  does not flip it twice. It stops, and needs a fresh press to start again, when
  the pad disconnects (a trigger held across the reconnect does not start it),
  when the option turns off, and when `GameLevel` stops reading pads: a new
  round, Pause, Results and focus loss (`PlayerInput::release_toggles`, the
  last from `GameLevel::on_deactivated`). A Shoot press during the resume
  count-in is dropped like a Sub press, so the first press after the count
  starts firing. Bots
  and the Sub are unchanged.
- **Save.** Version 10 adds `"toggle_fire"` (a boolean) to each `pad_comfort`
  entry and each `profiles` entry; absent or malformed reads as false, so older
  saves play as before.

CPU acceptance: `toggle_fire` drives the real `PlayerInput` with scripted pad
snapshots (latch, hold default, alternate control, repeated reads,
`release_toggles`, disconnect and held reconnect, Off mid-latch, the keyboard
seat); `settings` checks the save field's canonical writing and per-field
recovery; `player_profiles` the profile carry, worn overlay and recovery;
`menu_text` the copy. The painter's `-ControlsMenuOnly`, `-RumbleOnly`,
`-PercentageTextOnly`, `-StickReadoutsOnly`, `-MenuLabelsOnly`,
`-SettingsMenuOnly`, `-PausedOptionsOnly`, `-ResumeOnly`, `-PracticeReplayOnly`
and `-KeyboardJumpOnly` leaves pass with the longer page on this machine
(2026-11-12). How it feels on a pad has not been tried. Stick swap, d-pad
movement and follow-movement aim stay unscoped beside `ROAD#24` and `GAP#4.2`.

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

A content-only extra pair still joins Random without a rebuild unless it says
`in_random: false`, and now has to pass the same bar. [The preview sheet](images/road8/team-pairs-colour-vision.png)
shows every pair on a Gainsboro surface under each simulation. Nothing here
qualifies the colours on screen: the opt-in visual cases that capture palettes
(`paint_fx_visual`, `painter_visual`, the console's opening art check) still
name the original three.

**Random skips the two legacy pairs (`REV#14`, taken and built 2026-11-13).**
Each pair in `team_palette` takes an optional `in_random`, true by default.
`pink_green` and `blue_yellow` say false, so they stay fixed Options choices and
Random never lands on them. The roll redraws from the same palette stream until
it lands on a pair in Random, rather than taking the draw modulo the filtered
count: both are uniform over the four passing pairs, but the redraw moves only
a seed whose first draw was exempt. Of the pinned seeds, 0 moved from
`blue_yellow` to `purple_gold`, and 211 and 4242 from `pink_green` to
`blue_orange`; seeds 1-8, and so seed 7 that the review scripts use, kept their
pairs (filter-then-modulo would also have moved 3, 5, 6 and 7). No Debug tape
is tracked; a local one still replays bit for bit, because the match fingerprint
hashes team indices rather than colours and the palette stream feeds nothing
else, though `play` warns that `presentation.json` differs and a tape whose
seed first drew an exempt pair shows the new pair. Startup refuses a file with
no pair in Random, and a pair out of Random that is not a fixed Options choice
(it would be unreachable). `team_palette` ties `in_random` to the exemption
list, checks that 1,000 seeds never draw an exempt pair and reach every other,
and pins the [`+seed` table](debug-console.md#seed-record-and-play); `locales`
checks that a locale cannot set the flag. This revisits the author's earlier
answer here and to gap audit 2.1, fixed choices plus three new passing pairs
rather than a roll filter, while keeping the random-pair model.

### Controller rumble (`ROAD#9`)

**Local implementation qualified, 2026-10-26; accepted 2026-11-05 without a
physical-feel check.** The author waived the pad session on 2026-11-05; rumble
has not been felt on a real pad, and the pulse values ship as they stand. In a four-pad couch game rumble is the only private
per-player channel: the screen and speakers are shared
([gap audit F6](review/gap-audit.md)). The engine's `GamepadReader` exposed no
vibration when this was built, so it began as a client feature and the exhibit
for report 25, which the engine answered with `Gamepads::vibrate` (`cadc962`,
`v0.3.0`).

1. **Client slice.** Short, distinct pulses for being hit, being eliminated,
   a hard landing and a dry trigger, read from `tuning.json`. A per-seat On/Off
   beside the stick settings, saved with them, default On. Bots, replays and the
   keyboard seat never rumble. Pause, focus loss and the end of a match stop
   every motor. The call was a direct `XInputSetState` on the seat's slot,
   isolated in one file because it bypassed the engine's reader; since
   `ENG#25` it is the engine's `Gamepads::vibrate`.
2. **Report.** That file was the exhibit for `GAP#F6`'s ask,
   [report 25](review/engine-reports.md#25-no-public-controller-vibration-output--accepted-narrowed-cadc962);
   it was answered unfiled in `cadc962` and spent 2026-11-05.

Acceptance is a CPU case over a fake vibration sink: which events pulse which
seat, the per-seat setting and every stop condition. Feeling it needs pads
(`ROAD#2`), which was waived on 2026-11-05 and not performed.

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
validation. The author accepted their values on 2026-11-05 without the
physical-pad review, which was waived and not performed.

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
| Report filing | [Report 25](review/engine-reports.md#25-no-public-controller-vibration-output--accepted-narrowed-cadc962) was answered unfiled (`cadc962`, released in `v0.3.0`) and spent 2026-11-05 (`ENG#25`). |

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

`game/input/rumble_gamepads.cpp` (`colourwars_rumble_gamepads`) sends through the
engine's `Gamepads::vibrate` and `stop_vibration` (`ENG#25`, 2026-11-05; it was
`rumble_xinput.cpp` over `XInputSetState` before). It checks what the engine
would refuse first: invalid amplitudes or deadlines stop the slot, deadlines
past `max_vibration_seconds` are clamped, and nothing the engine throws leaves
the sink. Each nonzero request carries the time until the output falls to zero,
and `Rumble` renews it at every audible trigger and whenever that time grows,
because the engine ends a request on wall time while the policy's timers run on
game time.
The `rumble_adapter` case drives it through a real `Gamepads` over a fake
`GamepadReader`. Failures are nonfatal, without retries or log spam.

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

The physical feel review was not closed with fake-sink results: the author
waived it on 2026-11-05 and it was not performed. The native hook does not
require a pad to build, and nothing here claims that the four pulses feel
distinct or comfortable.

### Input devices and seats

Known limits, recorded so a later gap list does not file them again. The first
now has a row.

- **XInput only.** DualShock, DualSense and Switch Pro pads work only through
  Steam Input or a wrapper. The engine's reader seam
  (`engine/input/gamepad_reader.h`) is a concrete class chosen at build time:
  `sample`, `read`, `suspend`, `resume`, `vibrate` and `stop_vibration`, with
  XInput and browser backends at the pin (`v0.3.0` plus the vsync merge
  `e7fc50e`; vibration from `cadc962`). A GameInput backend is still Labrador
  work and would need an exhibit before it is a report. `ROAD#2` would have
  observed the limit (item 6 above); that session was waived on 2026-11-05
  and not performed, so the limit is still assumed rather than observed. The author asked for it to be tracked on 2026-10-27
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
is a first slice, before attended review. The sections below record the slices
since selected and their remaining gates.

Two gaps were already tracked and get no new row: a special charged by
painting was `POL#30` (answered no on 2026-11-03), and more arenas are the current aim (`POL#21`,
`POL#31`).

### Combat and tools

- **Sub weapons (`ROAD#11`).** Splatoon pairs each main weapon with a sub
  (bombs, Splash Wall, Sprinkler) that costs a large slice of ink. `Player`
  ignored a placeholder secondary until the first slice gave every player a
  paint grenade (2026-11-08). A sub changes the reserve and balance game, so
  it waited for `POL#27`; the five-tool balance was accepted on 2026-11-05.
  Bots (`ROAD#10`) need to use it. [Plan](#sub-weapons-road11).
- **Results awards (`ROAD#12`, done 2026-10-27).** Splatoon 3 medals and
  TowerFall awards give each player a few call-outs after a match. See
  [Results awards](#results-awards-road12) below.
- **More tool archetypes (`ROAD#13`).** Splatoon's brush, blaster, splatling,
  dualies and brella have no counterpart among the five tools; a fast-moving
  painter or a shield tool would differ most. Each new tool needs art, audio
  and bot ballistics. It waited for `POL#27`, which the author settled on
  2026-11-05, so it is no longer blocked. A new subclass suits the OOP rule.
  **Proposal recorded (2026-11-12), answered (2026-11-13):** the
  [umbrella tool proposal](#umbrella-tool-proposal-road13) recommended a
  Splatoon brella, and the author took it and answered Q1-Q7. Its first slice,
  `REV#3`'s fix, was done on 2026-11-13. Building slices 2-4 is a separate call
  after `PIN#2`.
- **Map pickups (`ROAD#14`).** TowerFall chests, Paint Rumble power-ups and
  Smash items. Needs a match toggle and bot awareness. It waited for
  `POL#27`, which the author settled on 2026-11-05, so it is no longer blocked.

### Stages

- **Paint-activated stage objects (`ROAD#15`).** Splatoon's propeller lifts
  rise while inked, inkrails open for the team that inks their bulb, and
  sponges grow. The [horizontal movement capability](#horizontal-movement-capability-2026-10-26)
  supplies the motion; paint ownership driving it is new. Arena use follows
  `POL#21`, and bots must ride the objects. The first slice, the
  [paint ferry](#paint-ferry-road15-first-slice), is on an explicit trial stage,
  and so is the second, the [team gate](#team-gate-road15-second-slice), a
  wall that the team holding its paint walks through.
- **Player-made stages without rebuilding (`ROAD#16`).** A stage saved by the
  editor played only after a manifest edit and a rebuild. First slice done
  2026-10-28: a per-user stage folder, each file validated on its own, a
  broken one refused without stopping startup; manifest stages stay
  never-optional. The editor's Save to My Stages, a rescan as Stage Select
  opens and a README note for players followed the same day. The author's
  2026-11-05 calls: a user stage joins Random only when its file says
  `"include_in_random": true`, and "Built by you." stays its only marker.
  See [User stages](#user-stages-road16) below.

### Awareness and presentation

- **Overview map (`ROAD#17`, declined 2026-11-03).** Splatoon's map shows ink
  and teammates, and the three-player layout leaves the fourth quadrant
  unused. The author declined it; the reason is under
  [Not tracked, by choice](#not-tracked-by-choice).
- **Off-screen teammate and threat markers (`ROAD#18`).** Splatoon shows
  allies through walls. Teammate markers done 2026-10-28; the author decided
  on 2026-11-05 to draw no threat markers. See
  [Teammate markers](#teammate-markers-road18) below.
- **End-of-round instant replay (`ROAD#19`, done 2026-10-27).** TowerFall
  replays each round's final seconds and can save a GIF. The End menu now
  offers Watch Replay; see [Instant replay](#instant-replay-road19) below.
  Saving a clip was not taken up. A Debug whole-round time-lapse followed
  ([`REV#48`](#whole-round-time-lapse-rev48)); a player-facing one is not
  designed.

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
  below. The author made Rotate the default on 2026-11-05.
- **Player profiles (`ROAD#21`, done 2026-10-27).** Smash name tags carry a
  person's controls, rumble and records. Deadzones, rumble, binds and
  accessibility toggles followed the pad slot (`POL#18`, `ROAD#7`, `ROAD#9`,
  `GAP#4.2`), so swapping controllers swapped settings. A pad can now wear a
  named profile that carries them. See [Player profiles](#player-profiles-road21)
  below.
- **Match variants (`ROAD#22`, endless paint implemented 2026-11-07).** The
  selected first slice is a named C++ option with a bot refill switch. See
  [Endless paint](#endless-paint-road22). The author's 2026-11-03 ruling keeps
  variants inside timed territory scoring; the accepted five-tool balance
  (`POL#27`, 2026-11-05) removed the implementation dependency. Low gravity and
  tool roulette have no implementation scope; revisit them only when selected.

### Endless paint (`ROAD#22`)

**Implemented 2026-11-07.** Mode has a **Variant** row, cycling **Standard** /
**Endless paint**, for single matches, series and solo practice. Standard is the
default. `MatchVariant` is an engine-free C++ vocabulary; `MatchSetup` owns the
choice, and no arbitrary JSON overrides or new ruleset seam is introduced.

Endless paint keeps every participant's tank full across all five tools. A
weapon's supply policy bypasses ammunition cost and affordability, including
valid tuning whose shot costs more than a full tank. Cooldowns, volleys, aim,
damage and movement retain their normal behavior. Supply never temporarily
falls low during a shot, so low/empty/recovered feedback does not chatter;
normal shot and cooldown-ready cues still apply. Respawn retains the policy.
The Debug `ammo` cheat remains independent: restoring normal cheats cannot
disable the shipping match option.

Shipping bots receive an explicit refill switch. Under Endless paint they
continue normal painting and engagement without reserve holds, refill retreats
or refill-only ground queries. Disabling refill also clears an existing refill
state. Difficulty, navigation and combat otherwise keep their normal policy.

The choice is copied through restart, rematch, series re-picks and shipping
instant replay. Save version **7** adds optional `last_match.variant`:
`endless_paint` is written explicitly; omitted, malformed or unknown values use
Standard. Debug replay format **9** requires each round's variant and refuses
older formats, consistent with replay's same-build contract. Player-facing copy,
including the bracketed test locale, lives in `presentation.json` and its locale
overlay; startup checks every label/value with the captured Mode font and paper.

The win condition remains the most painted surface at time up; solo practice
retains its existing untimed flow. Results and records follow their existing
paths, with no separate variant leaderboard.

**Validation.** Debug and Release game/editor builds pass under `/W4 /WX`,
along with the 13 selected consumer cases `match_variants`, `ammo`, `low_paint`,
`weapon_feedback`, `bot_driver`, `settings`, `remembered_setup`,
`moving_structure`, `menu_text`, `menu_presentation`, `series_stage_rule`,
`instant_replay` and `debug_console` in both presets. The new variant case
checks all five tools for human and bot players, multiple tanks of held fire,
normal cooldowns, release/death/respawn, valid expensive-shot tuning, timed
coverage scoring and shipping replay with Debug cheats off. Bot coverage checks
all difficulties and tools, low/empty observations, releasing an existing refill,
moving-support stall timing and the setup-to-driver switch. Save and Debug codec
cases cover legacy/default recovery, invalid values and multi-round replay.

The focused `-SetupPaperOnly` native matrices pass on hardware D3D11 (NVIDIA
GeForce RTX 5080) in both builds: actual Mode/Player Count pages across six
sizes/aspects, variant cycling and retention, independent paper/card fixtures,
focus motion and resize. Debug's `-DurationTextOnly` also passes its moved-row
capture and live-text/resource checks. These are selected regression and native
presentation checks, not full CI, physical-pad or human gameplay acceptance.
Isolated startup passes with the accepted audio bank and a system-only PATH in
Debug and Release, for both base English and the bracketed test locale, including
per-monitor v2 DPI awareness and graceful shutdown. Markdown links and whitespace
checks pass. Evidence: `out/validation/road22/`.

### Sub weapons (`ROAD#11`)

**Scoped 2026-11-08.** Every player carries a sub weapon beside their tool,
fired with its own button. `Player` was built with a secondary
(`WeaponType::grenade_std`) that it ignored; the first slice removed that
placeholder for a separate `SubType` vocabulary, so the five tools and the
subs stay two lists.

**Decided (the author, 2026-11-08).** Subs are a **loadout**: each player picks
one on Weapon Select beside their tool, Random included, and carries it all
match. Fixed kits per tool and map pickups (which would overlap `ROAD#14`)
were declined. A sub charged by painting is out because `POL#30` was answered
no.

**Confirmed (the author, 2026-11-08).** The three subs, the cost and the
controls below were proposed and accepted as written.
- **The three subs.** *Paint grenade*: lobbed along the aim, bounces, bursts
  after about a second into a circle of paint that damages enemies in range.
  *Sprinkler*: sticks to the first surface it hits and sprays short bursts
  around itself for about 8 seconds; enemy shots destroy it. *Paint wall*: an
  upright barrier in front of the player for about 4 seconds that stops enemy
  shots and paints the floor beneath it. Considered instead: a paint mine and
  a splat jump pad.
- **Cost.** A sub spends a large share of the same paint tank as the tool
  (about half), and fires only with the whole cost in the tank. It reuses the
  tank, the `POL#38` low-paint warning and the paint-underfoot refill. Under
  Endless paint (`ROAD#22`) the cost is ignored but a per-sub cooldown stays,
  so subs cannot be spammed. The alternatives were timed charges separate
  from the tank, or a smaller tank cost plus a cooldown.
- **Controls.** A new rebindable Sub action: pad RB and the keyboard seat's
  right mouse button, both unbound today.

**Shape.** C++ says how each sub behaves; `tuning.json` says how much (cost,
cooldown, fuse, radius, lifetime), and its copy lives in `presentation.json`.
A `SubWeapon` base, owned by `Player` beside its `Weapon`, launches a sub
object through new subclasses (`ProjectileGrenade`, a sprinkler and a wall that
are also `Collider`s), in the game's usual OOP grammar. `PlayerInputData` gains
a sub request, so bots, round tapes and Debug replays record it like any other
command. Debug replay and save formats move a version each where they change.

**Slices**, each a pull request with its own acceptance case:
1. **Grenade end to end.** Sub action through bindings; `SubType`, `SubWeapon` and `ProjectileGrenade`; tank cost, Endless
   paint cooldown; every player carries the grenade. Placeholder art and
   existing waves.
2. **Bots throw grenades**, using the existing ballistics for the arc, under
   each difficulty and the refill switch.
3. **Sprinkler.**
4. **Paint wall**, including which shots it stops.
5. **Loadout.** Sub row on Weapon Select with Random, `last_match` sub, the
   Debug match plan, the Sub action on the Controls pages and in the save, a
   HUD readiness mark on the tank, and localisation.
6. **Bots use the sprinkler and wall**, then a balance pass of sub costs
   against the five tools with the bot trials.
7. **Art and audio.** Final sprites and new cues in the `paint_exe` bank with
   provenance.

Then the author's attended play review, which implementation does not close.

**Play review waived (the author, 2026-11-13), not performed.** For the
`v0.1.0` release the author waived the attended review, as the hardware,
participant and listening checks were waived on 2026-11-05. The sub weapons
ship as implemented, with the costs and bot use below. The rows that waited on
the review are released: `REV#3`, `REV#21` and `REV#56` are ordinary work, and
`REV#36`'s respawn grace keeps its own condition, observed spawn pressure.

**Slice 1, done 2026-11-08.** `GameAction::sub` is a new action: pad RB (LB
when a pad profile already uses RB) and, on the keyboard seat, whichever mouse
button Shoot does not use. `PlayerInputData::sub_pressed` carries it, the
resume countdown clears it, and Debug replays move to format 10 for its flag.
`SubType` ([sub_type.h](../game/objects/sub_type.h)) names the subs, and
`tuning.json` `subs.grenade` holds the cost (half the tank), the Endless
cooldown, the throw and burst speeds, the burst's drop count and the body and
burst projectile records. `Weapon::spend_paint` takes a sub's cost from the
tool's tank only when the whole cost is there, and leaves it alone under
Endless paint, where the cooldown alone limits throws. `SubWeapon`
([sub_weapon.h](../game/objects/sub_weapon.h)) is owned by `Player`, and
`SubGrenade` throws a `ProjectileGrenade` along the tool's aim (the roller's
by its tilt) with the player's run added. The grenade bursts into a ring of
paint drops where it hits, or in the air when its fuse runs out;
`Level::burst_projectiles` admits the drops after the tick's contacts. Each
projectile now carries its own player damage, so the drops hurt like the
burst record says. Placeholder art is the ball shot, and its sounds are the
bucket's throw and the empty click. The `sub_weapons` acceptance case checks
the tuning, the tank, the throw, every player's grenade, the controls and a
real round.

Left for later slices, by plan: the grenade does not bounce (a bounce needs
the swept contacts to report a surface normal, so it waits for the art pass or
a play note asking for it); Sub is not yet listed on the Controls pages, so it
is not rebindable; there is no HUD readiness mark; bots do not throw.

**Slice 2, done 2026-11-08.** Bots throw the grenade. `BotBody` and the
bot's observation carry the seat's sub and whether a press would throw it
(`SubWeapon::ready`). On a reaction with a visible enemy, outside a climb or
recovery and not refilling, `MatchBot` asks
`match_bot_ballistics::throw_angle` for a throw from its centre, with its run,
that passes within `bots.sub_throw_miss` (120 units, about the burst's reach)
of the enemy; the roller's throw is fixed by its tilt, so a roller bot throws
only when that arc reaches. The throw takes the decision's aim error, so
difficulty spreads it as it does a shot, holds the tool's shot for that step
so the tank pays for the throw, and is a single-step press; the next step
reacts afresh. With refill on, a bot holds a throw that would leave its
reserve at `bots.reserve_low`; under Endless paint only the cooldown paces
it. The `sub_weapons` case checks every difficulty and tool against the
ballistics, the holds (not ready, no sub, out of reach, short tank) and two
real bot rounds, one per refill rule.

**Slice 3, done 2026-11-09.** The sprinkler is `SubType::sprinkler`, thrown
by `SubSprinkler` as a `ProjectileSprinkler`. It flies like the grenade;
the first cover its body meets plants it there, where it stops, empties its
collision mask (so neither cover nor players touch it, and it deals no
damage) and, every `interval` (0.6 s) for `lifetime` (8 s), sprays
`burst_count` (5) drops in a fan over its top half, every other spray turned
to fill the last one's gaps. An enemy player met first, or the body's
`delete_timer` (2 s) running out in the air, retires it unplanted, like a
shot. Enemy shots break it: `Level::strike_breakables` runs after the tick's
contacts, spends each enemy shot whose travel this tick
(`Projectile::travel_bounds`) reaches a projectile that `takes_hits`, and
the `hits`th (3) retires it; its own team's shots leave it be. (Since
umbrella slice 1, `REV#3`, swept shots meet it along their path instead; see
the [umbrella proposal's slice record](#umbrella-tool-proposal-road13).)
`Projectile::take_burst` is now asked every tick, which is how the planted
sprinkler sprays. It costs half the tank, with a 4 s cooldown. Each seat's
`SeatSetup::sub` chooses its sub, defaulting to the grenade, which
`PlayerBuilder` reads; nothing but code sets it until slice 5, so the save,
the Debug match plan and replays do not carry it yet, and every menu match
is still the grenade. The `sub_weapons` case checks the sprinkler's tuning
and what the loader refuses, a seat carrying it, and in a real round that
it sticks, sprays on schedule, paints, retires after its lifetime, and
breaks on its third enemy shot but not on its own team's.

Left for later, by plan: a sprinkler planted on a moving platform stays
where it stuck rather than riding it, and on a wall or ceiling it still
sprays upward. Slice 6 taught bots where to plant it.

**Slice 4, done 2026-11-09.** The paint wall is `SubType::wall`. The
sprinkler's planted behaviour moved into an abstract `ProjectilePlanted`
([projectile_planted.h](../game/objects/projectile_planted.h)), which both
`ProjectileSprinkler` and the new `ProjectileWall` derive from: flight, the
plant on first cover, the emptied mask, the `interval` and `lifetime`
schedule and the `hits` that break it are shared, and each says only what it
releases (`spray`). `SubWall` does not throw: it sets a 32 by 180 wall down
`reach` (140 units) to the side the player aims, its foot level with theirs,
dropping straight so it stands on the floor below. `Projectile::launch_origin`
lets the wall start its launch-obstruction check from the player's side
rather than their centre. Standing, it spends every enemy shot that reaches
it through `Level::strike_breakables` (the `hits`th, 15, breaks it) and none
of its own team's, drips `burst_count` (3) drops along its foot every 0.5 s
so it paints the floor beneath it, and goes after 4 s. Players of either
team walk through it, since its mask is empty: it is cover from shots, not a
barrier. It costs half the tank, with a 4 s cooldown. The `sub_weapons` case
checks the tuning and what the loader refuses, the placement to either side
and its cost, and in a real round that it stands, stops enemy shots but not
its own team's, paints, retires after its lifetime and breaks on its last
hit.

Left for later, by plan: a wall set down over a gap falls until it lands or
its body's `delete_timer` (1 s) ends it; one set down against a step is
not moved clear of it, and plants where it touches. Slice 6 taught bots when
to set it down.

**Slice 5, first part, done 2026-11-09.** Each row on Weapon Select picks its
tool, then its sub: A moves from the tool to the sub, Left/Right cycles the
grenade, sprinkler, paint wall and Random, and A again readies the row; B
steps back one step at a time ([LoadoutChoice](../game/states/loadout_choice.h)).
The sub is named on the row's Boost line (`menu_text.subs`), and its
description takes the tool's place while it is chosen, so each row keeps its
height. Random draws when every row is ready, as the tool's does. The seat's
sub goes into the remembered setup (save version 8, `last_match.players[].sub`,
omitted for the grenade), the Debug plan (`seat <n> sub
grenade|sprinkler|wall|random`) and replays (format 11, each seat's `sub`,
omitted for the grenade). The HUD marks readiness after the PAINT label: a
keylined square, solid ink when a press would release the sub, paper when not,
and an ink tick across the PAINT meter at the sub's cost outside Endless
paint (`hud.sub_mark`). The `sub_weapons` case checks the row's steps and
Random, the copy and the HUD's cue; `remembered_setup` the save; the Debug
console's roster checks the plan.

**Slice 5, second part, done 2026-11-09.** Sub is a rebindable action like
the others. Keyboard Keys gains a Sub row, which takes a key or any of the five
mouse buttons, as Shoot does (`KeyboardAction::sub`, default right click). Pad
Buttons gains one Sub row, default RB, with no second control: the page has
room for one more row, not two (`PadAction::sub`, `pad_entry`). One control
still does one action, so Shoot and Sub cannot share a mouse button or a pad
control. The save moves to version 9 (`input_bindings.keyboard.sub` and each
pad profile's `sub`, also in `profiles`); an older save reads Sub where older
builds put it: the right mouse button, or the left when Shoot had the right,
and RB, else LB, else the first free control. The Controls help gains a Sub
line for each device, and the countdown reminder names Sub
(`{sub}`, `{pad_sub}`). The `input_bindings`, `keyboard_jump_capture`,
`player_profiles`, `sub_weapons`, `menu_text` and `locales` cases check the
domains, defaults, conflicts, save migration, capture and copy.

**Slice 6, done 2026-11-09.** Bots use each sub by its own rule
(`MatchBot::sub_use`), on the same reaction steps, holds and aim error as the
grenade. The grenade is thrown as before, at a visible enemy a flight
reaches. The sprinkler is planted while no enemy is engaged, standing on
scoring floor the bot's team has not painted: a throw that lands within
`bots.sub_throw_miss` of the floor three body widths ahead, where the bot is
heading. The wall goes down when a visible enemy's tool reaches the bot, the
enemy is no higher than the wall's top and no lower than the bot's floor, and
far enough away that the wall, set down `reach` to that side, lands between
them; a roller bot waits until it faces that way, since its sub goes down on
the side it faces. The `sub_weapons` case checks every difficulty and tool
against each rule and its holds, and a real bot round per sub under each
refill rule.

The balance pass ran 1,440 one-minute bot rounds on the Linux harness: one
Normal bot a side on crossfeed, close_quarters and king_of_the_hill, both
sides carrying the same tool, each of the five tools, eight seeds, each side
in turn, one side carrying the grenade and the other each sub. The measure
is that side's share of the painted area; grenade against grenade averaged
0.50 (0.49 after the change), the noise being about 0.03.

| Side's sub, against the grenade | Standard | Endless, before | Endless, after |
|---|---|---|---|
| Sprinkler | 0.49 | 0.41 | 0.44 |
| Paint wall | 0.48 | 0.39 | 0.42 |

Under the refill rule, where the half-tank cost paces every sub, the three
are level within the noise, so every sub keeps the decided half-tank cost.
Under Endless paint the cooldown alone paces them and the grenade led, being
the only sub that also hits an enemy. Its cooldown moves from 2.5 s to 3.5 s
and the sprinkler's and wall's from 4 s to 3 s, which narrows the gap without
changing the refill rule's result. What is left is mostly the bots' play: a
roller bot never sets a wall against another roller, whose reach is shorter
than the wall's, and loses the most with either planted sub (0.30 to 0.33
under Endless paint); a sniper bot rarely finds a grenade throw, so either
planted sub beats its grenade (0.58 to 0.63). Those are bot policy and a
human's play, not costs, and were left to the author's play review (waived
2026-11-13). The
measuring probe was a scratch case and is not kept.

**Slice 7, done 2026-11-09.** Each sub draws and sounds as itself. The
`paint_fx` sheet gains three looping body strips, `sub_grenade` (a round paint
bomb whose cap wobbles), `sub_sprinkler` (a squat nozzled dome throwing beads)
and `sub_wall` (an upright slab with changing drips), drawn in the same
keylined grayscale as the shots and tinted by team; the atlas grows to 2048
rows ([paint-fx README](../game/art/paint-fx/README.md)). Three foley masters
join the `paint_exe` bank, cut from recordings it already retains, so no new
source or licence comes in: `sub.grenade.throw`, `sub.sprinkler.throw` and
`sub.wall.set` ([foley README](../game/audio/paint_exe/foley/README.md)). The
author accepted all six on 2026-11-09, recorded as an amendment in
[acceptance.json](../game/audio/paint_exe/acceptance.json); the bank holds 106
waves. The bursts and drops stay paint balls, and the empty click is the
tools'. The author's attended play review was waived on 2026-11-13.

**Player documents, 2026-11-10.** `HOW-TO-PLAY.txt` and the README gained the
Sub control row and a loadout paragraph, and the README's Pad Buttons and
Keyboard Keys sentences now name Sub (`REV#9`). The `SeatSetup::sub` comment
says what slice 5 made true (`REV#8`). The Movement Course still had no sub
station (`REV#56`).

**Sub lesson, 2026-11-13.** The Movement Course gained a `sub` station and
lesson at the right runway's far end (`REV#56`,
[polish-movement-course.md](polish-movement-course.md#practice-workshop)).

**Nightly follow-up, 2026-11-10.** No scheduled nightly ran between the
slices. The first after them failed eight Release painter leaves, one Debug
leaf, the plain matrix and one opt-in case, for three reasons.

*Stale painter drivers.* The fixtures still readied a Weapon Select row with
one Confirm. They now send the tool's Confirm and then the sub's, and two
Backs from ready to the tool: the series re-pick (`EndMenuOnly` and the plain
matrix), `RememberedSetupOnly`, `WeaponCatalogOnly`, `WeaponPaperOnly`,
`BotPlayersOnly`, `SetupMotionOnly`, `StagePreviewBackgroundTuneColourOnly`
and Debug's `WeaponTuneColoursOnly`. `ControlsMenuOnly` pressed Down one row
short of Apply on Keyboard Keys, which had gained the Sub row. The plain
matrix's menu-music flow was one Confirm short too; the hosted runner has no
audio endpoint and skips that flow, so only a run on a machine with one showed
it. The setup-motion leaf records two more rows per size (`weapon-sub`,
`weapon-sub-back`), and the preview-backing leaf's pinned tap, update and poll
budgets grow by its extra Confirms.

*One regression, fixed.* Slice 5 made Weapon Select's commit call `rand()`
once per seat for the sub whether or not the sub was Random, so committing a
concrete loadout moved the seeded sequence that Stage Select's Random and each
round's seed draw from; before the slice only a Random tool drew.
`MainMenuWeaponSelect::set_level_settings` now draws for a sub only when it is
Random, after that seat's tool. The preview-backing leaf pins that a concrete
commit draws nothing, and the weapon-paper leaf one draw per Random tool.

*The home-band call (the author, 2026-11-10): change the bot.*
`bot_team_trials` requires every bot's body to leave its home band and counted
559/560 after the slices. The miss was trial 66: Close Quarters, mixed
rotation 0, seed `3946`, human on team b, whose lone team-b Sprayer faces a
Roller and a Bucket. At step 454 it threw its grenade with 0.79 of a tank,
reached the reserve floor by step 500 a hundred units inside its band's edge,
backed away to refill from the Bucket bot, and died five units short of the
edge; its second life was fought inside its own third. It also logged an
off-home-paint miss, as did seed `8644` on the same side (one-bot retention
118/120). This was the side-swapped sibling of the pinned
[Close Quarters `61403` loop](#home-band-loop-slow-tool-spending-and-sustained-middle-retention-2026-10-27),
which the loop's guard, comparing only the `human=a ... team=a` key, could not
see. Of the two choices recorded, reclassifying the miss as an outnumbered
outcome or changing the bot, the author chose the bot.

*The change.* A grenade thrown at an enemy beyond the bot's own tool reach (not
`engaging`) must now leave `bots.sub_reserve` of the tank, 0.40, so a bot
throws that way only from 0.90 or more. Every other sub use keeps the old
rule, leaving the tank above `reserve_low`. Holding the throw only in a fight
left trial 66 failing, so the throw that sent it home was a long one. Applying
the reserve to every sub use, or to every grenade, moved the miss to other
outnumbered Close Quarters trials at 0.30-0.35 (seed `5242`'s lone Sprayer or
Roller kept no off-home paint) and at 0.50. The narrowed rule passes the whole
case at 0.38, 0.40, 0.42 and 0.45; 0.40 is the middle and keeps one-bot
retention at 120/120. A first version that also held the sprinkler cut its
planting by about 80% (to nearly none on King of the Hill), which is why the
reserve is the grenade's alone.

*Results.* `bot_team_trials` passes: bots leave their home band 560/560, and
mixed one-bot teams retain off-home paint 120/120. The loop guard now names
both sides' keys. `sub_weapons` checks the new hold and that a sprinkler still
plants on a tank just above the low mark; `bot_driver` rejects a missing or
too-low `sub_reserve`. Every opt-in case and the default Release and Debug
cases pass.

*Balance, measured again.* A scratch probe (not kept) ran 1,440 one-minute
rounds under the Standard refill rule: one Normal bot a side plus one idle
human a side, crossfeed, close_quarters and king_of_the_hill, five tools, eight
seeds, each side in turn, the measuring bot carrying each sub against a
grenade. "Before" is the old rule, reproduced by setting `sub_reserve` just
above `reserve_low`. Its seeds and roster are not the slice 6 probe's, so its
before figures differ slightly from the table above.

| Side's sub, against the grenade | Before | After | Paired change (SE) |
|---|---|---|---|
| Grenade | 0.500 | 0.500 | 0 (mirrored rounds) |
| Sprinkler | 0.493 | 0.496 | +0.003 (0.009) |
| Paint wall | 0.463 | 0.473 | +0.011 (0.009) |

Both planted subs stay level with the grenade within the noise, so the
half-tank cost stands. The bucket's wall gains most (0.433 to 0.473), and the
sniper's rounds are unchanged, since its throws are always within its reach. A
bot throws its grenade about 40% less often (1.53 a round against 2.56) and
plants the sprinkler and wall as often as before. Endless paint is untouched.

**Bots answer enemy planted subs (`REV#21`, done 2026-11-13).** Until now a
bot never shot an enemy sprinkler and fired straight into an enemy paint wall,
because `BotWorld` had no query for a planted sub and bot sight ignored the
wall. `BotWorld::planted_subs` now lists every standing planted sub, either
team's, by bounds, team and type (`LevelBotWorld` reads Level's admitted shots
through `Projectile::planted_sub`, which a sprinkler or wall answers only while
enemy shots still break it). Two rules use it, both in the shipping driver:

- *An enemy wall blocks sight.* `MatchBotDriver::clear_sight` is the world's
  line of sight, also stopped by an enemy paint wall whose bounds cross the
  line; a bot's own team's wall never blocks it, since only enemy shots are
  spent on a wall. Every sight test in `drive` uses it: an enemy behind the
  wall is not observed (the wall is cover, as it would be for a player),
  support paint behind it is not aimed at, and a territory sample behind it
  becomes a route destination where the stage has team gates, or is dropped.
  Bodies still walk through walls, so obstacles and supports are unchanged.
- *An enemy sprinkler in reach is a target.* A standing enemy sprinkler in
  sight is observed by its centre, and while no enemy is engaged the bot aims
  its tool at the nearest one within its own engagement distance and fires.
  The aim order is an engaged enemy, then that sprinkler, then support paint,
  then the paint goal.

Not taught, deliberately: a bot does not walk to a sprinkler out of its reach,
does not shoot at an enemy wall to break it, and does not pursue an enemy it
lost behind one. Each of these subs stands 4-8 s, so the bounded answer is
the one above; whether bots should do more is a play-feel call for later. No
random draw moved (the aim error is drawn every decision already, and territory
samples are drawn before their sight test), so a match with no enemy planted
sub in sight is step-identical to before. The balance tables above were
measured with the old policy and were not re-measured; the half-tank cost
stands. `sub_weapons` checks that the bots' world lists a sprinkler and a wall
exactly while each stands (not in flight, after breaking or past its lifetime)
and, through `MatchBotDriver` at every difficulty with the Sprayer and the
Sniper, that a bot aims past its own wall but never through an enemy's, shoots
an enemy sprinkler in reach and in sight but not its own team's, one beyond
reach or one behind an enemy wall, and fights an engaged enemy first. The
balance runner's `engaged_ticks` uses the same sight.

### Umbrella tool proposal (`ROAD#13`)

**Proposed 2026-11-12; questions answered 2026-11-13; nothing is built.** This
is the first step the 2026-11-12 audit set for `ROAD#13`: a scope proposal for
one new tool archetype, written in the shape [Sub weapons](#sub-weapons-road11)
used. No code, tuning or content changed. The author's answers are at the end;
whether to build slices 2-4 is a separate call, below them.

**The archetype: an umbrella (Splatoon's brella).** It fires a short fan of
paint pellets. While Shoot is held, a canopy opens in front of the player and
stops the enemy shots that reach it. It differs most from the five tools
because each of those is a way of placing paint: a stream (Sprayer), a jet
(Sniper), a contact volume at the feet (Roller), a mist (Mister) and a lob
(Bucket). None of them answers a shot, while the umbrella makes Shoot a defence
as well. A Sniper facing one has to break the canopy or go round it, and a
Roller can walk through it. Much of the mechanism is already qualified: the
paint wall's `takes_hits`/`take_hit` path, which since slice 1 stops a shot
along its swept path (`Projectile::resolve_movement_contacts`, given
`Level::resolve_projectile_movement`'s breakables), and the sprinkler's fan of
drops (`ProjectilePlanted::spray`).

**Considered instead.**
- *Brush* (the fast painter). The player runs faster while Shoot is held,
  leaves a thin floor trail and flicks drops on a tap. Of the five it fits the
  fiction best: the paint program's Brush is the one classic tool the drawings
  lack, and the Sprayer is its airbrush. But the brush's rule is a player
  speed, and three qualified things depend on speed. A faster take-off clears
  gaps the arenas were built around. The movers' relative-displacement budget
  (`LevelStructures::validate_budget`, `game/objects/level_structures.cpp:301`)
  counts the player's own speed. Bot navigation was qualified at today's speeds
  through `bot_team_trials`. Its trail also overlaps the Roller's. It is the
  second choice (Q1).
- *Dualies.* Their dodge roll is a new movement verb. It has the brush's speed
  ripple, plus a new player state and a dodge policy for bots.
- *Splatling.* It charges, then fires a stream. The stream is the Sprayer's and
  only the charge is new. Its case was sharing the hold-to-charge rule `REV#57`
  proposed for the Sniper; the author declined `REV#57` on 2026-11-13, so the
  splatling left Q1 with it.
- *Blaster.* Its shot bursts in the air near the target. The burst is the paint
  grenade's (`ProjectileGrenade`) and the lob is the Bucket's, so this
  archetype adds the least.

**Rule (proposed).** C++ says how the umbrella behaves and `tuning.json` says
how much.
- *Type and class.* `WeaponType::umbrella` sits between `bucket` and
  `random_primary` (the `static_assert`s and `weapon_names` in
  `game/objects/weapon_type.h` move with it). The class is `WeaponUmbrella
  final : RelativeVelocityWeapon`, beside `WeaponBucket`, so the player's run
  adds to the pellets as it does to the Bucket's ball. That is the game's usual
  subclass grammar, and every `Weapon` hook stays.
- *Fire.* Every `shoot_interval`, a paid shot releases `pellets` drops in a fan
  `spread` wide around the aim (the sprinkler's fan, aimed), on a new
  `ProjectileType::pellet` record with its own splash clip. One shot costs
  `ammo_usage`. `WeaponUmbrella` overrides `shoot`.
- *Canopy.* While Shoot is held the canopy is open. It is an upright
  `canopy.size` rectangle placed `canopy.reach` from the player's centre, on the
  side the aim points, and axis-aligned like the paint wall. It stops every
  enemy shot that reaches it, using the wall's rule, and lets its own team's
  shots through. Players of both teams walk through it. The `canopy.hits`th hit
  breaks it, and a broken canopy stays shut for `canopy.recover` seconds while
  the tool keeps firing. Blocking a shot costs no paint. Releasing Shoot shuts
  the canopy, and the canopy does not slow its holder (Q4).
- *Where Level asks.* A virtual `Weapon::canopy()`, empty by default, gives
  Level the canopy's rectangle and team, and `Weapon::take_canopy_hit()` spends
  a hit. The canopy joins the swept breakables that
  `Level::resolve_projectile_movement` hands each shot (slice 1), which then
  carry a rectangle, a team and a hit to spend rather than only planted
  projectiles; it does not go through `strike_breakables`, which is left to the
  roller's unswept volume. The canopy is never a Scene object, so nothing
  outlives the weapon that owns it.
- *Prerequisite: `REV#3`, satisfied 2026-11-13 by slice 1.* A canopy stands in
  front of its holder, which is the case `REV#3` recorded for a player standing
  inside the paint wall. Planted subs used to be struck by
  `strike_breakables` after `scene_->resolve()`, on travel bounding boxes, so a
  shot that reached the holder and the canopy in the same tick would have hit
  the holder and never counted against the canopy. Slice 1 sweeps breakables
  as participants in `Projectile::resolve_sweep` and its translating and slab
  forms, before player contacts, so the canopy has the order it needs.

**Numbers in `tuning.json`.** `weapons.umbrella` holds a tool's usual record
(offset, size, nozzle offset, `shoot_interval`, `starting_vel_length`,
`ammo_usage`, sheet and frame, projectile, sounds, `add_player_velocity`) plus
`pellets`, `spread` and a `canopy` record (`size`, `reach`, `hits`, `recover`).
It is joined by `projectiles.pellet` and `paint.splash.animations.pellet`. The
starting values are provisional. They borrow the Bucket's 0.5 s interval and
0.1 tank per shot, and the wall's 15 hits.

**Art.** `weapons.json` gains the tool's four poses (`umbrella`,
`umbrella_motion_1`, `umbrella_motion_2`, `umbrella_lift`), each with its paint
layer ([weapons README](../game/art/weapons/README.md)). The painter gains its
grip, and Weapon Select gains its choice frames. The `paint_fx` sheet gains a
canopy strip (open, cracked by hits left, and shut) and `shot_pellet` and
`impact_pellet`, drawn in the sub strips' keylined greyscale and tinted by
team. Weapon Select lays out six tools and Random where it laid out five
(`main_menu.weapon_select`); the painter's `-WeaponCatalogOnly` and
`-WeaponPaperOnly` leaves check that layout.

**Audio.** There are four new cues: `umbrella.shot`, `umbrella.open`,
`umbrella.block` (a shot stopped by the canopy) and `umbrella.break`. They are
cut from recordings the `paint_exe` bank already retains, as sub slice 7's
were, and recorded with an acceptance amendment. The empty, recovered and low
cues stay the tools' shared ones.

**Bot ballistics.** Bots aim with `match_bot_ballistics::aim_direction` on the
pellet record and judge reach with `horizontal_reach`, as for every tool. The
canopy needs one new `MatchBot` rule: hold Shoot facing a visible enemy whose
tool reaches the bot, even when the enemy is beyond the bot's own reach (the
wall's slice-6 test), and release it while the canopy is broken. The balance
pass repeats the slice-6 bot-round method (1,440 one-minute rounds) with the
umbrella against each tool, against the five-tool balance accepted on
2026-11-05 (`POL#27`).

**Records and formats.**
- *Save.* The name joins the save's `last_match`. An older build reads an
  unknown weapon as the Sprayer (`game/save/last_match_setup.cpp:135-136`), so
  the save stays at version 9.
- *Replays.* The Debug match plan and replays carry the name. The match
  fingerprint gains each seat's canopy state (open, hits left, recovery time),
  which moves the Debug replay format from 11 to 12.
- *Random.* Weapon Select's Random draws `rand() % max_prim_wep`
  (`game/states/main_menu_weapon_select.cpp:135`), so a sixth tool changes every
  seeded Random tool pick. The painter pins that count Random draws move with
  it.
- *Presentation and copy.* The presentation arrays sized by
  `tuned_weapon_count` (grips, muzzle sizes, motion styles, idle drips), the
  choices and the descriptions each gain an entry. The copy lives in
  `presentation.json` and its locale overlay, and the README and
  `HOW-TO-PLAY.txt` name the tool.

**Slices.** Each slice is a pull request with its own acceptance case.
1. **Breakables before player contacts** (`REV#3`'s fix), *done 2026-11-13*.
   Breakables are swept before player contacts, and `sub_weapons` gains a
   diagonal near-miss check and a check with a player inside the wall. This
   closes `REV#3`.
2. **Umbrella end to end.** The `WeaponType`, the class, the tuning, the pellet
   projectile and the canopy among the swept breakables, with placeholder art
   (the Bucket's frames and the wall's strip) and existing waves. It is
   selectable on Weapon Select and through Random, and carried by the save, the
   match plan, replay format 12 and the copy. A new `umbrella` case checks the
   tuning and its refusals, the fan, the cost, the canopy opening, shutting,
   breaking and recovering, a shot from the front stopped while one from behind
   is not, the holder's team passing, and a real round.
3. **Bots carry it.** Aim with the pellet record, and the canopy rule under each
   difficulty and refill switch, checked by `bot_driver` and by
   `bot_team_trials` with the umbrella in its rotations, then the balance pass.
4. **Art and audio.** The final frames and strips, and the four masters with
   provenance, checked by the painter leaves.

Then comes the author's attended play review, which implementation does not
close.

**Slice 1, done 2026-11-13 (`REV#3`).** A planted sub (a projectile that
`takes_hits`) is now a participant in the shot's sweep rather than a box test
after it. `Level::resolve_projectile_movement` snapshots the admitted
projectiles that take hits, in scene order, into `Level::breakables_` before
its walk, and hands that list to every `Projectile::resolve_movement_contacts`.
`Projectile::resolve_sweep`, its slab form (`resolve_fixed_slab_sweep`) and its
moving-structure form (`resolve_translating_contacts`) each find the earliest
enemy breakable the shot's path enters (`sweep_interval` against its bounds,
the earlier in the list on a tie) and compare it with the obstruction they
chose: an earlier breakable wins, cover keeps a tied entry, and a breakable
takes a tie from a player. A shot that loses to a breakable stops just inside
the face it entered, with the same inset cover gives it (so a struck grenade
bursts at the face, not past it), retires, and hands the sub one hit; it
paints, damages and cues nothing, as before. `takes_hits` is asked again as
the sweep reaches each breakable, so one broken earlier in the walk stops
nothing more; one planted during the walk takes hits from the next tick, in
any scene order. An empty list skips every new comparison, so a match without
a planted sub runs exactly the arithmetic it did. `Level::strike_breakables`
remains only for the roller, whose one-step volume is not swept, so no swept
shot can be counted twice. The launch sweep (`resolve_launch_obstruction`)
takes no breakables, deliberately: a muzzle that spans an enemy wall is still
caught by the next tick's sweep, as before. `sub_weapons` gains a Sniper jet
rising past the wall's top corner, whose travel box overlaps it but whose path
does not (the wall keeps its hits), the mirrored corner shot that does reach
the face (one hit), a host standing ten units inside the wall's face, shielded
by it, and a host just in front of the face, hit first. No seeded pin moved.

**The author's answers (2026-11-13).**
- **Q1, the archetype: the umbrella.** The brush was not taken. The splatling
  left the list when `REV#57`'s Sniper charge was declined.
- **Q2, `REV#3`.** Moot: `ROAD#11`'s play review was waived, so `REV#3` was
  open work and slice 1, whichever way the build call goes. It was done on
  2026-11-13.
- **Q3, the canopy input: open while Shoot is held** (Splatoon's rule).
- **Q4, speed: no slowdown.** An open canopy does not slow its holder, so no
  tool-dependent speed arrives with it.
- **Q5, the canopy's shape: upright on the aim side.**
- **Q6, the name: "Umbrella"** as the working name until the art pass; the copy
  lives in `presentation.json`.
- **Q7, two shields: yes.** An umbrella may carry the paint wall sub; the
  loadout stays free, as decided for `ROAD#11`.

**Still open: whether to build slices 2-4.** That call is separate and came
after `PIN#2` ([engine ledger](review/engine-reports.md#qualify-masters-behaviour-changes-pin2)),
which finished 2026-11-13, so it is the author's to make now.
Building reopens the five-tool balance accepted on 2026-11-05 (`POL#27`) and
adds four cues nobody will hear before they ship, which are the reasons
`REV#50` and `REV#43` were declined
([review calls](#project-review-calls-2026-11-13)). The row stays DECISION,
narrowed to that call.

### Input

- **Non-XInput controllers (`ROAD#23`).** INVERSUS and most Steam releases
  accept PlayStation and Switch Pro pads. The reader is Labrador's (see
  [Input devices and seats](#input-devices-and-seats)), so the engine part is a
  report with an exhibit; Steam Input covers it if the game ships on Steam.
  **Deferred (2026-11-12).** It was examined, so it is not unscoped: the
  engine ask is parked in `ENG#PARKED` as the non-XInput reader, and this
  client has no exhibit, because no such pad has been tried here and the
  `ROAD#2` session that would have observed the limit was waived. It resumes
  when a non-XInput pad is in hand to show the failure, or when a store
  release is chosen (`REV#68`), which decides whether Steam Input answers it.
- **Pad aim assist (`ROAD#24`).** One mouse player can join pad players, and
  mouse aim is much more precise. Relates to `POL#3`, `POL#18` and the
  `ROAD#5` handicaps. Story B8 holds it: raise and scope it if a couch session
  shows pad seats consistently losing to the mouse seat. The `ROAD#2` and
  `POL#24` sessions that were to look for this were waived by the author on
  2026-11-05 and not performed
  ([decision](design-process.md#open-points-for-the-author)).
  **Deferred (2026-11-12)** until attended evidence: story B8 already makes it
  conditional, so it waits for a couch session that shows pad seats
  consistently losing to the mouse seat. Without that, there is nothing to
  tune an assist against.

### Not tracked, by choice

The author left out the gaps rated low or low-to-medium value: respawn travel
to a teammate (Splatoon Super Jump), automatic catch-up for the team behind
(TowerFall autobalance), a last-minute music change, quick signals ("This
way!"), a lifetime stats screen, and cosmetics or unlocks. New modes
(Splatoon's objective modes, Tricolor and Salmon Run, free-for-all painting,
TowerFall's elimination and co-op modes, INVERSUS arcade, Smash stock
battles) stay excluded by the [match scope decisions](#match-scope-decisions-2026-10-15).
A later gap list should read these absences as intended, together with the
2026-11-10 review's rows the author declined on 2026-11-13
([Project review calls](#project-review-calls-2026-11-13)).

A live overview map (`ROAD#17`) was declined by the author on 2026-11-03.
ColourWars withholds the whole picture along with the percentage, because the
backstory [reveal](backstory.md#the-reveal-full-screen) is the first time
anyone sees the drawing whole; a map showing it for four minutes would spend
that beat. Only
three-human matches have a spare quadrant, so what players could see would
depend on how many pads are plugged in. The value was rated low, and
[teammate markers](#teammate-markers-road18) answer "where is my team". The
earlier draft also argued that a territory map is not Splatoon's; that was
wrong, since Splatoon's map shows both teams' ink, and the decline does not
rest on it. Reopen if the reveal no longer opens on the whole picture, if the
author decides its payoff is the choice and the percentage rather than the
picture, if attended 3P play shows players lost about where to paint despite
teammate markers, or if threat markers (declined under `ROAD#18` on 2026-11-05)
are reopened. A territory-free use
of the 3P quadrant (clock, team colours, the person's desk) would be its own
row.


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
engine-free `game/contracts/horizontal_motion_contract.h`, which the game's loader and
the editor both call, so the editor refuses at load and on every edit exactly
the drives the game would refuse, in the game's words. See
[Moving-structure authoring](level-editor.md#moving-structure-authoring-gapd5).
The palette has no ferry entry of its own; a ferry is a moving paintable whose
drive is switched to paint.

The stage-select painter checks (`check-painter.ps1 -StagePaperOnly`,
`-StageCountsOnly`, `-StageSummaryOnly`) passed with the tenth stage on MattPC
in Debug and Release (2026-10-28). The second object and its bot-route
qualification are under [Team gate](#team-gate-road15-second-slice).

### Team gate (`ROAD#15`, second slice)

**Which object next (2026-10-28).** Three candidates were left: vertical
lifts, team-gated passages (Splatoon's inkrails, which open for the team that
inks their bulb) and growing sponges. They were weighed against the movement
and collision code as it is.

- **Lifts** need a second motion axis. The mover, its admission envelope, its
  displacement budget, the rider carry (`HorizontalCarryStep` is an x
  displacement), the relative projectile sweeps and the translated shadow are
  all horizontal by construction. Carrying a rider up means resolving the
  rider against ceilings and refusing or resolving a crush, which nothing in
  `Player` does now. It is the largest of the three, and most of it is new
  physics rather than a paint rule.
- **Sponges** change their own size. A paintable's tiles, strips, credit and
  scored area are fixed at construction, and `PaintSurfaceSample` says outright
  that a scale needs its own scoring rule. Growing also pushes players, which
  is the crush problem again. It is about as large as a lift, and it reopens
  scoring.
- **A team gate** is static. Collision filtering already takes both objects'
  layer and mask on every pair, every step, so a gate whose mask leaves out one
  team's players is a mask override, and projectiles still hit it. Nothing
  moves, so admission, carry, sweeps and replay need nothing new. The one new
  hazard is a gate closing on a player inside it, and that is answered by not
  closing while they are there.

The gate is the most tractable honest slice, so it went first. Splatoon's rail
is a route that appears; this is its ColourWars reading, a wall one team walks
through, held by the paint on the wall itself rather than a separate bulb.

**What it does (2026-10-28).** A `TeamGateStructurePaintable`, a new final
subclass of `StructurePaintable`, is a solid paintable wall. The team holding
more of its own paint, by area over all its faces, walks through it, once that
team also holds `paint.gate.open_share` (0.25, `tuning.json`) of its paintable
area. Everyone else collides with it like any wall, and it is shut to both on
a tie or while unpainted. Shots of both teams always hit it, so it is painted
from either side whichever way it is open. `Level` settles every gate once a
step, after the step's movers are committed and before any movement or paint,
so a gate follows the paint held before the step, as the ferry does. Settling
changes the team only when no living player of the team it is open for
overlaps it: a gate never closes on a player inside it, and changes hands the
step after they are clear. Players who are dead are ignored, and no spawn body
may overlap a gate, so a respawn cannot land inside one.

It ships on one explicit-only stage, **Turbulence Yard - Gate Trial**
(`turbulence_gate_trial`, not in Random): the Turbulence blockout with an
eraser gate, 80 by 240, standing on the floor at its centre, between the two
lower beds. It is taller than a jump, so the floor route across the middle is
the gate's, while the upper crossover stays open to both. Its left, top and
right faces paint; each team reaches its own side face from the floor, so the
top decides an even exchange. The format is in
[Team gates](level-format.md#team-gates); the editor authors one from the
palette ([Team gate authoring](level-editor.md#team-gate-authoring-road15)).

**Bots in the initial gate slice** treated every gate as a wall for planning,
even when collision permitted passage. A gate's faces are scoring tiles, so
bots painted them as territory and could walk through incidentally. Deliberate
team-aware passage is now implemented and qualified in the
[route slice](#team-aware-bot-gate-routes-road15) below.

The `team_gate` acceptance case checks the rule in the contract and on a
standalone gate; walks two scripted humans at the trial's gate (shut to both
unpainted, open to team A only once team A paints it, held open for a team A
player standing inside while team B retakes it, then open to team B only); and
plays three whole 120-second matches with three bots, holding every step of
the gate to the paint and positions before it, requiring that nobody is ever
left inside a gate shut to their team and that no bot stays inside one body
box for 15 seconds. In Debug on MattPC the gate was open for 6311, 6673 and
6879 of the 7200 steps, changed hands one to two times a match, and bots went
into it 13 to 17 times; no bot held still for more than 271 steps. Passing is
a measurement, not a requirement. `instant_replay` replays a round on the Gate
Trial bit for bit, and `level_format` and the editor checks cover the record.
Release measured the same three matches. With the eleventh stage, the
stage-select painter checks (`-StagePaperOnly`, `-StageCountsOnly`,
`-StageSummaryOnly`) passed on MattPC in Debug and Release (2026-10-28).

**Defaults, accepted as built (the author, 2026-11-05, `ROAD#30`):** the 0.25 open share; the paint
on the wall deciding it, rather than a separate bulb; a tie shutting it to
both; a gate held open while its team is inside, rather than pushing them out;
the eraser material; and the trial's single central floor gate.

**Remaining work, split into independent scopes (2026-10-28):** the functional
gate and ferry cues (`ROAD#26`, `ROAD#27`) are implemented below. Vertical lifts (`ROAD#28`)
and growing sponges (`ROAD#29`) need their own contracts. Attended trial play
is `ROAD#30`. Deliberate gate routing (`ROAD#15`) is complete below; each
remaining scope keeps its own row.

### Team-aware bot gate routes (`ROAD#15`)

**Implemented (2026-10-28).** `Level::bot_obstacles(team)` and
`bot_supports(team)` omit only gates whose settled `open_for()` permits that
team through. Their top is no longer a standing support for that team;
closed and hostile gates retain both roles. Filtering preserves every other
support's identity. The shared traversal observations feed standing goals,
approaches, ascents, descents and local recovery.

Sampled non-team paint beyond a permitted gate can supply a navigation-only
goal. It cannot supply an occluded paint aim or reveal an enemy. Enemy sight,
support-paint qualification and ordinary paint targets still use projectile
visibility; `bot_line_of_sight` and actual shots retain every gate as cover.
The policy separately checks body clearance on candidate routes.

The shipping driver watches settled permissions and invalidates affected
teams' held routes, ascent/descent/ramp/finish state and recovery decisions
before resampling. It retains reserve policy and the private random sequence.
A paint-leader change alone does not invalidate a route while an occupant
holds its old permission. Input observation still precedes gate settlement:
the first observation after settlement sees the new permission. Collision,
no-crush settlement, tuning and authored stages are unchanged.

**Qualification (Debug and Release, 2026-10-28).** The default
`bot_gate_routes` acceptance case checks six mirrored policy/difficulty
controls whose fallback initially heads away from the route, closed-route
rejection, separate navigation/paint aim and RNG-preserving invalidation.
Actual Level observations cover closed and either-team permissions, stable
support IDs, occupied takeover and the input-before-settlement boundary.
Four paired shipping-driver experiments cover direct and occupied takeovers
from both teams, retaining decisions before settlement and replanning on the
first observation afterwards, before ordinary reaction/replan expiry.
Actual projectile launch sweeps stop both teams' shots at either open gate.

Two isolated public-loader fixtures contain a flush scored floor patch beyond
a tall gate and ordinary non-scoring floor elsewhere. Each permitted team
reaches and paints the patch; each 420-step final-input tape reproduces the
sampled player, projectile-count and exact paint state at every step.
These capability fixtures do not alter a shipped arena.

Both game builds, the new case and twelve retained acceptance cases pass:
`bot_driver`, `bot_runtime`, `bot_recovery`, `bot_ballistics`, `bot_navigation`,
`bot_territory`, `bot_mover_routes`, `bot_arena_routes`, `bot_hill_routes`,
`bot_turbulence_routes`, `team_gate` and `instant_replay`. Hidden startup with
the accepted audio bank also passes in both presets. Receipts are under
`out/validation/road15-gate-routes/`; the new case is in `focused/<preset>/`.
This is scoped software qualification, not the full acceptance suite or
attended difficulty, route-value or physical-pad acceptance. Those human
gates remain `ROAD#10`, `ROAD#30` and `ROAD#2`.

### Gate permission cue (`ROAD#26`)

**Implemented (2026-10-28).** A steady paper plaque shows a closed X or the
permitted team's A/B glyph and passage mark, accented with that team's selected
palette colour. Two pause bars beside the glyph identify occupied hold. The shape carries the
same information without hue or sound, including with Reduce effects. Match
construction and Stage Select's live miniature both supply the cue.

`held_open()` records the actual reason at settlement, alongside `open_for()`:
only a living permitted occupant retaining a different paint leader sets it.
A later shot cannot invent or clear that reason before the next settlement.
The plaque reads these settled facts, rather than recomputing permission from
current paint. Its size, keyline, inset and contrasting ink/paper are in
`presentation.json`'s `gate_permission`; it scales down inside the body's
paint-free interior, leaving face bands and existing cull bounds intact. The
permission/hold marks sit toward the top so the occupant and its badge do not
hide the important state at the bottom of the gate.

**Software qualification (Debug and Release, 2026-10-28).** Both game builds,
`team_gate`, `menu_presentation`, `bot_gate_routes`, `instant_replay` and
`terrain` pass, as does hidden startup with the accepted audio bank. Independent
checks cover either team's occupied takeover, late paint, restored lead, ties,
departure, dead/hostile occupants and final closure. Small/no-interior bodies,
cull containment and exact paint-surface/credit/permission preservation are
checked separately. Presentation fixtures reject missing, malformed, nonfinite,
invisible and low-contrast values. Receipts are under
`out/validation/road26-gate-cue/`.

The focused native check, `scripts/check-painter.ps1 -GatePermissionOnly`,
passes in both presets on D3D11 / RTX 5080.
It compares seven controlled states in eight Levels built by LevelBuilder, all six
palettes, native 1/2/4-player scales and the actual Stage Select miniature fit.
Qualified base draws isolate the plaque's changed pixels; frozen/reduced draws
retain paint, credit, permission, RNG and Debug replay fingerprints. Exact ink
masks distinguish closed/A/B/held shapes independently of hue, and the actual
StagePreview supplies a nonempty closed-sign witness. It retains 22 context
frames, 155 unscaled crops and 112 state rows per preset. These are controlled
inspection states; attended clarity and preference remain `ROAD#30`.

### Ferry direction and stop cue (`ROAD#27`)

**Done (2026-10-28).** Paint-driven movers carry a small paper/ink plaque
below their body. Fixed A/B letters and opposed arrows show each team's
authored direction even when unpainted; the current paint leader is inverted.
A separate lower shape describes the last positive committed step: initial
dots before any step, pause bars for a tie/unpainted hold, a moving chevron,
or a chevron against the reached end-stop bar. Cyclic movers have no plaque.
The optional `ferry_direction` presentation record owns its size, gap, keyline
and paper/ink colours; existing files without it retain valid defaults.

`cue_state()` keeps current paint demand separate from committed heading,
actual world movement and end-stop state. New paint cannot rewrite a previous
step's explanation, and zero updates retain that explanation. A local offset
advance that rounds to no world movement does not claim movement. The cue
translates/culls with its mover and draws in both gameplay and StagePreview.
No motion rate, displacement, collision, scoring area, paint credit or RNG
policy changed.

**Qualification (Debug and Release, 2026-10-28).** `paint_ferry` checks both
authored team-A headings, current versus pre-step ownership, repaint reversal,
ties at either endpoint, zero updates, world-coordinate rounding, contained
culling and bit-exact uncued motion/paint/credit controls. Existing
`moving_structure`, `instant_replay` and presentation checks pass in both
presets, as does hidden startup with the accepted audio bank.

The focused native `FerryDirectionOnly` matrix passes in each preset: six
Level builds, 3,138 updates, 424 readbacks, 43 full frames, 169 unscaled crops
and 84 state rows. It covers native 1/2/4-player and miniature scales, both
headings and seven controlled states, reduced-effects parity, monochrome
shape differences, bounded draw output and the actual StagePreview widget.
Contact sheets and representative full compositions were inspected. These
are controlled inspection states; attended clarity/preference remains
`ROAD#30`, and decorative production art remains a later pass.

### Unscoped paint-object contracts (`ROAD#28`, `ROAD#29`)

**UNSCOPED (2026-10-28); `ROAD#28`'s first slice was promoted on 2026-11-03,
below, and its paint-driven second slice built the same day. Regraded
2026-11-12: `ROAD#28`'s remainder is deferred, and `ROAD#29`'s contract draft
is recorded, both below. `ROAD#29` was parked behind `REV#27` on 2026-11-13.** These are the remaining capabilities from
`ROAD#15`, not ready implementations or prerequisites of the three slices
above:

- **Vertical lifts (`ROAD#28`, L left):** define vertical admission and swept
  displacement, rider support identity and carry, ceilings/crush refusal,
  projectiles, shadows, editor envelopes and replay before choosing a trial.
  The horizontal carry contract did not answer these; the
  [draft](#vertical-lift-contract-draft-road28) below does, and the author
  accepted it with two amendments on 2026-11-03. The
  [cycling](#vertical-lift-first-slice-road28) and
  [paint-driven](#paint-driven-lift-second-slice-road28) slices are built;
  deliberate bot lift routing, including a stuck check that discounts carried
  y, is left. **Deferred, LOW, behind `REV#27` (2026-11-12).** Lifts stand
  only on the Lift and Paint Lift Trials, and both have
  `"include_in_random": false` (`game/content/levels/turbulence_lift_trial.json`,
  `turbulence_paint_lift_trial.json`), so no match a bot plays from Random
  meets a lift and routing has no consumer. It resumes if `REV#27` sets an
  arena with lifts as an aim.
- **Growing sponges (`ROAD#29`, XL):** define occupied growth/shrink behavior,
  paint-tile identity, changing scored area and retained paint/contribution
  credit before choosing an implementation. `PaintSurfaceSample` explicitly
  requires a scoring rule for scale; resizing a rendered rectangle is not one.
  **Draft recorded (2026-11-12):** the
  [growing-sponge contract draft](#growing-sponge-contract-draft-road29)
  resolves the fixed-area sites against `220d4809`, answers each one
  (E1-E12), and leaves the gameplay sets G1-G7 to the author. It changed no
  code, tuning or content and chose no trial. **Deferred behind `REV#27`
  (2026-11-13):** the author picked none of G1-G7; they are picked from the
  draft's recommendations when `REV#27` sets a Random arena that wants a
  sponge.

Promote a bounded slice only once its owner records those choices and its
acceptance. Neither adds a new physics/scoring requirement to an accepted
arena while `POL#21` remains a geometry decision.

#### Vertical-lift contract draft (`ROAD#28`)

**Draft accepted with two amendments (the author's 2026-11-03 decision); its
first slice is [built](#vertical-lift-first-slice-road28).** The draft was
recorded on 2026-10-30 as a written step, not code; it is below. The author accepted E1, E2, E4 and E6 as written, amended E3 and
E5, and picked one option from each gameplay set; the calls and the promoted
slice are in section 4. Acceptance of the draft step: every inventory row below resolved against the
tree at the commit that records it, each engineering answer names its
precedent, and no code, tuning or content changed. The draft chooses no trial
and does not touch Turbulence's geometry (`POL#21`).

**1. Inventory of horizontal-only sites.** Re-resolved on 2026-11-03 against
`82b8bdb6`, the tree before the first lift slice; line numbers are that
tree's, and the slice has since moved most of them. "Assumes" is what the site
took for granted; "A lift needs" is the change a vertical mover forced there.
Rows marked *(new)* were missed by the 2026-10-30 draft and found by the
re-resolution. Every row still held; only line numbers had moved, mostly by
`HYG#40` (the opening out of `Level`) and `HYG#51`.

| Area | Site | Assumes | A lift needs |
|---|---|---|---|
| Capability note | `game/objects/translating_structure_paintable.h:23-24` | "not a lift or a rotating body" | the note rewritten for the accepted axis |
| Engine-free spec | `game/contracts/horizontal_motion_contract.h:32-40` (`Spec::travel_x`) | one travel along +x | a vertical travel (see answer E1) |
| Spec validation | `game/contracts/horizontal_motion_contract.h:91-110` | the endpoint is `anchor.x + travel_x` and must exceed `anchor.x` | the same distinct-endpoint check on y |
| Offset scalar *(new)* | `game/contracts/horizontal_motion_contract.h:112-116` (`offset_at`), `game/objects/horizontal_motion.cpp:12-16` (`contract_spec`) | the triangular offset scales `travel_x` | the axis's travel |
| Paint-drive heading | `game/contracts/horizontal_motion_contract.h:26-75` (`team_a_heading`, `decode_drive`, `validate_drive`) | headings are `right`/`left`; phase 0 is the left end | `up`/`down` naming, if a lift is paint-driven (option G3) |
| Pose | `game/contracts/horizontal_motion_contract.h:159-165`, `game/objects/horizontal_motion.cpp:59-64` | an offset adds to x only | an offset adds to y for a lift |
| Path | `game/contracts/horizontal_motion_contract.h:167-174` | the path keeps the anchor's top and bottom and widens left/right | widen top/bottom instead |
| Standing corridor | `game/contracts/horizontal_motion_contract.h:176-184` | one body height above a fixed `path.top`; its pad scale reads only `path.top` | the corridor rises with the highest pose (answer E3) |
| Motion state | `game/objects/horizontal_motion.h:22-60` | `HorizontalMotionSpec::travel_x`; segments carry `from_x`/`to_x` | segment offsets on the lift's axis; `Vector2F` offsets already hold a y that is always 0 |
| Offset construction *(new)* | `game/objects/horizontal_motion.cpp:45`, `:66-69`, `:109`, `:131`, `:155` | the constructor, `bounds()` and both prepare paths build `{ x, 0 }` or read `.x` | the offset on the lift's axis |
| Segment building | `game/objects/horizontal_motion.cpp:97-103`, `:142-149` | reads `current_offset.x` | read the lift's axis |
| Carry step | `game/objects/horizontal_motion.h:71-77` (`HorizontalCarryStep::displacement_x`) | x displacement only | a y displacement (answer E2) |
| Admission | `game/objects/level_structures.cpp:47-61`, `:230-270` | pose/path are horizontal; standing envelope and obstacle refusals use that path; nothing is checked below the path | the lift's path and corridor, same refusals, and the new clearance below the lowest pose (E3a as amended) |
| Displacement budget | `game/objects/level_structures.cpp:272-340`, refusals at `:334-338` | mover travel enters only the horizontal bound; the vertical bound counts the player's own y speed | the lift's per-step travel added to the vertical bound (E6) |
| Frame preparation | `game/objects/level_structures.cpp:354-366` (displacement `:362-363`), zero step `:378-396` *(new)* | carrier displacement is `after.x - before.x`; the zero step carries x 0 | `after.y - before.y` too, and a zero y |
| Persisted pose | `game/objects/level_structures.cpp:398-414` | poses rebuilt from `previous_offset.x`/`current_offset.x` | rebuild from the lift's axis |
| Rider carry | `game/objects/player.h:182-184`, `:380-387`, `game/objects/player.cpp:571-590` (call `:1336-1338`) | offsets the body by `displacement_x` and records `movement_carried_x_` | a y offset applied before the rider's own integration (answer E2) |
| Carrier choice | `game/objects/player.cpp:592-641` | a support is a measured or resolved top contact under the feet | unchanged rule; the top moves each step |
| Jump-through landing *(new)* | `game/objects/player.cpp:942-963` | a landing needs the previous feet above the platform's current top | a rising top can pass the feet, so a lift is solid only |
| Movement feedback | `game/objects/player.cpp:496-500`, `:1045-1048`, `game/objects/player.h:326-328`, `:385` | subtracts carried x from resolved x; landing reads the rider's own `velocity().y` | nothing: a carried y never enters velocity or the footstep distance |
| Level step order | `game/objects/level.cpp:139-149`, `:380-384`, `:404-409` | prepare before input, commit after it, supports after contacts | unchanged order |
| Flying-paint sweep | `game/objects/projectile_contacts.cpp:114-124` (`relative_rectangle_at`) | subtracts the mover offset from left/right only | subtract it from top/bottom |
| Swept response | `game/objects/projectile_contacts.cpp:251-331`, segments `:270-289`, response `:315-323` | segments are x offsets; response offsets by the full translation | segment offsets on the lift's axis |
| Roller footprint | `game/objects/projectile_contacts.cpp:333-372`, union `:355-364` | the visited footprint grows left/right, keeping `entered.top`/`bottom` | grow top/bottom |
| Mover world queries | `game/objects/translating_structure_paintable.cpp:248-249`, `:266-267`, `:272-273`, `:278-286`, `:300-306` | bounds, splash, runoff and paint samples shift by `translation().x`; `top_paint_team` queries by x | shift by y as well; `top_paint_team` is already right for a lift (draws at `:224`, `:230` and the cue at `:111` already use the full vector) |
| Ferry cue | `game/objects/translating_structure_paintable.cpp:195-208`, glyphs `:153-158`, `:182-187` *(new)* | heading is the sign of `after.x - before.x`; arrows and end stops are left/right | a lift's cue, if any, is its own presentation (option G3) |
| Translated shadow | `game/objects/translated_airbrush_shadow.cpp:52-65` | bounds/cull shift by `offset.x` only; anchored at the anchor's bottom | shift by y; the shadow rides with the lift |
| Shadow admission | `game/objects/level_object_builder.cpp:649-674` (rule `:663-664`) | casts for wide, thin movers | unchanged rule |
| Loader | `game/objects/level_object_builder.cpp:139-175` (refusals `:146-152`) | only `type: "horizontal"`; `axis`, `travel_y` and `rotation` are refused by name | a second type (answer E1) |
| Editor decode *(new)* | `editor/horizontal_authoring.cpp:65-91` | the editor's twin of the loader refusal | the same second type |
| Editor guide and checks | `editor/horizontal_authoring.cpp:108-156` | guide and refusals use the horizontal path/corridor | the shared contract's lift path and clearance |
| Editor duplicate | `editor/level_document.cpp:897-917` | a copy moves down past source body plus corridor | the copy clears the lift's whole vertical path and clearance |
| Editor mirror | `editor/level_document_mirror.cpp` (`mirror_object`) | reflects `travel_x` reach; refuses paint ferries | a lift reflects as its anchor only, without the half-cycle phase advance a horizontal path needs |
| Editor fields | `editor/level_document.cpp:615-631`, `:865-873` | `motion/type (horizontal)` and `motion/travel_x` labels and default | the lift's field and palette entry |
| Blockout aids | `editor/blockout_aids.cpp:177-184`, `:209` | initial-phase offset added to x | added to y |
| Bot supports | `game/objects/level_bot_world.cpp:111-130` (flag `:124-126`) | `moving` marks a support whose bounds change | unchanged flag |
| Bot riding | `game/ai/match_bot.cpp:81-113`, `:283-311`, `:449-459`; `game/ai/match_bot.h:118-138`; `game/ai/match_bot_ascent.cpp:174-187` | a ride is a feet-on-top match, and the bot paints/holds while carried sideways; `local_goal` keeps the y it chose at replan | the same match; deliberate lift routing is a later slice, as `ROAD#15` was for gates |
| Bot progress *(new)* | `game/ai/match_bot.cpp:425-444` | carried distance (including y) counts as progress | nothing for this slice; lift routing will need to discount it |
| Match fingerprint | `game/debug/replay/match_fingerprint.cpp:36-99` (spec words `:47-50`, domain tag `:76`, labels `:112`), `game/debug/replay/match_fingerprint.h:46-55` | seven spec words with `travel_x`; offsets/poses already hold y | the lift's travel in the spec words, and a new domain tag |
| Replay codec | `game/debug/replay/replay.cpp:146-317` (spec word 4 is `travel_x` at `:207`; y offset must be 0 at `:219-220`; pose x-only at `:222`; current offset from `travel_x` at `:233`; `schema` 1 at `:257`, `:284`) | horizontal-only motion block, schema 1 | schema 2 and a version bump (answer E5) |
| Replay version | `game/debug/replay/replay.h:21-22`, `:46`, `game/debug/replay/replay.cpp:682-686` | writes and reads 6 only (`HYG#39`) | writes 7 (answer E5) |
| Acceptance | `scripts/acceptance/moving_structure/`, `level_format.cpp:492-631`, `level_editor/motion_checks.cpp`, `debug_console/replay_checks.cpp`, `debug_console/replay_codec_checks.cpp`, `debug_console/bot_replay_checks.cpp` | horizontal fixtures; `type: "vertical"` and schema 2 asserted as refusals; version 6 | lift fixtures beside them; flipped refusals; version 7 |

**2. Engineering answers, each following existing practice.**

- **E1. One axis per mover, a second motion type.** A lift is
  `"motion": { "type": "vertical", "travel_y": …, "period": …, "phase": … }`
  on the same `TranslatingStructurePaintable`, with the triangular path,
  phase and drive rules unchanged; diagonal or two-axis motion stays refused.
  *Precedent:* the loader already reserves `travel_y` and `axis` by name
  (`level_object_builder.cpp:146-152` at `82b8bdb6`), and the paint ferry joined the same
  class as a new `drive` value rather than a subclass. The rectangle is the
  **topmost** anchor, as the horizontal one is the leftmost.
- **E2. Carry a rider by its support's identity, before its own motion.** The
  carrier step gains a y displacement; `apply_carrier` moves the body by both
  components after its velocity update and before its own integration, where
  the x carry sits now (`player.cpp:1336-1338` at `82b8bdb6`), under the same checks (alive,
  flat, overlapping, feet at the previous top). Jump, drop, walk-off, death and
  placement release it, and release adds no lift velocity to the rider.
  *Precedent:* the x carry (`player.cpp:571-590` at `82b8bdb6`), keyed by
  `HorizontalMotionId`, "without adding personal velocity or tool travel"
  ([capability](#horizontal-movement-capability-2026-10-26)).
- **E3. Refuse a crush; never push a player through geometry.** Two parts:
  (a) *static:* admission refuses a lift whose path or standing corridor —
  one body height above its *highest* pose — meets blocking geometry or
  leaves the scene, so a ceiling cannot close on a rider. The same check runs
  below the *lowest* pose: a descending underside must leave either a full
  body height plus the corridor pad above the floor beneath it, or meet that
  floor flush with no gap to stand in. (b) *dynamic:* a lift whose next step
  would close on a living player under it holds still for that step, and
  resumes as soon as they are clear. *Precedent:* (a) is the horizontal
  standing-envelope refusal (`level_structures.cpp:247-266`, mirrored in
  the editor at `horizontal_authoring.cpp:137-151`); (b) is the team gate,
  which stays open while a permitted player is inside rather than pushing them
  out (`team_gate_structure_paintable.cpp:65-84`,
  [Team gate](#team-gate-road15-second-slice)), and is settled from the state
  before the step, as the gate and the paint ferry's heading are.
  (Amended 2026-11-03: the first slice refuses the flush case. Every lift's
  lowest pose must leave a full body height plus the corridor pad above the
  floor beneath it, so (a) alone rules out a crush and (b)'s hold stays in the
  code as a safety check that should never fire, not a gameplay rule. A
  flush lift would let a player stand in the open space and stall it; nobody
  has played that. The gate's settle, now `:70-90`, holds a permission while
  nothing moves; the closer precedent for a mover holding its phase is the
  paint ferry stopping on a tie or at an end stop, `horizontal_motion.cpp:140`.)
- **E4. Sweeps and world queries generalise, they do not fork.** Flying paint,
  Roller footprints, splash, runoff, `top_paint_team`, shadows and culling take
  the full offset vector. Paint, credit and scored area stay on local anchor
  faces. *Precedent:* the horizontal increment kept paint local and moved only
  world queries; draws and the ferry cue already use the full vector
  (`translating_structure_paintable.cpp:111`, `:224`, `:230`).
- **E5. Version the record.** The motion block's `schema` goes 1 → 2 with the
  axis and travel, and `ReplayFile::format_version` 6 → 7. The reader refuses
  6, as the 4 → 5 bump (`HYG#35`) refused 3 and 4: a replay holds only on the
  build that recorded it. *Precedent:* that bump, and the motion block already
  carries its own schema number. (Amended 2026-11-03: the draft said 5 → 6,
  but `HYG#39` had already moved the format to 6 the day after it. Only Debug
  `record`/`play` files are affected; the shipping instant-replay tape is held
  in memory and never versioned.)
- **E6. Budget the relative motion.** The lift's per-step travel joins the
  vertical relative-displacement bound beside the player's own speed, refused
  at admission like the horizontal one. *Precedent:*
  `level_structures.cpp:334-338`.

**3. Gameplay calls, as options for the author.** Values follow the current
movers (320 travel, 4 s round trip, about 160 units/s) only as a reference
point; values stay provisional, and none of these gates the engineering.

- **G1. Travel.** (a) Less than a jump's height, a lift that only saves a
  jump; (b) one storey, reaching a ledge a jump cannot; (c) a full arena
  height, a route in itself.
- **G2. Speed.** (a) The current movers' 160 units/s; (b) slower, 80-100
  units/s, so a ride is a commitment under fire; (c) faster, a quick escape.
- **G3. What drives it.** (a) A cycle, always running; (b) paint, like the
  ferry, with team A's paint carrying it up or down (headings `up`/`down`, and
  a cue of its own); (c) a rider — it rises only while occupied and returns
  empty. (c) has no precedent here and would need its own contract.
- **G4. Whether its surface scores.** (a) Scored, like the ferry; (b) material
  only (cover/route, no score); (c) scored top, unscored sides.
- **G5. Which trial carries it.** (a) A new explicit-only Lift Trial beside the
  Ferry and Gate Trials; (b) an explicit variant of an existing trial. Neither
  joins Random nor touches Turbulence's selected geometry (`POL#21`).

**4. The author's calls (2026-11-03).** Accepted all recommendations:

- **Engineering:** E1, E2, E4 and E6 as written; E3 and E5 as amended above.
  E6 has a wide margin: about 2.7 units per step at 160 units/s against a
  vertical bound of about 140 (120 body plus a 20-thick mover), which shrinks
  if a lift is drawn much thinner than 20.
- **G1, travel: (b) one storey**, starting at about 320 units (provisional),
  well above a held jump's rise of about 196, so the lift reaches a ledge a
  jump cannot. If the trial stage cannot fit a 320-unit shaft with E3(a)'s
  clearance, use the largest travel above about 200 that fits; attended play
  of Press Tower (`POL#31`) or Turbulence (`POL#21`) showing vertical routes
  are the real problem would make (c) worth its cost.
- **G2, speed: (a) 160 units/s**, the qualified movers' leg speed, so the axis
  is the slice's only new variable. Start at about 100 instead if `ROAD#30`'s
  Ferry Trial play shows riding at 160 is a free, risk-free escape.
- **G3, drive: (a) a cycle** for this slice, as the Mover Trial came before
  the Ferry Trial. **(b) paint drive is the next lift slice**: `up`/`down`
  headings through the shared `decode_drive` and a cue of its own, modelled on
  `ROAD#27`. Pick (b) now instead if the lift's first attended play is meant to
  judge its paint value rather than its physics. (c) stays unscoped.
- **G4, scoring: (a) scored, like the ferry**, which is
  `TranslatingStructurePaintable`'s existing behaviour under E4. Choose (b), or
  (c) if the sides are farmed, if `ROAD#30` shows scored moving surfaces swing
  the score more than their area justifies.
- **G5, trial: (a) a new explicit-only Lift Trial** derived from Turbulence
  Yard like its siblings (for example `turbulence_lift_trial`), so the Ferry
  and Gate Trials stay unchanged for `ROAD#30`. It joins neither Random nor
  Turbulence's selected geometry (`POL#21`). A variant of the Mover Trial
  becomes reasonable after `ROAD#30` if Stage Select crowds with trials or
  lifts and ferries should meet.
- Reopen the hold if `ROAD#30` shows held-open rules read as unfair, and E3 as
  written if flush lifts that vanish into the floor are wanted as a look (then
  (b) is required and the trial must test a player stalling the lift).

**Promoted slice (2026-11-03).** One cycling vertical lift: E1-E6 as amended,
G1-G5 as above, with `moving_structure`, `level_format` and editor acceptance,
on the new explicit-only Lift Trial. Bot routing, the paint-driven lift and
attended play stay separate rows, as for the gate and ferry. The capability
remains XL; this slice is its physics.

#### Vertical lift, first slice (`ROAD#28`)

**Done 2026-11-03.** The promoted slice above, built on the re-resolved
inventory. A lift is `"motion": { "type": "vertical", "travel_y", "period",
"phase" }` on the same `TranslatingStructurePaintable`
([format](level-format.md#vertical-lift-motion)). What each answer became:

- **E1.** `HorizontalMotionSpec` and the contract's `Spec` gain `travel_y`;
  exactly one of the two travels is positive and the other exactly zero, so a
  NaN or a second travel is refused as a two-axis request. Offsets stay a
  scalar on the mover's own axis: segments carry `from`/`to` (renamed from
  `from_x`/`to_x`), and `axis_offset` turns one into a world translation. The
  loader and the editor accept `vertical` and refuse `travel_x`, `axis` and
  `rotation` on it; the schema's `motion` is now a `oneOf`. The type names keep
  their `Horizontal` prefix: renaming the family would touch every probe for no
  behaviour, so the prefix now names where the capability began.
- **E2.** `HorizontalCarryStep` gains `displacement_y`; `apply_carrier` offsets
  the rider by both, before its own integration, under the unchanged checks.
  Only carried x enters footstep distance; landing reads the rider's own
  velocity, which carry never touches, so a jump from a rising lift leaves at
  exactly the still-ground launch.
- **E3, as amended, and one more refusal.** Admission refuses a lift whose
  shaft, its corridor above the highest pose or its padded body-height
  clearance beneath the lowest pose leaves the Scene or meets geometry, so the
  flush case is refused. The independent review of the slice found that this
  was not enough: a rider may overhang a lift's edge by almost a body, so a
  ledge within a body width of the shaft, between the corridor and the
  clearance, would meet a rising rider's head or a descending rider's overhang
  and make the hold fire in ordinary play (the first trial draft had one 10
  units away). Admission therefore keeps the lift's whole **column** clear: the
  shaft widened by a body each side, from a body above its highest pose to a
  body below its lowest. With the column clear, `LevelStructures::hold_closing_lifts`,
  which holds a lift's phase for a step that would push a living player through
  geometry or out of the Scene (`HorizontalMotion::hold`, the ferry's held
  step), cannot fire in an admitted level; the `moving_structure` case checks
  that a ridden lift never holds and tests the hold itself on the contract.
  The column means a rider leaves a lift at its top by hopping a body-wide gap,
  not by walking onto a flush ledge.
- **Solid only.** A jump-through landing compares the rider's previous feet
  with the platform's current top, which a rising lift has already passed, so a
  jump-through lift is refused (`validate_collision`) by the loader, the editor
  and the constructor.
- **E4.** Splash, runoff, paint samples, culling and the translated shadow take
  the full offset; flying-paint sweeps subtract the offset from the projectile's
  top and bottom for a lift, and a roller's visited footprint grows on y.
  `top_paint_team` already queried by x alone. Paint, credit and scored area
  stay on anchor faces.
- **E5.** The motion block is schema 2: eight spec words, the last `travel_y`,
  each offset on its own axis with the other word zero, and the fingerprint
  domain tag is `colourwars-horizontal-terrain-2`. `ReplayFile::format_version`
  is 7 and 6 is refused. Static replays write version 7's otherwise unchanged
  bytes.
- **E6.** A lift's per-step travel joins the vertical bound and leaves the
  horizontal one with only its rounding reserve; a horizontal-only level's
  admission arithmetic is unchanged (its lift term is zero).

**The Lift Trial.** `turbulence_lift_trial` ("Turbulence Yard - Lift Trial")
is the Mover Trial with its two movers replaced by two mirrored lifts and their
four stations by `lift_top_a/b` and `lift_foot_a/b`; it is explicit-only and
not in Random. Each lift is 48 by 20, solid `pencil`, centred in the 160-unit
gap between the upper approach and the crossover (x 1296 and 2656), with its
top anchor flush with the crossover at y 800. G1 asked for one storey of about
320 or the largest above about 200 that fits: below, the lower bed (top 1200)
leaves room for a lowest pose whose bottom is 1076, so the travel is **256**,
from the crossover's level to 144 above the bed, a hop from it; the bed to the
crossover is 400, which no jump reaches. The period is 3.2 s, 160 units/s (G2).
Both lifts start at the top (phase 0), since a lift's mirror keeps its phase.
The column fixes the width: a body width plus its pad each side of a 48-wide
lift is the whole gap. The trial's shadows ride with the lifts, and
`review_turbulence_lift_trial` and `turbulence_lift_station_tour` give it the
siblings' review frames.

**Not in this slice.** The paint-driven lift (G3b, the
[second slice](#paint-driven-lift-second-slice-road28)); deliberate
bot lift routing (bots ride a lift they happen to stand on, as they ride a
mover, but plan no route by it; their stuck check also counts carried y as
progress, which routing will need to discount); and attended play, now part of
`ROAD#30`.

**Qualification (MattPC, 2026-11-03).** Debug and Release builds pass. The
full default acceptance list passes in both presets (77 cases in Debug, 76 in
Release), including the new `moving_structure` lift checks (axis, contract,
admission refusals including the flush floor, a ledge 10 and 51 units beside
the shaft and the acceptance of one 53 away, a 300-step ride with no hold, a
still-ground jump launch, world queries), `level_format`'s lift fixtures, the
replay format-7 controls in `moving_structure` and `debug_console`, and
`instant_replay`, which now also replays a 20 s Lift Trial round with bots step
for step. `scripts/check-level-editor.ps1` passes in Release, with
`check_lift_motion`; in Debug its document checks pass and its native DPI
window probe failed twice locally on keyboard-focus checks this change does not
touch (different checks each time; the Release run of the same probe passes).
Hidden startup passes. A one-player Debug ride of the Lift Trial was inspected
at both ends of the storey. Attended play is `ROAD#30`.

#### Paint-driven lift, second slice (`ROAD#28`)

**Done 2026-11-03.** G3(b) from section 4: a lift that travels only while one
team holds more of its own paint, as the [paint ferry](#paint-ferry-road15-first-slice)
does, with `up`/`down` headings through the shared `decode_drive` and a sign
of its own modelled on [`ROAD#27`](#ferry-direction-and-stop-cue-road27). It is
`"drive": "paint"` and `"team_a_heads": "up"` or `"down"` on a vertical motion
record ([format](level-format.md#vertical-lift-motion)). Nothing else about a
lift changed: it is the same `TranslatingStructurePaintable`, admitted,
carried, swept and replayed as the first slice made it.

- **The rule is the ferry's.** `+1` still runs toward phase 0.5, which for a
  lift is its bottom, so `down` decodes to `+1` and `up` to `-1`; team B's
  paint heads the other way, a tie or no paint holds it, and it stops at
  either end. It moves at the cycle's leg speed (160 units/s on the trial), so
  the admission column and E6's budget are unchanged. Each step reads the paint
  held before the step. A paint-driven lift keeps to the outgoing leg, phase 0
  (top) to 0.5 (bottom), as a ferry does. `decode_drive` takes the axis, so a
  lift refuses `right`/`left` and a ferry `up`/`down`, each in its own words;
  the loader and the editor both call it. The level schema's two `motion`
  branches carry their own heading enums.
- **E3(b) is untouched.** A paint lift's step can still only be held by the
  safety check, which the column keeps from firing; a hold commits no heading,
  so its sign would show pause bars rather than claim travel.
- **The sign.** The ferry plaque's grammar, hung below the lift and moving with
  it: fixed A/B letters with up/down arrows (a head on a shaft, in the same
  3-by-5 cell as the ferry's chevrons), the current paint leader inverted, and
  a lower glyph for the last positive committed step. A lift's end stops are
  its own (`top_stop`, `bottom_stop`): a bar above an up arrow, or below a down
  arrow. Committed movement is read on y, so `movement_heading` is `+1` for
  down. A ferry's sign is never wider than its body, but a 48-wide lift would
  shrink the 176-wide sign to about a quarter, below a readable glyph cell, so
  a lift's sign is `ferry_direction.lift_width` wide (144 in
  `presentation.json`, required and positive), and never wider than the
  shaft's clear column, the lift plus a body width each side (152 on the
  trial). Level admission sets that limit after it checks the column, so a
  narrow lift or a larger `lift_width` cannot draw its sign over a ledge.
  Cycling lifts have no sign.
- **The editor.** Switching a lift's drive to `paint` adds `"team_a_heads":
  "up"`; the inspector labels the heading `(up or down)` and the phase `(paint:
  0 top to 0.5 bottom)`, and refuses `right`/`left` and a return-leg phase in
  the game's words. The canvas guide is a **PAINT LIFT** panel with the
  ferry's two team rows and vertical arrows. Mirror copy still refuses a
  paint-driven mover; the symmetry line reports a partnered pair as
  "Authored: paint-drive heading", as it did for ferries.
- **Bots** need no new policy, as for the ferry: a bot that rides a lift holds
  there to paint it, which drives it. They still plan no route by a lift.

**The Paint Lift Trial.** `turbulence_paint_lift_trial` ("Turbulence Yard -
Paint Lift Trial", explicit-only, not in Random) is the Lift Trial with both
lifts paint-driven, so the Lift Trial stays the cycle for `ROAD#30`'s physics
questions. Both lifts start at their **foot** (phase 0.5). The left lift,
`lift_a` on team A's side, has `"team_a_heads": "up"`; the right, `lift_b`,
`"down"`, so each team's paint raises the lift on its own side and the other
team's paint brings it down. Painting your own lift is the route from the bed
to the crossover; painting theirs takes it away. It has the siblings' review
pack (`review_turbulence_paint_lift_trial`) and joins `turbulence_station_tours`
through `turbulence_lift_station_tour`.

**Accepted as built, 2026-11-05** (`ROAD#30`): the foot start, which side's
team raises which lift, the 144-unit sign, and the speed, which is the cycle's
160 units/s.

**Not in this slice.** Deliberate bot lift routing, still unscoped (bots ride a
lift they happen to stand on and paint it, but plan no route by it; their
stuck check counts carried y as progress, which routing will need to
discount); a mirror rule for paint-driven movers; and attended play, which
joins `ROAD#30`.

**Qualification (MattPC, 2026-11-03).** Debug and Release builds pass. The
full default acceptance list passes in both presets (77 cases in Debug, 76 in
Release). `paint_ferry` now also checks the paint lift's motion directly (both
end stops, leg speed on y alone, the mid-shaft turn), its sign on both team A
headings (moving on y, a lift's own end stops, ties, exact uncued
motion/paint/credit controls, the lift-width plaque on the trial's 48-wide
lift and clamped to the column of a 24-wide one, inside the cull bounds), and plays the ferry rosters' three whole 120-second 3-bot
matches on the Paint Lift Trial, holding every step of both lifts to the paint
held before it. The lifts moved in all three (291, 679 and 388 steps, with 1 to
5 reversals), bots rode one while it moved in each (3 to 42 steps), and Debug
and Release measured the same. `level_format` loads up and down paint lifts and
refuses `right`/`left`, a missing heading, a return-leg phase and a cycle's
heading; `moving_structure`'s contract checks take the lift's own refusals;
`instant_replay` replays a Paint Lift Trial round with bots step for step;
`menu_presentation` refuses a missing, null, zero, negative or huge
`lift_width`; and the stage lists count thirteen. The `terrain` case's
translated-material check read only x; the trial's foot start exposed that and
it now takes the whole offset. `scripts/check-level-editor.ps1` passes in
both presets, with the drive, guide and Paint Lift Trial symmetry checks
above. Its native DPI probe's clipboard checks failed intermittently in each
preset (paste once, copy once, both under load, as recorded for the first
slice) and passed on rerun. The
`check-painter.ps1` `-FerryDirectionOnly`, `-StagePaperOnly`,
`-StageCountsOnly` and `-StageSummaryOnly` checks pass in Release, so the
ferry's sign is unchanged and Stage Select holds the thirteenth stage. Hidden
startup passes in both presets. Seed-7 Debug screenshots of both lifts showed
the signs reading `A↑ B↓` and `A↓ B↑` with pause bars while unpainted. At a
lift's foot the sign hangs over a player waiting beneath it: players draw over
it, and the clearance beneath the lowest pose keeps it off terrain, but whether
it reads there, or should be a fixed indicator above the shaft instead, is a
`ROAD#30` question.

#### Growing-sponge contract draft (`ROAD#29`)

**Draft recorded 2026-11-12; parked behind `REV#27` (2026-11-13).** The
author picked none of G1-G7. No Random arena wants a sponge, so nothing would
consume one; when `REV#27` sets a Random arena that does, G1-G7 are picked
starting from this draft's recommendations, and the draft is the resume point.
This is the written step
the 2026-11-12 audit set, on the
[vertical-lift precedent](#vertical-lift-contract-draft-road28). It changes no
code, tuning or content, chooses no trial, and leaves every accepted arena
alone (`POL#21`). It is accepted on the same terms as the lift draft: every
inventory row below is resolved against `220d4809`, the tree it was written on,
and every engineering answer names its precedent.

*A sponge, for this draft,* is a solid paintable block whose rectangle changes
between a rest pose and a larger grown pose during a match. Splatoon's sponges
swell when inked. What grows it, how far, and what its growth scores are the
gameplay calls in section 3.

**1. Sites that assume a scored object's area never changes.** "Assumes" is
what the site takes for granted; "A sponge needs" is the change, keyed to an
answer in section 2. Line numbers are those of `220d4809`.

| Area | Site | Assumes | A sponge needs |
|---|---|---|---|
| Scoring role | `game/objects/paintable_object.h:75-89` (`PaintableObject`) | held area and capture tallies come from one fixed tile set | the tile set G3 picks (E2) |
| Tile area | `game/objects/paint_tile.cpp:179-182` (`PaintTile::area`) | a tile's area is its construction rectangle | unchanged per tile; under G3(a) the tile is drawn scaled while its area stays (E2) |
| Tile generation | `game/objects/structure_paintable.cpp:53`, `:320-393` (`generate_paint_tiles`) | tiles and strips are built once from the authored rectangle, with faces clipped against neighbouring terrain at that moment | built once on the frame G3 picks; the envelope (E4) keeps the clip true at every pose |
| Sample contract | `game/objects/paintable_object.h:57-74` (`PaintSurfaceSample`, note at `:68-69`) | "Rigid transforms preserve this area; a future shape/scale needs its own rule" | G3 is that rule: `corners` give the drawn world quad and `scored_area` the scored area (E2) |
| Paint lookup | `game/objects/structure_paintable.cpp:152-191` (`paint_from_projectile`) | a strip's origin and pitch are fixed, and the splash is already in tile space | the splash mapped into the scoring frame first (E3) |
| Underfoot paint | `game/objects/structure_paintable.cpp:395-425` (`top_paint_team`, `top_paint_team_translated`) | the top strip spans a fixed x range | world x mapped into the scoring frame (E3) |
| Capture credit | `game/objects/paintable_object.h:13-38` (`PaintCredit::add`), `structure_paintable.cpp:191` | credit is added on capture and never taken back | unchanged (E5) |
| Object totals | `game/objects/structure_paintable.cpp:434-461` (`paint_total`) | the total sums every tile | the tiles G3 counts (E6) |
| Level sum | `game/objects/level.cpp:854-903` (`level_end_info`) | each paintable's totals are summed once, at time-up | unchanged; under G3(c) the pose at time-up decides (E6) |
| Coverage, shares, winner | `game/objects/level_end_info.cpp:42-104`, `:139-148` | `total_area` is the same throughout a match | under G3(c) the sponge's pose at time-up moves the total (E6) |
| Solo record | `game/save/match_records.cpp:60-70` | the coverage ratio is over a fixed total | the same as the row above |
| Awards | `game/objects/level_end_info.cpp:150-175` (`awards`) | laid and recaptured are capture tallies | unchanged (E5) |
| Paint-driven precedents | `game/objects/team_gate_structure_paintable.cpp:57-89` (`paint_leader`, `settle`), `game/contracts/team_gate_contract.h:55-65` (`leader`), `game/objects/translating_structure_paintable.cpp:88-94` (`drive_heading`) | a share of the object's own fixed paintable area | the same leader rule over the sponge's scoring area, if G1 is paint (E7) |
| Collision shape | `game/objects/translating_structure_paintable.h:66-68` (`bounds`, `shape` overrides) | a structure's shape is its authored rectangle unless it is a mover | `bounds` and `shape` overridden with the pose (E1) |
| Pose timing | `game/objects/level.cpp:381` (`settle_gates`, before the Scene update) | a gate's permission changes only at the step boundary | the pose changes there too (E1) |
| Projectile sweeps | `game/objects/projectile_contacts.cpp:115-124` (`relative_rectangle_at`), `:206` (`any_mover`), `:403` (`resolve_sweep`) | a moving target is a rigid translation; everything else is static | static within a tick, so the ordinary sweep applies (E1) |
| Shots the pose engulfs | `game/objects/level.cpp:535-537` (a burst drop's `resolve_launch_obstruction`) | a drop that starts inside cover is resolved where it stands | the same treatment for a shot a new pose covers (E1) |
| Players | `game/objects/player.cpp:592` (`apply_carrier`), `game/objects/level_structures.cpp:409` (`hold_closing_lifts`), `team_gate_structure_paintable.cpp:69-89` (`settle`) | a support is static or a rigid mover carried by identity; a gate stays open while its team is inside | a hold while a living player is in the next pose, and carry for a rider on top (E8) |
| Admission | `game/objects/level_structures.cpp:196-300` (`admit_horizontal`, `validate_admission`), `team_gate_contract.h:74-80` (`require_clear_of_spawn`) | the lift's clear column; no spawn body inside a gate | the largest pose's column clear, and no spawn body inside it (E4) |
| Shadows | `game/objects/level_object_builder.cpp:317`, `:658-690` (`build_shadows`) | a caster's rectangle is fixed or rigidly translated | the shadow follows the pose (E9) |
| Bot supports and obstacles | `game/objects/level_bot_world.cpp:102-112` (`obstacles`), `:114-131` (`supports`; flag at `:129`) | boxes are read from the shape on each query; `moving` means `mover() != nullptr` | the sponge flagged as moving (E10) |
| Bot paint surfaces | `game/objects/level_bot_world.cpp:82-100`; `game/ai/match_bot_driver.cpp:26`, `:126`, `:229`, `:268`; `game/ai/match_bot_territory.h:73-113` | sample corners locate a tile, and thickness comes from tuning | corners are the drawn quad, so aim points stay true (E2) |
| Match fingerprint | `game/debug/replay/match_fingerprint.cpp:36-99` (the motion block and its domain tag), `:189-192` (totals) | movers print their spec and tiles; totals are hashed | a sponge block with its own domain tag (E11) |
| Replay version | `game/debug/replay/replay.h:57` (`format_version = 11`) | the current layout | 12 (E11) |
| Format and loader | `game/contracts/level_format.h:78-80` (keys `motion`, `gate`), `:138-147` (object types); `game/objects/level_object_builder.cpp:321-345` (`checked_record_gate`); `game/contracts/level.schema.json:201-209` | a gate is a key on a `StructurePaintable` record | a `sponge` key read the same way (E12) |
| Editor | `editor/level_document.cpp:590-594`, `:639-642`, `:771-810`, `:844` (`duplicate`); `editor/paint_mechanism_guide.h:12` (`PaintMechanismKind`); `editor/level_document_mirror.cpp:40` (`mirror_object`) | it knows gates, ferries and lifts only | the sponge's label, clear, palette entry, guide, duplicate and mirror (E12) |
| Debug paint overlay | `game/states/game_level_dev_tools.cpp:168-170` | reads the level totals | nothing |
| Stage preview | `game/objects/structure_paintable.cpp:81` (`draw_miniature`) | draws the authored rectangle | the rest pose |

**2. Engineering answers, each following existing practice.**

- **E1. The pose changes only at the step boundary.** Sponges settle with the
  gates, before the Scene update (`level.cpp:381`), reading the paint held
  before the step. Within a tick a sponge is a static rectangle, so sweeps,
  contacts, `top_paint_team` and the bots all read one shape, and nothing ever
  needs a swept scale. A growth rate (G2) is many small boundary steps.
  `bounds()` and `shape()` take the pose, as the mover's do. A shot that a new
  pose covers is resolved where it stands, as a burst drop that starts inside
  cover is (`level.cpp:535-537`), and paints the face it is in. *Precedent:*
  the gate's settle, and the ferry's heading read from the paint held before
  the step.
- **E2. One scored lattice: the corners show, the area scores.** Tiles are
  built once, on the frame G3 picks. Each `PaintSurfaceSample` carries the
  drawn world quad in `corners` and the scored area in `scored_area`, which the
  sample contract already keeps apart ("Authoritative area from the scored
  tile, not recomputed corner arithmetic"). Bots aim at the corners, so they aim
  at what is drawn. *Precedent:* fixed slabs, whose samples are rigid quads
  (`fixed_slab_paintable.cpp:221`).
- **E3. Paint lands in the scoring frame.** A splash rectangle, or a world x
  for underfoot paint, is mapped into the lattice by inverting the pose before
  the existing strip lookup. Under G3(a) that means scaling about the anchor;
  under (b) and (c) it means shifting by the face's offset. *Precedent:* "Paint
  lands in the rest frame: the splash is shifted back by the pose"
  (`translating_structure_paintable.h:69-71`) and
  `top_paint_team_translated`.
- **E4. Admission keeps the envelope clear.** The largest pose must stay
  inside the Scene and off every other collision object, together with a body
  width each side and a body height above. That is the lift's column. No spawn
  body may lie inside it, by the gate's spawn rule. Construction-time terrain
  clipping then holds at every pose, and E8's hold is the only occupancy rule.
  A sponge is solid only, because a rising top passes the feet a jump-through
  landing compares against. *Precedent:* the lift's column and solid-only
  refusal ([first slice](#vertical-lift-first-slice-road28)), and
  `require_clear_of_spawn`.
- **E5. Credit is never taken back.** `PaintCredit` counts captures. A tile
  that is hidden or shrinks debits no one, just as a recapture debits no one
  today, so the awards are unchanged. *Precedent:* `PaintCredit::add` only
  adds.
- **E6. Held area and total follow G3, read at time-up as today.** Under G3(a)
  and (b) the total never changes. Under (c), coverage, the shares, the winner
  and the solo record use what the sponge shows at time-up, with no special
  case in the results. *Precedent:* `level_end_info` already reads every
  paintable once, at the end.
- **E7. Paint drive reads the sponge's own totals** through the gate's
  `leader` rule (a majority and a minimum share of its scoring area), using the
  paint held before the step. *Precedent:* the gate's settle and the ferry's
  `paint_heading`.
- **E8. Occupied growth holds, and a rider on top is carried.** A pose step
  that would overlap a living player anywhere except standing on the top holds
  for that step and resumes once they are clear. A player standing on the top
  moves with it by the top's displacement, before their own integration, and is
  released on a jump or drop as on a lift. A shrinking top lowers its rider the
  same way. Unlike the lift's hold, this one is a gameplay rule that fires in
  ordinary play, because the envelope is open space until the sponge grows into
  it. The trial must test a player stalling a sponge, which is the case the
  lift's amended E3 refused to allow. *Precedent:* the gate held open while its
  team is inside, `hold_closing_lifts`, and the lift's carry (`apply_carrier`).
- **E9. The shadow follows the pose** under the existing caster rule (wider
  than tall, and thinner than `shadow_caster_limit`). *Precedent:* the
  translated shadow follows the mover's offset.
- **E10. Bots read a sponge as a moving support.** `supports()` already reads
  the shape's box on each query; the `moving` flag also becomes true for a
  sponge, and riding code treats it as it treats a mover. Deliberate sponge
  routing is a later slice, as it is for lifts. *Precedent:* the movers' flag,
  and lift routing left out of both lift slices.
- **E11. Version the record.** The fingerprint gains a sponge block under its
  own domain tag (`colourwars-sponge-1`), hashing each sponge's pose, its hold
  and its tile list, as the motion block does. `ReplayFile::format_version`
  goes from 11 to 12 and refuses 11. The shipping instant-replay tape is held
  in memory and re-simulated, so it carries no format. *Precedent:* the lift's
  E5.
- **E12. Format and editor follow the gate.** A sponge is a `"sponge"` object
  on a `StructurePaintable` record, beside `gate`. A new engine-free
  `game/contracts/sponge_contract.h` decodes it for both the loader (beside
  `checked_record_gate`) and the editor, so both refuse in the game's words.
  The class is `SpongeStructurePaintable : StructurePaintable`, in the game's
  usual subclass grammar. The schema gains its entry. In the editor, the sponge
  gets its inspector label, its clear and its palette entry, a
  `PaintMechanismKind::sponge` guide drawing the rest and largest poses, a
  duplicate placed past the envelope, and a mirror that reflects the growth
  side. A sponge refuses a `motion` or `gate` key. *Precedent:* the team gate's
  key, contract header and subclass.

**3. Gameplay calls, as options for the author.** Values are provisional
reference points only, and none of these gates the engineering.

- **G1. What grows it.** (a) Paint, by the gate's rule: it grows while one team
  leads its paint by at least a share of its area, and returns to rest when no
  team leads. (b) Any paint: the more of its area painted by either team, the
  larger it is. (c) A cycle, with no paint. *Recommended:* (a), because it
  reuses a qualified contract (E7) and gives the leading team a route.
- **G2. How it grows.** (a) Between two poses, rest and grown, at a fixed rate
  (the lift's 160 units/s as a reference). (b) In proportion to the leading
  share, which gives many poses. *Recommended:* (a). Two poses keep the
  envelope, the hold and the guide simple.
- **G3. What its growth scores.** (a) The rest area scores and the grown pose
  draws the same tiles scaled, so the total never changes and growth is route
  and cover only. (b) The grown lattice always scores, and tiles hidden at rest
  keep their paint and still count. (c) Only shown tiles score; hidden tiles
  keep their paint and come back with it, so a grown sponge adds territory.
  (d) Like (c), but hidden tiles return neutral. *Recommended:* (a) for a first
  slice, since no scoring site changes. Choose (c) if play shows a sponge
  should be worth territory.
- **G4. Which way it grows.** (a) Up and to both sides from a fixed foot,
  standing on a floor. (b) Up only, as a rising column. (c) About its centre,
  floating. *Recommended:* (a). It reads as a sponge, and the fixed foot keeps
  its bottom face clipped by the floor at every pose.
- **G5. Occupancy.** (a) E8 as written: hold while a player is in the way, and
  carry riders on top. (b) Riders hold it too, so standing on a sponge stops
  its growth. *Recommended:* (a).
- **G6. Which trial carries it.** (a) A new explicit-only Sponge Trial derived
  from Turbulence Yard like its siblings. (b) A variant of an existing trial.
  Neither joins Random or touches Turbulence's selected geometry (`POL#21`).
  *Recommended:* (a).
- **G7. Bots.** (a) Bots ride a sponge they happen to stand on and paint it,
  with no routing by it, as for lifts. (b) Deliberate routing in the first
  slice. *Recommended:* (a). Routing waits on a consumer, as lift routing waits
  on `REV#27`.

Once `REV#27` sets a consumer and the author has chosen, a bounded first slice
is promoted with `moving_structure`, `level_format`, `paint_ferry`-style scoring checks and
editor acceptance on its trial, as the lift's was.

### Paint-object trial play (`ROAD#30`)

**Accepted as built (the author, 2026-11-05).** The Ferry, Gate, Lift and
Paint Lift Trials stay explicit-only, out of Random, at their current values
(the gate's 0.25 open share and held-open rule, the ferry headings, the
lifts' 160 units/s, foot start and side assignment, the 144-unit sign); none
of these is provisional any more. The attended 1-4P trial play below was
waived and not performed, so the judgements it lists are unmade, not
answered. The original brief (M, 2026-10-28): play the explicit-only Ferry and Gate Trials
from both sides at 1-4P, and since 2026-11-03 the Lift Trial
([`ROAD#28`](#vertical-lift-first-slice-road28)) and the Paint Lift Trial
([second slice](#paint-driven-lift-second-slice-road28)). Judge whether ownership-driven travel and selective
passage create useful painting/route choices, whether the gate's held-open
rule is understandable, and whether the open share and ferry headings feel
fair. For the lifts, judge whether riding at 160 units/s is a free, risk-free
escape (the author's trigger for trying about 100), whether scored moving
surfaces swing the score more than their area justifies (the trigger for an
unscored or top-only lift), and whether the top-dock hop reads. For the paint
lifts, judge whether raising your own side's lift is worth the paint, whether
the foot start and side assignment are right, and whether the lift's sign
reads at 1-4P. Record keep/change/cut decisions and any value changes. The gate and
ferry cues (`ROAD#26`, `ROAD#27`) are ready for review. Current
trial play is already available. Neither automated matches nor this review opts a trial into Random
or selects Turbulence's geometry (`POL#21`).

### Instant replay (`ROAD#19`)

**Done 2026-10-27.** After a timed round, the End menu offers **Watch Replay**
on row 1, under the continuation, and the setup actions and Exit move down a
row. It plays the round's last `match.replay` seconds (8 in `tuning.json`)
with a REPLAY card at the top of the screen, then opens the End menu again over
the moment time ran out. Confirm, Back or Start ends it early. The panes are the
round's own: the same human panes, HUDs, clock and paint. The arena music and
the round's sounds play, and pads do not rumble. Watch it as often as you like.
Untimed practice has no End menu and so no replay.

**How.** It is the Debug record/play model (`DBG#2`) in Release, held in memory:
a Debug recording is the same `RoundTape` written to a file with its checks.
Each round draws its own seed from the running `rand()` sequence just before
it is built, and the build draws only from the `MatchRandom` made from that
seed (`HYG#39`): separate streams for a Random palette and the intro's persona,
and the seed itself for the shipping bots. `GameLevel` keeps the round's palette
preference, since Paused Options can save another mid-round, and `RoundTape`
(`game/match/round_tape.h`) keeps that seed, a copy of
the setup (a re-pick page can change the match's own and then be backed out
of), every participant's final input on every logic step, and where the round was
suspended (which clears jump buffers) or resumed (which re-reads held jumps).
`GameLevel` records timed rounds through `RoundTapeRecorder`, the seat filter
every live update passes through, behind the Debug filter when there is one.
Untimed practice keeps the filter but starts no shipping tape (`ROAD#31`). Watch Replay
destroys the finished level and builds it again from the seed and the round's
palette preference, with no intro and the countdown skipped. It runs the steps before the replay's window with
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

The End paper grew from 820 to 900 units tall, with rows 117 apart from y 300
(previously 125 from 330), so six rows fit with the item font's descenders.
Validation checks Watch Replay on row 1, the setup actions on both their rows,
and Exit on rows 1 and 5.

Rows were first 118 apart, which hung Exit's row-5 focus card 3 units into the
paper's drop shadow (card bottom 985, face bottom 982). The End painter check
now also puts Exit on row 5 and requires its card on the face (2026-10-28).

Startup validation did not catch this, nor the Credits Back card that the
Settings painter check caught the same day: it measures each card against the
whole paper box, shadow included. Measuring against the face (`ROAD#25`) is a
one-line change in `presentation_validation.cpp`. With both overhangs fixed the
shipped content passes it, but the `menu_text` acceptance case's hand-built
paper-edge fixtures (Home, Options Back, Restore defaults, Pause, End) are pinned
to the box edge and move with it.

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

### Whole-round time-lapse (`REV#48`)

Watch Replay shows only the last `match.replay` seconds in the round's own
panes, though the tape holds the whole round. Splatoon 3's Battle Replays
re-create a whole battle from a top-down camera. The 2026-11-10 review
proposed a time-lapse of the whole round from one arena camera
([record](review/2026-11-10-project-review.md#whole-round-time-lapse-replay-rev48)).

**Debug half done 2026-11-12.** `replay.timelapse [speed] [stills]`
([Time-lapse](debug-console.md#time-lapse)) plays the whole tape again,
`speed` taped steps a frame (default 16, so a 240-second round takes 15
seconds), through the stage's authored overview camera, silent, with no HUD,
clock or REPLAY card, and can save evenly spaced stills: candidates for the
README image (`REV#12`). It reuses the instant replay's rebuild; what is new
is a step multiple per frame and `Level::set_arena_view`, a single fullscreen
view through `zoom_out_finish_bounds` that leaves the simulation alone. The
`debug_console` case checks that a whole tape played that way ends on the
round's own paint.

**Open: the player-facing End row.** Not designed. It needs rulings on the
pacing (a fixed speed or a target length), on whether the clock or any HUD
shows during the pass and whether sound plays, and on where the row sits.
The End paper was resized for six rows (above), and `POL#15` wanted fewer
steps between rounds. Then it needs the row's copy in `presentation.json`, the
End menu's validation and painter updates, and a play check, since the payoff
is unproven without play. It runs after the reveal, so the `ROAD#17` decline
does not apply. The author declined `REV#54`'s coverage-over-time strip on
Results on 2026-11-13 in favour of this row; the strip is reopened only if this
End row is rejected and the author still wants the round's arc shown.

### Menu cards stay on the paper face (`ROAD#25`)

**Done (2026-10-28).** `check_menu_item` now uses the resting paper face's
right/bottom boundary: the whole paper size minus its existing shadow offset,
matching `MenuPaperArtwork::draw_card`. The renderer, authored presentation and
intended page layout are unchanged. Font/measurement/fitting checks still
precede containment, with the same diagnostic text and field order.

Home, Options Back, Restore defaults, Pause and End paper-edge fixtures now
place their inclusive pass at the face, then refuse the first overhang into
shadow with the existing relevant item diagnostic. The focused `menu_text`
case passes in Debug and Release. Native `MainMenuOnly`, `SettingsMenuOnly`,
`OptionsDefaultsOnly`, `PauseMenuOnly` and `EndMenuOnly` checks pass in both
presets across six sizes/aspects (720p, 1080p, QHD, 4:3, 16:10 and ultrawide),
including retained/reopened labels, focus cards/arrows and complete-page
bounds. Representative full-page and contact-sheet captures were inspected.
This is the corrected baseline for the separate `HYG#1` extraction.

### No shipping replay tape in untimed practice (`ROAD#31`)

**Done (2026-10-28).** The menu's untimed lesson practice previously
kept every logic step's inputs and timing in an unused shipping replay tape.
`GameLevel::build_and_enter_level` now clears the previous tape, builds the
level, and starts capture only when the built level is timed. An unstarted
`RoundTape` ignores both input steps and suspend/resume gaps.

The decision uses `Level::practice()`: ordinary solo arenas, multiplayer and
developer-started timed Movement Course still capture. Seed generation is
unchanged, and timed capture starts before the Debug post-build hook applies
cheats or placements. The shipping filter remains installed in practice, so
independent Debug input, recording and playback still pass through it.

**Qualification (Debug and Release, 2026-10-28).** The focused native
`scripts/check-painter.ps1 -PracticeReplayOnly -Preset <preset>` checks the
actual `GameLevel` on a hidden shell. It retains no shipping steps or gaps
through 4,800 simulated practice steps (60 seconds, then 10 seconds after
Restart and 10 seconds after a timed-to-practice rebuild), actual Pause/Resume
and focus count-ins. The rebuild first has a nonempty timed tape and gaps to
clear. Debug additionally forwards 120 steps through the real `ReplayRecorder`
and its inner input driver while the shipping tape remains empty.

Ordinary menu solo, developer-started timed Movement Course and multiplayer
with a shipping bot each offer Watch Replay through the actual End page.
Their 543 combined recorded steps reproduce public player gameplay bits,
projectile counts and hashes of every scoring tile. These checks initialize
native devices but perform no raster drawing or readback and make no
physical-pad or performance claim.

Both builds, the CPU `instant_replay` and `practice_workshop` cases, and
hidden startup with the accepted audio bank pass. The retained seven-arena
replay case still reproduces 1,201 steps per arena; its new disabled-capture
check forwards 10,000 downstream input steps and bot-replacement decisions.
Receipts are under `out/validation/road31-practice-replay/` (`acceptance/`,
`painter/` and `startup/`). This is focused qualification, not the full suite.

The workshop wording and its keep/cut call are `POL#19`: the author kept the
workshop with its current wording on 2026-11-05.

### Player profiles (`ROAD#21`)

**Done 2026-10-27.** A profile is a name of up to ten characters (A-Z, 0-9 and
spaces) carrying everything a person sets for their pad: both stick deadzones,
rumble, the three comfort switches (View shake, Aim marker, Reduce effects),
the pad buttons and a Boost. There are at most eight, saved in a `"profiles"`
section; the save is version 4. A damaged entry keeps the fields that read and
falls back for the rest, and an entry with no usable name, or a name already
taken, is dropped.

A pad wears at most one profile and a profile is worn by at most one pad. While
a pad wears one, the pad-slot settings every reader already uses (now
`SettingsSession::play_settings` and `input_bindings`, which `HYG#48` moved out of
`Save`) show the profile's values for that slot. Pad sticks and
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

Play with real pads was waived by the author on 2026-11-05 and was not
performed; profiles are accepted on the scripted and painter checks above.

#### Initial host profile Boost (`ROAD#34`)

**Done (2026-10-29).** Player Count preclaims the initiating device for P1.
Join formerly took a worn profile's Boost only after a later successful claim
or profile cycle, so the already-claimed host skipped inheritance. A pad wearing
`Boost +2` could reach Weapon Select with the old/default Boost; the normal
menu-start carry-back could then replace its profile's `+2` with that stale value.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | The preclaimed host misses the profile Boost rule used by later joins. | Selected as `ROAD#34`: apply the existing rule at the initial claim boundary. Small setup correction using the existing shared page hierarchy and scripted-pad painter. |
| 2 | The screenshot basename sanitizer preserves reserved Windows stems before appending `.png`. | A separate pure filename-policy slice; no reserved-path experiment. |
| 3 | Input-fixture resource cleanup needs an explicit measurement contract. | Continue `ED#20` independently; preserve its failed candidate and diagnostic limits. |

The existing `take_profile_boost` helper now belongs to `MainMenuPage`'s protected
interface, with its body unchanged. Player Count calls it immediately after
beginning the initial input claims. Later guests and profile cycling continue to
use that same rule. Bare pads and keyboard/mouse retain their existing policy;
there is no save format, value or profile-wearing policy change.

The old-source baseline drove actual Player Count confirmation through the
existing scripted reader, claimed Pad0 for P1 and recorded expected Boost `2`
versus actual `0`. Its private save stayed unchanged, and it failed before
any reference rendering or later case. Evidence is
`out/validation/implementation-loop-20261028/road34-baseline/9f640a714cdb4a2caa8aeb7789607425`.

Both game builds, the `player_profiles` CPU case and `-BotPlayersOnly` pass in
Debug and Release. Sixteen actual Player Count/Join/Weapon paths cover pad
slots 0 and 3, 1P/2P and Off/+1/+2, bare pads, keyboard/mouse and later guest
inheritance. Forty-three CSV rows pass, including committed manual Boost
preservation through Stage-to-Weapon-to-Team-to-Weapon navigation. Reference
pages explicitly contain the literal expected Boost. Confirming Player Count
again begins new claims and may inherit the profile again.

Navigation preserves the complete private profile file and records; a separately
labelled Save API call checks carry-back and reopening that existing private
file. The new fixture does not build a match or qualify actual match-start
persistence wiring. Existing 720p/1080p bot setup and mixed-GameLevel checks
also pass unchanged. Outer isolated saves remain byte-identical, all 14 guarded
sources remain unchanged, and independent review passes. Worktree-status
collection is explicitly omitted and recorded as null.

Candidate evidence is
`out/validation/implementation-loop-20261028/road34-candidate/849726c0c5154677b7960b8975acfc72`;
native painter leaves are Debug `2844bbe443f849c5a96e255b89bc30f6` and Release
`978ad28fc1a14d1e96fa46bd58b1678d` under its `painter` directory. No real pad or
player save was used. The attended `ROAD#21` session and `ROAD#5` fairness play
were later waived by the author (2026-11-05) and not performed.

#### Paused stick preview after controller recovery (`ROAD#35`)

**Done (2026-10-29).** A seat reclaimed by a controller in another XInput slot
keeps its original profile (`ROAD#4`). Paused Pad sticks correctly selects that
profile, but its preview reads the original physical slot directly. With logical
Pad 1 mapped to device 4, the page can show disconnected or unrelated input even
though gameplay and pause navigation use device 4.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | `StickSettingsMenu::refresh_preview` reads `state(slot_)` while pause navigation already resolves `device_slot`. | Selected as `ROAD#35`: use the recovered device only in paused previews, retaining the logical profile and labels. Small caller-wiring correction with an existing scripted-pad readout fixture. |
| 2 | Input-fixture bitmap/DC cleanup has no explicit ownership receipt. | Continue the independent `ED#20` scope; its aggregate discrepancy remains unexplained. |
| 3 | External capture loss leaves the editor's resize flag set. | Investigate separately before asserting a lost-edit defect. |

Preserve unpaused Options' physical-slot preview even if its shared setup object
retains an old mapping. Keep profile selection, deadzones, persistence, roster,
navigation and live gameplay unchanged. Extend `-StickReadoutsOnly` with literal
connection, raw movement, processed movement and aim expectations for a remapped
paused seat. Reconnecting the original device with different input must not
change that paused preview. An unpaused control with the same stale mapping
must read the original physical slot instead. Preserve every existing readout
matrix, private save bytes and seat/device mapping. Retain an old-source failure,
then qualify both builds and focused native checks with worktree-status collection
explicitly omitted. This software wiring check does not qualify physical-pad
comfort or the attended `ROAD#21` session (waived 2026-11-05, not performed).

`StickSettingsMenu::preview_device` now resolves a slot through
`MatchSetup::device_slot` only while the page has a pause owner. The
paused preview samples that device. A keyboard-owned pause starts on the first
logical pad whose device is connected, the rule `pad_seat_missing` already uses.
Profile, deadzone, rumble and comfort edits and labels remain keyed by the
logical slot. Independent review found the keyboard-owner start; the first
candidate had remapped only the preview.

The old source failed at the first new comparison, `road35-paused-recovered`,
because it read disconnected Pad 1 instead of device 4. With the preview remapped
but the old start loop, `road35-keyboard-owner-recovered` failed because the page
opened on physical Pad 3. The final fixture has 14 cases and 56 literal readouts
at 720p and 1080p, within its separate 96-update budget, and adds no crops.
The cases are paused owner, original-device reconnect, replacement reversal and
disconnect, unpaused stale map, unmapped paused owner and keyboard owner. Every
existing readout matrix passes unchanged. Private save bytes, active
preferences, worn profiles, both rosters and device mappings are unchanged.

`-StickReadoutsOnly` and `-PausedOptionsOnly` pass in Debug and Release. So
does the `stick_settings` acceptance case, which builds both games. Worktree-status
collection was explicitly omitted. Evidence is under `out/validation/road35`:
`red-debug` (old source), `red-keyboard` (preview only) and the `final-*` runs.
No real pad or player save was used.

#### Profile names beyond Join (`REV#35`)

**Done 2026-11-12.** A worn profile's name used to appear only on Join and the
Pad rows; every other roster label said "P1 - Pad 1". The
[2026-11-10 review](review/2026-11-10-project-review.md#profile-name-on-hud-and-results-rev35)
asked for the widest name to be checked on each surface first, and the name
used where it fits.

The check used ten `W`s, the widest name the rules allow, in `en` and
`qps-ploc`, at the shipped layout, in design pixels:

| surface | limit | plain label ends | with the name ends | outcome |
|---|---|---|---|---|
| Team Select | first team icon, x 650 | 376 (406 qps) | 925 (983 qps) | fits only shorter names |
| Weapon Select | weapon tile border, x 642 | 376 (406 qps) | 925 (983 qps) | fits only shorter names |
| Results | score bar's right edge, x 1310; panel x 1610 | 900 (940 qps) | 1266 (1326 qps) | fits a row without an award |
| Pause, two missing pads on a line | paper, 1050 wide | 478 (536 qps) | 1572 (1690 qps) | fits only shorter names |
| HUD identity box, full-width pane | clock paper, x 845 | 522 | 658 | fits, 187 px to spare |

So the name is used where it fits and the plain label stays where it does not.
`worn_player_label` ([player_label.h](../game/states/player_label.h)) composes
`menu_text.labels.worn_profile`, "P1 - Pad 1 (ALEX)", for a human pad seat
whose pad wears a profile (`SettingsSession::worn_profile_name`). The device
stays in the label, so a guest can still tell which pad is theirs. Bots, the
keyboard and bare pads keep their labels. The pages fall back to the plain
label, which startup validation already fits, at the point where the longer one
stops fitting:

- **Team and Weapon Select** fall back when the label reaches the first team
  icon or the weapon tile's border.
- **Results** falls back when the row reaches its award, or the score bar's
  right edge when it has none. A name never costs a row its award.
- **Pause** names the missing pads only while the whole two-to-a-line list
  fits the paper and ends above the first action's focus card.
- **The HUD identity box** always fits, so it shows the name as a third line
  under the pad. `GameLevel` hands each pad's name to `Level::set_profile_names`
  as plain text, so `game/objects` still never sees `GameData`. It does this
  after every build and update, because Paused Options can change what a pad
  wears, and the instant replay's level gets the same names. A new last
  validation phase holds the widest name, team label and device clear of the
  clock's paper, closing-clock ticks included, at the widest HUD scale. It fails startup if a font or offset change breaks that.
  Join keeps its existing rule (`worn_player_label` while connected).

Checks: `player_profiles` covers the session's name lookup and the composed
labels for pad, keyboard, bot, bare and widest-name seats. `player_identity`
covers the HUD box's third line, which follows the pad's identity slot and goes
when the profile comes off. `locales` runs the new HUD fit in `en` and
`qps-ploc`. `menu_text`, `menu_presentation`, `text_wrap`, `results_awards`,
`instant_replay`, `match_suspension`, `resume_countdown` and `session` pass
unchanged (`menu_text`, slow in Debug, in Release only). The whole default Release list passed before the replay hand-off was
added; the focused cases above passed after it in both builds, and Release
starts (`check-startup.ps1`).
On MattPC the painter's `-MenuLabelsOnly` leaf gained a worn-profile block. On
the real pages, Pad 1 wears ALEX and Pad 4 the widest name. ALEX shows on Team,
Weapon and Results. The widest name falls back on Team and Weapon, and is kept
on a Results row with no award. The four-pad Pause list falls back to plain
labels. Once Pad 4 returns, the three-pad list names ALEX. The leaf passes in
Debug and Release; in Debug, so do `-ContributionTeamOnly`,
`-CompetitiveWinnerOnly`, `-ConnectionTextOnly`, `-EndMenuOnly`,
`-IdentityOnly`, `-HudOnly`, `-JoinPaperOnly`, `-BotPlayersOnly` and
`-MenuTextOnly`, unchanged. The Debug default list was not run whole. Two parts
are not covered by a painter check: a Results row falling back because of its
award, and a drawn HUD box wearing a name. The first rests on the page code;
the second on `player_identity` and the startup fit. No real pad was used.

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
screenshots and replays). Looking it up never creates it; since 2026-11-12
every successful save write does, so a first run leaves it ready (`FTR#14`,
[Save layout](#stage-records-and-the-sectioned-save-gap62)). After the manifest
loads, `load_user_stages` (`game/config/stage_loading.h`) reads every `*.json` directly
in it, in file-name order, and checks each on its own. Since 2026-11-10 it
reads at most the first 256 by file name, and a file over 1 MiB is refused
unread past that point ([Limits](level-format.md#limits), `REV#2`):

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
Debug recordings now fingerprint the loaded semantic definition (`DBG#8`).
Playback requires the current catalog entry and matching loaded definition;
an edited/rescanned stage or a legacy unverified recording is refused before
playback changes, even with `force`. An edit without rescan still uses the
unchanged loaded snapshot. The usual same-build replay limits remain.

The author's calls (accepted 2026-11-05):

- **Random.** A user stage joins Random only when its file says
  `"include_in_random": true`; without the key, or with `false`, it stays out.
  This is the opposite of a shipped stage, where a missing key means "in
  Random", because an unfinished stage turning up in a party's Random pick
  seemed worse than having to choose it. `LevelLoadedInfo::asks_for_random()`
  is true only for a present `true`, and `load_user_stages` sets the stage's
  `include_in_random` from it. The Random hint counts the live pool, so an
  opted-in stage is counted. Until 2026-11-05 user stages were never Random
  candidates, whatever the key said.
- **Names and records.** The id is the bare file name, the same namespace as
  the shipped stages, so a user stage keeps its records when it is later
  registered. The cost is that a future shipped stage with the same name
  refuses the user stage and inherits its records.
- **Marker.** The hint is the only marker (accepted 2026-11-05). A mark on the
  name or preview would move the page and its painter fixtures.

The `user_stages` acceptance case points `LOCALAPPDATA` at a scratch folder
before it asks for the directory, and scans scratch folders beside its own
executable (one with a non-ASCII name): a missing, empty and non-folder path;
two valid stages, one asking for Random; and broken JSON, a level without
spawns, a missing sprite frame, a music/effect conflict, a case-only collision
with Crossfeed, two bad file names, an undrawable name, a text file and a
`.json` folder. It requires the valid two in order after the shipped stages,
marked, the one asking for Random in the Random pool (the shipped pool plus
that stage) and the other outside it, each bad file refused in one line naming the
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
  `game/contracts/user_stage_contract.h`. That header uses the standard library only
  and is included by the game's scan, `directory_consts` and the editor, as
  `horizontal_motion_contract.h` is. Shipped names come from the source
  manifest (`ContentCatalog::stages`). The editor asks before it replaces
  one of the player's own stages, creates the folder only on this command
  (removing it again if a cancelled or refused save leaves it empty), and
  never touches the save. See
  [Play the level](level-editor.md#play-the-level).
- **A rescan without a restart.** `UserStageFolder` (`game/config/stage_loading.h`)
  owns the folder for the session. `wWinMain` declares it above the
  `Application`, attaches it after the manifest and runs the startup scan
  through it. `GameMenu` takes it as an optional second argument and answers
  `MenuOwner::rescan_stages` with it. `MainMenuStageSelect::init` calls the
  rescan as the page opens from the main menu. A match's owner, `GameLevel`,
  has none to lend, and the page also skips it between rounds, so the list holds
  still while a match or series could be using it. A rescan replaces the
  user stages in the `StageCatalog` (`game/config/stage_catalog.h`), the only writer
  of the list and the registry: the manifest stages unchanged, then the
  folder's stages in file-name order, with the list it started from put back
  if the read throws. Each new or edited file is checked as at startup and
  replaces its registry entry. The sound groups are then
  reassigned over the new list (it cannot fail on stages that each passed
  `check_candidate`, and if it does the session keeps only the shipped
  stages). Only refusals the previous scan did not print reach stderr, so
  reopening the page stays quiet. A stage whose file vanished or now fails
  leaves the list. Stage Select's `slot_of` already falls back to the first
  slot for a remembered or current stage that is gone. The old definition
  stays registered in the catalog, unreachable from its list, because
  `labrador::Registry` has no remove; `StageCatalog::definition` answers from
  the list, so it does not find one. That costs memory only; see the draft
  report in [engine reports](review/engine-reports.md). Probes that build a
  `GameMenu` without the folder keep their fixed lists. The Debug `reload`
  command still rereads only tuning and presentation.
- **A note for players.** The release ZIP ships the root `README.md`
  (`cmake/release_package.cmake`). Its new "Your own stages" section names
  the folder and the name rule, says when the game reads the folder and
  where refusal details are available, and points to Save to My Stages, noting that
  the editor is not in the ZIP.

The `user_stages` case now also runs a session: an unattached folder (every
probe's) does nothing. A startup scan loads one stage. A rescan adds a new
stage that sorts first, rereads an edited one (both the list and the registry
show the new name), and refuses a broken file. A rescan with no change gives
the same list. A rescan after one file is deleted and the other broken
returns to the shipped stages, leaving Crossfeed untouched and the whole list
passing `validate_game_content`. The editor's document checks
(`check-level-editor.ps1`) cover Save to My Stages under a scratch
`LOCALAPPDATA` ([Acceptance checks](level-editor.md#acceptance-checks)).

Accepted as built (the author, 2026-11-05):

- **When to rescan.** Only as Stage Select opens from the main menu. It is the
  one screen that shows the list, and nothing there holds a definition. A
  file watcher or a Debug command would add moving parts for no extra
  player-visible effect.
- **The editor's shortcut** is Ctrl+M (for "My Stages"). Ctrl+Shift+S was
  taken.

The author settled the Random and marker calls above and accepted the
scan-details action/help wording of `ROAD#32` below on 2026-11-05. Attended
play of a real player-made stage was not performed.

### User-stage refusal details in the game (`ROAD#32`)

**Done (2026-10-28).** `UserStageFolder` retains its latest scan. Main-menu
Stage Select shows an issue count separately from the chosen arena's briefing;
Down while browsing opens a covering details page. Left/Right changes issue,
Up/Down changes page, and Confirm/Back/Pause dismisses. The page owns a copied
snapshot and four text widgets, preserving the arena cursor underneath.

Existing filename/reason text is retained. Actual font extents wrap even an
unbroken filename and divide it into pages. Printable ASCII stays verbatim;
other UTF-16 units use `\uXXXX`, and invalid UTF-8 bytes use `\xXX`, matching
the game's current font coverage. Folder-level and aggregate failures count
as issues too. No-refusal scans clear stale details. Startup/rescan boundaries,
per-file acceptance and stderr deduplication are unchanged; opening/closing
details never scans, and matches/series rounds do not offer this view.

Optional `main_menu.stage_scan` and `menu_text.stage_scan` records own layout
and copy, with compatible defaults. Diagnostic legends have their own font
and footer-relative geometry; status/counter text fits its reserved area.
Resource checks run after the existing ordered presentation checks and cover
full ASCII glyphs, templates, paper/row geometry and shadow-inclusive legends.
The new wording is identified in the content: `User-stage scan`,
`issue(s)`, `Down Scan details`, issue/page navigation and the escape hint.
The author accepted it, with the `ROAD#16` Random and marker calls, on
2026-11-05.

Both game builds, `user_stages`, `stage_selection`, `menu_presentation` and
`menu_text`, plus isolated accepted-audio startup/user-stage fixtures pass in
Debug and Release. `check-painter.ps1 -StageScanOnly` passes both presets:
one good/one broken actual folder, selected-stage return and actual valid-match
start; changed/removed/fixed scans; locked/series/unattached/clean controls;
three deduplicated stderr lines over six permitted scans. Each run has 151
menu updates, 181 level updates, 238 readbacks, 156 independent text/raster
comparisons across six window sizes/aspects, ten full frames and five crops.
The long Unicode/invalid-byte fixture spans five pages without losing text.
Review included compact parent/details captures and the 4:3 long-text page.
All folders and profiles are isolated; human play remains `ROAD#16`.

### Series stage rule (`ROAD#20`)

**Done 2026-10-28.** The Mode page has a Series stage row under Match length,
an option row like it: Left and Right (or Select) cycle **Same stage**,
**Rotate** and **Loser's pick**. It decides which stage a series' next round
plays. Rotate is the default (the author's call, 2026-11-05; until then it was
Same stage, what every series did before); a single match has no next round,
so the row does nothing there. The rule is saved with the remembered setup
(`"last_match.stage_rule"`, written only when it is not the default,
`default_series_stage_rule`; added in save version 5). A save without it, or
with a spelling this build does not know, reads as Rotate. That includes older
saves that chose Same: builds before the change left the key out for Same, so
a keyless save cannot tell "chose Same" from "never touched it", and such a
player sees Rotate and can change it back on the Mode page. The save stays at
version 6, because the format did not change. The row's value, Stage
Select's picking heading and the Mode prompt are `presentation.json` text with
startup fit checks ([Mode labels](menu-presentation.md#mode-and-player-count-labels)).

What each rule does, decided on the round that just finished
(`game/match/series_stage_rule.h`):

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
(`-SetupPaperOnly`) draws the new Mode row; since the 2026-11-05 default change
it opens on Rotate and walks Right to Loser's pick and Same. On MattPC (2026-10-28, Debug)
`-EndMenuOnly`, `-SetupPaperOnly`, `-DurationTextOnly`, `-SetupMotionOnly`,
`-RememberedSetupOnly`, `-MainMenuOnly`, `-HomeReturnFocusOnly` and
`-StagePaperOnly` pass. `-MenuTextOnly` had stopped compiling when `ROAD#21`
added two prompts; its prompt list is brought up to date, and it now runs but
fails first on Home's Credits label, a page this change does not touch.

The author's calls (2026-11-05): Rotate became the default, and the other
three behaviours were accepted as built:

- an explicit-only starting stage plays once and then leaves the rotation;
- a draw replays its stage under every rule, Loser's pick included;
- a bot-only losing team does not pick; the stage rotates.

The practice hint moved beside Single Match, and Mode's row spacing went from
100 to 96, to make room for the row; that is an art-pass call too.

### Random does not repeat the last stage (`REV#24`)

**Done 2026-11-10.** Stage Select's Random used to map one `rand()` draw onto
the whole Random pool, so from the shipped pool of four a new Random start
landed on the arena just played about one time in four. It now draws from the
pool without the setup's current stage whenever the pool holds more than one
stage, and the remaining stages stay equally likely. A pool of one, a setup
with no stage yet, and a setup whose stage is not a Random candidate all draw
exactly as before.

- **Where the last stage comes from.** It is the stage in the setup the page
  edits, read just before the confirmation overwrites it. In the main menu
  that is the save's `last_match` at launch and then each Stage confirmation.
  Between series rounds (Change Level, Loser's pick) it is the stage of the
  round just played. It is the stage last played from that setup whether
  Random or the player chose it.
- **The limit.** The main menu's setup keeps the stage that started a series
  ([Series stage rule](#series-stage-rule-road20)). After a series under
  Rotate or Loser's pick, a new Random start therefore avoids the series'
  opening stage, not its final one, and can land on the arena the series
  ended on. Making that exact would mean writing a series' last stage back
  into the remembered setup, which this change does not do.
- **What is unchanged.** Rotate and Loser's pick (`rotated_stage`,
  `series_next_stage`) never used this draw and are not touched. The Debug
  planner's `stage random` keeps the plain draw over the whole pool. The
  Random hint still counts the whole pool, one more than a draw can reach
  when the setup's stage is in it.
- **Seeds.** The pick still takes exactly one `rand()` draw, so every later
  draw under a Debug `seed` is the same as before. The stage a seeded menu
  Random pick lands on does change whenever the setup's stage is in the pool:
  it is now a function of the seed and that stage. One probe pinned a seed to
  a menu stage: the painter's stage-paper check starts Random under seed 4937
  from a fixture pool of three with the setup's stage in it, and expected the
  whole-pool draw (`close_quarters`; the page now commits `turbulence`). Its
  two expectations now leave out the setup's stage as the page does, still
  written out in the probe and not through `random_stage_index`.

`random_stage_index` (`game/objects/stage.h`) takes the last stage as a third
argument and stays a pure function; the two-argument form is the same draw
with nothing left out. The `stage_selection` acceptance case pins it: a pool
of two always returns the other stage, a pool of four gives each of the other
three an equal share in manifest order, a pool of one still returns its stage,
an empty, explicit-only or unknown last stage matches the two-argument draw,
and each of the four shipped Random arenas as the last stage is never drawn
again while the other three get four draws in twelve each. `stage_selection`,
`series_stage_rule`, `remembered_setup`, `user_stages`, `menu_navigation` and
`turbulence_slab_stage` pass in Debug and Release. On MattPC (2026-11-10)
`check-painter.ps1 -StagePaperOnly` passes in both presets with the updated
expectations. Not run: the other painter checks that drive the real Stage page
on a GPU (the join and stage motion check and the Random text check confirm
Random from a pool of one and expect one consumed draw, which this keeps), and
how it plays.

### Teammate markers (`ROAD#18`)

**Teammate half done 2026-10-28; no threat markers (the author, 2026-11-05).** In each
human pane, a living teammate whose body is wholly outside that pane's view
gets a chip at the pane edge. The chip sits on the line from the pane centre
toward the teammate, pulled in until it and its pointer are `margin` inside
the edge. It has an ink keyline, a fill in the teammate's team colour and their
roster badge (`P2`, the same drawing as the world badge), plus a stepped ink
pointer with a team-colour core aimed at them. Bots count as teammates and get
markers in the human panes, though they have no pane themselves. Enemies, the
pane's own player and dead teammates are never marked. A teammate even partly
in view has no marker: their world badge, which `ROAD#10` already keeps inside
the pane, does that job. Markers show during live play only, not in the
countdown, the opening or the overview.

How it is built: `TeammateMarker` (`game/objects/teammate_marker.h`) is a new
`CrudeDrawing`, beside `PlayerBadge`. Its static `place` does the geometry for
one pane, camera and body. `Level::teammate_markers(view)` lists a pane's
markers in roster order, and `draw_player_overlay` draws them in HUD
coordinates just before the HUD, so a pane's own panels stay on top where a
chip reaches a corner. Each marker fades in over `fade_in` from the moment its
teammate leaves the view; the age is per pane and per teammate, advanced by live
ticks only. `PlayerBadge::draw` gained an optional opacity so the badge fades
with its chip.

Content: `presentation.json` `hud.teammate_marker`: `enabled`, `ink`, `size`
(72x58), `keyline_width` (3), `badge_inset` (6), `pointer_length` (16),
`pointer_width` (12), `margin` (10) and `fade_in` (0.25 s), in HUD reference
pixels scaled with the pane as the rest of the HUD is. The loader requires
sizes from 16 to 128, a keyline of at most 8 and an inset of at most 16 that
together leave at least half the chip for the badge, a pointer of at most 48
that is wider than the keyline and at most half the chip, a margin of at most 64,
a fade of at most 1 s, and an opaque ink.

Accessibility: no new toggle. A viewer pad's Reduce effects (its `ROAD#7`
override, or Shared with the Options switch) shows a marker at full strength
at once instead of fading it. The keyboard seat follows the shared switch. The
marker never blinks or moves on its own, so nothing else needed reducing.

Accepted as built (the author, 2026-11-05; the attended readability look was
not performed):

- no Options or per-pad toggle of its own. If one is wanted it should be a fourth
  `pad_comfort` field next to View shake, Aim marker and Reduce effects
  (Off / Shared / On), with an Options switch, as `ROAD#7` set out;
- HUD scale, so at 4P a chip is half its 1P size (36x29 px at 1080p);
- drawn under the HUD panels;
- a full marker for a teammate wholly off-screen, nothing for one partly in
  view, and nothing while dead or respawning;
- the stable follow camera, not the shaken one, decides on-screen, so view
  shake never makes a marker flicker;
- a 0.25 s fade-in.

**Threat markers are not drawn (decided 2026-11-05).** Marking enemies off-screen would
show every player where the other team is, a change to the game's information
model: rear threats are currently found by looking and listening (the `POL#3`
C2 rear-threat recognition review in [polish-crossfeed.md](polish-crossfeed.md)).
On 2026-11-05 the author decided that threats get no marker of any kind, which
keeps the withheld-score decision (gap audit 1.1) on whose grounds the author
declined a live overview map (`ROAD#17`, 2026-11-03). Only teammates are
marked.

CPU acceptance: `teammate_markers` builds real Levels on Movement Course at 1,
2, 3 and 4 human panes, at 1280x720 and 1920x1080. The rosters include a bot
teammate in P2 with P3 owning the second pane, and an enemy in every roster. In
every pane it places teammates on screen, straddling the edge and off screen in
eight headings. It checks one marker per off-screen teammate and none for
enemies, self or a teammate in view. Each marker must be the presented size,
inside the margin with its pointer, on the line toward its teammate, aimed at
them and clamped to the edge. A death clears the marker and a respawn restores
it. The case also checks the fade from 0 to full, per-pad and shared Reduce
effects, the disabled presentation, and 38 malformed `teammate_marker`
variants (256 placed markers in all).

Native evidence: an isolated `+players 4 +bot all paint` Crossfeed capture
(seed 2, 1080p) shows P4's chip in P2's pane and P2's chip in P4's pane, both at
the edge toward each other. Team A's panes show none because those two were on
screen together. Evidence: `out/validation/road18-teammate-markers/`.

Painter checks on MattPC (2026-10-28): `-IdentityOnly`, `-MonitorOnly` and
`-SupplyOnly` pass in Debug and Release. `-HudOnly` fails in both builds at
*HUD meter lost its exterior dark keyline* in its 4P 720x1280 portrait case.
That case draws `InterfaceGameplay` alone over a blank canvas, with no Level and
so no markers. At that pane size the HUD scale is 0.1875, which makes the 2 px
keyline 0.375 px. The base branch without these markers fails `-HudOnly` the
same way in Debug (rerun at review), so the failure predates `ROAD#18` and is
not fixed here.

Nothing is left: the threat-marker call is made (none). The attended
readability look at 1, 2 and 4 panes with a pad (chip size at 4P, overlap with
the HUD panels at the top corners, pointer legibility, whether the fade helps)
was part of the pad sessions the author waived on 2026-11-05 (`ROAD#2`); it was
not performed.

## Project review calls (2026-11-13)

The author's calls, on 2026-11-13, on the DECISION rows the
[2026-11-10 project review](review/2026-11-10-project-review.md) raised. That
review is a frozen record, so its rulings live here. A later gap list checks
this section first: a declined row below is an intended absence, and each
names what would reopen it. Detail stays with the row's owner, linked.

**Taken.** Each row was taken as open work with a first step; a row that has
been built says so and when.
- **Out of bounds is an elimination (`REV#1`, M; built 2026-11-14).** A live
  player whose box leaves the bounds is killed through `Player::kill()` in
  place of the throw at the end of `Level::update` (`game/objects/level.cpp`),
  at the same point in the tick, so every in-bounds match and tape is
  unchanged: no owner, no splat credit for the
  [awards](#results-awards-road12), otherwise a normal death and redraw. A
  player already dead outside the bounds is not killed again: it stays where it
  fell and the normal respawn places it back at its spawn, rather than being
  moved mid-elimination. `PlayerOutOfBounds` and the fixed
  `player_out_of_bounds` reason
  ([failure reporting](failure-reporting.md#phase-and-original-cause)) are
  removed, the spawn and station diagnostics say a player built outside the
  bounds is eliminated, and the "seal the arena" warnings in
  [level format](level-format.md#coordinates-bounds-and-spawns),
  [the editor guide](level-editor.md) and the README are gone, so a stage may
  have pits. The `out_of_bounds` acceptance case pins the elimination, the
  missing credit, the respawn, the edge and a repeat run.
  ([Record](review/2026-11-10-project-review.md#a-player-leaving-the-arena-ends-the-process-rev1).)
- **Developer panel out of Release (`REV#13`, S; done 2026-11-13, see
  [Release behaviour](debug-console.md#release-behaviour)).** `GameAction::debug` is
  bound only under `COLOURWARS_DEV_TOOLS` (`action_bindings.cpp`); the draw path
  and members stay, so no layout depends on the gate, and F1 stays reserved in
  rebinding in both builds. The Debug help entries leave `presentation.json` and
  `qps-ploc.json`, and the README's "Debug overlay" Controls row goes. The
  controls-help pins are rebaselined and the `ControlsMenuOnly` painter leaf is
  re-qualified on MattPC. [The developer console](debug-console.md) is then the
  only place the panel is documented.
  ([Record](review/2026-11-10-project-review.md#developer-text-panel-ships-in-release-rev13).)
- **A crash record, not a minidump (`REV#63`, S-M).** Unhandled-exception and
  terminate handlers write the existing `last-error.log` fields: a crash phase,
  the exception code, the faulting module (`ColourWars.exe`, engine, system or
  "other", so injected modules go unnamed) and its RVA, which the published
  [PDB](releasing.md#release-symbols-rel7) symbolicates. It rewrites
  [failure reporting's Boundary](failure-reporting.md#boundary). A stack
  overflow, or an exception swallowed in a Win32 callback, may not reach it.
  ([Record](review/2026-11-10-project-review.md#no-crash-dump-and-no-release-symbols-rev63).)
  Done 2026-11-14 ([crash record](failure-reporting.md#crash-record-rev63)).
- **Reduce effects from Windows on first run (`POL#49`, S; built
  2026-11-13).** Yes; the
  [polish item](polish-plan.md#reduce-effects-default-from-windows-pol49) owns
  it. The read is the client exhibit for the engine's `REV#60`
  ([ledger](review/engine-reports.md#what-is-deliberately-not-here)).
- **Random skips the exempt pairs (`REV#14`, S; built 2026-11-13, see
  [`ROAD#8`](#more-team-palette-pairs-road8)).** A per-pair `in_random` flag
  in `team_palette`, false on `pink_green` and `blue_yellow`; both stay fixed
  Options choices. This revisits the author's earlier answer to
  [gap audit 2.1](review/gap-audit.md#21-the-discrete-random-pair-is-the-design-the-missing-piece-is-a-route-around-it)
  and [`ROAD#8`](#more-team-palette-pairs-road8), and keeps the random-pair
  model. First step: list every seeded colour pin and cross-build Debug tape
  before the roll changes.
  ([Record](review/2026-11-10-project-review.md#random-palette-draws-the-exempt-pairs-rev14).)

**Partly decided.**
- **Umbrella questions answered (`ROAD#13`).** Q1-Q7 are recorded in the
  [umbrella proposal](#umbrella-tool-proposal-road13). Building slices 2-4 is a
  separate call after `PIN#2`; slice 1 (`REV#3`) was done on 2026-11-13.

**Deferred, each with its trigger.**
- **Bot takeover of a missing seat (`REV#28`).** Trigger: a series actually
  lost to a pad that cannot return, seen or reported. Then only the
  between-rounds End menu version, never a mid-round takeover.
  ([Record](review/2026-11-10-project-review.md#bot-takeover-for-a-missing-seat-rev28).)
- **HUD text floor (`REV#29`).** Trigger: a player reports split-pane text
  unreadable, or `POL#16`'s UI pass rebaselines the HUD fixtures. Then an
  opt-in Options "HUD text: Normal / Large" row (default Normal), not a global
  floor. [Measurements](polish-hud.md#split-pane-text-size-rev29-2026-11-12).
- **Reveal strips as drawn (`REV#38`).** Trigger: `POL#16`'s art pass changes
  how paint is drawn, or someone watching a real reveal at couch distance cannot
  tell where the paint is.
  ([Record](review/2026-11-10-project-review.md#paint-strips-are-hairline-thin-in-the-reveal-rev38).)
- **Crossfeed's own music (`REV#42`).** Deferred with `POL#16`; the shared
  track stays. Under the default Rotate the two arenas never meet, but a Random
  first pick or Loser's pick can put them back to back.
  ([Record](review/2026-11-10-project-review.md#crossfeed-shares-the-close-quarters-music-rev42).)
- **Live enemy-paint cue (`REV#10`).** Trigger: attended play or a report shows
  people cannot tell why they slowed or stopped refilling. That reopens one
  choice under `POL#30`'s reopen rule: a cue, milder amounts, or no. A cue
  starts with the visual tank state, since a step sound is a new cue hook. The
  HOW-TO-PLAY line and the Movement Course lesson stand as the explanation.
  ([Record](review/2026-11-10-project-review.md#paint-underfoot-has-no-player-cue-rev10).)
- **Growing sponges (`ROAD#29`).** Parked behind `REV#27`; see the
  [draft](#growing-sponge-contract-draft-road29).
- **Distribution channels (`REV#67`).** No code signing and no separate
  checksum. itch.io waits for the first non-pre-release tag, or until the
  author wants a public store page; `REV#68` (Steam) stays deferred behind it.
  [Releasing](releasing.md).
  ([Record](review/2026-11-10-project-review.md#distribution-is-one-unsigned-github-zip-rev67).)

**Declined.**
- **No containment around the match or replay step (`REV#4`).** Declined;
  `REV#1` has landed and removed the only throw players could reach. Reopen if a
  released build writes a `run_loop` failure report `REV#1` does not explain.
  ([Record](review/2026-11-10-project-review.md#no-containment-around-the-match-or-replay-step-rev4).)
- **No Title/Home save notice (`REV#5`).** `HOW-TO-PLAY.txt` names the copies
  kept aside instead ([save layout](#stage-records-and-the-sectioned-save-gap62)).
  Reopen if a player reports lost settings or records.
- **Bot finish window kept (`REV#23`).** Closed in the code's own wording; see
  the [`ROAD#39` note](#boost-on-paint-underfoot-road39).
- **No time-up banner (`REV#41`).** The round ends on sound and the camera
  move. ([Record](review/2026-11-10-project-review.md#no-on-screen-time-up-signal-rev41).)
- **No roster strip (`REV#46`),** teammates-only or full; the full roster was
  already ruled out under [`ROAD#18`](#teammate-markers-road18).
- **No attacker or tool on the respawn wait (`REV#47`).**
  ([Record](review/2026-11-10-project-review.md#tell-a-splatted-player-who-got-them-rev47).)
- **No attacker-paint burst on elimination (`REV#50`).** Recorded beside
  [`POL#30`'s answers](polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30).
  Reopen if attended play shows kills feel unrewarded.
- **No own-colour wall slide or wall jump (`REV#51`).** Recorded beside
  `POL#30`. Reopen if a new arena is themed on vertical traversal, and then only
  as a default-off trial on that arena.
- **No Sniper charge (`REV#57`).** The Sniper accepted on 2026-11-05 (1.0 s
  interval, 2000 jet, shooter-only `sniper.ready` cue) stands: no problem has
  been observed and attended play was waived. Reopen if attended play shows
  Sniper eliminations reading as unfair. It removes the splatling from
  `ROAD#13`'s Q1.
- **Player cues stay centred at their authored pitch (`REV#43`).** The
  per-seat `ui.ready` pitch idea in
  [audio inventory 4.5](audio-inventory.md#45-menus-and-between-rounds) goes
  too. Reopen if an attended 3P/4P session finds players cannot tell their own
  sounds apart, which would be a mix defect.
- **No coverage-over-time strip (`REV#54`).** The
  [time-lapse](#whole-round-time-lapse-rev48) is the round's arc. Reopen if its
  End row is rejected and the author still wants the arc shown.
- **No graded solo goals (`REV#25`).** Stage-only record keys are now a ruling
  ([stage records](#stage-records-and-the-sectioned-save-gap62)). Reopen if the
  author decides solo against bots should have goals.
- **Series sides never swap (`ROAD#40`).** See
  [Series side swap](#series-side-swap-road40).
- **No exploration mode and no solo practice on any stage (`REV#55`).** Reopen
  when `REV#64` is taken.
- **The editor stays source-only (`REV#64`), and `REV#65` (Play Stage, Open My
  Stages) closes with it.** Reopen both when players other than the author ask
  to make stages, or when the author sets the editor as a release aim.
  ([Record](review/2026-11-10-project-review.md#level-editor-is-not-shipped-to-players-rev64).)
- **No passive game-code observer (`TOOL#5`).** It closes on the
  [diagnostics already measured](gameplay-tuning.md#bot-diagnostics-tool5).
  Reopen when `REV#20`'s per-tool bot roles are scoped and need exact shot or
  sub-use rates.
- **No Parsec paragraph (`REV#69`).** Reopen when someone has played a streamed
  session with a remote pad.
- **No trial push for foreground checks (`CI#1`).** `check-debug-console` stays
  a local, opt-in human check ([constraint](debug-console-record.md#foreground-checks-ci1)).
  Reopen if a self-hosted or dedicated runner appears, or a debug-console
  regression reaches `master` that the CPU-side `debug_console` case missed.

## Backlog audit additions (2026-11-12)

A 2026-11-12 audit of the open-work index looked for work no row carried and
checked each candidate against the existing rows, the declines under
[Not tracked, by choice](#not-tracked-by-choice) and the 2026-11-10 review.
These five survived. None is engineer-ready: the first was answered no on
2026-11-13, and the rest need a written scope or the next engine pin first.

### Series side swap (`ROAD#40`)

**Decided 2026-11-13: never.** A team keeps the same half in every round of a series.
Team A spawns must sit strictly left of the arena's midpoint and Team B's at or
right of it ([level format](level-format.md#coordinates-bounds-and-spawns)),
because the opening previews each team's colour over its own half
(`on_own_half` in `game/contracts/level_format.h`). The
[Series stage rule](#series-stage-rule-road20) chooses the next stage, not the
sides, so under Same stage a Best of 5 can play one side five times. The polish
plan's fairness and level-play checks name side swaps, and the balance harness
swaps sides to cancel any side advantage
([gameplay tuning](gameplay-tuning.md)). No side advantage is shown: the slab
trial's lopsided paint is recorded as "not yet a geometry finding"
([Turbulence](polish-turbulence.md)). Splatoon, which this game follows, does
not swap, because its maps are mirrored.

**The author's ruling:** series sides never swap, as today and as in
Splatoon. Reopen if a non-mirrored arena enters Random, or a balance run shows
a side advantage beyond noise. A swap would then be a value on the existing
Mode row, a match option inside the 2026-11-03 options ruling, and its design
note would cover the opening preview, what the A/B labels then mean, the
remembered-setup key and the replay format.

### Practice targets in the workshop (`ROAD#41`)

**UNSCOPED, LOW, M.** The Movement Course workshop teaches that eliminations
clear room ([stations](polish-movement-course.md)), but a solo practice has
nobody to eliminate and no station gives the player something to shoot. The
workshop proposal asks for "cover around which the different shot shapes can
be tried" ([polish plan](polish-plan.md)). The 2026-11-10 review left a
practice target out because a damageable figure with a damage readout would put
numbers into a game that shows none
([`REV#56`](review/2026-11-10-project-review.md#sub-lesson-station-in-movement-course-rev56)).
A grey, non-scoring figure that pops and redraws through the existing wordless
respawn, with no HUD element or number, avoids that objection.

First step: a scope section for a new station kind that places such a figure,
which takes hits through the existing `Collider` path, in untimed solo practice
only, with the `practice_workshop` checks it adds. It is a new subclass beside
the existing ones and flattens nothing. Related: `REV#56` (the Sub lesson, done 2026-11-13);
solo practice on any stage (`REV#55`) was declined on 2026-11-13, so the
figure stays in the workshop.

### Shot shapes on Weapon Select (`ROAD#42`)

**UNSCOPED, LOW, M.** Story A6 ([design process](design-process.md)) is a
player who wants to know what each tool does before picking it. Weapon Select
answers with an icon and wrapped text
([Weapon-selection names and artwork](menu-presentation.md#weapon-selection-names-and-artwork)),
and whether that text is enough was `POL#24`, waived on 2026-11-05 and not
performed. The five tools differ most in shape: the bucket arcs, the mister
spreads, the roller holds its aim and the sniper reaches furthest. No
readability problem has been observed, and the practice workshop already lets
a player try each tool.

First step, if selected: a small looping diagram on the focused card, drawn
from the tool's own tuning (speed, gravity, spread) so it cannot drift from the
shipped tool, behind a presentation switch that defaults off, compared with the
current card in the `-WeaponPaperOnly` painter fixture. It is new menu art, so
it may wait for `POL#16`'s UI pass.

### Flood-fill paint object (`ROAD#43`)

**UNSCOPED, LOW, L.** The paint objects so far are the ferry, the gate and the
lifts, with sponges (`ROAD#29`), a bounce pad (`REV#52`) and an eraser
(`REV#53`) proposed. None borrows the paint program's defining tool, the fill
([classic paint research](classic-paint-research.md)). A swatch that fills one
authored region for the team that holds its trigger would keep the drawings in
charge ([Rules for anything new](backstory.md#rules-for-anything-new)), only
lays paint, so it keeps the invariant `REV#53` breaks, and fills a fixed area,
so it needs none of the sponge's changing-area contracts.

First step: a contract draft in the manner of the
[vertical-lift draft](#vertical-lift-contract-draft-road28): the trigger rule
(the gate's held-paint rule is the precedent), the region as authored
paintable structures, a cooldown, `PaintCredit` for the triggering team, bot
awareness and a Yard trial stage. It ends with the author's question whether a
bulk territory swing fits the balance accepted on 2026-11-05.

### Circle shapes for round projectiles (`ROAD#44`)

**DECISION, LOW, M: the author decides on hit feel; measured 2026-11-14.**
Every projectile used to collide as a `RectangleF`, the round ones included.
The engine pinned since 2026-11-13 collides circles in the narrow phase
(Labrador `f6752ca7`, "Collide circles in the narrow phase", 2026-11-04), so
the round shots can now be circles. The circle code is in, behind a switch
that defaults to the shipped rectangle, and the contacts are measured below.
Whether shots keep the circle is the author's call on hit feel.

**The switch.** `RoundShotShape` ([round_shot_shape.h](../game/objects/round_shot_shape.h))
is `rectangle` (shipped) or `circle`. It covers every `ProjectileBall` (the
bucket's ball, the grenade's drops and the sprinkler's drops) and the
`ProjectileGrenade` body. A shot reads it once, when it is built, and keeps
that shape for life; a grenade's drops take the grenade's shape. Only two
things set it: the Debug console's `shots.round rectangle|circle`
([debug-console.md](debug-console.md#commands)) and the Debug balance runner's
`--round-shots` (`scripts/run-balance.ps1 -RoundShots`). With the default,
nothing a player sees changes and no pin or fingerprint moves:
`instant_replay`, `balance_runner`'s seeded byte identity and the other
projectile cases pass unchanged. The shape is how a shot collides, not a
number, so it is code and never a `tuning.json` leaf (AGENTS.md: data says
*which* and *how much*, never *how*). It is one process-wide value rather than
a `Level` option because weapons, subs and other shots build shots and none of
them holds the `Level`; threading a setting through `LevelBuilder`, `Player`,
`Weapon` and `SubWeapon` would be the larger change for a switch that is
meant to go once the author decides. A seeded tape therefore replays bit for
bit only under the setting it was recorded with.

**What circle changes.** The circle is the disc inscribed in the shot's
collision square (ball 100, drop 36, grenade 40 units), so it is the same size
face on and smaller only at the corners. `bounds()`, drawing and every paint
footprint stay the square, so the painted area of a hit is unchanged; only
whether and where the shot stops changes. The engine has no swept circle, so
`Projectile::resolve_sweep` sweeps one itself (`circle_sweep_interval` in
[projectile_contacts.cpp](../game/objects/projectile_contacts.cpp)): the
centre's path against the obstacle box rounded by the radius, in double
precision, with the same entry inset, tie rules and breakable order as the
box sweep. A disc that only grazes a corner in float arithmetic flies on to
the end of its step. Cover and players on flat stages are swept as discs;
two things stay boxes: the launch exemption (`launch_leaves_overlap`, which
only ever skips), and the fixed slabs and movers, whose own sweeps are box
sweeps (`FixedSlabGeometry::sweep`, the mover's relative segments). Those
stand only on the Yard trials and `turbulence_slab_trial`, so that stage's
numbers below are partly box-shaped. A circle shot paints a fixed slab with
its square, as it paints everything.

**The tally.** `Level::flight_retirements()` and `launch_retirements()`
([shot_retirements.h](../game/objects/shot_retirements.h)) count how round
shots leave play: on cover, on a player, on a planted sub, by running out of
time, or out of bounds; in flight, or stopped at the muzzle by launch
obstruction. Grenade bodies count apart from the balls. Nothing in a match
reads it back. The balance runner prints it as four `shot-contacts` lines per
match, never as CSV columns, so both CSV schemas are unchanged.

**Measurement (2026-11-14).** Debug runner, this commit's tree on engine pin
`1dc8533c`, four stages (`crossfeed`, `close_quarters`, `press_tower`,
`turbulence_slab_trial`) x 32 seeds (1000-1031) = 128 1v1 matches per run,
both seats the shipping bot at Hard, 60 s each:

```
scripts/run-balance.ps1 -SkipBuild -Stage crossfeed,close_quarters,press_tower,turbulence_slab_trial `
    -Seed (1000..1031) -Players 2 -Bot match -Difficulty hard,hard -Duration 60 `
    -Tools bucket,bucket -Subs wall,wall -RoundShots rectangle   # then circle
# the same with -Tools sprayer,sprayer -Subs grenade,grenade
```

The first pair has no grenades, so every ball is a bucket shot; the second
has no bucket, so every ball is a grenade drop.

| Bucket balls, 128 matches | rectangle | circle | change |
|---|---|---|---|
| Shots retired in flight | 9,829 | 10,356 | +5.4% |
| on cover / on a player | 8,558 / 978 | 8,974 / 1,123 | +4.9% / +14.8% |
| ran out of time / on a planted wall | 249 / 44 | 211 / 48 | -15.3% / +9.1% |
| Player share of flight retirements | 9.95% | 10.84% | +0.89 pt |
| Stopped at launch (cover / player) | 13,695 / 1,042 | 13,230 / 1,066 | -3.4% / +2.3% |
| Player contacts per shot, launch and flight | 8.22% | 8.88% | +0.66 pt |
| Hit ticks dealt (hit-view) / eliminations | 1,988 / 168 | 2,162 / 196 | +8.8% / +16.7% |
| Painted area, all matches | 8,026,039 | 8,172,652 | +1.8% |
| Final fingerprints that differ | | 121 of 128 | |

| Grenades (sprayer seats), 128 matches | rectangle | circle | change |
|---|---|---|---|
| Drops retired in flight (cover / player / time) | 1,750 / 1,060 / 668 | 1,807 / 1,082 / 663 | +3.3% / +2.1% / -0.7% |
| Player share of drops in flight | 30.48% | 30.46% | flat |
| Drops stopped at launch (of all drops) | 300 (7.9%) | 204 (5.4%) | -32% |
| Grenade bodies (cover / player / at launch) | 146 / 153 / 16 | 150 / 154 / 9 | flat |
| Hit ticks dealt / eliminations | 18,379 / 205 | 18,867 / 204 | +2.7% / flat |
| Painted area, all matches | 10,003,601 | 9,946,954 | -0.6% |
| Final fingerprints that differ | | 42 of 128 | |

Per stage, bucket balls on players in flight went 255 -> 274 (Close Quarters),
327 -> 337 (Crossfeed), 237 -> 347 (Press Tower) and 159 -> 165 (Turbulence
Slab Trial).

**Reading it.** The bots react to what happens, so a match diverges at its
first different contact and these are counts over different matches, not
the same shots twice; read the shares and the direction, not single digits.
The shape matters to almost every bucket match (121 of 128 end differently)
and to a third of grenade matches. It does not make the ball harder to land:
the bucket's share of player contacts went up slightly (8.2% -> 8.9% of shots,
eliminations +17%), because a disc that clears a ledge or a doorway corner
keeps flying instead of dying on the corner, and some of those reach the
enemy. Fewer shots die at the muzzle (-3% bucket, -32% grenade drops), for the
same reason: a drop burst against a wall or floor no longer catches the
corner it is born beside. Face-on contacts are identical in both shapes
(`round_shots` pins this), so what changes is only the corner case: in
circle mode a ball can visibly brush a corner and pass, where today the
square stops on it. Painted area moves under 2% either way.

**The author's question.** Keep circles for the ball and the grenade, or not?
`shots.round circle` in a Debug build (then `restart`) plays it before
deciding.

- **Keep:** make `circle` the default, rebaseline anything that moves (the
  seeded tapes and fingerprints that fire round shots, `bot_ballistics` and
  `bot_combat_trials`, whose aim is still worked out with rectangles), retire
  the switch and its console and runner options, and file the engine's lack
  of a swept circle in [engine-reports.md](review/engine-reports.md), with
  `circle_sweep_interval` as the exhibit. Slabs and movers would want disc
  sweeps too.
- **Decline:** delete the switch, the circle members of `ProjectileBall` and
  `ProjectileGrenade`, the circle branch of the sweep and the slab's circle
  footprint, and the `round_shots` case; keep the tally and the runner's
  `shot-contacts` lines if they stay useful.

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
repeatable reads and unchanged confirmation ownership. Physical-pad feel was
part of ROAD#2 (waived 2026-11-05, not performed); synthetic state checks do not close that session.

Validation: `scripts/run-acceptance.ps1 -Preset x64-debug` and `x64-release`
both passed, including the new case and Release developer-tool exclusion.
`scripts/check-startup.ps1` passed for both executables with isolated saves and
graceful shutdown; evidence is under `out/validation/menu-navigation-debug/`
and `out/validation/menu-navigation-release/`.

## Stage records and the sectioned save (GAP#6.2)

2026-10-17: every finished round now counts toward its stage's record, and the
save that holds it is built from sections and carries a version key.

**Save layout.** `Save` reads and writes a list of `SaveSection` objects
(`game/save/save_section.h`) against one root object; each section's read is caught
on its own, so a malformed section falls back to its defaults, printing one
stderr line with its key and the error, and leaves the others intact. There is
no migration pass: each section reads every older layout in place, as
[save_section.h](../game/save/save_section.h) sets out. The display and play preferences stay flat root keys exactly as
before, so every existing save loads unchanged and needs no migration.
`"save_version": 9` is written first (version 1 introduced records, version 2
remembered setup, version 3 independent input bindings, version 4
`"profiles"`, version 5 `"last_match.stage_rule"`, version 6 the flat
`"vsync"` display key, version 7 `"last_match.variant"`, version 8 each
player's `"sub"` and version 9 the Sub binding; [save.h](../game/save/save.h)
keeps the list). A missing key is version 0, the same flat layout.
A higher version still loads every key this build knows and prints one
stderr line; the next write drops what it does not know. That is accepted for a
local save, and since 2026-11-12 (`REV#5`) nothing is lost by it: the first
write after such a load copies the file beside itself as
`save_data.newer-<n>.json`, through the same never-overwriting copy that keeps
an unreadable save as `save_data.unreadable-<n>.json` (`HYG#22`), and prints
one stderr line naming the copy. It copies once per load, and a failed copy is
reported and the write goes ahead. The legacy path is never written, so it is
never copied. Every successful write also creates the player's `stages`
folder if it is missing (`FTR#14`), silently and best effort; looking the
folder up still creates nothing. `match_records`' `check_newer_save` covers
both writers (`save_to_file` and `remember_match_setup_and_save`), the byte-identical copy,
the second load's second copy, no copy on a later write and the folder, and
`check-startup.ps1` requires the folder in its isolated profile. These stderr
lines get no in-game notice: the author declined one on 2026-11-13 (`REV#5`),
and `HOW-TO-PLAY.txt` names the two copies kept aside in the same folder
instead. Reopen if a player reports lost settings or records. New sections use nested keys: `"records"`, the last-match
setup's `"last_match"` (`GAP#6.1`), `"input_bindings"` (`GAP#4.2`) and
the player profiles' `"profiles"` (`ROAD#21`).

**Records** (`game/save/match_records.h`) are an array of `{ "stage": ... }`
objects, keyed by the manifest name, because the engine's JSON reader has no
member iteration. Each holds versus matches played, wins by side (A/B), draws
and the best winning share, plus practice matches played and the best
coverage. Malformed entries are skipped one by one, counts are kept at 0 or
more, percentages are clamped to 0-100, and a repeated stage keeps its first
entry. Records for stages the manifest no longer lists are kept.

Defaults chosen without an author call, recorded here so they can be revisited,
except stage-only keying, which the author ruled on:
- **Metric:** the winning team's share of painted surface
  (`LevelEndInfo::team_*_percentage`), the number the results bar shows. Solo
  practice (one team) keeps a separate best coverage of the arena.
- **Records are keyed by stage only, not by match duration.** A shorter custom
  duration can set a best. This is the author's ruling of 2026-11-13, made
  when graded solo goals were declined (`REV#25`): records are a stage's tally,
  not a target. Reopen if the author decides solo against bots should have
  goals ([review calls](#project-review-calls-2026-11-13)).
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
`results_menu.record_y` and `stage_select.record_y`, and the lines are
`menu_text.records`' templates (`{percentage}`, `{count}`, `{played}`; words,
order and joins since `HYG#69`). No artwork was added; the later UI art pass may move both.

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
best-of length, match variant and an optional duration override. Omitted duration keeps using
the tuning default; an omitted variant is Standard. Random weapons and stages resolve before capture;
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
  `release-coverage-audit-*.json` receipt. Physical-pad acceptance was
`ROAD#2` (waived 2026-11-05, not performed); these checks use scripted input and establish no performance result.

## Action bindings (`GAP#4.2`)

**Default evaluators and keyboard Jump rebinding are implemented and validated.**
**Every keyboard action and per-slot pad Jump/Shoot/Pause rebinding are implemented (`GAP#4.2`).**
The pad defaults are unchanged (A / LT Jump, RT Shoot, Start Pause), and so are
the keyboard's (A / D, Space, S / Down, left click, Esc). The pad contract is
[below](#pad-buttons-slice-gap42), after the face-button slice it replaced; the
keyboard's is the [keyboard actions slice](#keyboard-actions-slice-gap42). The
author accepted that slice's choices as built on 2026-11-05, so nothing is left.
[Action bindings](../game/input/action_bindings.h) name the existing gameplay actions
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
comfort was `ROAD#2` (waived 2026-11-05, not performed).

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
was `ROAD#2` (waived 2026-11-05, not performed); attended readability, balance and hardware acceptance
were not performed either.

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
`SettingsSession::set_input_bindings_and_save` refuses it before mutation. Movement, aim and
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
Shoot Apply. Physical-pad comfort was `ROAD#2` (waived 2026-11-05, not performed), and rebinding artwork belongs
to the later UI pass.

Keyboard actions other than Jump are the
[keyboard actions slice](#keyboard-actions-slice-gap42) below.

Labrador declined the action-evaluation request (engine report 21, formerly
`GAP#F2`) in `197aff1`: evaluating a binding table is the action map, which its
boundary table gives to the game, and its clients already combine sources by
rules that disagree. The evaluator stays client code; see the
[closed entry](review/engine-reports.md#21-digital-action-evaluation-remains-client-code--declined-197aff1).
Pad Buttons is now a real pad-rebinding caller, but it chose discrete cycling
over capture, so a pad-capture request still has no caller.

### Keyboard actions slice (`GAP#4.2`)

**Contract (2026-10-28).** The keyboard seat's six gameplay actions become
rebindable on one page: Move left, Move right, Jump, Drop, Shoot and Pause.
Mouse aim stays on the mouse, F1 stays the gameplay debug control, and menus,
intro skip and the developer keys keep their fixed keys whatever the player
chooses.

- **Domain.** Every action takes one control. Move left, Move right, Jump and
  Drop take one key from the Keyboard Jump slice's domain (A-Z and Space, the
  same save names and labels). Shoot takes one of those keys or one mouse
  button (Left, Right, Middle, Mouse 4, Mouse 5), because its shipping default
  is the left click and the hand on the mouse is the one that aims. Mouse
  buttons on the other actions are not in this slice.
- **Fixed aliases.** Escape always pauses a match (it is also menu Back and the
  capture's cancel key), so the Pause row is **Pause (also)**: one optional
  extra key, default None. Down always drops beside the chosen Drop key. Neither
  alias is in the domain, so neither can be taken by another action.
- **Defaults** are the controls the game always had: A, D, Space, S (and Down),
  left click, Esc. An untouched keyboard behaves exactly as before.
- **Conflicts.** One control does one action. Capturing a control another row
  already uses is refused with a status, as Keyboard Jump refused A/D/S and Pad
  Buttons skips controls in use; nothing is swapped silently. Jump's refusal
  now follows the pending choices, so A is free for Jump once Move left moves.
- **Reserved keys.** Escape, Enter, Backspace, the arrows, F1-F12 and the grave
  key are outside the domain, so the console, the debug menu, the F-key binds
  and menu navigation cannot be captured. A bare developer bind on a chosen key
  stays a warning, now for every effective key rather than Jump alone.
- **Menus.** Menu navigation is unaffected: W/A/S/D and the arrows still move,
  Enter/Space still select, Esc/Backspace still go back, R still shuffles, and
  the menu prompts keep naming those fixed keys. The extra Pause key opens the
  pause menu from a match; Esc or Back closes it, as before.
- **Page.** Controls / audio's **Keyboard Keys** row (the Keyboard Jump row's
  place) opens the existing child, grown to Move left, Move right, Jump, Drop,
  Shoot, Pause (also), Reset all keys, Apply, Back on the same Controls layout.
  Each action row shows its pending control; activating it runs the existing
  press-and-release capture for that action (Shoot also accepts a click). Reset
  restores every default pending; Apply merges the six choices into the latest
  preferences, keeping pad profiles; Back discards. The capture's ownership,
  neutral, chord, focus and cancel rules are unchanged.
- **Save.** Version 3 stays: `input_bindings` keeps `keyboard_jump` and gains a
  `keyboard` object `{ "move_left": "a", "move_right": "d", "drop": "s",
  "shoot": "mouse_left", "pause": "none" }` (mouse tokens `mouse_left`,
  `mouse_right`, `mouse_middle`, `mouse_x1`, `mouse_x2`). A missing object reads
  the defaults. A damaged object, or one that conflicts with Jump, recovers the
  five to the defaults; if Jump then conflicts, Jump recovers to Space too. Pad
  profiles and every other section survive.
- **What the keyboard seat is shown.** The match-opening keyboard reminder and
  the Controls help name the effective keys (`{left}`, `{right}`, `{drop}`,
  `{shoot}` and `{pause}` join `{jump}` in the help template, each at most
  once). Menu prompts are unchanged because menu keys are.

**The calls, accepted as built (the author, 2026-11-05).** Refuse rather than swap on conflict;
mouse buttons for Shoot only; Escape and Down kept as fixed aliases; one
control per action with no keyboard alternates; the A-Z/Space domain kept
rather than widened to digits, arrows or modifiers.

**Implemented 2026-10-28.** `InputBindings` gains the five keyboard choices
beside `keyboard_jump`, with `KeyboardAction`, a `DesktopControl` (a key or a
mouse button) and per-action domain, name, label and conflict queries in
[input_bindings.h](../game/input/input_bindings.h). `effective_action_bindings` writes
the six into the desktop table, keeping Escape, Down, F1, the intro skip and
every menu key; `PlayerInput` rebuilds its copy only when a keyboard choice
changes, so adopting a held key still makes no edge. `KeyboardJumpCapture` now
targets one action against the pending choices and, for Shoot only, counts mouse
buttons in the same neutral/chord/focus rules. `KeyboardJumpMenu` keeps its
class and widgets and grows to the nine rows above, each action an `OptionRow`
whose activation starts that action's capture. The Controls help, the opening
reminder and the developer-bind warnings name the effective keys. The page text
is `menu_text.keyboard_jump` in `presentation.json`, validated for every label
in every row, every release message and the help with and without the warning.

The game spells the five mouse tokens itself, because `MouseButton` has no
names as `Key` has; that ask is parked in the
[engine-report ledger](review/engine-reports.md#what-is-deliberately-not-here).
One fixed interaction carries over: as everywhere in the menus, the first
direction after a capture needs one neutral frame, so a click released and an
arrow pressed in the same frame moves nothing.

**Checks.** `input_bindings` covers the six domains against explicit token and
label tables, twenty reserved keys, defaults equal to the old table, per-profile
conflicts, the custom and keyed-Shoot mappings, the actual reader (moved keys,
fixed Down/Escape/Space/menu A, held adoption, focus, conflict refusal), the
save schema and round trip, refusal before mutation and recovery for thirteen
damaged objects with and without a clashing Jump. `keyboard_jump_capture` runs
every key for every action against pending choices (142 accepted, 20 refused),
all five Shoot clicks, clicks ignored elsewhere, key-and-click chords, focus and
Escape. `menu_text` pins the copy, the tokens and the mouse labels.
`check-painter.ps1 -KeyboardJumpOnly` drives the real page through Controls,
moving Move left, Shoot (a right click), the extra Pause and Jump onto the freed
A, and checks Apply's merge, the reload and Reset all keys.

**Decided 2026-11-05.** The author accepted the calls above as they stand.
Widening the domain, keyboard alternates and the extra Pause key closing the
pause menu (Esc does that today) are not started and are not planned.

### Mouse clicks in menus (`REV#33`)

**Done 2026-11-12.** The keyboard seat's mouse now works the menus the way its
keys do: a left click selects (Enter, Space, pad A) and a right click goes back
(Escape, Backspace, pad B). The 2026-11-10 project review
([REV#33](review/2026-11-10-project-review.md#mouse-input-in-menus-rev33)) found
that menus read keys only.

- **No pointing.** A click acts on the focused row wherever the cursor is; there
  is no hover focus or hit-testing through the UI camera. That larger half was
  not designed.
- **A click held from play cannot confirm a menu.** Left click is the default
  Shoot and right click the default Sub, so a click held at the buzzer, into
  Pause or into Results would otherwise release onto a row. The buttons are
  fixed menu bindings like the keys (`ActionBindings::menu_mouse_buttons`) and
  pass through `MenuInput`'s armed release: a click fires on release only if
  its press was seen by the menu, so every `reset_actions()` disarms a held
  click as it does a held key. The mouse arms apart from the keys, as each pad
  does, so a click held from play never holds up Enter or Escape released over
  it (the keys win a frame where both release). Releasing both buttons
  together chooses neither, focus return disarms, and a cancelled mouse
  capture (a new button epoch) releases nothing.
- **Where.** The game's menus, the pause and end pages and the replay skip
  take clicks (`GameMenu`, `GameLevel` and the Debug director pass
  `GameData::mouse()`). Keyboard Keys' capture reader does not, so a click
  there is still a Shoot or Sub choice, never Back.
- **Copy.** The prompt bars and the Controls help still name A/Enter and
  B/Esc; with no pointing, a click is a shortcut rather than a control to teach
  there. The README and the how-to-play sheet name it.

`menu_navigation` checks the slice with a real `Mouse`: left proceeds and right
backs only on release, once; Shoot and Sub held across a `reset_actions()` stay
neutral on release; a fresh click then works; both buttons together, a
cancelled capture, a click across focus loss and return, and the middle button
do nothing; a reader built without a mouse ignores clicks; a pad reset keeps the
mouse's armed click; Enter and Escape released over a click held from play act
at once, and that click's later release does nothing. The painter's
`-PracticeReplayOnly` leaf, which pauses and resumes with Enter while the
keyboard seat holds Shoot, is the case that first showed the keys must not
share arming with the mouse.

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
composition. The author's 2026-11-05 `POL#21` call put the scored-slab trial
(`turbulence_slab_trial`, the Yard with movers and fixed scored slabs) into
Random; the other Turbulence comparisons stay explicit-only. Arbitrary moving
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
This finishes `GAP#D5`; arena geometry selection was `POL#21`, decided
2026-11-05 for the slab trial.

The explicit-only `turbulence_mover_trial` now uses the authored pair beside
the preserved static arena. Both-preset route and native composition checks,
Debug seeded public rebuilds and the Release local five-tool comparison are
recorded in the [arena owner](polish-turbulence.md#why-the-new-geometry-earns-its-place).
The concrete added role is a Sniper crossover-top paint angle at all four
sampled phases; the other tools retain different static/moving tradeoffs.

Arbitrary or combined moving/rotating bodies remain outside the implemented
contract. Preserve the existing hierarchy and static arithmetic. The
[Turbulence blockout work](polish-plan.md#turbulence-blockout-work-pol21) owns
useful placements and route/side-swap qualification. The author decided
`POL#21` on 2026-11-05: the slab trial, which carries this pair of movers,
joins Random, and the mover comparison stays explicit-only, out of Random. The
five-tool balance was accepted at its current values the same day (`POL#27`).
Attended arena acceptance was not performed.

## Presentation rate (`GAP#L9`)

**Vsync setting built 2026-11-04; accepted by the author 2026-11-05.** The author asked for a vsync option on 2026-11-04 and chose to
add the switch in Labrador rather than file a report first, as report 28 was
answered. See [Vsync setting](#vsync-setting-gapl9) below. The rest of this
section is the earlier inspection that led there.

**Inspected 2026-10-29 ([slice](#presentation-rate-inspection-slice-gapl9)).**
`SIMULATION_HZ` deliberately fixes gameplay at 60 Hz; changing
`ApplicationOptions::target_fps` changes that step, not just presentation. The
pinned D3D11 backend presents with sync interval 1 and exposes no client vsync
switch. A presentation-rate setting must first identify its engine seam and
supported policy, separately from simulation and any interpolation work.

Inspect the current engine API and master before proposing a client setting or
report. Define its persistence and visible acceptance on the intended refresh
rates before implementation; this row does not supply a ready engine change.

### Vsync setting (`GAP#L9`)

**Engine.** Labrador's `Renderer::set_vsync(bool)` and
`ApplicationOptions::vsync` (labrador#106, merged 2026-11-04) present with a sync interval of 0 on both
Direct3D backends when vsync is off, and with no tearing flag, so a windowed
game is still composed rather than torn. The pin is that merge on master, and
moves to the next tag that carries it.

**Game.** Options gains a `Vsync` On/Off row after Fullscreen
(`menu_text.items.vsync`, the forty-sixth item). It is saved with the other
display settings as the flat `"vsync"` key (save version 6; older saves read
as On). Apply hands it to the renderer straight away, without the display
trial, which is kept for resolution and window mode, and `main.cpp` sets it
before the first frame. To fit eight rows, Options' rows draw in their own
font, `main_menu.fonts.options` (`lato_black_36`), with
`options_spacing.y` at `82`. The other pages keep `lato_black_48`.

**What Off does here.** Gameplay still steps at `simulation::simulation_hz`, and nothing
reads `Application::step_position()`. With vsync off the engine draws as fast
as it can, and every frame between two steps shows the same picture. So Off
lowers input-to-display latency and costs GPU time. It does not make motion
smoother on a high-refresh display. Interpolation would be its own row.

**Accepted 2026-11-05.** The author accepted the setting as built. The check
on MattPC that was planned (Off on the 59.951 Hz display, both windowed and
fullscreen, against an `overlay.perf` capture: draws/s well above 60 and no
tearing in a window) was not performed. The painter's Options fixtures
describe the eight rows, and they and their leaves passed on MattPC in both
presets on 2026-11-04.

### Presentation-rate inspection slice (`GAP#L9`)

**Recorded and deferred (2026-10-29).** The inspection the boundary above
asks for, against the pin (v0.2.0, `1e60fb2`) and Labrador master (filed at
`b96bad6`, re-checked at `8e91aab`; none of the 22 commits between them touch
the cited lines):

- D3D11 calls `Present(1, 0)` (`engine/render/d3d11/device_resources.cpp:395`
  on both) and reports `requested_swap_interval = 1` (`d3d11/renderer.cpp:70` at
  the pin, `:50` on master). D3D12 is the same
  (`d3d12/device_resources.cpp:511` on both). Vulkan sets
  `VK_PRESENT_MODE_FIFO_KHR` only (`vulkan/device_resources.cpp:1157` at the
  pin, `:1178` on master).
- `ApplicationOptions` has `target_fps` (`engine/app/application.h:66` at the
  pin, `:69` on master) and no vsync or present-interval option on either.
- The game sets `target_fps = SIMULATION_HZ` (`game/main.cpp:52`), and nothing
  in `game/` or `scripts/` calls `Application::step_position()`
  (`application.h:295` at the pin, `:291` on master), so the client does no
  interpolation.

**Receipt.** One Debug `overlay.perf` run (`game/debug/commands/view_commands.cpp:55`)
on MattPC: visible 1920×1080 window, D3D11, host load not controlled,
`+seed 7 +stage movement_course +players 4 +bot all paint`, then `perf` and a
600-interval `perf.capture`. Windows reports the display as 59 Hz, which is
an integer truncation: `QueryDisplayConfig` gives 59951/1000 Hz on three paths
and 60/1 on one. The capture is complete and every row is engine-paired:

| Draws/s | p50 ms | p95 ms | p99 ms | Max ms | Updates | x2 frames |
|---:|---:|---:|---:|---:|---:|---:|
| 59.236 | 16.665 | 16.887 | 33.500 | 33.788 | 613 | 10 |

The ten ~33.5 ms intervals fall every 59 rendered frames (0-based rows 61
to 592, CSV `frame` 62 to 593),
each carrying two updates, while their `end` phase (9–11 ms) matches normal
frames, so the extra time is outside the measured Present-wait phase. A
59.951 Hz/60 Hz beat alone would drop a frame about every 20 s, not every
second; this run does not diagnose the cause. Row 0 (five updates) and row 2
(none) follow the capture start. The receipt, CSV/JSON, log and overlay
screenshot are under `out/validation/gapl9/`. CPU wall time, uncontrolled
load: it qualifies no visible pacing, which was
[`POL#25`](polish-frame-pacing.md#remaining-acceptance)'s to qualify (waived
2026-11-05, not performed).

**Deferred until** the author wants presentation decoupled from the 60 Hz step
(a high-refresh, uncapped or vsync-off mode), or attended visible captures
(`POL#25`'s, waived 2026-11-05) show the once-a-second long interval on an uncontended run. Either
way, the next step is an engine report for a present-interval option and,
for a decoupled rate, a client `step_position()` consumer; no client setting
precedes it. This chooses no refresh-rate policy, setting or persistence.
