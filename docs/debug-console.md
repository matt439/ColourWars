# Developer console (Debug only)

A Quake-style console, launch arguments and a few F-key toggles for reviewing
the game without walking the menus. It exists only in **Debug** builds
(`x64-debug`); Release compiles out the console, registry, commands and binds
(see [Release](#release-behaviour)).
Phase 1 covers what a Movement Course review needs: jump straight into a stage
with 1-4 seats, A/B the workshop art and the movement feedback, slow or step
time, reload tuning, and read back what is running.

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

# Half speed, with a transcript of everything the console prints
ColourWars.exe +logfile C:/temp/cw.log +stage movement_course +timescale 0.5
```

A bad statement prints an error and the others still run; a bad `+stage` is an
error, not a crash, and the game then starts at the title as usual.

## Keys

During a match, without opening the console:

| Key | Does |
|---|---|
| **F5** | `restart` - rebuild the match with its current settings (keeps the session) |
| **F6** | `pause` - pause or resume the match clock (a `PAUSED` strip shows bottom right) |
| **F7** | `step` - pause, then advance exactly one update |
| **F8** | `visual next` - cycle original -> rotating -> posed; rebuilds the Movement Course |
| **F9** | `feedback toggle` - movement feedback bursts on/off, immediately |

F1 (per-player debug overlay), Escape (pause menu) and the gameplay keys are
unchanged. A small strip in the bottom-right corner names any non-default state
(`x0.25`, `PAUSED`, `visual posed`, `feedback off`).

In the console:

| Key | Does |
|---|---|
| **`** (the key left of 1, by scan code) | open / close; its character never enters the line, even when held |
| **Escape** | close (on release, so the match does not also open its pause menu) |
| **Enter** | run the line (several statements may be separated by `;`) |
| **Up / Down** | browse this session's history (kept in memory only) |
| **Tab** | complete the command name, a variable, or a choice argument |
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
| `restart` | rebuild the current match, same session |
| `menu` | leave the match for the main menu |
| `timescale [0.25-4]` | match speed. Updates are stepped at the fixed 1/60 s (0.25 = one update every fourth frame, 2 = two per frame), never stretched; audio is not scaled. No argument prints it |
| `pause [on\|off\|toggle]` | pause or resume the match clock |
| `step [n]` | pause, then run n single updates (1-600), one per frame; closes the console so they run |
| `reload` | re-read `tuning.json` and `presentation.json`, from the source tree `game/content/` when this Debug build was made from one, otherwise the staged copy; validated before anything changes |
| `status` | stack depth, stage, seats (device, team, weapon, position, move state), clock, feedback, anything queued |
| `set <var> <value>`, `get <var>`, `list [prefix]`, `help [name]` | the registry; `feedback off` and `set feedback off` are the same |
| `logfile <path\|off>` | mirror every console line to a UTF-8 file |
| `clear` | clear the console output |
| `quit` | close the game |

Commands that rebuild (stage, players, weapon, visual, restart, reload in a
match) close the console and apply at the top of the match's next update, when
no world update is running. If a pause, results or end page is over the match,
they wait until the match is on top again.

**Idle dummies** take neutral input, never open the disconnected-pad pause menu,
and paint nothing; their HUD says "Waiting to join". They are for reviewing
2-4 panes, not opponents. Choosing *change teams/weapons* on the end page after
a dummy match returns to setup pages that wait for those seats; back out to the
player count.

**Reload** validates the new files with the same checks startup runs (resource
names, every stage's objects and sounds, fonts). On an error it prints the
reason and keeps the current values. In a match it then restarts the match with
the new values; in a menu it says the values apply to the next match, and they
are swapped in before that match is built. Reloading replaces a live `feedback`
toggle with the file's value, and says so. Rebuilding with new tuning can still
fail during play (for example a player knocked out of bounds); the console only
guarantees that the *commands* themselves never throw into the game.

## Movement Course review recipes

```powershell
ColourWars.exe +stage movement_course +visual posed
```

- **Feedback on/off.** Run the route; press **F9** and run it again. The bursts
  stop immediately; nothing else in the simulation changes (the acceptance
  probes check that on/off simulations are identical).
- **Visual A/B.** Press **F8** to rebuild with the next art (original ->
  rotating -> posed). The rebuild resets the round, so compare the same route.
- **Frame by frame.** **F6** to pause at the moment you care about, **F7** to
  step one update at a time, **F6** to resume. Or `timescale 0.25` in the
  console for slow motion; `timescale 1` to return.
- **Four panes, one keyboard.** `+players 4` gives P1 the keyboard and three
  idle dummies, so the split-screen framing and badges can be reviewed alone.
- **Tuning pass.** Edit `game/content/tuning.json`, then `reload` in the
  console: the match restarts with the new values, no rebuild needed.
- **Record what you saw.** `+logfile` (or `logfile <path>`) and `status` write
  positions and move states to a file.

## Release behaviour

Release builds (`x64-release`, and therefore the ZIP) contain no console,
registry, commands or key binds: the gate `COLOURWARS_DEV_TOOLS` is derived from
`_DEBUG` in `game/debug/dev_tools.h`, never from a project define, so the
acceptance probes see the same class layouts as the game. In Release:

- `+` arguments are ignored; if any are present, one line goes to stderr:
  `developer arguments ignored: Release build`. Without them nothing is printed.
- The grave key and F5-F9 do nothing; the clock always runs one update a frame.
- What does ship, by design, so no class layout depends on `_DEBUG`: the
  `DebugMatchTarget` interface (its RTTI name and vtable), `GameLevel`'s seven
  small `debug_*` adapters and `MainMenuPage::debug_start_match`, all
  unreachable; `DebugDirector` itself, one null pointer with inline no-op hooks;
  and the one stderr line above.
- `scripts/acceptance/debug_console.cpp` checks both ways: the Debug executable
  must contain the RTTI names `.?AVDebugConsoleState@@`, `.?AVDebugRegistry@@`
  and `.?AVLambdaCommand@@`, and the Release executable must contain none of
  them. CI also checks the packaged `ColourWars.exe`.

## For developers

- Code: `game/debug/` (the director and its Debug-only implementation, the
  registry, variables and commands, the clock, the match planner, the text field
  and the console state). The phase-1 commands are in `game_commands.cpp`.
- Hooks: `GameLevel::update` calls `before_match_update` and `update_match`;
  `GameLevel::build_and_enter_level`, the one build path (first build, pause- and
  end-menu restarts, debug rebuilds), calls `before_level_build` with no level
  alive, which applies reloaded content and consumes a queued restart; every
  `MenuPage` runs `before_page_update` before its `update_page`.
- Tests: `scripts/acceptance/debug_console.cpp` (tokenizer and launch grammar,
  registry and completion, clock, planner, text field, director, and the
  director's match hooks against a fake `DebugMatchTarget`; CPU only). In
  Release it checks the one stderr line for `+` arguments and silence without.
  `scripts/check-debug-console.ps1` is an opt-in local check that drives the real
  Debug executable with key events and saves screenshots.
- The engine report this forced is report 9 in
  [engine-reports.md](review/engine-reports.md).
