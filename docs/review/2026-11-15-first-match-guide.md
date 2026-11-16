# Optional first-match guide (2026-11-15)

## Outcome

Home now offers **How to play** to the right of Play. Play remains selected
by default and the existing Play/Options/Credits/Exit column stays in place.
The optional three-page guide explains the existing solo Movement Course,
local seats/bots/team setup, the actual saved keyboard/controller bindings,
painted-territory results and the existing Rematch/Next Round/Replay Draw actions.
Both visible Back and Escape/controller B return to the guide's Home focus.
Reading the guide does not alter the match plan or save player preferences.

This is a discoverability improvement, **not a reduction in setup actions**.
FTR#14's approximately eight confirmations and its unscoped Quick Play proposal
remain unchanged. No shortcut chooses teams, tools, sub weapons or arenas for
the player. FTR#10's existing Movement Course is reused, not replaced.

## Executable evidence

- Release D3D11 build, actual local hardware (RTX 5080), hidden/silent isolated
  profile. `check-painter.ps1 -Preset x64-release -HowToPlayOnly` exercises the
  actual Home/guide states with keyboard and owned synthetic controller events
  at 1280x720 and 1920x1080, page cycling, both Back paths, retained Home focus,
  frozen redraw and saved keyboard/fourth-pad remaps. Captures remain local.
- `run-acceptance.ps1 -Preset x64-release -Cases menu_text,menu_presentation`:
  schema/contextual errors, Home guide bounds/overlap and oversized Next label.
- Existing `-HomeReturnFocusOnly` and `-MainMenuOnly` hardware matrices preserve
  the original Home paths and independently drawn Home text/focus rasters.
  Home-return's pre-existing seven-Down Options-to-Back fixture was corrected
  to eight: the previously merged Graphics row moved Back to row nine. The
  original destination/raster assertions remain intact.

The focused guide is not a complete played match, physical-controller session,
listening/performance acceptance, Quick Play implementation or replacement for
issue #88's separate scheduled-master painter evidence. No release is published.
