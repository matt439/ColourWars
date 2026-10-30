# Scripts

Every script here is run by hand or by [CI](../.github/workflows/ci.yml); none
runs as part of a normal build. The PowerShell scripts need an x64 Visual Studio
Developer shell when they compile or build, and run under Windows PowerShell 5.1
and pwsh. Relative `-OutputDirectory` paths follow the caller's PowerShell
location. [AGENTS.md](../AGENTS.md#tests) and
[the README](../README.md#consumer-acceptance) describe the acceptance rules in
full.

## Building

| Script | What |
|---|---|
| [build.ps1](build.ps1), [../build.cmd](../build.cmd) | builds a preset from a plain shell or a double-click: fetches the submodule, finds Visual Studio with vswhere, loads vcvars64, falls back to its bundled vcpkg (`-Preset`, `-Package`, `-Run`) |

## Acceptance

| Script | What |
|---|---|
| [run-acceptance.ps1](run-acceptance.ps1) | builds the game and runs the consumer probes declared in [acceptance/cases.psd1](acceptance/cases.psd1) (`-Cases`, `-ExcludeCases`, `-OptIn`, `-SkipSlowDebug`, `-CompileOnly`, `-Parallel`); writes `acceptance-summary.json` |
| [acceptance-process.ps1](acceptance-process.ps1), [acceptance-process/](acceptance-process/) | the runner's native helper: one Windows Job Object, deadline and log per probe |
| [check-acceptance-selection.ps1](check-acceptance-selection.ps1) | checks the runner's case selection rules without CMake, a compiler or the helper |
| [check-acceptance-process.ps1](check-acceptance-process.ps1) | qualifies the helper's ownership, deadlines, logging and cancellation with isolated processes |
| [check-path.ps1](check-path.ps1) | dot-sourced helper that resolves explicit output paths against the caller's location |
| [drawing-references.ps1](drawing-references.ps1) | dot-sourced helper giving `check-paint-fx.ps1` and the debug-console driver the `Add-Type` references System.Drawing code needs under pwsh as well as Windows PowerShell |

[acceptance/](acceptance/) holds one `.cpp` per case, a same-named folder of
its checks where it has one, and [acceptance/support/probe.h](acceptance/support/probe.h)
(`acceptance::require` and `acceptance::run_probe`).

## Game checks

| Script | What |
|---|---|
| [check-startup.ps1](check-startup.ps1) | starts a hidden game process and checks DPI awareness, an isolated save, diagnostics and graceful shutdown |
| [check-failure-reporting.ps1](check-failure-reporting.ps1) | the caught-failure report ([failure-reporting.md](../docs/failure-reporting.md)) |
| [check-display-awake.ps1](check-display-awake.ps1) | `GameLevel`'s display-idle request over a match's lifetime, with a recording adapter |
| [check-debug-console.ps1](check-debug-console.ps1), [check-debug-console/](check-debug-console/) | drives the Debug console with real key events; needs a foreground desktop, so it is local only (`CI#1`) |
| [check-finish-details.ps1](check-finish-details.ps1) | runs the `review_finish_details` console script and requires equal checksums across its variants |
| [check-movement-feedback.ps1](check-movement-feedback.ps1) | renders and checks movement-feedback frames ([polish-feedback.md](../docs/polish-feedback.md)) |
| [check-paint-fx.ps1](check-paint-fx.ps1) | renders and checks projectile and fresh-paint frames ([polish-paint-effects.md](../docs/polish-paint-effects.md)) |
| [check-painter.ps1](check-painter.ps1), [check-painter/](check-painter/) | the painter visual probe and its focused leaves, evidence records and contact sheets |
| [check-painter-selection.ps1](check-painter-selection.ps1) | checks the painter probe's selectors and routing without a compiler or GPU |
| [check-frame-pacing.ps1](check-frame-pacing.ps1) | frame-pacing capture ([polish-frame-pacing.md](../docs/polish-frame-pacing.md)); analysed by [perf/](perf/) |
| [run-balance.ps1](run-balance.ps1) | headless seeded bot matches for balance comparisons ([gameplay-tuning.md](../docs/gameplay-tuning.md)) |

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
| [check-release-package.ps1](check-release-package.ps1) | extracts the Release ZIP and checks its packaging rules: no development files or unreviewed audio, every required file, the accepted bank's hash and no developer-console RTTI (`CI#6`; CI's inspection step) |

## Content builds

| Script | What |
|---|---|
| [build-fonts.ps1](build-fonts.ps1) | regenerates the sprite-font atlases (`-Update` writes them) |
| [build-painter-art.ps1](build-painter-art.ps1), [build-weapon-art.ps1](build-weapon-art.ps1), [build-paint-fx-art.ps1](build-paint-fx-art.ps1), [build-terrain-art.ps1](build-terrain-art.ps1) | rebuild the runtime art from its editable recipes ([game/art/](../game/art/README.md)) |
| [audio/](audio/) | the audio pipeline and runtime-bank publication ([game/audio/paint_exe/](../game/audio/paint_exe/README.md)) |

## Other folders

| Folder | What |
|---|---|
| [debug/](debug/) | the shipped developer-console scripts (`exec <name>`), listed in [debug-console.md](../docs/debug-console.md#scripts) |
| [ci/](ci/) | CI's changed-line lint, the decision whether an event needs the Windows jobs, the Markdown link check, and their tests; `export-msvc-environment.ps1` runs vcvars64 once per job into `GITHUB_ENV` |
| [perf/](perf/) | frame-pacing analysis and its test |
