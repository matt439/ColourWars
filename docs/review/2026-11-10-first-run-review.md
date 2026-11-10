# First-run review, 2026-11-10

A review of what ColourWars is like for an inexperienced person to install and
play for the first time. This file is a record, frozen as written. It owns the
`FTR` rows in [TODO.md](../../TODO.md), and under that file's rule 2 the row,
not this record, holds a finding's current status.

## Method

The reviewer is someone who has never used CMake, vcpkg or Visual Studio:
a friend who wants to play, or a hobbyist. They arrive at the GitHub page and
read `README.md` first.

1. **Walk.** Six reviewers each took one journey: first contact with the
   README, the prerequisites, a real build from a fresh clone, first launch
   and learning to play, looking for help, and the release ZIP a player would
   one day download. The build reviewer cloned into a scratch folder and ran
   `scripts/build.ps1` with `VCPKG_ROOT` unset and no MSVC environment. It also
   built a copy whose `external/labrador` was emptied, standing in for GitHub's
   **Download ZIP**, and repeated the clone under deep folders.
2. **Verify.** One skeptic per journey reopened every cited file, re-ran cheap
   checks and confirmed, corrected or refuted each finding, then listed what
   the walker missed.
3. **Write-up.** Refuted findings were dropped, ratings corrected and
   duplicates merged. What survived is below.

Evidence is by path and line as they stood at `1d863ee3`. Nothing was played
on a pad: this machine has none. Timings are from MattPC.

## Summary

A newcomer cannot get from the GitHub page to a match on their own today.
`README.md:8` promises "You don't need Git, Visual Studio or a terminal to
play", but there is no release and no tag. The interim route is a CI artifact
that needs a GitHub account, sits among about 14 artifacts, comes double-zipped
and expires after 14 days. Without an account the only route is Git, a
multi-gigabyte Visual Studio install and a `git clone` typed into a terminal.

Once Visual Studio is installed, `build.cmd` works well. It finds Visual
Studio, loads `vcvars64`, falls back to the bundled vcpkg, repairs a clone made
without submodules and catches a Download ZIP copy. It built in 37.7 s warm and
about 47 s with dependencies built from cold. The game is friendly once it
runs: keyboard and mouse drive every menu, each screen names its keys, a
controls card appears at the countdown, bots cover solo play and failures
always show a dialog. `docs/HOW-TO-PLAY.txt` is good. Most of the friction is
documentation and error wording, not code.

## Install and download

### No release exists (FTR#1)

The Releases page is empty, and `git tag` prints nothing. `README.md:10-19`
sends players to a pull-request CI artifact
(`.github/workflows/ci.yml:385`, `retention-days: 14`) and otherwise to
`build.cmd`. That breaks all three parts of the promise at `README.md:8`.
`CMakeLists.txt:3` is already `VERSION 0.1.0` and the `publish` job
(`.github/workflows/ci.yml:399-435`) is ready. Pushing `v0.1.0` would turn step
1 into one link to `releases/latest`, and the artifact paragraph could go.
The first tag is the author's call under AGENTS.md. The channel question beyond
GitHub is `REV#67`.

### The build fallback leads with the hard route (FTR#2)

The `#building` anchor lands on `wWinMain`, a clone command
(`README.md:257`) and a Prerequisites list (`README.md:266-271`) that leaves
out Git and says `VCPKG_ROOT` must be set and configuring fails without it.
The `build.cmd` paragraph that contradicts it comes after
(`README.md:277-279`). Nothing states download size, disk space or time
(measured here: a 311 MiB clone and a 0.58 GB Release tree, plus the Visual
Studio workload). `README.md:253-254` still says a plain clone gives a CMake
error, although `scripts/build.ps1:38-40` now fetches the submodule. The
Download ZIP message (`scripts/build.ps1:34-36`) asks a reader who was told
they need no terminal to type `git clone`, without saying where.

The fix is an "Easy build" section ahead of the developer steps: install Git
for Windows (with a link) and Visual Studio Community with "Desktop development
with C++", with rough size and time; don't use Download ZIP; open a terminal
and clone into a short folder such as `C:\ColourWars`; double-click
`build.cmd`. `VCPKG_ROOT` is then marked as applying to the by-hand route only.

### build.ps1 failures it does not foresee leave no next step (FTR#3)

- `scripts/build.ps1:96` and `:99` say only "the messages above say why", and
  nothing writes a log.
- `vcvars64` output is discarded (`scripts/build.ps1:65`, `>nul 2>&1`), so a
  broken Visual Studio or SDK shows only "Loading ... failed."
- A missing Windows SDK reaches
  `external/labrador/cmake/compile_shaders.cmake:27-35`, which first tells a
  `build.cmd` user to switch prompts.
- `vswhere` has no version floor (`scripts/build.ps1:47`), so Visual Studio
  2019 gets an "add the vcpkg component" message it cannot follow.
