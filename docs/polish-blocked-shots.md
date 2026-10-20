# Bare-cover shot contacts

Implemented 2026-10-15 as the blocked-shot portion of `POL#14`. Shots stopped
by bare cover now leave a brief, detached contact cue at the struck face. The
existing wet splash continues to explain a hit that paints an exposed tile.
The shared player-identity system is also implemented. The author approved the
current identity, readiness and blocked-shot cues for this development stage on
2026-10-20, closing `POL#14`'s current-arena review. They form a working baseline
that remains open to iteration; Close Quarters composition and readability are
rechecked with `POL#17`/`POL#24`.

## Contact and lifetime

The four flying paint kinds retain the collision normal and the contact point
on the cover's axis-aligned face. The existing launch and movement sweeps still
choose the first obstruction, including thin walls between the player and the
nozzle. The cue points outward from that face, rather than appearing at a
nozzle beyond cover or following the player after the shot is gone.

Contact classification finishes after both collision callbacks. If any exposed
paint tile splashed, no additional cue is emitted. Bare structures and disabled
paintable faces receive a white stop tick with a black keyline and two small
team-paint beads rebounding away from the obstruction. There is no persistent
surface mark. Player hits, pass-through ramps, lifetime expiry and the roller's
multi-contact volume do not emit this cue.

`Projectile::take_blocked_contact()` consumes the result once. Player forwards
launch contacts before blocked projectiles are erased; Level forwards flight
contacts after the scene's contact batch. The Level-owned fixed queue outlives
the projectile, and its world position survives owner movement or death. Every
pane draws the same read-only snapshot beneath the HUD, even when the owner is
outside that pane. Drawing does not age or spawn effects.

Level advances the queue once per active tick. Pause freezes it, long resumed
ticks expire it, and time-up clears contacts and their repetition budgets. A
new match owns a fresh queue. The queue holds at most 16 contacts, replacing
the oldest under excess load; each paint owner has a separate repetition
budget, including device-less debug players. No gameplay random values,
projectiles, scoring tiles or sound voices are created by the visual cue.
The existing budgeted `paint.dud` sound remains the audible response. The
optional accepted `weapon.blocked` sound is not layered over it; assessment of
optional cues and the multiplayer mix remains `POL#23`.

## Authored values

`presentation.json.blocked_shot` owns the effect's switch, utility fill frame,
colours and dimensions. The current defaults are a 0.16-second lifetime,
0.08-second per-owner repeat delay, 24-unit tick width, 18-unit rebound reach,
5-unit beads, 4-unit strokes and a one-unit black keyline. Disabling the section
suppresses drawing and admission; the next positive update clears its queue.
It does not disable wet paint splashes or change the aim-marker preference.

The loader requires finite positive dimensions and timing. Lifetime is at most
0.4 seconds, repeat delay is 0.05–1 second, width and reach are at most 48 units,
beads are at most half the width and strokes at most a quarter. Keylines must
leave visible fill inside both strokes and beads. The existing sheet/frame
resource validation covers this section too. No atlas generation is required.

## Reproduction

From an x64 Visual Studio Developer shell with `VCPKG_ROOT` set:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild
./scripts/check-painter.ps1 -Preset x64-release -SkipBuild
```

## Validation (2026-10-15)

- Debug and Release build with the pinned engine and pass all 20 consumer
  cases. The new case checks launch and flight against all four faces, nearest
  thin cover in both target orders, all four flying kinds and mist-volley
  aggregation. Painted hits suppress duplicate cues in either callback order,
  including after audio is flushed. Real Player and Level checks cover the
  immediate launch erase, flight retirement, sparse owner slots, frozen pause,
  time-up and a fresh match. Enabled/disabled physics, ammunition, health,
  coverage and projectile traces match. Invalid content and a bounded pool
  under sustained repetition are covered too.
- Both hardware D3D11 painter matrices pass on the NVIDIA GeForce RTX 5080.
  The new controlled cover/nozzle contexts cover four outward normals and all
  palettes at native four-player 720p, with representative 1080p/QHD frames.
  Enabled/disabled pixel changes stay inside projected cue bounds; the initial,
  middle and expired phases differ as intended. Repeated drawing preserves
  contact ages, simulation state and Debug replay fingerprints. The actual
  Level overlay renders detached contacts in every pane. Existing painter,
  tool, contact, quiet-detail, aim-marker, shake and redraw matrices also pass.
- The [native 720p contact sheet](images/polish-blocked-shots/native-720p.png)
  was inspected. These are controlled hardware contexts with an opaque cover
  overlapping the tool, not a human assessment of crowded-match readability.
- The 53-file Release ZIP contains the exact presentation source and unchanged
  accepted sound bank. Isolated Debug and extracted Release startup pass with
  the bank, a system-only PATH and graceful close. A separate extracted copy
  also passes without the optional bank, emitting only the expected silent-bank
  diagnostic. Receipts are under `out/validation/blocked-shot/`; deployment on
  a clean Windows PC remains `ROAD#1`.

CPU evidence is under `out/acceptance/<preset>/`, with the final focused Debug
case in `out/validation/blocked-shot/focused-run.log`. Hardware receipts,
`blocked-shots.csv`, lossless frames and contact sheets are in:

- `out/validation/blocked-shot/visual/x64-debug/5e351f9aed004e6daa595d5e6ed48a4f/`
- `out/validation/blocked-shot/visual/x64-release/c42bad5d586e45a0bedf96e3548f83bf/`

Human movement, physical-controller, listening and reference-composition
acceptance remain separate. The author's 2026-10-20 stage approval records no
particular device, play session or test result and does not establish final
quality.
