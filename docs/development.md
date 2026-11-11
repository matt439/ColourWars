# Development

Build, check and package ColourWars, work with the engine submodule, and find
the code and content references. For downloading and playing, start with the
[README](../README.md) and [player guide](HOW-TO-PLAY.txt). Open work is indexed
in [TODO.md](../TODO.md).

## The game as an engine client

ColourWars is the first client of [Labrador](https://github.com/matt439/Labrador),
a 2D game engine, which this repository consumes as a submodule. The engine is
the larger project and this is the thing that proves it works — an engine with
no client is a library of guesses, and ColourWars is what keeps saying "this
API is awkward" loudly enough to be worth fixing.

```
game/                 ~78k lines     the game: states, objects, content, and
                                     ~17k of Debug-only tools in game/debug/
editor/               ~7k lines      the desktop level editor
scripts/                             acceptance checks, startup, capture and
                                     art-build scripts, developer-console scripts
                                     (indexed in scripts/README.md)
docs/                                the backstory, polish plan, roadmap,
                                     guides and reviews (indexed in docs/README.md)
external/labrador/                   the engine, as a submodule
external/rapidjson/                  vendored, for the JSON the game and
                                     editor read and write
cmake/                               runtime-asset staging, the wave-bank,
                                     font and contract-include checks, release
                                     packaging and the build stamp
```

## Building

### Easy build

If no ready-made ZIP is available, `build.cmd` handles the build from a normal
Windows desktop. It finds the tools and configures them for you.

1. Install [Git for Windows](https://git-scm.com/install/windows) and
   [Visual Studio Community 2022 or newer](https://visualstudio.microsoft.com/downloads/).
   In Visual Studio Installer choose **Desktop development with C++**, keeping
   the Windows SDK, CMake tools and vcpkg components selected. The workload is
   a download of several gigabytes; the installer shows its disk requirement.
2. Open **PowerShell** from the Start menu and enter:

   ```powershell
   git clone --recurse-submodules https://github.com/matt439/ColourWars.git C:\ColourWars
   ```

   Use a short folder path. GitHub's **Download ZIP** does not include the
   engine and cannot be built as downloaded.
3. Open `C:\ColourWars` in Explorer and double-click `build.cmd`. It builds
   Release, then opens the game. You do not need to set `VCPKG_ROOT` or open a
   Developer prompt for this route.

Allow roughly 1 GB for the checkout and Release build, plus Visual Studio and
its package caches. The checkout measured about 311 MiB and a built Release
tree about 0.58 GB on MattPC. After the tools were installed, the review's first
build took about a minute there; allow several minutes on another PC or a
slower connection for dependencies to download and build. Later builds reuse
them. [Troubleshooting](../README.md#if-something-goes-wrong) covers common failures.

`build.cmd -Preset x64-debug` builds Debug from a prompt, and
`build.cmd -Package` also makes the Release ZIP. The script installs nothing;
if a component is missing it names it. See [scripts/build.ps1](../scripts/build.ps1).

### Developer build

**Windows only**: the game starts from `wWinMain` and builds the engine's
default backends, Direct3D 11 for rendering and XAudio2 for audio.

### Clone

The engine is a submodule, so a plain `git clone` gets you an empty
`external/labrador`. `build.cmd` fetches it when needed; for the by-hand route,
clone with the submodule or fetch it before configuring:

```
git clone --recurse-submodules https://github.com/matt439/ColourWars.git
```

If you already cloned without it: `git submodule update --init --recursive`.

Labrador is public, so the submodule needs no credentials of its own.

### Prerequisites

- **Git for Windows**, to fetch the repository and its engine submodule.
- **Visual Studio 2022 or newer**, with the "Desktop development with C++"
  workload. Ninja and CMake come with it.
- **The Windows 10/11 SDK**, for `d3d11.h` and friends.
- **vcpkg**, with `VCPKG_ROOT` set for this by-hand route. `CMakePresets.json`
  reads it for the toolchain file, and configuring fails without it. Visual Studio ships a copy
  at `<VS install>\VC\vcpkg` if you have no other.

DirectX Tool Kit is in `vcpkg.json` and is fetched at configure time.

### Configure and build

For the desktop helper, follow [Easy build](#easy-build).

By hand, from an x64 Developer prompt with `VCPKG_ROOT` set:

```
cmake --preset x64-debug          # or x64-release
cmake --build --preset x64-debug
```

The game runs from its own output directory, which the build fills with the
[reviewed runtime assets](../cmake/runtime_assets.cmake) from `game/content/`, so
`out/build/x64-debug/game/ColourWars.exe` runs from anywhere. Editable sources,
historical assets and content tools live under [game/art/](../game/art/README.md).

Debug builds also take developer launch arguments and have a console, for
reviewing without walking the menus:

```
out/build/x64-debug/game/ColourWars.exe +stage movement_course +players 4
```

In a match, **`** opens the console, **F5** restarts, **F6** pauses, **F7**
steps, and **F9** toggles movement
feedback. Release ignores `+` arguments and contains none of it. See
[the developer console](debug-console.md).

There is no `ctest` step here: the engine's test suite lives with
the engine. Labrador puts its tests, its benchmark and its samples behind
`PROJECT_IS_TOP_LEVEL`, so consuming it as a submodule brings in the library
and nothing else. The game's own checks below are not CTest tests either: their
runner builds them only on request and owns their selection, deadlines,
evidence and timing summary.

### Consumer acceptance

From an x64 Visual Studio Developer shell with `VCPKG_ROOT` set:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
# A focused subset from a native command prompt or PowerShell:
pwsh -NoProfile -File scripts/run-acceptance.ps1 -Preset x64-debug -SkipBuild -Cases ammo,text_wrap
```

These commands configure/build the game, then compile small native checks
against the game library the executable is linked from (`colourwars_game`).
They cover full-cost ammunition and held-fire recovery, exposed paint surfaces
and ownership, settings persistence and write-failure recovery, camera framing
and aiming, jump buffering and ledge grace,
takeoff/landing feedback events and lifetime, stage eligibility for Random,
Movement Course routes, developer tools and audio cue content/lifecycle.
They use no graphics device or speaker and isolate
all save writes. Evidence stays under `out/acceptance/<preset>/`. CI runs the
same checks; `-SkipBuild` is for an already-current build.

CI lints changed lines for whitespace errors and conflict markers on every run,
and checks that relative Markdown links and their heading anchors resolve.
The root [.editorconfig](../.editorconfig) gives an editor the whitespace the tree
already has; it leaves indent style unset where a folder mixes tabs and spaces,
and there is no `.clang-format`.
Pushes and pull requests containing only Markdown or images/videos under
`docs/images/` stop after lint. Code, build files, scripts, schemas, runtime assets,
submodule changes and deletion of packaged Markdown still run the full Windows
checks. A manual workflow run always runs the full checks.

For paired GPU captures of the takeoff/landing prototype, run
`./scripts/check-movement-feedback.ps1 -Preset x64-debug -SkipBuild`.
It uses a hidden D3D11 hardware window, scripted jumps and isolated profile;
see [feedback evidence and limitations](polish-feedback.md).
CI runs the same correctness matrix in both presets on the runner's Basic
Render Driver, a software rasteriser the engine reports as hardware. A Debug run
with `-AllowSoftwareAdapter` also accepts the engine's WARP fallback; CI no longer
uses it. Evidence names
the actual adapter, and CI uploads JSON/CSV results and logs rather than the
large raw captures. This does not qualify hardware, movement feel or performance.
The foreground-dependent `check-debug-console.ps1` remains a local check.

Check runtime startup separately, preferably against the extracted Release ZIP:

```powershell
./scripts/check-startup.ps1 -Executable C:/Games/ColourWars/ColourWars.exe -SystemOnlyPath
```

It checks the executable's version stamp, starts it hidden from an unrelated
directory with an isolated save, checks the game window, its per-monitor v2 DPI awareness and diagnostics, then
closes gracefully. CI requires startup in both presets; Release starts on the
runner's Basic Render Driver, and a missing adapter or any application failure
fails CI. These automated
checks do not establish controller feel, audible balance or multiplayer quality;
the hardware acceptance checklist is in [the roadmap](feature-roadmap.md).

### Level editor

The normal build also creates a separate desktop level editor:

```powershell
./out/build/x64-debug/editor/ColourWarsEditor.exe
```

Open a level from `game/content/levels/`, select and drag its geometry, edit
properties, add platforms/ramps/visuals, and save. The editor includes grid
snapping, zoom/pan, spawn and bounds editing, and undo/redo. Rebuild the game
after saving to copy the source level into its runtime content directory.

See [the editor guide](level-editor.md) for controls, registering a new
stage and acceptance checks. The canvas is a layout preview; play the level
in the game to test collisions and balance. The editor is a development tool
and is not included in the game's Release ZIP.

### Release ZIP

From an x64 Developer prompt with `VCPKG_ROOT` set:

```
cmake --preset x64-release
cmake --build --preset x64-release --target package
```

The shareable build is
`out/build/x64-release/packages/ColourWars-<version>-windows-x64.zip`
(`ColourWars-0.1.0-windows-x64.zip` for version 0.1.0). Extract the entire
ZIP and run `ColourWars.exe` on Windows 10/11 x64 with a Direct3D 11-capable
GPU. The ZIP includes DirectXTK, XAudio2Redist and the release MSVC runtime DLLs
beside the executable; Visual Studio and vcpkg are not needed to play. Windows
provides Direct3D, XInput and the Universal CRT.

The title screen displays the game revision and build configuration.
`build-info.txt` also records the full game and engine revisions and selected
backends. The identifier refreshes on every build, with `-dirty` for local
changes and `source-archive` when Git metadata is unavailable. Rebuild after
committing to produce a package labelled with that commit. The release version
is `project(VERSION)` in the root `CMakeLists.txt`; it is CPack's package version,
heads `build-info.txt` and is stamped into `ColourWars.exe` (Properties >
Details). A pull request's full CI run keeps its checked Release ZIP as the
`ColourWars-windows-x64` run artifact for 14 days. The push run that follows a
merge to `master` normally has none: it skips the Windows jobs when its tree
already passed on the pull request. A `v*` tag on `master` that matches
`project(VERSION)` (`v0.1.0` for `0.1.0`) runs full CI, and its `publish` job
attaches that run's checked ZIP to a new GitHub Release.

Only the assets listed in [cmake/runtime_assets.cmake](../cmake/runtime_assets.cmake)
are staged and packaged; configure checks that the list matches the runtime manifest.
The ZIP includes licence notices, the player guide `HOW-TO-PLAY.txt`, the seven compiled fonts, six sprite sheets,
the twelve arenas (including Crossfeed, Press Tower and the explicit
static/mover/slab/ferry/gate/lift/paint-lift Turbulence Yard comparisons) and Movement Course, tuning,
presentation, the replacement sound-bank definition and validated `paint_exe.xwb`,
with audio provenance and CC0 terms. Source audio, development art, content tools and saves are excluded.
Do not distribute the normal `game/` build directory in place of this ZIP.
Debug packaging is rejected because its runtime is not redistributable.

The current docs-and-packaging preparation scope and candidate checks are in
[Releasing](releasing.md). Qualify the missing-bank fallback separately by
removing the bank from a temporary extracted copy and using `-RequireNoAudio`
for its isolated startup check.

An attended solo match and listening to music and effects are optional checks:
the author waived the prior hardware, gameplay and listening checks on
2026-11-05; they were not performed. This docs-and-packaging pass does not
reopen them or close the separate sub-weapon play review (`ROAD#11`). Settings
still live in `%LOCALAPPDATA%\ColourWars\save_data.json`, independently of
where the ZIP was extracted.

## The engine submodule

`external/labrador` pins an exact Labrador revision. A submodule pin is a fact
about this repository, not a floating reference: the engine can move and
nothing here changes until the pin is moved on purpose.

To see which revision is pinned right now:

```
git submodule status
```

That is the authoritative answer, which is why no revision is written down
here — a SHA in a README is stale the first time the pin moves.

To move it:

```
git -C external/labrador fetch
git -C external/labrador checkout <rev>
git add external/labrador
git commit -m "Move the engine pin to <rev>"
```

Build before committing the move. The engine's `/W4 /WX` applies here too — the
game links `labrador_settings`, which Labrador defines and this repository
deliberately does not copy.

Then read [docs/review/engine-reports.md](review/engine-reports.md). It is
the ledger of reports this client owes the engine, and a pin bump
is the moment the answered ones become visible here — each closed report records
what it lets this repository delete.

## Presentation direction

The chosen art direction is Paint.exe Maximalism, crude mouse-drawn box-heads.
Its five round-2 rules are accepted in [the polish plan](polish-plan.md#1-establish-the-art-and-display-rules);
the superseded workshop comparison was retired on 2026-10-15.

The presentation uses dark ink panels, cream lettering and bright paint
accents, with original menu artwork and a compact split-screen HUD. Health
and paint meters are labelled; menus mark the focused choice with both a
colour and a panel. The arenas are built from drawn stationery (pencils,
rulers, erasers, a paint tin, set squares) on tiled ground against a crude drawn
backdrop, in a muted palette with no team hue in it ([terrain kit](polish-terrain.md)).
Colours and spacing are authored in `game/content/presentation.json`; arena
decoration lives in the level JSONs. The shared menu layout, fill atlas and
team-selection pictures are described in [menu presentation](menu-presentation.md).

## Audio

A fresh clone and the Release ZIP include the accepted Paint.exe replacement
audio. The live bank is `game/content/sounds/paint_exe.xwb`; its definition is
`paint_exe.json` beside it. The build validates the tracked bank before staging
it. Normal game builds need no audio encoder or Python installation.

All 106 accepted masters, editable scores/recipes, retained CC0 recordings and
per-wave provenance are in [game/audio/paint_exe](../game/audio/paint_exe/README.md).
That page describes how to reproduce the bank with FFmpeg and
`scripts/audio/tools/XWBTool.exe`. Music uses MS-ADPCM; short effects use PCM16.
The private legacy bank is retired and never staged or packaged. See
[NOTICE](../NOTICE) for licence terms and voluntary credits.

The existing game bindings use the replacement cues, including separate
countdown beats/GO and the story's pop, sigh and chair creak. On 2026-11-05 the
author accepted the hooks and gameplay mix as they stand and waived the listening
and multiplayer checks, which were not performed; a further cue hook is
[new work](audio-inventory.md#9-runtime-migration-and-remaining-integration).

The manifest still marks the bank `"optional": true`. If it is missing, the
loader substitutes a silent bank and reports it once on stderr. Gameplay
continues; the normal Release package requires the validated audible bank.

## About the code

ColourWars is a working example of object-oriented game code: abstract
interfaces, deep and multiple inheritance, virtual behavior, composition and
explicit ownership. [The architecture overview](architecture.md) maps the
directories, the dependency direction, the services and the state stack. The
[OOP reading guide](oop-guide.md) maps those concepts to the
implementation, follows a shot from input to score, and explains how to extend
the game. [docs/README.md](README.md) indexes the rest of the docs and
[scripts/README.md](../scripts/README.md) the scripts. It is Labrador's real client; the engine's `samples/`
provide its starter template.

The project began here on 2022-12-30 as ColourWars, became ChromaClash, then
ArtAttack, and the game has now taken its first name back — in the same
repository it started in. The identifiers followed the repository rather than
leading it: the executable was `ArtAttackGame.exe` and the engine namespace
`artattack::` for six days after the split, and both were swept afterwards, on
each side of it.

`master` runs back to 2023-12-19. That is the ArtAttack repository's history,
filtered to the game's side of the split. Most of it is under `ArtAttack/`
rather than `game/`, because the rename to `game/` came late; `git log
--follow` crosses it.

The first 119 commits, 2022-12-30 to 2023-10-10, are not published. They share
no ancestor with `master` (the ArtAttack repository was started fresh and the
old sources were copied in by hand), and they carry fonts and code that are not
ColourWars' to redistribute, so they are kept in a private archive rather than
on a branch here.

The middle name's history is in `matt439/ChromaClash`, which is not public.

The game is a *client*, and it is held to a client's standard rather than the
engine's. Labrador's PHILOSOPHY governs the engine's own code and everything it
ships; game code meets the engine at small interfaces and chooses its own
grammar behind them. Two rounds of full source review are recorded in the
engine repository under `docs/review/`, and the findings filed against `game/`
there refer to this code — read them knowing they were written while the two
repositories were one.
