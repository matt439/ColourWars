# Movement feedback

Second polish increment, implemented on 2026-10-08 after the camera/movement
foundation (`fbd45dc`), using the existing Labrador pin. It advances sections 1–2
of [the plan](polish-plan.md) alongside a retained
[workshop style candidate](polish-style-reference.md). The author requested the
timing/intensity decision be implemented now on 2026-10-15 (`POL#5`), with later
tuning allowed. The current values below strengthen the existing cues at
four-player 720p; their implementation no longer waits for final poses or audio.
Physical controller feel and the complete art/audio composition still need review.

## Behaviour

The player now owns a bounded cosmetic trail, drawn through the existing texture
interface. Successful jumps produce a short outward foot puff. A transition from
airborne movement to resolved support produces a landing puff whose strength
uses downward speed before collision resolution zeroes it. Buffered rebounds
emit landing and takeoff on the same contact tick, in that order.

Flat surfaces and jump-through platforms use the midpoint of the actual support
overlap. Ramps use the supporting bottom corner, clipped to the ramp. Grounded
takeoff retains the last resolved support position; a ledge-grace jump emits at
the current airborne feet as motion feedback. It does not invent a ledge contact.
Emitted puffs stay in world space while the player moves away.

Initial spawn settling and respawn settling are silent. Standing, walking over
seams or along supported slopes do not repeatedly land. A platform drop does not
produce takeoff feedback; reaching the lower support can produce a landing.
Ceilings and walls are not landing events. Fatal contact clears effects after the
contact phase, and death/respawn clear their history. Pause freezes the existing
effects; drawing additional panes or repeatedly drawing a paused scene advances
nothing. Scene teardown destroys the player and its trail.

The effect has no collision object, ammunition use, damage or scored paint.
Player collision bounds and movement tuning are unchanged. Visual culling bounds
include the remaining trail, so it can still be drawn after the player moves out
of a view. Storage holds at most eight bursts per player; a ninth replaces the
oldest. The fixed authored shapes bound the drawing work independently of elapsed
time. This is a storage bound, not a frame-time or crowded-combat performance claim.

<a id="authored-prototype-values"></a>

## Authored values

Values are under `movement_feedback` in
[presentation.json](../game/content/presentation.json), separate from balance:

| Choice | Current value |
|---|---|
| Enable/disable for comparison | `enabled: true` |
| Takeoff lifetime / spread / rise / initial size | 0.24 s / 30 / 16 / 16 world units |
| Landing lifetime / spread / rise / initial size | 0.32 s / 44 / 12 / 20 world units |
| Minimum landing speed | 60 world units/s |
| Full-strength impact | 1000 world units/s |
| Minimum visible strength | 0.65 |
| Effect grid | 2 world units |
| Body / highlight | Neutral `D4D7DA` / `FFFFFF` |

The 2026-10-15 choice doubles the initial body sizes, raises the weakest landing
to 65% strength and brightens the neutral greys. At 720p the takeoff body is
roughly 11 screen pixels wide and the initial landing body about 9–13, depending
on impact speed. The slightly longer lifetimes retain a readable recovery while
the player moves away. Landing remains wider and heavier than takeoff; longer
falls still strengthen it. Event emission is immediate on the successful jump
or resolved contact tick, with the same ordering and suppression rules above.
These are usable defaults that remain editable in presentation data.

