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
   resumed weapon loops and the multiplayer mix. The ZIP now includes the
   replacement bank; qualify actual playback from the extracted release and the
   missing-bank silent fallback separately.
5. Relaunch and confirm saved settings; record device models, display layout,
   build revision and observed failures before choosing the next feature increment.

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
`"save_version": 1` is written first. A missing key is version 0, the same flat
layout. A higher version still loads every key this build knows and prints one
stderr line; the next write drops what it does not know. That is accepted for a
local save. New sections use nested keys: `"records"` now, and the last-match
setup (`GAP#6.1`) adds its own the same way.

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

**OPEN.** Add a nested last-match `SaveSection`, using the existing sectioned
save. Save the resolved choices of a normally started match: player count,
teams, weapons, stage, layout, duration override and best-of length. Random
weapons and stages have already resolved at this point; remembering Random
intent would be separate work.

Seed the setup pages from those preferences on launch. Every device still joins
explicitly and every player confirms again. Do not persist device claims,
readiness, the active round, series scores or match progress. Keep the current
menu path and allow every remembered choice to be changed.
Returning from a match keeps the current in-memory choices. Developer
script/bot/replay launches do not overwrite the remembered user setup.

Acceptance: round-trip and relaunch restore the choices; old, missing or malformed
sections recover without losing display/play preferences or records. Removed
stages, unsupported weapons and invalid solo/series combinations fall back to
valid choices. A failed write preserves the prior file and the active setup.
Driven menu checks cover fresh claims, confirmation, changed choices and restart.

## Action bindings (`GAP#4.2`)

**OPEN; first slice:** introduce client-owned actions and binding evaluators for
the existing gameplay and menu controls, retaining today's default mappings.
Evaluate keyboard, mouse and pad snapshots with the same held, press and release
semantics, trigger thresholds, stick response and disconnected-device behavior.
Menu arming, neutral gates and direction repeat keep their current ownership.

Acceptance: compare default action output with the existing readers, including
opposing/aliased keys, thresholds, held entry, reconnect, focus return and
simultaneous inputs. Real-page checks retain joining, selection, pause and
confirmation behavior. Synthetic devices establish mapping behavior; physical-pad
comfort remains `ROAD#2`.

Persistence and the rebinding/capture page follow that working client seam; the
first slice alone does not close `GAP#4.2`. Use its concrete input requirements
as the exhibit for `GAP#F2`, checking engine master before filing a report.
Rebinding artwork follows the gameplay style lock; its implementation does not
gate the default evaluator. Upstream filing follows the
[publication decision](review/engine-reports.md#publication-gate-engpublish).

## Per-player menu focus (`GAP#D1`)

**OPEN; Team Select first.** Try the engine's per-slot `FocusGroup` cursors on
the existing three choices per player. Preserve each seat's row, the current
Left/Right limits, team requirements, shuffle, readiness, cancellation and choice
motion. Establish that shared candidate selection and derived focus painting can
retain those rules before extracting a shared page base.

Acceptance: drive one to four seats, simultaneous choices/confirmation/back,
shuffle and held inputs, missing devices and return from Weapon Select. Compare
the existing page's choices, readiness, appearance and RNG consumption. If the
API cannot keep row ownership or readiness painting, retain the working page
and draft a concrete local engine exhibit. Evaluate Weapon Select only after this
fit; its single cycled icon is a different navigation case.

## Other demonstration gaps

`GAP#D2` is **DEFERRED** until an accepted page requires a widget or page
abstraction beyond the existing `OptionRow`; a list of unused toggle, slider,
progress or modal types is not an implementation task.

`GAP#D5` is **DEFERRED** to the `POL#17`/`POL#21` blockouts. Add rotated and
moving structures behind the builder with a useful arena placement for each,
and collision, support, paint and editor acceptance for that layout. Choose
their placement during blockout rather than altering the geometry under style review.

## Presentation rate (`GAP#L9`)

**UNSCOPED.** `SIMULATION_HZ` deliberately fixes gameplay at 60 Hz; changing
`ApplicationOptions::target_fps` changes that step, not just presentation. The
pinned D3D11 backend presents with sync interval 1 and exposes no client vsync
switch. A presentation-rate setting must first identify its engine seam and
supported policy, separately from simulation and any interpolation work.

Inspect the current engine API and master before proposing a client setting or
report. Define its persistence and visible acceptance on the intended refresh
rates before implementation; this row does not supply a ready engine change.
