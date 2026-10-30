# Architecture

How ColourWars is put together: where the code lives, which way its
dependencies are meant to point, and the few structures everything else hangs
off. It is written for a person reading the code. The rules an agent follows
while changing it stay in [AGENTS.md](../AGENTS.md), and the
[OOP reading guide](oop-guide.md) walks the class hierarchies and one shot from
input to score. The 2026-10-30 [architecture review](review/2026-10-30-architecture-review.md)
records where the layout falls short of what is described here; its rows in
[code-hygiene.md](code-hygiene.md#architecture-review-2026-10-30) track the fixes.

## The repository

```text
game/                  the game executable and the colourwars_game library
  main.cpp             wWinMain: reads the save, tuning and presentation, builds
                       the Application and runs GameMenu
  (root files)         configuration, persistence, input, rumble, paint
                       geometry, the engine-free contracts and the match setup
  states/              the engine states: GameMenu, GameLevel and the menu pages
  objects/             one match: Level, Player, weapons, projectiles,
                       structures, terrain, HUD and the builders
  ai/                  the shipping match bots (MatchBotDriver and its helpers),
                       and the RNG and ballistics the Debug bots share with them
  carried/             engine types removed after v0.2.0, copied here (colourwars::)
  debug/               the Debug-only developer tools; compiled to nothing in Release
  content/             runtime assets staged beside the exe: manifest, tuning,
                       presentation, levels, fonts, textures, the sound bank
  art/, audio/         sources, recipes and provenance for that content; never staged
editor/                ColourWarsEditor.exe, the desktop level editor, and the
                       colourwars_level_document library it shares with its checks
scripts/               acceptance probes, check and build scripts; see scripts/README.md
docs/                  guides, owners of open work and frozen reviews; see docs/README.md
cmake/                 runtime staging, the build stamp, wave-bank and font checks,
                       release packaging
external/labrador/     the engine, a submodule at a pinned revision
external/rapidjson/    vendored JSON reader for the save and level files
```

[game/CMakeLists.txt](../game/CMakeLists.txt) builds every game source except
`main.cpp` and the two platform adapters into the `colourwars_game` static
library, `rumble_xinput.cpp` into `colourwars_rumble_xinput`,
`display_awake.cpp` into `colourwars_display_awake`, and links the three into
the `ColourWars` executable. The acceptance probes link the same library.

The `game/` root mixes several concerns:

| Concern | Files (examples) |
|---|---|
| Configuration and content checks | `tuning*`, `presentation*`, `content_value`, `content_validation`, `resource_validation.h`, `stage_catalog`, `stage_loading`, `drawable_text`, `text_wrap` |
| Persistence | `save*`, `save_section.h`, `match_records`, `last_match_setup`, `player_profiles`, `play_settings.h` |
| Input | `action_bindings`, `input_bindings`, `input_slots.h`, `player_control.h`, `controls_text` |
| Rumble and display | `rumble*`, `rumble_xinput.cpp`, `display_awake` |
| Sound | `sound_groups`, `sound_reference.h` |
| Engine-free contracts | the nine headers [below](#the-engine-free-contract-headers) |
| The match and the shell | `game_data`, `match_setup`, `seat_setup.h`, `session`, `series_stage_rule`, `round_tape`, `match_award.h`, `paint_owners.h`, `tunnelling_limits`, `fatal_report`, `directory_consts` |

Giving these role folders is the last review row, `HYG#51`.

## Dependency direction

The intended direction is downward:

```text
main.cpp
  -> states/          (GameMenu, GameLevel, menu pages)
       -> objects/    (Level and everything in a match)
            -> ai/    (the bots read the match; Level drives them)
       -> carried/
  -> configuration, persistence and input at the game/ root
  -> the engine-free contracts, which depend on nothing but each other
everything -> external/labrador (never the other way)
debug/ -> everything above; the rest reaches debug/ through three headers
```

What holds at `272ecb75`:

- **No file-level include cycles**, and nothing in `objects/`, `ai/` or
  `carried/` includes a `states/` header.
- **`carried/` depends on nothing in the game** but the precompiled header.
- **`objects/`, `ai/` and `carried/` never name `GameData`.** The states unpack
  it into the builders' constructors, and the builders hand each object only
  what it uses; that is why the probes can build a real `Level` against fakes.
- **The Debug seam is three headers.** Outside `game/debug/`, six files include
  one of `debug_director.h` (`game_data.h`, `main.cpp`, `pause_menu.cpp`),
  `debug_targets.h` (`game_states.h`, `menu_page.h`) or
  `debug_overlay_draw.h` (`level_draw.cpp`). `GameLevel` inherits
  `DebugMatchTarget` (since `HYG#33`, `DebugMatchView`, `DebugMatchControl`
  and `DebugMatchEdits`) privately and `MenuPage` inherits `DebugPageTarget`
  protected, so the tools reach a state only through those interfaces. Since
  `HYG#33` the match step is `GameLevel`'s own `step_match`, called from its
  `update`; the director only says how many steps run and with which
  developer filter.
- **The editor sees only the contracts**: it includes eight of the nine
  contract headers (and `simulation.h` through `horizontal_motion_contract.h`),
  and `colourwars_level_document` compiles without the engine's include path.

Where it does not hold yet:

- `ai/` and `objects/` include each other: the bot driver reads `Level`,
  `Weapon` and the team gate, and `level.cpp`/`level_audio.cpp` include
  `match_bot_driver.h`. A world-query interface for the bots is part of
  `HYG#40`.
- The root is not a layer of its own: `game_data.h` includes `objects/` headers,
  `action_bindings.h` includes `states/menu_input_action.h`, and thirteen
  presentation sources include `states/ui_player_badge.h` (`HYG#38`).
- Nothing enforces the contract headers' independence; an include check is
  part of `HYG#51`. The engine's `check_engine_includes` target only stops the
  engine reaching into `game/`.

## The engine-free contract headers

Nine headers at the `game/` root hold rules the game and the editor must apply
in the same words, with no engine include:

| Header | Holds |
|---|---|
| `level_format.h` | the level format's one owner: its version and compatibility rule, key names, object and collision vocabulary, which tags each object type may carry, the material table and the fixed slab's refused fields; [level.schema.json](../game/level.schema.json) sits beside it |
| `simulation.h` | `SIMULATION_HZ`, the fixed step written once |
| `horizontal_motion_contract.h` | authored horizontal motion and its envelope arithmetic (uses `simulation.h`) |
| `station_contract.h` | a level's review stations and their lessons |
| `team_gate_contract.h` | a team gate's authored rules |
| `user_stage_contract.h` | the names a player's own stage file may take |
| `paint_face_geometry.h`, `paint_polygon_geometry.h` | how paint tiles a face and a polygon, for the runtime tiles and the editor's scoring diagnostics |
| `fixed_slab_paint_geometry.h` | the fixed slab's paint polygon (uses the two above) |

They are header-only and include only the standard library and one another.
The game's builders and startup validation read them; so do the editor's level
document and JSON reader.

## `game/carried`

Seven engine types that Labrador v0.2.0 had and its master removes
(`AnimationObject`, `MovingObject`, `SoundBankObject`, `TextDropShadow`,
`UiTextDropShadow`, the camera tools and `RotationOrigin`), copied with their
shape unchanged into `namespace colourwars` so they can never be ambiguous with
an engine type. The folder is named
for where the code came from, not for what it does; the removal without a
deprecation release is report `ENG#28` in the
[engine-report ledger](review/engine-reports.md).

## Services: `GameData`

[game/game_data.h](../game/game_data.h) is a bag of six borrowed pointers:
the engine `Application`, this game's `Save`, the `StageCatalog`, the tuning,
the presentation and the developer `DebugDirector` (inert in Release, and
`DebugDirector::inert()` when unset). Everything else that looks like a
service (`ThreadPool`, `Gamepads`, `RenderResources`, ...) is a forward to the
`Application`. `wWinMain` in [game/main.cpp](../game/main.cpp) declares all six
before the `Application`, so they outlive every state that borrows them.

[game/stage_catalog.h](../game/stage_catalog.h) owns the ordered stage list
and the level registry and is their only writer: the manifest's `"level"` kind
adds the shipped stages, and `UserStageFolder`
([game/stage_loading.h](../game/stage_loading.h)) replaces the user stages
after them. `GameData` lends it read-only, with `stages()` and `level_infos()`
forwarding to its two halves.

It is the shell's bag, not the game's. `GameMenu`, `GameLevel`, `MenuContext`,
`MenuPage`, `StagePreview` and the developer tools hold it. Gameplay objects do
not: `GameLevel` unpacks it into `LevelBuilder`'s constructor (viewports,
render and audio resources, play settings, level registry, tuning,
presentation, resolution, thread pool, partitioner and input bindings),
and from there each object is constructed with only what it uses.

## The state stack

[game/states/game_states.h](../game/states/game_states.h) defines two states
over the engine's state stack. `GameMenu` sits at the bottom for the whole
session and owns what the menu pages share: the `MenuInput`, the `MatchSetup`
being assembled, the `MenuContext` and the menu music. `GameLevel` is one match
or series, with a fresh `Level` for each round. Pages derive from `MenuPage`
and share a `MenuContext` (the services, the menu input and the `MatchSetup`),
owned by whichever state owns the screen; `GameLevel` builds its own for the
pause, results and end pages. Both states implement
[`MenuOwner`](../game/states/menu_owner.h), the interface the context reaches
its owner through: the owner's music clock and, for `GameMenu` alone, the
player's stage folder.

A page replaces another in its frame with `transition_to` and covers it with
`push`. Inside the main menu's frame every replacement is a `MainMenuRoute`
followed by `MainMenuPage::go_to` and built by `make_main_menu_page`;
[main_menu_route.cpp](../game/states/main_menu_route.cpp) draws the setup flow
as one graph. The shapes that occur:

```text
[GameMenu]                                      only between a pop and a push
[GameMenu, MainMenu*Page]                       title, home, options, credits and setup
[GameMenu, MainMenuHome, MainMenuExitConfirmation]
[GameMenu, MainMenuStageSelect, UserStageRefusalsMenu]
[GameMenu, MainMenuOptions, MainMenuDisplayConfirmation]
[GameMenu, MainMenuOptions, ControlsSettingsMenu]
[GameMenu, MainMenuOptions, ControlsSettingsMenu,
    StickSettingsMenu | KeyboardJumpMenu | PadButtonsMenu | ProfilesMenu]
[GameMenu, GameLevel]
[GameMenu, GameLevel, PauseMenuInitial | PauseMenuConfirmation]
[GameMenu, GameLevel, PauseMenuInitial, MainMenuOptions, ...]   the Options shapes above
[GameMenu, GameLevel, ResultsMenuInitial]
[GameMenu, GameLevel, EndMenuInitial | MainMenuWeaponSelect | MainMenuStageSelect]
```

In Debug, the console (`DebugConsoleState`) and the debug menu
(`DebugMenuPage`) are pushed over whichever of these is on top.

The page bases are named by role. `MainMenuPage` is a page in the main menu's
frame, and `RosterSetupPage` under it adds the roster machinery that Player
Count, Join, Team, Weapon and Stage Select share. `SettingsPanelPage` is the five
settings panels, pushed with no result over Options or Pause, so their only way
out is to pop. `PauseMenuPage` is the two pause pages, which return a
`PauseMenuAction` through `finish`.

A main-menu page finishes by popping its frame with a `MatchStart`, and
`GameMenu` then pushes a `GameLevel` above itself, so a match never replaces
the menu and returning from it lands on a screen that still exists. The
results page pops and `GameLevel` pushes the end menu. Between rounds of a
series the end menu replaces itself in its frame with Weapon Select or Stage
Select (Back returns to it), and a page started from the pause menu's Options
is the same `MainMenuOptions` class the main menu uses. `GameLevel` restores the
screen layout it took in its destructor.

## Level and Scene

[game/objects/level.h](../game/objects/level.h): `Level` is one paint match:
the countdown, the clock, the HUDs, the cameras, the music, the bots. The
generic parts (object lists, the tick, the collision sweep, out-of-bounds
retirement, per-view render fan-out) are `labrador::Scene` in the engine, which
`Level` owns. `Level` keeps non-owning `Player*` into the scene; the index into
`players_` is the stable roster participant, not the player's device slot
(`player_num()`) or human pane. Bots have no device or pane, and
`participant_for_view` and `view_for_participant` map between panes and seats.
`Level::update` takes one command per participant: `GameLevel` reads a
`SlotInput` per device slot, keeps the pause signals, and its
[SeatCommandBuilder](../game/objects/seat_commands.h) maps slots to seats and
resolves mouse aim, so the level never reads a device or the mouse.

Construction goes through builders that read the level definition and the
tuning and hand finished objects to the scene. `LevelBuilder` owns a
`PlayerBuilder` (which owns a `WeaponBuilder`) and a `LevelObjectBuilder`; each
`Weapon` owns the `ProjectileBuilder` it fires through.

## Data

Four JSON files are read at startup, before any asset:

| File | What |
|---|---|
| `manifest.json` | which fonts, sprite sheets, levels and sound banks exist |
| `tuning.json` | every number a match is balanced on ([game/tuning.h](../game/tuning.h)) |
| `presentation.json` | every font, colour, offset and box size the shell is laid out from ([game/presentation.h](../game/presentation.h), with the menu-page layout records in [presentation_menu.h](../game/presentation_menu.h) and the menu text in [presentation_menu_text.h](../game/presentation_menu_text.h)) |
| `%LOCALAPPDATA%\ColourWars\save_data.json` | display and play preferences, read before the window exists, and per-stage records, in versioned `SaveSection`s ([game/save_section.h](../game/save_section.h)) |

Tuning and presentation are two files for two audiences (a balance pass and an
art pass), and neither is a manifest entry: they are read before every asset
and are what those assets are used by. The first three are content the build
stages beside the executable; the save is written by the player and created by
nothing but the game. An older save at `./save/save_data.json` in the staged
folder is read once, if nothing is at the real path, to carry it across
([game/directory_consts.h](../game/directory_consts.h)).

Content resolves beside the executable, not the working directory: the engine's
rule (`engine/app/content_root.h`), which `Application::load_manifest` applies
to the manifest and `main.cpp` applies by hand to the files it reads before it.
`main.cpp` also teaches the engine's resource loader the `"level"` kind
(`register_kind`), since a level definition means nothing to the engine.
[Gameplay tuning](gameplay-tuning.md), [menu presentation](menu-presentation.md)
and [the level format](level-format.md) document each file.

## The object model

The game is deliberately inheritance-first; [AGENTS.md](../AGENTS.md#the-game-is-deliberately-oop-heavy--do-not-fix-it)
says why and the [OOP guide](oop-guide.md) says how to read it. The main
chains, with every base:

```text
ProjectileMist -> DiffusingProjectile -> Projectile
    -> colourwars::MovingObject + colourwars::AnimationObject + labrador::CollisionObject
WeaponBucket -> RelativeVelocityWeapon -> Weapon -> labrador::TextureObject
StructurePaintable -> Structure + PaintableObject
    Structure -> labrador::TextureObject + labrador::CollisionObject
Player -> colourwars::MovingObject + labrador::CollisionObject
    + colourwars::AnimationObject + labrador::TextureObject
MainMenuStageSelect -> RosterSetupPage -> MainMenuPage -> MenuPage
    -> labrador::State + colourwars::SoundBankObject + protected DebugPageTarget
```

`AnimationObject` and `TextureObject` both derive from the engine's
`SpriteSheetObject`, so `Player` holds two `DrawObject` subobjects; the
[guide](oop-guide.md#players-two-drawing-bases) explains how it uses them.

## Checks

There are no CTest targets here. The game's probes are run by
`scripts/run-acceptance.ps1` from the cases declared in
[scripts/acceptance/cases.psd1](../scripts/acceptance/cases.psd1); the engine's
tests live with the engine. [scripts/README.md](../scripts/README.md) lists every
script, and [the README](../README.md#consumer-acceptance) covers running them.
