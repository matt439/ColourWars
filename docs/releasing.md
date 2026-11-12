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
[HOW-TO-PLAY.txt](HOW-TO-PLAY.txt) now explains the existing paint-underfoot rule,
and the Movement Course's refill lesson says it too (2026-11-12): own paint
refills faster, enemy paint stops it. A live cue for standing on enemy paint, on
the paint meter or the step sound, remains `REV#10` and is the author's
presentation call.

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

From a plain PowerShell at the repository root, one command builds the
candidate, checks it and writes its receipt (`REL#3`):

```powershell
pwsh -NoProfile -File scripts/release-candidate.ps1
```

[`scripts/release-candidate.ps1`](../scripts/release-candidate.ps1) refuses a
tree with any uncommitted or untracked change, and an engine submodule that is
not at the pin, before it builds anything. Then, into one new folder
`out/validation/release-candidate/<commit>-<UTC time>/` (`-OutputDirectory`
moves the parent, relative to the caller's location), it:

1. builds and packages `x64-release` through `scripts/build.ps1 -Package`,
   which loads the Visual Studio C++ environment and usable vcpkg itself, and
   keeps the build log;
2. inspects the ZIP with `scripts/check-release-package.ps1`: required
   runtimes, help, licences, the accepted bank's hash, no PDB and no
   developer-console classes, and the executable naming its symbols as plain
   `ColourWars.pdb`;
3. checks that the packaged `build-info.txt` names this commit, the engine
   pin, the version and `Release`;
4. starts the extracted game with `scripts/check-startup.ps1 -RequireAudio
   -SystemOnlyPath`: a separate working directory, an isolated save, a
   system-only DLL search path and a hidden, muted process. It checks content
   loading, executable version, DPI awareness, window fit and graceful
   shutdown;
5. starts a fresh extraction with `sounds/paint_exe.xwb` removed
   (`-RequireNoAudio`), leaving the first intact. This must load the silent
   substitute with only the expected missing-bank diagnostic. It does not
   qualify listening or the normal audible bank's mix.

It writes `candidate.json` beside the evidence: commit, engine pin, version,
the ZIP's path, size and SHA256, the packaged `build-info.txt`, and each
check's outcome, log and `result.json`. A copy of the ZIP stays in the folder
for the sandbox run (`REL#2`). Every check runs even when an earlier one
failed, and the script fails at the end if any did. `-SkipBuild` checks the ZIP
already in `out/build/x64-release/packages/` instead; it must still name this
clean commit, so a ZIP from an older commit is refused rather than reused.
[`scripts/check-release-candidate.ps1`](../scripts/check-release-candidate.ps1)
checks the refusals in a scratch repository without a build, in CI's Release
leg.

Check that the packaged guide and runtime assets match the committed sources.
Evidence under `out/validation/` is local, not a public download or a hosted
CI result.

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
[`publish` job](../.github/workflows/ci.yml) runs
[`scripts/ci/release.py`](../scripts/ci/release.py) to check that the tagged
commit is on `master` and that its own `CMakeLists.txt` declares the tag's
version, then publishes that run's inspected and started ZIP as the GitHub
Release, with the same executable's `ColourWars.pdb` beside it and the notes
`release.py` writes. `scripts/ci/test_release.py` tests the check and the notes
in the lint job (`REL#4`). The local candidate is for review; the published
archive comes from that tag's own successful run.

### If publish fails

Nothing is published until `gh release create` succeeds; it creates the
release with both assets in one call. What to do depends on the step that
failed:

- **The tag check** (`The tag is on master and names the packaged version`).
  The tag is wrong: it names a commit off `master`, or a version other than
  that commit's `project(VERSION)`. A rerun cannot help. Delete the tag
  (`git push origin :refs/tags/v<version>` and `git tag -d v<version>`), fix
  the cause (tag `master`'s commit, or land the version bump through a pull
  request first), then push the tag again.
- **A Windows job.** `publish` did not start. If the tree failed CI it is not
  a release: fix it through a pull request, then delete the tag and push it
  again on the fixed commit. Use **Re-run failed jobs** instead only when the
  runner failed rather than the tree, such as a download or cache failure.
- **Downloading the artifacts or creating the release.** The ZIP and PDB
  artifacts are kept 14 days. Within that time, use **Re-run failed jobs** on
  the tag's run: it publishes the same checked ZIP and PDB without a rebuild.
  If a partial attempt left a release with that tag, delete the release, not
  the tag (`gh release delete v<version>`), before rerunning. After 14 days
  the artifacts are gone: delete the tag and push it again, which builds and
  checks the tree afresh.

Never attach a ZIP or PDB built anywhere else to the release by hand: the
published archive is the one its own run inspected and started, and the PDB
fits only the executable it was linked with.

After publication, point the README's download step at the release, verify the
asset name and player instructions, and close `FTR#1` in the same commit that
finishes the remaining documentation work. Additional channels (`REV#67`) and
remote-streaming guidance (`REV#69`) remain separate choices.

## Release work items (`REL`)

Work this guide owns, found by the 2026-11-12 audit of [TODO.md](../TODO.md).
Each item has a row there. Ids are `REL#n` and are never reused. Line numbers
were checked against `68e3dc2` with the engine pin at `e7fc50e` (`v0.3.0` plus
nine commits).

`REL#5` and `REL#7` changed the executable. A candidate built before they
landed proves the procedure, not the archive the first tag will publish.

### Release candidate evidence (`REL#1`)

**Open.** No candidate evidence exists for the current `master`. The
documentation and packaging pass is merged, and NOW asks for a clean
candidate. On MattPC a ZIP built from `fba0bb1` (the same tree as `68e3dc2`)
is in `out/build/x64-release/packages/`, but `out/validation/release-preparation/`
does not exist: no package check, startup result or hash was kept beside it.

**First step.** On a clean checkout of `master`'s tip, run
[Build a clean candidate](#build-a-clean-candidate) as written:
`scripts/release-candidate.ps1`, which runs the package check, the packaged
`build-info.txt` check, `check-startup.ps1 -RequireAudio -SystemOnlyPath` and
the missing-bank fallback start.

**Done when** this entry records the commit, engine pin, version, ZIP SHA256 and
each check's result from `candidate.json`, and the local evidence holds the
build log, `build-info.txt` and both startup `result.json` files. `build-info.txt` must not
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

**Done 2026-11-12.** [Build a clean candidate](#build-a-clean-candidate) is
now one command, [`scripts/release-candidate.ps1`](../scripts/release-candidate.ps1).
It refuses a dirty tree (any tracked change or untracked file) and an engine
submodule that is not at the pin before it builds, runs the four steps that
were typed by hand, adds a check that the packaged `build-info.txt` names the
clean commit and pin, and writes `candidate.json` (commit, engine pin, version,
ZIP SHA256 and each check's outcome, log and result path) into one folder
under the repository's `OutputDirectory` rule.
[`scripts/check-release-candidate.ps1`](../scripts/check-release-candidate.ps1)
covers the refusals (an untracked file, an edited tracked file, `-SkipBuild`
with no ZIP, and no evidence folder left by a refusal) in a scratch repository
without starting a build, under Windows PowerShell 5.1 and pwsh, in CI's
Release leg. Both are in [scripts/README.md](../scripts/README.md#release).
The candidate evidence itself is still `REL#1`.

### Tested publish step (`REL#4`)

**Done 2026-11-12.** The `publish` job's inline `sed` version match, ancestry
check and heredoc notes are now
[`scripts/ci/release.py`](../scripts/ci/release.py). Its `check` refuses a tag
off `master`, a malformed tag, and a tag that does not match the
`project(VERSION)` in **the tagged commit's** `CMakeLists.txt` (read with
`git show`, not from the working tree), and hands `publish` the ZIP and PDB
asset names. Its `notes` writes the release notes, which keep `FTR#6`'s player
instructions and add one line saying the PDB is not needed to play.
[`scripts/ci/test_release.py`](../scripts/ci/test_release.py) runs in the lint
job's existing `unittest discover -s scripts/ci` against real Git histories: a
matching tag, a mismatched tag, the version read from the tagged commit rather
than a later one, a tag off `master`, malformed tags, an unreadable version, an
unknown commit, the `GITHUB_OUTPUT` values and the notes text. The job itself
still cannot run before a real tag, so its first end-to-end run is `v0.1.0`;
[If publish fails](#if-publish-fails) says when to rerun the job and when to
delete and push the tag again.

### Version in the game and its failure record (`REL#5`)

**Done 2026-11-12.** The title screen's build line is now
`v0.1.0 / Build <revision> / Release`. It is player copy, so it comes from
`presentation.json` (`menu_text.title.build`, a template that must use
`{version}`, `{revision}` and `{configuration}` once each); `qps-ploc` brackets
it like the rest of the title, and a locale can reword it but not drop a field
([menu-presentation.md](menu-presentation.md)). `cmake/build_info.h.in` gained
`version` and `game_short_revision`; `build_info::label` stays the untranslated
identity the console and replay files use. The title layout check
(`presentation_validation_menu.cpp`) and the painter's two title checks measure
the formatted line, so it still has to fit where the label did.
`last-error.log` gained `version=`, which the `fatal_report` probe and
`check-failure-reporting.ps1` check beside the other fields, and
[failure-reporting.md](failure-reporting.md#persisted-metadata-and-privacy)
lists it. The bug form asks for the version shown under the title.

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

**Done 2026-11-12.** The symbols half of `REV#63`
([record](review/2026-11-10-project-review.md#no-crash-dump-and-no-release-symbols-rev63)).
Every target now compiles Release with `/Zi`
(`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT` in the root `CMakeLists.txt`), and
`ColourWars` links with `/DEBUG:FULL /OPT:REF /OPT:ICF /PDBALTPATH:%_PDB%`
(`game/CMakeLists.txt`), so `out/build/x64-release/game/ColourWars.pdb` is
written and the executable's CodeView record names `ColourWars.pdb` with no
path. The ZIP still installs only the executable;
`scripts/check-release-package.ps1` refuses a `.pdb` as before and now also
reads the executable's debug directory and requires exactly that name. CI's
Release leg uploads the PDB as `ColourWars-symbols-<commit>`, and `publish`
attaches it to the GitHub Release. [The development guide](development.md#release-zip)
says where the symbols are. The crash-dump half, which is the privacy
question, stays `REV#63` and the author's call.
