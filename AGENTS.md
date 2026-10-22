# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

## What this is

ColourWars is a local split-screen paint-shooter (up to four players, two teams,
score is painted surface) and the first client of the **Labrador** 2D game
engine, consumed as a submodule at `external/labrador`. This repository is an
ordinary C++ application that links the engine; the engine is the larger
project. Windows-only, inherited from the engine's XAudio2 backend.

The engine now has **five render backends** — `d3d11`, `d3d12`, `gl`, `vulkan`
and `null`, chosen at configure time by `LABRADOR_RENDER_BACKEND`, and **two
audio backends** — `xaudio2` and `null`, chosen by `LABRADOR_AUDIO_BACKEND`.
This repository sets neither and gets the `d3d11` and `xaudio2` defaults, which
are the two it has always had. The rest are the engine's business: `gl` and the
two newer Direct3D and Vulkan paths are the implementations that prove the
render seam, and each `null` records what it was asked to do so a headless test
can read it back. There is nothing to do here unless this game grows a reason
to run without a GPU or without a speaker.

The project was ColourWars, then ChromaClash, then ArtAttack, and has taken its
first name back. The identifiers have now followed: the engine swept its own
side in `6b5c0bd` (`artattack::` → `labrador::`, `artattack_settings` →
`labrador_settings`), and this repository did the same in the matching commit,
renaming the executable to `ColourWars.exe`, its Win32 class to
`ColourWarsWindowClass` and the content target to `colourwars_content`. Nothing
in either tree is named for the paint-shooter's middle name any more.

Two things deliberately still spell it. `mattmath::` is the author's name, not
the game's. And the historical documents — `docs/review/` here and in the
engine — are the reviews as written, so rewriting them would make a record stop
matching what was actually reviewed.

## Open work

[TODO.md](TODO.md) is the index of open work: **NOW** gives the order, and each
row points to the doc that owns its detail. A fresh thread starts there. Its
editing rules are at its end; the main one is that a row leaves in the same
commit that finishes it.

## Build and run

Requires **VCPKG_ROOT** set (`CMakePresets.json` reads it for the toolchain
file; configure fails without it), VS 2022+ with the C++ workload, and the
Windows SDK. The presets use the Ninja generator, so from a plain shell every
`cmake` step needs the MSVC environment (`vcvars64.bat`) — a Developer prompt or
the VS-integrated CMake handles this for you.

```
git clone --recurse-submodules https://github.com/matt439/ColourWars.git
# or, on an existing clone: git submodule update --init --recursive

cmake --preset x64-debug            # or x64-release
cmake --build --preset x64-debug
out/build/x64-debug/game/ColourWars.exe
```

### Developer console (Debug only)

Debug builds take launch arguments and have a console and a debug menu; Release
compiles out the console, menu, registry, commands and binds (the unreachable
`DebugMatchTarget`/`DebugPageTarget` interfaces, their adapters and a few plain
`Level`/`Player` members remain, so no layout depends on `_DEBUG`). Full reference: [docs/debug-console.md](docs/debug-console.md).

```
out/build/x64-debug/game/ColourWars.exe +stage movement_course +players 4
```

