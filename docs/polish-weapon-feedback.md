# Paint.exe firing and paint-supply feedback

Implemented 2026-10-15 as part of `POL#10`, following the
[tool mechanisms](polish-tools.md) and [contact effects](polish-movement-contact.md).
These brief accents complete the visible firing and empty/recovery events;
optional view shake and menu focus motion remain in `POL#10`.

## Events and drawing

A paid shot starts a short, team-coloured, black-keylined muzzle phrase:
three wet ticks for the sprayer, a narrow pressure streak for the sniper,
a five-bead fan for the mister and loose rim splashes for the bucket.
The roller keeps its existing supported contact swash and turning sleeve.
An initial held request, held cooldown or unaffordable request cannot start
a muzzle pulse. Fast streams finish each pulse before a later paid shot
starts the next, so sustained fire does not pin the first frame.

The accent follows the actual continuous muzzle position and aim through both
facings. It neither moves that muzzle nor adds a projectile. Each weapon owns
three timers; there are no particle allocations, gameplay random draws, damage,
ammunition costs or changes to paint ownership and coverage.

An unaffordable fire request shows a short white, black-keylined cross above the
figure, anchored to the tool pivot. It shares the existing empty sound's
repetition limit and rearms when fire is released. Cooldown alone does not show it. Regenerating enough paint
for a complete shot replaces it with a team-coloured bead and two white rays.
That acknowledgement occurs once after depletion. A forced refill, including
infinite paint, stays silent and does not invent a recovery event. The cues are
brief event responses; the persistent readiness/aim treatment remains `POL#14`.

Positive finite simulation updates age the timers once before new events are
emitted. Frozen or invalid cosmetic deltas leave them unchanged; repeated draws
and extra views never age or restart them. Death, placement, respawn and time-up
clear them through the existing tool reset. Pause preserves them. Disabling tool
mechanism motion does not disable these separately configured cues.

`presentation.json.weapon_feedback` owns the enable flag, timings, per-tool
muzzle extents, status size/offset and keyline/highlight colours. Current defaults
are 0.10 seconds for muzzle pulses, 0.18 for empty paint and 0.24 for recovery.
The status is twelve world units high, offset 84 units above the tool pivot, with
a one-unit keyline. Startup rejects nonpositive/nonfinite timings and geometry,
oversized keylines and missing fill resources. The shapes use the existing
utility fill frame; no new atlas or build dependency is required.

## Reproduction

From an x64 Visual Studio Developer shell with `VCPKG_ROOT` set:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild
./scripts/check-painter.ps1 -Preset x64-release -SkipBuild
```

The CPU `weapon_feedback` case uses the real firing, affordability and refill
paths. The renderer probe compares enabled/disabled drawing at the same simulation
state; its controlled empty/recovery snapshots test geometry and lifetime, while
the CPU case establishes their production event triggers. Existing mechanism-pose
comparisons suppress the new pulses so they cannot substitute for different tool
artwork.

## Validation (2026-10-15)

- Debug and Release build with the pinned engine and pass all 15 consumer
  cases. New checks cover paid shots versus held cooldown, stream pulse expiry,
  the roller exclusion, empty repetition/release, whole-shot regeneration,
  forced refills, independently disabled tool motion, placement, death, both
  respawn paths and time-up reset. Five-tool enabled/disabled runs preserve
  ammunition, cadence, aim, projectile ownership and trajectories. The content
  case rejects 47 malformed variants. Existing movement, paint coverage/credit,
  audio and replay checks pass too.
- Both D3D11 renderer runs pass on the NVIDIA GeForce RTX 5080. Actual paid-shot
  calls cover all five tools in one view, and mixed four-player tools in all
  three palettes, through four aim quadrants and a live 180-degree aim change.
  Controlled four-player comparisons cover muzzle, empty and recovered initial,
  middle and expired states at 720p, 1080p and QHD. Active rasters differ from
  disabled drawing, expired rasters match, the roller emits no muzzle, and
  frozen draws leave timers and Debug replay fingerprints unchanged. Pixels stay
  inside the projected culling bounds and muzzle deltas follow the current nozzle.
- Native four-player 720p crops were inspected: [sprayer muzzle](images/polish-weapon-feedback/muzzle-720p.png),
  [precision recovery](images/polish-weapon-feedback/precision-recovery-720p.png),
  [empty paint](images/polish-weapon-feedback/empty-720p.png) and
  [recovered paint](images/polish-weapon-feedback/recovered-720p.png).
  Empty/recovery cues sit above the head rather than covering the face or tool.
  The expanded capture matrix now produces bounded, numbered contact-sheet
  pages; native crops are never enlarged. These quiet-course stills do not
  establish crowded-match readability or human movement/audio acceptance.
- The 53-file Release ZIP builds with the exact presentation source and accepted
  bank, excluding development assets. Isolated Debug and extracted Release
  startup pass with a system-only PATH and graceful close. Extracted Release
  also passes separately without its optional bank, reporting the expected
  single silent-fallback diagnostic. Clean-PC acceptance remains `ROAD#1`.

Final visual receipts, CSVs, lossless captures and paginated review sheets are
under `out/validation/weapon-feedback/final-visual/`:

- `x64-debug/750d28d528d64996a7ad12abea6ad591/`
- `x64-release/52dd4621ef134198af09c64e47359f78/`

CPU evidence is in `out/acceptance/<preset>/`; package/startup evidence is in
`out/validation/weapon-feedback/`. Earlier runs there retain the initial
face-overlapping status position and the oversized contact-sheet export failure;
the final receipts above use the corrected placement and bounded pages. The
timings and intensity remain tunable, and reference-arena composition acceptance
stays with `POL#7` and `POL#24`.
