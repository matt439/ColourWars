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

## Verification and remaining gap

The `gameplay_tuning` consumer case uses the actual builders, roller and
projectile integrator without creating a graphics or audio device. It checks
the shipped values against the previous formulas, custom values, boundary and
invalid content, and updates through borrowed records. The existing tool,
projectile, movement and paint checks continue to cover their wider behavior.
Debug-console acceptance checks the new tuning scopes and rejected-edit rollback.

Run `scripts/run-acceptance.ps1 -Preset x64-debug` and repeat with `x64-release`
from an x64 Visual Studio Developer shell.

GAP#8.5 remains open for the menu widget geometry and hardcoded `sprite_sheet_1`
menu choices.
The historical [gap audit](review/gap-audit.md#85-the-cjson-line-is-drawn-inconsistently)
is unchanged; [TODO.md](../TODO.md) records only the remaining work.
