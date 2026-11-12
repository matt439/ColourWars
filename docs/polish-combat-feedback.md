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

`POL#10` retains menu focus motion; [optional view shake](polish-view-shake.md) is
implemented.
[Muzzle/paint-supply accents](polish-weapon-feedback.md),
[step/skid and painted-support particles](polish-movement-contact.md) and
[idle drips and quiet character detail](polish-plan.md#tool-personality-and-quiet-detail)
are implemented, as is the [reduce-effects option](polish-reduced-effects.md) (`POL#13`).
Human readability in the reference match, movement feel, listening acceptance,
physical controllers and frame pacing remain separate acceptance work.

## Shooter-side confirmation tally (`REV#37`, 2026-11-12)

Every hit cue above is keyed to the victim: ticks, burst, shake and rumble.
They are drawn in the world, so the shooter sees them only when the victim is
in the shooter's own pane. The 2026-11-10 review filed the gap as a hit that is
outside or tiny in that pane. The Debug balance runner now counts it: in a 1v1
it prints a `hit-view` line per seat. Each line counts the ticks on which that
seat's projectile hurt its opponent and where the victim's body was in the
shooter's own camera at the time. A tick where the body was in view counts as
confirmed, and as `partial` when under half of it was in view. A kill by the
arena is not a hit. [`hit_view.py`](../scripts/balance/hit_view.py) totals the
lines from a matrix run.

The `REV#19` [sight ladder](gameplay-tuning.md#sight-distance-ladder-rev19-2026-11-12)'s
baseline gives 1,600 Hard 1v1 matches at shipped values: four arenas, every
ordered tool pair, seeds 1000-1015. The game is `220d4809` / engine `e7fc50e`.

| Shooter tool | Hit ticks | Victim outside pane | Under half in view |
|---|---|---|---|
| Sprayer | 45,633 | 111 (0.2%) | 165 (0.4%) |
| Sniper | 3,585 | 764 (21.3%) | 39 (1.1%) |
| Roller | 40,622 | 0 | 0 |
| Mister | 34,652 | 1,258 (3.6%) | 273 (0.8%) |
| Bucket | 4,379 | 19 (0.4%) | 15 (0.3%) |
| All | 128,871 | 2,152 (1.7%) | 492 (0.4%) |

- **"Tiny" does not occur.** The camera's scale follows the window height, not
  the pane, so a body is the same size in every pane of one layout. In the 1v1
  pane it is 80 px of a 360 px pane at 720p, and 120 px at 1080p. In four-way
  split screen it is 90 px at 1080p. Only "outside" is a real case.
- **The gap is the Sniper's.** One Sniper hit in five lands on a victim the
  shooter cannot see in their own pane. For every other tool the figure is at
  most one in twenty-five. Stream tools count a hit on each tick, so these are
  tick counts, not shots. A Sniper hit is close to one shot.
- **Its size tracks bot sight.** At `bots.sight_distance` 600 the Sniper's
  outside share falls to 1.8%, because the bot no longer engages beyond 600
  units. A human Sniper picks its own range, so this tally does
  not stand for human play.

Verdict: overall, 98% of hit ticks are confirmed in the shooter's own pane.
The unconfirmed share is under 4% for every tool but the Sniper, so `REV#37`
falls from MED to LOW. A shooter-side cue would be a Sniper-range feature. It
remains undesigned, and `player.hit_confirm` stays declined (`POL#23`).
Measured in bot-against-bot matches only. No physical pad, listening or human
play was tested.
