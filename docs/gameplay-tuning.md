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
2. **A `skirmish` bot.** `paint` and `chase` each play half the game. A bot that
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

