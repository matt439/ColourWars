# Optional local view shake

Implemented 2026-10-15 as the gameplay-camera portion of `POL#10`. Menu focus
and selection motion remain open. The reference-arena and human feel reviews
retain their existing gates; this increment does not freeze movement tuning.

## Events and restraint

Each player owns one cosmetic `ViewShake`. Accepted nonfatal damage starts a
short kick, elimination replaces it with a stronger one, and an established
airborne-to-supported transition can add a small landing accent. Spawn settling,
platform seams, immune hits, dead contacts and small impacts stay quiet. Shooting
does not shake the view continuously.

The authored values are `camera.shake` in `presentation.json`:

| Event | Maximum world displacement | Trigger |
|---|---:|---|
| Hurt | 3 units | Accepted nonfatal damage, at most once per 0.24 seconds |
| Elimination | 5 units | Entry to the dead state |
| Landing | 2 units | Proportional above 800 units/s, full at 1600 units/s |

An accent lasts 0.18 seconds, makes 1.5 deterministic oscillations and decays
quadratically. Its vertical excursion is half its horizontal excursion. Events
replace weaker accents instead of adding amplitudes; a weaker event cannot
extend a stronger one. The maximum displacement is about 3.34 display pixels at
720p and 6.67 at QHD, with the same world scale in one and four views. Bounds
clamping can reduce the movement at an arena edge. Axes wider than the arena
keep their centred framing.

The loader requires positive finite values, amplitudes at most 8 world units,
duration at most 0.5 seconds, at most 3 cycles, a repeat delay from the duration
through 2 seconds, and ordered landing thresholds. Live edits affect the next
accent; the current accent retains its timing and shape.

## Camera, settings and lifecycle

`Player::camera()` remains the base follow camera used by mouse aiming and the
next follow calculation. `Level::view_camera()` applies the displacement only
when building that player's world view, including its culling rectangle. It
never feeds the displacement back into following, cursor direction, projectile
launches, collisions or simulation randomness. The pane's HUD and dividers use
their existing screen cameras. A remote player's event cannot shake the local
pane.

**View shake** defaults On in **Controls / audio**, reachable from Options and
Pause. Left selects Off, Right selects On; Apply saves it for every player and
future launches. Back discards pending edits. Older saves and malformed values
recover independently to On, and writes preserve a JSON boolean. A failed write
keeps the chosen setting active for that run.

Simulation advances the accent once per player update. Drawing and frozen
updates cannot age it. Pause/reconnect suspension, time-up, placement and both
respawn paths clear it. Off prevents new events and immediately selects the
stable render camera, including beneath pause at an unchanged window size.
The next live update clears any retained accent. Essential contact and paint
cues stay available when shake is Off. A combined reduced-effects control for
flashes and ambient detail remains `POL#13`.

## Verification

The `view_shake` consumer case exercises actual Player contacts and movement,
damage rejection, heavy landings, death, natural/forced respawn, preference
changes and lifecycle resets. It compares movement, aim, paid shots, damage,
ammunition and respawn state with shake On and Off. Additional checks cover
bounded/repeated impulses, live edits, invalid/frozen/long steps, content
validation and camera clamping across one through four panes, standard
resolutions, odd window sizes and arenas smaller than the view.

`scripts/check-painter.ps1` includes real-Level D3D11 captures of hurt, a strong
resolved fall, elimination and expiry in one/four views at 720p, 1080p and QHD.
It compares each frozen frame On and Off, checks unaffected panes pixel for
pixel, samples real opaque HUD ink for stability, and verifies unchanged base
cameras, mouse direction, observed simulation and Debug replay fingerprints.
Captures and `shake-detail.csv` live under
`out/validation/view-shake/painter/`; consumer logs are in `out/acceptance/`.

On 2026-10-15, Debug and Release builds passed all 18 consumer cases. Both
hardware renderer probes passed on the D3D11 NVIDIA GeForce RTX 5080, including
same-sized paused views and suspension/resume reset. The retained four-player
720p elimination frames compare [shake On](images/polish-view-shake/4p-shake-elimination-1280.png)
with [shake Off](images/polish-view-shake/4p-shake-elimination-1280-off.png) at the
same frozen simulation state. Only the upper-left world view moves.

The 53-file Release ZIP contains the revised executable and byte-matching
presentation data. Isolated Debug and extracted Release startup passed with
the accepted sound bank and graceful shutdown under a system-only PATH. The
extracted Release also passed separately without the bank, reporting the
expected single silent-fallback diagnostic. Evidence is under
`out/validation/view-shake/startup-*/`.

These checks establish bounded local rendering and preserved gameplay. They do
not establish comfort over repeated matches or human movement/readability
acceptance, which remain in the polish plan.