- A stale `VCPKG_ROOT` from an earlier experiment is trusted unchecked
  (`scripts/build.ps1:73`).
- The missing-Visual-Studio message (`scripts/build.ps1:56-57`) points to
  "README.md, under Play", whose fallback ends with "use `build.cmd`".
- `scripts/build.ps1:94` warns that configure can take several minutes; the
  compile step has no such hint and looks hung.
- A deep clone folder fails confusingly. From a root of about 104 characters,
  the first cold vcpkg build dies with `C1083 ... Invalid argument` and a
  misleading "unable to find a build program corresponding to "Ninja"". The
  real cause, a 257-character path, is only in a vcpkg log.

The fix: tee output to a log under `out/` and print its path with the help
link, show `vcvars64` output on failure, check `fxc` after `vcvars64`, add
`-version '[17.0,'` to `vswhere`, validate `VCPKG_ROOT` and fall back to the
bundled copy, warn when the root path is long, add a compile-step hint, and
make the missing-Visual-Studio message stand on its own.

### Long engine test filenames break deep clones (FTR#4)

From a clone root of about 93 characters, `git clone --recurse-submodules`
prints `Filename too long` seven times and "fatal: Unable to checkout ...
external/labrador". The failing paths are golden images under
`external/labrador/tests/render/golden/`. The game still builds, but the
message reads as a failed download. Shortening the names is engine work; what
this repository owes is a report in the
[ledger](engine-reports.md).

### No troubleshooting section (FTR#5)

README's headings are Play, Your own stages, Building and the developer
material; none is about help. There is no written answer for network or disk
failures in the vcpkg step, long paths, antivirus, SmartScreen on a source
build, Remote Desktop or VMs, graphics drivers, or running from inside the
ZIP. The few answers are spread across README Play, `scripts/build.ps1`
strings and `docs/HOW-TO-PLAY.txt`; the Remote Desktop and VM limit appears
only at `docs/HOW-TO-PLAY.txt:24-25`. `README.md:76-80` links only
`docs/failure-reporting.md`, which opens with `wWinMain`. The fix is an
"If something goes wrong" section after Play, linked from HOW-TO-PLAY and from
the build script's failure lines.

### The release ZIP's contents and name (FTR#6)

The ZIP ships the 540-line developer `README.md` beside `HOW-TO-PLAY.txt`
(`cmake/release_package.cmake:47-48`), and a stock Windows install has no
`.md` association. Its name is fixed although the version is known
(`cmake/release_package.cmake:78-79`), so two downloads look identical. The
extracted folder is nested `ColourWars-windows-x64\ColourWars-windows-x64\`
(`cmake/release_package.cmake:81`) and holds about 25 loose files beside the
exe. `publish` adds `--generate-notes`, which appends the merged-PR list under
the player notes. The fix: drop `README.md` from the ZIP or name the guide
"READ ME FIRST", put the version in the ZIP name, flatten the nesting, and
consider dropping the generated notes. The README, HOW-TO-PLAY and the CI
inspection name the ZIP and move with it.

### Clean builds print 44 CMake warnings (FTR#7)

A successful first build prints 44 "CMake Warning (dev)" blocks from
`file(REAL_PATH)` on folders that do not exist yet
(`cmake/stage_runtime_assets.cmake:74`, `:97`). A newcomer reads them as
something gone wrong. Guard them with `if(EXISTS)`.

## First launch and learning to play

### Non-XInput pads are silently ignored (FTR#8)

A friend who plugs in a DualSense sees nothing happen. "Connect a controller"
never appears, because the keyboard slot always counts as connected
(`game/states/menu_input.cpp:201`, `game/states/main_menu.cpp:184-188`). The
README says "Xbox-style" (`README.md:29`) and "XInput gamepads"
(`README.md:89`) but never names PlayStation or Switch. A second friend at one
PC needs an Xbox pad and meets a seat they cannot claim, with no explanation on
screen. The reader itself is `ROAD#23`. The fix here is words: say in README
Play and HOW-TO-PLAY that PlayStation and Switch pads are not recognised (and
that Steam or DS4Windows can present them as Xbox pads), and give Join Players
a hint that a seat needs an Xbox-style pad or can be made a bot.

### The player guide is hard to find (FTR#9)

`README.md:32` names `HOW-TO-PLAY.txt` only "in the ZIP", as code rather than a
link. Only `cmake/release_package.cmake:48` installs it, so it is missing
beside the exe `build.cmd` produces. README's requirements
(`README.md:27-28`) leave out the Remote Desktop and VM limit. A source-built
player also has no clear way to play again: the exe sits in
`out\build\x64-release\game` among 297 build files, and re-running `build.cmd`
rebuilds before it launches. The fix: link `docs/HOW-TO-PLAY.txt` from Play,
add the Remote Desktop and VM line to the requirements, and have
`scripts/build.ps1:111` print the guide's path and the exe's path to run next
time.

