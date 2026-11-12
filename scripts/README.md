# Scripts

Every script here is run by hand or by [CI](../.github/workflows/ci.yml); none
runs as part of a normal build. The PowerShell scripts need an x64 Visual Studio
Developer shell when they compile or build; `build.ps1` sets that environment
up itself. They run under Windows PowerShell 5.1 and pwsh, except the four
`build-*-art.ps1` scripts, which require Windows PowerShell 5.1. Relative
`-OutputDirectory` paths follow the caller's PowerShell location in the check
scripts, acceptance runner and Study B capture; `run-balance.ps1` resolves its
paths against the repository root. [AGENTS.md](../AGENTS.md#tests) and
[the development guide](../docs/development.md#consumer-acceptance) describe the acceptance rules in
full.

## Building

| Script | What |
|---|---|
| [build.ps1](build.ps1), [../build.cmd](../build.cmd) | builds from a plain shell or double-click: VS/SDK/vcpkg checks, retained `out/build-<preset>.log`, guide and play-again paths (`-Preset`, `-Package`, `-Run`) |
| [check-build.ps1](check-build.ps1) | first-run helper failures, successful/failed Visual Studio discovery and streamed native output; isolated subprocess fixtures, no compiler or download |

## Acceptance

| Script | What |
|---|---|
| [run-acceptance.ps1](run-acceptance.ps1) | builds the game and runs the consumer probes declared in [acceptance/cases.psd1](acceptance/cases.psd1) (`-Preset`, where `x64-asan` runs Release's list under AddressSanitizer, `-Cases`, `-ExcludeCases`, `-OptIn`, `-QuickOptIn`, `-OwnLegOnly`, `-SkipOwnLeg`, `-SkipSlowDebug`, `-CompileOnly`, `-Parallel`, `-RunTimeoutSeconds`); writes `acceptance-summary.json` |
| [acceptance-process.ps1](acceptance-process.ps1), [acceptance-process/](acceptance-process/) | the runner's native helper: one Windows Job Object, deadline and log per probe |
| [check-acceptance-selection.ps1](check-acceptance-selection.ps1) | checks the runner's case selection rules without CMake, a compiler or the helper |
| [check-acceptance-process.ps1](check-acceptance-process.ps1) | qualifies the helper's ownership, deadlines, logging and cancellation with isolated processes |
| [check-path.ps1](check-path.ps1) | dot-sourced helper that resolves explicit output paths against the caller's location |
| [process-births.ps1](process-births.ps1) | dot-sourced helper that counts the processes a check starts, through a Job Object, so the two selection checkers measure that their preflights started none |
| [quiet-game.ps1](quiet-game.ps1) | dot-sourced helper that sets `COLOURWARS_MUTE` for the game and real-audio probes a check starts, and restores the caller's value; set `COLOURWARS_MUTE=0` first to hear a run |
| [drawing-references.ps1](drawing-references.ps1) | dot-sourced helper giving `check-paint-fx.ps1` and the debug-console driver the `Add-Type` references System.Drawing code needs under pwsh as well as Windows PowerShell |
| [probe-build.ps1](probe-build.ps1) | dot-sourced helper for the configured build's compiler arguments, game and adapter libraries, dependency includes and Windows libraries; for `x64-asan` also the sanitizer flags and the folder of its runtime DLL |
| [report-analyze.ps1](report-analyze.ps1) | builds the `x64-analyze` preset from clean and writes `analyze-warnings.log` and `analyze-summary.json`, the `/analyze` warnings counted by code for this repository and for the engine; a report that fails only if the build does |
| [check-diagnostics.ps1](check-diagnostics.ps1) | shared stderr checks; only the exact optional-bank fallback or a caller's explicit fixture diagnostic is allowed |
| [check-windows.ps1](check-windows.ps1) | shared Win32 source for enumerating a child process's top-level windows, finding a class and closing those windows |
| [check-probe-helpers.ps1](check-probe-helpers.ps1) | checks build preflights, diagnostics and owned-window enumeration with synthetic files and an invisible native window; no CMake, MSVC or GPU |

[acceptance/](acceptance/) holds one `.cpp` per case declared in
[cases.psd1](acceptance/cases.psd1), a same-named folder of its checks where it
has one, and [acceptance/support/probe.h](acceptance/support/probe.h)
(`acceptance::require` and `acceptance::run_probe`). A probe that builds game
objects loads the manifest's sheets, fonts and banks with
[support/content_world.h](acceptance/support/content_world.h), and one that
claims every stage walks `manifest_level_paths` in
[support/stage_catalog.h](acceptance/support/stage_catalog.h), not a list of its
own. A probe that touches the save points `LOCALAPPDATA` at a private root with
`acceptance::IsolatedSave` in
[support/isolated_save.h](acceptance/support/isolated_save.h), and one that
checks a reader refuses bad JSON writes edited copies with
[support/json_fixture.h](acceptance/support/json_fixture.h); both keep their
scratch by [support/scratch.h](acceptance/support/scratch.h), whose checked
`read_text` and `write_text` the probes use for whole files. A probe that reads what
the game prints to stderr captures it with
[support/stderr_capture.h](acceptance/support/stderr_capture.h). The opt-in
`reader_mutations` case feeds fixed-seed mutants of every staged level to
`load_user_stages` and of three saves to `Save`, in one private root, and
requires that each loads or is refused; its own watchdog thread fails it on a
mutant that takes over 120 seconds
([record](../docs/code-hygiene.md#mutation-probe-for-the-save-and-user-stage-readers-rev62)). A probe that
steps one real Player in a bare Scene derives its course from
`acceptance::PlayerCourse` in
[support/player_course.h](acceptance/support/player_course.h), which keeps
`Level`'s tick order; the Crossfeed, Close Quarters and Press Tower cases share
one stage's route-run fixture in
[support/route_run.h](acceptance/support/route_run.h), whose `.cpp` they list
as a source; and the bot cases share their rosters, tape and replay check in
[bot_navigation/fixtures.h](acceptance/bot_navigation/fixtures.h). It also holds the sources
of probes that are not cases and that their own entry points build:
`painter_visual` ([check-painter.ps1](check-painter.ps1)), `paint_fx_visual`
([check-paint-fx.ps1](check-paint-fx.ps1)), `movement_feedback_visual`
([check-movement-feedback.ps1](check-movement-feedback.ps1)), `frame_pacing`
([check-frame-pacing.ps1](check-frame-pacing.ps1)), `display_awake_lifecycle`
([check-display-awake.ps1](check-display-awake.ps1)), `debug_menu_navigation`
([check-debug-console.ps1](check-debug-console.ps1)), and `level_editor` and
`level_editor_dpi` (built by [editor/CMakeLists.txt](../editor/CMakeLists.txt)
and run by [check-level-editor.ps1](check-level-editor.ps1)). Do not add them
to `cases.psd1`.

## Game checks

| Script | What |
|---|---|
| [check-startup.ps1](check-startup.ps1) | starts a hidden game process and checks DPI awareness, an isolated save and its `stages` folder, diagnostics and graceful shutdown; `-Locale` starts it in a locale |
| [check-failure-reporting.ps1](check-failure-reporting.ps1) | the caught-failure report ([failure-reporting.md](../docs/failure-reporting.md)) |
| [check-display-awake.ps1](check-display-awake.ps1) | `GameLevel`'s display-idle request over a match's lifetime, with a recording adapter |
| [check-debug-console.ps1](check-debug-console.ps1), [check-debug-console/](check-debug-console/) | drives the Debug console with real key events; phase 3 compiles a CPU menu-navigation probe against the game's model (`-CompileOnly` builds/queries it without a GUI); the full driver needs a foreground desktop, so it is local only (`CI#1`) |
| [check-finish-details.ps1](check-finish-details.ps1) | runs the `review_finish_details` console script and requires equal checksums across its variants |
| [check-movement-feedback.ps1](check-movement-feedback.ps1) | renders and checks movement-feedback frames ([polish-feedback.md](../docs/polish-feedback.md)) |
| [check-paint-fx.ps1](check-paint-fx.ps1) | renders and checks projectile and fresh-paint frames ([polish-paint-effects.md](../docs/polish-paint-effects.md)); `-CompileOnly` builds the probe without a window, as CI does |
| [check-painter.ps1](check-painter.ps1), [check-painter/](check-painter/) | the painter visual probe and its focused leaves, evidence records and contact sheets |
| [check-painter-sweep.ps1](check-painter-sweep.ps1) | runs every focused painter leaf or one shard of them, one `check-painter.ps1` run each; [nightly.yml](../.github/workflows/nightly.yml) runs it with [check-painter/hosted-known-failures.txt](check-painter/hosted-known-failures.txt) |
| [check-painter-selection.ps1](check-painter-selection.ps1) | checks the painter probe's selectors and routing without a compiler or GPU |
| [check-painter-evidence.ps1](check-painter-evidence.ps1) | checks the shared painter PPM, finite-number, key-set, CSV, save and PNG evidence helpers without the game or a GPU |
| [check-frame-pacing.ps1](check-frame-pacing.ps1) | frame-pacing capture ([polish-frame-pacing.md](../docs/polish-frame-pacing.md)); analysed by [perf/](perf/) |
| [run-balance.ps1](run-balance.ps1) | headless seeded bot matches, per-seat subs and an in-memory numeric/boolean tuning patch, with bot counters ([gameplay-tuning.md](../docs/gameplay-tuning.md)) |
| [run-balance-matrix.ps1](run-balance-matrix.ps1) | one compile, bounded parallel jobs, hashed resume and byte-identity repeat; `balance/shipped.psd1` and `balance/bot-ladder.psd1` designs (`-PlanOnly` previews the count) |

## Editor checks

| Script | What |
|---|---|
| [check-level-editor.ps1](check-level-editor.ps1) | the level editor's hidden-window checks; dot-sources [check-level-editor-save-guidance.ps1](check-level-editor-save-guidance.ps1) |
| [check-stage-registration.ps1](check-stage-registration.ps1) | `ColourWarsRegisterStage`, the opt-in source registration tool |

## Release

| Script | What |
|---|---|
| [sandbox-release.ps1](sandbox-release.ps1), [sandbox/](sandbox/) | prepares, and optionally runs, a Windows Sandbox trial of the Release ZIP (`ROAD#37`) |
| [check-sandbox-release.ps1](check-sandbox-release.ps1) | checks that generator's output on the host without starting a sandbox |
| [release-package-path.ps1](release-package-path.ps1) | dot-sourced helper that reads `project(VERSION)` from `CMakeLists.txt` and returns the Release ZIP's path; used by `build.ps1`, `sandbox-release.ps1`, `check-release-package.ps1` and [releasing.md](../docs/releasing.md)'s fallback check |
| [check-release-package.ps1](check-release-package.ps1) | extracts the Release ZIP and checks its packaging rules: no development files or unreviewed audio, every required file, the accepted bank's hash and no developer-console RTTI (`CI#6`; CI's inspection step) |

## Content builds

| Script | What |
|---|---|
| [build-fonts.ps1](build-fonts.ps1) | regenerates the sprite-font atlases (`-Update` writes them) |
| [build-painter-art.ps1](build-painter-art.ps1), [build-weapon-art.ps1](build-weapon-art.ps1), [build-paint-fx-art.ps1](build-paint-fx-art.ps1), [build-terrain-art.ps1](build-terrain-art.ps1) | rebuild the runtime art from its editable recipes ([game/art/](../game/art/README.md)) |
| [art-png.ps1](art-png.ps1) | shared PNG encoder for those four art recipes; removes colour-profile chunks as the runtime texture pipeline requires |
| [build_icon.py](build_icon.py) | rebuilds `game/colourwars.ico` and its preview from the editable drawing ([game/art/icon/](../game/art/icon/README.md)); `--check` fails if they are stale, which CI runs through [ci/test_build_icon.py](ci/test_build_icon.py) |
| [audio/](audio/) | the audio pipeline and runtime-bank publication ([game/audio/paint_exe/](../game/audio/paint_exe/README.md)) |

## Other folders

| Folder | What |
|---|---|
| [debug/](debug/) | the shipped developer-console scripts (`exec <name>`), listed in [debug-console.md](../docs/debug-console.md#scripts) |
| [ci/](ci/) | CI's changed-line lint, the decision whether an event needs the Windows jobs, the engine pin's check against Labrador, the Markdown link check, and their tests; `export-msvc-environment.ps1` runs vcvars64 once per job into `GITHUB_ENV` |
| [balance/](balance/) | `TOOL#4`'s matrix driver/designs and analyser: tool and 2v2 team paint/eliminations, bot counters, profile checks and independent A/B intervals with design margins ([gameplay-tuning.md](../docs/gameplay-tuning.md#balance-matrix-tool-tool4)), and its test |
| [perf/](perf/) | frame-pacing analysis and its test |
| [studies/](studies/README.md) | the kits for the drawing-mistake human studies ([drawing-mistakes-research.md](../docs/drawing-mistakes-research.md#8-human-tests)): Study A's references, sheets and analysis, Study B's stimulus capture, test page and analysis |
