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
  (root files)         the shell: GameData, the fatal report, content paths,
                       the display-idle adapter, the precompiled header
  contracts/           the engine-free headers the editor shares, and the
                       level schema; an include check holds them to it
  match/               what a match is: setup, seats, the session, the series
                       rule, the round tape, awards and paint owners
  config/              tuning, content values and validation, the stage catalog
                       and loading, settings, locales, text fitting and sound
                       groups
    presentation/      presentation.json's records, readers and validators,
                       and the menu geometry
  save/                the save file, its sections, profiles, records and the
                       remembered setup
  input/               bindings, input slots, the controls text, and rumble
                       with its engine-Gamepads adapter (`rumble_gamepads.cpp`)
  states/              the engine states: GameMenu, GameLevel and the menu pages
  objects/             one match: Level, Player, weapons, projectiles,
                       structures, terrain, HUD and the builders
  ai/                  the shipping match bots (MatchBotDriver and its helpers),
                       and the RNG and ballistics the Debug bots share with them
  carried/             engine types and members v0.3.0 removed, copied here (colourwars::)
  debug/               the Debug-only developer tools; compiled to nothing in Release.
                       The director and the three seam headers sit at its root;
                       commands/, console/, input/, script/, replay/, tune/
                       and capture/ hold its subsystems
  content/             runtime assets staged beside the exe: manifest, tuning,
                       presentation, levels, fonts, textures, the sound bank
  art/, audio/         sources, recipes and provenance for that content; never staged
editor/                ColourWarsEditor.exe, the desktop level editor, and the
                       colourwars_level_document library it shares with its checks;
                       colourwars_editor_shell is the window the DPI probe also links
scripts/               acceptance probes, check and build scripts; see scripts/README.md
docs/                  guides, owners of open work and frozen reviews; see docs/README.md
cmake/                 runtime staging, the build stamp, wave-bank and font checks,
                       release packaging
external/labrador/     the engine, a submodule at a pinned revision
external/rapidjson/    vendored JSON reader for the save and level files
```

[game/CMakeLists.txt](../game/CMakeLists.txt) builds every game source except
`main.cpp` and the two platform adapters into the `colourwars_game` static
library, `input/rumble_gamepads.cpp` into `colourwars_rumble_gamepads`,
`display_awake.cpp` into `colourwars_display_awake`, and links the three into
the `ColourWars` executable. The acceptance probes link the same library.

The role folders date from `HYG#51`:

| Folder | Holds (examples) |
|---|---|
| the `game/` root | the shell: `main.cpp`, `game_data`, `fatal_report`, `directory_consts`, `display_awake`, `pch.h`, `resource.rc` and the `colourwars.ico` it embeds |
| `contracts/` | the twelve engine-free headers [below](#the-engine-free-contract-headers) and `level.schema.json` |
| `match/` | `match_setup`, `match_rules.h`, `seat_setup.h`, `player_control.h`, `session`, `series_stage_rule`, `round_tape`, `match_award.h`, `paint_owners.h` |
| `config/` | `tuning*`, `tunnelling_limits`, `content_value`, `content_validation`, `resource_validation.h`, `stage_catalog`, `stage_loading`, `stage_scan_diagnostics`, `play_settings.h`, `settings_session`, `drawable_text`, `text_wrap`, `sound_groups`, `sound_reference.h` |
| `config/presentation/` | `presentation*` and `menu_geometry` (the menu design size, every page rectangle, the fit-to-box scale and rectangle predicates, and the settings pages' value rows, shared by the pages, the reader and the validators) |
| `save/` | `save*`, `save_section.h`, `match_records`, `last_match_setup`, `player_profiles`, `play_settings_json` (the one `StickSettings` and `SeatComfort` reader and writer), and `atomic_file` (`replace_file_atomically`, the one whole-file writer the save, the fatal-error record and the developer tools share) |
| `input/` | `action_bindings`, `menu_input_action.h`, `input_bindings`, `input_slots.h`, `controls_text`, `controls_copy.h`, `rumble*`, `rumble_gamepads.cpp` |

`play_settings.h` and `settings_session` are in `config/` rather than `save/`:
every gameplay object reads the play settings, and the save is one of their
readers. `game/carried` keeps its name; see [below](#gamecarried).

`game/debug` keeps at its root what the rest of the game may name
(`dev_tools.h`, `debug_director.h`, `debug_targets.h`,
`debug_overlay_draw.h`) and the director's implementation, which ties the
subsystems together. `commands/` is the registry, the variables, the tokenizer,
the log and the general commands; `console/` the console and the debug menu;
`input/` keys, binds, the pad chord and the background policy; `script/` the
scripts, waits and the developer files; `replay/` recording, plans,
fingerprints, the developer clock and the Debug bots; `tune/` live tuning; and
`capture/` the perf overlay and screenshots. Each subsystem's command file sits
with the code it registers.

## Dependency direction

The intended direction is downward:

```text
main.cpp
  -> states/          (GameMenu, GameLevel, menu pages)
       -> objects/    (Level and everything in a match)
            -> ai/    (the bots read the match; Level drives them)
       -> carried/
  -> match/, config/, save/ and input/
  -> contracts/, which depends on nothing but itself
everything -> external/labrador (never the other way)
debug/ -> everything above; the rest reaches debug/ through three headers
```

What holds (checked 2026-11-05):

- **No file-level include cycles**, and nothing in `objects/`, `ai/` or
  `carried/` includes a `states/` header.
- **`carried/` depends on nothing in the game** but the precompiled header.
- **`ai/` sees a match only through [BotWorld](../game/ai/bot_world.h)**
  (since `HYG#40`): it reaches `objects/` through value headers and the
  `paintable_object.h` interface, never `Level`, `Player`, `Weapon` or a gate.
  `Level` implements the interface with its owned `LevelBotWorld` and owns the
  `MatchBotDriver`, so the remaining edge runs one way, `objects/` to `ai/`.
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
- **Gameplay headers see only the in-match presentation.** `objects/` headers
  include [presentation_match.h](../game/config/presentation/presentation_match.h), and `level.h`
  and the builders forward-declare `Presentation`, so no `objects/` or `ai/`
  header reaches the menu records. Some `objects/` sources do, by including
  `presentation.h` (or `presentation_menu_text.h`) themselves; since `HYG#38`
  no presentation source includes a `states/` header.
- **The editor sees only the contracts**: it includes nine contract headers
  directly, and `rectangle_contract.h` and `simulation.h` through them;
  `match_variant.h` belongs to match setup and is not an editor dependency.
  `colourwars_level_document` compiles without the engine's include path.

Where it does not hold yet:

- The root is not a layer of its own: `game_data.h` includes `objects/` headers.
- The role folders include each other: `save/` reads `config/`'s play
  settings and `match/`'s setup, and `config/` names `match/` and `input/`
  types. Only `contracts/` has a checked boundary.

## The engine-free contract headers

Twelve headers in `game/contracts/` hold engine-free rules. Eleven give the game
and the editor the same level vocabulary and geometry; `match_variant.h` gives
match setup, persistence and replay one vocabulary for named variants:

| Header | Holds |
|---|---|
| `level_format.h` | the level format's one owner: its version and compatibility rule, key names, the paintable-face names, the scenery greys, object and collision vocabulary, which tags each object type may carry, the material table, the fixed slab's refused fields and the format's size limits (`level_format::limit`); [level.schema.json](../game/contracts/level.schema.json) sits beside it |
| `simulation.h` | `simulation::simulation_hz` and `simulation::seconds_per_step`, the fixed rate and the step length the engine's timer derives from it (0.0166666 s), written once |
| `rectangle_contract.h` | the one rectangle the station, gate and motion contracts take |
| `horizontal_motion_contract.h` | authored horizontal and vertical motion, its heading names and its envelope arithmetic (uses `simulation.h`) |
| `station_contract.h` | a level's review stations and their lessons |
| `team_gate_contract.h` | a team gate's authored rules |
| `user_stage_contract.h` | the names a player's own stage file may take, which stage registration also applies |
| `drawable_text_contract.h` | the characters the fonts draw and the punctuation folded to ASCII, for `drawable_text` and the editor's display-name check |
| `paint_face_geometry.h`, `paint_polygon_geometry.h` | how paint tiles a face and a polygon, for the runtime tiles and the editor's scoring diagnostics |
| `fixed_slab_paint_geometry.h` | the fixed slab's footprint (`slab_span`, `slab_rise`, `slab_thickness`) and paint polygon (uses the two above) |
| `match_variant.h` | `MatchVariant` (`standard`, `endless_paint`), its ordered choices, validation and stable serialization keys; variants are fixed C++ policies |

They are header-only and include only the standard library and one another.
The game's builders, setup and startup validation read them; the editor's level
document and JSON reader read the level contracts. The `colourwars_check_contract_includes` target
([cmake/check_contract_includes.cmake](../cmake/check_contract_includes.cmake))
fails the build when one includes anything else. Both `colourwars_game` and the
editor's `colourwars_level_document` depend on it. The engine's
`check_engine_includes` only stops the engine reaching into `game/`.

## `game/carried`

Six engine types that Labrador v0.2.0 had and v0.3.0 removes
(`AnimationObject`, `MovingObject`, `SoundBankObject`, `TextDropShadow`,
`UiTextDropShadow` and the camera tools), copied with their shape unchanged
into `namespace colourwars` so they can never be ambiguous with an engine type.
A seventh, `RotationOrigin`, was carried too; `HYG#73` deleted it with the
`rotation_origin` tuning key it served, which nothing read. `removed_members.h` does the same for two members v0.3.0 also
removed, `RectangleF::from_top_left_bottom_right` and
`Camera::calculate_intermediate_camera`, as free functions. The folder is named
for where the code came from, not for what it does; the removal with an
engine-only caller count is report `ENG#28` in the
[engine-report ledger](review/engine-reports.md). `HYG#51` kept the name: the
folder is that report's exhibit. The author's 2026-11-03 answer was to amend the
engine's changelog before the tag, but `v0.3.0` shipped both entries unamended,
so on 2026-11-05 the author closed the report as declined. The folder is the
migration in use; its contents are deleted, not renamed, only if the engine
brings the types back.

## Services: `GameData`

[game/game_data.h](../game/game_data.h) is a bag of eight borrowed pointers:
the engine `Application`, this game's `Save`, its `SettingsSession`, the
`StageCatalog`, the tuning, the presentation, the `ActiveLocale` and the
developer `DebugDirector`
(inert in Release, and `DebugDirector::inert()` when unset). Everything else
that looks like a service (`ThreadPool`, `Gamepads`, `RenderResources`, ...) is
a forward to the `Application`. `wWinMain` in [game/main.cpp](../game/main.cpp)
declares the other seven before the `Application`, so they outlive every state that
borrows them.

The `Save` ([game/save/save.h](../game/save/save.h)) is what the file holds, each pad slot
as itself. The [`SettingsSession`](../game/config/settings_session.h) borrows it and
owns what belongs to the run: which pad wears which player profile, and the
settings in effect, the save's with each worn profile laid over its pad. Every
reader of play settings or input bindings reads the session's, and the Controls
pages write through it, so a worn pad's edits reach its profile.

[game/config/stage_catalog.h](../game/config/stage_catalog.h) owns the ordered stage list
and the level registry and is their only writer: the manifest's `"level"` kind
adds the shipped stages, and `UserStageFolder`
([game/config/stage_loading.h](../game/config/stage_loading.h)) replaces the user stages
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
    StickSettingsMenu | KeyboardKeysMenu | PadButtonsMenu | ProfilesMenu]
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
is the same `MainMenuOptions` class the main menu uses. `GameLevel::init`
creates two scopes the destructor ends: one restores the screen layout the
match replaced, the other attaches the match to the developer tools. Every
reason a match is suspended (a state over it, its pause menu, a resume
count-in, lost focus, and Debug background running) is one
[MatchSuspension](../game/states/match_suspension.h), which answers whether
audio may play and whether to count in; each callback reads the window's focus
and the director's clock and background policy into one `SuspensionGate`.

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

`Level` is built from owned parts, each of which keeps one concern's state
and drawing: `LevelStructures` (the movers, team gates and projectile
targets), `LevelBotWorld` (the [BotWorld](../game/ai/bot_world.h) the bots
query), `MatchAudio` (the voices and the music), `CountdownCards`,
`PaperCard`, `PracticeLessons`, `PlayerIdentityHud`, `TeammateMarkers`,
`CursorMarkerOverlay` and, when the match has an intro, `MatchOpening` (the
monitor, the person, the reveal and the figures' acting). `Level` keeps the
match state, its timers and the order of update, draw and layout, so its
`update`, `draw` and `refresh_layout` read as the match's sequence; a part that
needs the state is handed it per call and returns a transition for `Level` to
apply rather than writing it. The parts borrow the tuning and presentation and
read them per call, so `tune` stays live.

Construction goes through builders that read the level definition and the
tuning and hand finished objects to the scene. `LevelBuilder` owns a
`PlayerBuilder` (which owns a `WeaponBuilder`) and a `LevelObjectBuilder`; each
`Weapon` owns the `ProjectileBuilder` it fires through. `LevelBuilder` hands
`Level` four records from
[level_inputs.h](../game/objects/level_inputs.h): the world it owns (scene,
players, dividers), the services and live records it borrows, what the stage
says, and this round's decisions.

`MatchSetup` carries one `MatchVariant`, selected on Mode and retained across
series rounds. `PlayerBuilder` passes that policy to each `Player`; Endless
paint gives the tool a supply policy that bypasses ammunition cost and keeps
the tank full, while the weapon keeps its normal cooldown and shot feedback.
`MatchBotDriver` turns off the bot's reserve
and refill rules for that variant, including refill-only ground queries.
Scoring, duration, movement and the other bot decisions keep their normal
rules. The match policy is independent of the Debug infinite-ammunition cheat,
and recorded setup preserves the selected variant.

## Data

Five JSON files are read at startup, before any asset:

| File | What |
|---|---|
| `manifest.json` | which fonts, sprite sheets, levels and sound banks exist |
| `tuning.json` | every number a match is balanced on ([game/config/tuning.h](../game/config/tuning.h)) |
| `presentation.json` | every font, colour, offset and box size the shell is laid out from ([game/config/presentation/presentation.h](../game/config/presentation/presentation.h), with the in-match records in [presentation_match.h](../game/config/presentation/presentation_match.h), the menu-page layout records in [presentation_menu.h](../game/config/presentation/presentation_menu.h) and the menu text in [presentation_menu_text.h](../game/config/presentation/presentation_menu_text.h)) |
| `locales/locales.json` | the languages the copy is in; each listed `locales/<tag>.json` is laid over `presentation.json`'s text before it is read ([localisation.md](localisation.md), [game/config/locale.h](../game/config/locale.h)) |
| `%LOCALAPPDATA%\ColourWars\save_data.json` | display and play preferences, remembered match setup and per-stage records, in versioned `SaveSection`s ([game/save/save_section.h](../game/save/save_section.h)) |

Tuning and presentation are two files for two audiences (a balance pass and an
art pass), and neither is a manifest entry: they are read before every asset
and are what those assets are used by; neither are the locales, which are
`presentation.json`'s copy in another language. The first four are content the build
stages beside the executable; the save is written by the player and created by
nothing but the game. An older save at `./save/save_data.json` in the staged
folder is read once, if nothing is at the real path, to carry it across
([game/directory_consts.h](../game/directory_consts.h)).

Save version 7 adds the remembered setup's `last_match.variant`. An absent or
unrecognized variant restores Standard, so older saves keep their existing
match policy; Endless paint is written with its stable `endless_paint` key.
Save version 8 adds each seat's `last_match.players[].sub`, written only when
it is not the grenade; absent or unknown is the grenade, which every older
match carried. Save version 9 adds the Sub action to `input_bindings`
(`keyboard.sub` and each pad profile's `sub`, also in `profiles`); absent reads
as the right mouse button (the left when Shoot has the right) and RB (else LB,
else the first free control), where older builds put it.

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
    -> colourwars::MovingObject + colourwars::AnimationObject + Collider
WeaponBucket -> RelativeVelocityWeapon -> Weapon
    -> labrador::TextureObject + colourwars::SoundBankObject
StructurePaintable -> Structure + PaintableObject
    Structure -> labrador::TextureObject + Collider
Player -> colourwars::MovingObject + Collider
    + colourwars::AnimationObject + labrador::TextureObject
    + colourwars::SoundBankObject
Collider -> labrador::CollisionObject
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
