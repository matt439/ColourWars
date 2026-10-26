# Gameplay tuning

The game reads `game/content/tuning.json` before creating the application. The
build stages that file beside the executable. `GameTuning` owns the records that
weapons and projectiles borrow; gameplay mechanics stay in C++, while authored
balance values select their amounts.

## Tool angles and projectile wind threshold

These required fields replace the gameplay constants from gap-audit 8.5. The
shipped values preserve the previous binary32 angles and wind threshold exactly.

| JSON path | Shipped value | Accepted range | Meaning |
|---|---|---|---|
| `weapons.roller.aim_tilt` | `0.7853982` | `0 <= tilt < pi/2` radians | Downward angle from horizontal when facing right; left facing uses `pi - tilt` |
| `projectiles.mist.spread_step` | `0.2617994` | `0 <= step <= pi/4` radians | Angular separation between adjacent mist pellets |
| `projectiles.wind_stop_factor` | `4.0` | Finite and nonnegative | Shared multiplier used to derive the horizontal snap-to-zero speed from each projectile's wind resistance |

The roller's shipped tilt is the same float as `pi/4` (45 degrees); its mirrored
rotation is the same float as the previous `3*pi/4`. A vertical roller tilt is
excluded because it makes left and right facing ambiguous. Zero gives horizontal
aim. Input still cannot aim the roller independently of its facing direction.

The mist's shipped step is the same float as `pi/12` (15 degrees). Its five
pellets remain in the order `0, +1, -1, +2, -2` times that step around the supplied
firing velocity. Zero sends five parallel pellets; the upper bound places the
outer pellets perpendicular to that velocity. Pellet count and order remain
mechanics rather than editable data.

The wind threshold remains:

```text
stop_speed = wind_resistance * wind_stop_factor
if abs(horizontal_velocity) > stop_speed:
    apply this step's existing wind deceleration
else:
    snap horizontal_velocity to zero
```

`wind_resistance` is acceleration, so the multiplier's dimensional equivalent is
seconds. It does not replace the update timestep. Equality still snaps to zero;
both directions use the same threshold. Zero removes the positive early-stop
threshold without disabling wind deceleration. Gravity, velocity caps, movement
integration, collision handling and projectile lifetime are unchanged.

Every projectile's `wind_resistance` must also be finite and nonnegative, and its
float product with the shared factor must be finite. Invalid values fail at
startup with the source filename and key. The two angle fields are required only
on their respective roller and mist records, not on the other tool types.

In Debug, all three fields work through the existing `tune` commands and use the
same validation. Roller tilt applies on the next weapon update, and wind factor
on the next projectile update, including existing objects. Mist spread applies
to newly constructed shots; a flying pellet keeps its launch direction. See
[the console's tuning scopes](debug-console.md#live-tuning).

## Accepted ranges

Every number in `tuning.json` is read through the shared checks in
`game/content_value.h`, which `presentation.json` and the level definitions use
too. A value outside its range fails at startup, and a `tune` edit is refused,
with the file, the object path and the key, for example
`'respawn_delay' must be finite and not negative, and is -1.` or
`'jump_buffer_time' is 2, which is outside [0, 1].`

The bounds say what the code can use or what a field means. They do not fence
in today's numbers, so a balance pass never needs a rebuild to leave them.

| Kind | Rule | Fields |
|---|---|---|
| Signed rate | finite | player `gravity`, `jump_launch_velocity`, `jump_acceleration` and `drop_down_velocity` (y is down, so the sign is the direction); projectile `gravity`; `player_velocity_amount`; offsets and other plain vectors |
| Delay or amount | finite, `>= 0` | `match.countdown` and `overview`; player `x_acceleration` (zero is a constant run), `ramp_gravity_multiplier`, `jump_max_time`, health regeneration, `respawn_delay`, `damage_sound_delay`; `weapons.ammo_regen_delay` and `ammo_regen_rate`; weapon `shoot_interval` (zero fires every update), `starting_vel_length`, `ammo_usage`; projectile `player_damage` and diffusion `start_time`/`end_time` |
| Extent, cap or divisor | finite, `> 0` | player `x_initial_velocity` (zero never leaves rest) and `x_deacceleration` (the braking divisor, and what stops a released stick); projectile `delete_timer`, `size`, `collision_size` and diffusion `end_scale`; `projectiles.max_velocity`; and the fields that were already positive |
| Gain, fraction or threshold | `0..1` | every sound `volume`, the `input` thresholds, `player.drop_down_analog_threshold` |

Rules that compare two fields sit at the end of the reader that reads both.
Besides the paint-tile geometry and the input thresholds
(`0 < menu_release_threshold < menu_engage_threshold`):

- a projectile's diffusion `start_time` is at most its `end_time` (equal is a
  step straight to `end_scale`);
