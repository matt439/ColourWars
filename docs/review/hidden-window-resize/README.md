# Hidden-window resize activation request (ENG#23)

This prepared report remains **unfiled**. `sanitized-request.md` is the small
request; `hidden_resize_exhibit.cpp` is its synthetic native example.

The example constructs the real Labrador `Window` and requests an 800x600
client area while hidden. It creates no Application, renderer, audio device,
content, state or game objects. A Windows desktop and real HWND are required.
A current-thread CBT hook is installed before construction and kept through
destruction. It records and vetoes only this process/thread's activation/focus
requests. It sends no input and never changes another window or security setting.
The pre-existing foreground HWND is compared as an opaque value and never logged.

From an x64 Visual Studio Developer shell, using the normal static engine build
and matching its Debug/Release runtime:

```powershell
.\build-exhibit.ps1 -EngineSourceDirectory '<Labrador checkout>' `
  -EngineBuildDirectory '<selected engine build directory>' `
  -OutputDirectory '<new output directory>' -Configuration Debug
```

For this consumer's build, the engine build directory is
`out/build/x64-debug/external/labrador`; for a standalone engine it is the
chosen build root containing `engine/LabradorEngine.lib`. Use Release with
its corresponding build. The script compiles only. Run the emitted executable
from an existing terminal to read its one JSON result; no visible probe window
is needed. Automated callers should use a console-free child process.

Exit 0 means an activation request was observed and vetoed, with matching
client size and sampled isolation/foreground checks. Exit 2 is inconclusive:
no activation request, unavailable/changed foreground baseline, or mismatched
client size. Focus requests are counted independently. Exit 1 is setup or
isolation failure. There is no unguarded negative control or measured takeover.

The initial runs used Windows x64 (reported OS version 10.0.26200) and MSVC toolset 14.51.36231.

On 2026-10-24 the checked engine pin and upstream master were both
`19cf650a6aa5569125dac577a2281561f5a215f4`. Debug and Release initial guarded
runs each vetoed one activation request, counted zero focus requests, preserved
the foreground after resize and destruction, and reported exactly 800x600.
Source, library, compiler/cache and binary pins remain in the local validation
receipts under `out/validation/hidden-resize-20261024/`. This establishes no
GPU/audio, physical-input, human-readability or performance acceptance.

Recheck upstream before any future filing. Keep the existing five client
activation guards until an engine answer is consumed and the hidden-window
regressions are requalified. This task changes no engine implementation.
