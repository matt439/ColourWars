# ColourWars

A local split-screen paint-shooter for up to four players. Two teams, one
arena, and a scoreboard that counts painted surface rather than kills.

Play with XInput gamepads, or with one keyboard-and-mouse player alongside
them. Up to four players can play at once. The device that selects the player
count becomes Player 1; additional players use the remaining connected pads
in slot order, followed by the keyboard and mouse if they are not Player 1.
That roster stays fixed through team, weapon and stage selection and the match.
If a controller disconnects during setup, reconnect it or press B/Escape to
return to player count and choose a new roster.

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

There is no `ctest` step here and that is not an oversight: the tests live with
the engine. Labrador puts its tests, its benchmark and its sample behind
`PROJECT_IS_TOP_LEVEL`, so consuming it as a submodule brings in the library
and nothing else.

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
