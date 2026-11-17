# Optional CRT monitor graphics

This local branch adds **Options > Graphics > CRT monitor (restart)**. It is
**Off by default**. Apply saves the choice for the next launch; Back discards
unapplied changes. The page separately shows the current launch's state. A
failed write leaves the previous saved choice intact and reports failure.
Nothing is pushed, deployed or enabled in the owner's normal profile.

For CRT throughout gameplay, the separate opt-in Debug
[shader playground](shader-playground.md) offers `shader crt` and the
combined `shader paint90` treatment. That experiment replaces this saved
monitor-glass effect for its session; the normal option described here still
applies only to opening and reveal glass.

## Behavior and lifecycle

- Only the opening and final-reveal monitor glass receives the CRT texture.
  Physical foreground, authored UI and score cards remain sharp. Normal
  split-screen gameplay clears the effect.
- **Reduce effects** suppresses CRT texture immediately without changing the
  saved CRT choice. Per-seat settings do not override this shared-screen rule.
- An immutable `crt_trial::Runtime` captures the saved choice before Application
  creation. It is borrowed explicitly by the shell and real Level consumers.
  Saving/reloading preferences cannot mutate it during a running match.
- Off launches keep the empty post chain and four renderer views. On launches
  reserve five views for four player views plus the crisp foreground.
- Save version 11 adds `crt_monitor`. Missing/malformed values read as false.
  The dedicated SettingsSession setter changes only this field, rolls it back
  on failed persistence and preserves other preferences/profiles.
- No curvature or chromatic split: pointer coordinates remain unchanged.
  `opening.crt_trial` contains optional presentation values; absent is neutral.
  Scanline count respects Labrador's four-output-pixel minimum spacing.

### Developer override

`COLOURWARS_CRT_TRIAL=1` forces On, `0` forces Off, and absent/other values follow
the saved choice. The Graphics page explicitly identifies an override. Saving
under an override tells the player to remove it and restart to use that choice.
The override is never written to the save. Legacy isolated probes can still
use the original environment path; the production executable uses explicit
launch state. Restart the process after changing an environment override.

## Dependency and local isolation

Branch `work/crt-trial`, isolated worktree `ColourWars-crt-trial`. The original
checkout is untouched. Separate pin commit `9eed4e9f` advances Labrador from
`1dc8533c175059f84f947f99e21b11f36967d135` to
`d44101a08d5e57327d17ef64015e9e973358fe5b`, 51 intervening commits. Runtime changes
include the post pipeline on all backends, CRT material and software-adapter
classification. No engine code is edited. This is not a minimal engine patch;
verify the engine green-tip policy before proposing release/merge.

## Validation evidence, 2026-11-15

### Rendering

The focused hidden painter fixture draws real seeded 2P/4P Levels at opening,
active gameplay, post-pullback overview and painted-result beats. It captures
normal/reduced effects at 720p and 1080p, without physical input or foreground
focus. Each frame has a duplicate frozen readback and state check; Debug also
checks replay fingerprints. Both flag-based and saved-choice paths were tested.

32 off/on pairs: all eight active pairs and all sixteen reduced-effects pairs
are byte-identical. Twelve normal monitor pairs have nonzero changes strictly
inside glass; zero outside pixels change. Independent score-card interiors are
identical. Inspected painted results show clear A 100.0% / B 0.0% labels and crisp
foreground. A no-paint fixture that closed the program was not accepted as
score-text evidence; final fixtures paint before the reveal.

- Saved-choice Debug off (environment unset):
  `out/validation/crt-saved-off/x64-debug/909ed349931b4f06bf95fceb850328ea`
- Saved-choice Debug on (environment unset):
  `out/validation/crt-saved-on/x64-debug/a3d9d306de134685b269def6f14f57bf`
- Comparison: `out/validation/crt-saved-diff.json`
- Saved-choice Release off (environment unset):
  `out/validation/crt-saved-release-off/x64-release/c2f77221d8494be5bf8008ed0ffcae98`
- Saved-choice Release on (environment unset):
  `out/validation/crt-saved-release-on/x64-release/60f58b6f1a62445e976ee1c0296907ed`
