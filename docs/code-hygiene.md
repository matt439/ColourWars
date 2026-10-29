# Code hygiene and maintenance

This is the live owner for `HYG` rows in [TODO](../TODO.md). The author asked
for source-file splits and comment audits on 2026-10-28. These are independent
maintenance slices alongside the gameplay backlog; they do not change its
arena/tuning order. Since 2026-10-29 it also owns the CI-gate rows `CI#2` to
`CI#5`; `CI#1`, the foreground debug-console check, stays with
[the debug console](debug-console.md#foreground-checks-ci1).

## Open shortlist (2026-10-29)

The author approved adding every proposal from the 2026-10-29 verified
brainstorm to the index. Each entry below is **open, not started**. Evidence
was re-read against the tree on that date; refresh line numbers before
starting. None of them closes a human gate, and any listening, feel, art or
attended-play judgment it touches stays a `DECISION` row.

### Unreadable save preservation (`HYG#22`)

**Done 2026-10-29.** `Save::load_save_data` now hands a save that fails as a
whole to `keep_unreadable_save` ([save.cpp](../game/save.cpp)) before keeping
the defaults. It copies the file beside itself as
`save_data.unreadable-<n>.json`, taking the first free `n` (up to 999), through
`std::filesystem::copy_file` with no overwrite, so an earlier copy is never
replaced. It prints one stderr line naming the save, the reader's error and the
copy; a failed copy prints its own line instead and changes nothing else. The
missing-file and legacy paths are unchanged. `match_records`'
`check_unreadable_save` loads a truncated and an invalid save in turn. It
checks that each leaves the defaults and the file untouched, prints exactly one
line naming both paths, and leaves a byte-identical copy; that the second does
not touch the first copy; and that a later `save_to_file` writes the real path
as version 5 and leaves both copies. The check failed against the old `Save`
before the change. The roadmap's save-layout record now says version 5 and
lists versions 4 and 5 and the nested `profiles` section. Both presets pass the
default acceptance suite. Still not decided: an in-game notice for a reset save,
and whether copies are ever pruned.

The record as filed:

A save that exists but will not parse is silently
replaced. `Save::load_save_data` catches the whole-file failure and keeps
`SaveData()` with no message ([save.cpp](../game/save.cpp) lines 433-441), and
the next match start writes that default state over the file:
`GameLevel` calls `remember_match_setup_and_save` on every match start
([game_states.cpp](../game/states/game_states.cpp) lines 237-239), which always
writes. A recorded round writes again (`record_result_and_save`, save.cpp
lines 568-579). The engine's `read_json_file` throws on open, read and parse
failures alike (`external/labrador/engine/assets/json.cpp` lines 66-89). The
existing protection is per section only: `load_from_json` resets one malformed
section (save.cpp lines 367-379), which the `match_records` case's
`check_section_isolation` covers; nothing covers a file that fails as a whole.

Scope: when an existing save fails to load, copy it beside itself under a name
that is never reused or overwritten, best effort, then print one stderr line
naming both paths before keeping defaults. A failed copy prints its own line
and changes nothing else. The missing-file and legacy-save paths are
unchanged. In the same change, correct the save-layout record in
[the roadmap](feature-roadmap.md#stage-records-and-the-sectioned-save-gap62),
which still says `"save_version": 3` and lists versions up to 3, against
[save.h](../game/save.h) lines 33-44: version 5, versions 4 (`"profiles"`) and
5 (`"last_match.stage_rule"`) listed, `profiles` nested, and the display and
play preferences flat.

First step: add the failing case to an isolated-save acceptance case
(`match_records` or `settings`) before touching `Save`.

Acceptance, CPU only: an invalid-JSON save and a truncated one each leave
defaults, one stderr line, and a byte-identical side copy; a
second corrupt load does not overwrite the first copy; a later
`save_to_file` writes the real path and leaves the copy. Both presets pass the
default acceptance suite.

Not decided here: any in-game notice or dialog for a reset save, and whether
side copies are ever pruned.

### Participant parameter names in `Level` (`HYG#26`)

**Done (2026-10-29).** `HYG#8` corrected the comments about participants and
panes but no identifiers (see [its record](#source-comment-contract-audit-hyg8)).
This commit renamed the identifiers, and nothing else: every [level.h](../game/objects/level.h)
parameter that indexes a roster participant in `players_` is now `participant`
in declaration and definition: `player`, `view_camera`, `idle_drip`,
`aim_marker_visible`, `player_identity_visible`, `player_identity_emphasis`,
`player_identity_bounds` and their `_for` forms, and `place_player`,
`respawn_player` and `kill_player`. The comment over the developer-tools block
lost its "`view` here indexes roster participants" caveat. Three loop and
buffer locals that walked `players_` under the name `view` (the death snapshot
in `update_level_logic`, `refresh_identity_texts` and `apply_player_inputs`'s
`by_view`) became `participant`/`by_participant`; a review follow-up renamed
the fourth, the figure-act loop in `level_intro.cpp`'s `update_acting`.

`view` is kept where it really is a human pane: `participant_for_view`,
`teammate_markers`, `hud_scale`, `update_teammate_markers`' loop and the pane
local in `refresh_player_cameras`. `draw_acting`'s unnamed `view` is the
Scene's view index and stays. `view_camera` keeps its name: it is the
cosmetic camera for that participant's pane, and its public name was out of
scope.

Verified: the diff changes identifiers and one comment only; both presets
build under `/W4 /WX`; the review follow-up ran the default acceptance suite,
which passes in Debug (73 cases) and Release. Release's `moving_structure`
first failed on a temp folder left by an earlier run with the same process id
and passed on rerun.

Not decided here: any rename outside `Level`, or of the public accessors
`view_count`, `participant_for_view` and `view_for_participant`.

### Stale README and AGENTS structure facts (`HYG#27`)

**Done (2026-10-29).** README now gives `game/` as ~69k lines with ~16k in
`game/debug/` and `editor/` as ~5.5k (tracked `.cpp`/`.h`, blank lines and
comments included, rounded), and lists the gate and ferry Turbulence Trials as
explicit-only. AGENTS.md, with the author's approval, names the three audio
backends and the `xinput`/`web` input axis and the Windows defaults this
repository gets. The record below is the finding as filed.

Two structural descriptions had drifted.
[README.md](../README.md) lines 176-177 give `game/` as ~34k lines with ~14k of
Debug tools and `editor/` as ~2k; tracked `.cpp`/`.h` files measured on
2026-10-29 are about 69k in `game/`, 15.7k in `game/debug/` and 5.5k in
`editor/`, counted with blank lines and comments. README lines 35-38 list the
explicit-only stages but omit the Turbulence Ferry and Gate Trials, both
`"include_in_random": false`
([ferry](../game/content/levels/turbulence_ferry_trial.json) line 543,
[gate](../game/content/levels/turbulence_gate_trial.json) line 507).
[AGENTS.md](../AGENTS.md) says the engine has two audio backends, but the
pinned engine offers `xaudio2`, `webaudio` and `null`
(`external/labrador/engine/CMakeLists.txt` line 325), and a separate
`LABRADOR_INPUT_BACKEND` axis, `xinput` or `web` (line 368), which AGENTS.md
does not mention.

Scope: correct those README lines and the AGENTS.md backend paragraph to what
the pinned engine and the tree say, without new prose about the web build.
Keep the counting rule stated or rounded so the figures do not need a refresh
for every slice. The AGENTS.md edit is project instruction text, so the author
approves it at commit time; the README edit can land without it.

First step: recount with the same rule as the source-size snapshot below and
re-read the backend `STRINGS` lines at the current pin.

Acceptance: each corrected figure and list matches a command recorded in the
commit message; the CI change-detection checks still classify the commit as
docs-only.

Not decided here: the AGENTS.md wording (the author's), and whether README
keeps line counts at all.

### Compile gate for opt-in probes (`CI#2`)

**Open, not started.** CI compiles only what it runs. The fourteen opt-in
acceptance cases (`bot_trials` through `turbulence_slab_tools` in
`$availableCases`, [run-acceptance.ps1](../scripts/run-acceptance.ps1) line 22)
are outside the default list (line 33), and the CI step passes no `-Cases`
([ci.yml](../.github/workflows/ci.yml) line 135); the workflow has no schedule
(lines 6-10). `check-painter.ps1` and `check-frame-pacing.ps1` are not in CI
at all. The painter has no `-CompileOnly` switch, while its compile already
splits on `COLOURWARS_PAINTER_SCRIPTED_PADS`
([check-painter.ps1](../scripts/check-painter.ps1) lines 392-407); frame
pacing already has one (`check-frame-pacing.ps1` lines 6 and 194). Probes kept
off CI drift: the 2026-10-28 painter run found stale fixtures nobody had
noticed (the MattPC paragraph under
[Player profiles](feature-roadmap.md#player-profiles-road21)). Those were runtime
failures; this gate catches only the compile-breaking kind.

Scope: a CI step, in both presets, after the existing build, that runs
`run-acceptance.ps1 -SkipBuild -CompileOnly -Cases` over the fourteen opt-in
cases, a new `check-painter.ps1 -CompileOnly` in both reader variants (normal
and scripted pads), and `check-frame-pacing.ps1 -CompileOnly`. No probe
executes and no GPU or window is used. `balance_runner` stays Debug-only.

First step: add `-CompileOnly` to `check-painter.ps1`, stopping after the
compiler returns, and qualify it in the existing painter selection checker.

Acceptance: locally in both presets, the new step compiles all sixteen
targets and fails when one opt-in source is broken in a scratch copy; the
selection checkers pass in PowerShell 5.1 and pwsh. Hosted CI stays
unconfirmed until the author authorizes a push.

Not decided here: running any opt-in case in CI, a scheduled workflow, or a
GPU runner.

### Registration and failure-reporting checks in CI (`CI#3`)

**Open, not started.** `check-stage-registration.ps1` and
`check-failure-reporting.ps1` both take `-SkipBuild` but neither is in
[ci.yml](../.github/workflows/ci.yml). Their owners document only local runs
([level-editor.md](level-editor.md) lines 1438-1439,
[failure-reporting.md](failure-reporting.md#verification) lines 81-82), and
no doc says why they are left out; `CI#1` is the only recorded exclusion.
Hosted focus has surprised local assumptions before
([level-editor.md](level-editor.md) line 434, runs 290 and 291).

Scope: run each with `-SkipBuild` in both presets and record duration, any
focus or foreground dependence, and whether every dialog is dismissed without
input. Then either add CI steps that upload their JSON/log evidence, or write
the exclusion reason into each owner doc.

First step: the four local timed runs, with receipts under
`out/validation/`.

Acceptance: the measurements are recorded here; then either the steps exist
with uploads and pass locally, or both owner docs state their exclusion.
Hosted results wait for an authorized push.

Not decided here: an attended or foreground runner model (that is `CI#1`).

### Acceptance timing summary (`CI#4`)

**Open, not started.** `run-acceptance.ps1` measures each case only as a
combined `Seconds` it prints (lines 322-333), and its `$longestFirst`
scheduling order says it comes "from CI timings" (lines 337-341) without
recorded data. The Debug pull-request exclusions rest on prose figures
([ci.yml](../.github/workflows/ci.yml) lines 128-131), and CI uploads nothing
from the acceptance run.

Scope: write `acceptance-summary.json` beside the run's evidence with each
case's compile seconds, run seconds and outcome, sorted longest first, and
upload it from both CI jobs. No change to case selection or order in this
row.

First step: time compile and run separately inside the per-case script block,
then emit the summary after the report loop, including on failure.

Acceptance: a local `-Parallel 4 -Cases` run and a `-CompileOnly` run each
produce a valid summary with every selected case; the selection and process
checkers pass in both shells.

Not decided here: re-ordering `$longestFirst` or changing the Debug
pull-request exclusions; those follow from the data in a later change.

### Local Markdown link check (`CI#5`)

**Done 2026-10-29.** [check_doc_links.py](../scripts/ci/check_doc_links.py)
runs in the always-on lint job after change selection. It reads tracked
Markdown outside `external/` and checks inline links, images and reference
definitions outside code. Each target must reach a file, and an anchor into a
Markdown file must reach a GitHub heading slug (duplicates numbered) or an
`<a id>`/`<a name>`. It allows targets under `out/`, `#` inside an existing
file name, external schemes, and missing files named by a document that opens
a line with "Historical" or "Frozen" in its first twelve. That rule matches
the four retired workshop studies and nothing else; a passing mention of
history is not a declaration. Those records' anchors into documents that still
exist are still checked, which is how `polish-refine.md` line 131 was caught.
Its first run on the tree found exactly the two stale `#retained-route-trials`
anchors. Both now point at `#route-trials-2026-10-20-provisional-profile`, and
the tree's 1,352 local links, 433 with anchors, resolve.
[test_check_doc_links.py](../scripts/ci/test_check_doc_links.py) covers slugs,
each allowance, a broken-anchor fixture, missing files, code spans and fences,
the historical rule and the `external/` exclusion. All 24 tests in
`scripts/ci` pass, and change selection is untouched, so a docs-only change
still skips the Windows jobs. A hosted run waits for an authorised push.

The record as filed: nothing checked relative links. `scripts/ci/` holds only
`lint.py` and its tests, run by the always-on lint job
([ci.yml](../.github/workflows/ci.yml) lines 21-36). A survey of tracked
Markdown outside the engine on 2026-10-29 found nine unresolved local targets,
two of them real: [polish-foundation.md](polish-foundation.md) line 94 and
[polish-refine.md](polish-refine.md) line 131 link
`polish-movement-course.md#retained-route-trials`, a heading now titled
"Route trials (2026-10-20 provisional profile)" (line 208). The rest are
evidence under `out/`, sources a historical document says were retired
(`polish-style-kit.md`), and files with `#` in their names
(`game/audio/paint_exe/PROVENANCE.md`).

Scope: `scripts/ci/check_doc_links.py` with unit tests, run in the lint job
over tracked Markdown outside `external/`. It resolves files and GitHub
heading anchors; it allows targets under `out/`, documents that declare
themselves historical, and `#` in file names. Repoint the two anchors in the
same change.

First step: the checker and tests against fixtures, then a clean run on the
tree.

Acceptance: `python3 -m unittest discover -s scripts/ci -p test_*.py` passes,
including a broken-anchor fixture; the checker reports nothing on the tree
after the two repoints; a docs-only change still skips the Windows jobs.

Not decided here: checking external URLs, or links inside `external/labrador`.

## Startup-check evidence paths (`HYG#21`)

**Done (2026-10-28).** `check-startup.ps1` and
`check-failure-reporting.ps1` now resolve explicit output paths from the
caller's PowerShell filesystem location. These finish the thirteen checkers
identified in the output-path audit. Default roots, run hierarchies and each
checker's initialization order remain unchanged.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Both startup-related checkers still call `Path.GetFullPath(OutputDirectory)`. | Selected as `HYG#21`: finish the established caller-relative contract with two imports and two substitutions. Private preflight copies can qualify paths without starting a game. |
| 2 | The pending editor alignment feature has an unresolved Release font-retirement assertion. | Continue diagnosis separately; diagnostic passes do not clear its original qualification gate. |

Exact private copies reproduced both old scripts placing relative output
under the process directory. Startup used an inert executable and a synthetic
`Add-Type` failure after output creation, before its native helper loaded.
Failure-reporting really compiled/loaded its existing C# helper, then stopped
at an absent private build cache before any helper method or game launch.
That helper has no static constructor or field initializer. Receipts record
the real compilation and its private temporary files explicitly.

Fresh PS5 and pwsh candidate hosts each pass eight cases: relative
Unicode/space paths, absolute/default controls and non-filesystem refusal.
Receipt/file boundaries, three source hashes, private input bytes and caller
environment/location/stack are preserved. No CMake, C++ compiler, Git/CIM query
or game/native probe ran. Independent review confirms exactly two imports
and two output-expression substitutions, with all remaining script text
unchanged. `run-balance` keeps its existing repository-relative policy.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`hyg21-baseline/fd0ff88b0be949c38f882adc07956bec`, candidate PS5
`hyg21-candidate/c27cf4be40c04ee3be3143a8cddf6fa6` and pwsh
`hyg21-candidate/d9f5140ccb814165b85ac840fd0b8f9b`.

## Tool-runner evidence paths (`HYG#20`)

**Done (2026-10-28).** Continued the established caller-relative path contract
for `check-acceptance-process.ps1`, `check-stage-registration.ps1` and
`check-debug-console.ps1`. These three converted explicit output paths
with process-relative `Path.GetFullPath`.

The current ranked scope is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Three remaining runners have private preflight stops before native helpers, compilers or UI/process work. | Selected as `HYG#20`: reuse the existing filesystem resolver, preserving defaults, run layout and failure receipts. Three imports and three substitutions; independent of editor DPI diagnosis. |
| 2 | Startup has another caller-relative output conversion and can stop at a private `Add-Type` fault after output creation. | A separate bounded follow-up with an inert executable and explicit fault boundary. |
| 3 | Failure-reporting compiles a C# interop helper before output resolution. | Qualify its own initialization order separately; disclose helper compilation rather than claiming a compiler-free check. |

The scripts use `Resolve-CheckFileSystemPath` for their output values, retaining
acceptance-process's GUID root, registration's preset-bearing default but
direct explicit root, and debug-console's flat output directory. Defaults and
preflight order stay intact; `run-balance` keeps its repository-relative rule.

Exact private copies reproduced misplaced relative output in all three old
scripts before implementation. Fresh PS5 and pwsh candidate hosts each pass
12 cases: relative Unicode/space paths, absolute/default controls and explicit
non-filesystem refusal. An absent compiler stops acceptance-process; empty
PATH plus `-SkipBuild` stops registration at its required-CMake check; an inert
executable and synthetic throwing debug driver stop before the real helper,
tuning reads, profile cleanup or process/UI work. Exact receipt/file boundaries,
five source hashes, private input bytes and caller environment/location/stack
are preserved. No Git/CIM, CMake/compiler, native helper or probe executable
ran. The existing denied queries remain stopped.

Independent review confirms exactly three imports and three output-expression
substitutions, with all remaining script text unchanged. Evidence under
`out/validation/implementation-loop-20261028/`: baseline
`hyg20-baseline/683fad45e43b467dabbe2b71a77b896e`, candidate PS5
`hyg20-candidate/f8ae203df8ea451ba8a739c205730a63` and pwsh
`hyg20-candidate/b71d92d89f9148f5944fe9011a2c2861`.

## Probe-wrapper evidence paths (`HYG#19`)

**Done (2026-10-28).** Extended `HYG#17`'s caller-relative filesystem contract
to `check-movement-feedback.ps1`, `check-paint-fx.ps1`,
`check-display-awake.ps1` and `check-frame-pacing.ps1`. Their explicit
`OutputDirectory` values used `Path.GetFullPath`, so distinct PowerShell
and process directories placed evidence under the wrong caller directory.

The ranked follow-up shortlist is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Four build/probe wrappers retain the same process-relative output conversion. Each can stop at missing-tool preflight before Git, compiler or native work. | Selected as `HYG#19`: reuse the existing filesystem resolver, preserving defaults, preset/GUID suffixes, preflight order and normal receipts. Four imports and four substitutions; low risk. |
| 2 | Five other wrappers had the same conversion, but different startup/helper boundaries. | Three are completed by `HYG#20`; startup and failure-reporting retain separate initialization requirements. |
| 3 | Vertical lifts (`ROAD#28`) would add a new gameplay affordance. | Current loader, carry, projectile, shadow, bot and replay paths explicitly model horizontal motion. Define the coordinated physics contract before promoting that larger capability; an axis toggle is insufficient. |

Each wrapper now dot-sources `check-path.ps1` and resolves its output value
through `Resolve-CheckFileSystemPath`. Existing default roots, run hierarchies,
options and preflight order remain unchanged. `run-balance` retains its
documented repository-relative policy.

Exact private copies reproduced misplaced relative output in all four old
scripts. Fresh PowerShell 5.1 and pwsh candidate hosts each pass 16 cases:
relative Unicode/space paths, absolute and default-root controls, and explicit
non-filesystem refusal for every wrapper. Empty private PATH stops each
runner at missing-CMake preflight, before Git, metadata/CIM, compilation,
native helpers or GPU work. Literal roots, preset/GUID layout, existing failed
receipt boundaries, five source hashes, private input files and caller
environment/location/stack remain correct. Independent review passes. The
denied worktree-status and global process queries remain stopped.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`hyg19-baseline/15f709722f8940a798475e02c95cca95`; candidate PS5
`hyg19-candidate/8efa89e5510c42acbea157d5dab619de` and pwsh
`hyg19-candidate/59468bc27ab042829558f504b1b3e9a9`. This qualification
establishes path/preflight behavior, with no new runtime or hardware claim.

## Painter initialization cleanup (`HYG#18`)

**Done (2026-10-28).** The first `HYG#17` native attempt exposed a separate
cleanup defect: painter pushes the repository location, then constructs its
evidence record before entering the `try` that restores the caller. An
initialization error therefore leaves PowerShell in the repository. The outer
qualification wrapper restored its own caller, but the runner did not.

The current bounded shortlist is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | The retained failed native receipt reports `checker_caller_location_restored=false` after evidence initialization threw. | Selected as `HYG#18`: guard only initialization with a catch that pops the runner's location and rethrows the original error. Small failure-cleanup change; preserve normal cleanup and failed-initialization receipt semantics. |
| 2 | Nine other validation wrappers still use process-relative explicit output paths. | Extend the established filesystem resolver in a later bounded scope, after tracing each wrapper's path base and preflight order. |

The evidence-record load and Debug rumble supplement now have a narrow
`try`/`catch`. Failure restores the single pushed location and rethrows the
original error. Successful initialization continues into the existing normal
`try`/`finally`, which still owns environment/process cleanup and its own
single pop. Failed initialization still writes no incomplete result record.
Git queries, settings, renderer behavior and public options are unchanged.

An ignored private fault-injection harness reproduced the old-source location
and extra-stack-frame leak. Fresh PowerShell 5.1 and pwsh candidate hosts pass
both initialization-failure and normal-cleanup controls. Exact private copies
of the runner use a synthetic evidence helper that throws before metadata
collection; local dummy tool commands throw if invoked. Original exception
identity/type/id/message, caller location/stack, process directory, environment,
private inputs and five source hashes are preserved. The successful synthetic
helper followed by a missing private cache retains the existing failed receipt
and single restoration. No Git, compiler, CMake, native helper or GPU runs.
The actual denied worktree-status query remains stopped.

Independent review and all 145 focused painter-selection cases pass in each
host. Evidence under `out/validation/implementation-loop-20261028/`: baseline
`hyg18-baseline/aa54ae3c8ed84152b9f27272fdb89d1b`; candidate PS5
`hyg18-candidate/0800c8425bd34f24ae33b8c4152aeb30` and pwsh
`hyg18-candidate/46237d47052d4f00a4a94b90d6f92fca`; selection checks
`hyg18-selection/ps5/106c335c33c545169e4430b30f0034af` and
`hyg18-selection/pwsh/83fe48e2719b4be3897fb2ed46f6d16f`.

## Caller-relative validation paths (`HYG#17`)

**Done (2026-10-28).** Fresh source review found that explicit relative
`OutputDirectory` values used `Path.GetFullPath`, whose base could differ from
PowerShell's location after `Set-Location`. Painter `TrialPath` had the same
issue. Existing selection checks aligned the two locations or passed absolute
paths, hiding the discrepancy.

This bounded shortlist was recorded before implementation:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Painter and acceptance input/evidence paths can resolve beneath the process directory instead of the caller's PowerShell location. | Selected as `HYG#17`: define caller-relative filesystem paths for these entry points and their selection checkers. Private preflight can prove location, quoting and hashes without a compiler or native probe. |
| 2 | Starting a left drag during a middle-button pan leaves both gestures active; releasing the middle button cancels the left drag. | A bounded editor usability candidate, recorded in the authoring shortlist; establish button ownership and native controls before implementation. |

One small shared path resolver now serves `check-painter.ps1`,
`run-acceptance.ps1`, `check-painter-selection.ps1` and
`check-acceptance-selection.ps1`. Explicit relative paths use the caller's
PowerShell filesystem location; absolute paths and default roots retain their
meaning. It resolves literal, potentially not-yet-existing filesystem paths through
the provider-aware
[PowerShell path API](https://learn.microsoft.com/en-us/dotnet/api/system.management.automation.pathintrinsics.getunresolvedproviderpathfrompspath),
then retains native full-path normalization and rejects other providers before
filesystem access. Selection/trial diagnostic order, preset/GUID suffixes,
quoting and global process-directory state are preserved. `run-balance` has an
explicit repository-relative contract and keeps it.

The actual private baseline reproduced misplaced output in both runners and
a wrong painter trial hash from different same-named process-directory bytes.
The existing selection checks now cover separate provider/process locations,
relative paths with spaces and Unicode, absolute controls, all three trial
selectors' bytes/hash/quoted arguments and restored caller state. Fresh
PowerShell 5.1 and pwsh hosts each pass 145 painter and 24 acceptance cases,
including the checkers' own relative evidence roots. Six resolver/API cases
and all six changed scripts' syntax pass in both hosts. No compiler, native
process helper or GPU runs during this CPU qualification.

The first native attempt stopped before compilation when Git worktree-status
collection hit a denied global ignore file. That query remains stopped: an
explicit painter `-OmitWorktreeStatus` option skips it and records a null status
plus an omission flag. HEAD/source hashes remain available. No Git settings
were changed and no clean status was manufactured. The Release native preview
leaf passes with this option: 68 readbacks, 26 rows and 16 crops under the
relative caller-selected root, with caller state restored and the omission
verified in its receipt. Default metadata behavior remains unchanged.

Independent review and six-file source-immutability checks pass. Evidence under
`out/validation/implementation-loop-20261028/`: failing private baseline
`hyg17-baseline/85f23b68914e44108d3f6e396f6451cb`, final four-host CPU batch
`hyg17-qualification/967a47bcf11643eba088bbe5a8c5f3aa`, native batch
`hyg17-native/1f97aa8d62eb480787300d0710db3bf8` (leaf
`e067d5a5840546299ce439f626526c7d`). Four more wrappers are qualified by
`HYG#19` and three by `HYG#20`; startup and failure-reporting need separate
scoping. The observed initialization location
leak is resolved by `HYG#18` above, without retrying the denied metadata query.

## Follow-up shortlist (2026-10-28)

The author authorized autonomous selection once ready work is exhausted or
blocked. This ranked shortlist was recorded during editor qualification;
its evidence describes the code before the associated fixes:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Unicode folder matching: `editor/user_stage_save.cpp` folds paths with locale-sensitive `towlower`, while the save guard uses Windows ordinal comparison. | Check non-ASCII case aliases, including a not-yet-created My Stages folder. Small scope; coordinate after the editor changes land. |
| 2 | Bounded painter builds: `scripts/check-painter.ps1` invokes `cmake --build` without a job count; the acceptance/editor runners already cap build concurrency. | Directly supports five projects sharing the PC. One argument change; low risk and independent of the active editor investigation. Selected as `HYG#9`. |
| 3 | Acceptance process timeouts: `scripts/run-acceptance.ps1` has no per-case compiler/probe deadline. | Useful for unattended loops, but needs generous limits for slow Debug cases and cleanup restricted to owned children. Medium scope/risk; retain for later scoping. |

After `HYG#11`, a fresh source scan found five other local check wrappers
without a CMake build limit. The next ranked shortlist is:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Complete build caps: display-awake, failure-reporting, frame-pacing, movement-feedback and paint-fx wrappers still have uncapped `cmake --build` calls. | Selected as `HYG#12`: five argument-only changes, low risk, directly useful on the shared PC. |
| 2 | Acceptance subset CLI: `Cases` treats `ammo,text_wrap` as one unknown case under `pwsh -File`, while `ExcludeCases` explicitly handles comma lists. | Small usability fix; scope selection normalization and preflight validation before implementation. |

### Focused painter preflight (`HYG#16`)

**Done (2026-10-28).** Selected the second candidate from `HYG#15` after
compatibility qualification. All 100 current focused switches had native
mappings, but the separate manual count and late argument chain allowed a
future added switch to fall through to the full matrix. A scratch copy adding
only `FutureUnmappedOnly` reproduced that fallthrough at missing-CMake
preflight before the fix; no compiler or native probe started.

The count now derives from actual bound `*Only` switch values that are
present; explicit false values remain inactive. TrialPath and Debug-only
checks retain their diagnostic order. The existing selector chain follows
`Quote-Path`, with unchanged branches/operands and an initially empty
selector-only argument array. An active selector with no native argument
fails before output/tool setup. The original quoted content/capture paths
are prepended once at the existing launch point. Independent source review
confirms all 100 ordered clauses, parameters and trial/Debug rules are
preserved. Reader, capacity and native dispatch policies remain unchanged.

The actual-runner preflight checker passes 138 checks in both PowerShell 5.1
and pwsh: all 100 discovered selectors, default/false/conflict cases, three
trial paths and Debug-only refusals. The synthetic unmapped selector now fails
before output/tool creation, while false preserves ordinary routing. Private
relative-path fixtures align both provider and process directories and restore
both; receipts confirm unchanged source and caller environment.
Final receipts under `hyg16-selection/` are
`final-ps5/b73e0742b7bf4beb883ef2ceadff432d` and
`final-ps7/f576d6cc5d8a4adfbb48904dd63bb7ee`.

The native Release stage-preview-background leaf also passes under 5.1:
68 readbacks, 26 rows and 16 native 64 by 32 crops, including real menu reopen
transitions. It verifies the final quoted launch prefix and caller restoration;
it does not run all 100 native matrices or establish visual/hardware acceptance.
Evidence is `hyg16-native/x64-release/6ea8af81f7d34ba69aee5400892f150c`
under `out/validation/implementation-loop-20261028/`.

That launch also exposed an inherited `PSModulePath` selecting PowerShell 7's
Utility module in host 5.1, leaving `Get-FileHash` absent despite `#Requires`.
Runner and checker now import their host's own standard Utility manifest
without reload, installation or environment changes. The same failing host
environment then passes native compilation/execution. CI runs both preflight
hosts and retains receipts; hosted CI has not been run locally.

### Painter PowerShell compatibility (`HYG#15`)

**Done (2026-10-28).** A fresh painter review recorded these findings before
the compatibility fix:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Twelve expressions in eleven focused verifiers use `SHA256.HashData` and `Convert.ToHexString`, unavailable under Windows PowerShell 5.1. The runner explicitly supports that host, but these calls happen after native work. | Selected as `HYG#15`: one compatible hashing helper and narrow call substitutions; preserve exact bytes, uppercase hexadecimal and every oracle. |
| 2 | Focused-selection wiring repeats 100 switches across the one-leaf guard and argument chain. All current leaves are wired, but a future missed branch could run the full matrix. | Lower priority: scope a fail-closed preflight later; no registry rewrite or speculative discovery CLI. |

The actual retained stage-preview-background verifier first failed at
`HashData` under PowerShell 5.1, without native execution or evidence writes.
One shared `SHA256.Create()` / `ComputeHash()` helper now disposes its algorithm
and returns identical uppercase, hyphenless hex. The twelve substitutions retain
the eight local wrapper names and every input array/slice.

Actual retained Track and Scribble replays also confirmed five missing
`Math.Clamp` calls. Their admitted operands are finite doubles with fixed
0..255 bounds. Inline `Max(0.0, Min(255.0, expression))` now preserves
each expression exactly once, its existing rounding, alpha division and byte
casts. Independent review confirms the same quantization for admitted values;
no numeric admission, literal pin, schema or comparison was weakened.

Four independent Python `hashlib` vectors pass in fresh 5.1 and pwsh hosts:
empty, ASCII, all byte values and a binary boundary sequence. Each returns one
uppercase digest without changing input bytes. All eleven actual affected
verifiers and their PNG completion phases pass under each host, on scratch
copies with fresh child processes. Hash manifests confirm every original
retained capture/log/receipt is unchanged. Replay batches under
`out/validation/implementation-loop-20261028/` are
`hyg15-replay-Desktop-3fcaef5a41ff4d0d921ab7deb776ce87` and
`hyg15-replay-Core-ddf1fc3453e24606a66012c6bf198d79`.

The native HUD-health leaf passes under 5.1 Release and pwsh Debug: each checks
54 readbacks, 12 rows/crops and 36,864 RGB pixels; Debug additionally checks
nine real tuning commands, 36 readbacks and private Save/rebase/reset behavior.
Native evidence under `hyg15-native/<preset>/` has IDs
`6f8feabd3b9647ebb8723325d1ffecdb` (Release) and
`91f743e4b16641e59b75b3a43d1de942` (Debug).

The initial native preflight also found this session had not loaded the
standard `Get-FileHash` function. An explicit Utility requirement initially
addressed that preflight; `HYG#16` subsequently pinned module loading to the
host's own manifest after finding a cross-edition inherited module path.
Neither change installs a module or supplies a hash shim.
Fresh direct-runner preflight in both hosts passes fixture hashing before the
expected missing-tool refusal, with no compilation. All thirteen touched
scripts parse in both hosts and independent review passes. These checks do
not establish hardware or attended visual acceptance.

### Editor build-profile isolation (`HYG#14`)

**Done (2026-10-28).** The next ranked candidate from the
[editor review](level-editor.md#raw-nul-json-admission-ed12) was an observed
tooling defect: `check-level-editor.ps1` redirected `LOCALAPPDATA` before
configure, so vcpkg lost the caller's existing registry cache. During `ED#11`,
this triggered a fresh registry fetch that failed at the unavailable GitHub
connection despite installed dependencies. It was selected after both metadata
preservation slices were qualified.

The profile assignment now follows build/cache preflight and precedes
the first acceptance executable. Build tools inherit the caller's environment;
the document/native checks and every editor launch retain the private profile.
`SkipBuild`, arguments, concurrency, nested guidance isolation and the outer
`finally` restoration are unchanged. Dependency-root and install-policy
settings are unchanged by the source fix.

The baseline captured the actual configure-call environment, then returned an
explicit failure before executing CMake; it confirmed the redirected profile.
The final one-off receipt records two real forwarded CMake invocations per
preset, both inheriting the caller's profile, followed by complete passing
document/native/window checks and caller restoration. The build used installed
dependencies with manifest installation temporarily off, then restored the
original `ON` cache settings; the failed network fetch was not retried.
Independent ordering review and parsing pass in PowerShell 5.1 and pwsh.

Profile receipts are under `out/validation/implementation-loop-20261028/hyg14-profile/`:
baseline `baseline/8d625ab173d94d2388a31f2cd61aeed3`, final
`candidate/8ac69dfe92b84b7588d25cb97de0a339`. Full editor evidence is under
`out/acceptance/<preset>/level-editor/`: Debug
`8efc0067637248eeb2ff6b426bbfb32c`, Release
`46b5227ad30748b8862add8a01be105e`. These source/tooling checks do not establish
attended editor, gameplay or hardware acceptance.

### Acceptance subset CLI (`HYG#13`)

**Done (2026-10-28).** After completing the build caps, the second candidate
now normalizes each explicit `Cases` value by splitting commas, trimming each
name and applying invariant lowercase. It preserves array arguments and order,
default Debug/Release selections, unknown-name and duplicate refusals,
Debug-only tool restrictions, and exclusion behavior. Empty selected names,
including empty comma fields, remain errors. No discovery/list option or
default-case expansion is needed for this defect.

The new selection checker first reproduced the comma/whitespace failures on
the old runner. All 22 checks then pass in fresh PowerShell 5.1 and pwsh hosts:
the actual selected list/order, defaults, arrays, normalization, empty/unknown
names, duplicate precedence, Debug-only restrictions and exclusions. An actual
current-host `-File` child verifies command-line binding. These checks use
private scratch directories and empty PATH, stopping at the required
missing-CMake preflight; no native helper, compiler or CMake starts. Caller
PATH/location are restored and the receipts include source hashes.

Final receipts under `out/validation/implementation-loop-20261028/hyg13-selection/`:
PowerShell 5.1 `ps5/b631eb4adade4c1992a1907ebb8bee0e`, pwsh
`ps7/ae967210690e4a3696968c998ac2b865`. A real native `-File` invocation using
`-Cases " ammo , TeXt_WrAp "` also passes ammunition/text wrapping in Debug
under 5.1 and Release under pwsh. Independent source review and whitespace
checks pass. CI runs selection qualification in both shells alongside process
qualification and retains their receipts/logs; hosted CI has not run locally.

### Remaining local build caps (`HYG#12`)

**Done (2026-10-28).** Each of the five wrappers above now passes
`--parallel 1`, matching the existing acceptance, painter and editor wrappers.
The argument-only changes preserve
targets, configure arguments, `SkipBuild` branches and probe execution. The
stage-registration wrapper already has an explicit two-job limit; hosted CI
keeps its separately chosen concurrency.

Independent review and parsing of all five scripts pass in PowerShell 5.1 and
pwsh. A scan of all script `--build` calls finds no remaining uncapped local
wrapper. No new test repeats the literal flag, and this source change provides
no new gameplay, hardware, rendering or performance acceptance.

### Bounded painter builds (`HYG#9`)

**Done (2026-10-28).** The painter wrapper now uses `--parallel 1`, matching
`run-acceptance.ps1` and `check-level-editor.ps1`. This bounds CMake/Ninja
build fan-out when five projects share the PC. Configure, visual-probe
execution, scope switches and `-SkipBuild` behavior are unchanged.

PowerShell parsing and scoped whitespace checks pass. Review confirmed the
only command change is the explicit job limit; no new test repeats that
literal argument, and no game/runtime behavior changed.

### Windows folder case matching (`HYG#10`)

**Done (2026-10-28).** A new regression case first failed on the original
comparison: not-yet-created Unicode case aliases were rejected. The helper
now uses `CompareStringOrdinal` with case ignored, as the save guard already
does. Absolute/lexical normalization and trailing separators are unchanged;
no locale is changed and no filesystem identity lookup was added.

The checks exercise both directions of e-acute and a-umlaut case pairs before
creation, then independently verify existing aliases through the filesystem.
They retain ASCII/dot/separator behavior, distinct/sibling/nested refusals,
ASCII filename and shipped-name rules, source/My Stages guidance, actual
save/reopen/ordinary Save and original replacement spelling.

Independent review, all three editor targets and complete editor checks pass
in Debug and Release. Evidence: Debug `ef93762bdefe4e70bdc6d0de24e3b914`,
Release `c71f6a2d9fbe4077a32ac99b6059d1cf`, under
`out/acceptance/<preset>/level-editor/`. The original failing run is retained
in `out/validation/implementation-loop-20261028/hyg10-baseline-run.log`.
Discovery destinations, junction/symlink policy and human gates are unchanged.

### Acceptance probe deadlines (`HYG#11`)

**Done (2026-10-28).** A hung acceptance executable previously held its serial
run or parallel slot indefinitely, and its buffered output was written only
on exit. `RunTimeoutSeconds` (default 3,600) now bounds the executable
phase after successful compilation. The runner preserves case order, `Parallel`,
`CompileOnly`, UTF-16 compiler response files and the serial UTF-8 tool scope.

The Windows-specific process helper supports Windows PowerShell 5.1 and pwsh.
It launches the probe suspended, assigns it to a private
kill-on-close [Job Object](https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects),
then resumes it. Explicit inherited output handles keep parallel cases separate.
It retains merged output incrementally in a raw sidecar and finalizes the existing
UTF-8 log with its captured encoding. PowerShell polls in short interruptible
steps inside `try/finally`; disposal/cancellation cleans only owned jobs and
queued probes do not launch after cancellation. Launch failure, timeout and
uncertain cleanup fail the case. A containment denial cancels the run instead
of launching uncontained or repeatedly retrying that denied action.

Compiler and initial CMake execution remain outside this slice. `/MP`
enables [synchronous PDB writes](https://learn.microsoft.com/en-us/cpp/build/reference/fs-force-synchronous-pdb-writes?view=msvc-170)
through MSPDBSRV; current flags do not establish exclusive ownership of such
a server if supplied by compiler environment options. A compiler job could
therefore capture shared work. This is an ownership uncertainty, not proof
that the current invocation starts one. Compiler invocation is preserved, and
already-running compiles finish during parallel cancellation. A core cancellation
token also skips queued `CompileOnly` compiles without loading the native helper.

Independent review and syntax parsing pass in both shells. The isolated native
fixture passes eleven real-process checks in each: success/nonzero/missing
launch, Unicode/quoted arguments, legacy decoding, large merged output,
flushed partial output on timeout, owned sleeping-child cleanup, an unrelated
owned sentinel surviving, normal-parent-exit child cleanup, parallel log
separation, queued cancellation and serial pipeline interruption. The interruption
check exercises PowerShell pipeline cancellation and `finally`, not injected
console input. Known process handles replace global process inventory.
An additional guard extracts the actual runner's `runCase` script block and
checks canceled `CompileOnly` work never calls its compiler delegate.

Final receipts under `out/validation/implementation-loop-20261028/hyg11-process/`:
PowerShell 5.1 `ps5/8a785e248f9947939621af5259d47d39`, pwsh
`ps7/91ec7c2faf4f42118ccd9c647b7f959a`. Each records source hashes and case facts.
Actual runner checks also pass: Debug ammunition/text wrapping with two slots
under 5.1, Release equivalents serially under pwsh, and Debug `balance_runner`'s
UTF-8 path. Both fresh shells compile into Unicode/spaced output paths without
loading the helper or running the executable; response files remain UTF-16 LE.
Caller PATH, location and console output encoding are restored.

A one-second Debug `menu_text` runtime deadline produces the expected named
timeout, retaining both logs and restoring the caller environment. That case
emits nothing before its first completed check, so its retained logs are empty;
the isolated fixture separately proves flushed partial-output retention. The
interrupted `menu_text` run is not a gameplay acceptance pass. The sixteen CI
change-detection regressions pass. CI now runs the isolated fixture in both
shells and retains receipts/logs; hosted CI itself has not run locally.

## Scope and working rules

File length identifies candidates to inspect, not a maximum or a reason to cut
a cohesive implementation. Preserve the game's inheritance, public signatures,
names, layouts and behavior. Engine and vendored code are outside this pass.
Keep historical review documents as written.

For a split, move whole definitions and their private helpers together wherever
possible. Preserve evaluation, callback, RNG, diagnostic and floating-point
operation order. Do not combine a split with bug fixes, renaming, broad
formatting, a new abstraction model or gameplay changes. An extraction that
needs new private parameters must say so; it is not a literal source move.

Register new game translation units in [game/CMakeLists.txt](../game/CMakeLists.txt),
with the existing PCH, include and warning conventions. The
[acceptance runner](../scripts/run-acceptance.ps1) already compiles a case's
same-named folder of `.cpp` files. Header-based visual/document fixtures have
their own inclusion context; preserve it or wire an explicit replacement.

Review the moved definition bodies and narrow build/include changes, then build
Debug and Release and run the existing checks named in each scope. Do not add
tests that merely mirror a move. Comment-only work needs caller/implementation
review and a comment-only diff, not a new runtime test suite. Keep structural
changes and behavioral defects in separate commits when implementation lands.

## Source-size snapshot (2026-10-28)

These counts include blank lines and comments in tracked authored files. Refresh
the affected files before starting; this is audit evidence, not a permanent size
gate. The earlier figure/animation and other source splits already exist.

| file | lines | audit result |
|---|---:|---|
| [presentation_validation.cpp](../game/presentation_validation.cpp) | 25 | Ordered coordinator and nine family sources; `HYG#1` complete |
| [player.cpp](../game/objects/player.cpp) | 1,848 | Retain the coupled movement/contact/carrier core; figure code is already separate |
| [input_bindings.cpp](../scripts/acceptance/input_bindings.cpp) | 56 | Four topic sources and shared fixtures; `HYG#5` complete |
| [evidence-record.ps1](../scripts/check-painter/evidence-record.ps1) | 354 | Four explicit topic helpers; `HYG#6` complete |
| [painter_visual.cpp](../scripts/acceptance/painter_visual.cpp) | 1,347 | Retain the explicit include/setup/dispatch entry; no registry redesign selected |
| [level.cpp](../game/objects/level.cpp) | 966 | Horizontal-motion subsystem extracted (380 lines), `HYG#2` complete |
| [tune_colour_document_checks.h](../scripts/acceptance/debug_console/tune_colour_document_checks.h) | 17 | Ordered topic coordinator; `HYG#7` complete |
| [tuning.cpp](../game/tuning.cpp) | 907 | Retain cohesive key-by-key parsing and validation |
| [level_document.cpp](../editor/level_document.cpp) | 949 | Retain the shared document transaction; `ED#7` save protection complete |
| [projectile.cpp](../game/objects/projectile.cpp) | 287 | Contacts/sweeps extracted (587 lines), `HYG#3` complete |
| [weapon.cpp](../game/objects/weapon.cpp) | 855 | Retain the current firing/lifecycle core; motion already has its own file |
| [save.cpp](../game/save.cpp) | 602 | Profile overlay extracted (209 lines), `HYG#4` complete |

## Ordered presentation validation (`HYG#1`)

**Done (2026-10-28).** `Presentation::validate_resources` is now a 25-line
ordered coordinator over nine validation families. The private nested worker
borrows presentation/resources and carries concrete already-computed setup,
stage, controls and roster geometry/font contexts. It performs no eager
resource lookup or measurement. Shared helper and metrics definitions live
with those families; `sound_references` and `validate_stage_counts` moved as
whole methods. The largest cohesive family remains 737 lines.

The public API and member layout are unchanged. Independent source comparison
reconstructs all nine original phase statement streams after explicit
receiver/context transfers, plus sixteen helper bodies, three metrics and the
two catalog methods. Lookup/calculation order, floating-point operations and
the Stage/Results/roster interleaving remain intact. The qualified `ROAD#25`
paper-face boundary is the retained baseline.

Debug and Release builds, `menu_presentation`, `weapon_catalog`,
`stage_selection`, loader/user-stage checks and hidden accepted-bank startup
pass. The instrumented existing `menu_text` case compares the preserved
original source against the split with current headers, compiler and generated
build identity: **3,709 exact first diagnostics match in each preset**. This
includes five multiple-invalid-field probes across resource and interleaved
page families. Only scratch filename prefixes are normalized. All existing
menu-text assertions also pass in both baseline and candidate executions.

## Level horizontal motion (`HYG#2`)

**Done (2026-10-28).** Moved the horizontal admission helpers and all eight
motion query, admission/budget, prepare, commit, zero-step and snapshot methods
together into `game/objects/level_horizontal_motion.cpp` (380 lines). The
current `level.cpp` is 966 lines; its update coordinator and call order stay
in place. Public signatures, member layout, diagnostics and pre-step paint
ownership timing are unchanged.

Independent source comparison confirmed that the helper block and all moved
method bodies are identical, including their float/double arithmetic. The
remaining implementation differs only by the move and includes. Debug and
Release builds and the existing `moving_structure`, `paint_ferry`,
`fixed_slab`, `movement` and `instant_replay` checks pass. Debug retains the
manual/public-loader checkpoint, paint-credit and terminal-RNG replay checks;
the shipping seven-arena replay passes in both configurations.

## Projectile contacts and sweeps (`HYG#3`)

**Done (2026-10-28).** Moved the private sweep/relative-impact helpers and
all eleven definitions from `resolve_launch_obstruction` through
`flush_paint_audio` into `game/objects/projectile_contacts.cpp` (587 lines).
Construction, diffusion, movement, lifetime and accessors remain in the
287-line `projectile.cpp`; the contact helpers remain private to their new
translation unit.

Independent block comparisons confirm identical definitions and retained
sweep ordering, launch exemptions, relative moving-surface geometry,
endpoint deduplication and impact/paint/audio callback order. This only moves
source and adjusts includes/registration. Debug and Release builds and
`blocked_shot`, `tunnelling`, `moving_structure`, `fixed_slab`, `paint` and
`instant_replay` pass, including Debug's public-loader replay checks.

## Save profile overlay (`HYG#4`)

**Done (2026-10-28).** Moved `copy_slot`, the play-preference/binding setters,
effective-preference accessors and profile/wearing/Boost operations into
`game/save_profiles.cpp` (209 lines). The 602-line `save.cpp` retains JSON
sections, migration, loading and atomic writing. Display settings stay with
the resolution conversion used by serialization; they have no profile overlay.

Independent comparison confirms identical helper and method bodies, including
effective-reference identity, per-slot association, mutation/write order and
failure semantics. The public API and save schema are unchanged. Debug and
Release builds and the existing `player_profiles`, `settings`,
`input_bindings`, `handicap` and `remembered_setup` checks pass, including
isolated real write/replacement failures, active-setting recovery and reloads.

## Input-binding acceptance topics (`HYG#5`)

**Done (2026-10-28).** The 56-line `input_bindings` entry retains `main` and
the single fake `GamepadReader`/state seam. Four topic sources hold independent
domain/oracle checks, actual reader/resume checks, save/schema/relaunch checks
and keyboard actions. `fixtures.h` shares only the required fixtures, literal
oracle tables and declarations; the acceptance runner discovers the sources.

Exact source-group comparisons preserve all assertion/oracle literals,
function bodies, invocation order and isolated profile lifetimes. Shared names
now use `input_bindings_acceptance`; defaults live on declarations and shared
constant tables have inline linkage. No production helper replaces an oracle.

The existing `input_bindings` case passes in Debug and Release. In each
configuration, separate `relaunch-write` and `relaunch-read` processes also pass
against the same retained private profile, preserving the real persisted-J
reader and unchanged-save checks. No player data or hardware is used.

## Painter evidence metadata (`HYG#6`)

**Done (2026-10-28).** The 354-line entry retains its ordered base record,
all 98 explicit precedence conditions and the four final fixups. Their bodies
are dot-sourced from coherent arena (15 branches), colour (31), menu/text (44)
and action/HUD (8) helpers under `scripts/check-painter/evidence-record/`.
The unique selector parameters do not shadow existing inputs; no moved body
uses the helper's script path. There is no registry or painter-matrix change.

Independent inspection and PowerShell token comparison retain every condition,
body token, literal string, ordered key and dot-sourced failure boundary. Base
and final-fixup statements are byte-identical. All five scripts parse. The
baseline/candidate harness passes 800 ordered metadata comparisons: every one
of 99 focused switches in both presets, adjacent/first-winner precedence,
PracticeReplay overrides and combined final fixups. Only game revision, engine
revision and working-tree metadata are normalized.

Representative native `FerryDirectionOnly`, `OptionsDefaultsOnly`,
`CrackTuneColourOnly` and `ResumeOnly` leaves pass in Debug and Release,
covering every extracted group and retaining their existing scope receipts.
No full painter matrix or new timing claim was needed for this source move.

## Colour-document fixture families (`HYG#7`)

**Done (2026-10-28).** The 17-line `check_colour_document` entry coordinates
six topic helpers in four headers: splice/alias behavior, selectors/feedback,
opening artwork, and HUD/figure/identity colours. The interleaved additions
retain their original order: generic, feedback, opening art, HUD,
crack/acting, then limbs/identity. All headers keep the original anonymous
namespace and `last_key` inclusion context.

Reconstructing the original function body from those ordered helper bodies
reproduces all 1,038 lines unchanged after physical newline normalization.
Literal JSON, source-span assertions, conflict/reset checks and byte-preserving
fixtures are unchanged; no production accessors or registration were added.
Independent review and the existing `debug_console` case pass in Debug and
Release, including Release's executable exclusion checks.

## Source-comment contract audit (`HYG#8`)

**Done (2026-10-28).** Corrected the three bounded comment groups against
current implementations and callers. Source edits are comments only; the
executable tokens, APIs, member layouts and behavior are unchanged. An
independent caller review agreed with the corrections. No new runtime tests
were added for this comment-only slice.

Completion evidence, by edited declaration or claim:

| comment location | verified implementation/caller |
|---|---|
| `PlayerBuilder::build_players` declaration and local identity comment | `player_builder.cpp` keeps roster order, increments `human_view` only for humans, passes participant to sound and `paint_owners::index(setting.num, participant_index)` to the weapon |
| `MenuPlayerSettings::num`, `MenuLevelSettings::device_slot` and `Player::player_num` | `menu_level_settings.cpp` assigns human slots, gives bots `-1` and remaps only the device; `Level::apply_player_inputs` reads stable slots; `results_menu.cpp` maps participants through `paint_owners::index` |
| `paint_owners.h` owner-domain comment and `Player` constructor's sound comment | `PlayerBuilder` uses slot-based human paint credit and participant-derived bot credit; the constructor receives a separate participant sound index |
| `Level` participant/pane, feedback and acting comments | `Level` constructs `view_participants_` only for humans; `take_rumble_events` retains `player_num()`; `level_dev_tools.cpp` indexes players by participant; `level_draw.cpp::refresh_identity_texts` falls back to bot difficulty or human device; `level_intro.cpp::update_acting` and `draw_acting` index `acts_` alongside `players_` |
| `Level::update`, `DebugDirector::update_match`, `GameLevel` adapter declarations and `game_level_dev_tools.cpp` heading | `game_states.cpp::update` calls the director in both builds; the Release inline hook forwards to `debug_update_level`, which supplies `RoundTapeRecorder`; `game_level_replay.cpp::step_replay` supplies `RoundTapePlayer` |
| `Level` shipping setup/replay adapter comment | `game_states.cpp` calls `set_pad_devices` during build and update; `game_level_replay.cpp::start_replay` restores pad mappings and cheats |
| `DebugDirector` Release overview | `debug_director.h` retains ordinary update forwarding and `opening_mode` pass-through; `debug_director.cpp::run_launch_arguments` retains the Release argument warning |
| `RoundTape` invalidation overview | `GameLevel::debug_place`, `debug_respawn` and `debug_kill` call `spoil`; `RoundTape::set_cheats` spoils after a recorded step; `tune_commands.cpp` applies live edits without tape invalidation; the `ROAD#19` owner records that exception and Debug-injected suspends |
| `GameData::stages` and `MenuLevelSettings::set_stage` | `stage_loading.cpp` appends accepted user definitions after manifest stages in file-name order; `MainMenuStageSelect::set_level_settings` chooses the selected stage's registry name |
| `LevelDocument::unresolved_resources` and `LevelLoadedInfo::validate_content` icon comments | both still check `icon_frame` in `sprite_sheet_1`; `MainMenuStageSelect::update_stage_select_visuals` selects `StagePreview`, not the legacy icon |

The accurate `stage_loading.h` rescan, borrowed-registry and no-match lifetime
comments were retained: `GameMenu::init` lends the callback to Stage Select,
whose main-menu entry rescans, while between-round entry does not. Missing
definitions remain registered but unreachable from the stage list. Historical
reviews and the engine's design decisions were left untouched.
