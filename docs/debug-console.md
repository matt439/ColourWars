# Developer console (Debug only)

A Quake-style console, a pad- and keyboard-driven debug menu, launch arguments,
F-key toggles, overlays, player cheats and screenshots for reviewing the game
without walking the menus. It exists only in **Debug** builds (`x64-debug`);
Release compiles out the console, menu, registry, commands and binds (see
[Release](#release-behaviour)).

## Start a review from the command line

```powershell
out/build/x64-debug/game/ColourWars.exe +stage movement_course +players 4 +visual posed
```

Every argument that starts with `+` and a letter begins a statement; the words
after it are its arguments, so `+timescale +0.5` keeps `+0.5` as a number.
Statements run in order, before the first frame, through the same registry as
the console. The match starts directly, without menu navigation, and you are
returned to the real main menu on `menu` or when the match ends normally.

Useful combinations:

```powershell
# Movement Course, one keyboard/mouse player, the posed workshop art
ColourWars.exe +stage movement_course +visual posed

# Four panes on a machine with no pad: seat 1 is keyboard/mouse, the rest idle
ColourWars.exe +stage movement_course +players 4 +visual rotating

# Feedback off from the first frame, everyone on the roller
ColourWars.exe +stage movement_course +players 2 +weapon all roller +feedback off

# Straight to the high step, collision and camera overlays on, no damage
ColourWars.exe +stage movement_course +goto high_step +overlay collision +overlay camera +god 1

# Half speed, with a transcript of everything the console prints
ColourWars.exe +logfile C:/temp/cw.log +stage movement_course +timescale 0.5
```

A bad statement prints an error and the others still run; a bad `+stage` is an
error, not a crash, and the game then starts at the title as usual.

## Keys

During a match, without opening the console:

| Key | Does |
|---|---|
| **F2** | `overlay collision` - collision bounds per pane |
| **F3** | `overlay camera` - each pane's scroll border, camera bounds and followed centre |
| **F4** | `overlay paint` - paint coverage per team and seat, bottom left |
| **F5** | `restart` - rebuild the match with its current settings (keeps the session) |
| **F6** | `pause` - pause or resume the match clock (a `PAUSED` strip shows bottom right) |
| **F7** | `step` - pause, then advance exactly one update |
| **F8** | `visual next` - cycle original -> rotating -> posed; rebuilds the Movement Course |
| **F9** | `feedback toggle` - movement feedback bursts on/off, immediately |

Anywhere (a match or any menu page):

| Key | Does |
|---|---|
| **F10** or **F11** | open / close the [debug menu](#debug-menu) |
| hold pad **Back+Start** for 0.5 s | open / close the debug menu from any pad |
| **`** | open / close the console |

F1 (per-player debug overlay), Escape (pause menu) and the gameplay keys are
unchanged. A small strip in the bottom-right corner names any non-default state
(`x0.25`, `PAUSED`, `visual posed`, `feedback off`, `GOD`, `AMMO INF`,
`ovl collision camera paint`).

**The chord and its buttons.** Back alone still toggles a player's F1 overlay
and Start alone still pauses. Once a pad has both down, both buttons belong to
the chord until both are up again: their releases neither toggle the overlay
nor pause the match or a menu, and that holds for a chord let go before
0.5 s. On a menu page only the chording pad's presses are forgotten; the
keyboard and other pads keep theirs.

**F10 is normally the system's.** The engine hands F10 and a lone Alt to
`DefWindowProc`, which opens the window's system menu and freezes the game
until the next key. Debug builds take F10 back with a thread message hook
(engine report 11); F11 does the same without one. A lone Alt tap still enters
the system menu: press Escape to leave it.

In the console:

| Key | Does |
|---|---|
| **`** (the key left of 1, by scan code) | open / close; its character never enters the line, even when held |
| **Escape** | close (on release, so the match does not also open its pause menu) |
| **Enter** | run the line (several statements may be separated by `;`) |
| **Up / Down** | browse this session's history (kept in memory only) |
| **Tab** | complete the command name, a variable, or a choice argument (including `goto` stations) |
| **Left / Right / Home / End**, **Backspace / Delete** | edit; Backspace, Delete and arrows repeat when held |
| **Ctrl+U** | clear the line |

Ctrl chords never type, but AltGr (Ctrl+Alt) does, so `\ @ [ ] { } | ~` work
on German, French, Nordic and similar layouts. The toggle key is US/UK-oriented:
on layouts where the key left of 1 is a dead key (German `^`), Windows holds the
dead key and combines it with the next key typed, so the first character after
opening the console can come out as `^x` or be lost. Retype it.

The console is a pushed state drawn semi-transparently over the top of the
window, across every pane. **The match is frozen while it is open** - only the
top of the state stack updates, which is also what keeps your typing out of the
game. Use the F-keys above for live A/B toggling during play.

## Commands

| Command | Does |
|---|---|
| `stage <name\|next\|prev\|random>` | start that stage (from a menu) or rebuild the match on it |
| `players <1-4>` | seats: connected pads in slot order, then keyboard/mouse, then idle dummies. With no pad, seat 1 is keyboard/mouse. In a menu it is stored for `stage`; a match started from the menus instead drops it (logged), as does a match ending with it still queued |
| `weapon <seat\|all> <sprayer\|sniper\|roller\|mister\|bucket\|random>` | a seat's weapon (seats are 1-4) |
| `visual <original\|rotating\|posed\|next\|prev>` | the Movement Course art comparison (variable; `continuous`/`authored` also accepted). Off the course it is remembered, not applied |
| `feedback <on\|off\|toggle>` | movement feedback (variable; `1`/`0` too). Immediate |
| `overlay [player\|collision\|camera\|paint\|all\|none]` | toggle one [overlay](#overlays), or all on/off; alone, list them |
| `overlay.player`, `overlay.collision`, `overlay.camera`, `overlay.paint` `<on\|off\|toggle>` | the overlays as variables |
| `goto <station> [seat]` | put a seat (default 1) at a named [station](#stations-and-goto), at rest |
| `respawn [seat\|all]` | respawn a seat or every seat now (default all) |
| `god <0\|1>` | no player takes paint damage (variable) |
| `ammo <normal\|inf>` | `inf` keeps every tank full (variable; `infinite` too) |
| `restart` | rebuild the current match, same session |
| `menu` | leave the match for the main menu |
| `timescale [0.25-4\|next\|prev]` | match speed (variable). Updates are stepped at the fixed 1/60 s (0.25 = one update every fourth frame, 2 = two per frame), never stretched; audio is not scaled. `next`/`prev` walk 0.25, 0.5, 1, 2, 4. No argument prints it |
| `pause [on\|off\|toggle]` | pause or resume the match clock |
| `step [n]` | pause, then run n single updates (1-600), one per frame; closes the console so they run |
| `reload` | re-read `tuning.json` and `presentation.json`, from the source tree `game/content/` when this Debug build was made from one, otherwise the staged copy; validated before anything changes |
| `screenshot [name]` | save the window as a PNG (see [Screenshots](#screenshots)) |
| `status` | stack depth, stage, seats (device, team, weapon, position, move state), clock, feedback, overlays, cheats, the stage's stations, anything queued |
| `set <var> <value>`, `get <var>`, `list [prefix]`, `help [name]` | the registry; `feedback off` and `set feedback off` are the same |
| `logfile <path\|off>` | mirror every console line to a UTF-8 file |
| `clear` | clear the console output |
| `quit` | close the game |

Commands that rebuild (stage, players, weapon, visual, restart, reload in a
match) close the console and apply at the top of the match's next update, when
no world update is running. Commands that act on the live match (goto,
respawn, the overlays and cheats) apply at once, between world updates - except
while a rebuild is queued, since it puts every player back at a spawn: `goto`
then waits and applies after that rebuild (checked against the stage it builds),
and `respawn` is refused with the reason.

**Idle dummies** take neutral input, never open the disconnected-pad pause menu,
and paint nothing; their HUD says "Waiting to join". They are for reviewing
2-4 panes, not opponents. Choosing *change teams/weapons* on the end page after
a dummy match returns to setup pages that wait for those seats; back out to the
player count.

**Reload** validates the new files with the same checks startup runs (resource
names, every stage's objects, stations and sounds, fonts). On an error it prints
the reason and keeps the current values. In a match it then restarts the match
with the new values; in a menu it says the values apply to the next match, and
they are swapped in before that match is built. Reloading replaces a live
`feedback` toggle with the file's value, and says so. Rebuilding with new tuning
can still fail during play (for example a player knocked out of bounds); the
console only guarantees that the *commands* themselves never throw into the game.

## Unwinding

A request queued while a pause, controls, results or end page is over the match
does not wait for you to close it. At the top of that page's next update the
page is closed the way its own controls would close it, one page a frame, until
the request can apply:

- **Pause menu** (initial or confirmation page): Resume, then the match
  rebuilds. For `menu`, Quit.
- **Controls / audio** (over pause or over Options): closed like Back.
- **Results**: proceeds as a confirm would, which opens the end menu.
- **End menu**: Restart, and that restart's build *adopts* the queued plan, so
  the level is built once, with the queued settings. Like the page's own Restart
  it also moves the series on (the next round, or a new session once the series
  is complete) unless the plan itself starts a new session (a new stage or
  roster). For `menu`, Exit to Main Menu.

The pause menu's own Restart adopts a queued plan the same way. In the menus, a
queued `stage` starts from any main-menu page; a page that cannot start a match
(Controls over Options) is closed first.

## Debug menu

**F10**, **F11** or holding pad **Back+Start** opens it over whatever is on top,
match or menu page, and the same again closes it from anywhere. It is the
registry as a menu page: the same entries as the console, grouped.

| Category | Entries |
|---|---|
| match | menu, players, restart, stage, visual, weapon |
| player | ammo, god, goto, respawn |
| view | feedback, overlay, overlay.camera, overlay.collision, overlay.paint, overlay.player |
| time | pause, step, timescale |
| content | reload, screenshot |
| console | quit, status |

- **Up/Down** move; **A/Enter** selects; **B/Escape** goes back a level
  (picker -> entries -> categories) and closes at the top; **Left/Right** step a
  variable (`toggle` for a bool, `next`/`prev` otherwise).
- A variable row shows `name  value`. Selecting a bool or a number steps it; a
  choice (`visual`, `ammo`) opens a picker on its current value.
- `stage`, `players`, `weapon` (for every seat), `overlay` and `goto` open a
  picker over their choices. Lists longer than twelve rows end with a note to
  use the console.
- Every action runs as a console line and is echoed `[menu] <line>`, so the
  log and a `logfile` transcript say what the menu did. The last three log lines
  show at the bottom of the menu, so a refused `goto` says why.

**Staging.** `stage`, `players`, `weapon` and `visual` only queue: the menu
stays open, so several can be combined, and the queue shows under the title.
Backing out to the top lands on **Apply: ...**, which closes the menu; the
queued request then applies through the normal path, once (in a match, one
rebuild; in the menus, `Apply: start ...` starts the match, and `Apply: pick a
stage` is disabled until a stage is queued). **Discard queued** drops it. Any
other command that would close the console (restart, step, goto, respawn,
screenshot, menu, quit) closes the menu.

The match is frozen while the menu is open, as under the console. The console
can be opened over the menu; anything it queues waits until the menu closes.

## Overlays

| Overlay | Shows |
|---|---|
| `overlay.player` | every player's per-player debug text, as F1 shows one. Setting it turns every player's text on or off; F1 and pad Back still toggle single players afterwards, and the other overlays leave those toggles alone. It does not track them, so it can read `off` while a player's own text shows |
| `overlay.collision` | per pane: the collision shape of every object in view (structures white, paintables green, jump-throughs cyan, ramps orange with their slope, players in their team colour, projectiles yellow) and the out-of-bounds edge in red |
| `overlay.camera` | per pane: the camera's scroll border (magenta; the player moves inside it before the camera follows), the camera bounds (blue) and a cross at the followed centre |
| `overlay.paint` | bottom left: paint coverage per team and per seat, as a share of the paintable area, live. When a narrow window cannot hold it and the bottom-right status strip on one row, the strip moves above it |

They are set from the console, the debug menu, launch arguments (`+overlay all`)
or F2-F4, survive restarts and rebuilds, and apply to the next match when set in
the menus. The pane overlays are drawn in world space under the HUD; the paint
panel stays visible while the console is open.

## Stations and goto

A level may name places a developer can jump to: an optional `"stations"` array
in its JSON, each `{ "name", "x", "y" }`, the character's top-left like a spawn.
Every level also has `spawn_a` and `spawn_b` (each team's first spawn). Station
names are lower-case letters, digits and `_`, unique, not `spawn_a`/`spawn_b`,
and inside the level's out-of-bounds rectangle; a bad one fails at startup like
any other level content bug.

The Movement Course names eight, each with its feet 5 units above its surface:
`drop_through`, `short_gap`, `long_gap`, `running_gap` (680 units of run-up),
`low_ceiling`, `steps`, `high_step`, `recovery_lane`.

`goto <station> [seat]` puts the seat (1-4, default 1) there at rest: velocity
zero, airborne for one tick so it settles onto the surface, jump input and
movement feedback cleared, a dead player respawned first. The weapon and the
camera follow at once, so it shows even with the clock paused. Tab completes
from the stations of a concrete queued `stage` if there is one, else the current
stage's; `status` lists them, and the debug menu's `goto` picker offers them.
Typed before a match (launch arguments, or the menus with `stage` queued), it
applies at that match's first build; a match started by walking the menus drops
it, with a log line. Typed in a match with a rebuild queued, it stays queued
(the debug menu stays open) and applies after that rebuild; it is dropped, with
a log line, if the rebuild cannot be planned or the match ends first.

## Player cheats (respawn, god, ammo)

- `respawn [seat|all]` respawns now, at the spawn, health and ammo full, as the
  respawn timer would.
- `god 1`: players take no paint damage. The projectile side is unchanged, so
  paint still lands where it hits.
- `ammo inf`: every tank is refilled before and after each shot, so the HUD
  stays full.

Both cheats are kept by the director and re-applied to every build.

## Screenshots

`screenshot [name]` saves the window as a PNG in
`%LOCALAPPDATA%\ColourWars\screens\` (beside the save; without `LOCALAPPDATA`,
`save\screens` beside the executable). The default name is
`colourwars-YYYYMMDD-HHMMSS-mmm.png`; a given name keeps `A-Z a-z 0-9 _ -` and
replaces anything else with `_`. The console (or the debug menu) closes first,
and the shot is taken at the top of the next update once nothing queued would
change the picture, so `+stage movement_course +screenshot start` captures the
match, not the title. The status strip and overlays are included; the log says
`screenshot: <path> (WxH)`.

It re-renders the stack into an unpresented frame and reads it back, because the
engine offers no way to read the frame it presents (engine report 10).

## Movement Course review recipes

```powershell
ColourWars.exe +stage movement_course +visual posed
```

- **Feedback on/off.** Run the route; press **F9** and run it again. The bursts
  stop immediately; nothing else in the simulation changes (the acceptance
  probes check that on/off simulations are identical).
- **Visual A/B.** Press **F8** to rebuild with the next art (original ->
  rotating -> posed). The rebuild resets the round, so compare the same route.
- **One obstacle, many times.** `goto running_gap`, run, and `goto running_gap`
  again; or stage it from launch with `+goto`.
- **Frame by frame.** **F6** to pause at the moment you care about, **F7** to
  step one update at a time, **F6** to resume. Or `timescale 0.25` in the
  console for slow motion; `timescale 1` to return.
- **Four panes, one keyboard.** `+players 4` gives P1 the keyboard and three
  idle dummies, so the split-screen framing and badges can be reviewed alone;
  **F3** shows each pane's camera border.
- **Tuning pass.** Edit `game/content/tuning.json`, then `reload` in the
  console: the match restarts with the new values, no rebuild needed.
- **Record what you saw.** `+logfile` (or `logfile <path>`) and `status` write
  positions and move states to a file; `screenshot` keeps the picture.

## Release behaviour

Release builds (`x64-release`, and therefore the ZIP) contain no console, debug
menu, registry, commands, overlay drawing, screenshot code or key binds: the gate
`COLOURWARS_DEV_TOOLS` is derived from `_DEBUG` in `game/debug/dev_tools.h`,
never from a project define, so the acceptance probes see the same class layouts
as the game. In Release:

- `+` arguments are ignored; if any are present, one line goes to stderr:
  `developer arguments ignored: Release build`. Without them nothing is printed.
- The grave key, F2-F9, F11 and the pad chord do nothing (pad Back still
  toggles F1, Start still pauses); the clock always runs one update a frame.
  F10, like a lone Alt, still enters the system menu loop until the next key,
  because the hook that claims it is Debug-only ([engine report
  11](review/engine-reports.md)).
- What does ship, by design, so no class layout depends on `_DEBUG`: the
  `DebugMatchTarget` and `DebugPageTarget` interfaces (their RTTI names and
  vtables), `GameLevel`'s small `debug_*` adapters and the pages'
  `debug_start_match`/`debug_dismiss`/`debug_reset_input`, all unreachable;
  `Level`'s overlay member and its developer setters, `Player`'s two cheat flags
  and `place_at`/`force_respawn`, never called; `LevelLoadedInfo::stations()`
  and its validation, which are level data and correct in both;
  `directory_consts::screenshot_directory()`; `DebugDirector` itself, one null
  pointer with inline no-op hooks; and the one stderr line above.
- `scripts/acceptance/debug_console.cpp` checks both ways: the Debug executable
  must contain the RTTI names `.?AVDebugConsoleState@@`, `.?AVDebugRegistry@@`,
  `.?AVLambdaCommand@@`, `.?AVDebugMenuPage@@` and `.?AVNumberVariable@@`, and
  the Release executable must contain none of them. CI also checks the packaged
  `ColourWars.exe`.

## For developers

- Code: `game/debug/`. The director and its Debug-only implementation
  (`debug_director.*`, `debug_director_impl.h`); the registry, variables and
  commands (`debug_registry.*`, `debug_variable.*`, `debug_command.*`); the
  commands themselves (`game_commands.cpp` for match, time, content and
  console, `view_commands.cpp` for the overlays, `player_commands.cpp` for
  goto, respawn, god and ammo); the clock, the match planner, the text field
  and the console state; the pad chord (`debug_chord.*`); the debug menu
  (`debug_menu_model.*`, CPU-testable, and `debug_menu_page.*`, a `MenuPage`);
  the pane overlays (`debug_overlay_draw.*`); the screenshot
  (`debug_screenshot.*`).
- Menu metadata is declared where each entry is registered
  (`DebugMenuTraits`: category, form - hidden, run, pick or variable - the
  picked argument and its prefix, and whether it is staged).
- Hooks:
  - `GameLevel::update` calls `before_match_update` (debug menu toggle,
    console key, pending screenshot, F-key binds, queued requests), then
    `filter_match_inputs` on the inputs it read, then `update_match`.
  - `GameLevel::build_and_enter_level`, the one build path (first build, pause-
    and end-menu restarts, debug rebuilds), calls `before_level_build(match)`
    with no level alive - it applies reloaded content, adopts a plan queued
    under a menu's own restart, and consumes a queued restart - and
    `after_level_build(match)` once the level exists, which re-applies overlays
    and cheats, applies a goto stored before the match, and resets the clock's
    partial step.
  - Every `MenuPage` runs `before_page_update(page, dt)` before its
    `update_page`; the page is a `DebugPageTarget`, which is how the director
    starts a match, dismisses a page or resets its input.
- Tests: `scripts/acceptance/debug_console.cpp` (tokenizer and launch grammar,
  registry and completion, clock, planner, text field, director, the match
  hooks against a fake `DebugMatchTarget`, the chord and input filter, menu
  traits, the debug menu model, unwinding against a fake `DebugPageTarget`,
  overlays and cheats, goto, the screenshot name and PNG writer, and the
  course's stations against its geometry; CPU only). In Release it checks the
  one stderr line for `+` arguments, silence without, and that the chord and
  unwinding are inert. `scripts/check-debug-console.ps1` is an opt-in local
  check that drives the real Debug executable with key events (F10 debug menu,
  staging and Apply, overlay keys, goto, cheats, the screenshot command, pause
  menu unwinding) and saves screenshots. The pad chord needs a pad and is
  covered on the CPU side only.
- The engine reports this forced are 9, 10 and 11 in
  [engine-reports.md](review/engine-reports.md).

## Still to come (phase 3)

- live `tune <path> <value>` and `tune.save`
- bots for idle seats
- `record` / `play` of input
- a see-through console that tunes a running match (blocked on engine report 9)
