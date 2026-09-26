# Gameplay, input and countdown review fixes, 2026-09-26

The three findings from the review of game revision `fa513d4` are addressed.

- Flying paint checks eligible enemies between the player and muzzle, removing
  the sniper's close-range blind spot. Cover wins simultaneous impacts, and
  friendly/dead-player filtering, ramp pass-through and the roller's ordinary
  multiple-contact behavior are preserved.
- Returning keyboard focus clears stale menu action ownership. Releasing a key
  held on return cannot resume a match or confirm Quit; a fresh press and
  release works normally. Repeated input reads within one update stay consistent.
- Countdown layout fits and centers the combined text and shadow bounds within
  the window. Every beat and resize starts from the authored scale, so shrinking
  does not permanently reduce the text after the window is enlarged.

Validation on the repaired sources:

- Debug and Release configure/build passed under the repository warning policy.
- Native gameplay checks passed 20 sniper near/far cases across both teams and
  directions, 24 cover-order cases, friendly/dead-player exclusions, exact roller
  damage and retirement, and the earlier projectile-impact regressions.
- Native input checks passed eight focus/alias combinations, eight actual Pause
  cases, selected Quit confirmation, repeated reads, fresh subsequent input,
  20 join/leave combinations, recovery and session scoring.
- D3D11 checks passed 192 countdown text/shadow cases, including small windows,
  shrink/grow cycles and restored authored scale. The earlier 40 player/device/
  resize combinations and 120 menu occlusion cases also passed. Representative
  rendered captures were visually inspected.
- The regenerated Release ZIP passed DLL and content-exclusion checks and
  started outside the checkout from an unrelated working directory, using an
  isolated save and system-only PATH. Its only stderr output was the expected
  optional-wave-bank fallback.

Local ignored reproduction sources and logs are under
`out/validation/project-review-fixes-20260926/`. These are focused native probes,
not a new game test suite. The engine pin is unchanged. Physical-controller feel,
audible balance and prolonged multiplayer play remain outside this validation.