Each `+name` starts a statement; the match starts without menu navigation. In a
match **`** opens the console (the match freezes while it is open; Escape
closes), **F2**-**F4** collision, camera and paint overlays, **F5** restart,
**F6** pause, **F7** step, **F9** toggle
movement feedback. Anywhere, **F10**/**F11** or holding pad **Back+Start** opens
the debug menu. `help` lists the commands. The console also tunes
`tuning.json`/`presentation.json` live (`tune`; `tune.save` writes only the
edited tokens back to `game/content/`), puts scripted bots on idle seats
(`+stage movement_course +players 4 +bot all paint`), and records and replays
seeded matches bit for bit (`seed`, `record`, `play`). The F-keys are default
binds (`bind`/`binds`, kept in `%LOCALAPPDATA%\ColourWars\debug\binds.cfg`), and
`console.live 1` keeps the match running under the console. A repeatable review
is one command - `ColourWars.exe +exec review_feedback_ab +quit` - with the
scripts in `scripts/debug/`. The gate
`COLOURWARS_DEV_TOOLS` comes from `_DEBUG` in `game/debug/dev_tools.h` - never a
`/D` flag, so the acceptance probes agree on class layouts - and no existing
class has a member that depends on it. Code is in `game/debug/`; the CPU case is
`scripts/acceptance/debug_console.cpp` (its checks are in the same-named
folder; `run-acceptance.ps1` compiles a case's folder with it), which also
checks that the Release executable carries none of the console's RTTI names.

The build mirrors the reviewed runtime assets from `game/content/` beside the exe
([cmake/runtime_assets.cmake](cmake/runtime_assets.cmake), the `colourwars_content`
target, which runs every build so asset-only edits are picked up), so the game
runs from anywhere. It resolves content beside the executable, not against the
working directory — the engine's rule (`engine/app/content_root.h`), which
`Application::load_manifest` applies to the manifest and
[game/main.cpp](game/main.cpp) applies by hand to the two files it reads before
the manifest. See [game/directory_consts.h](game/directory_consts.h).

## Tests

**There are no CTest targets in this repository, by design.** The engine test
suite lives with the engine, and Labrador puts its tests, benchmark and sample
behind `PROJECT_IS_TOP_LEVEL` — so consuming it as a submodule brings in the
library and nothing else. A `ctest` run from here is empty, not broken.

Consumer acceptance checks live under `scripts/acceptance/`. From an x64 VS
Developer shell, `scripts/run-acceptance.ps1 -Preset x64-debug` (or
`x64-release`) builds the game and checks its actual objects for ammunition,
paint coverage and save regressions. They do not create a graphics/audio device
or touch the player's save. `scripts/check-startup.ps1` separately checks a
hidden game process, its per-monitor DPI awareness, isolated save, diagnostics
and graceful shutdown.

To run engine tests, configure the submodule as its own top-level project:

```
cmake --preset x64-debug -S external/labrador -B out/labrador/x64-debug
cmake --build out/labrador/x64-debug
ctest --test-dir out/labrador/x64-debug --output-on-failure
ctest --test-dir out/labrador/x64-debug -R <name>    # a single test
```

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) builds both presets,
runs consumer acceptance and checks startup. Debug startup is required; Release
may skip only the exact missing-hardware-adapter diagnostic because its engine
build has no software fallback. Content failures, crashes and other startup
errors fail CI. Release also builds and inspects the redistributable ZIP.

## Audio: accepted replacement bank and optional fallback

A fresh clone includes `game/content/sounds/paint_exe.xwb` and its definition.
The build validates the tracked bank before staging it; normal game builds do
not need Python or FFmpeg. All 103 accepted masters, retained CC0 recordings,
editable scores/recipes and per-wave provenance live under
[game/audio/paint_exe/](game/audio/paint_exe/README.md), which documents bank
reproduction with `scripts/audio/tools/XWBTool.exe`.

The existing game bindings use the replacement set; remaining new cue hooks and
gameplay mix are `POL#23`, and actual listening/hardware acceptance is `ROAD#3`.
The normal Release ZIP includes the validated bank, provenance and CC0 terms.
Private legacy audio must never be staged or packaged.

Absent the bank, `manifest.json` still marks it `"optional": true`: the loader
substitutes a silent bank and reports it once on stderr. Keep that fallback
working and check it separately from the normal release. See [NOTICE](NOTICE).

## The engine submodule

`external/labrador` pins an exact revision — a fact about this repository, not a
floating reference. `git submodule status` is the authoritative answer to which
revision; deliberately no SHA is written into docs, since it goes stale the first
time the pin moves. To move it, checkout the new revision inside the submodule,
**build before committing**, then `git add external/labrador`.

Engine changes belong in the Labrador repository, not here. `labrador_settings`
— the `/W4 /WX` interface target every real target compiles against — is defined
by the engine and deliberately *not* copied here; two definitions across two
repositories would drift.

What this client owes the engine instead is a **report**, and there is a ledger
of them at [docs/review/engine-reports.md](docs/review/engine-reports.md): each
one a case where the API forced a bad answer, carrying as its exhibit the
workaround that is in this repository because of it. They are drafted here
rather than in the submodule because a report is a thing the client owes, and
this is the client. Reports are struck as the engine answers them, and a pin
bump is when the answers become visible — so **after moving the pin, read that
file**: it records what each closure lets this repository delete.