### The only tutorial cannot be found (FTR#10)

The Movement Course lesson cards appear only with one player on that arena
(`game/objects/level_builder.cpp:124-125`). HOW-TO-PLAY never mentions them,
and the README calls the arena experimental (`README.md:65-67`). The cheap fix
is a line in HOW-TO-PLAY and the README: "New? Single Match, 1 Player,
Movement Course." A Practice item on Home is the larger option.

### Startup failures show developer text with no remedy (FTR#11)

With no hardware device, over Remote Desktop or in a VM, the dialog reads
"Failure while initializing ... No Direct3D hardware device found". The game
already classifies this as `no_hardware_device`
(`game/fatal_report.cpp:117-120`) but the dialog does not use it. A partial
copy without content gives a raw `read_json_file - cannot open
'...\./tuning.json'`. The dialog ends with "Diagnostic metadata saved to ..."
(`game/fatal_report.cpp:153-154`) and stops: it does not say where to report,
and `last-error.log` deliberately leaves out the error text
(`game/fatal_report.cpp:167-189`, `README.md:77-78`), so sending the file alone
loses the cause. The fix: a remedy line for `no_hardware_device` (Direct3D 11
card, update the driver, not over Remote Desktop or in most VMs) and for
missing content (extract the whole ZIP and run from that folder), "A short
error report was saved to" in place of "Diagnostic metadata", and a line
asking the player to copy the message and attach the file when reporting. The
issue form itself is `REV#15`. These strings are written before presentation
text can be trusted to load, so they may have to stay in C++.

### The first window can be larger than the screen (FTR#12)

The default is a 1920x1080 client area, windowed (`game/save/save.h:13-15`),
and the window is not clamped to the work area
(`external/labrador/engine/app/win32/window.cpp:78-97`). On a 1080p laptop,
worse at 125-150% scaling, the title bar and taskbar push the prompt bar off
screen. There is no Alt+Enter. The fix is a first-run size fitted to the
monitor's work area, or borderless full screen on first run. If the engine's
window cannot be told the work area, that part is a report.

### A fresh save opens the arena page on King of the Hill (FTR#13)

`game/states/main_menu.h:529` starts at `slot = 0`, King of the Hill, among 13
entries that include six "Trial" variants, and Random is the last slot. The
arenas replaced in Random on 2026-11-05 are the first a newcomer sees. Later
sessions remember the last setup, so this is first-run only. Opening on Random
or a chosen starter, and grouping or hiding the Trial stages, sits beside the
2026-11-05 arena decision, so it is the author's call.

### The setup path is long, and the save folder is hidden (FTR#14)

There are about eight confirm screens before a first match. The save and
`stages` folders are never shown in the game, and the player creates `stages`
by hand (`docs/HOW-TO-PLAY.txt:77-79`). Candidates: a Quick Play item, an
"Open save folder" item and creating `stages` on first run. None is designed.

## Finding help

### The README mixes audiences (FTR#15)

The 540-line README puts art direction, review status and TODO material right
after the install steps (`README.md:66-74`, "five round-2 rules are accepted
... retired on 2026-10-15"; `README.md:128` "Physical-pad feel remains under
review"). The controls table starts at line 162, and the intro skip keys sit at
the end of a ten-line backstory paragraph (`README.md:35-44`). CI, the engine
pin and repository history fill `README.md:309-372`, `439-470` and `508-526`.
The fix is to keep Play to download, requirements, controls and help, cut the
story to a line linking `docs/backstory.md`, and move the rest to
`docs/` or a `CONTRIBUTING.md` (`REV#15`). `REV#12` called this trimming a
separate M and a matter of taste, so the shape is the author's call.

### The docs index is written for maintainers (FTR#16)

`docs/README.md:7` opens with "Reading the code". HOW-TO-PLAY sits under
"Using the tools" beside the debug-console record, and 31 of 50 top-level docs
are `polish-*.md`. `TODO.md` and `scripts/README.md` carry no maintainer-notes
banner, and HOW-TO-PLAY's last line points only at the repository. `NOTICE`
opens plainly but does not say in one line that the ZIP may be shared. The fix
is a Players block at the top of the index (HOW-TO-PLAY, troubleshooting, level
editor and format, backstory) and a line saying everything below is maintainer
material.

## Already tracked

Reviewers also raised these. Each has a row already, so none has an `FTR` row.

| finding | row |
|---|---|
| No screenshot or clip in the README | `REV#12` |
| The F1/Back developer panel is a player control in Release | `REV#13` |
| No issue form, CONTRIBUTING or SECURITY file | `REV#15` |
| Large docs mix manual and records | `REV#18` |
| PlayStation and Switch pads need a reader | `ROAD#23` |
| The level editor is not in the ZIP, though HOW-TO-PLAY points at it | `REV#64` |
| Unsigned GitHub ZIP as the only channel | `REV#67` |
