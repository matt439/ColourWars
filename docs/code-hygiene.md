# Code hygiene and maintenance

This is the live owner for `HYG` rows in [TODO](../TODO.md). The author asked
for source-file splits and comment audits on 2026-10-28. These are independent
maintenance slices alongside the gameplay backlog; they do not change its
arena/tuning order.

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
`e067d5a5840546299ce439f626526c7d`). Other wrappers remain candidates for a
later bounded scope. A separate observed cleanup issue also remains: an
evidence-initialization error happens before painter's `try` and can leave
the caller's PowerShell location changed. Scope that failure path with a
synthetic private helper, without retrying the denied metadata query.

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
