# Paint.exe hurt and elimination feedback

Implemented 2026-10-15 as part of `POL#10`, continuing the
[projectile and surface-paint pass](polish-paint-effects.md). This adds contact
feedback to the existing painter action poses and accepted hurt/pop sounds.
The reference arena and human composition review remain in their existing rows.

## Contact and lifetime

Accepted, positive nonfatal damage starts four short white, black-keylined ticks
beside the head and upper body. They fade once over 0.12 seconds, leaving the
face, team clothing and aiming tool visible. A 0.3-second per-player repeat
limit keeps continuous contact from restarting the accent on every hit. Damage
and the existing hurt pose remain immediate during that limit. Immune contacts
do not emit it.

The alive-to-dead transition replaces any hurt accent with eight irregular,
team-coloured fragments. They separate from the death position, rise and fade
over 0.32 seconds. They cannot place paint, take ammunition, do damage or claim
surface ownership. The figure's elimination pose and the spawn's existing
undo-history redraw keep their timings. The story's whole-dead pose hides the
burst, so its finished figure stays intact during the reveal.

Each player owns one hurt timer, one repeat timer and one death snapshot.
Positive simulation updates age them once; drawing is const across all views.
Nonpositive or nonfinite cosmetic deltas freeze the timers, and a long update
expires the effects. Placement, natural respawn and forced respawn clear them;
level teardown destroys them. Fragment directions use fixed authored geometry
and consume no gameplay random numbers. Visual culling includes the detached
burst's arc while collision geometry stays unchanged.

`presentation.json.combat_feedback` owns durations, geometry, colours and the
enable flag. Startup checks reject invalid positive values, an accent repeat
delay shorter than its duration and oversized keylines; resource validation
checks the utility fill frame. The shapes use the existing white fill, with
code-authored blunt corners and black outlines. No new atlas, build dependency
or runtime asset is required.

## Reproduction

From an x64 Visual Studio Developer shell:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild
./scripts/check-painter.ps1 -Preset x64-release -SkipBuild
```

The CPU `combat_feedback` case uses real player/projectile contacts and the
Scene update/contact ordering, without graphics, audio devices or player saves.
The painter renderer probe compares enabled/disabled effects at the identical
simulation state and records their timers and death snapshot. Its existing body
frame comparisons suppress combat effects so particles cannot stand in for
different character artwork.

## Validation (2026-10-15)

- Debug and Release build with the pinned engine and pass all 13 consumer
  cases. The new case covers accepted/ignored and repeated damage, fatal input
  suppression, the detached death snapshot, timer expiry, lifecycle resets,
  malformed content and exact enabled/disabled simulation equivalence through
  movement, paid shots, death and respawn. Existing coverage/credit, movement,
  audio and Debug replay checks also pass.
- Both D3D11 renderer runs pass on the NVIDIA GeForce RTX 5080. One/four-player
  action captures cover all three palettes; controlled four-player comparisons
  cover hurt initial/recovery/expiry and elimination initial/middle/late/expiry
  at 720p, 1080p and QHD. Enabled/disabled rasters differ only while a cue is
  active; repeated frozen draws stay byte-identical and leave simulation and
  cue timers unchanged. Raster output stays inside projected culling bounds,
  and whole-dead story figures suppress the burst.
- Native four-player 720p crops were inspected: the hit ticks remain distinct
  from the face and tool, and fragments separate before fading. Retained crops
  show [hurt](images/polish-combat-feedback/hurt-720p.png),
  [hurt recovery](images/polish-combat-feedback/hurt-recovery-720p.png),
  [elimination](images/polish-combat-feedback/elimination-720p.png) and
  [late elimination](images/polish-combat-feedback/elimination-late-720p.png).
  These quiet-course captures do not establish crowded-match readability.
- The 53-file Release ZIP builds and contains the exact authored presentation
  data, with no development assets. Isolated Debug and extracted Release startup
  pass with the accepted bank, a system-only PATH and graceful shutdown.

Evidence is under `out/validation/combat-feedback/` and `out/acceptance/`.
Successful renderer run IDs are Debug `d9e1c7260d194e65a4b20c173e3b9861` and
Release `04925a0c672c4a79b8f3874146b0a19c`.

## Remaining feedback

`POL#10` retains step/skid and painted-support particles, muzzle accents,
optional view shake, empty-paint visuals and menu focus motion. `POL#12` owns
idle drips and quiet character detail; `POL#13` owns the reduce-effects option.
Human readability in the reference match, movement feel, listening acceptance,
physical controllers and frame pacing remain separate acceptance work.
