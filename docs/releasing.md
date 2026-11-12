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
out-of-bounds handling and `REV#26`'s arena presentation (which took in `FTR#13`). They have not
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

## Release work items (`REL`)

Work this guide owns, found by the 2026-11-12 audit of [TODO.md](../TODO.md).
Each item has a row there. Ids are `REL#n` and are never reused. Line numbers
were checked against `68e3dc2` with the engine pin at `e7fc50e` (`v0.3.0` plus
nine commits).

`REL#5` and `REL#7` change the executable. A candidate built before they land
proves the procedure, not the archive the first tag will publish.

### Release candidate evidence (`REL#1`)

**Open.** No candidate evidence exists for the current `master`. The
documentation and packaging pass is merged, and NOW asks for a clean
candidate. On MattPC a ZIP built from `fba0bb1` (the same tree as `68e3dc2`)
is in `out/build/x64-release/packages/`, but `out/validation/release-preparation/`
does not exist: no package check, startup result or hash was kept beside it.

**First step.** On a clean checkout of `master`'s tip, run
[Build a clean candidate](#build-a-clean-candidate) as written: the package
check, `check-startup.ps1 -RequireAudio -SystemOnlyPath` and the
missing-bank fallback start.

**Done when** this entry records the commit, engine pin, version, ZIP SHA256 and
each check's result, and the local evidence holds the build log,
`build-info.txt` and both startup `result.json` files. `build-info.txt` must not
say `-dirty`. A later commit supersedes the record; rebuild rather than reuse.

### Sandbox run of the candidate (`REL#2`)

**Open.** The only Windows Sandbox runs of the ZIP were made at engine `v0.2.0`
on 2026-10-28 and 2026-10-30
([`ROAD#37`](feature-roadmap.md#windows-sandbox-run-of-the-release-zip-road37)).
The pin is now past `v0.3.0`. Labrador's `PI-14` waits on exactly this run:
its `TODO.md:112` at the pin says "close as delegated, or await its `v0.3.0`
run". `ROAD#1`'s clean-PC run was waived on 2026-11-05; the sandbox run was not.
`scripts/sandbox-release.ps1 -Run` already works on MattPC, so no author
hardware is needed. This guide does not yet mention the sandbox.

**First step.** Run `pwsh -NoProfile -File scripts/sandbox-release.ps1 -Zip
<REL#1 candidate> -Run`.

**Done when** the receipt's hash, engine revision, exit code and guest details
are added to `ROAD#37`'s record, the candidate steps above name the sandbox run,
and Labrador's `PI-14` can be pointed at the record.

### One-command candidate receipt (`REL#3`)

**Open.** [Build a clean candidate](#build-a-clean-candidate) is four commands
run by hand, including a typed `Expand-Archive`/`Remove-Item` fixture. The
clean-tree check and the evidence (build log, `build-info.txt`, `result.json`,
ZIP SHA256) are also gathered by hand, and NOW asks for a rebuild after every
commit. `CI#6` scripted only the package inspection
(`scripts/check-release-package.ps1`).

**First step.** Write `scripts/release-candidate.ps1`. It refuses a dirty tree,
runs the four steps into one `-OutputDirectory` under the repository's
`OutputDirectory` rule, and writes `candidate.json`: commit, engine pin,
version, ZIP SHA256 and each check's result path.

**Done when** this guide's candidate procedure is that one command, the script
is in [scripts/README.md](../scripts/README.md), and a check covers its refusals
(dirty tree, missing ZIP) without starting a build.

### Tested publish step (`REL#4`)

**Open.** The `publish` job has never run, because no tag exists. Its version
match is an inline `sed` (`.github/workflows/ci.yml:419`), a third parser beside
CMake and `scripts/release-package-path.ps1:4`. Its release notes are an inline
heredoc (`ci.yml:431-439`). `scripts/ci/test_lint.py` tests tag selection
(lines 162 and 228) but nothing tests publish. A tag that does not match
`project(VERSION)` is the only rehearsal possible, so the first time this code
runs is the real `v0.1.0`. This guide says nothing about a publish that fails
after the tag is pushed.

**First step.** Move the ancestry and version check and the notes into
`scripts/ci/release.py`, with `test_release.py` beside it so the lint job's
existing `unittest discover -s scripts/ci` runs it (`ci.yml:42`). Call it from
`publish`. The notes keep `FTR#6`'s player instructions.

**Done when** `publish` calls the script, its tests cover a matching tag, a
mismatched tag, a tag off `master` and the notes text, and this guide has an
"If publish fails" paragraph. It should say when to rerun the job (the ZIP
artifact is kept 14 days, `ci.yml:390`) and when to delete and push the tag
again.

### Version in the game and its failure record (`REL#5`)

**Open.** A player of the `v0.1.0` ZIP never sees `0.1.0`. The title screen's
label is `Build <commit> / <configuration>` (`cmake/write_build_info.cmake:30`,
drawn at `game/states/main_menu.cpp:228`), and `last-error.log` records the
revisions, configuration and backends but no version
(`game/fatal_report.cpp:190-194`). The version reaches only `build-info.txt`
(`write_build_info.cmake:31`) and the executable's resource
(`game/CMakeLists.txt:348`). The bug form asks for "the release/tag".

**First step.** Add the version to `cmake/build_info.h.in`, put it first in the
title label (`v0.1.0 · Build <commit> / Release` or similar) and add a
`version=` field to `last-error.log`.

**Done when** the label still fits where it is checked
(`game/config/presentation/presentation_validation_menu.cpp:143` and
`scripts/acceptance/painter_visual/main_menu_checks.h:148`), the
`fatal_report` probe checks the new field beside the others
(`scripts/acceptance/fatal_report.cpp:64-68`), and
[failure-reporting.md](failure-reporting.md#persisted-metadata-and-privacy)
lists it.

### Versioning rule (`REL#6`)

**Decision.** The author's call. `CMakeLists.txt:3` is `project(ColourWars
VERSION 0.1.0)`, and the only rule written down is "bump CMakeLists.txt before
tagging" (`ci.yml:401-403`). Nothing says when the version moves after a tag or
what a minor or patch step means for this game. Left alone, every build after
`v0.1.0` is also `ColourWars-0.1.0-windows-x64.zip`, which undoes `FTR#6`'s aim
of telling two downloads apart. `gh release create` (`ci.yml:440-442`) makes
no choice about marking a `0.x` release as a pre-release.

**First step.** Draft a "Versions" section here for the author: move to the next
version in the first commit after a tag; a minor step for gameplay, content or
save-format changes and a patch step for fixes; and whether `0.x` releases are
marked pre-release.

**Done when** the author accepts or edits the rule, it is in this guide, and
`publish` follows its pre-release choice.

### Release symbols (`REL#7`)

**Open.** The symbols half of `REV#63`
([record](review/2026-11-10-project-review.md#no-crash-dump-and-no-release-symbols-rev63)).
The `x64-release` preset is plain `Release` (`CMakePresets.json:21-25`) with no
`/Zi` or `/DEBUG`, and CI uploads only the ZIP (`ci.yml:383-390`), so no PDB is
kept. A crash in a published build can never be symbolised, so this has to land
before the first tag. The PDB holds no player data; the privacy question
belongs to the crash-dump half, which stays `REV#63` and the author's call.

**First step.** Build Release with `/Zi` and link `ColourWars` with `/DEBUG
/OPT:REF /OPT:ICF` (`/DEBUG` turns those two off unless they are named) and
`/PDBALTPATH:%_PDB%`, so the executable names the PDB without a local path.
Keep the PDB out of the ZIP: `cmake/release_package.cmake:25` installs only the
executable, and `scripts/check-release-package.ps1:51` already refuses a
`.pdb`.

**Done when** the Release leg uploads `ColourWars.pdb` as an artifact named for
the revision, `publish` attaches it to the GitHub Release, the package check
still passes, and the executable's debug directory names `ColourWars.pdb` with
no path. [The development guide](development.md#release-zip) says where the
symbols are.
