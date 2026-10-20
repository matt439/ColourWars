# CPU elapsed phase statistics for the Application frame loop

Local **ENG#14** feature-request draft. **Unfiled.** The manual public-call
example is verified; the requested Application statistics do not exist yet.
No engine fix or hardware performance qualification is recorded here.

## The missing observation

ColourWars has a client perf overlay and chronological frame capture, but can
time only its own Level updates and match draw. Its draw-to-draw wall interval
includes intervening submit/Present delays indirectly; it cannot separate those
calls or observe all Application work. The public Application API offers no
completed-frame timing snapshot, and Renderer debug markers return no timings.

Upstream master was freshly checked on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The engine worktree was also compared
with that FETCH_HEAD before preparing these examples. Relevant boundaries are:

- [`Application::tick/update/render`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.cpp#L282):
  fixed-step updates, device polling/audio, begin-frame, state draw, submit and
  end-frame are owned here. A render iteration can contain zero or multiple
  fixed updates; target simulation dt is not elapsed rendered-frame time.
- [Public Application API](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.h#L123)
  has no frame-statistics accessor.
- [Public Renderer frame calls](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/render/renderer.h#L349)
  permit manual bracketing by a caller that owns its loop, but do not expose
  Application's completed-frame statistics.
- [D3D11 submit](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/render/d3d11/renderer.cpp#L989)
  finishes, submits and releases command lists. Its CPU call duration is not
  GPU execution duration.
- [D3D11 Present](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/render/d3d11/device_resources.cpp#L390)
  may return early or block on presentation backpressure. Its CPU elapsed time
  is neither a display interval nor a scan-out timestamp.

Refresh upstream unconditionally before filing.

## First useful request

Please provide a small read-only last-completed-Application-frame CPU elapsed-
wall snapshot, or an equivalent supported frame-loop observation seam. The
proposed data and semantics are:

- A stable frame sequence and explicit valid/unavailable state, unchanged by a
  capture client's manual Renderer use outside Application's normal loop.
- An elapsed interval with its boundaries documented, distinct from fixed
  simulation dt and from the sum of measured phases.
- Aggregate Application-update duration and engine fixed-update count, including
  zero/multiple updates. State whether timer bookkeeping is included; actual
  update calls also include input polling and audio work.
- Prepare/record duration, including begin-frame clear/reset and state draw;
  submit-call duration; and end-frame-call duration. Name the last scope honestly
  if it includes housekeeping beyond the native Present call.
- Documented behavior before the first sample, after timing discontinuities,
  and for failed or abandoned frames.

CPU-observed elapsed wall time can include waits; it is not CPU busy time.
Phase sums need not equal an interval containing message dispatch, scheduling
and other work. Summed worker durations must not be presented as elapsed frame
time. GPU timestamps, scan-out, sprite/particle counters, distributions, an
engine-drawn overlay and hardware timing targets are outside this first slice.

This is the observability caller for the deferred engine **REV#10** idea,
without requesting its whole overlay.

## Verified manual public-call example

The [standalone source](../../scripts/engine-reports/frame_phase_boundaries.cpp)
creates a real hidden Application with one worker/view, then drives a separate
actual StateContext and public Renderer through three manual empty-view
iterations. It uses steady_clock around its own state update, prepare/record,
submit and end-frame calls. No manifest, game assets, save, Application state
or Application loop is used, and no sound is played. The configured normal
D3D11/XAudio2/XInput shell still initializes, so this is asset-free, not
device-free. It uses no fabricated accessor or compile-fail test.

Both Debug and Release compiled with MSVC **19.51.36256.0**, C++20 and
`/EHsc /W4 /WX /permissive-`, exited 0 and had empty stderr. Debug used
`/MDd /Od /D_DEBUG`; Release used `/MD /O2 /DNDEBUG`. Each output has exactly
six lines: the scope line, CSV header, three observed rows and final PASS:

```text
MANUAL_PUBLIC_SEAM backend=d3d11 hidden=1 assets=0 app_run_calls=0 app_phase_stats_observed=0 gpu_timed=0
manual_iteration,state_update_cpu_wall_ms,prepare_record_cpu_wall_ms,submit_call_cpu_wall_ms,end_frame_call_cpu_wall_ms
```

```text
PASS manual_context_updates=3 manual_draws=3 application_frame_interval_measured=0 timing_thresholds=0
```

The three rows contain finite nonnegative raw CPU-observed elapsed durations.
Their transient values are retained as evidence, not published as meaningful
benchmark numbers. No accuracy, precision, timing threshold, frame-rate target,
GPU completion or visible uncontended performance qualification follows.

The example illustrates public call boundaries only. It **does not measure
Application's internal phases or frame interval**, and does not establish a
missing accessor by runtime failure. Header/source inspection establishes the
API gap. A manual empty view is not a representative game workload.

The exact executed source SHA-256 is
`85634C09EE0B1E7869107456D3052C388A727868E077735D21B9D6942FB4B170`.
The [public build recipe](../../scripts/engine-reports/build-frame-phase.ps1)
is byte-identical to the executed private recipe, SHA-256
`8630A3C3A4FEB819939BB85FCD9F668B55FF2A304F5A31B122D89D65E063BF72`.
It compiles only the source against matching prebuilt engine/math libraries,
DirectXTK/XAudio2 imports and normal Windows libraries, copying two runtime
DLLs beside the output. No engine configure/build or game objects are needed.

From an x64 VS Developer PowerShell, after building the engine configuration:

```powershell
$phaseBuild = 'out/build/x64-debug'
$phaseOutput = 'out/validation/frame-phase-debug'
$phaseExe = & ./scripts/engine-reports/build-frame-phase.ps1 `
    -EngineSourceDirectory 'external/labrador' `
    -EngineBuildDirectory "$phaseBuild/external/labrador" `
    -DependencyDirectory "$phaseBuild/vcpkg_installed/x64-windows" `
    -RuntimeDirectory "$phaseBuild/game" -Configuration Debug `
    -OutputDirectory $phaseOutput
$phaseChild = Start-Process -FilePath $phaseExe -WindowStyle Hidden -PassThru `
    -WorkingDirectory $phaseOutput `
    -RedirectStandardOutput "$phaseOutput/run.log" `
    -RedirectStandardError "$phaseOutput/run.err"
$null = $phaseChild.Handle
if (-not $phaseChild.WaitForExit(15000)) {
    Stop-Process -Id $phaseChild.Id -Force
    throw 'Manual phase illustration timed out.'
}
$phaseChild.WaitForExit()
$phaseChild.ExitCode
```

Use corresponding Release paths and `-Configuration Release` for that build.
RuntimeDirectory can be any directory with the matching `DirectXTK.dll` and
`xaudio2_9redist.dll`. Source/recipe hashes should be checked before execution.

## What the client could delete

A completed-frame observation could replace the paired match-draw stopwatch
and associated draw-begin/last-begin measurement state. It could replace the
nested Level-update stopwatch if the overlay explicitly changes to Application-
update semantics. Engine update counts cannot replace the game's separate
DebugClock/Level-update counts, since several Level updates can occur inside
one Application update.

The draw ingestion hook may remain for eligibility and sequence deduplication.
The perf ring, capture/export, object counts, worker-safe interval snapshot and
overlay remain client code. QPC remains useful for level-build timing. Manual
screenshot redraws would no longer invent extra engine-frame samples, but the
client still needs screenshot-gap/continuity policy: screenshot work inside an
Application update can contaminate that phase's elapsed time. Its `capturing_`
flag also controls screenshot/status behavior. No blanket deletion of those
guards is promised.

The retained local receipts are under
`out/validation/crossfeed-20261020/frame-stats-report-final-v1/`, including
`result.json`, exact compile arguments/logs and both stdout/stderr sets. This
manual example does not complete the broader ENG#14/DBG#5 GPU-timing wishes.
The request remains unfiled; no engine implementation has been changed.
