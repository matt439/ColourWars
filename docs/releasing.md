# Preparing a release

The Release ZIP is the player download. Its version comes from
`project(VERSION)` in [CMakeLists.txt](../CMakeLists.txt); the current first-release
target is `v0.1.0`. [The development guide](development.md#release-zip) describes
what it contains. [AGENTS.md](../AGENTS.md#commits-and-pull-requests) owns the
branch, required `gate` and publication rules.

## Current preparation scope

**Selected by the author, 2026-11-11: docs and packaging first.** Prepare a
player-focused README, the packaged help and contributor/bug-report guidance,
then build and inspect a fresh Release ZIP. Gameplay decisions remain open;
the selection does not change stage ordering, Random palettes, out-of-bounds
behaviour, tuning, audio or the accepted art direction.

The documentation pass completes the README reshape (`FTR#15`) and contributor
and issue guidance (`REV#15`). The README links to the source-build and
maintainer material in [development.md](development.md). The packaged
[HOW-TO-PLAY.txt](HOW-TO-PLAY.txt) now explains the existing paint-underfoot rule;
the in-game refill lesson and any live cue remain `REV#10`.

Author calls left for this release aim:

- **Sub-weapon play review (`ROAD#11`).** The implementation is complete; the
  author's attended review is still open in the
  [roadmap](feature-roadmap.md#sub-weapons-road11). Documentation and a startup
  check do not close it.
- **README image/clip (`REV#12`).** The author still chooses the hero material;
  this pass adds no new image or implied art acceptance.
- **First tag (`FTR#1`).** The author decides when to publish. The download text
  must describe the available pre-release routes until a release exists; after
  publication, replace the artifact fallback with the direct release download.

Other design calls stay indexed in [TODO.md](../TODO.md), including `REV#1`'s
out-of-bounds handling and `REV#26`/`FTR#13`'s arena presentation. They have not
been accepted by selecting this documentation/package pass. Arena artwork
(`POL#16`) remains deferred.

## Build a clean candidate

Commit validated source and documentation on the working branch first. A
candidate's `build-info.txt` must name that commit without `-dirty`, and name
the checked-out engine pin. An older ZIP can have the same version while
containing older runtime data; do not reuse it solely because its name matches.

From a plain PowerShell at the repository root, the build helper loads the
Visual Studio C++ environment and usable vcpkg itself:

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Preset x64-release -Package
pwsh -NoProfile -File scripts/check-release-package.ps1 `
  -OutputDirectory out/validation/release-preparation/package
pwsh -NoProfile -File scripts/check-startup.ps1 `
  -Executable out/validation/release-preparation/package/ColourWars-windows-x64/ColourWars.exe `
  -RequireAudio -SystemOnlyPath `
  -OutputDirectory out/validation/release-preparation/startup
```

The build produces
`out/build/x64-release/packages/ColourWars-<version>-windows-x64.zip`.
The package checker extracts it, checks required runtimes, help, licences,
accepted-bank hash and the absence of developer-console classes. The startup
check uses a separate working directory, an isolated save, a system-only DLL
search path and a hidden, muted process. It checks content loading, executable
version, DPI awareness, window fit and graceful shutdown.

Check the optional-bank fallback separately in a fresh extraction, leaving the
normal candidate intact:

```powershell
. ./scripts/release-package-path.ps1
$fallbackFixture = Join-Path 'out/validation/release-preparation/fallback' ([Guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath (Get-ColourWarsReleasePackagePath (Get-Location).Path) `
  -DestinationPath $fallbackFixture
Remove-Item -LiteralPath (Join-Path $fallbackFixture 'sounds/paint_exe.xwb')
pwsh -NoProfile -File scripts/check-startup.ps1 `
  -Executable (Join-Path $fallbackFixture 'ColourWars.exe') `
  -RequireNoAudio -SystemOnlyPath `
  -OutputDirectory out/validation/release-preparation/fallback-startup
```

This must load the silent substitute with only the expected missing-bank
diagnostic. It does not qualify listening or the normal audible bank's mix.

Retain the build log, extracted `build-info.txt`, startup `result.json` and ZIP
SHA256 beside the local evidence. Check that the packaged guide and runtime
assets match the committed sources. Evidence under `out/validation/` is local,
not a public download or a hosted CI result.

These checks establish package integrity and startup on the machine that ran
them. The hardware, participant and listening checks waived on 2026-11-05
remain waived, not performed; the newer sub-weapon play review remains open.
See [what a green pull request means](../AGENTS.md#what-a-green-pull-request-means).

## Publish only after the author authorizes publication

Keep work local until a push is authorized, and open a pull request only when
requested. Before merging, bring the branch up to date with `master` and wait
for the required `gate` on that updated tree. No ruleset bypass is part of
release preparation.

When the author authorizes the first tag, tag the selected commit on `master`
with the version matching `project(VERSION)`. The tag push runs full CI. Its
[`publish` job](../.github/workflows/ci.yml) checks master ancestry and version,
then publishes that run's inspected and started ZIP as the GitHub Release.
The local candidate is for review; the published archive comes from that tag's
own successful run.

After publication, point the README's download step at the release, verify the
asset name and player instructions, and close `FTR#1` in the same commit that
finishes the remaining documentation work. Additional channels (`REV#67`) and
remote-streaming guidance (`REV#69`) remain separate choices.