- Release comparison: `out/validation/crt-saved-release-diff.json`

### Settings and interface

Debug and Release builds passed. Focused Debug `crt_trial`, `settings`, `player_profiles` and
`menu_presentation` passed after the final UI/library rebuild:
`out/validation/crt-settings-final-core/x64-debug`.
The full focused `menu_text` CPU suite also passed:
`out/validation/crt-menu-text-final/x64-debug`. Existing independent Options
row/fit fixtures were updated for the Graphics entry and 72-pixel row spacing;
inclusive-edge acceptance and +0.25 overflow rejection remain intact.

The hidden Graphics child test passed pending/Back cancellation, durable save
versus immutable current launch, parent Options preservation, failed-write
rollback/recovery, and developer-override disclosure. Five frozen 720p captures
cover default, saved restart, save failure, saved Off and override saving.
Saved/failure/override captures were inspected: no clipping or overlaps, clear
contrast, distinct saved/current state and readable help/status.
Debug evidence: `out/validation/crt-ui/x64-debug/f69ba79dbccd4d52ba2a2ad93105f12a`.
The same focused Graphics route also passed in Release:
`out/validation/crt-ui-release/x64-release/82592bd19cc94b609c86f1d194a528e7`.

### Reproduction

From an x64 Visual Studio developer PowerShell, after the appropriate build:

```powershell
Remove-Item Env:COLOURWARS_CRT_TRIAL -ErrorAction SilentlyContinue
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -CrtTrialOnly -OutputDirectory out/validation/crt-off
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -CrtTrialOnly -CrtSavedOn -OutputDirectory out/validation/crt-on
python scripts/compare-crt-captures.py OFF/captures ON/captures
./scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -GraphicsSettingsOnly
```

Use the printed captures directories in the comparison. Pillow/numpy are
required. The comparator enforces expected filenames, matching state CSVs,
glass containment, nonzero enabled changes, neutral equality and score interiors.
All wrappers use isolated saves. No comparison manipulates the owner's profile.

## Remaining gates

Fixed captures do not qualify moving transitions, pause/resume or resize while
playing, non-D3D11 backends, hardware floors or all text at every size. The
texture is conspicuous: retain opt-in and review owner taste before changing it.
The runner does not record comparable gameplay phase timings; hidden readback
runtimes are not FPS and no performance claim is made. Native builds were
capped at two compiler jobs after checking for competing build/game processes.

## Adapter evidence policy

The engine pin correctly classifies the hosted Microsoft Basic Render Driver
as software (ENG#34). CI previously ran on that same CPU rasteriser but received
an incorrect hardware label. It now explicitly selects
`-HostedSoftwareFunctional` in both Debug and Release. This separate lane
requires a D3D11 software device and runs every existing movement/pixel assertion
without skips or changed tolerances. Hardware, unknown and null devices are
rejected in that lane; it cannot provide hardware or performance acceptance.

The default movement probe remains strict hardware-only in both configurations.
The older Debug-only `-AllowSoftwareAdapter` retains its Release refusal and
cannot be combined with the hosted lane. Both wrapper and native probe enforce
the requested evidence policy; compile-time policy cases cover their boundaries.

Before merging this pin/renderer change, strict hardware-default Debug and
Release movement checks must pass on the final source and engine revisions,
with recorded device identity and no software opt-in. This is separate local
qualification, not a hosted check or a fabricated GitHub status. Hosted `gate`
remains required and cannot be bypassed. The nightly painter and paint-fx wrappers now also select an explicit
`-HostedSoftwareFunctional` lane. It requires software, rejects hardware/null/
unknown devices, retains all pixel assertions, and records the requested policy
and observed device in each result. The sweep forwards it to every leaf and
Remembered setup forwards it to its second process. Local calls without the
switch still require D3D11 hardware. Existing known-failure exemptions are not
expanded. A green scheduled sweep may close its functional-CI issue, but does
not discharge the separate real-GPU obligation above. No manual close or
hardware qualification follows from this policy change.

This follow-up changes admission only. Native Debug/Release compilation,
hosted painter/paint-fx pixels and strict hardware checks must be reported as
separate results; source checks alone cannot establish any of them.
