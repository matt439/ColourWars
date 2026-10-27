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
brief event responses; the later [aim/readiness cue](polish-aim-marker.md)
provides persistent shot eligibility independently of these pulses.

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

## Low-paint advance warning (`POL#38`)

**Implemented 2026-10-27, requested by the author the same day; listening and
attended readability remain.** There was no separate depletion penalty in the
code: the penalty is that an empty tool cannot fire until it regenerates a whole
shot, and regeneration only starts `weapons.ammo_regen_delay` (0.5 s) after the
last paid shot. The warning now arrives while at least one paid shot remains.

**The line.** A tool is low below the larger of `weapons.low_paint.supply_fraction`
of the tank (0.25) and `shots` (2) times its own shot cost, capped at a full tank.
The shipped lines are 0.25 for the sprayer, roller, mister and bucket (about 0.6,
1.0, 1.25 seconds of held fire and two throws respectively) and 0.6 for the
sniper, so its warning comes with one shot left. Startup requires `shots >= 2`:
the shot that crosses the line then always leaves another affordable shot.

**Sound.** The paid shot that crosses the line plays
`weapons.<tool>.feedback_sounds.low` once. Held fire, dry fire and partial
refills cannot repeat it; it rearms only after the tank recovers
`rearm_fraction` (0.15) of a full tank or one shot, whichever is more, above
the line, or on any refill. The shot term is what keeps the sniper quiet: its
0.3 shot is larger than the band, and the paint it regenerates between held
shots would otherwise rearm the warning mid-burst. Like the
other feedback edges it never fires on a zero-delta update. All five tools use
the accepted `tool.drip` master at full gain, a provisional choice from the
existing bank: it is distinct in kind from the empty pump and the recovery
valve, but measures about 9 dB RMS below those cues (peak -12.7 dBFS against
-3.0). Whether it carries through a four-player mix, or needs a dedicated
master, is a listening decision for `POL#23`.

**Pictures.** Two persistent cues show while the tool is below the line,
including when empty, and blink on the tool's simulated low age (0.5-second
period; Reduce effects holds them steady). Pause freezes them.

- The HUD's PAINT label inverts into an ink tag with paper lettering
  (`presentation.json.hud.low_paint`). It changes lightness and shape, never hue
  against the team colour, and it is the cue that stays legible at four-player
  size. It shows only while the player is alive: a dead player's weapon stops
  updating, which would otherwise freeze the blink until the respawn refill.
- Above the figure, a white keylined tank with a team-colour fill falling from
  the line to empty (`weapon_feedback.low_paint`, 12x22 world units). It sits 28
  units right of the player's centre at the status slot's height, clear of the
  owner's identity badge whichever way the tool faces, and gives way to the
  brief empty cross and recovery bead in that slot. It is visible to
  every pane, like the tool's own supply strip; disabling `weapon_feedback`
  removes it but leaves the HUD tag.

Costs, cadence, regeneration and every projectile are unchanged: the line is
read only by feedback. The CPU `low_paint` case drives the production weapon
update for all five tools. It checks each line against tuning, exactly one
warning per descent with a paid shot still to come, the latch through dry fire,
rearming only past the band, pulsed fire that never warns again without
recovering past the rearm line, frozen updates, refill clearing and the tank's
cull bounds. It also checks that malformed tuning and presentation are rejected.
`story_audio` records the actual cue: one `tool.drip` per held descent before
the empty cue, none from dry fire or recovery, one again on the next descent,
at the SFX gain.
A Debug 4P Crossfeed launch with paint bots shows the tag and tank on every low
seat.

**Remaining (`DECISION`).** Listen to the cue in the 1/2/4-player mix (with
`POL#23`), and look at the tag and tank at four-player size with the sound
muted. Both values and the cue choice are provisional.

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
timings and intensity remain tunable. The author approved the current treatment
for this development stage on 2026-10-20; reference-arena composition is rechecked
with `POL#17`/`POL#24` as the layout and tuning evolve.
