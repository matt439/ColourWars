# Local aim and tool readiness

Implemented 2026-10-15 as the gameplay cue portion of `POL#14`. The accepted
Paint.exe keyline language carries through a small optional marker in the
owning player's pane. The later [bare-cover contact cue](polish-blocked-shots.md)
explains shots stopped at an obstruction. The player-identity system remains
open; human readability review still needs the reference arena.

## Direction and eligibility

Sprayer, sniper, mister and bucket show a marker just beyond the actual nozzle,
rotated with continuous aim through either facing. It describes initial direction,
including the bucket's initial throw direction. It does not predict a trajectory,
hit point or clear line of fire. The fixed-aim roller has no marker.

| Shape | Actual tool state |
|---|---|
| Forward chevron with a small team bead | Enough paint for a whole shot and cooldown elapsed |
| Two white bars | Enough paint, still recovering |
| White cross | Insufficient paint, including when also recovering |

All three shapes have a black keyline and differ without team colour or sound.
The state comes from `Weapon::readiness()`, which also supplies the existing
`ready_to_shoot()` decision: strict `shoot_timer > shoot_interval` and the same
cost-relative floating-point allowance for a whole shot. It is independent of
held fire and switches immediately when paint or cooldown eligibility changes.
The brief empty/recovered event pulses remain separate acknowledgements.

The marker draws once in the owning pane using that pane's world camera, so
nearby players do not add their markers to the local view. It appears only during
active play for a living, healthy player with their ordinary tool visible.
Opening/countdown, whole-figure story poses, grey figures, death/redraw and
time-up hide it. Pause keeps the exact frozen state beneath the pause menu;
resume has no timer or animation to restart. Drawing is const and changes no
weapon, player, sound, paint, random or replay state.

## Settings and authored values

Controls / audio exposes **Aim marker**, On by default, from Options and Pause.
Apply saves the shared choice; Back discards pending edits. Older saves and
invalid field types retain the default without resetting unrelated preferences.
Changes apply to the current paused match on resume.

`presentation.json.weapon_feedback` owns the marker's world dimensions:

- `aim_marker_distance`: 42 units beyond the nozzle.
- `aim_marker_size`: 16 units.
- `aim_marker_stroke`: 3 units, with the existing one-unit black keyline and
  white highlight.

The loader requires finite positive dimensions, distance greater than size,
distance at most 128, size at most 32 and `keyline < stroke <= size / 4`.
These bounds keep it near the tool with separated strokes. The utility fill
frame is already validated; no new atlas or generated asset is required.

The renderer submits the marker as an owner overlay rather than a scene object;
it cannot disappear because the body was culled. `aim_marker_bounds()` includes
the full rotated shape and keyline for validation. It remains subject to the
pane viewport and does not alter the stable camera used for mouse aiming.

## Reproduction

From an x64 Visual Studio Developer shell with `VCPKG_ROOT` set:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild
./scripts/check-painter.ps1 -Preset x64-release -SkipBuild
```

## Validation (2026-10-15)

- Debug and Release build with the pinned engine and pass all 19 consumer cases.
  The new case checks all five weapons' exact/adjacent paint and strict cooldown
  boundaries, released-fire readiness, insufficient-paint precedence, 32 aim
  directions, invalid views and live settings. Real Level checks cover opening,
  countdown, tool/story visibility, pause, fatal-before-dead, both respawn paths,
  the roller exclusion and time-up. Paired enabled/disabled simulation preserves
  movement, aim, firing cadence, ammunition, health/death and paint coverage.
  Eighteen malformed presentation variants are rejected.
- Both hardware D3D11 marker matrices pass on the NVIDIA GeForce RTX 5080.
  Four aimed tools cover ready/recovering/empty in every palette and aim quadrant
  at four-player 720p, with representative 1080p/QHD captures. A mixed roster
  uses different aim and readiness in each pane. Each changed pixel stays in
  that pane's marker bounds; independent native rasters distinguish all three
  shapes. Repeated draws, pause and preference toggles preserve simulation and
  Debug replay fingerprints, and the disabled isolated-save fixture is unchanged.
  Existing painter/action/tool/contact/quiet/shake/redraw matrices also pass.
- A hidden native harness exercises the real Options and Pause pages with an
  isolated profile: pending edits, Escape discard, Apply, Back, persistence and
  returning to Pause pass. Native [720p settings](images/polish-aim-marker/settings-720p.png)
  and 1080p captures were inspected. The original full-size option rows keep
  their spacing; the controls reference and cue legend fit in the right column
  above the footer.
- Native [four-tool shape crops](images/polish-aim-marker/native-shapes-720p.png)
  and a [mixed four-player frame](images/polish-aim-marker/mixed-720p.png) were
  inspected at 720p. The chevron, bars and cross differ at this scale and remain
  local to the owning pane. These are quiet-course captures; readability through
  crowded paint and combat still needs human review.
- The 53-file Release ZIP contains the exact presentation source and accepted
  sound bank. Isolated Debug and extracted Release startup pass with the bank,
  a system-only PATH and graceful close. Extracted Release also passes without
  the optional bank, reporting the expected silent fallback. Clean-PC deployment
  remains `ROAD#1`.

CPU evidence is in `out/acceptance/<preset>/`; build, native menu, package and
startup evidence is in `out/validation/aim-marker/`. These checks retain the
human movement, controller, listening and reference-composition review gates.

Final renderer receipts, CSVs, lossless captures and contact sheets are under
`out/validation/aim-marker/visual/`:

- `x64-debug/d741438ff76643ac865bf7d4f5b57cc9/`
- `x64-release/770f9f493ad94401bd9e53bacecbc36a/`

The first Debug attempt's breakpoint and focused reproduction are retained too.
Rapid depletion in that rendering fixture exhausted the real one-shot voice
pool without audio updates. The final marker fixture uses a silent bank, as CPU
acceptance does; runtime audio and its listening acceptance are unchanged.
