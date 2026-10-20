# Request: DPI-correct client-size conversion for an opted-in application

Local **ENG#17** request draft. **Unfiled.** Use Labrador's engine-request
route after review and a fresh upstream check. No engine change, scaled-display
failure reproduction or OCA-66 acceptance is recorded here.

## Caller and current source

ColourWars declares per-monitor-v2 awareness in its executable manifest and
checks that declaration in its startup probe. Selecting process awareness is
the executable's choice. A manifest is Microsoft's recommended route;
[an API call before creating UI is also supported](https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process).

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC** as
`52e809a5a0229d37db4373faa7853df4c861a310`, and the engine worktree was checked
against that tree. At this revision:

- [Win32 `outer_size_for_client`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/win32/window.cpp#L196)
  uses `AdjustWindowRectEx` for creation and the ordinary windowed resize paths.
- [The window procedure](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/win32/window.cpp#L510)
  has no `WM_DPICHANGED` case. Public Window/WindowNotify expose no DPI query or
  DPI-change notification.
- [Application's existing size notification](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.cpp#L440)
  already adopts actual client dimensions in layout and renderer state.

Microsoft says
[AdjustWindowRectEx is not DPI aware](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectex)
and should not be used by per-monitor-aware callers. Its
[DPI-explicit counterpart](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectexfordpi)
accepts the DPI for frame arithmetic. This establishes a source/API mismatch
for an opted-in client, without establishing a pixel error on an untested
monitor. Refresh upstream immediately before filing.

## Minimal baseline and measured results

The [synthetic source](../../scripts/engine-reports/window-dpi/dpi_window_baseline.cpp)
and its [small awareness manifest](../../scripts/engine-reports/window-dpi/dpi_window_baseline.manifest)
initialize the actual hidden, windowed D3D11/XAudio2 Application with one
worker/view. No game objects, resource manifest, assets or state are loaded,
no sound is played, and no loop or frame rendering starts. Initialization
still needs the normal shell's graphics/audio services and runtime DLLs.

The probe verifies ownership and per-monitor-v2 context, reads the HWND's
effective DPI and client size, records layout/back-buffer dimensions, and
checks hidden/nonforeground state and scope teardown. It also queries legacy
rectangle arithmetic and `AdjustWindowRectExForDpi` at four supplied DPIs for
that ordinary no-menu window style. The calls change no DPI, display or window.

Both Debug and Release compiled with MSVC **19.51.36256.0**, C++20 and
`/EHsc /W4 /WX /permissive-`, exited 0 with empty stderr, and produced the same
seven normalized stdout lines. The actual HWND reported **96 DPI**, a
**1280x720** client matching the request, and **1280x720** layout/back buffer.
Rectangle results on this host were:

| API calculation | Outer extent | RECT offsets |
|---|---|---|
| legacy, no DPI argument | 1296x759 | -8,-31,1288,728 |
| explicit 96 DPI | 1296x759 | -8,-31,1288,728 |
| explicit 120 DPI | 1298x767 | -9,-38,1289,729 |
| explicit 144 DPI | 1302x776 | -11,-45,1291,731 |
| explicit 192 DPI | 1306x791 | -13,-58,1293,733 |

The source SHA-256 is
`354DC9B8580648F729165AC6C2954BA35F2D95D366116079365F362BBBD1EA7B`.
The manifest matches its frozen pre-run SHA-256
`EB42B8F3DFE4A415479DC84E7066D48CD41DD66BF6D3B05FA088F4ECED02AAE9`;
the runtime assertion confirms that the owned HWND has the declared context.
Local receipts, compiler arguments/logs and process outputs are retained under
`out/validation/crossfeed-20261020/dpi-report-final-v1/`.

This is a **100% desktop baseline and arithmetic exhibit**. A 144-DPI query is
not a 150% monitor run. Exit 0 means these baseline checks passed; the final
line explicitly records `scaled_monitor_tested=0`, `dpi_transition_tested=0`
and `pixels_tested=0`. There are no display-setting changes, monitor moves,
synthetic DPI messages, direct show/resize/activation calls, input injection
or general message pumping. Foreground state is sampled, not continuously traced.

## Build and run

The [build recipe](../../scripts/engine-reports/window-dpi/build-exhibit.ps1)
compiles one source against prebuilt engine/math and normal Windows/import
libraries, copies the two runtime DLLs and embeds the dedicated manifest.
It compiles no game objects and makes no new engine configuration.

From an x64 VS Developer PowerShell, adapt these existing build paths:

```powershell
$build = 'out/build/x64-debug'
$output = 'out/validation/window-dpi-debug'
$exhibit = & ./scripts/engine-reports/window-dpi/build-exhibit.ps1 `
    -EngineSourceDirectory 'external/labrador' `
    -EngineBuildDirectory "$build/external/labrador" `
    -DependencyDirectory "$build/vcpkg_installed/x64-windows" `
    -RuntimeDirectory "$build/game" -Configuration Debug `
    -OutputDirectory $output
$process = Start-Process -FilePath $exhibit -WindowStyle Hidden -Wait -PassThru `
    -RedirectStandardOutput "$output/run.log" `
    -RedirectStandardError "$output/run.err"
Get-Content -LiteralPath "$output/run.log"
$process.ExitCode
```

Use matching Release paths and `-Configuration Release` for the second run.
The recipe uses `/MDd /Od /D_DEBUG` for Debug and `/MD /O2 /DNDEBUG` for
Release, with Windows 10 API declarations. Require exit 0 and empty stderr;
do not fix theme-specific frame margins into a portable acceptance oracle.

## Request and policy boundary

Document the Windows awareness decision and supported behavior for a caller
that opts in, as already tracked by G151/OCA-66. Convert explicit client-pixel
requests using the applicable window/target-monitor DPI, and continue reporting
the resulting client size through the existing notification.

Define the DPI-transition behavior too. Microsoft's
[WM_DPICHANGED contract](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged)
supplies a suggested size/position and recommends applying it. Always retaining
a fixed requested physical-pixel size is a different policy, not the only
correct handler. If the caller needs to choose logical-size versus physical-
pixel behavior, document or expose that choice. The game's initial apparent
size, UI scale and font coverage remain client policy.

A portable DPI query/notification can be a separate request when a concrete
caller needs it. This Windows-only client can already query
`GetDpiForWindow` through its native HWND; the lack of a portable accessor does
not make the value unobtainable.

No existing client workaround is deleted by this request, and the executable's
awareness manifest stays. Real scaled/mixed-DPI sizing, transition behavior and
text sharpness remain unverified. The engine template's attended 150% review
and awareness-default decision remain open under OCA-66.
