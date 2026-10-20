# Bug: windowed Apply shows an explicitly hidden application

Local **ENG#16** draft. **Unfiled.** Use Labrador's Bug or build problem form
when publishing; this document records no upstream issue or engine fix.

## Expected and actual behavior

`ApplicationOptions::visible=false` documents a hidden shell for smoke tests
and capture tools. Applying an already-windowed setting should retain that
deliberate hidden visibility.

At the checked revision, the public `Application::set_fullscreen(false)` call
sets the window's visible flag even when the application started hidden and
was already windowed. The standalone Debug and Release runs both measured:

```text
REPRODUCED hidden Apply: before_visible=0 after_visible=1 final_visible=0 offscreen=1 foreground=0
```

The visibility measurement is `IsWindowVisible`. The experiment intentionally
keeps its owned window outside the virtual desktop and hides it immediately
after sampling, so reproducing the flag change does not require an on-desktop
presentation.

## Checked revision and source path

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The entire engine worktree was
confirmed unchanged against that `FETCH_HEAD` before preparing these runs.

- [`ApplicationOptions::visible`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.h#L53)
  declares the hidden-window intent.
- [`Application::set_fullscreen`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.cpp#L248)
  unconditionally calls `Window::leave_fullscreen` for `false`.
- [Win32 `Window::leave_fullscreen`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/win32/window.cpp#L473)
  unconditionally calls `ShowWindow(handle, SW_SHOWNORMAL)`.

Refresh upstream immediately before filing.

## Minimal reproduction

The [synthetic exhibit](../../scripts/engine-reports/hidden_apply_exhibit.cpp)
uses the actual public Application call. It initializes the normal Windows
D3D11/XAudio2 shell with one worker/view, `visible=false` and `fullscreen=false`.
No manifest, assets, game objects or state are loaded, and no sound is played.
Initialization still requires that shell's supported graphics/audio environment.

The program checks ownership of its HWND, moves only that window outside the
measured virtual-desktop bounds and installs a current-thread CBT hook. The
hook vetoes `HCBT_ACTIVATE` only for that HWND. The program then calls
`set_fullscreen(false)`, samples visibility/foreground/bounds and immediately
hides the window before reporting or throwing. A cleanup guard also hides it
on exceptional paths. No global input or other window is changed.

The [build recipe](../../scripts/engine-reports/build-hidden-apply.ps1) compiles
only that source against prebuilt engine/math libraries, DirectXTK/XAudio2
import libraries and the normal Windows libraries. It copies the two runtime
DLLs beside the output and links no game object files.

From an x64 VS Developer PowerShell, after building the relevant engine
configuration, adapt these paths to the consumer's build layout:

```powershell
$exhibitBuild = 'out/build/x64-debug'
$exhibitOutput = 'out/validation/hidden-apply-debug'
$exhibit = & ./scripts/engine-reports/build-hidden-apply.ps1 `
    -EngineSourceDirectory 'external/labrador' `
    -EngineBuildDirectory "$exhibitBuild/external/labrador" `
    -DependencyDirectory "$exhibitBuild/vcpkg_installed/x64-windows" `
    -RuntimeDirectory "$exhibitBuild/game" `
    -Configuration Debug -OutputDirectory $exhibitOutput
$observed = Start-Process -FilePath $exhibit -WindowStyle Hidden -Wait -PassThru `
    -RedirectStandardOutput "$exhibitOutput/run.log" `
    -RedirectStandardError "$exhibitOutput/run.err"
Get-Content -LiteralPath "$exhibitOutput/run.log"
$observed.ExitCode
```

Use the corresponding Release paths and `-Configuration Release` for the
second configuration. RuntimeDirectory can be any directory containing the
matching `DirectXTK.dll` and `xaudio2_9redist.dll`.

**Actual result:** both retained runs compiled with MSVC **19.51.36256.0**,
C++20 and `/EHsc /W4 /WX /permissive-`, exited 0 and produced the exact line
above with empty stderr. Debug used `/MDd /Od /D_DEBUG`; Release used
`/MD /O2 /DNDEBUG`. The source bytes match SHA-256
`85C80A9737F16EDD584D5DD6484D76EEA3D07EEEB3A0EB935156C77E2937828F`.

Exit 0 means the visibility bug was reproduced with the isolation checks
passing. Exit 2 and `NOT_REPRODUCED` mean hidden visibility was retained and
the report should be rechecked or withdrawn. Setup or isolation failures exit
1. These checks remain active in Release. Foreground and bounds are sampled
before/after the call; the source does not claim a continuous OS-focus trace.

## Client workaround and requested behavior

A hidden capture client currently moves its window offscreen, prevents its
activation, applies the ordinary setting, hides it again and restores local
input activation. Preserving deliberate hidden visibility would remove this
guard around Apply.

Please retain the existing visibility policy when applying windowed mode,
including an already-windowed `false` call. The client continues to choose
when to show its window and whether to use fullscreen.

This reproduces only the already-windowed call. It establishes no fullscreen
enter/exit, DPI/monitor-transition, listening, physical-controller, human
readability or performance acceptance.
