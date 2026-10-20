# Request: optional host capture hook without State lifecycle changes

Local **ENG#10** draft. **Unfiled.** Use Labrador's Engine request form when
publishing. The earlier claim that a second draw is unavoidable is withdrawn:
the existing public APIs support capture from a final State's draw. Any host
hook is a convenience to justify against that alternative, not a missing
renderer readback implementation.

## Need and existing alternatives

A screenshot command wants a completed render composition without a second
walk through every state's draw code. The current client workaround performs
an out-of-band `begin_frame`, public stack `draw`, `submit` and
`read_back_buffer`, leaving that frame unpresented. That extra draw also
repeats client instrumentation, so the client excludes it from normal draw
and pacing intervals.

The [synthetic engine-only control](../../scripts/engine-reports/frame-readback/frame_readback.cpp)
first qualifies that redraw pattern, then qualifies a public-API alternative:
a non-covering final State draws after the world, calls `submit`, and reads
the completed buffer. The outer submit is idempotent; a comparison read after
it and `end_marker` preserves the captured image without another world draw.
Its only texture is a synthetic one-pixel RGBA resource constructed in memory;
no game asset or asset file is loaded.

This observer is visually non-covering but affects lifecycle. Pushing it
suspends the underlying State; its next update queues removal, is consumed by
the observer, then resumes the world after observer destruction. It must not
pop itself from `draw`, which lacks the update call's deferral. These are
integration costs of this particular composition pattern, not proof that
every client capture integration must change lifecycle. A client-owned truly
final draw path could place equivalent submit/read code directly there.

## Checked revision and contract

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC** as
`52e809a5a0229d37db4373faa7853df4c861a310`; the whole engine worktree was
subsequently confirmed unchanged against that `FETCH_HEAD`.

