# Developer console (Debug only)

A Quake-style console, a pad- and keyboard-driven debug menu, launch arguments,
F-key toggles, overlays, player cheats and screenshots for reviewing the game
without walking the menus; [live tuning](#live-tuning) of `tuning.json` and
`presentation.json` with a save back to the source files; scripted
[bots](#bots) for the seats nobody is holding; and a [seed, record and
play](#seed-record-and-play) loop that replays a match bit for bit; and
[scripts](#scripts), rebindable [binds](#binds), a [live
console](#live-console) and a [perf overlay](#perf), so that **a repeatable
review is one command**:

```powershell
out/build/x64-debug/game/ColourWars.exe +exec review_feedback_ab +quit
```

It exists only in **Debug** builds (`x64-debug`); Release compiles out the
console, menu, registry, commands, binds, scripts, bots, recorder and tuner (see
[Release](#release-behaviour)).

This page is the reference: what the console does now. How each `DBG` row
got there, and the deferred and unscoped rows, are in
[the work record](debug-console-record.md).

## Start a review from the command line

```powershell
out/build/x64-debug/game/ColourWars.exe +stage movement_course +players 4
```

Every argument that starts with `+` and a letter begins a statement; the words
after it are its arguments, so `+timescale +0.5` keeps `+0.5` as a number.
Statements run in order, before the first frame, through the same registry as
the console. The match starts directly, without menu navigation, and you are
returned to the real main menu on `menu` or when the match ends normally. The
launch arguments are themselves a [script](#scripts): `+wait match`,
`+exec <name>` and `+screenshot` hold the statements after them until they are
done, so `+stage movement_course +wait match +screenshot start +quit` works.

Useful combinations:

```powershell
# Movement Course, one keyboard/mouse player
ColourWars.exe +stage movement_course

# Four panes on a machine with no pad: seat 1 is keyboard/mouse, the rest idle
ColourWars.exe +stage movement_course +players 4

# Feedback off from the first frame, everyone on the roller
ColourWars.exe +stage movement_course +players 2 +weapon all roller +feedback off

# Straight to the high step, collision and camera overlays on, no damage
ColourWars.exe +stage movement_course +goto high_step +overlay collision +overlay camera +god 1

# Half speed, with a transcript of everything the console prints
ColourWars.exe +logfile C:/temp/cw.log +stage movement_course +timescale 0.5

# A four-player match on a machine with no pad: you on seat 1, paint bots on the rest
ColourWars.exe +stage movement_course +players 4 +bot all paint

# The same match every time (seeded bots and palette), ready to record
ColourWars.exe +seed 7 +stage movement_course +players 4 +bot all paint

# You and a Hard shipping bot against an Easy bot on Boost +2 and a Normal bot
ColourWars.exe +stage crossfeed +players 4 +seat 2 bot easy +seat 2 boost 2 +seat 3 bot normal +seat 3 team b +seat 4 bot hard +seat 4 team a

# A shipped review, start to finish, then exit (see Scripts)
ColourWars.exe +exec station_tour +quit
```

`players`, `seat` and `bot` need `stage` beside them: on their own they are stored for a
`stage` that never comes, and a match started by walking the menus drops them.

A bad statement prints an error and the others still run; a bad `+stage` is an
error, not a crash, and the game then starts at the title as usual.

## Keys

During a match, without opening the console - these are the default
[binds](#binds), and `bind` changes them:

| Key | Does |
|---|---|
| **F2** | `overlay collision` - collision bounds per pane |
| **F3** | `overlay camera` - each pane's scroll border, camera bounds and followed centre |
| **F4** | `overlay paint` - paint coverage per team and seat, bottom left |
| **F5** | `restart` - rebuild the match with its current settings (keeps the session) |
| **F6** | `pause` - pause or resume the match clock (a `PAUSED` strip shows bottom right) |
| **F7** | `step` - pause, then advance exactly one update |
| **F9** | `feedback toggle` - movement feedback bursts on/off, immediately |

Anywhere (a match or any menu page):

| Key | Does |
|---|---|
| **F10** or **F11** | open / close the [debug menu](#debug-menu) |
| hold pad **Back+Start** for 0.5 s | open / close the debug menu from any pad |
| **`** | open / close the console |

F1 (per-player debug overlay), Escape (pause menu) and the gameplay keys are
unchanged. A small strip in the bottom-right corner names any non-default state
(`x0.25`, `PAUSED`, `feedback off`, `GOD`, `AMMO INF`,
`BOTS 3x paint`, `REC drive 240`, `PLAY drive 240/600`, `PLAY drive done
(MATCH)`, `TUNED 2`, `ovl collision camera paint`, and `SCRIPT
review.cfg:14 wait 30 updates (12 left)` while a script waits). It keeps clear
of the paint and perf panels and, during the countdown, of the opening cards.

**The chord and its buttons.** Back alone still toggles a player's F1 overlay
and Start alone still pauses. Once a pad has both down, both buttons belong to
the chord until both are up again: their releases neither toggle the overlay
nor pause the match or a menu, and that holds for a chord let go before
0.5 s. On a menu page only the chording pad's presses are forgotten; the
keyboard and other pads keep theirs.

**F10 is an ordinary key.** The engine's window treats F10 and a lone Alt as
keys and nothing else, in every build, so neither opens the system menu or
freezes the game; F10 and F11 are both read from the `Keyboard`. Alt+Space
still opens the system menu, and Alt+F4 still closes the window.

In the console:

| Key | Does |
|---|---|
| **`** (the key left of 1, by scan code) | open / close; its character never enters the line, even when held |
| **Escape** | close (on release, so the match does not also open its pause menu) |
| **Enter** | run the line (several statements may be separated by `;`) |
| **Up / Down** | browse the history: the last 64 lines, kept across sessions in `history.txt` (see [History](#history)) |
| **Tab** | complete the command name, a variable, or a choice argument (including `goto` stations) |
| **Left / Right / Home / End**, **Backspace / Delete** | edit; Backspace, Delete and arrows repeat when held |
| **Ctrl+U** | clear the line |

Ctrl chords never type, but AltGr (Ctrl+Alt) does, so `\ @ [ ] { } | ~` work
on German, French, Nordic and similar layouts. The toggle key is US/UK-oriented:
on layouts where the key left of 1 is a dead key (German `^`), Windows holds the
dead key and combines it with the next key typed, so the first character after
opening the console can come out as `^x` or be lost. Retype it.

The console is a pushed state drawn semi-transparently over the top of the
window, across every pane. **By default the match is frozen while it is open** -
only the top of the state stack updates, which is also what keeps your typing
out of the game. Use the F-keys above for live A/B toggling during play, or
`console.live 1` for a [live console](#live-console) that leaves the match
running.

## Commands

| Command | Does |
|---|---|
| `stage <name\|next\|prev\|random>` | start that stage (from a menu) or rebuild the match on it |
| `players <1-4> [bot]` | seats: connected pads in slot order, then keyboard/mouse, then idle dummies. With no pad, seat 1 is keyboard/mouse. A second word is `bot all <bot>` as well (`players 4 paint`). In a menu it is stored for `stage`; a match started from the menus instead drops it (logged), as does a match ending with it still queued |
| `weapon <seat\|all> <sprayer\|sniper\|roller\|mister\|bucket\|random>` | a seat's weapon (seats are 1-4) |
| `seat <1-4> team <a\|b>`, `seat <2-4> bot <easy\|normal\|hard>`, `seat <2-4> human`, `seat <1-4> boost <0\|1\|2>` | what the shipping setup chooses per seat: the team, a [shipping bot](debug-console-record.md#roster-statements-for-bots-teams-and-boost-dbg17) at a difficulty (the seat's controller, with no device or pane), a bot handed back to a human, and Boost. Queued like `weapon`, applied after `players` in typed order; seat 1 stays human. Console only, not in the debug menu |
| `intro <auto\|full\|quick\|off>` | how the next match begins: `auto` is the game's choice (the whole intro on a match's first build, the quick one from the colour preview on a rematch), `off` starts at the split-screen countdown. `record` and `play`, and any one-player match, always build without one (variable) |
| `feedback <on\|off\|toggle>` | movement feedback (variable; `1`/`0` too). Immediate |
| `overlay [player\|collision\|camera\|paint\|stations\|perf\|all\|none]` | toggle one [overlay](#overlays), or all on/off (`all` leaves `perf` alone, `none` clears it); alone, list them |
| `overlay.player`, `overlay.collision`, `overlay.camera`, `overlay.paint`, `overlay.stations`, `overlay.perf` `<on\|off\|toggle>` | the overlays as variables |
| `goto <station> [seat]` | put a seat (default 1) at a named [station](#stations-and-goto), at rest |
| `respawn [seat\|all]` | respawn a seat or every seat now (default all) |
| `kill [seat\|all]` | splat a seat or every seat (default all), god or not, to watch the respawn redraw |
| `god <0\|1>` | no player takes paint damage (variable) |
| `ammo <normal\|inf>` | `inf` keeps every tank full (variable; `infinite` too) |
| `restart` | rebuild the current match, same session |
| `menu` | leave the match for the main menu |
| `timescale [0.25-4\|next\|prev]` | match speed (variable). Updates are stepped at the fixed 1/60 s (0.25 = one update every fourth frame, 2 = two per frame), never stretched; audio is not scaled. `next`/`prev` walk 0.25, 0.5, 1, 2, 4. No argument prints it |
| `pause [on\|off\|toggle]` | pause or resume the match clock |
| `step [n]` | pause, then run n single updates (1-600), one per frame; closes the console so they run |
| `reload` | re-read `tuning.json` and `presentation.json`, from the source tree `game/content/` when this Debug build was made from one, otherwise the staged copy; validated before anything changes |
| `screenshot [name]` | save the window as a PNG (see [Screenshots](#screenshots)) |
| `screenshot.dir [folder\|off]`, `screenshot.strip <on\|off>` | a folder under `screens\` for the next shots (`{time}` is the local time), and whether the status strip is in them |
| `exec <name> [args]`, `exec.stop`, `exec.list` | run a [script](#scripts), stop every running script, list the scripts |
| `wait <n>\|<x>s\|match\|frames <n>\|replay\|record`, `echo <text>` | in a script: wait for updates, match seconds, the match, hook frames, a replay or a recording; print a line |
| `bind [key [line]]`, `unbind <key\|all>`, `binds`, `binds.reset` | the [key table](#binds) |
| `console.live <on\|off>` | the [live console](#live-console) (variable) |
| `perf`, `perf.reset`, `perf.capture <frames> <label>` | print/clear the [perf](#perf) numbers (and print the last level build's time) or export a bounded rendered-frame capture |
| `tune <path> [value\|next\|prev\|+=x\|-=x\|on\|off\|toggle] [restart]` | print or set a number, boolean or one of fifty-eight audited presentation colours; colours take a name or `RRGGBB`/`RRGGBBAA` within their field's validation (see [Live tuning](#live-tuning)) |
| `tune.diff [tuning\|presentation]`, `tune.reset [path\|tuning\|presentation\|all]`, `tune.save [tuning\|presentation\|all] [force]`, `tune.find <word>` | list, revert, write back to the source files, search the paths |
| `bot <seat\|all\|off> <idle\|walk\|jump\|chase\|paint\|skirmish\|off>` | scripted input for a seat or every idle dummy; `bot` alone lists (see [Bots](#bots)) |
| `seed [n\|off]` | fix the round seed of every build, the menus' `rand()` and the planner (see [Seed, record and play](#seed-record-and-play)) |
| `record [name\|stop] [updates]`, `play <name\|stop> [force]`, `replays`, `checksum` | record every seat's input from a fresh build, replay it and compare, list recordings, print the running match's state hash |
| `status` | stack depth, stage, seats (device, team, weapon, position, move state), clock, feedback, overlays, cheats, the stage's stations, anything queued |
| `set <var> <value>`, `get <var>`, `list [prefix]`, `help [name]` | the registry; `feedback off` and `set feedback off` are the same |
| `logfile <path\|off>` | mirror every console line to a UTF-8 file |
| `clear` | clear the console output |
| `quit` | close the game |

Commands that rebuild (stage, players, weapon, restart, reload in a
match) close the console and apply at the top of the match's next update, when
no world update is running. Commands that act on the live match (goto,
respawn, the overlays and cheats) apply at once, between world updates - except
while a rebuild is queued, since it puts every player back at a spawn: `goto`
then waits and applies after that rebuild (checked against the stage it builds),
and `respawn` is refused with the reason.

**Idle dummies** take neutral input, never open the disconnected-pad pause menu,
and paint nothing; their HUD says "Waiting to join" ("Bot: paint" when a bot
drives them, "Replay" under a replay). They are for reviewing
2-4 panes, not opponents - unless a [bot](#bots) drives them. Choosing *change teams/weapons* on the end page after
a dummy match returns to setup pages that wait for those seats; back out to the
player count.

**Reload** validates the new files with the same checks startup runs (resource
names, every stage's objects, stations, sounds and anti-tunnelling bound,
fonts). On an error it prints the reason and keeps the current values. In a match it then restarts the match
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
| match | intro, menu, players, restart, stage, weapon |
| player | ammo, bot (picker: `bot all <behaviour>`), god, goto, kill, respawn |
| view | feedback, overlay, overlay.camera, overlay.collision, overlay.paint, overlay.perf, overlay.player, overlay.stations |
| time | pause, step, timescale |
| tuning | the `tuning` and `presentation` trees, then tune.diff, tune.reset, tune.save |
| replay | checksum, play (picker over the recordings), record, replays |
| content | reload, screenshot |
| console | binds, binds.reset, console.live, exec.list, exec.stop, quit, status |

- **Up/Down** move; **A/Enter** selects; **B/Escape** goes back a level
  (picker -> entries -> categories) and closes at the top; **Left/Right** step a
  number or boolean (`toggle` for a bool, `next`/`prev` for a number).
- A variable row shows `name  value`. Selecting a bool or a number steps it; a
  choice (`ammo`) opens a picker on its current value.
- An audited colour row shows its quoted token and `(console)`. **A/Enter**
  prints its current value and adoption scope; **Left/Right** leave it alone.
  Enter a literal colour through `tune` in the console.
- `stage`, `players`, `weapon` (for every seat), `overlay` and `goto` open a
  picker over their choices. Lists longer than twelve rows end with a note to
  use the console.
- Every action runs as a console line and is echoed `[menu] <line>`, so the
  log and a `logfile` transcript say what the menu did. The last three log lines
  show at the bottom of the menu, so a refused `goto` says why.

**Staging.** `stage`, `players` and `weapon` only queue: the menu
stays open, so several can be combined, the queue shows beside the title, and
each says `queued; rebuilds when the debug menu closes`.
Backing out to the top lands on **Apply: ...**, which closes the menu; the
queued request then applies through the normal path, once (in a match, one
rebuild; in the menus, `Apply: start ...` starts the match, and `Apply: pick a
stage` is disabled until a stage is queued). **Discard queued** drops it. Any
other command that would close the console (restart, step, goto, respawn,
screenshot, menu, quit) closes the menu.

The match is frozen while the menu is open, as under the console. The console
can be opened over the menu; anything it queues waits until the menu closes, and
so does a running script. The menu draws below the band where each pane's HUD
names its player, and without the menus' brand line, so it never covers P1's
label.

## Overlays

| Overlay | Shows |
|---|---|
| `overlay.player` | every player's per-player debug text, as F1 shows one. Setting it turns every player's text on or off; F1 and pad Back still toggle single players afterwards, and the other overlays leave those toggles alone. It does not track them, so it can read `off` while a player's own text shows |
| `overlay.collision` | per pane: the collision shape of every object in view (structures white, paintables green, jump-throughs cyan, ramps orange with their slope, players in their team colour, projectiles yellow) and the out-of-bounds edge in red |
| `overlay.camera` | per pane: the camera's scroll border (magenta; the player moves inside it before the camera follows), the camera bounds (blue) and a cross at the followed centre |
| `overlay.paint` | bottom left: paint coverage per team and per seat, as a share of the paintable area, live. When a narrow window cannot hold it and the bottom-right status strip on one row, the strip moves above it |
| `overlay.stations` | in the world: every [station](#stations-and-goto)'s name, the spawns included, above the place `goto` puts a player. The names are `labrador::UiText` objects in the level's scene, made the first time the overlay is turned on and hidden after that by `set_hidden`, which keeps their words, box and colour, so they draw wherever the world is drawn (the intro picture and the overview too) and under structures and players. Font, scale, offset and colour are `presentation.debug_overlay.station_label`; scale, offset and colour tune live. No default key |
| `overlay.perf` | bottom left, above the paint panel: the [perf](#perf) graph and numbers. Not part of `overlay all` |

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

The Movement Course names ten, each with its feet 5 units above its surface:
`drop_through`, `short_gap`, `long_gap`, `running_gap` (680 units of run-up),
`low_ceiling`, `steps`, `high_step`, `recovery_lane`, `workshop` (beside the
left spawn) and `refill` (on the right runway).
Each also carries the lesson untimed solo practice shows there
([polish-movement-course.md](polish-movement-course.md#practice-workshop));
`goto` lands inside it. A developer start (`+stage movement_course`) is the
timed course, so the lessons appear only from the menus' solo practice.

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

## Player cheats (respawn, kill, god, ammo)

- `respawn [seat|all]` respawns now, at the spawn, health and ammo full, as the
  respawn timer would.
- `kill [seat|all]` sets health to zero, through `god` too, so the next world
  update dies as a fatal hit does: the paint-off pop, then the figure redrawn at
  its spawn over the respawn wait. In the countdown it waits for the match to
  start, because players do not update until then.
- `god 1`: players take no paint damage. The projectile side is unchanged, so
  paint still lands where it hits.
- `ammo inf`: every tank is refilled before and after each shot, so the HUD
  stays full.

Both cheats are kept by the director and re-applied to every build. While a
recording or a replay runs, `goto`, `respawn`, `kill`, `god`, `ammo`, `bot` and `tune`
are refused (they would change the match outside the recorded input).

## Live tuning

Every number and boolean in `tuning.json` and `presentation.json`, plus the
fifty-eight audited colour paths below, can be read and changed while the game runs,
by the path its keys give it:
`tuning.player.gravity`, `tuning.weapons.sprayer.offset.x`,
`presentation.movement_feedback.enabled`; an array element is `[i]`. The names
are the files' own keys, scanned from the files, so they cannot drift from what
the game reads (the numeric/boolean acceptance case replaces each value with a
string in turn and requires the game's reader to reject it by name). Tab
completes a path one segment at a time (`tune tuning.pl` -> `tune tuning.player.`), and then the
value (`next prev`, `on off toggle`, or the colour suggestions `WHITE BLACK`).

```text
tune tuning.player.gravity                 -> tuning.player.gravity = 2000.0  [live]
tune tuning.player.gravity 2100            -> tune: tuning.player.gravity 2000.0 -> 2100.0 (live)
tune tuning.player.gravity next            -> one step (2100 -> 2200; see below)
tune tuning.player.gravity -=150           -> relative
tune presentation.movement_feedback.enabled off
tune tuning.paint.thickness 13 restart     -> a rebuild-scoped value, and the restart that shows it
```

**Validation is the game's own.** Each edit is spliced into the file's text and
the real `GameTuning` or `Presentation` reader is run on it, so every range and
cross-field rule startup has applies, with startup's message (`tune: rejected:
... 'jump_buffer_time' is 2, which is outside [0, 1]`). A rejected edit changes
nothing. In the running game every edit also runs the file's own startup checks
against the loaded resources (`validate_resources`): for presentation, that
its named fonts, frames and sounds exist, and `match.countdown_size` leaves
room under the GET READY heading. Edits under
`tuning.paint`, `tuning.player.size` and `tuning.player.max_velocity`, and
of `tuning.player.x_initial_velocity` and `tuning.player.drop_down_velocity`,
also run the startup check that builds every stage's objects, so a speed cap at
which a player could step through a floor is refused
(`game/tunnelling_limits.h`).
Only then is the validated object copied over the live one: the
records that weapons, projectiles, the HUD and movement feedback point at keep
their addresses, and only the edited value changes. A colour edit also keeps
the live F9 feedback toggle.

**When it shows.** Each path has a scope, printed after the edit and marked with
`*` in the debug menu when it is not live:

| Scope | Numeric/boolean paths | Applies |
|---|---|---|
| live | `tuning.input.*`, `tuning.player.*` (not `size`), `tuning.match.sounds.*.volume`, `tuning.weapons.*`, `tuning.projectiles.*` (except sizes and mist `spread_step`), `tuning.match.opening.*`, `tuning.match.reveal.*`, `presentation.movement_feedback/respawn_redraw/opening/debug_overlay/hud/camera.*` | at once |
| new shots | `tuning.projectiles.*.size`, `*.collision_size`, `tuning.projectiles.mist.spread_step` | the next projectile |
| rebuild | `tuning.player.size`, `tuning.match.*`, `tuning.paint.*`, `presentation.match.*`, anything else not listed | the next build: `restart`, or add `restart` to the tune |
| next screen | the rest of `presentation` (menus, pages, prompts) | when that page is next opened |

**Colours.** These fifty-eight existing string fields are supported; numeric path
wildcards do not make other strings editable. Use an engine colour name or
six/eight hexadecimal digits, without a leading `#`. Names use the engine's
case-sensitive spelling, such as `WHITE`, `BLACK` or `LIGHT_GRAY`; hexadecimal
digits accept either case. Tab suggests `WHITE` and `BLACK`; those suggestions
do not restrict the other names accepted by the normal presentation reader.
Colour readouts and diffs
show the quoted JSON token.
The latest validation is recorded under
[remaining live-tuning work](debug-console-record.md#remaining-live-tuning-work-dbg1).

| Colour path | Scope | Consumer |
|---|---|---|
| `presentation.movement_feedback.colour` | live | Existing movement puffs and the fallback tint for step/skid flecks. |
| `presentation.movement_feedback.highlight` | live | Highlights of existing movement puffs and contact flecks. |
| `presentation.movement_feedback.keyline` | live | Outlines of existing movement puffs and contact flecks. |
| `presentation.debug_overlay.colour` | live | Player debug text, console input and ordinary log text. |
| `presentation.debug_overlay.shadow_colour` | live | Player debug text and console/status/perf text shadows. |
| `presentation.debug_overlay.station_label.colour` | live | Existing station labels, refreshed before the match is drawn. |
| `presentation.main_menu.backgrounds.home` | next screen | Home, setup/preferences pages, Credits and confirmations copy this tint into their background widgets when opened. |
| `presentation.main_menu.team_select.selected` | live | Ready players' chosen Team Select icons, on the next active, connected visual refresh. |
| `presentation.main_menu.team_select.unselected` | live | Team Select's other icons, on the next active, connected visual refresh. |
| `presentation.main_menu.weapon_select.selected` | live | Ready players' Weapon Select tiles, on the next active, connected visual refresh. |
| `presentation.main_menu.weapon_select.unselected` | live | Unready players' Weapon Select tiles, on the next active, connected visual refresh. |
| `presentation.weapon_feedback.highlight` | live | Existing dry-fire crosses, recovery rays and all three aim-readiness shapes, on the next draw. |
| `presentation.weapon_feedback.keyline` | live | Outlines of existing muzzle/status cues and aim-readiness shapes, on the next draw. |
| `presentation.player_identity.ink_colour` | live | Borders and P1-P4 glyphs of existing world, HUD, Team Select, Weapon Select and Results badges, on the next draw. |
| `presentation.player_identity.background_colour` | live | Opaque paper behind those same existing identity badges, on the next draw. |
| `presentation.combat_feedback.highlight` | live | Hurt ticks of existing combat cues, on the next draw. |
| `presentation.combat_feedback.keyline` | live | Outlines of existing hurt ticks and detached elimination fragments, on the next draw. |
| `presentation.blocked_shot.highlight` | live | Stop ticks of existing blocked-shot cues, on the next draw. |
| `presentation.blocked_shot.keyline` | live | Outlines of existing blocked-shot stop ticks and rebound beads, on the next draw. |
| `presentation.idle_drip.keyline` | live | Outlines of existing attached beads and falling drops for Sprayer, Sniper and Bucket, on the next draw. |
| `presentation.respawn_redraw.pen_colour` | live | Existing partial respawn outlines, on the next draw. |
| `presentation.opening.cursor_art.ink` | live | Ink of existing cursor arrows and reveal click ticks, on the next draw. |
| `presentation.opening.cursor_art.fill` | live | Fill of those same existing arrows and click ticks, on the next draw. |
| `presentation.opening.silhouette_art.ink` | live | Contours of existing visible intro/reveal silhouettes, on the next draw. |
| `presentation.opening.silhouette_art.rim` | live | Lit edges of those same silhouettes, on the next draw. |
| `presentation.opening.silhouette_art.shade` | live | Ordered shade passes of those same silhouettes, on the next draw. |
| `presentation.opening.reveal_art.ink` | live | Contours, text outlines and click-tick ink of existing visible percentage cards, on the next draw. |
| `presentation.opening.reveal_art.paper` | live | Stepped paper interiors of those same cards, on the next draw. |
| `presentation.opening.reveal_art.shade` | live | Displaced shadows and ordered inner airbrush passes of those same cards, on the next draw. |
| `presentation.opening.cursor_art.shade` | live | Shaded cells of existing cursor arrows, on the next draw. |
| `presentation.opening.cursor_art.shadow` | live | Shifted shadows of those same arrows, on the next draw. |
| `presentation.opening.desktop_colour` | live | Desktop fill of the existing closed program in the winner-none reveal, on the next applicable draw. |
| `presentation.opening.monitor_art.power_light` | live | Power-light inset of existing visible Monitor front details, on the next applicable draw. |
| `presentation.opening.program_colour` | live | Program backing of existing Monitor draws, on the next applicable draw; later composition may cover it. |
| `presentation.opening.acting.face` | live | Eye paint of existing FigureActing overlays, on the next applicable draw; later BLACK pupils remain. |
| `presentation.opening.acting.sweat` | live | Sweat of existing FigureActing cheat overlays, on the next applicable draw. |
| `presentation.opening.glass_colour` | live | Initial glass and palette of existing Monitor draws, plus picker panels and close-button faces, on each applicable draw. |
| `presentation.opening.silhouette` | live | Body mass of existing Silhouette story draws, on the next applicable draw. |
| `presentation.opening.scribble` | live | Three scribble rows per captured sticky note, on the next applicable Monitor front-details draw; absent notes produce no pixels. |
| `presentation.hud.health_colour` | live | Fill of existing HUD HEALTH meters, on the next applicable draw; zero health draws no fill. |
| `presentation.hud.meter_track_colour` | live | Backing tracks of existing HUD HEALTH and PAINT meters, read on every applicable shared meter draw; later opaque fills cover their owned prefixes. |
| `presentation.opening.crack` | live | Thin crack dabs on the next applicable Monitor front-details draw; tint edits preserve captured persona points and do not create or reroll cracks. |
| `presentation.figure.arm_colour` | live | Procedural arms, outer hand squares and quiet blink lids on each applicable FigureAttachments draw; captured rest/sleeve layers retain their existing atlas tints. |
| `presentation.figure.hand_colour` | live | Inner hand squares drawn after the tool and their arm-coloured borders on each applicable FigureAttachments draw. |
| `presentation.figure.quiet.face_colour` | live | Eye paint-out on each applicable quiet blink draw; the later lid retains its independent arm colour, quiet timing and captured resources. |
| `presentation.hud.timer.colour` | live | Normal HUD clock foreground on each applicable draw; the running final-ten-seconds signal uses separate closing-clock text. |
| `presentation.hud.timer.shadow_colour` | live | Offset normal-clock shadow before its independent foreground; closing-clock draws skip both normal text calls. |
| `presentation.hud.closing_clock.text` | live, opaque | Final-ten-positive-seconds clock text on applicable running draws; opaque named/RGB/RGBA-FF values retain the existing reader contract. |
| `presentation.hud.player_identity.colour` | live | Shared foreground of the existing identity label and HEALTH/PAINT meter labels, on each applicable HUD draw; fonts remain captured. |
| `presentation.hud.player_identity.shadow_colour` | live | Offset identity-label shadow drawn before its independent foreground; the meter labels have no shadow call. |
| `presentation.hud.player_identity.background_colour` | next screen | Captured backing behind the Stage Select preview; a newly constructed page adopts it, while the gameplay HUD paper remains separate. |
| `presentation.hud.closing_clock.ink` | live, opaque | Closing-panel ink cross fills and detached beat tick under-strokes on applicable running draws. |
| `presentation.hud.closing_clock.paper` | live, opaque | Inner closing-panel paper cross fills on applicable running draws. |
| `presentation.hud.closing_clock.accent` | live, opaque | Four detached tick stripes while the existing closing beat geometry is active; settled/reduced draws omit them. |
| `presentation.hud.closing_clock.shade` | live, alpha > 0 | Full RGB in both base shadows and six nested inset bands; bands scale only alpha by 0.12 before the existing premultiplication. |
| `presentation.hud.paper.ink` | live, opaque and distinct | Borders of the identity, combined HEALTH/PAINT and ordinary-timer paper panels, plus both direct meter borders, read on each applicable draw. |
| `presentation.hud.paper.paper` | live, opaque and distinct | Inner paper of those same three panel callers, read on each applicable draw. |
| `presentation.hud.paper.shade` | live, alpha > 0 | Full RGB in those panel shadows and inset bands; bands scale only alpha by 0.12 before the existing premultiplication. |

Team and Weapon tile colours retain their last visual refresh while drawing is
frozen, the page is inactive or a claimed device is missing. Weapon body artwork,
paint/supply masks, names, descriptions, fonts and page paper keep their captured
styles; these two Weapon paths tint only the readiness tiles.

Supported colours use ordinary straight RGBA. Translucent-capable drawing paths
premultiply base RGB by its alpha before applying any event fade; a permitted
zero-alpha colour contributes no RGB. Movement, combat, weapon and blocked-shot
feedback apply the event fade once, preserving the existing opaque presentation. Captured menu
backgrounds convert the base colour when their widgets are constructed. Root badge
ink and background retain their existing stricter contract: both must be fully opaque and distinct
from one another. RGB and RGBA ending `FF` are accepted; partial/zero alpha and
a colour matching the current other field are rejected before live content or
the diff changes.

Closing-clock text separately retains its existing opaque-only validation:
opaque names, RGB and RGBA ending `FF` are accepted; partial/zero alpha is
refused before live content or the diff changes.

Shared HUD paper has a separate coupled contract: `hud.paper.ink` and
`hud.paper.paper` must be fully opaque and different decoded RGBA colours.
Names, RGB and RGBA ending `FF` are accepted; partial/zero alpha or a colour
matching the other material are refused atomically. `hud.paper.shade` requires
positive alpha, including `01` through `FF`; it may equal either material.
Resetting one opaque material can also refuse if its saved baseline would
equal the pending sibling. Save and force Save validate the entire merged pair;
force does not bypass these reader rules.

Identity badges read both borrowed colours during every draw, including frozen
world, HUD and menu views. Editing either preserves the other colour, geometry,
roster labels, focus behaviour and emphasis timers; hidden badges remain hidden.
Reset recolours the same existing badges on their next draw.

Weapon-feedback colours are read on every draw, including frozen cues beneath
a paused console. Their team-paint centres, geometry and event timers stay
unchanged; sheet/frame bindings retain their constructor capture.

Combat-feedback highlight and keyline are also borrowed on every draw of an
existing hurt accent or detached elimination burst. Highlight colours the hurt
ticks; keyline colours both hurt and elimination outlines. Editing either
preserves the other tint, opaque team/grey fills, cue timers, detached body
rectangle and culling bounds. Sprite-sheet/fill bindings retain their constructor
capture. Reset recolours those same existing cues on their next draw.

Blocked-shot highlight and keyline are borrowed on every draw of existing cues.
Editing either preserves the other tint, captured team-colour bead interiors,
contact position/normal/owner, cue age, all owner-repeat budgets and bounds.
The base RGBA is premultiplied once before the quadratic lifetime fade; reviewed
opaque defaults retain their pixels. Setting both colours to zero alpha removes
their ticks/outlines while retaining the stored team-colour beads under the
same fade. Sheet/frame capture, cue admission and geometry are unchanged.
The pair's validation is recorded under
[remaining live-tuning work](debug-console-record.md#remaining-live-tuning-work-dbg1).

Idle-drip keyline is borrowed on every attached-bead and falling-drop draw.
The common colour propagates into all five fixed tool records while only
Sprayer, Sniper and Bucket remain enabled and receive Level drip objects.
Editing it preserves those record addresses, tool geometry, bead/drop state,
captured opaque team-paint interiors and captured sheet/frame bindings.
Only the keyline is premultiplied; these drips have no event fade. Nonblack
zero alpha removes the outline while retaining the paint interior. The common
enabled flag and live Reduce effects preference still suppress drawing.

Respawn pen colour is borrowed on every partial-path rectangle submission.
The base RGBA is premultiplied once; progress controls ordered path length,
with no event fade. Existing segment order and repeated blending at overlaps
are preserved, and the opaque BLACK default is unchanged. Editing the colour
does not advance the player's respawn timer or change strokes, geometry,
thresholds or captured sheet/frame bindings. Whole dead-story poses and filled
figures bypass the pen; Reduce effects leaves this essential outline visible.

Cursor ink and fill are borrowed on every draw of the resting arrow and reveal
click ticks. Their existing base-alpha premultiplication is unchanged; click
age changes tick placement/length rather than opacity, with no event fade.
Shade and shadow are separate live materials, also borrowed on each arrow draw.
Setting ink/fill to nonblack zero alpha retains shade/shadow. Setting shade/shadow
to nonblack zero alpha retains ink/fill and click ticks. A partial shade reveals
the shifted shadow below it; each ordered material is premultiplied once.
Reduce effects suppresses the click ticks while retaining the arrow. Editing
either colour preserves the other, fixed hotspot, geometry, timing, draw inputs
and captured sheet/frame; no drawing, default colour or artwork changes here.

Silhouette ink, rim and shade are borrowed on every visible intro/reveal draw.
The separate body fill, persona/pose inputs, geometry and captured sheet/frame
remain unchanged. Existing base-alpha conversion is retained, with no temporal
fade. Each of the four ordered shade passes submits
`(R*A/4, G*A/4, B*A/4, A/4)`; overlapping writes blend and quantize separately.
Making all three art colours nonblack zero alpha removes their contributions
while retaining the separate body fill. This increment changes no drawing,
default colour or artwork.

The separate `opening.silhouette` body scalar is borrowed on the next applicable
story draw. One mass-tint wrapper premultiplies base RGBA once; opaque `0E0E0E`,
art/order/resources and caller persona/pose stay intact. Overlaps retain their
ordered one/two/three-write accumulation and per-write quantization; no union
mask, event fade or Reduce Effects change is introduced. Alpha0 removes only
body contributions, retaining prior ink masses and later rim/shade.

Body qualification is limited to nine literal 3x3 interior patches over the
fixed opaque-black undercoat. Copied full artwork proves adoption only, not
independent full geometry. The historical field26 art proof still fixes body
RGB14 and independently covers only three mug patches; it is unchanged.
Actual-Level body visibility remains unmeasured, with source caller traces only.

Reveal card ink, paper and shade are borrowed on each visible percentage-card
draw. Captured sheet/fill/font bindings and the existing captured percentage-text
policy stay unchanged, as do caller team colours, geometry and timing. Each
inner airbrush pass multiplies shade alpha by 0.12 before premultiplying once.
Click ink and team-paint alpha use the existing quadratic beat before that
conversion; half-age gives beat 0.25. Ordered paper, airbrush and outline writes
blend separately. Zero alpha reveals the appropriate underlayers rather than
removing the whole card. No drawing, default, font or artwork change is included;
validation is recorded below.

Desktop colour is borrowed whenever an existing Monitor draws the closed program
in the winner-none reveal branch. Its one rectangle premultiplies straight base
RGBA once, with no event fade. The opaque `008080` default, geometry, captured
sheet/frame and palette swatches, and captured persona remain unchanged.
Nonblack zero alpha reveals underlying content; physical monitor details and
other foreground draws remain. Edits have no visible effect while this story
route is hidden. Validation is recorded below.

Monitor power-light colour is borrowed on each visible front-details draw.
Its one inset premultiplies straight base RGBA once, with no event fade.
The opaque `C5D6AA` default, captured resources/persona and geometry remain
unchanged. Nonblack zero alpha reveals the retained opaque indicator ink;
it does not remove the rest of the monitor. Validation is recorded below.

Program colour is borrowed by each existing Monitor draw. Its one program fill
premultiplies straight base RGBA once, with no event fade; opaque `2C2C2C`,
resources, persona, geometry and palette occlusion remain unchanged. Nonblack
zero alpha reveals the opaque `161616` glass below the visible program mask,
not the initial Canvas. The native contract uses one effective Canvas.

**Actual-Level visibility witness:** `Level::draw_intro` submits Monitor behind
its miniature world, with later intro/reveal/front layers able to cover the
program. The separate `ProgramLevelVisibilityOnly` (`program-level-visibility`)
check below verifies exposed program adoption/restoration in one retained
two-player Crossfeed full-opening checkpoint at 1280x720. Direct-Monitor alpha
evidence remains separate; other story phases/rosters and human story acceptance
remain open.

Monitor borrows `opening.glass_colour` at four writes: initial glass, palette
repaint, picker panel and close-button face. Each premultiplies base RGBA once,
without event fade. Opaque `161616`, API/geometry, captured persona/resources,
caller swatches and white frame/X overlays retain their existing behavior.
At zero authored alpha, initial glass exposes ordered bezel/lip; palette
exposes program RGB (44,44,44). Isolated picker/close faces reveal their Canvas
while their white overlays remain. The entire button does not disappear.

The main-method alpha reference covers five guarded ROIs, with ordered opaque
beige, four separate quarter-alpha black shadow writes and lip, then glass,
program and palette. It does not independently reproduce the complete bezel;
copied-Monitor full rasters prove adoption only. Opaque beige erases the main
Canvas, leaving one effective backdrop. Standalone picker/close each use two
genuine Canvas underlays, with independent white frame/caller swatches or
24 literal white X dabs. Actual-Level glass visibility remains unmeasured;
source caller traces and direct methods do not establish that composition.

Monitor reads borrowed `opening.scribble` during applicable front-details note
draws. Tint edits neither add notes nor roll a persona; no captured notes means
no scribble pixels. One existing line expression premultiplies base RGBA once,
retaining three rows, all repeated dabs/order, opaque `3050C0`, authored art,
arrays, geometry, resources and API. No event fade is added.

The independent reference covers only note ROI `[18,337,85,404)`: five
static rectangles then 74 literal selected dabs per context. Context A pairs
note colour0 (`FFF27A`) with tilt0; B pairs colour2 (`AEEBFF`) with tilt0.1.
These are two genuine opaque-paper underlays, not a colour-by-tilt matrix;
one Canvas does not add another scribble underlayer. Alpha0 returns rows to
paper and retains shadow/header/border/underline, rather than erasing the note.
Copied whole front-details rasters prove adoption only. Actual-Level scribble
visibility remains unmeasured, with source callers only.

Monitor reads borrowed `opening.crack` for every thin crack line on an applicable
front-details draw. One base premultiplication preserves authored opaque
`3A3A3A`, captured persona points/order, pen widths, repeated dabs, resources
and API. Each segment draws broad BLACK ink before its selected thin dabs;
later segments can erase earlier tint. No event fade, persona roll or update
is added. Alpha0 retains the existing BLACK crack skeleton and unselected
highlight/Canvas in the declared direct fixture. Copied front-details parity
proves adoption only; independent native ROI and actual-Level visibility
claims remain separate.

InterfaceGameplay reads borrowed `hud.health_colour` when drawing the HEALTH
meter. One base premultiplication feeds its existing fill, preserving opaque
`76E8B5`, geometry and captured sheet/fill/fonts. The supplied normalized
remaining health determines the fill prefix; zero health draws no fill. The
authored track is opaque; its own live tint can change that alpha. Caller
team/ammunition tint keeps its existing path.

Both HUD meters borrow `hud.meter_track_colour` on each shared meter draw.
One base premultiplication preserves authored opaque `B8B8B8`; there is no
event fade. The complete expanded opaque `hud.paper.ink` rectangle (`141414`)
is drawn first, followed by track, then HEALTH/team PAINT fill prefixes.
The authored HEALTH and fixed native-fixture fill colours are opaque;
those covered pixels hide the track. Health40 also permits translucent or
zero-alpha HEALTH fills, so amount 1 alone does not guarantee hiding it.
Track alpha0 restores the ink before later fills blend over or replace it.
Health-only premultiplication and caller team/ammo semantics, amount clamping,
geometry, resources and captured fonts stay unchanged.

Normal HUD timer foreground is borrowed on every applicable normal-clock draw.
One base premultiplication preserves authored opaque `141414`, the captured
timer font, formatting and geometry. Alpha0 retains the opaque paper and the
current independent shadow (authored `00000022`). A running positive time with `ceil(timer) <= 10`
uses the separate closing-clock text and returns before either normal text
call; foreground edits leave that signal unchanged. Zero, stopped short clocks
and displayed times above ten use the normal foreground path. Public-HUD
fixtures establish this draw behavior, not natural Level visibility or human
timer contrast/readability acceptance.

Normal timer shadow reads its separate borrowed colour before the foreground
text. One base premultiplication preserves authored BLACK with alpha34
(`00000022`), the existing offset/font and all geometry. Nonblack RGBA follows
the same ordered composition; later foreground glyph coverage can hide or
blend over the shadow. Shadow alpha0 retains paper and the later foreground,
while foreground alpha0 retains the current shadow. Neither normal colour
changes the closing signal or decides which clock branch runs.

Closing-clock text is borrowed on running positive displayed times up to ten,
including the existing Reduce Effects branch. Its reader already requires
alpha1; accepted opaque tints satisfy the renderer's premultiplied contract
without a draw/fade change. Partial or zero alpha remains a rejected complete
candidate. The normal foreground/shadow, closing paper/ink/accent/shade,
formatting, resource/font capture and geometry remain independent. Actual
public-HUD fixtures do not establish a naturally advancing Level or human
urgency/contrast/readability acceptance.

Closing-clock ink, paper and accent are also borrowed on applicable closing
draws. Their existing reader accepts opaque names, RGB and RGBA-FF and rejects
partial or zero alpha before applying the complete candidate. Equal opaque
material colours remain valid; there is no new distinctness rule. Existing
defaults `141414`, `FFF5DF` and `FFD34D`, drawing order and the existing
premultiplication stay unchanged.

Ink supplies the panel body and detached tick under-strokes, paper supplies the
inner body, and accent supplies four thin tick stripes. The existing beat
changes stroke geometry rather than selected colour alpha. Settled and reduced
draws omit detached strokes; normal-clock draws omit the closing panel.

Closing-clock shade is borrowed live with its separate positive-alpha rule.
Named colours, RGB and RGBA with alpha01 through FF are valid; alpha0 is
rejected. It is neither opaque-only nor zero-allowed, and no contrast or
equality restriction is added. Authored `00000055` and the existing reader
and drawing stay unchanged.

Both base shadows retain full author RGB. Each of the six nested inset bands
also copies that RGB, scales only alpha by 0.12, then uses the existing one
premultiplication. Bottom/right overlaps keep their ordered repeated blends.
This is no forced-black or default/render correction. Actual draw qualification
is separate from the CPU metadata and reader checks below.

HUD text foreground is read from `hud.player_identity.colour` for three
submissions: the identity label, HEALTH and PAINT. Its separate
`hud.player_identity.shadow_colour` supplies only the identity label's offset
shadow, before the foreground; neither meter label has a shadow call. Each
selected argument premultiplies base RGB once, without a caller fade, retaining
authored foreground `141414` and BLACK-alpha34 shadow `00000022`.

Existing HUDs borrow both colours on every applicable draw. The identity font
and meter-label font retain their constructor capture, as do the sheet/fill;
identity strings, geometry, badge colours, meter fill/track and clocks keep
their existing policy. Foreground alpha0 leaves the paper and current identity
shadow, while the two meter labels contribute no selected ink. Shadow alpha0
leaves paper and the later identity foreground. The distinct root badge ink/
background remain opaque and independently supported. The separate
`hud.player_identity.background_colour` preview backing is described below; it
does not colour the HUD identity paper and is outside this pair.

Stage Select captures `hud.player_identity.background_colour` into its preview
backing UiTexture when the page is initialized. A new page adopts an edit;
stable updates, stage cycling, ready/unready and resizing do not recapture the
tint. Ready state can change the underlying filled preview-edge colour, so
retention does not mean browse and ready frames have identical composites.

The captured tint premultiplies base RGB once without a fade. Authored straight
RGBA `121827EE` stays unchanged, but its nonblack translucent default now submits
corrected RGB; old default pixel parity is not promised. Alpha0 contributes no
selected RGB and exposes the current underlay. Random has no stage objects;
real preview art draws later and can cover this backing. This field is neither
a map tint nor gameplay HUD paper, and fonts, layout and stage values retain
their existing policy.

FigureAttachments borrows both `figure.arm_colour` and `figure.hand_colour`
on each applicable procedural draw. Arm colour supplies four arguments:
blink lids, both segments of each arm, and outer hand squares; hand colour
supplies their inner squares. Each selected argument premultiplies base RGBA
once, without event fade, retaining authored BLACK/WHITE and repeated-dab order.
Geometry, grips, resource capture and APIs stay fixed. The separately tuned
quiet face paint-out is described below. Captured rest-arm/sleeve layers still
draw with their team/WHITE atlas tints.

Hand alpha0 exposes its earlier arm/tool underlayer; arm alpha0 leaves the
other layers, including selected hand interiors and quiet face paint-out,
intact. Direct attachment panels do not prove natural Player blink timing or
integrated Player/tool/body/Level composition. Human art and comfort gates
remain separate.

Quiet blink face colour, `figure.quiet.face_colour`, is borrowed on each
`FigureAttachments::draw_blink` call. Its base RGBA premultiplies once for
the eye paint-out, before the independently arm-coloured lid. Authored WHITE
is unchanged; alpha0 retains the underlying pixels outside that later lid.
The existing quiet-enabled flag and Player blink/pose/Reduce Effects gates
still decide when it is drawn. This field changes no quiet timing, geometry,
captured resources or simulation. It is separate from `opening.acting.face`,
which supplies the story eyes below. Direct blink panels establish neither
natural Player blink timing nor full Player/Level visibility and human
readability/comfort acceptance.

FigureActing borrows both acting colours for each applicable draw. Only its
face and sweat tints premultiply straight base RGBA once, without event fade;
authored `WHITE`/`8FD3FF`, stroke geometry, fixed caller inputs and captured
sheet/fill stay intact. Zero face alpha retains later opaque BLACK pupils;
zero sweat alpha reveals the previous underlayer. No new Reduce Effects
behavior is introduced. Level caller/pose coverage for this pair is source-only;
the direct component proof does not establish actual-Level acting visibility.

Step/skid effects that captured a contacted surface's paint colour retain that
tint; changing the fallback does not recolour those stored flecks. Their
highlight and keyline still read the live presentation. Console error, warning,
echo, caret, status and perf foreground inks keep their separate fixed colours.
Station labels use the new premultiplied tint on the next layout refresh,
including drawing beneath a paused console; hidden labels adopt it too and stay hidden.
An already-open page keeps its Home background tint until the page is reopened.
This includes Home, Mode, Players, Join, Team, Weapon, Arena, Options, Controls,
Pad Sticks, Credits, Exit confirmation and display confirmation. Team Select
retains the colours from its last visual refresh while a
draw is frozen or a pad is missing; an active update with every claimed device
connected adopts both tints without reopening the page. Its atlas, frame names,
text and fonts keep their existing captured behavior. These colour fields live
inside fixed presentation records, so
replacement preserves the addresses their live consumers borrow.

```text
tune presentation.movement_feedback.colour WHITE
tune presentation.movement_feedback.highlight FFEEDD
tune presentation.debug_overlay.shadow_colour 00000080
tune presentation.main_menu.backgrounds.home 30343A
tune presentation.movement_feedback.colour
tune.diff presentation
tune.reset presentation.movement_feedback.colour
tune.reset presentation
tune presentation.debug_overlay.station_label.colour YELLOW
tune.save presentation
```

The first three edits demonstrate a named colour, RGB and RGBA. Colours accept
literal values only; `next`, `prev`, arithmetic and boolean words are not
colour stepping commands. Invalid colours leave the live object, current
document and pending diff unchanged. Read, diff, reset and save use the same
paths as numbers and booleans; edits, reset and save retain the existing
record/replay and queued-reload restrictions.

The shared `presentation.menu_style.layout.position/size/spacing` values use
the menu's 1920 by 1080 design coordinates and apply when a page is next opened.
For example, `tune presentation.menu_style.layout.spacing.y 140` changes the
default row spacing; pages with their own layout still use their overrides.
Position must be nonnegative, size positive and the first widget must fit within
the design area. Spacing may be zero and is bounded by the design size on each
axis. Named menu sheets and frames remain string settings edited in the JSON;
only the fifty-eight colour strings listed above join numbers and booleans in `tune`.

Weapon-choice names and artwork are also string settings under
`presentation.main_menu.weapon_select.choices`. Edit them in JSON and use
`reload`; an open weapon page keeps its captured catalog until the page is
reopened. The six required choices and their resource/text validation are
documented in [menu presentation data](menu-presentation.md#weapon-selection-names-and-artwork).
Names must fit the existing description-column gap when measured with their
actual font and horizontal shadow; there is no character-count limit.

Shared brand/headings, Home/Options/Credits/Mode/Player Count/Pause/End labels, the
solo-practice hint and complete control
legends are string settings under
`presentation.menu_text`; they also require JSON edits and `reload`. A page
captures the text and prompt presentation together, so an open page's
controller/stage/pause legend updates keep that pair until the page is reopened.
Legend text includes its displayed button names and spacing; editing it does
not change input bindings. The real font measurement, shadow and design-space
right margin determine whether a line fits; spaces are preserved and there is
no character-count limit. See [shared menu text](menu-presentation.md#shared-headings-and-complete-control-legends)
for the required keys and validation.
`menu_text.items` labels use the actual item font. Plain actions must fit their
paper/focus area; the first three Options labels keep their existing fitted
column/row scaling, so long supported labels can remain valid. Editing a label
does not change its action or row order, and reopening adopts the new text.
Mode/Player Count rows keep their measured size and existing item shadow;
`menu_text.solo_practice_hint` uses the detail font with that same item shadow.
Their text/shadow bounds must fit the design/row limits; the Match length label
also leaves room for its existing duration-value column. Focusable rows require
the text/shadow height to fit their row spacing; the nonfocusable hint has only
the text/shadow design bounds. These complete strings do not introduce
formatting placeholders or alter controls or practice rules.
Pause Resume/Restart/Quit/Yes/No and the shared Controls / audio label are also
captured string settings. Their plain item text is validated against the
full-design Pause paper and padded row height, retaining its measured size.
Reopening adopts edits; confirmation still defaults to No. Dynamic Pause
titles, status and questions remain outside this authored label set.

End Change Teams/Change Weapons/Change Level/Exit to Main Menu are also captured
string settings. Their plain item text and focus padding must fit the full-design
End paper and positive row spacing, with Exit checked at its ongoing-series and
completed-match positions. They retain their measured size and captured item font
until reopening. Label edits leave action availability, continuation, navigation
and music unchanged; session-dependent headings and continuation text remain in
code. See [End Menu actions](menu-presentation.md#end-menu-actions).

Mist `spread_step` constructs the velocity fan when a shot is created. Roller
`aim_tilt` and the shared projectile `wind_stop_factor` are read during updates,
so tuning them affects existing objects. Angles are radians: mist spread accepts
`0..PI/4`; roller tilt accepts `0..PI/2` with the upper endpoint excluded. The wind
factor and each projectile's resistance must be nonnegative and finite, and
their product must stay finite.

The `tuning.input` block controls the jump/shoot trigger thresholds, movement
stick deadzone and menu stick engagement/release thresholds. The shipped values
preserve the previous feel (0.6/0.6 triggers, 0.5 movement, 0.8/0.6 menu).
All five are normalized to [0, 1]; movement deadzone must be below 1, and menu
thresholds must satisfy `0 < release < engage <= 1` so neutral always releases.
They are read each frame and can be tuned live, for example
`tune tuning.input.movement_deadzone 0.4`. Aim sensitivity/deadzone stays in
saved Controls preferences; menu repeat timings stay in presentation data.

**Steps.** `next`/`prev` (and the debug menu's Left/Right) step a decimal value
by one unit of the digit below its leading one (2000 by 100, 0.35 by 0.01, 900 by
10, 0 by 0.1) and an integer-written value (`"pixel_size": 4`) by whole numbers.
New values keep the token's style: `240.0` becomes `250.0`, never `250`.

**`tune.diff`** lists each edit with the file's value and its scope, and notes
when the F9 feedback toggle disagrees with the file (F9 is a switch, not an
edit; a tune of another presentation value keeps it). **`tune.reset`** puts a
path, a file or everything back to the file's value through the same validation.
**`tune.find <word>`** lists matching paths with their values.

**`tune.save [tuning|presentation|all] [force]`** writes the edits into the
**source** files in `game/content/` (known to a Debug build made from a
checkout; elsewhere it says it needs one). Only the edited tokens change: key
order, indentation, comments-as-keys, unrelated string escapes, number style and
line endings - including a file that mixes CRLF and LF - stay byte for byte.
Before writing, each edited token on disk must still be the value the game
loaded; if someone changed it in
the meantime, that is a conflict: nothing in that file is written and the paths
are listed (`force` overwrites them). Colour conflicts compare the original
quoted token bytes too: an externally changed spelling or escape is an edit,
even if it decodes to the same colour. Each supplied colour literal is encoded
as one JSON string token; its surrounding source bytes stay intact. The file
is replaced atomically, read back through the game's reader and every startup
check that reads it (resources and every stage), so a save never leaves a file
the next launch refuses; on failure
the old bytes go back (and if even that rename fails, the log says so and keeps
them in `<name>.json.bak`). The staged copy beside the executable is replaced
the same way, so a relaunch without a rebuild sees the save. `tune.save` and
`tune.reset` are refused while a recording or a replay runs and while a
`reload` waits to be applied. After
a save the file is the new baseline and `tune.diff` is empty. `reload` likewise
makes the reloaded files the baseline and drops live edits (it is refused
while a recording or a replay runs).

In the debug menu, **tuning** opens on the two files, then their groups one level
at a time (`DEBUG / tuning / player`); long groups are paged with a `more (1/2)`
row. A numeric/boolean value row shows its token; Left/Right run
`tune <path> prev|next` (a boolean toggles). A colour inspection row shows its
token and `(console)`; A/Enter runs `tune <path>` to read it, and Left/Right
neither changes it nor emits a command. Menu commands are echoed
`[menu] tune ...`, and B/Escape climbs one level.

Not tunable: additional unaudited colours, arbitrary strings, font and sheet
names, and optional keys a file leaves out (`frame_time` on most clips,
`diffusion` blocks) - `tune.save` never inserts keys.

## Bots

`bot <seat|all|off> <behaviour>` gives seats scripted input, in place of the
device (or the neutral input of an idle dummy):

| Behaviour | Does |
|---|---|
| `idle` | neutral input (for a device seat: hands it a still player) |
| `walk` | walks one way, turns at a wall or now and then |
| `jump` | walks, and jumps when blocked or every second or three, holding for a random time so the heights vary |
| `chase` | walks at the nearest living opponent, jumps when it is above, aims and fires at it within range |
| `paint` | roams like `jump`, sweeps its aim over the floor ahead and behind and fires until the tank is nearly empty, then lets it refill; an opponent close by is shot at instead |
| `skirmish` | roams and paints, engages a nearby opponent using a tool-derived reach heuristic, withdraws without firing to refill, then returns toward its saved X or re-engages |

- `bot all <behaviour>` drives every **idle dummy** of the current and later
  matches; `bot 2 chase` drives seat 2 whatever its device (the keyboard seat
  too); `bot 2 off`, `bot all off`, `bot off` hand seats back. `bot` alone lists
  the assignments. `players 4 paint` is `players 4` plus `bot all paint`. In a
  match the change applies at once; assignments survive restarts and rebuilds.
- These are Debug behaviours, which only drive a seat's input. A **shipping
  bot** (Easy/Normal/Hard, as the menus' bot seats) is the seat's controller and
  is set with `seat <n> bot <difficulty>`; it is never an idle dummy, so
  `bot all` leaves it alone, while `bot <n>` on its seat overrides its input.
- A bot's input replaces the seat's after the device and the mouse aim are
  resolved, through a seat filter `Level::update` takes as a parameter
  (`game/objects/seat_input_filter.h`).
- Bots draw on their own random numbers (splitmix64 from the seed and the seat,
  the shipping bots' `BotRandom` in `game/ai/bot_random.h`), never `rand()`:
  with `seed` set, the same match plays out the same way every time. Shipping
  bots are seeded with the round seed itself (`MatchRandom::bot_seed`), as the
  balance runner seeds its bots.
- A seat a bot drives never opens the disconnected-pad pause, the pause page's
  "Controller lost" or the restart fallback - not even a pad seat whose pad is
  unplugged.
- A driven seat's HUD says `Bot: <behaviour>` instead of its device ("Waiting
  to join" for a dummy), and every seat says `Replay` while a replay drives the
  match. The label is a plain list of strings on `Level` that only the Debug
  director sets; Release never fills it.
- `status` adds `bot <behaviour>` to each driven seat. The paint overlay and
  results credit surface held by every seat, including bots on idle dummies.
  Device players keep their slot-based credit; each device-less player has a
  separate paint owner by view. Friendly repaint retains the original capturer's
  credit, and enemy repaint transfers it.

Per-seat credit completed on 2026-10-15 (`DBG#3`). Debug and Release consumer
acceptance passed: separate dummy owners, sparse device slots, friendly/enemy
repaint, result percentages, and real bot projectiles accounting for all held
paint. The seeded real-Level recording still replays with matching checksums.
Debug startup passed with an isolated save and graceful shutdown. Evidence is
under `out/acceptance/{x64-debug,x64-release}/` and `out/validation/dbg3-startup/`.

### Skirmish measurement bot

Use `bot 2 skirmish` for a seat, `bot all skirmish` for idle dummies, or
`ColourWars.exe +stage crossfeed +players 4 +bot all skirmish` in Debug.
The [headless runner](gameplay-tuning.md#headless-balance-runner-tool1-2026-10-25)
accepts `-Bot skirmish`; its default remains `paint` and existing bots retain
their behavior.

Skirmish paints while roaming, then approaches and fires at the nearest living
opposing-team seat within its engagement heuristic. It refills when actual
full-shot affordability is empty, even with positive ammunition, or its reserve
falls to 15%. It withdraws without firing and waits for at least 80% reserve
and full-shot affordability to return. Cooldown alone does not trigger refill.
It then conserves that refill while returning to its saved X; saved Y is only
a jump hint. A nearby enemy can interrupt the return. Death discards the old
life's refill/return goal and releases a held jump.

Reach is the shipping bots' own estimate (`match_bot_ballistics::horizontal_reach`
in `game/ai/`): current tool/projectile tuning for a stationary horizontal
launch over at most one second of fixed updates, capped for a falling shot at
the ideal same-height range, plus muzzle/body/collision extent. Before
2026-10-30 (`HYG#34`) the Debug driver kept its own copy without that cap, so
skirmish engaged gravity tools from farther away. It is an engagement
heuristic, not a hit-range guarantee: inherited velocity, diffusion, target
motion and terrain are not solved.
There is no line-of-sight test, route planner or ballistic intercept solver;
the bot can fire into a wall or fail a route. Seeded developer measurements
do not establish human comfort, arena acceptance or tool balance.

### Headless paired paint measurements

The Debug-only headless runner accepts `-PaintCsv <new path>` through
`scripts/run-balance.ps1`, or `--paint-csv` through the native
console. Its companion defaults to `<mainstem>.paint.csv` beside the main CSV.
Main schema 2/81 stays unchanged; the independent companion is schema 1/29.
These are final held-paint measurements by region/authored face and validated
owner, not route occupancy or human balance acceptance. See the
[measurement and two-file recovery contract](gameplay-tuning.md#paired-final-paint-csv-contract)
before interpreting denominators or a partial pair. This adds no in-game console
command or shipping bot policy.

## Seed, record and play

**`seed <n|off>`** fixes the random seed of every build until `seed off`:

- **The round seed.** Every build under `seed n` is built from round seed `n`
  (`HYG#39`). A build draws only from the `MatchRandom` made from it
  (`game/objects/match_random.h`): one splitmix64 stream for a Random team
  palette and another for the intro's monitor persona, and the shipping bots
  get the seed itself. Which of them draw never moves another, and a build
  never reads or moves `rand()`.
- **The menus' `rand()`.** The random teams, weapon and stage stay on `rand()`.
  `seed n` seeds it at once, and every seeded build seeds it again with the
  round seed, so the menus' picks after any build under `seed` repeat.
- **The planner and the scripted bots.** The planner's `stage random` and
  `weapon all random` are re-seeded for each plan, so every one draws the
  same, and the Debug bots take the seed directly.

`+seed 7` before `+stage` makes the whole launch repeat. `seed` alone prints
it, and `status` shows the running round's seed (`round seed n`): `seed n` then
`restart` builds that round again, palette and persona included.

With the save's palette on Random, the round seed picks the pair. Against the
shipped `presentation.json` list (`blue_orange`, `pink_green`, `blue_yellow`,
`purple_gold`, `red_azure`, `magenta_forest`):

| `+seed` | Team pair |
|---|---|
| 0 | `blue_yellow` |
| 1 | `blue_orange` |
| 2 | `magenta_forest` |
| 3 | `red_azure` |
| 4 | `purple_gold` |
| 5 | `magenta_forest` |
| 6 | `blue_orange` |
| 7 | `purple_gold` |
| 211 | `pink_green` |
| 4242 | `pink_green` |

The draw is plain integer arithmetic, so the table holds on any build, and the
`team_palette` acceptance case pins it. Adding or reordering a pair moves it:
update this table and that pin together.

**`record [name] [updates]`** records every seat's input on every logic step,
from a fresh build: in a match it restarts the match (and closes the console),
in the menus it waits for the next match. It stops and saves after `updates`
Level updates (the 3 s countdown is 180 of them), at the end of the round, when
the match is rebuilt or left, or on `record stop` (a bare `record` while
recording stops too). The file is `%LOCALAPPDATA%\ColourWars\replays\<name>.json`
(default name `replay-YYYYMMDD-HHMMSS`): the stage, roster (device slots, teams,
weapons), best-of, the round seed the level was built from, the cheats, hashes of both content files
and the live tune edits, a semantic hash of the loaded stage definition
(`stage_hash`; [how it is computed](debug-console-record.md#loaded-stage-replay-fingerprint-dbg8)), the step size, one run-length list of inputs per seat
(floats as bit patterns), where the match was suspended and resumed, a state
hash every 60 updates and the final state. In memory a recording is the same
`RoundTape` the shipping instant replay keeps (`game/round_tape.h`); the file
is that tape plus the checks.

Files use **format 6**, with explicit human/bot controller kind and
Easy/Normal/Hard difficulty for each participant. `seed` is required and is the
round seed, which `play` builds from, so a recording made without `seed`
replays its Random palette too. Layout counts humans, while
input arrays count all participants and are all the same length. `gaps` lists
every suspend and resume in order, a resume with the seat commands it re-read;
each has the tape `step` it preceded (an index into the inputs, which the
countdown and the end of the round do not advance) and the Level `update` it
was noted at, which is where playback repeats it.
Formats 3 and 4 (which noted suspends only) and 5 (whose `seed` was the
optional developer seed) are refused: a replay holds only on the build that
recorded it. The metadata is strict; bot seats cannot own devices and P1 must
be human. Human difficulty is unused and written as
Normal. Normal setup now supports playable bots
(`ROAD#10`); recording captures their final inputs, and playback bypasses their
live decision driver. Arena navigation/combat and attended difficulty acceptance
remain separate from replay qualification.

**`play <name> [force]`** rebuilds the match with the recorded settings
verbatim - from a match or from the menus - and feeds every seat its recorded
input, ignoring every device. At each checkpoint and at the last recorded
update it compares the state; then it pauses the clock to hold the last frame
(the next `play` resumes it; a pause you set yourself with F6 or `pause` stays,
so `pause on` then `play` steps a replay from its first update with F7) and
logs one of:

```text
replay drive: MATCH (600 updates, 10 checkpoints, checksum 1b2bcf45e4694e88, paint A 11136 B 34896 of 194880)
replay drive: DIVERGED at update 240 (checkpoint 4 of 10): state hash ..., recorded ...
replay drive: final state differs: P2 x 1532.5 != 1530.25
```

`play stop` hands the seats back. `replays` lists recordings; `checksum` prints
the running match's state hash and paint, which is what the comparison uses.
`play` refuses a recording made with other tuning (`tune.reset` or `reload` to
match it, or `force` to watch it diverge), refuses another step size, and warns
about another build or presentation file.

**What is deterministic, and what is not.** Verified by
`scripts/acceptance/debug_console/replay_checks.cpp` on a real `Level` (four seats, paint bots,
a scripted keyboard seat, 600 updates): the replay matches at timescale 1 and 4
and at another window size, and an unrecorded re-run of the same seeded match
ends on the same hash. The simulation is single-threaded (the thread pool only
draws), collisions are processed in index order, nothing in a step calls
`rand()`, and the keyboard seat is recorded after its mouse aim is resolved, so
neither the mouse nor the camera is read on replay. The limits:

- **Same build, same machine.** Floats are compared bit for bit.
- **Same step size.** The executable steps 0.0166666 s (the engine's 10 MHz
  timer rounds 1/60 down); the CPU acceptance case uses 1.0f/60. A recording
  from one never replays in the other, and `play` says so.
- **Suspends.** Covering the match (the console, the debug menu, the pause
  menu) and losing window focus suspend the level, and suspending clears every
  player's jump buffer outside the input stream; resuming re-reads which jumps
  are still held. The recording notes the Level update of every suspend and
  resume, with the seat commands a resume read, and the replay repeats each there,
  without pausing a voice. A suspend during playback
  that the recording did not have is reported, and a divergence after it is
  expected - so do not open the console or switch windows while a replay you
  care about is running. Pausing (F6) and stepping (F7) are not suspends: the
  paused clock pauses the voices only, so a replay paused or stepped one update
  at a time still matches.
- **Side doors are shut.** While recording or replaying, `goto`, `respawn`,
  `kill`, `god`, `ammo`, `bot`, `tune`, `tune.reset`, `tune.save` and `reload` are
  refused; a `play` queued and then left behind (`menu`, the match ending, a
  plan that fails) is disarmed rather than taking over the next match; the
  cheats in force when recording began are stored and re-applied by `play`.
- **What is compared** is public state: each seat's position, velocity, health,
  ammo, respawn timer, move state and life, the paint areas and the level state.
  Hidden state (a jump buffer, projectiles in flight) shows up when it changes
  one of those, a little later.
- One round: a recording stops at the end of the round.

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

- `screenshot.dir <folder>` puts the next shots in `screens\<folder>\` (up to
  three `/`-separated levels, each kept to `A-Z a-z 0-9 _ -`; `{time}` becomes
  `YYYYMMDD-HHMMSS` once, when it is set), so a script collects a review set in
  one place. `screenshot.dir off` goes back to `screens\`.
- `screenshot.strip off` leaves the status strip out of the shots (the perf and
  paint panels stay), so two pictures that differ only in a setting the strip
  names (`feedback off`) differ only in the world.
- In a script a screenshot is a barrier: the next line waits until the shot is
  taken. With nothing queued it is taken at once, between the same two updates,
  so `screenshot a; feedback off; screenshot b` is two pictures of one
  simulation frame.

It re-renders the stack into an unpresented frame and reads it back, from an
update, which is the engine's documented way to capture a frame inside a running
`Application` (`Renderer::read_back_buffer`; a submitted frame that is never
presented is legal). That redraw adds no sample to the perf numbers, and the
frame after it is not timed across the capture.

## Scripts

A script is a file of console lines: `exec <name>` runs `<name>.cfg`.

- **Where.** `exec` looks in the source tree's `scripts/debug/` (a Debug build
  knows its checkout, as `reload` does), then in
  `%LOCALAPPDATA%\ColourWars\debug\`. A name is `A-Z a-z 0-9 _ -`, with or
  without `.cfg` - never a path. `exec.list` lists both folders with each
  script's first `//` line; Tab completes the names.
- **Grammar.** One console line per text line, `;` between statements as at the
  prompt, `//` to the end of the line outside quotes, blank lines ignored, CRLF
  and a UTF-8 BOM accepted, ASCII only, at most 64 KiB. `exec name a b` makes
  `$1`/`$2` mean `a`/`b` in that file (`$$` is a `$`); an argument may not hold
  `;`, `"`, `//` or a space, so it can never add a statement, and a `$n` with no
  argument is an error at its line. There are no loops, conditions or variables,
  deliberately: a script says *which* frames and stations, the code says how.
- **Waits.** A script runs between updates, never inside one, and stops at a
  `wait`:

  | Wait | Until |
  |---|---|
  | `wait <n>` | `n` Level updates (1-1000000). It follows `timescale`, `pause` and `step` exactly: at x4 the step that reaches the count stops there, paused nothing counts, and `step 10` completes `wait 10` |
  | `wait <x>s` | `x` match seconds: `round(x / dt)` updates at the engine's step (0 < x <= 3600) |
  | `wait match` | the queued match is built, nothing is queued to rebuild it, and its countdown is over (exactly the update GO appears, even at x4) |
  | `wait frames <n>` | `n` director hooks on which the script could run (a match update or a menu page's, not while the debug menu or the frozen console holds it); the only wait that counts in the menus |
  | `wait replay` | the armed or running replay has finished. It then holds its last frame **paused**, so a `wait <n>` after it needs `pause off` first |
  | `wait record` | the armed or running recording has been saved (a recording with no limit is saved when its round ends). Armed with no match queued to start it - `record` in the menus without `stage`, or its restart dropped - is an error |

  `wait <n>`, `<x>s` and `match` with no match running or queued are errors
  (`stage` first, or `wait frames`), and a wait whose match ends stops the
  script. So does a `wait <n>` or `<x>s` that runs past the end of a round:
  the results page does not advance by itself, so no update would come. `wait` outside a script is refused.
- **Errors.** The first failing line stops the script, with
  `script <file>:<line> stopped: <why>`; an `exec` that fails (no file, a parse
  error) stops its caller at the `exec` line. `exec` nests up to eight files; a
  file cannot `exec` itself. `exec.stop` stops everything.
- **What holds a script.** A frozen console, the debug menu, and a window that
  is not in front (the match only updates while the window is active) stop it
  with the world: nothing it waits for can happen. The status strip shows
  `SCRIPT <file>:<line> <wait>` and why (`the clock is paused`, `the window is
  inactive - click it`), and `status` lists the frames. **Keep the window in
  front during a review**: losing focus also suspends the level, which clears
  every jump buffer and can change the checksums. Use `console.live 1` to type
  while a script runs.
- **Background scripts.** `script.background 1` (session-only, default `0`;
  bare, it reports the choice) lets scripts, bots and replays keep updating in
  a shown window that is not in front, for an unattended review such as
  `ColourWars.exe +script.background 1 +stage movement_course +bot all paint +wait 300 +quit`.
  Physical input stays out, music and SFX are muted without touching the saved
  preferences, and rumble stays at zero; hidden, minimised and power-suspended
  windows still stop the script. Turning it off while inactive restores the
  normal hold. How it was qualified is in
  [the work record](debug-console-record.md#background-script-work-dbg6).
- **Typed or bound.** A typed or bound `exec` closes the console and starts on
  the next frame; a second one while a script runs is refused (`exec.stop`
  first). A line typed while a script runs executes at once, beside it.
- **Launch and autoexec.** The launch arguments are a script that keeps going
  after an error, as they always have. `autoexec.cfg` in
  `%LOCALAPPDATA%\ColourWars\debug\`, if there is one, runs before them (the
  transcript `+logfile` opens starts with everything logged before it, so it
  shows autoexec too). Only the game reads that folder: the acceptance case,
  with no application, reads and writes nothing there.
- **Record and play.** A script line is refused during a recording or a replay
  exactly when the same line typed would be, and that stops the script; `exec`,
  `wait`, `echo` and `exec.stop` touch no input and are always allowed.
- **Not sandboxed.** A script changes the session like typing would. The
  shipped ones put back the bots, the seed, the screenshot folder and strip and
  the timescale as their last lines; each one's header says what it leaves
  changed (`review_feedback_ab` leaves the overlays off on four seats).

**Shipped scripts** (`scripts/debug/`, each with how to run it at the top):

| Script | Does |
|---|---|
| `review_feedback_ab` | the movement feedback A/B below |
| `station_tour` | one player on the Movement Course, god and the collision overlay on, a screenshot at each of the ten stations, into `screens\station-tour-<time>\` |
| `bots_4p` | four paint bots on Close Quarters, seed 7, 20 match seconds at x4, then a paused screenshot with the paint overlay and the checksum, into `screens\bots-4p-<time>\` |
| `feedback_ab_pass`, `feedback_ab_shot` | helpers of `review_feedback_ab` |
| `review_finish_details` | the [finish-review detail A/B](polish-plan.md#finish-review-detail-ab-script-pol43): seed 7, four seats on Crossfeed (seat 4 idle with a sprayer), frames `t07s`, `t17s` and `t54s` with every presentation detail on, then once per `enabled` flag off (`tune ... off restart`), each with its checksum, into `screens\review-finish-details-<time>\`; ends with `tune.reset presentation`. `scripts/check-finish-details.ps1` runs it and requires equal checksums across the eleven variants |
| `finish_details_flag`, `finish_details_pass` | helpers of `review_finish_details` |
| `jump_baseline` | live-tunes the 2026-10-08 historical jump on the Movement Course (held 207 high, 1.02 s airborne, tap 95) |
| `jump_low`, `jump_mid`, `jump_snappy` | live-tune the three `POL#3` jump candidates on the Movement Course; each header gives its height, airtime and tap |
| `crossfeed_baseline` | Crossfeed comparison P0: the 2026-10-08 movement and original tool supply and lifetime |
| `crossfeed_movement`, `crossfeed_lower` | Crossfeed P1 and P2: an experimental lower jump with the original (P1) or coupled (P2) tool supply and lifetime; neither clears existing 200-unit climbs |
| `crossfeed_coupled` | Crossfeed P3: the provisional global compatibility movement and coupled tools |
| `review_arena <stage>` | the [arena review pack](debug-console-record.md#arena-review-screenshot-pack-dbg18): seed 7, paint bots on every seat, a clean frame at GO and a heavy one after 20 match seconds at 4, 3, 2 and 1 players, each with its checksum, into `screens\review-<stage>-<time>\` |
| `review_press_tower`, `review_close_quarters`, `review_crossfeed` | `review_arena` for that stage, then every station photographed on the one-player heavy frame with the station labels on |
| `review_king_of_the_hill`, `review_turbulence`, `review_turbulence_blockout`, `review_turbulence_mover_trial`, `review_turbulence_slab_trial`, `review_turbulence_ferry_trial`, `review_turbulence_gate_trial` | `review_arena` for that stage |
| `review_arena_frames`, `review_arena_pass`, `review_arena_end` | helpers of `review_arena` and the wrappers |
| `turbulence_station_tours` | every Turbulence variant's stations, one screenshot each with the collision overlay: `turbulence_station_tour <stage>` (the ten shared stations) on the blockout and gate trial, `turbulence_mover_station_tour <stage>` (plus the four mover ends, photographed paused as placed) on the mover, ferry and slab trials, into `screens\turbulence-stations-<stage>-<time>\` |
| `turbulence_station_tour_shared` | helper of the two Turbulence tours |
| `review_bots_boost` | seed 7, four seats on Crossfeed: P1 (a paint bot drives it; drop `bot 1 paint` to play) with a Hard shipping bot against an Easy bot on Boost +2 and a Normal bot on Boost +1; `status` lists the roster, then 30 match seconds at x4, the checksum and a paused screenshot, into `screens\review-bots-boost-<time>\` |

The jump and Crossfeed scripts change tuning live and globally, and do not put
it back: `tune.reset tuning` restores the loaded defaults, and `tune.save
tuning` is the only thing that writes them.

The acceptance case follows every shipped script from the top-level ones
through each `exec` with the arguments it passes, and checks every statement's
command and word count and every stage, bot and station it names. A station
must belong to the stage the walk last named, or the Movement Course before any.

### Worked example: `review_feedback_ab`

```powershell
out/build/x64-debug/game/ColourWars.exe +exec review_feedback_ab +quit
```

One command, about five seconds, and the window closes. What it does:

1. `screenshot.dir review-feedback-ab-{time}`, `screenshot.strip off`, `seed
   4242`, `overlay none`, `feedback on`: one folder for the set, pictures that
   differ only in the world, and the same bots and palette on every build.
2. `bot 1 jump` and `players 4 jump`: four panes, every seat a jump bot (the
   keyboard seat too), so nobody has to hold a key.
3. `exec feedback_ab_pass`: `stage movement_course`, `timescale 4`, `wait
   match`, then three frames counted in updates after GO: `t040`, `t096`,
   `t156`, chosen where the seeded jump bots' bursts are on screen.
4. Each frame is `exec feedback_ab_shot <frame>`: `pause on`, `checksum`,
   `feedback on`, `screenshot <frame>-on`, `feedback off`,
   `screenshot <frame>-off`, `feedback on`, `pause off`. Both shots are taken
   between the same two updates.
5. It puts back the strip, folder, bots and seed, and echoes
   `review_feedback_ab: done (3 frames x on/off)`; `+quit` closes the game.

The result: six PNGs in
`%LOCALAPPDATA%\ColourWars\screens\review-feedback-ab-<time>\`, named
`<frame>-on.png` / `-off.png`, and three `checksum` lines in the log
(add `+logfile <path>` before `+exec` to keep them):

```text
checksum 4f69583bfe85bb3e at update 221, paint A 0 B 0 of 194880, bursts 1
```

`bursts` is how many feedback bursts are alive at that frame: above 0, so each
on/off pair differs. `scripts/check-debug-console.ps1` runs the review and
checks those paired frames. The workshop art comparison and its F8 default
binding were retired on 2026-10-15; the review now covers the current artwork.

## Binds

The match's key table: a key (with `ctrl+` and/or `shift+`) to a console line.

- `bind shift+f5 exec station_tour`, `bind shift+f2 "overlay perf"` (several words are
  joined; a quoted line keeps its `;`s), `bind f5` shows one, `bind` or `binds`
  lists the table (`*` marks a change from the defaults), `unbind f4`,
  `unbind all` (the defaults too), `binds.reset`.
- Keys: `a-z 0-9 f1-f12 numpad0-9 space tab enter backspace insert delete home end
  pageup pagedown left right up down minus equals lbracket rbracket backslash
  semicolon apostrophe comma period slash pause escape`, case-insensitive. No
  `alt+`: Alt+F4, Alt+Space and Alt+Tab belong to the system whatever a bind
  says. **`grave`, `f10` and `f11` are reserved** (console, debug menu). Bare
  binds on A, D, S, Space, Down, Escape and F1 keep their existing warning;
  Space still skips the opening after Jump is rebound, and Enter retains its
  existing omission. A bare bind on any effective saved keyboard key (Move
  left/right, Jump, Drop, a keyed Shoot or the extra Pause) also warns: it
  does both. The Keyboard Keys page warns about an existing bare bind on any
  of its pending keys. These are warnings, so the bind and gameplay source both
  remain available. See the [keyboard Jump slice](feature-roadmap.md#action-bindings-gap42)
  and the [keyboard actions slice](feature-roadmap.md#keyboard-actions-slice-gap42).
- Modifiers match exactly: Shift+F5 does not run F5's bind. (Before phase 4,
  Shift+F5 restarted like F5.)
- Binds run only in a match, and not while the live console is open. The echo
  is `[F5] restart`, `[SHIFT+F5] exec station_tour`.
- The defaults are the F2-F9 table above. `bind` and `unbind` save
  `%LOCALAPPDATA%\ColourWars\debug\binds.cfg` at once, holding only the
  differences from the defaults as exec-able lines (so a later change to a
  default reaches a profile that never touched that key), and it is read at
  startup; `binds.reset` deletes it. A line it cannot use is a warning, never a
  stop.

There is no free F-key for the perf overlay: F12 is the Windows debugger hotkey
(it breaks into an attached debugger). `bind shift+f2 "overlay perf"` gives it one.

While the Keyboard Keys page captures a physical key (or Shoot's click), it owns input before
developer page hooks run. Grave/F10/F11 presses and the pad Back+Start chord
cannot open developer UI or leak into it afterward. Capture discards queued
scripts, match/leave requests, goto, screenshots, reload content and armed
record/replay work, with one cancellation notice. It retains active
recording/replay, binds and current settings. A consumed pad chord must release
both buttons before it can start another session. The developer hooks and
collision query are inert in Release; keyboard rebinding is available there.

## History

The console's history (Up / Down) is the last 64 non-blank lines, adjacent
repeats collapsed, kept in `%LOCALAPPDATA%\ColourWars\debug\history.txt` and
rewritten after every line, so it carries into the next session. The live and
frozen consoles share it.

## Live console

`console.live 1` makes the console key open a **live** console over a running
match: a shorter panel (the top 30%, opaque enough to read over the HUD cards;
`live]` prompt) while the match keeps updating. `console.live 0` (the default) keeps the frozen console.

- While it is open, the keyboard/mouse seat is connected and idle: no move,
  jump, drop, shot, F1 or pause - the typing never reaches the player. Pads,
  bots and replays play on. Mouse aim still follows the cursor (the level reads
  the mouse directly), but nothing fires.
- Escape (on release) or the console key closes it; the frame it closes on is
  blanked too, because that Escape release is the keyboard player's pause key.
- Lines run at once and the console stays open: a rebuild applies on the next
  update without closing it. The F-key binds are off while it is open; F10
  still opens the debug menu, which closes it.
- It exists only over a running match. In the menus, or when a page (pause,
  results, the debug menu) comes up over the match, the console is the frozen
  one.
- It causes no level suspend (no state is pushed), so typing into it during a
  recording does not desynchronise the replay; the recording holds the blanked
  seat.

Blanking the seat is this client's input policy. Raw reads are never consumed,
and the engine declined input capture as game policy
([report 9](review/engine-reports.md#9-no-application-level-per-frame-hook-or-input-capture--document-only-f1e5244), closed).

## Perf

`overlay perf` (or `overlay.perf on`, or a bind) shows, bottom left above the
paint panel, a graph of the last 120 frames' frame time (one bar a frame; green
up to 60 Hz, amber up to 30 Hz, red beyond; the white line is 16.7 ms; the graph
is two 60 Hz frames tall unless a frame was longer) over eight lines. Illustrative values:

```text
CPU wall ms min    avg    max  (240 frames)
frame   14.68  16.68  18.80
update   0.06   0.11   0.17  x1.00 a frame
draw     0.14   0.23   0.37
begin    0.01   0.02   0.04 (240 engine frames)
submit   0.10   0.16   0.25
end     14.20  16.10  18.10
objects 23  projectiles 0
```

Min, average and max over the last 240 rendered match frames, timed with
`QueryPerformanceCounter`: **frame** is the time between two match draws,
**update** the game's own `Level::update` calls in that frame (the seat filter -
bots, record, play - included; `x` is updates per frame, so x4 shows 4.00),
**draw** the match's draw from the top of `GameLevel::draw` to the debug panels
(refreshing the layout and recording the draw lists, with the scene's workers
joined). **objects** counts the scene's collision objects. `perf` prints the
lines, headed once a match has been built by `build 6.7 ms  movement_course  4P`:
the last level build's wall time, from the end of the director's
`before_level_build` to the top of `after_level_build` (the layout and
`LevelBuilder::build_level`, the stall the last menu frame sits through;
[measured](polish-plan.md#level-build-pause-measured-2026-10-17) for `GAP#5.1`).
`perf.reset` starts the frame numbers again (the build line stays). They are
measured whenever a match runs, overlay or not, and only while the match is on
top: a build, the menus, a page over the match (pause, results) or a
screenshot's capture is a gap, not a frame - the first draw after one starts a new interval and records nothing (so does the
very first). Focus loss and covering pages now break the interval immediately,
including draws that occur before another simulation update.
Frame intervals include intervening submit/Present wall time indirectly.
The `begin`, `submit` and `end` lines read `Application::frame_timings()`
(engine report 14), pairing its last completed frame with the pending match
sample by the exact application frame number. Their statistics include only
paired samples; the `begin` line reports that count, and unavailable phases
show `n/a`. A measured zero is valid. The final sample waits for publication
before capture export, including a one-frame capture followed by scripted quit.
These are CPU wall times, which may include GPU/display waits, and need not
sum to the draw-to-draw interval. **Not measured separately**: Present, GPU
work, CPU busy time, compositor timing or scan-out.

The per-player F1/Back text also uses the current measured draw-to-draw interval:
`render fps: 30.0` means a 33.33 ms rendered interval even when `sim dt` remains
1/60. The main thread snapshots it once before view workers start; every pane
reads the same value and drawing advances no clock. The first draw, focus/menu
gaps, screenshot redraws and other unavailable timing show `render fps: n/a`.
Release has no developer timing service, so its legacy F1/Back text also shows
`n/a`, without inferring a frame rate from the simulation step. This closes
`GAP#5.4`; visible, uncontended qualification and the hardware baseline remain
`POL#25`.

`perf.capture <1-36000> <label>` records chronological raw intervals independently
of the ring, then writes CSV and completion JSON under `debug/perf/` in the user
profile. Labels use 1–64 letters, digits, `_` or `-`. Sampling starts with a fresh
interval in a live match at 1x; focus/coverage, count-ins, clock changes, resizing,
rebuilds and screenshots invalidate continuity. Export happens after sampling,
and a script's next statement waits for it. `perf.reset` refuses while a capture
is pending. Interrupted captures are marked incomplete. New captures use
schema 2, with `application_frame`, `begin_frame_ms`, `submit_ms` and
`end_frame_ms` alongside the existing interval, game-update, game-draw and
population columns. Completion requires every engine frame to be paired;
unavailable phases are blank, never invented zeroes. The analyzer accepts
historical schema 1 captures without adding engine-phase measurements. The
[frame-pacing workflow](polish-frame-pacing.md) adds isolated workloads,
machine/load metadata, percentile summaries and the remaining POL#25 acceptance.

## Movement Course review recipes

```powershell
# The feedback A/B, as screenshots, in one command
ColourWars.exe +exec review_feedback_ab +quit

# Every station, one screenshot each
ColourWars.exe +exec station_tour +quit

# By hand
ColourWars.exe +stage movement_course
```

- **Feedback on/off.** Run the route; press **F9** and run it again. The bursts
  stop immediately; nothing else in the simulation changes (the acceptance
  probes check that on/off simulations are identical).
- **One obstacle, many times.** `goto running_gap`, run, and `goto running_gap`
  again; or stage it from launch with `+goto`.
- **Frame by frame.** **F6** to pause at the moment you care about, **F7** to
  step one update at a time, **F6** to resume. Or `timescale 0.25` in the
  console for slow motion; `timescale 1` to return.
- **Four panes, one keyboard.** `+players 4` gives P1 the keyboard and three
  idle dummies, so the split-screen framing and badges can be reviewed alone;
  **F3** shows each pane's camera border.
- **Tuning pass.** `tune tuning.player.jump_launch_velocity next` (or Left/Right
  in the debug menu's tuning tree) while running the route; `tune.diff` to see
  what changed; `tune.save tuning` to write exactly those values into
  `game/content/tuning.json`. Or edit the file by hand and `reload`: the match
  restarts with the new values, no rebuild needed.
- **A four-player match alone.** `+players 4 +bot all paint` (or `chase`): the
  bots move, jump, shoot and paint, and their seats never pause for a pad.
- **Record and play.** `+seed 7 +stage movement_course +players 4 +bot all
  paint`, then `record a 600`; wait for `record: saved` without touching the
  window; `play a`, and wait for `MATCH`.
- **Record what you saw.** `+logfile` (or `logfile <path>`) and `status` write
  positions and move states to a file; `screenshot` keeps the picture.
- **Your own review.** Write `%LOCALAPPDATA%\ColourWars\debug\my_review.cfg`
  (copy a shipped one), then `ColourWars.exe +exec my_review +quit`; or bind it,
  `bind ctrl+f5 exec my_review`, and press it in a match.

## Release behaviour

Release builds (`x64-release`, and therefore the ZIP) contain no console, debug
menu, registry, commands, overlay drawing, screenshot code or key binds: the gate
`COLOURWARS_DEV_TOOLS` is derived from `_DEBUG` in `game/debug/dev_tools.h`,
never from a project define, so the acceptance probes see the same class layouts
as the game. In Release:

- `+` arguments are ignored; if any are present, one line goes to stderr:
  `developer arguments ignored: Release build`. Without them nothing is printed.
- The grave key, F2-F9, F11 and the pad chord do nothing (pad Back still
  toggles F1, Start still pauses). There is no debug clock: `GameLevel::update`
  runs its own `step_match` (one `Level::update` through the shipping
  instant-replay recorder) once a frame, because the director's step hooks are
  inline and answer one step with no developer seat filter. No script, bind,
  history or autoexec file is read or written.
  F10 does nothing either; like a lone Alt, it no longer enters the system
  menu loop, because the engine treats both as plain keys
  ([engine report 11](review/engine-reports.md#11-f10-and-a-lone-alt-belong-to-the-system-menu-with-no-opt-out--accepted-narrowed-0f3b283)).
- What does ship, by design, so no class layout depends on `_DEBUG`: the
  `DebugPageTarget` interface and `DebugMatchTarget` with its three bases,
  `DebugMatchView`, `DebugMatchControl` and `DebugMatchEdits` (their RTTI names
  and vtables), `GameLevel`'s small `debug_*` adapters and the pages'
  `debug_start_match`/`debug_dismiss`/`debug_reset_input`, all unreachable
  (the match step is not among them);
  `Level`'s overlay member and its developer setters, `Player`'s two cheat flags
  and `place_at`/`force_respawn`, never called; `LevelLoadedInfo::stations()`
  and its validation, which are level data and correct in both;
  `directory_consts::screenshot_directory()`, `replay_directory()` and
  `debug_directory()`; phase 4's `Level::seat_labels_` (always empty there),
  `set_seat_labels`, `opening_cards`, `projectile_count` and
  `collision_object_count`, `InterfaceGameplay`'s label overload (the slot
  overload forwards to it), `debug_countdown_over`, `debug_set_seat_labels` and
  `debug_screen_obstacles` with their `GameLevel` adapters, and
  `DebugDirector::begin_match_draw`, an inline no-op; the
  `SeatInputFilter` interface, also used by the shipping `MatchBotDriver`, and
  the developer `seats` parameter of `Level::update`, always null in Release
  (shipping bots are owned by Level); `DebugMatchView::debug_level`;
  `pad_seat_missing`, whose "driven by a bot or replay" half is always false;
  `DebugDirector` itself, one null pointer with inline no-op hooks
  (`match_suspended`, `drives_slot` among them) and four inline step hooks
  that say one step, no filter; and the one stderr line above.
- `scripts/acceptance/debug_console.cpp` checks both ways: the Debug executable
  must contain the RTTI names `.?AVDebugConsoleState@@`, `.?AVDebugRegistry@@`,
  `.?AVLambdaCommand@@`, `.?AVDebugMenuPage@@`, `.?AVNumberVariable@@`,
  `.?AVBot@@`, `.?AVPaintBot@@`, `.?AVBotDriver@@`, `.?AVReplayRecorder@@`,
  `.?AVReplayPlayer@@`, `.?AVTuneTarget@@`, `.?AVTuningTarget@@`,
  `.?AVDebugWait@@`, `.?AVUpdatesWait@@` and
  `.?AVMatchWait@@`, and the Release executable must contain none of them;
  in Release it also checks that the director supplies no developer filter or
  driven physical slot. `SeatInputFilter` remains present for shipping bots.
  CI also checks the packaged
  `ColourWars.exe`.

## For developers

- Code: `game/debug/`. The director and its Debug-only implementation
  (`debug_director.*`, the facade; `debug_director_impl.*`, its core;
  `debug_director_hooks.cpp`, the match and page hooks and the queued
  requests); the registry, variables and
  commands (`debug_registry.*`, `debug_variable.*`, `debug_command.*`); the
  commands themselves (`game_commands.cpp` for match, time, content and
  console, `debug_status_text.cpp` for the `status` lines, the status strip
  and the paint readout, `view_commands.cpp` for the overlays, `player_commands.cpp` for
  goto, respawn, god and ammo); the clock, the match planner, the text field
  and the console state; the pad chord (`debug_chord.*`); the debug menu
  (`debug_menu_model.*`, CPU-testable, and `debug_menu_page.*`, a `MenuPage`);
  the pane overlays (`debug_overlay_draw.*`); the screenshot
  (`debug_screenshot.*`). Phase 3: live tuning (`json_spans.*`, a span scanner
  over rapidjson's SAX reader; `tune_document.*`, the baseline, current text and
  edits of one file and the splice `tune.save` writes; `tune_target.*`, which
  builds the real reader's object and copies it over the live one;
  `tune_scope.*`, the scope table, steps and path tree; `tune_commands.cpp`);
  bots (`bot.*`, the deliberately OOP `Bot -> WalkBot -> JumpBot -> ChaseBot /
  PaintBot` hierarchy; `bot_driver.*`, the seat filter that runs them);
  record and play (`replay.*`: the file, the input codec, `ReplayRecorder` and
  `ReplayPlayer`; `match_fingerprint.*`; `replay_commands.cpp`, which also holds
  the update hooks that count updates, repeat recorded suspends and resumes and
  compare).
  Phase 4: scripts (`debug_script.*`, the pure parser; `debug_wait.*`, the
  `DebugWait -> UpdatesWait / FramesWait / MatchWait / ReplayWait / RecordWait`
  hierarchy; `debug_script_runner.*`, the frame stack the director ticks from
  its hooks; `script_commands.cpp`, which also holds history, the live
  console, the `screenshot.dir` and perf commands and `draw_status`;
  `bind_commands.cpp`, the bind commands and `binds.cfg`; `debug_files.*`, the
  console's atomic file reads and writes); the key names
  (`debug_keys.*`) and the bind table (`debug_binds.*`); the
  console's editing and drawing, shared by the frozen and live consoles and
  the panels (`debug_console_view.*`; `debug_console_state.*` is now a thin
  state around it); the perf ring (`debug_perf.*`). The shipped scripts are data
  in `scripts/debug/`.
- Menu metadata is declared where each entry is registered
  (`DebugMenuTraits`: category, form - hidden, run, pick or variable - the
  picked argument and its prefix, and whether it is staged).
- Hooks:
  - `GameLevel::update` calls `before_match_update` (debug menu toggle,
    console key, pending screenshot, the live console or the binds, a script
    tick, queued requests), then `filter_match_inputs` on the inputs it read
    (the chord's pad edges; the keyboard seat while the live console is open
    and on the frame it closes), then runs its match steps: `match_steps`
    says how many (the clock's 0..4), `before_match_step` hands each step its
    inputs and seat filter, or stops a fast-forward at a finished round,
    `GameLevel::step_match` runs it, and `after_match_step` times it and stops
    the fast-forward where a replay ended or a waiting script asked;
    `end_match_steps` syncs the audio after a step on the paused clock.
  - `GameLevel::draw` calls `begin_match_draw` first and `draw_status` last
    (the paint, perf and strip panels, placed clear of
    `debug_screen_obstacles()`, and the live console).
  - `GameLevel::build_and_enter_level`, the one build path (first build, pause-
    and end-menu restarts, debug rebuilds), calls `before_level_build(match)`
    with no level alive - it applies reloaded content, adopts a plan queued
    under a menu's own restart, and consumes a queued restart - then
    `round_seed(drawn)` for the round seed the level is built from (a replay's
    own, else `seed`'s, else the one the game drew; an override re-seeds
    `rand()` with it), and `after_level_build(match)` once the level exists,
    which re-applies overlays and cheats, applies a goto stored before the
    match, and resets the clock's partial step.
  - Every `MenuPage` runs `before_page_update(page, dt)` before its
    `update_page`, except that raw keyboard capture calls `capture_input_frame`
    first and bypasses ordinary developer work. The page is a `DebugPageTarget`,
    which is how the director starts a match, dismisses a page or resets its input. It also ticks
    scripts there (not under the debug menu) and closes a live console.
  - `before_match_step` hands the step the seat filter in force (a replay,
    else a recorder wrapping the bots, else the bots, else none), which the
    shipping tape recorder forwards to `Level::update`, and `after_match_step`
    counts Level updates since the build.
  - The director holds the match as a `DebugMatchTarget`, which is three
    interfaces by client: `DebugMatchView` (read-only: settings, the level,
    status, stations, paint, countdown and panel obstacles), `DebugMatchControl`
    (rebuild, adopt, leave, audio sync, background changes) and
    `DebugMatchEdits` (overlays, player text, cheats, place, respawn, kill,
    seat labels). Helpers that only read take a `DebugMatchView`.
  - `GameLevel` calls `match_suspended` after each of its `Level::suspend`
    calls (covered, deactivated, and an audio sync while covered or inactive);
    an audio sync for the paused debug clock alone calls `Level::pause_audio`,
    which pauses the voices without touching the players, and is not a
    suspend. It asks `drives_slot` through `pad_seat_missing` before pausing
    for a pad (the update's check, the pause page's "Controller lost" and the
    restart confirmation's fallback).
- Tests: `scripts/acceptance/debug_console.cpp` and its checks in
  `scripts/acceptance/debug_console/`, one file per group (tokenizer and launch grammar,
  registry and completion, clock, planner, text field, director, the match
  hooks against a fake `DebugMatchTarget`, the chord and input filter, menu
  traits, the debug menu model, unwinding against a fake `DebugPageTarget`,
  overlays and cheats, goto, the screenshot name and PNG writer, and the
  course's stations against its geometry; phase 3: the span scanner on both
  content files and on CRLF, escapes, nested arrays, repeated keys and malformed
  text, the scope table against the files' paths, the consumed-leaf check,
  `tune`/`tune.diff`/`tune.reset`/`tune.save` on scratch copies (CRLF and LF,
  conflicts, `force`, no source tree), tune completion and the menu tree, the
  bots' determinism and the missing-pad rule, the seeded planner, the replay
  codec and file, and record/play on a real `Level` built without a device;
  phase 4: the script parser, the runner through the real hooks (every wait,
  exact at x4 and paused, errors with file:line, nesting, `exec.stop`,
  autoexec first, the screenshot barrier, a script under a replay), the shipped
  scripts followed and linted, key names and the bind table with `binds.cfg`,
  `history.txt`, the live console's input blanking, the perf ring, panel
  placement, the staged-rebuild note, seat labels and the stale tune folders,
  all on scratch folders; CPU only, and the real content files are checked
  byte-for-byte unchanged). In
  Release it checks the one stderr line for `+` arguments, silence without, and
  that the chord, unwinding, seat filter and driven slots are inert.
  `scripts/check-debug-console.ps1` is an opt-in local check that drives the
  real Debug executable, mostly through scripts it writes into each launch's
  isolated profile (`exec`, `+exec`) and the transcript's `echo` markers, with
  key events only where keys are the point: the console, F9 and F1-F4, the
  F10 debug menu, staging and Apply, Escape into the pause menu, and a lone
  Alt tap that must not freeze the game (informational). In order:
  phases 1-2 (`check_drive.cfg`: goto, status, restart+goto, cheats, the
  screenshot command; `check_menus.cfg`; a 1280x720 launch paused in the
  countdown for the panels and the debug menu); phase 3 (`-Phase3Only`: a
  four-seat paint-bot match, live tune, record and play to `MATCH`, the tuning
  tree); phase 4 (`-Phase4Only`: `+exec review_feedback_ab +quit` with an
  autoexec, 18 PNGs and nine checksums; `station_tour`; `bots_4p`; Shift+F5 on
  a bind; a tune edit that leaves nothing in an isolated TEMP; the live
  console; perf; binds and history across a restart). It saves screenshots
  throughout. `-SaveToSource` adds a
  `tune.save` on the real `game/content/tuning.json`: it refuses unless git
  sees the file clean, checks with git that exactly one value changed, and
  writes the saved original bytes back (a `git checkout` would not restore them:
  it renormalises a working copy that mixes CRLF and LF). The pad chord needs a
  pad and is covered on the CPU side only. Its Win32 key and screenshot driver
  and its launch, focus and transcript helpers are in
  `scripts/check-debug-console/driver.ps1`, which it dot-sources.
- The engine reports this forced are all closed: 9 (per-frame hook and input
  capture, `f1e5244`), 10 (frame capture, `ac58ca1`), 11 (system-menu keys,
  `0f3b283`), 12 (live tuning: candidates are parsed in memory by
  `parse_json`, `3af746b`, and source spans stay this tool's by the engine's
  decision), 13 (key names, `2851ad9`) and 14 (frame statistics, `7344610`),
  in [engine-reports.md](review/engine-reports.md).

## Not done

Each item with an open-work row names it; the first two are deliberate and have
none.

- **Loops, conditions and variables in scripts** - deliberately. A script is
  data (which frames, which stations); anything that needs logic is a command.
- **A live console over the menus**, and **binds in the menus**: menus have no
  simulation to keep running, and F5 (`restart`) means nothing there.
- **Typing in the live console while the keyboard player keeps playing**: the
  whole keyboard seat is blanked (`DBG#4`). The keys you type are the keyboard
  seat's play keys, and nothing can tell typing a key from playing it; the
  engine declined input capture as game policy (report 9), so a per-key gate
  would be this client's own and is not built.
- **Further colour fields, arbitrary strings, fonts and absent keys in `tune`**,
  and inserting keys in `tune.save`: the fifty-eight listed colour fields above are
  implemented; see [the remaining scope](debug-console-record.md#remaining-live-tuning-work-dbg1) (`DBG#1`).
- **Replays across builds or machines**, across step sizes, and across rounds
  (a recording stops at the end of the round) (`DBG#2`).
- **A driven check of the pad chord and of a bot on an unplugged pad seat**:
  this machine has no pad; both are covered on the CPU side (`DBG#7`).

## Experimental paint underfoot (`POL#36`)

The prototype is disabled in shipped tuning. In an isolated Debug match:

```
tune tuning.player.paint_underfoot.enabled
tune tuning.player.paint_underfoot.enabled on
tune tuning.player.paint_underfoot.enabled off
```

The stored opt-in trial factors are own speed `1.10`, own ammunition recovery
`1.25`, enemy speed `0.85`, and enemy recovery `0`. Their full paths are
`tuning.player.paint_underfoot.own_speed_multiplier`,
`tuning.player.paint_underfoot.own_ammo_regen_multiplier`,
`tuning.player.paint_underfoot.enemy_speed_multiplier`, and
`tuning.player.paint_underfoot.enemy_ammo_regen_multiplier`. They are live
numeric fields; `tune.reset` restores file tokens. Do not use `tune.save` for an
unattended experiment. There is no new special, damage or jump boost.

Two switches only act while the prototype is enabled:

```
tune tuning.player.paint_underfoot.enemy_blocks_health_regen on
tune tuning.bots.refill_off_enemy_paint off
```

The first (shipped `false`) pauses health regeneration, and its delay, while a
player stands on enemy paint. The second (shipped `true`) sends a refilling bot
back off enemy paint and holds it still until its reserve resumes; turn it off to
see the unaware bots.

A completed confirmed flat top contact is sampled at the foot center by team;
the selected fixed scored slabs use their actual support corner in the local
top frame. The result is used consistently by movement and ammunition on the following physics step.
Conflicting owned overlaps are neutral. Air, unscored ramps, walls, ceilings and reset or
dead players do not retain terrain context. Capture credit and colour palettes
do not select the effect. See the [owner's timing and geometry contract](polish-plan.md#experimental-underfoot-prototype).

Enabled replay fingerprints include the latched/frozen team context and the
optional seat fields; disabled fingerprints/seat bytes keep the legacy form.
Tuning text still participates in content-hash refusal even when its switch is
off. An old recording made from different tuning text remains refused unless
the existing explicit force policy is used. Gameplay identity is established
separately by the frozen old-build/direct-input and report controls, not by
weakening those guards. The [owner](polish-plan.md#experimental-underfoot-prototype)
records the completed CPU, Release, disabled-identity and real-Application checks
and their observation limits. Human `POL#27`/`POL#30`/`POL#31` decisions remain open.

## Horizontal moving-state replay boundary

A Level with horizontal movers appends an optional
`final.horizontal_motion` object to replay format 3; its schema is 1. Stable
mover IDs, exact specification/phase/direction/offset and pose bits, ordered local
face ownership/scored areas and seat-ordered carrier IDs contribute to the
fingerprint. The decoder validates the optional domain strictly, including
geometry, canonical IDs, roster joins and array cardinalities. Stored offsets
remain exact even when world poses round to equal floats. These are observations,
not a world-state restore mechanism.

The Debug `moving_structure` acceptance case records and plays 240 real updates
through Recorder/Player on two independently constructed seeded Levels, checks
all four checkpoints, final state and terminal RNG, and rejects malformed tapes.
Existing static encodings remain byte-identical in the retained old/new controls.
Public builder/editor construction and seeded stage-loader rebuilds are now
qualified in the [capability record](feature-roadmap.md#horizontal-movement-capability-2026-10-26).
The 240-update case above remains a manual fixture; ordinary `play` cannot
reconstruct that unregistered fixture. The separate public Turbulence route
proofs use decoded stage/setup/seed factories, with their exact countdown and
playback limits recorded in the [arena owner](polish-turbulence.md).
