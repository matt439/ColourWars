# Follow-up review fixes, 2026-09-26

All five findings from the broad review of game revision `70efadf` are
addressed. The engine pin and the game's OOP design are unchanged.

- Player grounding now records actual support from floors, ramps and accepted
  jump-through landings. Walls and ignored platform overlaps cannot preserve
  grounded status; ceiling contact has a separate lifetime.
- Fullscreen countdown drawing follows the completed scene render. Menus draw
  once into the final view, after the countdown, preserving nested state order
  within the existing four-view capacity.
- Controls text fits the current window using the measured text and shadow
  bounds. Resize recalculates from authored scale, so enlarging restores the
  original size rather than retaining a previous shrink.
- Spray and mist honor `no_diffusion`, `linear_size`, and
  `linear_size_and_col_rect`. Sprite and collider sizes use their respective
  authored bases; visibility bounds include both. Shipped tuning retains its
  existing behavior.
- An audio bank that needs a newer recipe but cannot rebuild is invalidated,
  and staging removes its stale runtime copy. A current bank remains usable
  without private source WAVs or the compiler. Removing a source bank also
  removes its staged copy; unrelated banks are untouched.

Validation on the repaired sources:

- Debug and Release configure/build passed; the Release ZIP was regenerated.
- Native movement checks reproduced the original Close Quarters route: the
  falling player is now `in_air` and cannot jump. Floor, wall, ceiling, ramp,
  platform landing, drop-through and rising-through contacts passed. Another
  21,600 scripted steps across both starting sides of all three stages stayed
  within their arenas.
- Spray and mist passed all three diffusion modes with different sprite and
  collider base sizes, phase-boundary continuity, center preservation, bounds
  and actual collision effects. Earlier impact regressions also passed,
  including all eight Turbulence cases painting 288 area units, launch cover,
  enemy/friendly/dead-player behavior and roller multiple contacts/retirement.
- D3D11 pixel checks passed 40 player/device/resize combinations and 120 menu
  cases. Opaque pause, controls and confirmation panels remain unobscured;
  countdown pixels outside the panels remain visible. Controls and shadows
  fit sizes from 320x200 to 1920x1080 and recover authored size after enlarging.
  Representative screenshots were visually inspected.
- Existing native menu, paint attribution and session checks passed for
  simultaneous input, 1-4 players, wins/draws, pause/restart/reconnect,
  rematch and quit.
- Six isolated audio lifecycle checks passed. The rebuilt Release package
  started with a retained synthetic bank, with an incomplete updated recipe
  falling back to silence, and after source-bank deletion cleared staging.
  No commercial audio was used.
- Package content exclusions and startup from an unrelated working directory
  passed with isolated saves and a system-only PATH, including Unicode
  installation and profile paths.

Local ignored evidence is under `out/big-review-fixes-2026-09-26/`,
`out/big-review-2026-09-26/gameplay/post_fix*`, and
`out/review-2026-09-26-data/bank-fix*`. These are focused native probes, not a
new game test suite. Physical-controller feel, audible balance and prolonged
multiplayer soak testing remain outside this validation.
