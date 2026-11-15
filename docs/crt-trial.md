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

## Focused capture follow-up: harness boundary

Read-only inspection found a useful but incomplete route:

- `scripts/check-painter.ps1` launches its probe hidden with an isolated profile.
  Its `-CountdownOnly` route reaches real `Level` opening/countdown scenes.
- `scripts/acceptance/painter_visual.cpp` constructs its own `Application`.
  It does not run `game/main.cpp`, which currently declares the CRT chain.
  Merely setting the trial flag for that probe is therefore invalid: Level
  would supply CRT values without the matching declared post chain.
- `painter_visual/reveal_checks.h` tests RevealArt primitives rather than the
  complete MatchOpening final-reveal composition. It cannot establish that
  the CRT glass boundary preserves the full final scene.
- `check-finish-details.ps1` explicitly focuses the real game window and parks
  the desktop pointer. It was not run: unattended foreground interference is
  outside this check's scope.

No new screenshots were captured in this follow-up. Existing startup results
are not evidence of glass-only rendering, final-screen text readability,
Reduce effects pixel neutrality, or unchanged split-screen pixels.

### Short attended owner check

Use a **new PowerShell window** and the isolated trial build. This process-only
profile override keeps the normal player's save out of the experiment:

```powershell
Set-Location C:\Projects\ColourWars-crt-trial
$env:LOCALAPPDATA = Join-Path $PWD 'out\validation\crt-owner-profile'
New-Item -ItemType Directory -Force $env:LOCALAPPDATA | Out-Null
$env:COLOURWARS_CRT_TRIAL = '0' # first pass; use '1' for the second
& .\out\build\x64-debug\game\ColourWars.exe
```

1. In the isolated profile, set Reduce effects off. Start a multiplayer match
   with the full opening, not solo practice. Watch the monitor opening, the
   push into gameplay, and the final-score reveal. Note glass texture versus
   physical bezel/person, cursor, instructions and score text.
2. Close the game, set the flag to `1`, and repeat the same setup. Only the
   glass picture should gain mild texture; the physical foreground and text
   should remain crisp. Normal split-screen should not acquire a film.
3. With the trial enabled, set Reduce effects on and repeat. The CRT texture
   must disappear. Also check pause/resume and a window-size change.
4. Close the game and this PowerShell window. Its environment overrides do not
   persist. This short check is feedback, not deterministic pixel/perf proof.

### Small proposed harness change (not implemented)

Extract chain registration and required view capacity into shared inline helpers
in `game/config/crt_trial.h`; call those from `game/main.cpp` and
`scripts/acceptance/painter_visual.cpp`. This removes the divergent startup seam.
Then add one focused CRT leaf, not a general painter sweep, capturing real
`Level::draw` at fixed opening, active and final-reveal ticks, with chain off/on
and Reduce effects. The leaf must compare equal neutral/split-screen frames and
bound every changed pixel to the glass, while preserving the foreground.
The exact new leaf dispatch/registration would touch `scripts/check-painter.ps1`,
`scripts/acceptance/painter_visual.cpp` and a new
`scripts/acceptance/painter_visual/crt_trial_checks.h`; its evidence selector and
record must follow the existing painter leaf registration rules. This is the
next automated gate, deferred rather than built during shared-machine contention.

## Rendered gate completed (2026-11-15, supersedes harness blocker above)

The shared initializer and narrowly scoped `-CrtTrialOnly` painter route are now
implemented. Inspection found `monitor_story` can reach the complete real Level
reveal safely, without the broad monitor matrix. The route uses that fixture,
2P/4P seeded inputs, 720p, and four frozen beats: opening, active split-screen,
post-pullback overview, and visible painted-result cards. Each beat is drawn
with normal and reduced effects, with duplicate readbacks and simulation
observation/Debug fingerprint checks. Flag off and on run in separate processes.
It uses hidden windows and isolated profile fixtures, never foreground input.

- Shared-initializer Debug build passed, capped at two compiler jobs.
- Both final focused painter runs passed on D3D11 / NVIDIA RTX 5080.
- 16 off/on frame pairs (32 captures, 64 duplicate-check readbacks) compared.
- All four active split-screen pairs are byte-identical.
- All eight reduced-effects pairs are byte-identical, including results.
- Six normal opening/overview/result pairs change, strictly inside the authored
  glass (x=128..1151, y=50..596). Zero changed pixels outside it.
- Independently selected score-card interiors spanning the visible glyphs are
  byte-identical. Inspected opening and painted-result captures: physical
  foreground remains crisp; large score labels remain clearly readable.
- Earlier no-paint fixtures closed the program instead of showing score cards.
  Those captures were not accepted as score-readability evidence. Final runs
  paint with the existing deterministic monitor fixture and show A 100.0%,
  B 0.0%. The first off-run also exposed a contact-sheet selector omission;
  it was fixed and final off/on runs pass the full focused wrapper.

Reproduce from an x64 developer PowerShell after building Debug:

```powershell
$env:COLOURWARS_CRT_TRIAL='0'
./scripts/check-painter.ps1 -SkipBuild -CrtTrialOnly -OutputDirectory out/validation/crt-off
$env:COLOURWARS_CRT_TRIAL='1'
./scripts/check-painter.ps1 -SkipBuild -CrtTrialOnly -OutputDirectory out/validation/crt-on
# Substitute the captures directories printed by these two runs:
python scripts/compare-crt-captures.py OFF/captures ON/captures
```

The comparison uses Pillow/numpy, exact expected frame names, equal state CSVs,
authored glass bounds, nonzero enabled deltas, neutral equality and score-card
interiors. Final local evidence:

- off: `out/validation/crt-painted-off/x64-debug/043bb9ba73cd458ab0148231e3873d59`
- on: `out/validation/crt-painted-on/x64-debug/8e194980c1e94f8786a3c9c17c869326`
- comparison: `out/validation/crt-painted-diff.json`
- inspected result image: `out/validation/crt-painted-result.png`

Remaining gates: full moving transition and pause/resume/resize visual checks,
other resolutions, Release/non-D3D11, frame-time/hardware-floor qualification,
engine green-tip policy and owner taste. These fixed 720p samples do not prove
all text or all frames are readable, nor establish performance acceptance.
