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
**Remaining scope: UNSCOPED (`GAP#4.2`).** Broader action and Pad 1-4 rebinding
need their own bounded slices, as described below.
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
and private posted F10 hook messages. Actual OS focus delivery is outside that
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

**Remaining scope.** Broader action and Pad 1-4 rebinding still need their own
bounded slices. Future profiles follow the existing XInput-slot preferences,
not player numbers or physical-device identity. Pad movement, aim and drop
remain analog reader behavior. Rebinding artwork belongs to the later UI pass
using the approved gameplay style. Physical-pad comfort remains `ROAD#2`, and
human readability, comfort, balance and hardware acceptance stay open. This
keyboard slice does not close `GAP#4.2`.

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