The two-unit effect grid keeps moving anchors and particle sizes stable; it does
not set the production art density. The historical workshop kit is retired. The
production puffs below follow the accepted
[Paint.exe art rules](polish-plan.md#1-establish-the-art-and-display-rules).
Invalid non-positive or non-finite
dimensions/timings, inconsistent impact speeds, strength above one and particles
smaller than one effect pixel are rejected at startup. The fill frame is checked
against loaded resources.

The neutral colour avoids implying new paint ownership. The
[painter action poses](polish-painter.md#action-poses),
[painted step/skid variation](polish-movement-contact.md) and accepted movement
sounds now accompany these events. The complete composition and audio mix still
need review. This drawing pass adds no audio cue or gameplay effect.

## Paint.exe production puffs (2026-10-15)

Implemented as a gameplay-art increment of `POL#16`. The prototype squares now
have connected, stepped silhouettes with black keylines, neutral bodies and a
small white highlight. Takeoff has a taller crown; landing spreads lower and
wider across the support. Each side has one detached neutral chip. The lobes,
outlines and highlights are partitioned into disjoint rectangles so fading does
not expose internal joins or brighten overlapping fills. The shared fill texel
supplies the shapes; there is no new atlas or art build dependency.

The existing outward motion, rise, shrinking recovery, fade, landing strength
and authored values remain unchanged. Actual successful jumps and resolved
landings still produce the events; drawing remains const. Puffs stay neutral
even on painted support. Their culling allowance includes the wider silhouette,
detached chips, keyline and snapping, including small valid particles. Step/skid
artwork and all movement, collision, ammunition, damage and scoring rules retain
their existing paths. Reduced effects retain these useful movement cues.

Production validation on 2026-10-15:

- Debug and Release build under `/W4 /WX` and pass all 21 consumer acceptance
  cases each, including actual movement/contact events, bounded storage,
  pause/death/respawn and enabled/disabled gameplay equivalence.
- Both pass the D3D11 hardware movement-feedback probe on the NVIDIA GeForce
  RTX 5080: one/four views at 720p, 1080p and QHD; initial/mid takeoff and landing,
  expiry, twelve frozen redraws and the moving aim-swept course routes.
  Differences stay inside projected bounds and below one percent of a pane.
- The isolated matrix checks neutral fill and black keyline at native gameplay
  scales, minimum/full landing strength, distinct silhouettes with equal numeric
  styles, disabled/frozen drawing and clean single-blend colours through fading.
  Valid wide keylines and tiny three-unit particles cover culling extremes; the
  tiny fixture uses 16× inspection without an extra pixel of bounds slack.
- Isolated Debug and Release startup pass with the accepted sound bank, a
  system-only PATH and graceful shutdown.

The four-player 720p per-pane minima match in Debug and Release. `Readable`
counts pixels with a maximum RGB channel difference of at least 32:

| Phase | Changed pixels | Readable pixels |
|---|---:|---:|
| Takeoff | 614 | 614 |
| Mid takeoff | 380 | 244 |
| Landing | 835 | 835 |
| Mid landing | 464 | 292 |

Native [takeoff](images/polish-feedback/production-takeoff-720p.png) and
[landing](images/polish-feedback/production-landing-720p.png) captures retain
the current course and UI. Full frames and isolated fading silhouettes were
inspected. Evidence is under `out/validation/movement-bursts/` and
`out/acceptance/`; visual run IDs are Debug
`778e04baecac478fac55bf290c6998f6` and Release
`1d66bc9b00db46cf95ba1a7fdbffed18`. These scripted checks and image inspection
do not establish human movement feel, crowded-match readability, listening or
physical-controller acceptance.

## Thicker keyline and the wider four-player route (2026-10-25)

CI's Basic Render Driver and the RTX 5080 gave identical numbers for a
regression the earlier run missed. With a one-unit keyline, the four-player
720p mid-landing puff changed 244 pixels in view 1 but only 20 were readable,
because the neutral fill is nearly the colour of the pale course floor.
`movement_feedback.keyline_width` in `presentation.json` is now 2.0. The thicker
black outline carries the cue on pale support, and every pane clears the 128
changed / 64 readable minimum again in the Debug probe on CI.

Four-player panes now frame at least 1280 world units. Team B's views start
pinned to the arena's right edge, so a jump from a 170-unit inset barely panned
(11.4 of the required 16 units in view 2). A 120-unit inset panned but failed a Team B
landing check, so the route probe now starts Team B's jump 150 units in. Gameplay is unchanged; this only moves where the
scripted check jumps.

## Reproduce the checks

From an x64 Visual Studio Developer shell with `VCPKG_ROOT` set:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-movement-feedback.ps1 -Preset x64-debug -SkipBuild -SequenceFrames 90
./scripts/check-movement-feedback.ps1 -Preset x64-release -SkipBuild
```

CPU checks use the actual Player/Scene collision path. They cover launch success
and refusal, impact strength, actual support positions, buffered event ordering,
platform drops, ceilings, seams, slopes, expiry, capacity, culling lifetime,
pause/resume, fatal contacts and respawn. Enabled and disabled feedback run the
same input sequence with equal positions, velocities and player resources.

The separate visual probe requires a D3D11 hardware device. It creates a hidden
window and isolated profile, loads the real Movement Course, settles the authored
spawns and performs scripted jumps. It captures paired enabled/disabled frames
at the same simulation state, for one/four players at 720p, 1080p and QHD. Five
phases cover initial/mid takeoff, initial/mid landing and fully expired effects.
Repeated suspended draws must produce identical pixels. In the settled
four-player 720p fixture, every initial/mid takeoff and landing phase must change
at least 128 pixels per pane, including at least 64 with a maximum RGB channel
difference of 32 or more. This rejects the earlier tiny/faint recovery without
treating a one-level pixel difference as readability. The existing upper bound
remains below one percent of each pane, and expired phases change no pixels.
These are bounded visibility checks for this fixture, not human acceptance of
crowded combat. Each run retains compiler and device diagnostics, source/content
identities, PPM captures and phase data (including `readable_pixels`) under
`out/validation/movement-feedback/<preset>/<run>/` by default.

`-SequenceFrames 90` also retains consecutive 720p frames for each player layout
at a 60 Hz simulation step. It is optional because raw PPM sequences use substantial
space. These scripted inputs do not exercise a physical controller or establish
interactive frame pacing.

## Intensity validation before production puffs (2026-10-15)

Debug and Release builds and all 11 consumer acceptance cases passed with the
current values. Both visual probes passed on **NVIDIA GeForce RTX 5080, D3D11
hardware**, covering one/four views at 720p, 1080p and QHD, feedback on/off at the
same simulation state, 12 identical suspended redraws and the moving aim-swept
course route with camera pan in every view. All changed pixels remained within
the projected culling bounds and below one percent of each pane. Expired phases
changed zero pixels; the settled jump landed at tick 60 and was checked expired
at tick 81.

The per-pane minima in the settled four-player 720p captures were identical in
Debug and Release. `Readable` counts pixels whose maximum RGB channel difference
was at least 32:

| Phase | Changed pixels | Readable pixels |
|---|---:|---:|
| Takeoff | 432 | 402 |
| Mid takeoff | 262 | 262 |
| Landing | 580 | 430 |
| Mid landing | 366 | 188 |

Full-size PNG conversions of all four active Debug phases were inspected.
Neutral bodies and flecks remain visible into recovery close to the feet/contact,
with the HUD and platform edges readable in these empty-course stills. The light
body initially meets the light floor before spreading away. These captures record
the earlier square geometry; the production puffs above supersede that drawing.

Evidence, including `result.json`, diagnostics and `captures/phases.csv`, is
retained under `out/validation/pol4-pol5/visual/`:

- `x64-debug/126c02393cc74d6caa24c81da2d84141/`
- `x64-release/4f8daf007137444f9507471fd79d223c/`

These runs used scripted inputs and `-SequenceFrames 0`. They establish neither
physical-pad feel, clip playback, listening acceptance nor readability in a
crowded painted match. Those checks can tune the implemented defaults later.

## Prototype evidence (2026-10-08)

The following checks used the earlier 8/10-unit body sizes and 0.45 minimum
strength. They establish the original mechanism, not the revised intensity.

Debug and Release builds passed with the pinned engine and `/W4 /WX` on
2026-10-08. All six retained consumer cases passed in both configurations;
the final extended movement case was rerun in each after adding downhill ramp
coverage. Tap/held measurements remain 95/207.472 world units of rise and
506.665/813.33 units of running travel, respectively. Existing arena exposed
paint areas remain 205920, 249000 and 91200.

Both visual runs passed on **NVIDIA GeForce RTX 5080, D3D11 hardware**. Each
configuration captured 30 matched on/off pairs (five phases, two layouts, three
resolutions). All active phases changed visible pixels within projected visual
bounds and below one percent of each pane; expired phases changed zero pixels.
Landing occurred at tick 60, and the checked expired phase was tick 80. Twelve
repeated suspended draws per layout were pixel-identical. These checks exercise
Scene culling; they are not exhaustive camera-edge coverage.

The Debug run also retains 90 consecutive frames for each layout. Sampled stills
show quiet grey flecks close to the feet/contact, leaving platform edges and HUD
information visible. At four-player 720p they are deliberately small and already
faint midway through recovery. That is a candidate intensity to compare in play,
not acceptance of the feel or crowded-combat readability. No human clip playback,
physical-device input, replacement-audio listening or final-art motion review was
performed in this increment.

Debug startup passed with an isolated save and graceful close. The Release ZIP
passed a 42-file inspection, contains the exact revised presentation data, and
excludes private audio, source art and probe executables. Extracted Release startup
passed from an unrelated directory with system-only PATH, isolated save, absent-bank
silent fallback and graceful close. Clean-Windows deployment remains untested.

Local evidence is under `out/acceptance/<preset>/` and
`out/validation/polish-feedback-20261008/`. Successful visual run directories are:

- `visual/x64-debug/04dfbbce59964e49b589d3d63c77a81f/`
- `visual/x64-release/0290ca8d83ba4ae1ae87c961b3996dd6/`

Their `result.json` records the source baseline plus changed working files and
staged content hashes. The earlier failed Debug probe compilation is also retained;
it exposed a test-harness rectangle/point API mismatch, fixed before both passing
runs. It did not require a game or engine change.

## Further tuning

Compare the current cues while playing the Movement Course with keyboard/mouse
and physical controllers, including running landings, busy paint, aim sweeps and
camera motion. Adjust the authored values as the accepted Paint.exe character
poses and audio mix come together. This is subsequent refinement, not an
unanswered `POL#5` decision or a prerequisite for implementing the next slice.
Movement-feel playtests, all-palette art review and clean-Windows release
deployment remain open in their respective work items.
