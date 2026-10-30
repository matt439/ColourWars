# First polish increment: camera and movement foundation

Implemented on 2026-10-08 against game revision `2387736`, using the existing
Labrador pin. This begins sections 1–2 of [the polish plan](polish-plan.md).
It does not close either section's visual or playtest exit condition.
The display and movement baseline below records that first increment. The
[2026-10-20 camera follow-up](#camera-follow-up-2026-10-20) and
[coupled P3 comparison](polish-crossfeed.md#coupled-movement-and-five-tool-comparison-2026-10-20)
own the current candidates.

## Display baseline

`presentation.json` now authors `camera.reference_height = 1080` world units for
the full display. The camera uses the engine's world-space follow helper with
the visible world extent, rather than passing physical pane pixels as world units.
At 16:9, the following extents hold at 720p, 1080p and QHD:

| Layout | World extent per player pane |
|---|---|
| One player | 1920 × 1080 |
| Two players | 1920 × 540 |
| Three or four players | 960 × 540 |

This preserves the former 1080p framing. Other aspects preserve vertical extent
and expose more or less horizontal world; gameplay adds no letterboxing. Odd
window dimensions round pane edges to physical pixels, so pane extents can differ
by one pixel. Camera bounds, scene culling and mouse aim use that same pane.
A view larger than the arena is centred on that axis, avoiding alternating clamps.
The final overview continues to fit its complete authored bounds independently.

Camera scales are 2/3, 1 and 4/3 at the three reference resolutions. The existing
point sampling is retained, with continuous simulation aim and weapon rotation.
This is a framing baseline, not an integer-pixel rendering policy: fractional
camera placement, rotation, texel density and the mixed existing artwork still
need the planned pixel-art composition and motion review. No sprite grid, new
font, character silhouette or final art treatment is approved by this increment.

## Camera follow-up (2026-10-20)

The selected Crossfeed/Course framing candidate adds optional
`camera.minimum_world_width = 1280`, `horizontal_lead_seconds = 0.25` and
`horizontal_lead_max_distance = 200`. Missing or zero values preserve the
original policy above. The scale begins at display-height/reference-height,
then decreases uniformly if necessary to show the minimum horizontal world.
Lead shifts only horizontal follow focus, bounded by the distance cap and a
quarter of visible width. It is disabled for dead/zero-health players known
at camera refresh, and mouse projection still uses the physical player.

At 16:9 the current 1/2P extents remain 1920 by 1080 / 1920 by 540; 3/4P panes
show 1280 by 720. At 720p, their character height changes from 80 to 60 pixels.
The measured running landing now exposes its body-width strip seven ticks
before takeoff, compared with seventeen ticks after under the original camera.
This corrects geometric exposure, while comfort, recognition and human warning
remain open. The [comparison](polish-crossfeed.md#narrow-pane-framing-comparison-2026-10-20)
retains the lead-only alternative, rear-view budgets, native observations and
the late-contact update-order limit. This remains client policy, with no engine
API or submodule change.

## Movement baseline

`tuning.json` now contains `player.jump_buffer_time = 0.12` seconds and
`player.ledge_grace_time = 0.10` seconds. Zero disables each allowance; invalid
negative, non-finite or greater-than-one-second values are rejected at startup.
Existing speed, acceleration, gravity and jump-extension tuning are unchanged.

A jump pressed shortly before landing is consumed after the complete contact
phase on that landing tick. A released early tap still produces a short jump.
Ground support refreshes ledge grace; takeoff consumes it, preventing a second
air jump. Holding jump does not repeat jumps on later landings.

Pause, focus loss, reconnect suspension, death, respawn and platform drops clear
queued jumps and grace. A neutral jump input tick rearms jumping after those
transitions; holding an old button cannot start an extra jump. Pausing during an
existing held jump preserves its continuation when play resumes.
A fresh simultaneous jump/drop keeps the existing jump precedence. A held drop
on a pass-through landing overrides an older airborne jump buffer. Fatal contact
now prevents one final movement tick, and respawn clears previous collision state.

The actual Player/Scene acceptance fixture measures these same-height jumps at
the engine's 0.0166666-second timestep and maximum horizontal speed of 800:

| Input | Apex above launch | Time to land | Horizontal travel |
|---|---:|---:|---:|
| One-tick press, immediately released | 95 | 0.633331 s | 506.665 |
| Held jump | 207.472 | 1.01666 s | 813.33 |

These are measured trajectories, not safe-gap specifications. They exclude
acceleration from rest and player input variation. The course's 180, 400 and
560-unit gaps were traversed in both directions through the actual level scene
with running held jumps and horizontal braking beyond the destination lip.
Continuous full-speed input can overshoot a short landing and enter the next gap.
That traversal ran in a scratch probe. Retained route trials, including recovery
steps and staircases, have since replaced it; see
[the course document](polish-movement-course.md#route-trials-2026-10-20-provisional-profile).

## Playable course

Choose **Single Match → 1 Player (practice) → Movement Course** to try the
[course](polish-movement-course.md). It also supports the existing multiplayer
setup for split-screen trials. It uses ordinary timed match rules and placeholder
geometry; the optional untimed workshop is a later experiment. The three existing
arenas are unchanged. `include_in_random: false` keeps the course out of Random;
levels without that metadata retain their previous eligibility.

## Evidence and limits

Retained consumer acceptance now covers camera framing/resize/aim, movement
transitions and Random-stage eligibility alongside ammunition, paint and saves.
The editor check also saves/reopens the course and launches its saved copy.
Run from an x64 Visual Studio developer shell:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-level-editor.ps1 -Preset x64-debug
./scripts/check-level-editor.ps1 -Preset x64-release
```

Both Debug and Release builds and all six consumer cases passed on 2026-10-08.
The two builds produced identical movement measurements. The existing three
arenas retained exposed paint areas of 205920 (King of the Hill), 249000
(Turbulence) and 91200 (Close Quarters).
Both editor configurations passed document round trips and hidden launch/open/
responsive-window/graceful-close checks, including the saved course. Debug game
startup passed with an isolated save. The 42-file Release ZIP passed inspection
and extracted startup with a system-only PATH, isolated save, absent-bank silent
fallback and graceful shutdown. It contains the course and no private audio or
development executables. The separate clean-Windows deployment check remains open.

Local evidence is under `out/validation/polish-foundation-20261008/` and
`out/acceptance/<preset>/`. The scratch `scene.cpp` runs the actual LevelBuilder
and Scene through spawn settling, active play, pause/resize and final overview.
Its D3D11 device reported **NVIDIA GeForce RTX 5080**. Captures cover Close
Quarters and the course with 1–4 views at 1280×720, 1920×1080 and 2560×1440;
the paused/overview resize uses 1600×1200. Teams alternate blue/orange with
sprayer/roller, using scripted input slots. This is not physical-controller or
crowded-combat evidence. No frame-pacing claim or hardware minimum is established.

The inspected four-player 720p capture keeps character silhouettes, platforms
and HUD labels visible, but the existing character/tool art remains inconsistent.
Matched resolution captures establish framing; they do not replace before/after
clips, aim-sweep review, fresh-player observations or a listening pass. No final
art, takeoff/landing effects or replacement audio was authored here.

Follow-up: [the next increment](polish-feedback.md) adds takeoff/landing feedback
and a [workshop style candidate](polish-style-reference.md). A grid-exact runtime
composition and keyboard/mouse/physical-controller course review remain ahead.
Resolve those observations before the Close Quarters blockout pass.