The engine's own review documents live at `external/labrador/docs/review/` and
are historical, not current.

## Architecture

### Service bag, not globals

[game/game_data.h](game/game_data.h) — `GameData` is handed to every state,
object and drawable at construction. It holds exactly seven borrowed pointers: the
engine `Application`, this game's `Save`, the level registry, the stage list, the
tuning, the presentation and the developer `DebugDirector` (inert in Release, and
`DebugDirector::inert()` when unset). Everything else that looks like a service
(`ThreadPool`, `Gamepads`, `RenderResources`, …) is a forward to the shell. All
seven are owned by `wWinMain` in [game/main.cpp](game/main.cpp) and declared above
the `Application`, so they outlive every state that borrows them.

### State stack

[game/states/game_states.h](game/states/game_states.h) — two game states over the
engine's state stack:

```
[GameMenu]                                  bottom for the whole session; owns
[GameMenu, MainMenu*Page]                   what the main-menu pages share
[GameMenu, GameLevel]
[GameMenu, GameLevel, PauseMenu*Page | ResultsMenu | EndMenu]
```

A match is pushed *above* the menu, never in place of it, so returning from a
match returns to a screen that still exists. `GameLevel` restores the screen
layout it took in its destructor. Menu pages derive from `MenuPage` and share a
`MenuContext` (services + pad reader + the `MenuLevelSettings` being assembled),
owned by whichever state owns the screen.

### Level vs Scene

[game/objects/level.h](game/objects/level.h) — `Level` is one paint match: the
countdown, the clock, the HUDs, the cameras, the music. The generic parts —
object lists, the tick, the collision sweep, out-of-bounds retirement, per-view
render fan-out — are `labrador::Scene` in the engine. `Level` holds non-owning
`Player*` back into the scene that owns them; the index into `players_` is the
index of that player's view, **not** the player's pad slot (`player_num()` is,
and the two diverge when a pad is unplugged).

Construction goes through builders — `LevelBuilder` → `PlayerBuilder`,
`LevelObjectBuilder`, `WeaponBuilder`, `ProjectileBuilder` — which read the level
definition and the tuning and hand finished objects to the scene.

### Everything numeric is data

Four JSON files are read at startup, before anything else:

| File | What |
|---|---|
| `manifest.json` | which fonts, sprite sheets, levels and sound banks exist |
| `tuning.json` | every number a match is *balanced* on ([game/tuning.h](game/tuning.h)) |
| `presentation.json` | every font, colour, offset and box size the shell is *laid out* from ([game/presentation.h](game/presentation.h), with its menu-page layout records in [game/presentation_menu.h](game/presentation_menu.h) and its menu text in [game/presentation_menu_text.h](game/presentation_menu_text.h)) |
| `%LOCALAPPDATA%\ColourWars\save_data.json` | display and play preferences, read before the window exists, and per-stage records; versioned `SaveSection`s ([game/save_section.h](game/save_section.h)) |

Tuning and presentation are deliberately two files with two audiences (a balance
pass and an art pass) and are deliberately *not* manifest entries — they are read
before every asset and are what those assets are used *by*.

The save is in that list because of *when* it is read, and is otherwise the odd
one out: the first three are content, staged beside the exe by the build and only
ever read; the save is written by the player and created by nothing but the game.
It used to sit with them at `./save/save_data.json` inside the staged mirror,
where a clean rebuild deleted it and two Windows accounts shared one file. That
path is still read once, if nothing is at the real one, to carry an older save
across — see [game/directory_consts.h](game/directory_consts.h). Nothing seeds a
save; `Save` writes its own defaults on first run.

The rule these follow: **C++ says *how*; data says *which* and *how much*.** A
jump's shape is code, its three numbers are tuning. Anything whose value would
change the code — animation states the code switches on, member defaults a
builder overwrites — stays in C++. Moving a number into JSON so it can be a
scripting layer is the failure mode to avoid.

