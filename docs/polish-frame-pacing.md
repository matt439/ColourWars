# Frame pacing and the reference hardware (POL#25)

POL#25 is `DECISION`: arrange an attended measurement session and choose the
hardware baseline. The target is a consistent 60 fps on a recorded reference
machine, followed by an explicit supported hardware baseline. The capture and
analysis workflow below is implemented; it does not select that baseline.

## What is measured

The Debug `perf.capture <frames> <label>` command retains 1–36,000 chronological
rendered match intervals independently of the 240-frame overlay ring. The first
draw primes the clock. Each subsequent completed match draw contributes one row,
including draws with no simulation update and draws after multiple catch-up
updates. `wait frames` counts update hooks and is not used to count these samples.

`frame_ms` is the QPC wall time from one match draw's beginning to the next.
It includes intervening engine work, render submission and Present indirectly.
`update_ms` sums the game's Level updates before the current draw; `draw_ms`
times its layout refresh and draw-list recording, including joined render workers.
These components are diagnostics, not an exact sum of the interval. No GPU
timestamps, separate Present duration, compositor timing or scan-out is measured.
Since [engine report 14](review/engine-reports.md#14-no-frame-statistics-or-gpu-timing--accepted-narrowed-7344610)
`Application::frame_timings()` publishes the last completed frame's begin_frame,
submit and end_frame CPU wall times. The overlay and schema 2 capture now pair
these by exact application frame number with the pending match sample, after
the engine publishes it. The game-update and match-draw stopwatches remain:
engine update/draw timings cover additional work outside those scopes.
Engine phases can include GPU/display waits; they are neither CPU busy time
nor GPU timing, and need not sum to the draw-begin interval. GPU, compositor
and scan-out time stay unmeasured: the engine declined GPU timing.

Schema 2 CSV columns are `frame,application_frame,frame_ms,update_ms,draw_ms,`
`begin_frame_ms,submit_ms,end_frame_ms,updates,objects,projectiles`.
Completion requires positive consecutive application IDs and a completed timing
snapshot for every row, including the last row before export or scripted quit.
A missed/regressed/invalid snapshot interrupts recording; an unavailable phase
is blank, while a measured zero is valid. The overlay excludes unavailable
engine samples from phase statistics and prints their paired count. Capture
JSON retains `timing_scope: match_draw_begin_to_begin_cpu_wall_ms` and adds
`engine_timing_scope: matched_completed_application_frame_cpu_wall_ms` and
`engine_paired_frames`. The analyzer accepts historical schema 1 captures as
recorded, without inventing application IDs or engine-phase measurements.

CSV rows also retain simulation-update, collision-object and projectile counts.
The summary uses nearest-rank p50/p95/p99, mean, maximum, zero-update and catch-up
frame counts, and observed intervals above 1000/60, 1000/30 and 50 ms. Threshold
counts are observations, not a strict pass/fail test at 59.94/60 Hz. Aggregate
draws per second is sample count divided by summed intervals, never `1 / fixed dt`.

The per-player F1/Back readout (`GAP#5.4`, 2026-10-15) now labels simulation
`dt` separately and displays the current rendered interval's reciprocal. It
uses one main-thread snapshot shared by every pane, with no clock read or
mutation in worker drawing. Unavailable timing is `render fps: n/a`, including
Release's inert developer timing service. This diagnostic fix does not qualify
the visible, uncontended baseline retained by POL#25.

Validation (2026-10-15): Debug and Release builds and both full consumer suites
pass. Deterministic checks cover 30/60/120 FPS with zero, one and four simulation
updates, priming, timing gaps, reset and invalid values. The hidden D3D11 fixture
feeds 30/60 FPS snapshots into the real Level overlays in both builds and compares
the rendered timing text in all 1/2/4 panes; only the FPS pixels change while
simulation `dt`, repeated draws, replay state and RNG stay fixed. This injected
fixture does not enable developer timing in Release. Isolated real-bank startup
and graceful shutdown also pass in both builds. Evidence is under
`out/validation/debug-fps/`, including `verified-debug/` and `verified-release/`.

Sampling needs a live match at normal speed. Pause, resume count-in, focus loss,
covering pages, a changed clock, resize, rebuild and screenshot capture cannot
silently become part of a continuous recording. Interruptions retain a partial
capture marked `complete: false`; the analyzer rejects it. Memory is reserved
before sampling and files are written afterward. A capture is a script barrier,
so a following `quit` or rebuild waits for export.

## Repeatable diagnostics

From an x64 VS Developer shell with `VCPKG_ROOT` set:

```powershell
scripts/check-frame-pacing.ps1 -Preset x64-debug -LoadCondition shared `
  -LoadNote 'Other tests are running concurrently; host load is not isolated.' `
  -Python '<path to Python 3>'
```

The runner builds and links the actual Debug game library into a small diagnostic
shell, then uses the engine's ordinary Application loop and Present. It defaults
to hidden windows, takes no screenshots or GPU readbacks, and changes no native
focus, process priorities or unrelated processes. Every child has its own profile,
muted save, working directory and bounded runtime; timeout cleanup uses only that
child's retained process handle. The player's save is never used.

The default matrix is 1/2/4 players, Close Quarters, 1280×720 windowed, seed 439,
all seats driven by paint bots with sprayers, god mode and infinite ammunition,
120 warmup simulation updates and 600 measured rendered intervals. Aim/readiness,
view shake and ordinary effects stay enabled. Thread-pool limits retain the
Application's machine-dependent default unless explicitly overridden. This is a
repeatable paint workload, not a physical-controller or ordinary-ammunition test.

`-SkipBuild`, `-Players`, `-Frames`, `-WarmupUpdates`, `-Resolution`, `-MaxThreads`
and `-TimeoutSeconds` control the bounded run. `-CompileOnly` launches no workload.
`-Visible` explicitly opens the diagnostic window for a later attended run; the
runner still does not take foreground focus. A nonblank `-LoadNote` is required
for every capture. `-LoadCondition` is `shared`, `unknown` or operator-declared
`uncontended`; the tool does not infer quietness by inspecting other processes.

Under `out/validation/frame-pacing/<run>/<players>p/`, retain:

- `debug/perf/sample.csv` and `.json`: raw samples and exact completion status.
- `run-metadata.json`: revisions and dirty status, executable/source/content
  hashes, machine/driver information, selected renderer device and presentation
  policy, actual pixels, window/focus state, thread pool, preferences and workload.
- `summary.json`: statistics, input SHA-256 fingerprints and limitations.
- `console.log`, `run.log` and `run.err`: script progress and process diagnostics.

Metadata is finalized before analysis. The runner verifies the analyzer's input
fingerprints against the retained files. The analyzer always writes
`baseline_accepted: false`, even when a caller declares uncontended load.

For an attended capture in the ordinary Debug executable, a script can use:

```text
players 4 paint
bot 1 paint
weapon all sprayer
seed 439
intro off
god 1
ammo inf
stage close_quarters
wait match
wait 120
perf.capture 600 cq4
quit
```

Its CSV/JSON are in `%LOCALAPPDATA%\ColourWars\debug\perf\`. Record machine,
display, load and build metadata separately when using this direct route.

## Shared-load diagnostic record (2026-10-15)

**Measurement support verified; baseline not accepted.** Another project's
test work was authorized concurrently on this PC. Host load was not isolated or
controlled. No other ColourWars build/test ran during the three captures.

Recorded machine: Windows 11 Pro build 26200, Ryzen 9 9950X3D
(16 cores/32 logical processors), OS-reported 63.6 GiB RAM, selected D3D11
NVIDIA GeForce RTX 5080, driver 32.0.16.1692. The pool used 1–32 threads.
The actual client was 1280×720, windowed and hidden; native visibility was false,
logical activation true, keyboard focus false and synthetic activation false.
DXGI requested sync interval 1; no reported/scan-out interval was available.

Each workload used the defaults above and retained exactly 600 rendered intervals:

| Players | Interval p50 ms | p95 ms | p99 ms | Maximum ms | Level updates | Catch-up frames |
|---|---:|---:|---:|---:|---:|---:|
| 1 | 16.668 | 16.833 | 16.905 | 17.622 | 600 | 0 |
| 2 | 16.669 | 16.866 | 16.929 | 17.546 | 600 | 0 |
| 4 | 16.666 | 16.843 | 16.943 | 33.497 | 602 | 2 |

These raw CPU wall-time observations qualify neither visible pacing nor a
supported hardware floor. All summaries retain `baseline_accepted: false`.
The four-player long interval is retained, without attributing it to the game
or the concurrent workload. Evidence, including immutable metadata, exact raw
rows and input fingerprints, is under
`out/validation/frame-pacing/shared-load/44bc6f29032642cea66ed2667cfb0068/`.

Correctness validation: Debug and Release build under `/W4 /WX` and pass all
23 consumer acceptance cases. Deterministic tests cover exact sample counts,
chronological captures past the overlay ring's wrap, zero/catch-up updates,
population snapshots, prime frames, completion, interruption, invalid clocks,
reset and command grammar. Five analyzer tests cover quantiles, threshold
counts, input rejection, fingerprints and refusal to accept a baseline.
The hidden D3D11/XAudio2 state-stack probe also rejects capture during Pause,
resume count-in and focus loss without starting a recording. Its run is
`out/validation/frame-pacing/readiness-debug/x64-debug/71f5370eea2545c0a4e5301428ae4675/`;
consumer logs are under `out/validation/frame-pacing/` and `out/acceptance/`.
Isolated Debug and Release real-bank startup and graceful shutdown also pass.
The engine pin and runtime content are unchanged.

## Completed engine-phase pairing (2026-10-24)

DBG#5 is implemented. Debug and Release game/editor builds and the focused
`debug_console` and `story_audio` consumer cases pass, including Release's
developer-code exclusion. Thirteen analyzer tests cover both schemas and
invalid/missing/duplicate/regressed engine frames. CPU fixtures cover exact
pairing, delayed publication, final one-frame capture, interruptions, zero
phases and ring wrap.

Six hidden real-Application captures pass: targets 1 and 32 rendered intervals
for each of 1P, 2P and 4P. Every exported row has its exact positive consecutive
application frame ID and completed engine phases. These bounded shared-load
runs verify capture correctness; every summary retains
`baseline_accepted: false`. Evidence and input fingerprints are under
`out/validation/perf-timings-20261024/actual-target1/` and `actual-target32/`;
`retained-evidence-reconciliation-v1.json` verifies their source, binary, content
and raw-input pins alongside the build/CPU/Python receipts.

The Debug native alpha leaf passes 389 readbacks, 190 rows and 16 crops,
including the eight-line perf panel against independent geometry/text
references. Its first run caught a reference height still fixed at five lines;
the source-derived correction to eight retains the same comparison rules and
budgets. Both attempts remain under `native-alpha/` and `native-alpha-v3/`.
No GPU timing, human readability or visible performance acceptance is claimed.

## Remaining acceptance

A hidden/occluded window can make Present return rapidly. Those intervals do not
qualify visible display pacing, and Debug instrumentation does not establish
Release performance. Shared or unknown host load cannot serve as an uncontended
acceptance baseline. The current Close Quarters content revision is also an
explicit input; repeat qualification when the accepted reference composition changes.

Run attended, visible reference workloads after the concurrent test work is over,
record the chosen build, monitor/refresh, resolution, hardware, content and input
conditions, and review long intervals and crowded effects against 60 fps. Use a
measurement path appropriate to the build being qualified. Repeat representative
matches, including ordinary combat/elimination/respawn as well as paint-heavy
play, and establish the supported CPU/GPU/RAM/display floor from that evidence;
one high-end machine cannot establish a minimum specification. No minimum hardware
or performance acceptance is claimed by this increment.

These captures also decide whether `GAP#L9` resumes: its Debug receipt showed a
~33.5 ms, two-update interval once a second on uncontrolled load. If a visible,
uncontended capture repeats it, record it in the
[presentation-rate slice](feature-roadmap.md#presentation-rate-inspection-slice-gapl9).