[`Application::render`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/application.cpp#L309)
is private and calls begin-frame, the stack draw, submit, marker end and
end-frame without a host callback in the post-submit/pre-end-frame interval.
That missing convenience does not make capture impossible. Public
[`StateContext::draw`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/core/state_context.cpp#L47)
orders the final observer after underlying drawing, and public submit is
idempotent. `Scene::draw` joins its pooled work before returning; the control
itself records only one view on the calling thread and does not qualify
pooled-worker runtime behaviour or arbitrary unfinished client tasks.

[`Renderer::read_back_buffer`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/render/renderer.h#L401)
already supplies synchronous, top-row-first RGBA between submit and
end-frame. It stalls for GPU copying. D3D11 stages/maps the copy and converts
row pitch/BGRA ordering; its end-marker is an advisory immediate-context PIX
operation. Null records draw requests without rasterizing and refuses pixel
readback. It is not a substitute for this pixel control.

## Build and run

The retained Debug/Release builds used x64 MSVC **19.51.36256.0**, C++20,
`/EHsc /W4 /WX /permissive-`, with `/MDd /Od /D_DEBUG` or
`/MD /O2 /DNDEBUG`. The [exact build recipe](../../scripts/engine-reports/frame-readback/build-frame-readback.ps1)
contains backend/import definitions, external headers, prebuilt LabradorEngine,
MattMath, DirectXTK and XAudio2Redist inputs and system libraries. It copies
only the matching two runtime DLLs beside the helper and builds no engine
source. The configured D3D11/XAudio2/XInput shell initializes real devices;
the example needs no asset files, while still requiring those devices.

From an x64 VS Developer PowerShell at this checkout's root, with matching
`x64-debug` libraries already built at the checked revision:

```powershell
$frameRecipe = (Resolve-Path 'scripts/engine-reports/frame-readback/build-frame-readback.ps1').Path
$frameBuild = (Resolve-Path 'out/build/x64-debug').Path
$frameCache = @{}
foreach ($frameLine in Get-Content -LiteralPath (Join-Path $frameBuild 'CMakeCache.txt')) {
    if ($frameLine -match '^(?<key>[^#/:][^:]*):[^=]+=(?<value>.*)$') {
        $frameCache[$Matches.key] = $Matches.value
    }
}
$frameDependencies = Join-Path $frameCache.VCPKG_INSTALLED_DIR $frameCache.VCPKG_TARGET_TRIPLET
$frameOutput = Join-Path (Get-Location) 'out/validation/frame-readback-debug'
if (Test-Path -LiteralPath $frameOutput) { throw 'Choose a fresh output directory' }
$frameExe = & $frameRecipe -EngineSourceDirectory (Resolve-Path 'external/labrador').Path `
    -EngineBuildDirectory (Join-Path $frameBuild 'external/labrador') `
    -DependencyDirectory $frameDependencies -RuntimeDirectory (Join-Path $frameBuild 'game') `
    -Configuration Debug -OutputDirectory $frameOutput
$frameWork = Join-Path $frameOutput 'asset-free-work'
New-Item -ItemType Directory -Path $frameWork | Out-Null
$frameLog = Join-Path $frameOutput 'run.log'
$frameError = Join-Path $frameOutput 'run.err'
$frameProcess = Start-Process -FilePath $frameExe -WorkingDirectory $frameWork `
    -WindowStyle Hidden -PassThru -RedirectStandardOutput $frameLog -RedirectStandardError $frameError
$null = $frameProcess.Handle
if (-not $frameProcess.WaitForExit(15000)) {
    Stop-Process -Id $frameProcess.Id -Force -ErrorAction SilentlyContinue
    throw 'Started readback helper timed out'
}
$frameProcess.WaitForExit()
if ($frameProcess.ExitCode -ne 0 -or (Get-Item -LiteralPath $frameError).Length -ne 0) {
    throw 'Frame-readback helper failed; retain stderr and compiler logs'
}
Get-Content -LiteralPath $frameLog
```

Repeat with `x64-release`, `Configuration Release` and a fresh output
directory. For a separate engine checkout, substitute its source/build,
dependency and runtime paths in the same recipe. Local Git attributes retain
LF for the exact source-hash guard. The helper keeps its owned window hidden
and makes no show, foreground or global-input request.

## Measured results and limits

Both builds compiled. Both runs exited 0 with empty stderr and these exact
three lines:

```text
FRAME_READBACK buffer=1280x720 first=red second=blue draws=3 updates=0 readbacks=4 end_frame_calls=0 app_run_calls=0 hidden=1 backend=d3d11
FINAL_STATE captures=1 additional_world_draws=1 outer_submit_unchanged=1 deferred_pop_depth=2 after_pop_depth=1 world_suspends=1 world_resumes=1 consumed_updates=1 world_updates=0
PASS out_of_band_stack_redraw=1 final_state_capture_without_redraw=1 rgba_oracles=3 saved_first_copy_unchanged=1 presented_or_displayed_tested=0
```

Two out-of-band captures produce full-buffer red and blue RGBA oracles. The
final-State capture uses one additional world draw, with an unchanged
post-outer-submit/end-marker comparison. Its next update leaves pop pending
at depth 2 until update return, then destroys the observer and resumes the
world at depth 1. The underlying world receives zero updates. Hidden ownership
and scope teardown checks pass, and all checks remain active in Release.

The fixture models the draw/submit/end-marker order using public APIs. It
does not invoke the normal Application loop, call end-frame/present, load
asset files, or change the game's screenshot implementation. No repeated
observer draws before an update, pooled Scene run, other-backend abandonment,
performance measurement or complete game integration is qualified.

Source SHA-256:
`5FA71174A19F9DF75871BBC4A22951088C75B60C9626237028DBA0C6ED2E33EF`.
Build recipe SHA-256:
`0707D2B96F92ADFE77FFFF2EC29F01B4B605F0F7EE17821843F6FD945B07AD81`.

## Submitted, presented and displayed

The portable interval identifies the submitted back buffer selected for a
subsequent present, not a previously displayed image. D3D11's `Present(1,0)`
can enqueue work or wait for synchronization/backpressure before discarding
the target. Neither CPU return nor preceding readback proves compositor
acceptance or scan-out. Reading after present is outside the portable
readback contract; these hidden, unpresented controls establish no display
outcome.

A callback would not make GPU copying asynchronous or remove the current
stall. An asynchronous implementation would need frame/copy lifetime and
delivery semantics. This report supplies no such implementation, continuous
capture pipeline or display-timing guarantee.

## Remaining optional convenience

Consider a one-shot host request after normal stack drawing/submission and
before end-frame if avoiding observer suspend/resume and update ownership
justifies it. Alternatively document the existing final-State/final-draw
pattern. The maintainer may decide that is sufficient without another API.

PNG encoding/naming, destinations, screenshot display preferences and when to
incur a capture stall remain client policy. The current redraw workaround
could be replaced through either route; it is not proven necessary. No
host-hook implementation, game integration, presented-frame guarantee or
engine defect is claimed here.
