# ColourWars

A local split-screen paint-shooter for up to four players. Two teams, one
arena, and a scoreboard that counts painted surface rather than kills.

Choose **Single Match**, **Best of 3** (first to two wins), or **Best of 5**
(first to three). Series keep the same players, teams, weapons and stage;
draws award no win and are replayed. Pause > Restart restarts only the current
round. After the series, Play Series Again clears the wins. Quitting discards
the session. For solo practice, choose Single Match and 1 Player.

Results show cumulative series wins and each player's **surface held** as a
percentage of the arena. Capturing neutral or enemy paint earns its area;
enemy repaint transfers credit, while friendly repaint keeps the original
owner. This measures paint remaining at the finish, rather than total shots
or paint laid during the round. Team bars retain their share of painted area.

Play with XInput gamepads, or with one keyboard-and-mouse player alongside
them. Up to four players can play at once. The device that selects the player
count becomes Player 1; additional players claim their devices on the Join
Players screen with A (Enter for keyboard and mouse).
Each device claims one seat, in join order. B/Escape releases your seat;
Player 1 uses it to return to player count. Once everyone has joined,
Player 1 presses A/Enter to continue. Player and device labels appear during
setup and in each gameplay view.

That roster stays fixed through team, weapon and stage selection and the match.
If a controller disconnects during setup, reconnect it or press B/Escape to
return to player count and choose a new roster. During play, a participating
controller disconnect pauses the match and audio and identifies the missing
players. Resume and Restart stay disabled until every participant reconnects;
reconnecting alone never resumes play. Quit remains available, including from
the keyboard if all participating controllers are gone.

Controller identity follows the XInput slot shown as Pad 1-4. Reconnect to the
same slot to keep playing; if Windows assigns a different slot, quit and claim
devices again. Spare controllers do not pause a match or take over an occupied
player's seat.

Options > Team palette keeps Random as the default, or fixes Team A/B to
blue/orange, pink/green or blue/yellow. Use Left/Right to choose, then Apply
to save it between launches. Fixed palettes also stay fixed on a rematch.
Each gameplay HUD names Team A or Team B on its dark identity panel; results
label the teams with A/B letters beside the bars and percentages.

Options and Pause both offer **Controls / audio**, with a controls reference,
five levels of controller stick sensitivity, and separate music and SFX volume.
Higher sensitivity needs less aim-stick travel; level 3 preserves the original
deadzone. Mouse aim follows the cursor and the roller keeps its fixed aim.
Use Left/Right to adjust, then Apply to save for all players and future launches.
Back discards unapplied changes. From Pause, Back returns to the pause menu;
Resume uses the new settings in the current match. Volume is 0-100% of the
authored mix, including menu sounds, and zero mutes that category.

| Action | Gamepad | Keyboard and mouse |
|---|---|---|
| Move | Left stick | A/D |
| Jump | A or left trigger | Space |
| Aim | Right stick | Mouse cursor |
| Shoot | Right trigger | Left mouse button |
| Drop through a platform | Left stick down | S or Down |
| Pause/resume | Start | Escape |
| Debug overlay | Back/View | F1 |
| Menu navigation | Left stick or D-pad | Arrow keys or WASD |
| Menu confirm | A | Enter or Space |
| Menu back | B | Escape or Backspace |

