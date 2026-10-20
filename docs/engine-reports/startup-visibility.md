# Request: explicitly show an Application initialized hidden

Local **ENG#19** draft. **Unfiled.** Use Labrador's Engine request form when
publishing; this records no upstream request or engine change.

## Need and existing capability

An application can initialize its devices while hidden, load and validate
content, prepare its first state, and choose when to become visible.
`ApplicationOptions::visible=false` already supports the hidden-creation half.
The missing convenience is a later public show operation or a documented
public sequence that keeps the shell's show-command, sizing and focus policy
inside its platform boundary.

At checked upstream master
`52e809a5a0229d37db4373faa7853df4c861a310`,
[`Application`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.h)
and [`Window`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/window.h)
offer no later-show method. Their native handle is public. A Windows client
can call `ShowWindow` on `Application::window()`; that workaround is available
and has not been demonstrated broken here. It makes the caller choose the
platform show command and understand subsequent resize/focus effects.

The source check follows a fresh upstream fetch on **2026-10-20 at 19:34:53
UTC**, with the whole engine worktree confirmed unchanged against that
`FETCH_HEAD`. The Win32 constructor already chooses `SW_HIDE` when visibility
is false; otherwise it chooses the fullscreen or launcher-default show
command. A default-visible launch's actual visibility follows that launcher
policy. Hidden smoke launches do not establish the first visible game frame.

## Asset-free positive control

The [exact tested source](../../scripts/engine-reports/startup-visibility/startup_hidden_creation.cpp)
initializes a real Application with `visible=false`, one worker and one view.
It checks that the resulting native window belongs to its process and remains
hidden, records client/layout/back-buffer dimensions and backend, then checks
that scope teardown destroys its window. Its default and `--fullscreen`
variants load no assets and start no state/update/render loop. There is no
save, later-show call or first presentation.

The D3D11 renderer, XAudio2 audio backend and XInput shell still initialize;
this is asset-free, not device-free. Header inspection establishes the absent
convenience. The executable is a positive control for supported hidden
creation, not a failing startup-render reproduction.

## Build and run

The retained builds used x64 MSVC **19.51.36256.0**, C++20,
`/EHsc /W4 /WX /permissive-`. Debug used `/MDd /Od /D_DEBUG`; Release used
`/MD /O2 /DNDEBUG`. The [build recipe](../../scripts/engine-reports/startup-visibility/build-startup-hidden.ps1)
contains the exact backend/import definitions, external include paths and
system link libraries. It directly links prebuilt LabradorEngine, MattMath,
DirectXTK and XAudio2Redist libraries, copying only the matching DirectXTK and
XAudio2Redist runtime DLLs beside the helper. It builds no engine source and
uses no game implementation or assets.

From an x64 VS Developer PowerShell at this checkout's root, with the existing
`x64-debug`/`x64-release` D3D11/XAudio2/XInput `x64-windows` builds already
available, the following builds both helpers and runs the four cases. For a
separate engine checkout, pass its source/build/dependency/runtime directories
to the same build recipe. Use a fresh output directory.

