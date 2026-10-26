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
Debug bots remain developer tools; shipping AI opponents is outside the chosen
scope. The accepted solo story treatment is unchanged: no person, opening or
reveal, as recorded in the [backstory](backstory.md#building-it). These decisions
close the ruleset and shipped-AI questions raised by the frozen gap audit.

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

**ROAD#5 — DEFERRED**, split from the remaining `GAP#1.5` scope, until the
physical multiplayer sessions (`ROAD#2`) identify a skill imbalance that needs
a handicap. Those observations must establish the
need and the mechanic before implementation is scoped. No per-player paint or
score multiplier has been chosen; the current score remains painted surface.
`ROAD#5` tracks this deferred work separately from the random-team action.

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

Identity is bound to an XInput slot for the match. A controller returning in a
different slot requires quitting and joining again. Physical-device identity
and reassignment during a match are follow-up work if this proves inconvenient.

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

**DEFERRED** until after the coupled `POL#27` pass. View shake, the aim marker
and Reduce effects are single `PlaySettings` fields that apply to every pane,
while stick deadzones are already per slot (`StickSettings`). On a shared screen
a motion-sensitive player cannot opt out alone. The shape is the stick-profile
pattern: per-slot copies with the global value as the default, edited from the
same per-pad page. Before scoping, decide whether Reduce effects is per pane
(view shake, flashes) or global (shared overlays, the resume signal).

### More team palette pairs (`ROAD#8`)

**DEFERRED** to the later UI art pass. Three pairs ship. Adding one is a content
edit to the [team palette block](#team-palette-content-gap22-2026-10-15), and
each extra pair widens the colour-vision route around the random roll. Making
one a fixed Options choice still needs an enum entry and save key. Check each
candidate pair against the arena art under deutan, protan and tritan simulation
before shipping it.

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

No rows. These are known limits, recorded so a later gap list does not file
them again.

- **XInput only.** DualShock, DualSense and Switch Pro pads work only through
  Steam Input or a wrapper. The engine's reader seam is one function wide
  (`engine/input/gamepad_reader.h`), so a GameInput backend is Labrador work and
  would need an exhibit before it is a report. `ROAD#2` observes the limit
  (item 6 above).
- **One keyboard seat.** That seat aims with the mouse, so a second keyboard
  player needs a keyboard-only aim model. That is a design question, not a
  missing binding. It gets a row only if the author asks for two players on one
  keyboard.

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
**Keyboard and per-slot Pad Jump selection are implemented and qualified (`GAP#4.2`).**
The face button defaults to A; the existing LT alias remains alongside every
choice. Broader action and trigger rebinding remain unscoped; the bounded contract is
[below](#pad-jump-face-button-slice-gap42).
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

**Remaining scope after this slice: UNSCOPED.** Other keyboard actions, pad
actions and trigger choices still need separate contracts. Rebinding artwork
belongs to the later UI pass using the approved gameplay style. Neither the
completed keyboard slice nor this bounded pad slice closes the broader
`GAP#4.2` capability.

Labrador declined the action-evaluation request (engine report 21, formerly
`GAP#F2`) in `197aff1`: evaluating a binding table is the action map, which its
boundary table gives to the game, and its clients already combine sources by
rules that disagree. The evaluator stays client code; see the
[closed entry](review/engine-reports.md#21-digital-action-evaluation-remains-client-code--declined-197aff1).
A pad-capture request still needs a real pad-rebinding caller.

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

`GAP#D5` is **OPEN** with the Turbulence blockout (`POL#21`, 2026-10-25). Its
brief, offset platforms and crossing routes that reward repositioning, is the
first layout where a moving platform earns its place. Add rotated and moving
structures behind the builder with a useful arena placement for each, and
collision, support, paint and editor acceptance for that layout. Moving support
needs the player to ride it, paint on it to stay owned by its face, and replays
to stay deterministic. Choose placements during blockout rather than altering
the geometry under style review; Crossfeed and Press Tower stay static.

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