It is also the first client of [Labrador](https://github.com/matt439/Labrador),
a 2D game engine, which this repository consumes as a submodule. The engine is
the larger project and this is the thing that proves it works — an engine with
no client is a library of guesses, and ColourWars is what keeps saying "this
API is awkward" loudly enough to be worth fixing.

```
game/                 ~12.8k lines   the game: states, objects, content
external/labrador/                   the engine, as a submodule
external/rapidjson/                  vendored, for the save and level files
cmake/                               the wave-bank recipe
```

## Building

**Windows only**, inherited from the engine: the render backend is Direct3D 11
and the audio backend is XAudio2.

### Clone

The engine is a submodule, so a plain `git clone` gets you an empty
`external/labrador` and a CMake error on the first configure.

```
git clone --recurse-submodules https://github.com/matt439/ColourWars.git
```

If you already cloned without it: `git submodule update --init --recursive`.

Labrador is public, so the submodule needs no credentials of its own — this
repository is the private half.

### Prerequisites

- **Visual Studio 2022 or newer**, with the "Desktop development with C++"
  workload. Ninja and CMake come with it.
- **The Windows 10/11 SDK**, for `d3d11.h` and friends.
- **vcpkg**, with `VCPKG_ROOT` set. `CMakePresets.json` reads it for the
  toolchain file, and configuring fails without it. Visual Studio ships a copy
  at `<VS install>\VC\vcpkg` if you have no other.

DirectX Tool Kit is in `vcpkg.json` and is fetched at configure time.

### Configure and build

```
cmake --preset x64-debug          # or x64-release
cmake --build --preset x64-debug
```

The game runs from its own output directory, which the build fills with
`game/content/` — so `out/build/x64-debug/game/ColourWars.exe` runs from
anywhere.

There is no `ctest` step here: the engine's test suite lives with
the engine. Labrador puts its tests, its benchmark and its sample behind
`PROJECT_IS_TOP_LEVEL`, so consuming it as a submodule brings in the library
and nothing else.

### Consumer acceptance

From an x64 Visual Studio Developer shell with `VCPKG_ROOT` set:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/run-acceptance.ps1 -Preset x64-release
```

These commands configure/build the game, then compile small native checks
against its actual objects. They cover full-cost ammunition and held-fire
recovery, exposed paint surfaces and ownership, and settings persistence and
write-failure recovery. They use no graphics device or speaker and isolate
all save writes. Evidence stays under `out/acceptance/<preset>/`. CI runs the
same checks; `-SkipBuild` is for an already-current build.

Check runtime startup separately, preferably against the extracted Release ZIP:

```powershell
./scripts/check-startup.ps1 -Executable C:/Games/ColourWars/ColourWars.exe -RequireNoAudio -SystemOnlyPath
```

It starts hidden from an unrelated directory with an isolated save, checks the
game window and diagnostics, then closes gracefully. CI requires Debug startup
(the engine supports software rendering there). Release startup may be explicitly
skipped only for the engine's exact missing-hardware-adapter error; all application
failures fail CI. Local checks default to requiring startup. These automated
checks do not establish controller feel, audible balance or multiplayer quality;
the hardware acceptance checklist is in [the roadmap](docs/feature-roadmap.md).

### Level editor

The normal build also creates a separate desktop level editor:

```powershell
./out/build/x64-debug/editor/ColourWarsEditor.exe
```

Open a level from `game/content/levels/`, select and drag its geometry, edit
properties, add platforms/ramps/visuals, and save. The editor includes grid
snapping, zoom/pan, spawn and bounds editing, and undo/redo. Rebuild the game
after saving to copy the source level into its runtime content directory.

See [the editor guide](docs/level-editor.md) for controls, registering a new
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
`out/build/x64-release/packages/ColourWars-windows-x64.zip`. Extract the entire
folder and run `ColourWars.exe` on Windows 10/11 x64 with a Direct3D 11-capable
GPU. The ZIP includes DirectXTK, XAudio2Redist and the release MSVC runtime DLLs
beside the executable; Visual Studio and vcpkg are not needed to play. Windows
provides Direct3D, XInput and the Universal CRT.

The title screen displays the game revision and build configuration.
`build-info.txt` also records the full game and engine revisions and selected
backends. The identifier refreshes on every build, with `-dirty` for local
changes and `source-archive` when Git metadata is unavailable. Rebuild after
committing to produce a package labelled with that commit.

Only the assets listed in [cmake/release_assets.cmake](cmake/release_assets.cmake)
are packaged; configure checks that the list matches the runtime manifest.
The ZIP includes licence notices, the seven compiled fonts, the sprite sheet,
all three levels, tuning, presentation and the optional sound-bank definition.
It is always silent: local wave banks and source audio are excluded even on a
developer's machine. Development art, content tools and saves are excluded too.
Do not distribute the normal `game/` build directory in place of this ZIP.
Debug packaging is rejected because its runtime is not redistributable.

To check a release, extract the ZIP outside the checkout, launch from a different
working directory, confirm the title's build label, and start a solo match.
The expected stderr message says the optional wave bank is absent. Settings
still live in `%LOCALAPPDATA%\ColourWars\save_data.json`, independently of where
the ZIP was extracted.

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

Then read [docs/review/engine-reports.md](docs/review/engine-reports.md). It is
the ledger of reports this client has filed against the engine, and a pin bump
is the moment the answered ones become visible here — each closed report records
what it lets this repository delete.

## Audio

**A fresh clone has no sound, and that is expected.** The game runs; it is
silent, and it says so once on stderr.

The wave bank is built from source `.wav` files that are not in this
repository. Two reasons: four of the twenty-four waves are commercial music
tracks nobody can redistribute, and the sources are 148 MB against a built bank
of 154 MB, past GitHub's per-file limit. See [NOTICE](NOTICE).

What *is* here is the recipe:

- `game/content/sounds/sound_bank_1.json` — the bank's definition, and the
  authoritative list of the waves it expects.
- `game/content/sounds/sound_bank_1/XWBTool.exe` — Microsoft's wave-bank
  compiler.
- `cmake/build_wave_bank.cmake` — run from the build, it compiles the bank
  whenever every source wave named by the definition is present, and does
  nothing when they are not.

To get audio, put a `.wav` for each name in `sound_bank_1.json`'s `waves` array
into `game/content/sounds/sound_bank_1/` and build. The bank is compiled for
you.

Absent the bank, `game/content/manifest.json` marks it `"optional": true`, the
loader substitutes a silent sound bank, and every play is a no-op. Nothing else
in the game changes.

## About the code

ColourWars is a working example of object-oriented game code: abstract
interfaces, deep and multiple inheritance, virtual behavior, composition and
explicit ownership. The [OOP reading guide](docs/oop-guide.md) maps those
concepts to the implementation, follows a shot from input to score, and explains
how to extend the game. It is Labrador's real client; the engine's `samples/`
provide its starter template.

The project began here on 2022-12-30 as ColourWars, became ChromaClash, then
ArtAttack, and the game has now taken its first name back — in the same
repository it started in. The identifiers followed the repository rather than
leading it: the executable was `ArtAttackGame.exe` and the engine namespace
`artattack::` for six days after the split, and both were swept afterwards, on
each side of it.

There are two disjoint histories in it, and that is deliberate:

- **`master`** runs back to 2023-12-19. That is the ArtAttack repository's
  history, filtered to the game's side of the split. Most of it is under
  `ArtAttack/` rather than `game/`, because the rename to `game/` came late;
  `git log --follow` crosses it.
- **`original-2022`** is the first 119 commits, 2022-12-30 to 2023-10-10,
  exactly as they were. They share no ancestor with `master`: the ArtAttack
  repository was started fresh and the old sources were copied in by hand
  ("starting the process of adding all of my old source files back"), so
  there is no commit to graft onto. The branch is the honest way to keep
  them — reachable, unmodified, and not pretending to be an ancestor.

`matt439/ChromaClash` holds the middle name's history.

The game is a *client*, and it is held to a client's standard rather than the
engine's. Labrador's PHILOSOPHY governs the engine's own code and everything it
ships; game code meets the engine at small interfaces and chooses its own
grammar behind them. Two rounds of full source review are recorded in the
engine repository under `docs/review/`, and the findings filed against `game/`
there refer to this code — read them knowing they were written while the two
repositories were one.

## Licence

MIT — see [LICENSE](LICENSE). [NOTICE](NOTICE) records the third-party code
carried here and the content that is deliberately not distributed. The engine
has its own LICENSE and NOTICE in `external/labrador`.