- `match.opening.push` happens inside the countdown, so it is at most
  `match.countdown`. The countdown is deliberately not compared with the match
  duration: they are sequential timers.

A weapon's `ammo_usage` is deliberately not compared with
`weapons.starting_ammo`. A tank too small for one shot is a tool that reads as
empty, a visible state rather than a fault, and the empty-paint acceptance
fixtures are built exactly that way.

## Verification and menu presentation

The `gameplay_tuning` consumer case uses the actual builders, roller and
projectile integrator without creating a graphics or audio device. It checks
the shipped values against the previous formulas, custom values, boundary and
invalid content, and updates through borrowed records. The existing tool,
projectile, movement and paint checks continue to cover their wider behavior.
Debug-console acceptance checks the new tuning scopes and rejected-edit rollback.

Run `scripts/run-acceptance.ps1 -Preset x64-debug` and repeat with `x64-release`
from an x64 Visual Studio Developer shell.

The remaining menu geometry and sheet choices are documented in
[menu presentation](menu-presentation.md). The historical
[gap audit](review/gap-audit.md#85-the-cjson-line-is-drawn-inconsistently) is unchanged;
[TODO.md](../TODO.md) records open work.

## Headless balance runner (`TOOL#1`, 2026-10-25)

**OPEN.** The five-tool balance pass (`POL#27`) and the arena blockouts
(`POL#21`) are judged today from a handful of attended matches and scripted
feasibility cases. Both questions are statistical: does one tool win a stage
too often, does one side's spawn hold more paint, does a route go unused. The
pieces to answer them in bulk already exist. Levels build without a device (the
acceptance cases and the §6 timing harness do it), the Debug
[bots](debug-console.md#bots) are seeded and deterministic, and paint credit is
per seat.

The runner is a Debug-only console executable beside the acceptance cases. It
builds a stage with no graphics or audio device, puts bots on every seat, steps
the fixed 60 Hz simulation as fast as the CPU allows, and writes one CSV row per
match: stage, seed, roster, tools, final paint share per team and seat,
eliminations, time starved of ammunition, and paint by region and face direction
(the measures the blockout docs already use). A sweep is stage x tool pairing x
seed, with a summary per cell and its spread across seeds.

Slices, in order:

1. **Runner.** Device-free build, bots on all seats, CSV out, deterministic per
   seed. Acceptance: the same seed reproduces the same row bit for bit, and a
   short sweep on `crossfeed` completes inside the acceptance budget.
2. **A `skirmish` bot (implemented).** `paint` and `chase` each play half the game. A bot that
   paints, engages an opponent in range, retreats to refill and returns is the
   minimum for the numbers to say anything about tools rather than bots. It
   stays a developer tool: shipping AI opponents is outside the chosen scope
   ([match scope](feature-roadmap.md#match-scope-decisions-2026-10-15)).
3. **First report.** A sweep over `crossfeed` and `press_tower` with all five
   tools, recorded in [polish-crossfeed.md](polish-crossfeed.md) and
   [polish-press-tower.md](polish-press-tower.md) as evidence for `POL#27` and
   `POL#31`.

Bot results are evidence, not acceptance. They find outliers for an attended
session to check; they never replace one, and a tuning change still goes
through the author.

### Device-free runner core (2026-10-25)

The first runner slice is implemented. `scripts/run-balance.ps1` compiles a
Debug-only console beside the acceptance cases, using the actual game objects.
It loads manifest resources as CPU metadata with null texture handles and silent
banks, builds real `Level` instances and supplies `BotDriver` input to every
seat. It creates no `Application`, graphics/audio device or physical input reader.

From an x64 VS Developer shell:

```powershell
scripts/run-balance.ps1 -Stage crossfeed,press_tower -Seed 0,17 -Players 2 -Tools sniper,sniper -Duration 20
```

Stages and seeds form the sweep dimensions; tool order and the A/B/A/B roster
are fixed for each invocation. One to four players are supported. Tools default
to Sprayer per seat and the bot defaults to `paint`; the existing `idle`, `walk`,
`jump`, `chase` and `skirmish` behaviours are also available. `-OutputDirectory` chooses a
fresh evidence directory and `-Csv` chooses a new CSV path. Existing CSVs and
run directories are refused. `-SkipBuild` is for an already current Debug build;
the tool itself is always freshly compiled. Run metadata pins source/data,
executable, arguments and CSV bytes separately from the deterministic rows.

The original 67-column prefix records stage, seed, roster/tools, requested duration,
actual active ticks and the fixed-step bits, raw team/seat paint areas, final
health/ammunition and a gameplay fingerprint. Team/seat `share_of_painted`
uses the painted-area denominator; `coverage_of_arena` uses all scoring area,
including neutral tiles. Zero denominators yield zero. Seat paint credit uses
`paint_owners::index(player_num, view)`, so device-less bots retain distinct owners.
Unused seats retain `none` labels, paint owner `-1` and zero-valued measures.

Ammunition starvation is sampled at the real active-input seam, after the bot
chooses input and before that world step: an alive, positive-health player's
actual full-shot readiness is `empty`. It is not an `ammo == 0` test.
`fire_starved_ticks` separately requires attempted fire; a refilling bot may have
zero such ticks. `observed_death_edges` counts sampled alive-to-dead state edges,
not credited eliminations. It can miss a terminal lethal health crossing whose
death-state transition would occur after time-up, so it is a lower bound.

Each match resets the seeded RNG and builds a fresh Level and bot driver. The
step is the existing replay convention, `TicksPerSecond / 60` (166666 ticks),
converted to float; the runner stops at the first real time-up/zoom-out transition.
Rows use classic locale, stable numeric precision and UTF-8/LF bytes, with no
wall-clock timing. Reproduction is qualified on the same build/data/machine and
step, rather than promised across toolchains. A bounded step ceiling rejects a
configuration that cannot finish instead of spinning indefinitely.

Validation: four nontrivial CPU self-check matches cover same-seed whole-row byte
identity, two/four-player owner mapping, seed zero/uint32 maximum and a short
Crossfeed sweep, plus CLI refusals and Unicode CSV/no-overwrite publication.
The public launcher completed a four-row Crossfeed/Press Tower, seed 0/17,
20-second Sniper control: each row had at least 1201 active ticks, the eight
seat samples totalled 2394 empty-readiness ticks, and all final ammunition values
were positive. Same-order replay produced identical complete CSV bytes; reversing
stage and seed order preserved all four complete rows by key. Existing
`debug_console` and `press_tower` CPU cases passed in Debug and Release.
Receipts are in `out/validation/todo-loop-20261025/tool1-core-v1/`,
`tool1-qualification-v1/` and `tool1-order-v1/`.

Native UTF-8 log decoding is scoped to this runner and restores the caller's
console encoding. A separate Unicode-path launcher control preserved the exact
prior CSV row and printed the real path correctly; both the helper and launcher
restored code page 437 after execution. This corrected only diagnostic decoding;
the earlier CSV files and match results remain valid. The supplemental receipt
is in `out/validation/todo-loop-20261025/tool1-encoding-v1/`.

`scripts/run-acceptance.ps1 -Preset x64-debug -Cases balance_runner` runs the
bounded self-checks. The default Debug suite includes them; Release rejects this
developer-only case. Case selection is validated before building, and
`-CompileOnly` lets the launcher reuse the compiler/object linkage without
pretending the self-checks ran during compilation.

### Credited projectile eliminations (schema 2)

`Player::lethal_projectiles()` records the first projectile contact that takes
an alive player's positive health through zero, before the later dead-state
transition. Match-lifetime counters survive timed and forced respawn; a fresh
match starts them at zero. Repeated contacts cannot replace the fatal source.
The existing sampled `observed_death_edges` remains a separate measure and can
miss that terminal contact at time-up.

A valid projectile owner is only provisional provenance. The runner validates
unique actual roster owners/teams and credits a different opposing participant;
unknown, friendly, self or absent owners remain uncredited. Credit snapshots
recompute rather than accumulate twice. Each seat records
`credited_enemy_eliminations`, `credited_enemy_projectile_deaths` and
`uncredited_projectile_lethals`; match totals reconcile the credited and
uncredited counts. This records projectile lethals, not a new scoring rule.

Schema 2 retains the original 67-column prefix/order and appends 14 fields:
two match totals and those three measures for each of four seats, giving
81 columns. The declared version changes from 1 to 2; unused-seat counters
remain zero. Earlier schema-1 files remain historical controls. Damage, tuning and
gameplay decisions are unchanged.

The ordinary `lethal_provenance` consumer case exercises real Player/projectile
callbacks, immunity and repeated/competing hits, respawn retention and
uncredited sources. The added collision controls cover a fast Jet
crossing with both endpoints clear and an actual Scene batch where one
shared Roller volume hits two victims. The Debug runner adds actual-roster validation,
idempotent attribution and deterministic match checks. `lethal_provenance` runs in both presets; the
`balance_runner` case and public launcher remain Debug-only.

Qualification: Debug/Release game/editor builds and include-boundary checks
passed, along with `lethal_provenance`, `combat_feedback`, `view_shake` and
`debug_console` in both presets plus Debug's `balance_runner` (nine CPU cases).
The public two-Sniper, 20-second Crossfeed/Press Tower control at seeds 0/17
retained all 66 non-schema legacy values, including fingerprints, exactly;
only schema 1 became 2. Complete 81-column repeats were byte-identical and
reversed-order rows matched by key. Press Tower seed 17 recorded one credited
elimination/death; the other rows recorded zero, with no uncredited lethals.
These short paint-bot controls qualify measurement, not tool balance. Actual
source baseline `e5256a5` and engine `1e60fb2`, source/producer/CSV pins,
case logs and the separate CSV comparison are retained under
`out/validation/todo-loop-20261025/tool1-lethal-native-v1/`.

### Skirmish measurement bot

`scripts/run-balance.ps1 -Bot skirmish` selects the new Debug-only policy.
It paints, engages, withdraws to refill and returns, using actual full-shot
affordability rather than positive ammunition alone. The
[bot policy and limits](debug-console.md#skirmish-measurement-bot) describe
the saved-X return/Y jump hint and tuning-derived stationary-launch heuristic.
There is no line-of-sight, path or ballistic-intercept solver. Default `paint`,
the earlier bots, schema-2 metrics and gameplay tuning remain unchanged.

The `skirmish_bot` consumer case exercises policy transitions, death/reset,
private RNG and deterministic input, actual five-tool observations, a real
positive-but-unaffordable Sniper tank and unchanged old-bot observations.
Release checks developer-code exclusion instead of running the policy.

Qualification: Debug/Release game/editor builds and include-boundary checks
passed. `skirmish_bot` and `debug_console` passed in both presets, plus Debug's
`balance_runner` (five CPU cases). Four 4P, 60-second controls used
Sprayer/Sniper/Mister/Bucket on Crossfeed and Press Tower at seeds 0/17;
two 2P, 60-second controls used Roller/Sprayer on those stages at seed 0.
Every new match ran 3601 active ticks. The primary four-row Skirmish CSV
repeated byte-for-byte, and its reversed-order rows matched all 81 fields by
key. The previous four-row, 20-second
two-Sniper `paint` control retained its entire 81-column CSV bytes.
These six new controls cover the tools across different rosters; the full
comparative report and attended balance decisions remain open.

The first Debug qualification stopped because an old CLI negative still
expected registered `skirmish` to be rejected. That failed run remains under
`tool1-skirmish-native-v1/`. Its passed Debug producers, dedicated Skirmish
and console checks were retained with a frozen input/evidence record.
The narrow fixture repair adds positive Skirmish parsing and uses a genuine
unknown bot for the negative; no bot policy or gameplay changed. Fresh
`tool1-skirmish-native-v2/` runs passed the affected Debug runner and remaining
Release/public checks. Source baseline `95ac1e9`, engine `1e60fb2`, exact
source/producer/CSV hashes, both comparison reviews and the preserved failure
are under `out/validation/todo-loop-20261025/` in those native roots and
`tool1-skirmish-prep-v1/`.

**Still open in TOOL#1:** final region/face-owned paint
and the first full five-tool report. The short control above
qualifies the pipeline, not tool balance or human acceptance. No tuning value or
shipping opponent behaviour changes in this slice.
