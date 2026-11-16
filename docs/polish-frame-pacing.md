# Frame pacing and the reference hardware (POL#25)

POL#25 asked for an attended measurement session and a chosen hardware
baseline. The author **waived both on 2026-11-05; they were not performed**, so
no performance capture has passed and no supported hardware baseline exists. The
target stays a consistent 60 fps on a recorded reference machine. The capture
and analysis workflow below is implemented; it does not select that baseline.

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
mutation in worker drawing. Unavailable timing is `render fps: n/a`; the
readout is Debug-only since `REV#13`. This diagnostic fix does not qualify
the visible, uncontended baseline that POL#25 waived (2026-11-05, not performed).

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

## Static paint draw cost (`REV#7`, 2026-11-13)

`StructurePaintable::draw_paint` submits a surface and an edge sprite for
every owned 4-unit tile, plus a highlight while it is wet and a splash while one
plays, in every pane that sees the structure (structures cull whole, not tile
by tile). The
[project review](review/2026-11-10-project-review.md#static-paint-draws-one-tile-at-a-time-rev7)
asked whether that shows before anyone merges runs. **It shows, and it fits.**

Method: the `REV#38` four-bot Crossfeed match
([paint strip weight](polish-match-accents.md#paint-strip-weight-in-the-reveal-rev38)),
Debug build `12dd21a3`, D3D11, 1920x1080 windowed (shown; whether it was in
front was not recorded), four panes, seed 7. The
isolated profile's save set the size and, for the main runs, `vsync` off, so
Present never waits for the display and the interval is the work. Nothing else
ran on MattPC (Ryzen 9 9950X3D, RTX 5080, driver 32.0.16.1692, 59 Hz display):

```
ColourWars.exe +script.background 1 +overlay none +seed 7 +bot 1 paint +players 4 paint
  +stage crossfeed +wait match +wait 1s +checksum +perf.capture 600 start +checksum
  +timescale 4 +wait 214s +timescale 1 +wait 1s +checksum +perf.capture 600 late +checksum +quit
```

Crossfeed has 2,590 paint tiles (124,320 units at 48 a tile). The early
capture began at 12.5% painted (325 tiles) and ended at 23% (vsync off) or 59%
(vsync on, ten seconds); the late one held 95% (about 2,455 tiles). Each capture
is 600 rendered intervals; milliseconds are CPU wall time, p50 / p95:

| Run | Paint | frame | draw | submit | end_frame |
|---|---|---:|---:|---:|---:|
| vsync off, A | 12-23% | 4.51 / 6.29 | 4.04 / 5.39 | 0.06 / 0.11 | 0.06 / 0.10 |
| vsync off, A | 95% | 6.85 / 9.08 | 6.40 / 8.38 | 0.06 / 0.08 | 0.06 / 0.09 |
| vsync off, B | 12-23% | 2.99 / 4.07 | 2.71 / 3.68 | 0.03 / 0.04 | 0.03 / 0.05 |
| vsync off, B | 95% | 5.59 / 7.64 | 5.23 / 7.19 | 0.04 / 0.06 | 0.04 / 0.07 |
| vsync on | 12-59% | 16.65 / 16.96 | 3.38 / 5.02 | 0.05 / 0.07 | 12.63 / 13.80 |
| vsync on | 95% | 16.66 / 16.93 | 4.87 / 7.03 | 0.05 / 0.07 | 11.11 / 12.14 |

Painting the arena adds 2.4-2.5 ms to the median Debug draw, and the interval
follows it: about 1.2 µs per owned tile per frame across four panes. In the
vsync-off runs update time, collision objects and projectiles do not change
between the captures, so the difference is the paint. Runs A and B differ by 1.2 ms at both moments
(host variance between identical launches), but not in the delta. The GPU is
not the limit: with vsync off, `end_frame` (which includes Present) stayed at
0.03-0.06 ms while the game drew 146-334 frames a second, so the RTX 5080
finished each frame inside the CPU's interval. Its own time is not measured.
With vsync on, a fully painted four-pane frame leaves about 11 ms of the
16.7 ms spare. The 10 and 13 intervals over 33.3 ms in those captures are the
once-a-second pattern of `GAP#L9`, present before the paint too.

This is Debug, with checked iterators and no optimisation; `perf.capture` and
launch arguments do not exist in Release, so Release was not measured and
would draw the same sprites for less CPU. Merging runs is therefore not worth
moving the painter pins for: the review's objection holds and the draws stay
per tile. Look again if a stage much larger than Crossfeed, or an eight-pane
mode, arrives. The raw CSVs were not kept.

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
or performance acceptance is claimed by this increment. The author waived this
session on 2026-11-05 and it was not performed; this paragraph stays as the
method for whoever runs it later.

These captures were also to decide whether `GAP#L9` resumes; the author
accepted the vsync setting as built on 2026-11-05 without them. Its Debug receipt showed a
~33.5 ms, two-update interval once a second on uncontrolled load. If a visible,
uncontended capture repeats it, record it in the
[presentation-rate slice](feature-roadmap.md#presentation-rate-inspection-slice-gapl9).
