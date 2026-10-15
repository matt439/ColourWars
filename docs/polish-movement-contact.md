# Paint.exe movement contact particles

Implemented 2026-10-15 as part of `POL#10`, following the
[hurt and elimination pass](polish-combat-feedback.md). Two small, irregular,
black-keylined flecks accompany a supported step; three trail a fast reversal.
They rise briefly, settle toward the contact and fade. Quiet corrections, idle
feet, airborne motion and movement blocked by a wall do not produce a trail.
The existing takeoff/landing puffs and painter poses keep their treatment.

## Events, paint and lifetime

The existing movement-audio stride and braking decisions now return their
accepted step/skid events to the player. Particles use those same events even
when sound is muted. Stride distance, minimum interval, reversal speed threshold,
repeat delay and sound variation remain the existing tuning. A long tick emits
at most one cue of each kind; when both coincide, the skid supplies the visual.
The reversal trails the incoming travel direction, while a step trails resolved
walking distance. Wall separation cannot count as walked distance. Spawn
settling, the landing tick and a buffered jump do not produce a walking cue.

Dry contact uses a neutral fill with a white highlight. Wet contact samples the
actual top paint under the final supported footprint, including enemy paint,
after the Scene's paint callbacks. A sampled colour travels with its contact
point: a neighbouring painted floor at a seam can supply the wet cue even when
the dry floor resolved first. The top-face lookup retains exact half-open tile
and clipped-surface intervals; side/bottom paint cannot turn a dry top wet.
Detached flecks keep their original tint when the surface is repainted.

The particles use the existing eight-burst per-player storage. They contain no
collision or scoring objects, use no random draws and cannot spend ammunition,
change damage, claim paint or transfer credit. Simulation ages each effect once;
drawing reads the same position, direction, age and colour across all views.
Pause and invalid cosmetic deltas freeze ages. Lifetime expiry removes flecks;
placement, death and respawn clear them, and teardown destroys the storage.
The player's visual bounds include the complete trail and arc.

`presentation.json.movement_feedback` owns the step/skid durations, sizes,
spread, lift and black keyline. Startup rejects missing bursts, invalid numeric
values, particles smaller than the presentation pixel and oversized keylines.
The existing fill texture supplies the shapes, so this pass adds no runtime
asset or artwork build dependency. The movement-feedback switch also controls
these cues for comparisons.

## Reproduction and acceptance

From an x64 Visual Studio Developer shell:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild
./scripts/check-painter.ps1 -Preset x64-release -SkipBuild
```

The CPU `movement_contact` case exercises actual Player/Scene movement,
support, paint callbacks and lifecycle changes. The painter renderer probe adds
real four-player dry walking/reversal captures and a controlled rendering matrix.
The controlled matrix directly emits cues on builder-owned players to isolate
direction, tint, lifetime and drawing from unrelated locomotion frames; the CPU
case establishes the painted-support producer.

## Validation (2026-10-15)

- Debug and Release build with the pinned engine under `/W4 /WX`; all 14
  consumer cases pass. The new case covers shared stride/braking budgets,
  muted events, real walking/reversals, idle/air/wall suppression, friendly/enemy
  tint and repaint, exact tile boundaries, clipped and side-only surfaces,
  both seam insertion orders, same-tick paint callbacks, pause/lifecycle resets,
  old-anchor culling lifetime and malformed content. Enabled/disabled matches
  preserve movement, health, paid shots, ammunition, coverage and owner credit.
- Both D3D11 renderer runs pass on the NVIDIA GeForce RTX 5080. Real four-player
  dry steps/reversals exercise the producer. The controlled matrix captures
  initial, middle and expired step/skid phases across all three palettes at
  720p, 1080p and QHD, with dry/wet and left/right views. At the identical player
  state, direction and tint each change the raster; the draw switch changes
  pixels only during an active cue. Repeated frozen draws stay byte-identical,
  leave observed simulation/tint/timers and the Debug replay fingerprint
  unchanged, and remain inside projected culling bounds.
- Native four-player 720p crops were inspected and retained:
  [dry step](images/polish-movement-contact/dry-step-720p.png),
  [dry skid](images/polish-movement-contact/dry-skid-720p.png),
  [wet step](images/polish-movement-contact/wet-step-720p.png) and
  [wet skid](images/polish-movement-contact/wet-skid-720p.png). The flecks remain
  close to the feet, with the larger reversal readable as a short trail. The wet
  crops come from the controlled tint matrix. These quiet-course stills do not
  establish crowded-match readability or human movement feel.
- The 53-file Release ZIP builds, retains the accepted bank byte-for-byte and
  contains the exact authored presentation data without development assets.
  Isolated Debug and extracted Release startup pass with the accepted bank,
  a system-only PATH and graceful shutdown.

Evidence is under `out/validation/movement-contact/` and `out/acceptance/`.
Successful renderer run IDs are Debug `c3892909debc41a1a4899d435e5e18fb` and
Release `5e4d9e6decae4802ba3cfb069a90e448`.

## Remaining feedback

`POL#10` retains optional view shake and menu focus motion. The
[muzzle/paint-supply accents](polish-weapon-feedback.md) and
[quiet character detail and idle dripping](polish-plan.md#tool-personality-and-quiet-detail) are implemented;
reduce-effects settings remain `POL#13`. Human movement feel, crowded-match
readability in the reference arena, physical controllers and frame pacing remain
their existing acceptance work.