```powershell
$startupOutput = Join-Path (Get-Location) 'out/validation/startup-hidden-public'
if (Test-Path -LiteralPath $startupOutput) { throw 'Choose a fresh output directory' }
$startupEngine = (Resolve-Path 'external/labrador').Path
$startupRecipe = (Resolve-Path 'scripts/engine-reports/startup-visibility/build-startup-hidden.ps1').Path
foreach ($startupPreset in @('x64-debug','x64-release')) {
    $startupBuild = (Resolve-Path "out/build/$startupPreset").Path
    $startupCache = @{}
    foreach ($startupLine in Get-Content -LiteralPath (Join-Path $startupBuild 'CMakeCache.txt')) {
        if ($startupLine -match '^(?<key>[^#/:][^:]*):[^=]+=(?<value>.*)$') {
            $startupCache[$Matches.key] = $Matches.value
        }
    }
    $startupDependencies = Join-Path $startupCache.VCPKG_INSTALLED_DIR $startupCache.VCPKG_TARGET_TRIPLET
    $startupConfiguration = if ($startupPreset -eq 'x64-debug') { 'Debug' } else { 'Release' }
    $startupPresetOutput = Join-Path $startupOutput $startupPreset
    $startupExe = & $startupRecipe -EngineSourceDirectory $startupEngine `
        -EngineBuildDirectory (Join-Path $startupBuild 'external/labrador') `
        -DependencyDirectory $startupDependencies -RuntimeDirectory (Join-Path $startupBuild 'game') `
        -Configuration $startupConfiguration -OutputDirectory $startupPresetOutput
    foreach ($startupMode in @('windowed','fullscreen')) {
        $startupRun = Join-Path $startupPresetOutput $startupMode
        $startupWork = Join-Path $startupRun 'asset-free-work'
        New-Item -ItemType Directory -Path $startupWork -Force | Out-Null
        $startupLog = Join-Path $startupRun 'run.log'
        $startupError = Join-Path $startupRun 'run.err'
        $startupLaunch = @{
            FilePath=$startupExe; WorkingDirectory=$startupWork; WindowStyle='Hidden'
            PassThru=$true; RedirectStandardOutput=$startupLog; RedirectStandardError=$startupError
        }
        if ($startupMode -eq 'fullscreen') { $startupLaunch.ArgumentList=@('--fullscreen') }
        $startupProcess = Start-Process @startupLaunch
        $null = $startupProcess.Handle
        if (-not $startupProcess.WaitForExit(15000)) {
            Stop-Process -Id $startupProcess.Id -Force -ErrorAction SilentlyContinue
            throw 'Started helper timed out'
        }
        $startupProcess.WaitForExit()
        if ($startupProcess.ExitCode -ne 0 -or (Get-Item -LiteralPath $startupError).Length -ne 0) {
            throw "Hidden-creation helper failed: $startupPreset/$startupMode"
        }
        Get-Content -LiteralPath $startupLog
    }
}
```

The helper console and owned Application window remain hidden. Its source
makes no show, global input or foreground request. A timeout stops only the
helper process just started. Missing devices or dependency inputs are setup
failures, not evidence of later-show semantics.

## Measured result and limits

Both builds compiled. All four runs exited 0 with empty stderr and successful
owned-hidden-window and teardown checks. Debug and Release each recorded:

```text
HIDDEN_INITIALIZE fullscreen=0 owned=1 visible=0 assets_loaded=0 run_calls=0 client=1280x720 layout=1280x720 back_buffer=1280x720 backend=d3d11 api=Direct3D 11
PASS hidden_creation_and_teardown=1 later_show_tested=0 first_present_tested=0
```

Their fullscreen-option runs recorded the same dimensions with
`fullscreen=1` and the same PASS line. These are observed hidden dimensions;
the probe does not assert a monitor-sized hidden popup or exercise a later
fullscreen transition. The exact source SHA-256 is
`E4CE532B903884BC2B6A06D3CF228554E042251D1FDCD93E7F2BBB9C48DC6A1E`.

No later show, first visible pixels, first present, normal visible startup
timing, continuous foreground trace, hardware or human-comfort acceptance was
measured. Showing after asset validation alone does not establish a first
visible game frame: the current client's first state construction and first
render still follow that validation point.

## Optional API boundary

Consider an explicit later-show operation for an Application created hidden,
or document the supported public sequence. Document launcher show-command,
fullscreen sizing and focus/input effects; intentional hidden tools must not
be shown automatically. Existing native-handle access remains an alternative.

There is no implemented client later-show workaround for this request to
delete. Its value is keeping platform visibility policy at the shell boundary;
the maintainer may decide documenting native-handle usage is sufficient. The
separate hidden-window fullscreen-Apply issue does not establish a later-show
defect. Automatic initialization-time drawing/presenting, a loading card and
transition artwork are outside this request. An actual first-frame visibility
check remains necessary before declaring the client's startup gap closed.
