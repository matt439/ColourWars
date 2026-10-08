# Movement feedback prototype

Second polish increment, implemented on 2026-10-08 after the camera/movement
foundation (`fbd45dc`), using the existing Labrador pin. It advances sections 1–2
of [the plan](polish-plan.md) alongside a retained
[workshop style candidate](polish-style-reference.md). Visual finish and physical
controller feel remain open acceptance gates.

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
of a view. Storage holds at most eight bursts per player, each drawing four small
quads; a ninth replaces the oldest. This is a storage/draw bound, not a frame-time
or crowded-combat performance claim.

## Authored prototype values

Values are under `movement_feedback` in
[presentation.json](../game/content/presentation.json), separate from balance:

| Choice | Current value |
|---|---|
| Enable/disable for comparison | `enabled: true` |
| Takeoff lifetime / spread / rise / initial size | 0.22 s / 24 / 14 / 8 world units |
| Landing lifetime / spread / rise / initial size | 0.30 s / 38 / 10 / 10 world units |
| Minimum landing speed | 60 world units/s |
| Full-strength impact | 1000 world units/s |
| Minimum visible strength | 0.45 |
| Effect grid | 2 world units |
| Body / highlight | Neutral `B3BDC6` / `E5E9E8` |

These are comparison candidates, not frozen game-feel decisions. The separate
art study proposes a four-world-unit texel for a first sprite kit; this two-unit
effect grid does not decide that kit's final density. Invalid non-positive or
non-finite dimensions/timings, inconsistent impact speeds, strength above one
and particles smaller than one effect pixel are rejected at startup. The fill
frame is checked against loaded resources.

The neutral colour avoids implying new paint ownership. Final contact sprite
shapes, takeoff/landing character poses, painted-surface variation and replacement
sounds still need coordinated production and review. This increment adds no
audio and does not approve any legacy cue for the finished game.

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
Repeated suspended draws must produce identical pixels. Each run retains compiler
and device diagnostics, source/content identities, PPM captures and phase data
under `out/validation/movement-feedback/<preset>/<run>/` by default.

`-SequenceFrames 90` also retains consecutive 720p frames for each player layout
at a 60 Hz simulation step. It is optional because raw PPM sequences use substantial
space. These scripted inputs do not exercise a physical controller or establish
interactive frame pacing.

## Evidence and next review

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

Next, compare these cues while playing the Movement Course with keyboard/mouse
and physical controllers, including running landings, busy paint, aim sweeps and
camera motion. Decide timing and intensity together with the new character poses
and audio. Reauthor the workshop candidate as an editable native-pixel kit under
the actual camera before accepting the style or building the Close Quarters
reference blockout. Recovery/staircase route trials, all-palette art review and
clean-Windows release deployment remain open.
