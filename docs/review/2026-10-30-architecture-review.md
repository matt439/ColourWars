# Architecture review (2026-10-30)

A record of the architecture review run at the author's request on 2026-10-30,
against `fix/paused-stick-preview` at `96c65373` with the engine pinned at
`v0.2.0`. The question was whether the architecture is well laid out and sound,
given that the project is read as an example (see [the OOP guide](../oop-guide.md)).

**This file is frozen.** It records the findings as they were made. Live status
is in [code-hygiene.md](../code-hygiene.md#architecture-review-2026-10-30)
(the `HYG#28` to `HYG#51` rows and `CI#6`) and, for the engine's side, in
[the engine-report ledger](engine-reports.md) (`ENG#27` to `ENG#32`).
[TODO.md](../../TODO.md) indexes both.

## Method

Ten reviewers each read one area: layout, dependency direction, the object model,
states and menus, data and configuration, Debug isolation, simulation/input/bots,
the engine seam, build/tests/tooling, and the docs as a guide. A separate
skeptic re-read the code for each finding and tried to refute it; a completeness
critic then looked for areas the ten had missed. 77 findings survived and one was
refuted. The skeptics corrected many line numbers and moved every `high` to
`medium` or `low`, so the severities below are the adjusted ones.

The guardrails from [AGENTS.md](../../AGENTS.md) applied: no finding holds the
game to Labrador's T11, engine changes go to the ledger, tuning values are out
of scope. Line numbers are as of `96c65373`.

## What is sound

- **No file-level include cycles.** A strongly-connected-component pass over every
  `#include "game/..."` edge found none. Directory pairs include each other (see
  theme 9), but files never do.
- **Gameplay objects never see `GameData`.** No file in `game/objects`, `game/ai` or
  `game/carried` names it; builders unpack it and hand objects only what they use.
  This is why acceptance probes build real levels against fakes.
- **The Debug seam is narrow at the header level.** Non-debug code reaches three
  `game/debug/` headers from six files, and the interfaces are inherited privately
  (`GameLevel : private DebugMatchTarget`, `MenuPage : protected DebugPageTarget`).
- **The hierarchies are well formed.** Leaves are `final`, intermediate bases hold
  real shared behaviour, `PaintableObject` is a clean role interface, and ownership
  is linear and commented (builders return `unique_ptr`, `Scene` owns, `Level`
  borrows with a comment beside each borrow).
- **One draw path for menus.** No page overrides `draw()`; pages build widgets in
  `init()` and handle input in `update_page()`.
- **One shared range-checked reader.** `content_value` is called 358 times from
  tuning and presentation, and startup checks run in clear phases.
- **The editor's `colourwars_level_document` library** compiles the shared level
  rules with no engine include path, which keeps those headers engine-free.

## Findings by theme

### 1. The match setup lives in the menu layer

Raised independently by six reviewers. `MenuLevelSettings` and `MenuPlayerSettings`
(`game/states/menu_level_settings.h`, `menu_player_settings.h`) describe the
match roster: seats, teams, weapons, handicaps, bots, stage, duration, best-of,
series rule and pad remapping. Their only UI member is `player_label(int, const
MenuText&)`. `objects/level.h:44`, `objects/level_builder.h:6`,
`objects/player_builder.h:6`, `ai/match_bot_driver.h:5`, `round_tape.h:5`,
`debug/replay.h:5` and others include them, and they cause every include from
`objects/` and `ai/` into `states/`. `content_validation.cpp:8` also includes
`states/main_menu.h` to call two static row-fit validators.

### 2. Acceptance bypasses the build system

`game/CMakeLists.txt:6` is the only target in `game/`: one `add_executable` with
about 210 sources and no library. `scripts/run-acceptance.ps1:199-213` globs
`CMakeFiles/ColourWars.dir/**/*.obj`, drops `main.cpp.obj` by name and then
hard-codes flags, defines and the engine's link list (`:219-246`). Five more
scripts (`check-display-awake`, `check-frame-pacing`, `check-movement-feedback`,
`check-paint-fx`, `check-painter`) repeat both. Platform adapters are swapped by
object filename (`rumble_xinput.cpp.obj`, `display_awake.cpp.obj`), and four
probes redefine the engine's `GamepadReader` inside `namespace labrador` and
rely on link order. The list of cases is written in several places
(`run-acceptance.ps1:19-22`, `$longestFirst` at `:412-414`, CI), and the
"fourteen opt-in cases" in the docs is now 16. 73 probes each define their own
`require`. The "no CTest by design" rationale only covers the engine's tests,
not the game's own 78 checks.

### 3. `Level` and `GameLevel` are god classes

`Level` (`objects/level.h`, 634 lines, 44 includes) has a 28-parameter positional
constructor (`:84-110`), about 118 data members and about 4,090 lines of member
definitions across 8 `level_*.cpp` files that read as parts of one class. It
carries bot-only queries (`:195-206`), so changing the AI's view of the world
means editing `Level`. `GameLevel` (`states/game_states.h:67`) keeps its audio,
focus and pause rules in four booleans plus a replay phase across six callbacks,
with 43 calls into the debug director woven through them.

### 4. The shipping match step runs through debug code

`GameLevel::update` (`game_states.cpp:492`) advances the match only via
`data()->debug().update_match(...)`. In Release that forwards to
`GameLevel::debug_update_level(inputs, dt, nullptr)`, defined in
`game_level_dev_tools.cpp`, which is the real step including the shipping
instant-replay recorder. `DebugMatchTarget` (`debug/debug_targets.h:28-103`)
has 22 virtual methods mixing lifecycle, the step, audio/focus, cheats and
probes. `docs/debug-console.md`'s Release paragraph describes the data path
incorrectly.

### 5. Weak spots in the showcase OOP

Judged as OOP, not against T11:

- 33 `dynamic_cast`s in `game/objects/*.cpp`, many to concrete structure leaves
  from `Player`, `Projectile` and `Level`; `Structure` lacks the support and
  moving-surface queries its callers need, and the OOP guide teaches the cast
  as the extension recipe.
- Bases test their subclasses' tags: `weapon.cpp:388`/`:395` return early for
  `WeaponType::roller` although `WeaponRoller` exists; `Projectile` and `Level`
  switch on projectile kind.
- Collision response switches on a kind-times-team tag
  (`collision_object_type.h:8-30`) and then reference-`dynamic_cast`s. Part of
  the cost is the engine's (`ENG#29`).
- `PauseMenuPage` is the base of five settings pages pushed from the main menu,
  where its `finish()` is invalid; which exits are valid is decided at runtime.
  The five pages repeat their panel scaffolding, `MainMenuPage` carries roster
  machinery every page inherits, and `MenuPage` has hooks no page uses
  (`shows_brand` is overridden to false by all 18 pages).
- `Player` has two `DrawObject` subobjects through a non-virtual diamond
  (`AnimationObject` and `TextureObject`), undocumented in the guide.
- `const` methods with `mutable` state change gameplay and audio
  (`Projectile::note_paint_contact`, `projectile.h:79`), while `const` is the
  engine's draw-worker safety contract.
- `MenuContext` holds five `std::function` slots so pages can drive their
  owner's music clock; main-menu navigation is 27 direct `transition_to` sites
  beside a mostly bypassed `MainMenuScreen` factory; `GameLevel`'s layout
  restore is acquired in `init()` but released unconditionally in the destructor.

### 6. Copied mechanisms, some already drifted

- Bot ballistics: `debug/bot_driver.cpp:31-56` copies `match_bot_driver.cpp:95-127`
  and lacks the shipping gravity cap; projectile flight is predicted in three
  places besides `Projectile`; splitmix64 and the seed/seat mix are written twice.
- Two record/replay models: `RoundTape` (`round_tape.h`) and the Debug
  `ReplayFile`/`ReplayRecorder`/`ReplayPlayer` (`debug/replay.h`), with
  different gap models.
- The weapon-type-to-name mapping is written by hand in 8 or more files
  (`content_validation.cpp:90`, `last_match_setup.cpp:24`, `presentation_read.h:20`,
  `presentation_read_menu.cpp:525`, four lists in `presentation_read_match.cpp`,
  three in `debug/match_plan.cpp`).
- Menu page geometry is worked out in the reader, the validator
  (`presentation_validation_setup.cpp:44`, from a literal 1920) and each page
  (`main_menu_team_select.cpp:359`, from `DESIGN_RESOLUTION`).
- Level JSON has two decoders: `editor/level_json.cpp`/`level_document.cpp` and
  `objects/level_loaded_info.cpp`/`level_object_builder.cpp`; the
  material-collision table exists twice.
- Stages live in a `StageList` and a `Registry` that two writers keep in step
  (`main.cpp:93-100`, `UserStageFolder`).
- Bots hard-code `60` where `simulation.h` says `SIMULATION_HZ` is written once.
- Input defaults are stated in `default_action_bindings()`, `InputBindings` and
  `PadProfile`.

### 7. The precompiled header claims a backend

`game/pch.h:37-79` includes D3D11, DXGI, DirectXMath and 18 DirectXTK headers;
`game/CMakeLists.txt:231` links `Microsoft::DirectXTK`. The engine made
DirectXTK private (`engine/CMakeLists.txt:398-423`) and no game file uses it;
32 files carry a dead `using namespace DirectX`.

### 8. Determinism rests on the global `rand()`

`main.cpp:42` seeds with `srand(time)`, `game_states.cpp:268-270` draws a round
seed and calls `srand(seed)`, `game_level_replay.cpp:44` re-seeds before
`build_level`. Team colours, bot seeds and the monitor persona all draw from the
same stream, so replay correctness depends on call order.

### 9. The layout does not show dependency direction

The `game/` root holds 93 files across about seven concerns (configuration,
persistence, input, rumble, paint geometry, contracts, the app). Every directory
pair includes each other. The nine engine-free headers the editor shares
(`*_contract.h`, `level_format.h`, `paint_*_geometry.h`, `simulation.h`) are an
unmarked subset of the root. The presentation split copied a 24-line include
block into 13 files, creating false includes into `states/` and a `build_info.h`
dependency that recompiles them on every commit. `game/debug` is 79 flat files;
`game/carried` is named for its origin and is the only namespaced code.
Input files span three folders. `Level` takes a `Mouse*` and slot-indexed input,
and `PlayerInputData` mixes simulation commands with shell signals.

### 10. Docs

There is no human-facing architecture overview: it sits in `AGENTS.md` after
about 250 lines of agent policy. `AGENTS.md` says `GameData` is handed to every
object, which is wrong, and the code's boundary is the better design. The
state-stack diagram omits most real shapes, and `game_states.h:28` itself is
wrong (`pause_menu.cpp:393` pushes `MainMenuOptions`). The inheritance chains
omit bases. Neither `docs/` nor `scripts/` has an index; `docs/debug-console.md`
is 3,724 lines, mostly work record.

### Smaller findings

- `Save` mixes the file with run-only profile wearing (`save.h:71-115`), and a
  malformed section resets silently.
- Audio has three idioms (`SoundBankObject`, raw `SoundBank*` plus handles, small
  audio components) and a hand-written pause fan-out.
- Presentation validation is split by size and held together by check order;
  `presentation.h` drags the menu records into 45 includers.
- Levels are read lazily from a kept `JsonDocument` rather than into a typed record.
- `ci.yml` keeps the Release package rules inline (`:253-292`), so they cannot be
  run locally.

### Engine reports owed

Each is drafted in the ledger, not here:

- No headless or recording pad backend (the `GamepadReader` redefinition).
- Public types deleted without a deprecation release, after report 7 closed
  (`game/carried/` is the exhibit).
- `Scene` offers no typed access to short-lived objects or retirement notice.
- Startup failures can only be classified by message text
  (`fatal_report.cpp:113`, `check-startup.ps1:225`).
- `JsonValue` cannot report unread keys, so a misspelled optional key is silent.
- Word wrap: the engine backlog (OCA-77) defers it because "no caller in either
  client wraps by width", but `game/text_wrap.h` does.

## Refuted

- *`CrudeDrawing` uses public inheritance only for helpers.* `TextureObject`'s
  `draw` overloads are protected, so there is no public interface the leaves fail
  to honour.
