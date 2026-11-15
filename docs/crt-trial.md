# CRT glass trial (local, opt-in)

This is a reversible consumer experiment, not a shipping graphics option.
Default startup declares no post chain. `COLOURWARS_CRT_TRIAL=1` enables it for
that process; other values leave it off. Nothing is written to the saved settings.
Reduce effects disables the material. Clear the environment variable to compare.

## Art boundary

Only the monitor glass region in the opening and final reveal receives the
material. Physical foreground, authored UI and menus are recorded after the
post boundary. Normal gameplay clears the effect. No curvature or chromatic
aberration is used, so the existing cursor coordinates need no inverse mapping.
This preserves the push-through-the-glass direction in `backstory.md`.

The `opening.crt_trial` presentation record holds the mild trial values. The
record is optional and defaults to neutral. Scanline count is capped by the
actual glass height so spacing never falls below Labrador's four-pixel minimum.
The opt-in allocates one extra renderer view for the crisp foreground, including
four-player layouts. No blanket gameplay or HUD film is intended.

## Dependency and isolation

The isolated `work/crt-trial` worktree moves Labrador from
`1dc8533c175059f84f947f99e21b11f36967d135` to
`d44101a08d5e57327d17ef64015e9e973358fe5b`. That is 51 intervening commits;
this is not a minimal engine patch. The pin change is a separate commit.
The material API comes from `1d12336`, built on the P1 post pipeline.
Relevant runtime changes include render targets/post stages on all backends and
Basic Render Driver software-device classification. No engine edits are made.
The original checkout and its engine pin remain untouched. Nothing is pushed.

## Evidence and limits (2026-11-15)

- Pin-only Windows D3D11 Debug consumer build passed.
- Trial Windows D3D11 Debug build passed.
- `crt_trial`, `menu_presentation`, `gameplay_tuning` acceptance passed.
- CRT probe checks neutral identity, adaptive spacing at small/large glass sizes,
  absent-config neutrality and zero geometric/chromatic displacement.
- Hidden isolated startup and graceful close passed with opt-in off and on.
  This checks content loading and startup, not the match material's appearance.
- Evidence is in `out/validation/crt` and `crt-startup-default` / `crt-startup-on`.

Still required before proposing this for shipping: actual opening/reveal visual
comparison at 720p/1080p/1440p, all player counts, pause/resume and resize;
Reduce effects comparison; full-match readability and frame-time measurement;
Release and non-D3D11 qualification; verify target engine tip/green CI under
the repository's pin policy. Startup/probe success does not establish these.
A permanent option, default change, motion/curvature, gameplay film and engine
migration acceptance are explicitly out of scope of this trial.