`main.cpp` teaches the engine's resource loader what a `"level"` kind is
(`register_kind`), because a level definition means nothing to the engine. Levels
are never optional; a stage the manifest names and the disk lacks is a content
bug that must fail at startup, not three screens later.

## Conventions

Includes are written **from the repository root** — `#include "game/pch.h"`,
`#include "engine/render/camera.h"` — resolved by the one include directory
[game/CMakeLists.txt](game/CMakeLists.txt) adds. Engine files reaching for
`"game/..."` is caught by the engine's `check_engine_includes` target, which from
this tree is the only thing enforcing that direction.

Naming (from the engine's `docs/design/CONVENTIONS.md`, which the game follows):
`PascalCase` types, `snake_case` everything else, `snake_case_` private members,
bare `snake_case` public struct fields, `snake_case` enumerators
(`WeaponType::sprayer`), `SCREAMING_SNAKE` for macros only. Accessors are the
bare noun (`bounds()`, not `get_bounds()`); setters take `set_`.

The game is a *client* and is held to a client's standard, not the engine's.
Labrador's `PHILOSOPHY.md` governs engine code; game code meets the engine at
small interfaces and chooses its own grammar behind them. It still compiles under
`/W4 /WX`, and `game/pch.h` is the precompiled header for every translation unit.

Two rounds of full source review are recorded in the engine repository under
`external/labrador/docs/review/`; findings filed against `game/` there refer to
this code, written while the two repositories were one.

## The game is deliberately OOP-heavy — do not "fix" it

This code is full inheritance and virtual dispatch, several levels deep and
multiple where it suits: `ProjectileMist → DiffusingProjectile → Projectile →
MovingObject + CollisionObject`, `WeaponBucket → RelativeVelocityWeapon →
Weapon`, `MainMenuStageSelect → MainMenuPage → MenuPage + SoundBankObject`,
`StructurePaintable → Structure + PaintableObject`. **That is the design, not
drift.**

The engine says the opposite about itself and says so loudly, which is exactly
why this is worth stating. Labrador's **T11** makes inheritance and virtual
dispatch "tools of last resort, not the default modelling grammar" — and then
scopes itself, in its own words: T11 is *"not a licence for ... imposing this
style on the engine's users"*; it governs "the engine's own code and everything
it ships", while "game code meets the engine at small interfaces and chooses its
own grammar behind them — full OOP or full performance."

`PHILOSOPHY.md`, "The object model", is explicit that both ends are
first-class: a game may be "written in full OOP — one registered object per
entity, hierarchies as deep as its author likes — or for full performance",
and interface granularity is "the user's performance dial". It names this game
directly: *"The paint-shooter is a client rather than shipped code and is bound
by none of it."*

**The rule that follows, and the one that matters here:** a finding that holds
this code to T11 — "replace the hierarchy with values", "this should be a
variant", "too much virtual dispatch" — is misfiled. Per PHILOSOPHY, the honest
answer to it is "the engine's API made that awkward, fix the API" *in Labrador*,
or nothing at all. Read the `game/` findings in
`external/labrador/docs/review/` with that in mind; they were written while the
two repositories were one. Refactoring this game toward value-first C++ to match
the engine is not a cleanup, it is deleting the demonstration.

## What this game is for

It is the engine's **first and real client** — the proving ground. The README
puts it plainly: an engine with no client is a library of guesses, and this is
what keeps saying "this API is awkward" loudly enough to be worth fixing. Its
OOP-heavy shape is a load-bearing part of that: it exercises the full-OOP end of
the dial that the engine's own code never will.

It is **not** the copy-me template. That role belongs to the engine's
`samples/`, which `ARCHITECTURE.md` calls "the permanent second client that
keeps the boundary honest, and the template a new project copies" — and, unlike
this game, samples are shipped code and therefore *are* bound by T11. When
demonstrating engine usage, know which of the two you are pointing at.

## History

Two disjoint histories, deliberately:

- **`master`** runs back to 2023-12-19 — the ArtAttack repository's history
  filtered to the game's side of the split. Most of it is under `ArtAttack/`
  rather than `game/`; use `git log --follow` to cross the rename.
- **`original-2022`** is the first 119 commits (2022-12-30 to 2023-10-10),
  unmodified, sharing no ancestor with `master`.
