# ColourWars

A local split-screen paint-shooter for up to four players. Two teams, one
arena, and a scoreboard that counts painted surface rather than kills.

## Play

You don't need Git, Visual Studio or a terminal to play.

1. Get `ColourWars-windows-x64.zip`. No release has been published yet, so the
   [releases page](https://github.com/matt439/ColourWars/releases) is empty;
   once it lists one, download the ZIP from there. Until then, a signed-in
   GitHub account can take the `ColourWars-windows-x64` artifact from a passing
   [pull-request CI run](https://github.com/matt439/ColourWars/actions/workflows/ci.yml?query=event%3Apull_request+is%3Asuccess):
   open a run and look under **Artifacts** at the bottom of its summary (the
   download comes wrapped in a second ZIP). Only a run that built the game has
   one, so a pull request that changed only documents does not, and GitHub
   deletes it 14 days after the run. If no listed run has it, use `build.cmd`
   (see [Building](#building)).
2. Right-click the ZIP and choose **Extract All**. Running the game from
   inside the ZIP doesn't work.
3. Open the extracted folder, then the `ColourWars-windows-x64` folder inside
   it, and run `ColourWars.exe`. If Windows says
   "Windows protected your PC", choose **More info**, then **Run anyway**: the
   game isn't signed with a publisher certificate.

It needs Windows 10 or 11 (64-bit) and a Direct3D 11 graphics card, which nearly
every PC from the last ten years has. Nothing needs installing. Play with
Xbox-style controllers, or one player on keyboard and mouse. On your own,
practise solo or fill the empty seats with bots: on Join Players, Player 1
picks a seat with Up/Down and makes it a bot with Left/Right.
`HOW-TO-PLAY.txt` in the ZIP has the controls, where settings are saved and how
to remove the game. Building from source is under [Building](#building).

The arena is a picture left unsaved in a paint program: everything in it is
grey, and the palette has one empty slot. Each match opens full screen on the
monitor of the person at the computer, who previews one team's colour over
each half of the picture, undoes both and walks away; then the camera pushes
through the glass and split screen begins. When the clock runs out it pulls
back to the monitor, the person returns, hovers each colour over the whole
picture and clicks the winner, and the coverage percentages appear. If nobody
painted, the person closes the program instead. A, B or Start (Space, Escape
or Enter) skips the opening, a restart or rematch starts at the preview, and
solo practice skips the person at both ends. See [the backstory](docs/backstory.md).

The presentation uses dark ink panels, cream lettering and bright paint
accents, with original menu artwork and a compact split-screen HUD. Health
and paint meters are labelled; menus mark the focused choice with both a
colour and a panel. The arenas are built from drawn stationery (pencils,
rulers, erasers, a paint tin, set squares) on tiled ground against a crude drawn
backdrop, in a muted palette with no team hue in it ([terrain kit](docs/polish-terrain.md)). Colours and spacing are authored in
`game/content/presentation.json`; arena decoration lives in the level JSONs.
The shared menu layout, fill atlas and team-selection pictures are described in
[menu presentation](docs/menu-presentation.md).
Stage selection previews the actual arena objects alongside the match setup,
with a separate confirmation before starting. The opening countdown shows
compact controls cards for the keyboard/mouse and gamepads present in the match.

Choose **Single Match**, **Best of 3** (first to two wins), or **Best of 5**
(first to three). Series keep the same players, teams and weapons; by default
each round moves on to the next Random arena (Mode's series stage rule also
offers Same stage and Loser's pick), and draws award no win and replay their
stage. Pause > Restart restarts only the current
round. After the series, Play Series Again clears the wins. Quitting discards
the session. For solo practice, choose Single Match and 1 Player. The experimental
[Movement Course](docs/polish-movement-course.md) provides repeatable jump, landing
and platform-drop trials; select it explicitly, as Random uses four arenas:
Close Quarters, Crossfeed, Press Tower and Turbulence Yard - Slab Trial. The
original King of the Hill and Turbulence, and the static, mover, gate, ferry,
lift and paint-lift Turbulence Yard comparisons, are explicit-only.
The chosen art direction is Paint.exe Maximalism, crude mouse-drawn box-heads.
Its five round-2 rules are accepted in [the polish plan](docs/polish-plan.md#1-establish-the-art-and-display-rules);
the superseded workshop comparison was retired on 2026-10-15. Open work is in
[TODO.md](TODO.md).

Caught startup and runtime failures show the original cause in an error dialog
and stderr, then exit with code 1. The latest failure also writes diagnostic
metadata to `%LOCALAPPDATA%\ColourWars\last-error.log`; arbitrary error text and
player data stay out of that file. See [failure reporting](docs/failure-reporting.md)
for the recorded fields, checks and exception boundary.

Results show cumulative series wins and each player's **surface held** as a
percentage of the arena. Capturing neutral or enemy paint earns its area;
enemy repaint transfers credit, while friendly repaint keeps the original
owner. This measures paint remaining at the finish, rather than total shots
or paint laid during the round. After the reveal the page keeps only the
percentages; solo practice, which has no reveal, still fills the team bars.

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
same slot to keep playing. If Windows gives the pad a different slot, press
Start on it while the pause names the missing player: it takes back the first
missing seat, keeping that seat's settings. Spare controllers do not pause a
match or take over an occupied player's seat.

Options > Team palette keeps Random as the default, or fixes Team A/B to
blue/orange, pink/green, blue/yellow, purple/gold, red/azure or
magenta/forest. The last three are checked under protan, deutan and tritan
colour-vision simulation. Use Left/Right to choose, then Apply
to save it between launches. Fixed palettes also stay fixed on a rematch.
Each gameplay HUD names Team A or Team B on its dark identity panel; results
label the teams with A/B letters beside the percentages (and the bars, when
shown).

Options and Pause both offer **Controls / audio**, with a controls reference,
per-pad movement and aim deadzones, separate music and SFX volume,
and On/Off **View shake** and **Aim marker** settings (both On by default).
Open **Pad sticks** to select Pad 1-4, adjust each deadzone from 0-90%, and
watch raw stick input and its movement/aim response before applying it.
Its **Rumble** row switches feedback On or Off for that physical pad slot
(default On); Apply saves the choice. Pausing, losing focus or disabling the
slot stops feedback immediately. Physical-pad feel remains under review.
Use D-pad/arrows on that page so the sticks stay free for testing.
These settings follow the pad slot, independently of player numbers, unless the
pad wears a **profile**. Old saves keep their existing deadzones until edited.
Mouse aim follows the cursor and the roller keeps its fixed aim.
Open **Pad Buttons** to choose Jump, Shoot, Pause and Sub for each Pad 1-4 slot
from A/B/X/Y, LB/RB, LT/RT, a stick press (LS/RS) or Start; all but Sub may take
an optional second control. The defaults are A / LT Jump, RT Shoot, Start Pause
and RB Sub, and a control in use on that pad is skipped. **Keyboard Keys** changes
the shared keyboard player's Move left, Move right, Jump, Drop, Shoot and Sub
(each of those two a key or a mouse button) and an extra Pause key: select a row
and press the new key. Esc always pauses, Down always
drops and a key in use is refused. These choices leave menu and intro controls
unchanged.
**Profiles** keeps up to eight named profiles, each carrying a person's
deadzones, rumble, comfort switches, pad buttons and Boost. Choose which pad
wears one there, or press X/R on Join to cycle a joined pad through them; the
pad's settings pages then edit the profile. Wearing lasts until the game closes,
and a pad that reconnects keeps its profile. Profile changes save at once.
Use Left/Right to adjust; Apply saves the current page's changes for future launches.
Back discards unapplied changes. From Pause, Back returns to the pause menu;
Resume uses the new settings in the current match. Volume is 0-100% of the
authored mix, including menu sounds, and zero mutes that category.
View shake adds a small local world-camera accent on hurt, elimination and heavy
landings. The HUD and aim stay steady; Off applies to every player.
The aim marker shows a ready chevron, cooldown bars or a no-paint cross just
beyond the nozzle in your own pane. It shows direction, including the bucket's
initial throw direction; it does not predict where paint will hit. The fixed-aim
roller has no marker. Shapes remain distinct with sound muted or without colour.

**Credits** on the home menu acknowledges the engine, third-party code, content
tools, fonts, artwork and accepted replacement audio sources. Full notices and
licence terms are included as `NOTICE`, `LICENSE` and `licenses/` in the release.

The following controls are the defaults:

| Action | Gamepad | Keyboard and mouse |
|---|---|---|
| Move | Left stick | A/D |
| Jump | A or left trigger | Space |
| Aim | Right stick | Mouse cursor |
| Shoot | Right trigger | Left mouse button |
| Sub | Right bumper (RB) | Right mouse button |
| Drop through a platform | Left stick down | S or Down |
| Pause/resume | Start | Escape |
| Debug overlay | Back/View | F1 |
| Menu navigation | Left stick or D-pad | Arrow keys or WASD |
| Menu confirm | A | Enter or Space |
| Menu back | B | Escape or Backspace |

Each player carries a **sub weapon** beside their tool for the whole match. On
Weapon Select a row picks its tool, then its sub: A/Enter confirms the tool,
Left/Right chooses the Grenade, Sprinkler, Paint wall or Random, and A/Enter
again readies the row. The grenade is lobbed along the aim and bursts into a
ring of paint that hurts enemies caught in it. The sprinkler sticks where it
lands and sprays paint around it until its time runs out or enemy shots break
it. The paint wall stands beside the player, on the side they aim, stops enemy
shots and drips paint on the floor beneath it. A sub costs half the paint tank
and fires only with that much in it; under Endless paint a cooldown paces it
instead. The square after the HUD's PAINT label is solid when a Sub press would
release it.

## Your own stages

To play a stage of your own, put its level file in
`%LOCALAPPDATA%\ColourWars\stages\` (paste that into Explorer's address bar;
create the `stages` folder if it is not there). The file name is the stage's
name: up to 64 plain letters (A to Z, no accents), digits, `_` or `-`, not
starting with `-`, ending in `.json`, not a reserved Windows name (`CON`,
`PRN`, `AUX`, `NUL`, `COM0`-`COM9`, `LPT0`-`LPT9`), and not the name of a stage
the game already has in any mix of capitals. Your stages come after the built-in ones
in Stage Select, marked "Built by you." Random picks one only when its file
says `"include_in_random": true`.

The game reads the folder when it starts and again each time Stage Select opens
from the main menu, so a new or changed file shows up without restarting. It
is not reread between the rounds of a series. A file the game cannot use is
left out, and the other stages still load. Stage Select shows a scan issue
count; press **Down** while browsing to read the filename and reason.
**Left/Right** changes issue, **Up/Down** changes page, and **A/Enter** or
**B/Esc** returns to your arena choice. The game also prints the reason to
stderr, which you can retain with `ColourWars.exe 2> stages.txt`.

The level editor makes these files: **File > Save to My Stages** (Ctrl+M)
saves into that folder, checking the name as the game does. The editor is
built from source and does not come in the ZIP; see
[the editor guide](docs/level-editor.md) and
[the level format](docs/level-format.md).

It is also the first client of [Labrador](https://github.com/matt439/Labrador),
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

**Windows only**: the game starts from `wWinMain` and builds the engine's
default backends, Direct3D 11 for rendering and XAudio2 for audio.

### Clone

The engine is a submodule, so a plain `git clone` gets you an empty
`external/labrador` and a CMake error on the first configure.

```
git clone --recurse-submodules https://github.com/matt439/ColourWars.git
```

If you already cloned without it: `git submodule update --init --recursive`.

Labrador is public, so the submodule needs no credentials of its own.

### Prerequisites

- **Visual Studio 2022 or newer**, with the "Desktop development with C++"
  workload. Ninja and CMake come with it.
- **The Windows 10/11 SDK**, for `d3d11.h` and friends.
- **vcpkg**, with `VCPKG_ROOT` set. `CMakePresets.json` reads it for the
  toolchain file, and configuring fails without it. Visual Studio ships a copy
  at `<VS install>\VC\vcpkg` if you have no other.

DirectX Tool Kit is in `vcpkg.json` and is fetched at configure time.

### Configure and build

The quickest way is `build.cmd` at the repository root. Double-click it and it
fetches the engine submodule if it's missing, finds Visual Studio, uses its
bundled vcpkg when `VCPKG_ROOT` isn't set, then builds Release and starts the
game. From a prompt, `build.cmd -Preset x64-debug` builds Debug and
`build.cmd -Package` also makes the Release ZIP. It installs nothing: if Visual
Studio or one of its components is missing, it says which one. See
[scripts/build.ps1](scripts/build.ps1).

By hand, from an x64 Developer prompt with `VCPKG_ROOT` set:

```
cmake --preset x64-debug          # or x64-release
cmake --build --preset x64-debug
```

The game runs from its own output directory, which the build fills with the
[reviewed runtime assets](cmake/runtime_assets.cmake) from `game/content/`, so
`out/build/x64-debug/game/ColourWars.exe` runs from anywhere. Editable sources,
historical assets and content tools live under [game/art/](game/art/README.md).

Debug builds also take developer launch arguments and have a console, for
reviewing without walking the menus:

```
out/build/x64-debug/game/ColourWars.exe +stage movement_course +players 4
```

In a match, **`** opens the console, **F5** restarts, **F6** pauses, **F7**
steps, and **F9** toggles movement
feedback. Release ignores `+` arguments and contains none of it. See
[the developer console](docs/debug-console.md).

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
Pushes and pull requests containing only Markdown or images/videos under
`docs/images/` stop after lint. Code, build files, scripts, schemas, runtime assets,
submodule changes and deletion of packaged Markdown still run the full Windows
checks. A manual workflow run always runs the full checks.

For paired GPU captures of the takeoff/landing prototype, run
`./scripts/check-movement-feedback.ps1 -Preset x64-debug -SkipBuild`.
It uses a hidden D3D11 hardware window, scripted jumps and isolated profile;
see [feedback evidence and limitations](docs/polish-feedback.md).
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
committing to produce a package labelled with that commit. The release version
is `project(VERSION)` in the root `CMakeLists.txt`; it is CPack's package version,
heads `build-info.txt` and is stamped into `ColourWars.exe` (Properties >
Details). A pull request's full CI run keeps its checked Release ZIP as the
`ColourWars-windows-x64` run artifact for 14 days. The push run that follows a
merge to `master` normally has none: it skips the Windows jobs when its tree
already passed on the pull request. No release has been cut yet. A `v*` tag on
`master` that matches `project(VERSION)` (`v0.1.0` for `0.1.0`) runs full CI,
and its `publish` job attaches that run's checked ZIP to a new GitHub Release.

Only the assets listed in [cmake/runtime_assets.cmake](cmake/runtime_assets.cmake)
are staged and packaged; configure checks that the list matches the runtime manifest.
The ZIP includes licence notices, the player guide `HOW-TO-PLAY.txt`, the seven compiled fonts, six sprite sheets,
the twelve arenas (including Crossfeed, Press Tower and the explicit
static/mover/slab/ferry/gate/lift/paint-lift Turbulence Yard comparisons) and Movement Course, tuning,
presentation, the replacement sound-bank definition and validated `paint_exe.xwb`,
with audio provenance and CC0 terms. Source audio, development art, content tools and saves are excluded.
Do not distribute the normal `game/` build directory in place of this ZIP.
Debug packaging is rejected because its runtime is not redistributable.

To check a release, extract the ZIP outside the checkout, launch from a different
working directory, confirm the title's build label, and start a solo match.
Check music and effects as well as startup. Qualify the missing-bank fallback
separately by removing the bank from a temporary extracted copy and using
`-RequireNoAudio` for its startup check. Settings still live in
`%LOCALAPPDATA%\ColourWars\save_data.json`, independently of where the ZIP was extracted.

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
the ledger of reports this client owes the engine, and a pin bump
is the moment the answered ones become visible here — each closed report records
what it lets this repository delete.

## Audio

A fresh clone and the Release ZIP include the accepted Paint.exe replacement
audio. The live bank is `game/content/sounds/paint_exe.xwb`; its definition is
`paint_exe.json` beside it. The build validates the tracked bank before staging
it. Normal game builds need no audio encoder or Python installation.

All 106 accepted masters, editable scores/recipes, retained CC0 recordings and
per-wave provenance are in [game/audio/paint_exe](game/audio/paint_exe/README.md).
That page describes how to reproduce the bank with FFmpeg and
`scripts/audio/tools/XWBTool.exe`. Music uses MS-ADPCM; short effects use PCM16.
The private legacy bank is retired and never staged or packaged. See
[NOTICE](NOTICE) for licence terms and voluntary credits.

The existing game bindings use the replacement cues, including separate
countdown beats/GO and the story's pop, sigh and chair creak. On 2026-11-05 the
author accepted the hooks and gameplay mix as they stand and waived the listening
and multiplayer checks, which were not performed; a further cue hook is
[new work](docs/audio-inventory.md#9-runtime-migration-and-remaining-integration).

The manifest still marks the bank `"optional": true`. If it is missing, the
loader substitutes a silent bank and reports it once on stderr. Gameplay
continues; the normal Release package requires the validated audible bank.

## About the code

ColourWars is a working example of object-oriented game code: abstract
interfaces, deep and multiple inheritance, virtual behavior, composition and
explicit ownership. [The architecture overview](docs/architecture.md) maps the
directories, the dependency direction, the services and the state stack. The
[OOP reading guide](docs/oop-guide.md) maps those concepts to the
implementation, follows a shot from input to score, and explains how to extend
the game. [docs/README.md](docs/README.md) indexes the rest of the docs and
[scripts/README.md](scripts/README.md) the scripts. It is Labrador's real client; the engine's `samples/`
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

## Licence

MIT — see [LICENSE](LICENSE). [NOTICE](NOTICE) records the third-party code
carried here and the content that is deliberately not distributed. The engine
has its own LICENSE and NOTICE in `external/labrador`.
