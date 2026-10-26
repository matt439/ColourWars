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

# A shipped review, start to finish, then exit (see Scripts)
ColourWars.exe +exec station_tour +quit
```

`players` and `bot` need `stage` beside them: on their own they are stored for a
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
| `seed [n\|off]` | fix the random seed of the next build (see [Seed, record and play](#seed-record-and-play)) |
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
[remaining live-tuning work](#remaining-live-tuning-work-dbg1).

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
[remaining live-tuning work](#remaining-live-tuning-work-dbg1).

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
- A bot's input replaces the seat's after the device and the mouse aim are
  resolved, through a seat filter `Level::update` takes as a parameter
  (`game/objects/seat_input_filter.h`).
- Bots draw on their own random numbers (splitmix64 from the seed and the seat),
  never `rand()`: with `seed` set, the same match plays out the same way every
  time.
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

Reach uses current tool/projectile tuning for a stationary horizontal launch
over at most 60 fixed updates (about one second), plus muzzle/body/collision
extent. It is an engagement heuristic, not a hit-range guarantee: gravity,
inherited velocity, diffusion, target motion and terrain are not solved.
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
`rand()` (the random palette, the menus' random weapon and stage), the
planner's `stage random` and `weapon all random` (re-seeded for each plan, so
every one draws the same), and the bots. `+seed 7` before `+stage` makes
the whole launch repeat. `seed` alone prints it.

**`record [name] [updates]`** records every seat's input on every logic step,
from a fresh build: in a match it restarts the match (and closes the console),
in the menus it waits for the next match. It stops and saves after `updates`
Level updates (the 3 s countdown is 180 of them), at the end of the round, when
the match is rebuilt or left, or on `record stop` (a bare `record` while
recording stops too). The file is `%LOCALAPPDATA%\ColourWars\replays\<name>.json`
(default name `replay-YYYYMMDD-HHMMSS`): the stage, roster (device slots, teams,
weapons), best-of, the seed, the cheats, hashes of both content files
and the live tune edits, the step size, one run-length list of inputs per seat
(floats as bit patterns), where the match was suspended, a state hash every 60
updates and the final state.

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
  player's jump buffer outside the input stream. The recording notes the update
  of every suspend and the replay suspends there too. A suspend during playback
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

The acceptance case follows every shipped script from the top-level ones
through each `exec` with the arguments it passes, and checks every statement's
command and word count and every stage, bot and station it names.

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
  existing omission. A bare bind on the effective saved Jump key also warns:
  it does both. The Keyboard Jump page warns about an existing bare bind on
  its pending key. These are warnings, so the bind and gameplay source both
  remain available. See the [keyboard Jump slice](feature-roadmap.md#action-bindings-gap42).
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

While the Keyboard Jump page captures a physical key, it owns input before
developer page hooks run. Grave/F10/F11 presses and the pad Back+Start chord
cannot open developer UI or leak into it afterward. Capture discards queued
scripts, match/leave requests, goto, screenshots, reload content and armed
record/replay work, with one cancellation notice. It retains active
recording/replay, binds and current settings. A consumed pad chord must release
both buttons before it can start another session. The developer hooks and
collision query are inert in Release; keyboard Jump rebinding is available there.

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
  toggles F1, Start still pauses); the clock always runs one update a frame. No
  script, bind, history or autoexec file is read or written.
  F10 does nothing either; like a lone Alt, it no longer enters the system
  menu loop, because the engine treats both as plain keys
  ([engine report 11](review/engine-reports.md#11-f10-and-a-lone-alt-belong-to-the-system-menu-with-no-opt-out--accepted-narrowed-0f3b283)).
- What does ship, by design, so no class layout depends on `_DEBUG`: the
  `DebugMatchTarget` and `DebugPageTarget` interfaces (their RTTI names and
  vtables), `GameLevel`'s small `debug_*` adapters and the pages'
  `debug_start_match`/`debug_dismiss`/`debug_reset_input`, all unreachable;
  `Level`'s overlay member and its developer setters, `Player`'s two cheat flags
  and `place_at`/`force_respawn`, never called; `LevelLoadedInfo::stations()`
  and its validation, which are level data and correct in both;
  `directory_consts::screenshot_directory()`, `replay_directory()` and
  `debug_directory()`; phase 4's `Level::seat_labels_` (always empty there),
  `set_seat_labels`, `opening_cards`, `projectile_count` and
  `collision_object_count`, `InterfaceGameplay`'s label overload (the slot
  overload forwards to it), `MenuPage::shows_brand` (true for every Release
  page), `DebugMatchTarget`'s `debug_countdown_over`, `debug_set_seat_labels`
  and `debug_screen_obstacles` with their `GameLevel` adapters, and
  `DebugDirector::begin_match_draw`, an inline no-op; the
  header-only `SeatInputFilter` interface (nothing in Release derives from it,
  so it has no vtable or type information there) and the `seats` parameter of
  `Level::update`, always null; `DebugMatchTarget::debug_level`;
  `pad_seat_missing`, whose "driven by a bot or replay" half is always false;
  `DebugDirector` itself, one null pointer with inline no-op hooks
  (`match_suspended`, `drives_slot` among them); and the one stderr line above.
- `scripts/acceptance/debug_console.cpp` checks both ways: the Debug executable
  must contain the RTTI names `.?AVDebugConsoleState@@`, `.?AVDebugRegistry@@`,
  `.?AVLambdaCommand@@`, `.?AVDebugMenuPage@@`, `.?AVNumberVariable@@`,
  `.?AVBot@@`, `.?AVPaintBot@@`, `.?AVBotDriver@@`, `.?AVReplayRecorder@@`,
  `.?AVReplayPlayer@@`, `.?AVTuneTarget@@`, `.?AVTuningTarget@@`,
  `.?AVSeatInputFilter@@`, `.?AVDebugWait@@`, `.?AVUpdatesWait@@` and
  `.?AVMatchWait@@`, and the Release executable must contain none of them;
  in Release it also checks that a match update gets no seat filter and that no
  slot is driven. CI also checks the packaged
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
  the update hooks that count updates, inject recorded suspends and compare).
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
    and on the frame it closes), then `update_match` (which stops a step's
    fast-forward where a waiting script asked, and times each update).
  - `GameLevel::draw` calls `begin_match_draw` first and `draw_status` last
    (the paint, perf and strip panels, placed clear of
    `debug_screen_obstacles()`, and the live console).
  - `GameLevel::build_and_enter_level`, the one build path (first build, pause-
    and end-menu restarts, debug rebuilds), calls `before_level_build(match)`
    with no level alive - it applies reloaded content, adopts a plan queued
    under a menu's own restart, and consumes a queued restart - and
    `after_level_build(match)` once the level exists, which re-applies overlays
    and cheats, applies a goto stored before the match, and resets the clock's
    partial step.
  - Every `MenuPage` runs `before_page_update(page, dt)` before its
    `update_page`, except that raw keyboard capture calls `capture_input_frame`
    first and bypasses ordinary developer work. The page is a `DebugPageTarget`,
    which is how the director starts a match, dismisses a page or resets its input. It also ticks
    scripts there (not under the debug menu) and closes a live console.
  - `update_match` passes `Level::update` the seat filter in force (a replay,
    else a recorder wrapping the bots, else the bots, else none) and counts
    Level updates since the build.
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
  implemented; see [the remaining scope](#remaining-live-tuning-work-dbg1).
- **Replays across builds or machines**, across step sizes, and across rounds
  (a recording stops at the end of the round).
- **A driven check of the pad chord and of a bot on an unplugged pad seat**:
  this machine has no pad; both are covered on the CPU side.

## Background script work (`DBG#6`)

**Implemented and qualified, 2026-10-26.** `script.background [0|1]` is a Debug-only,
session-only opt-in, default `0`. The pinned native loop already ticks without
focus; the client coordinates its own script, match and suspension gates.
The opt-in permits automation in an ordinarily inactive, shown window. With it
enabled, hidden, minimized and power-suspended windows remain held. It never marks the real
Application active or admits physical gameplay/menu controls while inactive.

Truthful connection checks, manual/covered pauses and existing resume countdowns
remain authoritative. Ordinary opt-in focus loss avoids gameplay-buffer and
replay-suspend mutations; disabling it while inactive restores the normal hold.
Music/SFX are muted before background simulation without changing saved
preferences, current preferences restore on actual activation, and rumble output
stays zero. New inactive matches and script-driven rebuild/menu/quit transitions
obey the same policy. Normal focus behavior and Release remain unchanged.

For an unattended developer match in a shown, ordinarily unfocused window:

```powershell
ColourWars.exe +script.background 1 +stage movement_course +bot all paint +wait 300 +quit
```

The command without an argument reports the current choice. It is not saved.
Physical inputs are cleared after mouse-aim resolution, before bots/replay
supply their effective inputs; connection facts are retained. Buffered physical
edges from a zero-run foreground tick are discarded without changing clock
pacing or pending manual steps. Disabling the mode during an inactive match
holds the next script statement and queued match action until activation.
Default-off menu-side script handling keeps its earlier behavior.

Both game presets build under `/W4 /WX`. The focused `debug_console
--background-only` case passes in Debug and Release, including all-field
post-mouse neutralization, driver ordering, held edges, both disable-batch
orders, command validation, existing clock/script/bot/replay regressions,
shared layouts and Release command/RTTI exclusion. Nine relevant existing
consumer cases also pass per preset.

The Debug `RumbleOnly` supplement passes twenty cases through actual GameLevel
and StateContext, with injected window facts and synthetic activation. It
records and replays 420 updates, checks the final fingerprint and neutral
non-bot tape channels, pre-update Music/SFX gains, every motor intent and
unchanged private Save bytes. It also checks native private-HWND visibility,
minimize and message forwarding/lifetime, including synthetic power messages.
The existing Rumble checks pass in both presets; no extra screenshots are added.

Receipts are under `out/validation/overnight-20261026/phase20/`:
`background-cpu-*-v2.log`, `regression-*` and `background-native-v4/` (Debug
`30d7b44643864a248f5fb8d9d48e252b`, Release `b014cdcd1d36467a81a2817885dc293a`).
Both published-bank SystemOnlyPath startups also pass with isolated saves,
per-monitor v2 DPI awareness and graceful shutdown: Debug
`99c565c8260f455e9ea69b4f1c664180`, Release `378b21cc7e2046398367b2f154a65ce2`.
Failed native receipts are retained: v1 had a diagnostic-string compile error;
v2 exposed post-neutralization mouse aim and prompted the production fix;
v3 drove the reconnect menu with the wrong pad, corrected to its actual owner.
These controlled results do not establish attended OS-focus/power-sleep,
physical-pad, listening or performance acceptance, and do not satisfy `CI#1`.

## Remaining live-tuning work (`DBG#1`)

**DEFERRED (readiness audit, 2026-10-26).** No additional field/caller is selected;
resume when an implementation task needs one. General string/font edits and
missing-key insertion must first get their own bounded scope. The existing
fifty-eight colour paths listed under [Live tuning](#live-tuning)
support read, validated apply, diff, reset and save: the initial seven, both
readiness colours on Team Select and Weapon Select, the weapon-feedback pair,
the opaque player-identity ink/background pair, the combat-feedback pair and
the blocked-shot highlight/keyline pair, the common idle-drip keyline,
respawn pen colour, cursor ink/fill/shade/shadow, silhouette body/ink/rim/shade and reveal
ink/paper/shade, plus desktop, monitor power-light, program, acting face/sweat, glass, scribble, crack, HUD health/track and shared label foreground/identity shadow, captured Stage Select preview backing, normal timer foreground/shadow, opaque closing text and panel ink/paper/accent, positive-alpha closing shade, shared HUD paper ink/paper/shade, and figure arm/hand and quiet blink face colours.
General string/font edits and insertion of absent keys remain separate scoped
implementations. No additional colour field is selected after this family.

A future selected colour increment must follow an actual field's consumer,
add that field explicitly and report when it is adopted: live, new objects,
rebuild or next screen. Numeric path wildcards do not establish a colour's
adoption scope. Keep the supported list tied to those audits rather than
exposing arbitrary strings by their spelling. General string/font edits and
missing-key insertion need their own scoped implementations. Further consumer
audits are optional if selected: choose an actual field and caller before
implementation. An inventory is not a mandatory backlog or a target count.

Every extension must validate the complete candidate through `Presentation`
and its existing resource checks before applying it; a rejected edit changes
neither live content nor the diff.

Acceptance: valid named/hex colours apply and round-trip through diff/reset/save;
invalid values leave state intact. Preserve surrounding source bytes, line
endings and unrelated external edits, existing conflict/force rules and atomic
replacement behavior. Check captured versus reopened/rebuilt consumers and replay
edit restrictions. Keep numeric/boolean behavior and Release developer-tool
exclusion intact. These checks use scratch content and isolated profiles.

The local RapidJSON span reader and in-memory candidate validation
(`labrador::parse_json`, engine report 12) already provide the required route.
The engine declined to keep source spans, so the span reader and splice stay
this tool's.

Shared HUD paper material family (2026-10-24), **verified**:
paths 56-58 select `presentation.hud.paper.ink`, `.paper` and `.shade`
as live borrowed colours. Only three exact allowlist rows change production.
The existing reader requires distinct opaque ink/paper and positive-alpha
shade; no additional contrast or shade-equality restriction is introduced.
Authored `141414` / `FFF5DF` / `00000055`, drawing, full-RGB shadows,
alpha-scaled inset bands, layout and resources remain unchanged. The actual
callers are the identity panel, the combined HEALTH/PAINT panel and the
ordinary-timer panel; ink also reaches the two direct meter borders. The
separate closing-clock region and root identity badges keep their own materials.

Debug and Release game/editor builds and all four focused `debug_console`
and `story_audio` CPU cases passed on their first actual attempts.
The qualified private core is v2: its named-colour expected-value oracle and
stale count comment were corrected during source review before any runtime.
No failed build or CPU run preceded this qualification. Evidence is under
`out/validation/hud-paper-colours-20261024/` in `core-result-v1.json`.

Independent 58-path fixtures preserve the earlier contracts. Isolated ink/paper
LF/CRLF cases use valid distinct tuples, exact one-token and two-token saves,
external-byte adoption, conflict/force, full-reader partial/zero/equality
refusals, and rebase/reset. A selected reset which would equal its pending
sibling refuses with both edits intact. Separately valid disk/current pairs
which would become equal on Save or force Save also refuse, including an
unedited counterpart and external selected value after selected reset.
Shade retains AA01/partial/FF success and zero refusal after selected reset
while sibling/overlay ledgers remain. Retained HUD metadata covers all three
new subjects, complete 58-value guards, borrowed records, geometry and captured
sheet/frame/font measurements. Direct loaded-resource/missing-frame checks
remain separate from no-Application full-reader commands. Record/replay
restrictions and Release developer exclusion remain covered. CPU results
establish neither rendered pixels nor human visual acceptance.

Native Debug and Release passes succeeded on their first actual attempts,
reusing the qualified core producer binaries without repeating game builds.
Release records 64 readbacks / 29 rows / 18 crops; Debug totals 100 / 43 / 18.
The shared consumer checks 35 material sites (560 sampled pixels) across the
three paper panels and two extra ink borders. Twelve counterexample controls
produce 202 site discriminations and 3,232 differing pixels per preset.
Independent underlays, repeat/copied adoption, normal/reduced and
settled/reduced comparisons, ordered bands and detached-tick counterexamples
pass. Closing-branch checks isolate the separate closing-clock region while
positive full-HUD changes confirm that identity/meters still adopt shared
paper colours; no whole-HUD closing invariance is claimed.

Debug adds 36 readbacks and 14 rows: nine accepted rendered states and five
refusal rows. Its twenty statements include fifteen accepted commands
(three queries, seven tunes, four resets and one Save) and five refusals.
The genuine three-token Save/rebase and original/saved resets preserve
independent sibling edits. Equal ink, equal paper, partial ink, partial paper
and zero shade refuse atomically in typed state, each followed by one
held-valid-frame read. Refusal reads do not count as accepted states, new raw
references or copied adoption; zero is not an accepted shade palette.
The Debug state checks include 5,040 patch checks. Observation update/input
poll and game Save counters remain zero.

The eighteen ordinary-HUD crops show six palettes across identity, meters and
timer panels: 898,584 pixels and 2,696,022 P6 bytes per preset. A bounded montage
of those panels was reviewed for artifact QA; the closing region has no
exported crop or human acceptance claim. Reconciliation passes all 1,498
frozen source pins, eleven prior fixtures, qualified producer/result hashes
and the preserved 4795ca4 ZIP. Twenty-two shared raw files and eighteen PNG
pairs are byte-identical between presets. Independent PNG CRC/filter decoding
also confirms full RGB parity with each PPM payload.

Evidence is under `out/validation/hud-paper-colours-20261024/` in
`native-result-v1.json` and `validation-reconciliation-v1.json`.
There were no native runtime failures or runtime source corrections.

The preserved 4795ca4 ZIP predates these changes. Human contrast/readability,
comfort, C2 recognition, arena/weapon balance, physical controls, listening,
clean-PC and performance acceptance remain open. `DBG#1` remains open.
No further unattended field is selected. A new automated slice requires a
concrete observation or bounded owner requirement; further colour audits are
optional if selected. This does not close the remaining TODOs. The dated
checkpoints below preserve their original scope and next-step statements.

Positive-alpha closing-clock shade increment (2026-10-24), **verified**:
path 55, `presentation.hud.closing_clock.shade`, has an exact live row.
That allowlist row is the only production change. Its existing positive-alpha
reader accepts names/RGB and RGBA alpha01 through FF, refuses zero, and imposes
no opaque-only, contrast or equality rule. Authored `00000055`, both base
shadows, six RGB-preserving alpha-scaled bands, geometry, resources and
rendering remain unchanged.

Debug and Release game/editor builds and all four `debug_console` and
`story_audio` CPU cases passed on their first attempts. Independent 55-path
arrays and a separate positive-alpha classifier retain the 54 earlier
contracts. Nine retained-HUD metadata subjects guard borrowed records, colours,
geometry, fonts/resources and RNG without drawing or advancing a match.

Actual commands accept named/RGB/partial shade, alpha01/FE/FF boundaries and a
shade equal to ink. Black and nonblack zero refusals preserve complete live,
document, ledger and file state. Exact escaped spans, one-token and shade/paper
pair saves, selected/sibling resets, LF/CRLF conflict/force and rebase remain
covered. External selected zero after reset cannot be adopted while other
ledger entries are saved. Valid-positive recording/replay write attempts
prove policy refusal without relying on an invalid zero input. Separate direct
complete metadata resource validation and missing-frame refusal remain distinct
from no-Application full-reader commands. Release developer exclusion passes.

Both native presets passed on their first actual attempts, without a runtime
failure or runtime source correction. Release uses **68 readbacks, 30 rows and
12 native 202 x 100 crops**; combined Debug totals are **93/39/12**. Each preset
has 242,400 crop pixels and 727,380 P6 bytes. Complete decoded PNG RGB matches
the PPM payloads, with opaque alpha, for all twelve crops.

Eight material sites cover 128 pixels: both base shadows and one/two/three-band
depths in each direction. Two controlled underlays and four valid palettes,
including minimum-positive alpha01, retain full author RGB and repeated
source-over bands. Twelve independent patch observations compare 1,536 RGBA
pixels. Eight wrong-reference controls produce all 48 site discriminations
and 768 pixel inequalities per preset. The low positive input is valid without
a universal visible-contrast guarantee.

Twenty-two full-frame repeats and eight copied HUD comparisons establish
retained drawing/adoption separately from those material references. Five
normal-branch invariances, paired normal/reduced and settled/reduced relations,
and two beat counterexamples retain applicability. Band pixels match across
Canvases over opaque paper; exposed base-shadow relations retain the controlled
underlay distinction. The proof covers these material sites, not complete
glyph/panel reconstruction or natural Level sequencing.

Debug executes ten statements with eight accepted rendered states, one private
one-token Save/rebase and original/saved resets. Its single zero-alpha refusal
preserves typed live/document/ledger/file/resource state, followed by one draw
equal to the held valid partial-alpha frame. Zero is not an accepted visual
palette. These checks add twenty-five reads and nine rows, including
1,024 cached-reference patch checks; no extra crops or raw-reference reads
are needed.

Calibration totals are 2,176 pixel checks in Release and 2,976 in Debug.
Every observation update, input poll and game Save counter remains zero.
Evidence is under `out/validation/closing-shade-colour-20261024/`, with
`validation-reconciliation-v1.json`. All 1,496 frozen source pins, nine prior
fixtures, qualified producer/result hashes and the preserved 4795ca4 ZIP
reconcile. Existing evidence remains intact; qualified core producers were
reused for the native probes without repeating game builds.

The preserved ZIP predates these changes. Human contrast/readability, comfort,
C2 recognition, arena/weapon balance, physical controls, listening, clean-PC and
performance acceptance remain open. `DBG#1` remains open. The separate shared
HUD paper audit maps distinct opaque ink/paper, positive-alpha shade, three
helper callers and two extra meter-border ink uses. It selects no implementation
or mandatory backlog; reassess the actual report before choosing further work.

Opaque closing-clock material family (2026-10-24), **verified**:
paths 52-54 expose the existing `presentation.hud.closing_clock.ink`,
`.paper` and `.accent` as live borrowed colours. Production changes only
three exact allowlist rows. The opaque-only reader, equal-colour validity,
defaults, drawing, beat geometry, shade, layout and resources remain unchanged.
This does not expose the whole closing-clock palette.

Debug and Release game/editor builds and all four `debug_console` and
`story_audio` CPU cases passed on their first attempts. Independent 54-path
fixtures retain earlier RGBA and next-screen contracts. Span checks preserve
nested escaped tokens and exact source bytes; actual full-reader commands
enforce opaque named/RGB/RGBA-FF acceptance and atomic partial/zero refusal.
Equal opaque sibling values are explicitly valid.

Eight retained-HUD metadata subjects preserve borrowed records, fonts, resources,
geometry, other colours and RNG without drawing or advancing a match. The three
material subjects exercise cyclic opaque sibling pairs, saved and unsaved
selected resets, exact two-token saves/rebase, LF/CRLF conflict/force behavior
and replay restrictions. Invalid merged controls target an unedited opaque
neighbour, then invalid external selected alpha after selected reset, preserving
the remaining ledgers and files. Separate complete metadata resource validation
and missing-frame refusal are distinct from the no-Application command checks.
Release developer-tool exclusion passes.

Both selected native presets passed on their first attempts at hidden
1920x1080. Release uses **64 readbacks, 27 rows and 9 native 202 x 100 crops**:
181,800 crop pixels and 545,535 P6 bytes. Combined Debug totals are
**104/37/13**, 262,600 crop pixels and 787,995 P6 bytes. All nine/thirteen PNGs
have complete decoded RGB parity with their PPM payloads and opaque alpha.

Twelve flat patches cover two ink body sites, four ink tick under-strokes,
two paper body sites and four accent stripes. Eleven independent raw
observations compare 2,904 RGBA patch pixels. Six wrong-opaque or omitted
references produce all 24 per-site discriminations and 528 pixel inequalities.
Twenty-one full-frame repeats and five copied HUD comparisons establish
retained drawing/adoption separately. Five normal-branch invariances, two
isolated inactive-accent invariances, three settled/reduced equalities and one
active/settled beat counterexample retain applicability. Opaque straight/double
alpha cannot discriminate and is not claimed.

Debug executes seventeen statements: fourteen accepted operations, including
three queries and eleven mutations/Save operations, plus three atomic
partial-alpha refusals. Ten rendered states add forty reads, ten rows, four
crops and 2,640 patch checks, with ten repeat/copied/sibling checks. One exact
two-token ink/paper Save/rebase and selected resets retain saved siblings or
pending accent; equal opaque paper/accent is valid. The three structured JSON
refusal records say `raster_checked=false` and add no reads or CSV rows.
They establish atomic reader rejection, not refused-input pixels.

Calibration checks total 2,048 pixels in Release and 3,328 in Debug. Every
observation update, input poll and game Save counter remains zero. Independent
proof covers the twelve material patches; envelope crops include existing
glyph/shade art, while copied HUD frames establish adoption rather than an
independent reconstruction of that art.

Evidence is under `out/validation/closing-material-colours-20261024/`,
with `validation-reconciliation-v1.json`. All 1,494 source pins, fifteen prior
fixture pins, qualified producer/result hashes and the preserved ZIP reconcile.
The preview51 and timer46-48 bodies remain unchanged. Thirteen shared raw files
and nine PNG pairs match byte for byte between presets. Native probes reuse the
qualified core producers without repeating game builds.

The preserved 4795ca4 Release ZIP predates these changes. Human urgency,
contrast/readability, comfort, C2 recognition, arena/weapon balance, physical
controls, listening, clean-PC and performance acceptance remain open.
`DBG#1` stays open. Closing shade remains unsupported; reassess its bounded
source report before selecting any implementation. Its bands copy shade RGB
and scale only alpha, with zero alpha rejected by the existing reader.
No shade rendering correction or optional colour inventory is selected here.

Captured Stage Select preview backing increment (2026-10-24), **verified**:
path 51, `presentation.hud.player_identity.background_colour`, has an exact
`next_screen` classification. Stage Select captures one premultiplied tint at
preview-background construction. Authored `121827EE` is unchanged; correcting
its nonblack translucent RGB submission is an explicit behavior change. This
field does not colour gameplay HUD paper or tint the map. Other values, art,
layout, fonts, labels and stage data remain fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on their first attempts, including Release
developer-code exclusion. They cover actual reader/commands, complete candidate
and resource validation, atomic refusal, exact save/reset/rebase/conflict/force
and replay restrictions. Derived UiTexture metadata checks the capture model;
the actual page evidence comes from the separate native leaf.

Both selected native presets pass on their first attempts: Release has
68 readbacks, 26 rows and 16 native 64 x 32 crops; Debug has 92/38/16. The crops
cover 32,768 pixels and 98,512 P6 bytes per preset, with complete decoded PNG RGB
and opaque-alpha checks. Four palettes cover Random/Crossfeed browsing and
ready states. Eight wrong-straight/double controls produce 16,384 aggregate
pixel inequalities. Explicit opaque-edge underlays are `445268` when browsing
and `75EDD7` when ready; comparisons match those states. Retained pages keep
their captured tint through updates, cycling and ready changes; actual
Back-to-Weapon-to-Proceed reconstruction adopts the new tint. The Weapon
transition row is a navigation observation without a raw colour oracle.

Debug adds seven actual query/edit/reset/Save commands and 12 paired rendered
observations, preserving retained-page capture and exact private one-token
Save/rebase. These navigation fixtures use 63 menu updates and 249 device polls
in Release, or 97/363 in combined Debug. Positive-time, Level/player/Scene and
gameplay-weapon updates and game Saves remain zero.

Targeted StagePaper and MenuText dependent checks pass in both presets. Two
legacy preview-background references now independently premultiply their tint.
An old malformed StagePaper case reached the stronger reader before its intended
page rejection; both boundaries are now checked separately. That failure and a
private wrapper's optional-metadata error after a native pass are preserved.
Three completed dependent probes were reused by exact hashes. Only Release
MenuText was retried after a deep-path capture-write failure; identical sources
pass under a shorter output path. The original writer exposed no OS error.
No production reader, geometry, values or equality tolerance was relaxed.

Evidence is under `out/validation/stage-preview-background-colour-20261024/`,
with final `validation-reconciliation-v4.json`. Nineteen shared raw files and
16 PNG pairs match byte for byte across presets; 1,492 source pins, qualified
producer hashes and the preserved 4795ca4 Release ZIP are reconciled. That ZIP
predates these increments. Automated checks do not establish human preview
contrast, C2 recognition/readability, comfort, arena balance, physical controls,
listening, clean-PC or performance acceptance. `DBG#1` remains open. The next
selected bounded group is opaque closing-clock ink/paper/accent; its preparation
remains private.

HUD label foreground and identity shadow increment (2026-10-24), **verified**:
paths 49/50, `presentation.hud.player_identity.colour` and
`presentation.hud.player_identity.shadow_colour`, are live on retained
InterfaceGameplay draws. Foreground supplies the identity label and both
HEALTH/PAINT labels; shadow is identity-only. Three existing C++ tint arguments
premultiply base RGB once, preserving authored opaque foreground `141414` and
BLACK-alpha34 shadow `00000022`. Root badge colours, HUD background, values,
text, captured fonts, geometry, clocks and resources stay outside this pair.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on their first attempts, including Release
developer-code exclusion. Independent 50-path fixtures preserve the badge and
closing-text opacity contracts and unrestricted RGBA on this pair. Named/RGB/
partial/zero input, exact escaped/nested spans, selected reset, pending sibling
preservation, paired two-token save/rebase, conflict/force/full-candidate
refusal and recording/replay restrictions are covered. Retained CPU HUD and
resource checks are metadata evidence, not rendered adoption.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudLabelTuneColoursOnly`
passes at hidden 1920x1080 in one supplied prepared HUD context, with normal
clock at 123.25 and fixed label/caller inputs. Shared Release evidence is 39
readbacks, eight rows and sixteen native crops; Debug totals are 99/23/16.
The three independent regions are identity 380x105, HEALTH 141x26 and PAINT 108x26.
Eight observations compare 46374 RGBA pixels each (370992 comparisons). Same-font
BC2 references use literal ordered materials and explicit base channels:
identity includes its two-row shade strip, then shadow and foreground; meter
labels use unshaded paper without shadows. Copied complete-HUD parity proves
adoption, not independent full-HUD art or engine font-rasterizer correctness.

Seven negative frames provide 13 separately required region discriminations
and 66983 observed aggregate pixel inequalities. Foreground controls must change
all three regions; shadow controls change identity and preserve both meter
regions. A reversed-order control uses source-derived overlap (139,91).
Repeated draws, unselected regions, captured resources and draw RNG remain
guarded. The sixteen crops total 345096 pixels and 1035520 P6 bytes: eight
identity crops plus four of each meter label. PNG decode verifies native
sizes, opaque alpha and complete RGB parity with each retained PPM.

Debug executes 15 real statements and renders 15 states (baseline plus 14
non-Save command states), adding 60 readbacks and fifteen rows. Actual commands
cover pending sibling adoption, one two-token private Save/rebase, original
resets and saved-sibling-preserving resets with exact joint saved pixels.
CPU checks own pending-sibling reset/conflict/force variants. There are no game
Save calls, observation updates or input polls; calibration comparisons total
3168 in Debug and 1248 in Release. Eighteen shared raw files and all sixteen
PNG pairs are byte-identical between presets. Native probes were compiled
fresh while qualified game/editor builds and CPU results were retained with
unchanged producer hashes.

The first native-v1 attempt failed at an exact expected-origin guard before
ROI readbacks. A derived summary had copied an adjacent wrong float even
though its detailed source audit already recorded the correct origin. Source
arithmetic and independent review corrected that one expected literal and its
comment, with line-ending normalization. Production formulas, fonts, ROIs,
materials, equalities, controls and counters did not change; fresh native-v2
runs passed. The failed receipt, original sources and summary erratum remain.
This establishes no compiler or renderer defect. Earlier invalid case-folded
letter analysis was separately retracted before this fixture was prepared;
it did not invalidate the prior numeric clock evidence.

Evidence is under `out/validation/hud-label-colours-20261024/`.
The preserved 4795ca4 Release ZIP predates the clock and HUD-label increments.
Hidden checks establish no natural Level sequence, arbitrary label/layout/
scale, small-pane readability, C2 identity/rear-threat recognition or comfort,
physical controls, listening, clean-PC or performance acceptance.
`DBG#1` remains open. The selected next consumer is the captured Stage Select
preview backing, with next-screen adoption and an explicit default-RGB
correction; that separate field is not implemented by this result.

Opaque closing-clock text increment (2026-10-24), **verified**:
path 48, `presentation.hud.closing_clock.text`, is live on applicable closing-
clock text draws. The sole production change is its exact colour
allowlist row. The existing reader's alpha1 constraint, draw/fade behavior,
normal timer pair, other closing materials, resources and content stay fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on first attempts, including Release developer-code
exclusion. Independent 48-path fixtures preserve unrestricted RGBA on the
normal pair and scope closing text to opaque names/RGB/RGBA-FF. Actual command
partial/zero refusals retain live values, documents, ledgers and files. Exact
save/rebase/reset/conflict/force and replay checks remain; full-reader saves
also refuse invalid opaque-neighbour merges and invalid external selected text
after selected reset, preserving remaining sibling/unrelated edits.

The existing `HudTimerTuneColourOnly` leaf now qualifies all three clock
subjects at hidden 1920x1080. Release family totals are 161 readbacks, 54 rows
and twelve native 122x51 crops; Debug totals are 261/85/12. Closing-only shared
evidence is 50/18/4 and its combined Debug subtotal is 82/29/4. The twelve PPMs
total 224160 bytes. Both native presets passed on their first actual attempts.

Closing observations start with a running clock at 10 seconds and cover eight
clock/Reduce Effects contexts. Correct references use the audited opaque closing
paper and same-font opaque glyphs, with no normal timer shadow. Eight applicable
observations compare all 6222 RGBA ROI pixels. Wrong opaque tint and an added
normal-shadow control produce 3267 pixel inequalities; source witness (980,79)
must remain closing paper in the correct output. Straight/double-alpha
discriminators are inapplicable at alpha1 and are not claimed.

Closing Debug executes nine real statements: seven accepted and two partial/
zero refusals. Seven rendered accepted states, two refusal pixel checks and
two normal-branch observations add 32 reads and eleven rows. Refusals preserve
live data, both documents' baseline/current/revision/ledger, private source/
staging and resources. One-token save/rebase and exact original/saved resets,
RNG, normal clocks and the other closing materials stay guarded. No game Save,
observation update or input poll is performed.

Twenty-one shared CSV/JSON/PPM files and twelve PNG pairs are byte-identical
between presets; complete decoded PNG RGB matches the retained PPM payloads.
All fourteen prior normal-clock shared raw files, six Debug command files and
their PNGs retain the qualified path47 bytes. Native probes were compiled
fresh while qualified game/editor builds and CPU results were retained with
unchanged producer hashes. Copied whole-HUD comparisons establish adoption;
the same-font reference does not independently test engine font rasterization.

Evidence is under `out/validation/closing-clock-text-colour-20261024/`.
The preserved 4795ca4 Release ZIP predates these clock increments. Hidden HUD
checks establish no natural Level sequence, human clock/C2/arena readability
or comfort, physical-pad, listening, clean-PC or performance acceptance.
`DBG#1` remains open with further actual consumers selected by audit.

Normal HUD timer shadow increment (2026-10-24), **verified**:
path 47, `presentation.hud.timer.shadow_colour`, is live on the same retained
normal-clock consumer. One base premultiplication preserves authored BLACK
alpha34 and the independently supported foreground. The running closing branch,
font/resource capture, offsets, formatting, geometry and content stay fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass on their first attempts, including Release
developer-code exclusion. Independent 47-path fixtures and shared foreground/
shadow subjects cover named/RGB/RGBA parsing, escaped nested tokens, save/rebase,
complete candidate rejection and record/replay restrictions. Selected resets
preserve the sibling colour and unrelated pending edits; invalid merged saves
retain all three ledger entries and source/staging/current/baseline bytes.

The existing `HudTimerTuneColourOnly` leaf now qualifies both normal-clock
subjects through shared mechanisms at hidden 1920x1080. Release family totals
are 111 readbacks, 36 rows and eight native 122x51 crops; Debug totals are
179/56/8. Foreground retains its 55/18/4 shared and 89/28/4 Debug subtotals;
shadow contributes 56/18/4 shared and 90/28/4 Debug. The eight PPMs total
149440 bytes. These family runs passed on their first actual attempts.

The shadow reference composes explicit-channel shadow before the unchanged
foreground, using the same BC2 font and one effective opaque-paper backdrop.
Twelve shadow observations compare all 6222 RGBA ROI pixels. Four negative
tint/order controls produce 4552 pixel inequalities, including a reversed-order
check at source-verified glyph overlap (919,79). Defaults, nonblack partial/zero
alpha, repeated draws, unselected regions and normal/closing/Reduce Effects
boundaries remain exact. Copied whole-HUD comparisons establish adoption only.

Each Debug subject executes eight actual commands/states, with one-token private
save/rebase and exact authored/saved resets; both keep resources, input, RNG and
default preferences intact. No game Save, observation update or input poll is
performed. Fourteen shared CSV/JSON/PPM files and eight PNG pairs are byte-equal
between presets, with complete decoded PNG RGB matching each PPM. The seven
foreground shared raw files, three Debug command files and foreground PNGs
also retain their bytes from the qualified path46 native-v2 evidence.

Native qualification retained the qualified game/editor builds and CPU results,
with unchanged producer hashes, and compiled fresh native probes. Source review corrected the private wrapper's expected family-scope
label before launch; it required no producer, oracle or tolerance change.

Evidence is under `out/validation/hud-timer-shadow-colour-20261024/`.
The preserved 4795ca4 Release ZIP predates both normal-clock increments.
Automated HUD checks establish no natural Level sequence, human clock/arena
readability or comfort, physical-pad, listening, clean-PC or performance
acceptance. `DBG#1` remains open with further consumers selected by audit.

Normal HUD timer foreground increment (2026-10-24), **verified**:
path 46, `presentation.hud.timer.colour`, is live on existing InterfaceGameplay
normal-clock draws. Only its exact colour path and existing foreground tint
argument change. The shadow, closing-clock branch, font/resources, caller
inputs, formatting, geometry and authored content remain fixed.

Both game/editor builds and all four Debug/Release `debug_console` and
`story_audio` CPU cases pass, including Release developer-code exclusion.
The independent 46-path sweep, escaped nested tokens, exact LF/CRLF source
splices, one-token save/rebase, sibling-preserving selected resets and complete
invalid-merge ledger retention are covered. Recording/replay reject new edits;
retained CPU HUD/resource evidence is metadata only, with no pixel claim.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudTimerTuneColourOnly`
passes at hidden 1920x1080. Shared consumer evidence is 55 readbacks, 18 rows
and four native 122x51 crops (74720 PPM bytes). An independent same-font,
linear-filter reference uses literal placement/channels on one effective
opaque paper backdrop, retaining the separate shadow. Twelve observations
check all 6222 RGBA ROI pixels. Three straight/double-alpha negative controls
discriminate 7968 pixels. Copied full-HUD comparisons prove adoption only;
the reference does not independently test the engine's font rasterizer.

Eight source-derived contexts cover ordinary time, just above ten, exactly ten,
fractional closing time, zero, stopped short time and Reduce Effects on normal
and closing draws. Foreground edits retain the closing-clock signal exactly.
Debug adds eight actual private commands/states, 34 readbacks and ten rows,
including one-token save/rebase and exact original/saved resets; combined Debug
is 89/28/4. Fonts/resources, unselected regions, repeated draws and RNG stay
guarded, with zero observation updates, input polls or game Save calls. Seven
shared raw CSV/JSON/PPM files and all four PNGs are byte-identical between
presets; decoded PNG pixels match their complete PPM RGB payloads.

The first Debug native launch stopped at a missing probe argument registration,
before Application/window/renderer construction. Its failed receipt is retained.
Only the accepted-mode predicate and usage string were corrected and reviewed;
the unchanged oracle and exact tolerances passed in fresh native-v2 runs.
Qualified game/editor and CPU producers were reused by hashes.

Evidence is under `out/validation/hud-timer-colour-20261024/`. The preserved
clean 4795ca4 Release ZIP predates this increment. Automated checks establish
neither human clock readability/comfort nor attended Crossfeed, physical-pad,
listening, clean-PC or performance acceptance. `DBG#1` remains open.

Quiet blink face colour increment (2026-10-24), **verified**:
path 45, `presentation.figure.quiet.face_colour`, reads live through existing
FigureAttachments. The face fill is premultiplied once; the independently tinted
arm-colour lid stays later in the draw order. Authored opaque WHITE, quiet timing,
geometry, captured resources, input and simulation remain unchanged.

Both game/editor builds and the Debug/Release `debug_console` and `story_audio`
CPU cases pass, including Release developer-code exclusion. The 45-path sweep,
escaped nested spans, exact two-token save/rebase and sibling-preserving resets
are covered. An invalid merged eyelid/eye-height relationship is refused by
ordinary and forced saves without changing source, staging, live state or the
edit ledger; recording/replay restrictions remain enforced.

The existing Limb native family retains all 43/44 checks and adds a separately
counted direct quiet-face consumer. Shared face evidence is 38 readbacks,
10 rows and ten 176x24 crops: four literal mirrored eyes, 128 RGBA pixels per
case split into 64 face and 64 lid pixels, five face/arm palettes, two underlays,
and eight negative references with 512 role-pixel inequalities. Null/disabled
records, repeated draws, opaque-default parity, exterior pixels, resources and RNG stay
guarded. Debug adds nine actual commands/states, 54 readbacks and 18 rows for
the private face/arm Tune save and selected resets, with no game Save writes.

Combined readback/row/crop totals are Debug **150/41/14** and Release
**60/14/14**, including the unchanged legacy subtotals **58/13/4** and
**22/4/4**. Calibration checks are 4,800 and 1,920; all fourteen shared PPMs are
byte-identical between builds, and every PNG's decoded RGB matches its source.
The old preparation-only Weapon calls remain separate: 20 Debug and 10 Release
zero-dt calls, with no additional quiet-face preparation or observation updates.
Both native first attempts pass. Exact source/build/probe/content/image pins
are reconciled under `out/validation/quiet-face-colour-20261024/` in
`validation-reconciliation-v1.json`; native run IDs are
`bca67f7d000f4813a969ee1f6e799521` and `a3ace014888245378457d9d29d25af14`.
This proves the retained direct consumer, not natural Player blink timing,
full body artwork, 1-4P visibility, human comfort, hardware or performance.
`DBG#1` remains open for another bounded actual-consumer audit.

Figure arm/hand colour family (2026-10-21), **verified**:
paths 43/44, `presentation.figure.arm_colour` and `presentation.figure.hand_colour`,
read live with base RGBA premultiplied once; defaults, geometry, quiet face and
captured rest/sleeve layers remain unchanged.

Both game/editor builds and four [CPU cases](../out/validation/crossfeed-20261020/limb-colour-v2/build-consumer-result.json)
pass, reusing the unchanged Debug production build after hash checks.
The earlier [CPU freshness fixture](../out/validation/crossfeed-20261020/limb-colour-prep/first-CPU-failure-v1.json) and
[native WHITE-eyelid inequality](../out/validation/crossfeed-20261020/limb-colour-prep/first-native-failure-v2.json)
failures are preserved; production needed no further correction.

Corrected [native runs](../out/validation/crossfeed-20261020/limb-colour-v2/native-v2/runner-result.json)
pass: `-LimbTuneColoursOnly` gives Release 22 readbacks/4 rows, Debug 58/13,
four 1280x480 crops each. Twelve procedural regions independently compare
21142 native RGBA pixels/case; six controls retain 248 discriminating inequalities
and four exact-WHITE guards. Copied full-fixture parity proves adoption only;
rest/sleeves are invariance guards.

Ten zero-delta Weapon preparations in Release, twenty combined in Debug,
precede zero observation/game updates or input polls. Debug passes nine
commands/states, two-token Save/rebase/reset and two private PlaySettings/Jump J
seed Saves/default display.

[Cross-preset reconciliation](../out/validation/crossfeed-20261020/limb-colour-v2/native-v2/reconciliation.json)
confirms 12 shared files/four PPMs byte-identical. Four native RGB PNGs pass
13-field metadata, complete decoded RGB parity and opaque decoded-alpha checks;
[root inspection](../out/validation/crossfeed-20261020/limb-colour-prep/actual-native-evidence-check-v1.json)
covers all four palettes. Actual-Player/Level, natural blink timing,
integrated body artwork and human/hardware, contrast, comfort, listening,
balance and performance gates remain open.

Monitor crack colour increment (2026-10-21), **verified**:
path 42, `presentation.opening.crack`, reads live; one premultiplication preserves
opaque `3A3A3A`, broad-ink/thin-dab order, captured paths and resources.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/crack-colour-v1/build-consumer-result.json)
pass. Ordinary/forced merged saves reject the invalid document
(corner_step 0.002 below pen_width 0.004), preserving exact state and both
ledger keys/token pairs; selected reset retains unrelated edits.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -CrackTuneColourOnly`
passes first execution in [both presets](../out/validation/crossfeed-20261020/crack-colour-v1/native/runner-result.json):
Release 42 readbacks/8 rows, Debug 78/17; eight native contiguous 115x34 crops,
31280 pixels/93952 P6 bytes, left/right paths across four palettes.

Independent proof uses direct front-details, Canvas and calibration only.
All 3910 actual/raw RGBA pixels/case precede whole-ROI source checks; four
models are byte-equivalent, without GPU precision inference. Ten controls
check 1274 changed/37826 unchanged comparisons; alpha0 retains BLACK ink.
Calibration checks 32 WHITE/BLACK RGBA pixels/read without extra reads.
Copied complete front-details parity proves adoption only.

[Parity](../out/validation/crossfeed-20261020/crack-colour-v1/native/reconciliation.json)
retains 20 identical shared files, including eight PPMs. Eight PNGs pass post-contact
IHDR, dimensions and decoded RGB parity, with 13 flat metadata fields each.
[Inspection](../out/validation/crossfeed-20261020/crack-colour-v1/root-view-v1.json) and
[source reconciliation](../out/validation/crossfeed-20261020/crack-colour-v1/root-native-evidence-gate-v1.json) pass.

Debug: nine left-only commands/states, one `"2060C080"` crack-token Save/rebase/reset,
two private PlaySettings/Jump J seed Saves/default display; all eight update/poll
fields zero. Actual-Level, arbitrary paths/transforms, human contrast/readability/comfort,
hardware/input/listening, balance and performance gates remain open.

HUD meter-track colour increment (2026-10-21), **verified**:
path 41, `presentation.hud.meter_track_colour`, reads live on both borrowed
HUD meters. One track premultiplication preserves authored `B8B8B8`, geometry,
resources and existing health/team fill paths.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/hud-track-colour-v1/build-consumer-result.json)
pass. Ordinary/forced keyline_width 4.1 merged saves retain exact state and
both ledger keys/token pairs; selected reset retains unrelated edits.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudTrackTuneColourOnly`
passes [both presets](../out/validation/crossfeed-20261020/hud-track-colour-v1/native/runner-result.json):
Release 54 readbacks/12 rows, Debug 90/21; twelve lossless paired 192x32 ROI
stacks, 73728 native pixels/221352 P6 bytes. Paired health/ammo 0/0,.5/.5,1/1
covers three cases, not nine Cartesian combinations. Disjoint HEALTH/PAINT
192x16 regions omit the source gap; these are not contiguous crops.

All 6144 raw RGBA pixels/case precede coherent whole-stack source-model
admission. Independent ROI order is opaque `141414` ink, track, opaque
health/team prefixes. Six controls discriminate twelve tracks: 27648 changed/
9216 unchanged comparisons. Full-covered frames are invariant; alpha0 exposes
ink. Copied whole-HUD equality proves adoption only.

[Parity](../out/validation/crossfeed-20261020/hud-track-colour-v1/native/reconciliation.json)
retains 28 identical shared files, including twelve PPMs.
[Inspection](../out/validation/crossfeed-20261020/hud-track-colour-v1/native/contact-and-view-reconciliation.json)
and [source reconciliation](../out/validation/crossfeed-20261020/hud-track-colour-v1/final-source-reconciliation.json)
pass. Historical source-equivalence defects/correction failures remain preserved;
there is no pixelwise mixing or tolerance.

Debug half/half: nine commands/states, one Track-token Save/rebase/reset,
two private PlaySettings/Jump J seed Saves/default display; zero local/Player/
Level/Scene/State/input/time updates and input polls. Actual-Level, human,
small-view, balance, hardware/listening/performance gates remain open.

HUD health colour increment (2026-10-21), **verified**:
path 40, `presentation.hud.health_colour`, reads live on retained HUD draws.
One base premultiplication preserves opaque `76E8B5`, geometry, resources and
the separate ammunition/team tint.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/hud-health-colour-v1/build-consumer-result.json)
pass. External HUD paper keyline_width 4.1 rejects ordinary/forced merged saves,
retaining exact source/staged/live/current/baseline and both ledger keys/token
pairs; one-token Save/rebase/reset retains unrelated edits.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -HudHealthTuneColourOnly`
passes its first execution in [both presets](../out/validation/crossfeed-20261020/hud-health-colour-v1/native/runner-result.json):
Release 54 readbacks/12 rows/12 crops, Debug 90/21/12. Three supplied health
amounts 0/.5/1 across four palettes use independent track
`[1036,64,1228,80)`: one opaque B8 underlay, then a zero/96/192-pixel prefix.
All 3072 raw RGBA pixels/case and 36864 retained RGB pixels match
(twelve native 192x16 crops, 110760 P6 bytes). Six exact-mask controls
discriminate 13824 changed/4608 unchanged comparisons. Empty-health whole
frames are invariant; ammunition bytes stay fixed; alpha0 retains the track.
Copied full-HUD rasters prove adoption only.

[Parity](../out/validation/crossfeed-20261020/hud-health-colour-v1/native/reconciliation.json)
confirms all 28 shared files, including twelve PPMs, byte-identical.
All twelve crops were [inspected](../out/validation/crossfeed-20261020/hud-health-colour-v1/native/contact-and-view-reconciliation.json);
contacts match and native serialization has no warnings.
[Twelve source/default hashes and four fresh objects/executables reconcile](../out/validation/crossfeed-20261020/hud-health-colour-v1/final-source-reconciliation.json).

Debug's nine full-health commands/states prove one Health-token Save/rebase/
saved reset, two private PlaySettings/Jump J seed Saves/default display and
zero updates/input polls. No Player/Level simulation runs. Actual-Level
visibility and human contrast/readability/comfort, small-view, balance,
hardware/listening/performance acceptance remain open.

Scribble colour increment (2026-10-21), **native v2 verified**:
path 39, `presentation.opening.scribble`, reads live on applicable captured-note
draws. One premultiplication preserves opaque `3050C0`, art/order/resources;
no notes are created or rerolled.

Both game/editor builds and four focused [CPU cases](../out/validation/crossfeed-20261020/scribble-colour-v1/build-consumer-result.json)
pass. Ordinary/forced merged corner_step 0.002 below pen_width 0.004 is rejected,
retaining source/staged/live/current/baseline and both ledger keys/token pairs;
one-token save/reset retains unrelated edits.

[Native v2](../out/validation/crossfeed-20261020/scribble-colour-v2/native/runner-result.json)
passes both presets: shared 40 readbacks/eight rows/eight native 67x67 crops
(35912 pixels, 107840 P6 bytes), Debug 76/17/8. Two paired notes, colour0/tilt0
and colour2/tilt0.1, provide different opaque papers, not a complete matrix.
Five static rectangles plus 74 ordered literal dabs/context match all 4489
raw RGBA pixels; eight wrong controls discriminate 24 rows. All 35912 retained
RGB pixels match one complete representative source model per crop, with no
per-pixel mixing, blanket tolerance or universal GPU claim. Eight PPMs are
byte-identical across presets; all eight crops were inspected. Alpha0 retains
paper/static note. Copied whole front details prove adoption only.

The [first native failure](../out/validation/crossfeed-20261020/scribble-colour-v1/first-native-failure-investigation.json)
retains a supplementary precision-oracle mismatch (175 versus 174).
Four predictive source models represent precision choices;
[D3D11 spec 17.5](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)
allows blending at output precision or higher. Model constants were not fitted
to captures; the precision correction changes no production code.

Debug executes nine commands/states in Context A only: one-token Save/rebase/
saved reset, two private PlaySettings/Jump J seed Saves/default display;
all updates remain zero. [Metadata revalidation](../out/validation/crossfeed-20261020/scribble-colour-v2/native/metadata-revalidation-v3.json) passes on saved captures, without a GPU rerun.
Actual-Level visibility and human/readability/small-view/balance/hardware/
listening/performance acceptance remain open.

Silhouette body increment (2026-10-20), **verified**:
path 38, `presentation.opening.silhouette`, reads live on applicable story draws.
One mass-tint premultiplication preserves opaque `0E0E0E`, art/order/resources
and repeated-write multiplicity.

Both game/editor builds, four focused CPU cases and both native first
executions pass. Shade_width 0.004 below pen_width 0.008 rejects ordinary/forced
merged saves, preserving source/staged/live/current/baseline and every ledger
key/token pair; one-token save/rebase/reset retains unrelated edits.

Shared evidence has 61 readbacks, 12 rows and 12 crops (759808 pixels).
Nine literal 3x3 RGBA patches qualify ordered one/two/three body writes;
retained PPMs independently check 324 literal RGB pixels. Thirteen wrong
references yield 34 affected-patch refusals/eight unchanged guards. Alpha0
retains BLACK ink and later art. Copied full artwork proves adoption only;
broad exterior bounds do not independently prove the complete body mask.

Debug's nine commands/nine states across three contexts add 108 readbacks/27
rows: totals 169/39/12. Exact one-Body-token Save/rebase/saved reset and two
private PlaySettings/Jump J seed Saves retain default display, real content/
private preferences/resources/RNG. All updates are zero.

All 28 shared files, including 12 PPMs, and the supplemental contact PNG match
between presets. Root inspected all crops; twelve frozen hashes/default JSON
and fresh Silhouette source/object hashes/timestamps reconcile. Receipts:
`out/validation/crossfeed-20261020/silhouette-body-colour-v1/`.
Child half-size uses a simulated 640x360 screen inside the 1280x720 application.
Actual-Level body visibility, human story/comfort/readability, small-view,
balance, hardware/listening and performance remain open. More fields remain.

Glass colour increment (2026-10-20), **verified**:
path 37, `presentation.opening.glass_colour`, is live at all four Monitor
initial-glass/palette/picker/close writes. Four once-premultiplied tints preserve
opaque `161616`, API/geometry/resources and overlays.

Both game/editor builds, four focused CPU cases and both native first
executions pass. Merged glass width 0.95/x 0.1 rejects ordinary/forced saves,
retaining source/staged/live/current/baseline and both ledger keys/token pairs.
One-token save/rebase/reset retains unrelated edits.

Shared evidence has 100 readbacks, 20 rows, 20 crops (55272 pixels),
18 per-write controls (12 partial straight/squared, six alpha0 straight) and two
blanks. Four main 80x24 montages use one effective opaque-beige backdrop;
eight picker 99x53/eight close 27x26 crops use two genuine Canvas underlays.
Independent main proof covers five ordered ROIs only; standalone proof covers
guarded picker/close ROIs, retaining white frames/caller swatches/24 X dabs.
Copied full-Monitor rasters prove adoption only. Alpha0 underlayer/overlay
checks pass; this does not independently reproduce the full bezel.

Debug's nine commands/nine states across three methods add 108 readbacks/27
rows: totals 208/47/20, standalone black only. Exact one-Glass-token
Save/rebase/saved reset and two private PlaySettings/Jump J seed Saves preserve
default display, real content/private preferences. All updates are zero.

44 shared files, including 20 PPMs, and the supplemental contact PNG match
between presets. Root inspected all crops; twelve frozen hashes/default JSON
and fresh Monitor source/object hashes/timestamps reconcile. Receipts:
`out/validation/crossfeed-20261020/glass-colour-v1/`.
Actual-Level glass visibility remains **unmeasured**. Human story/readability,
small-view, balance, hardware/listening and performance remain open.

Acting face/sweat increment (2026-10-20), **verified**:
paths 35–36 are live on retained FigureActing draws. Two once-premultiplied
tints preserve opaque `WHITE`/`8FD3FF`, geometry and later BLACK pupils.

Both game/editor builds, four focused CPU cases and both native first
executions pass. CPU per-field one-token save/rebase/reset retains siblings;
external `opening.acting.hop=1.001` rejects ordinary/forced merged saves,
retaining source/staged/live/current/baseline and all three ledger keys with
both token strings.

Shared evidence has 90 readbacks, 16 rows, sixteen native 320x128 crops
(655360 pixels), two blank reads and 24 field-specific controls across both
facings/two Canvas colours. First backend comparisons qualify independent
ordered two-eye masks, BLACK pupils and eight sweat strips, including per-write
alpha/opaque parity. Alpha0 retains pupils. Copied-FigureActing full rasters
prove adoption only.

Debug's 14 commands/14 states add 56 readbacks/14 rows, right-facing/black only:
totals 146/30/16. Sweat reset precedes exact one-Face-token Save/rebase;
sibling-retaining resets pass. Two private PlaySettings/Jump J seed Saves retain
default display; real content/private preferences/resources/RNG stay unchanged.
All local/Player/Level/Scene/StateContext/input/time updates are zero.

All 36 shared files, including 16 PPMs, and the supplemental contact PNG match.
Root inspected all crops; twelve frozen hashes/default JSON and fresh actor
object source/hash/timestamps reconcile. Receipts:
`out/validation/crossfeed-20261020/acting-colour-v1/`.
Actual-Level acting visibility remains **unmeasured**, with source callers only.
Human story/readability, small-view, balance, hardware/listening and performance
remain open. More colour fields remain.

Program Level visibility witness (2026-10-20), **verified**:
both presets pass first execution of separate `ProgramLevelVisibilityOnly` /
`program-level-visibility`: one retained actual two-player Crossfeed full
opening, 1280x720, seed 9411.

Each builds one Level and makes one explicit positive 0.4-second setup Level
update, retaining state `opening`. Constructor Scene admission remains source
provenance, not runtime instrumentation. Observation makes zero
Level/Player/Scene/StateContext/input/time/layout updates. One RNG bracket spans
all six draws/assignments/restoration; public state/style/resources stay fixed.
Private story clocks/camera/persona are not public snapshots.

Each preset records six readbacks, three case plus nine sample rows (12 total),
two full baseline/changed PPMs, zero crops/Debug commands/Save calls. All 256
program-patch pixels at (640,100) change RGB (44,44,44) to `35BCE1`, then reset;
256 paper pixels at (400,250) retain `C9D3D9`, and glass (132,100) retains RGB
(22,22,22). Native full-RGBA repeat/reset checks pass. Independent saved-PPM
checks confirm literal patches and 160535 changed pixels, zero outside program
mask `[136,59,1143,539)`.

Nine shared files, including two PPMs, and the supplemental contact PNG match
between presets. Root inspected both compositions; 33 source/support and two
reused game-executable hashes reconcile, with production/defaults unchanged.
Receipts: `out/validation/crossfeed-20261020/program-level-visibility-v1/`.
This adds no colour paths; Program34's separate 20/56 raw-alpha/command evidence
stays unchanged. Other phases/rosters, small views, human story/readability,
balance, physical input, listening and performance remain open.

Program colour increment (2026-10-20), **verified**:
path 34, `presentation.opening.program_colour`, is live on retained Monitor
draws. One local premultiplication preserves opaque `2C2C2C` without event fade
or geometry/art/font changes.

Both game/editor builds, four focused CPU cases and both native first executions
pass. CPU rejects external `opening.palette_height=0` in ordinary/forced complete
merged saves, preserving exact source/staged/live/current/baseline and every
ledger token. One-token save/rebase/reset retains unrelated edits.

Shared evidence has 20 readbacks, four rows, four native 320x192 corner montages
and three negative controls, using one effective Canvas. Debug's nine
statements/nine states add 36 readbacks/nine rows: totals 56/13/4. Independent
ordered raw writes use opaque `161616` glass then visible mask
`[136,59,1143,539)` within ROI `[135,58,1144,540)`, with glass/palette guards;
alpha0 retains glass. Copied-Monitor full raster proves adoption only.
Two private PlaySettings/Jump J seed Saves retain default display;
resources/persona/RNG, real content and private preferences remain unchanged,
with zero
local/Player/Level/StateContext/input/time updates.

All 12 shared files, including four PPMs, and the separate supplemental contact
PNG match
between presets. Root contact inspection, 12 frozen source/support hashes and
unchanged authored presentation reconcile; both Monitor objects have fresh
compile timestamps and reconciled source/object hashes. Receipts: `out/validation/crossfeed-20261020/program-colour-v1/`.
At that direct-consumer checkpoint, actual-Level visibility was
**source-only/unexecuted**; the separate witness above now verifies that one checkpoint.
Human readability/story,
small-view, OS/global input, physical pads, listening and performance remain open.

Power-light colour increment (2026-10-20), **verified**:
path 33, `presentation.opening.monitor_art.power_light`, is live on visible
retained Monitor front details. One local premultiplication preserves default
`C5D6AA`; there is no event fade or geometry/art/font change.

Both game/editor builds, four focused CPU cases (console and story audio per
preset), independent review and both native first executions pass. CPU checks
reject external `opening.monitor_art.shadow_width=0.101` in ordinary/forced
merged saves, preserving exact source/staged/live/current/baseline/ledger state.
Source/profile/resources/persona retention passes across the fixtures.

Shared native evidence has 40 readbacks, eight rows, eight 32x32 ROI crops
(8192 pixels) and six controls. Debug's nine statements/nine states add
72 readbacks/eighteen rows: totals 112/26/8. Independent ordered raw writes
cover outer ink `[1246,652,1256,662)` then inner light
`[1249,655,1254,659)`; colour deltas stay within that inner 5x4 area.
Alpha0 retains opaque ink. The copied-Monitor full-front reference proves
adoption only. Two private PlaySettings/Jump J seed Saves retain default
display, with zero local/Player/Level/StateContext/input/time updates.

All 21 shared artifacts, including eight PPMs, match byte for byte. Root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/power-light-colour-v1/`.
Attended story/art
comfort, small-view, OS/global input, physical pads, listening and performance
acceptance remain open.

Desktop colour increment (2026-10-20), **verified**:
path 32, `presentation.opening.desktop_colour`, is live on the retained closed
program. One local tint conversion preserves the opaque `008080` default;
there is no event fade or geometry/art/font change.

Both game/editor builds, four focused CPU cases (console and story audio per
preset), independent review and both native first executions pass. CPU coverage includes unrelated-edit retention,
one-token save/rebase/saved reset and invalid external `glass.width=0.95` with
`x=0.1`: ordinary and forced merged saves reject the candidate and preserve exact source/staged/live/
current/baseline/ledger state.

Shared native evidence has 40 readbacks, eight rows, eight 320x192 crops
and six controls. Debug's nine commands add 72 readbacks and eighteen rows:
totals 112/26/8. The independent full-frame literal raw rectangle
`[136,59,1143,588)` is 1007x529, including edge/exterior and alpha0 checks;
a second Monitor supplies adoption comparison rather than the independent
alpha oracle. Two private PlaySettings/Jump J seed Saves, default display and
zero local/Player/Level/StateContext/input/time updates pass.

All 21 shared artifacts, including eight PPMs, match byte for byte; root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/desktop-colour-v1/`.
Actual story/art comfort, OS/global input, physical pads, listening and performance
acceptance remain open. This proof isolates public `draw_closed`, without a full story route.

Cursor shade/shadow increment (2026-10-20), **verified**:
exact paths 30-31 are `presentation.opening.cursor_art.shade` and `.shadow`,
live on retained story arrows. Production adds only their allowlist entries;
existing drawing, authored defaults and geometry remain unchanged. Both already
premultiply base alpha once, without an event fade. Click ticks keep ink/fill.

Both game/editor builds and four focused CPU cases (console and story audio
per preset) pass. CPU checks cover both sibling resets, unrelated edits, exact two-token
save/rebase and saved-baseline resets. Invalid external `shadow_offset=0.201`
rejects ordinary and forced merged saves, retaining exact source/staged/
live/current/baseline/ledger state and all unselected opening fields/resources.
Build and CPU receipts reconcile.

Both native first executions pass. Shared evidence is 68 readbacks, 24 rows (eight cases and sixteen
sample rows) and eight 72x80 crops. Independent raw samples account for one
shifted shadow write followed by one S-cell shade write at `(649,262)`, and
one shadow-only write at `(656,273)`, including per-write target quantization.
Sixteen targeted comparisons reject straight/squared alpha, including the
shadow contribution beneath translucent shade. Full-cursor copied-dependency raster comparisons
prove adoption; the two raw sites supply independent alpha/layer evidence.
Reduce Effects restores rest, and click ticks retain the fixed arrow.
The existing Cursor23 helper and historical receipts remain unchanged.

Debug's fifteen commands and twelve states add 144 readbacks and 72
rows, for totals of 212/96/8. Two private PlaySettings/Jump J seed Saves, default
display, content, captured resources, caller inputs and RNG remain intact.
There are zero local, Player, Level, StateContext, input or time updates.
All 22 shared artifacts, including eight PPMs, match byte for byte. Root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/cursor-material-colour-v1/`.
No active-story interaction, physical/global input, human readability, listening
or performance acceptance is claimed. Further colour work remains open.

Reveal ink/paper/shade increment (2026-10-20), **verified**:
exact paths 27-29 are `presentation.opening.reveal_art.ink`, `.paper` and
`.shade`, live on retained visible percentage cards. Only the explicit colour
allowlist changes; drawing, defaults, captured font/sheet/fill bindings and
percentage-text policy stay unchanged. Existing ordered premultiplication,
0.12-alpha airbrush layers and quadratic click beat are retained.

Both game/editor builds and four focused CPU cases (console and story audio
per preset) pass. CPU checks cover all three sibling resets, source-preserving saves,
conflicts/force and exact three-token save/rebase. Invalid external padding
0.181 refuses both ordinary and forced merged saves without changing
source/staged/live/text/baseline/ledger state.

Both native first executions pass. Shared evidence is 228 readbacks and 264 rows (22 cases plus
242 sample rows), with ten crops: six 400x120 material panels and four 800x120
label pairs, 672000 pixels total. Seven material sites and four two-team
half-age tick sites use independent ordered raw-channel references and negative
alpha/overlap/beat controls; actual sample footprints match in both presets.
The material-only route measures actual captured-font `L" "` as exactly
`Vector2F::ZERO`; empty text is not a substitute. A second production consumer
compares complete-card/text adoption only, not independent geometry or glyph
coverage. Expired/Reduce Effects draws retain the settled card.

Debug's 21 commands and fourteen states add 196 readbacks and 336 rows
(28 cases plus 308 samples), for totals of 424/600/10. Three-token private
save/rebase/reset preserves siblings, real content, preferences, Jump J
and default display; two preference seed Saves and zero local/Level/StateContext/
input updates pass. All 26 shared artifacts, including ten PPMs, match byte
for byte; root contact-sheet inspection and twelve frozen source/support
hashes reconcile. Receipts live under
`out/validation/crossfeed-20261020/reveal-colour-v1/`. No active-story route,
physical/global input, human art/readability, listening or performance
acceptance is claimed. `DBG#1` remains open beyond these twenty-nine paths.

Silhouette ink/rim/shade increment (2026-10-20), **verified**:
exact paths 24-26 are `presentation.opening.silhouette_art.ink`, `.rim` and
`.shade`, live on retained visible intro/reveal silhouettes. The production
change is only the explicit allowlist; existing conversion, drawing, defaults
and artwork remain unchanged. Ink/rim use ordinary base premultiplication.
Each shade pass submits `(R*A/4, G*A/4, B*A/4, A/4)` in the existing four-pass
order, with per-write target quantization and no temporal fade. The separate
`opening.silhouette` body fill stays outside this colour scope.

Both game/editor builds and four focused CPU cases (console and story audio
per preset) pass. CPU checks retain both borrowed record addresses, body fill, unselected colours,
resources, geometry and caller-owned persona/pose inputs. The three fields
have independent sibling resets, exact LF/CRLF saves, conflicts/force and a
three-token save/rebase. An external `shade_width < pen_width` rejects the
complete merged candidate, even with force, preserving source/staged/live/text/
baseline/ledger state and allowing each colour to reset to its saved baseline.

Both native first executions pass. Shared evidence has 38 readbacks, ten rows, eight 440x240
montage crops and ten controls. Independent adult mug patches are three 3x3
interiors centred at ink `(963,690)`, rim `(932,627)` and shade `(959,679)`,
audited against per-edge truncation. Their ordered raw submissions represent
two ink writes, one rim write over the opaque body and four quarter-alpha
shade writes; target quantization occurs after each write. Controls
reject straight RGB, squared base alpha, a missing ink/shade pass and full
shade alpha. Uniform patch interiors, all-art zero alpha retaining adult/child
body, fixed caller inputs/resources/RNG and exact default restoration pass.
Child coverage is aggregate colour adoption/restoration,
not an independent full-geometry oracle.

Debug adds thirteen tune/reset/save commands, 78 readbacks and 26 rows:
totals of 116 readbacks, 36 rows and eight crops. Exact three-token
private save/rebase, sibling retention, saved-raster resets, actual content,
private preferences and no queued match work pass. Its two
seed Saves cover PlaySettings and Jump J while retaining default display.
All 21 shared artifacts, including eight PPMs, match byte for byte; the root
contact-sheet inspection and twelve frozen source/support hashes reconcile.
Receipts live under `out/validation/crossfeed-20261020/silhouette-colour-v1/`.
The native scope
has zero local, Player, Level, StateContext, input or time updates. No
active-match route, physical/global input, human readability, listening or
uncontended performance acceptance is claimed. Further colour consumers and
unscoped string/font/missing-key work remain open.

Cursor ink/fill increment (2026-10-20), **verified**:
the twenty-second/twenty-third exact paths expose
`presentation.opening.cursor_art.ink` and `.fill` on retained resting arrows
and reveal click ticks. Both already had correct base-alpha conversion; this
increment changes only the production allowlist. No drawing, authored value,
geometry or timing changes are made. Shade/shadow remain independent residual
materials when both selected colours have alpha0. Reduce Effects removes only
the detached click ticks, retaining the arrow; there is no event opacity fade.

Both game/editor builds and the focused `debug_console` and `story_audio` CPU
cases pass in both presets. Shared tokens/refusals, independent sibling resets,
exact LF/CRLF save/rebase, conflicts/force and structural aliases include both
fields. Borrowed opening/cursor records and unselected geometry/resources are
retained. External click_duration0.501 rejects ordinary and forced complete
merged saves without changing source/staged/live/text/baseline/ledger state.
The first Debug run caught a stale object: private promotion preserved a source
timestamp older than the previous `tune_scope.cpp.obj`, so Ninja skipped the
new allowlist. Source bytes and assertions stayed unchanged. Refreshing source
timestamps rebuilt the object with both paths; fresh Debug/Release checks pass.
The original failure, build log and `stale-object-diagnosis.json` are retained.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -CursorTuneColoursOnly`
passes on its first native execution in both presets. Shared evidence is
124 readbacks, 112 CSV rows and twelve 72x80 crops. Seven literal colour states
over black/nonblack underlays exercise independent ink/fill changes, partial
alpha and pair alpha0 on the same existing cursor. Immutable baseline-derived
second-consumer rasters prove full-shape adoption/default parity; seven raw
sample sites with targeted straight/squared-base-alpha controls provide the
independent alpha proof. They are not an independent full cursor geometry
oracle. Samples account for actual per-edge sprite truncation and ordered
click-ink/fill overlap. Unselected shade/shadow and Reduce Effects restoration
remain checked. The fixed synthetic draw input is screen_height720.

Debug adds fifteen actual commands, 168 readbacks and 192 rows: totals of
292 readbacks, 304 rows and twelve crops. Independent reset keeps the sibling
pending; exact two-token private save/rebase and saved-baseline reset pass.
Actual content, private preferences, current saved keyboard binding, queued
work and global RNG remain unchanged. Both private PlaySettings/display seed
Saves succeed; this scope does not seed a nondefault Jump key. Release remains
consumer-only. No local preparation, Level or StateContext update is performed.

All 30 shared artifacts, including twelve PPM crops and the raw-sample CSV,
match byte for byte between presets. The contact sheet was visually inspected;
twelve frozen source/support hashes still match, including the unchanged
ScreenCursor consumer. Evidence is under
`out/validation/crossfeed-20261020/cursor-colour-v1/`, including
`build-consumer-reconciliation.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. Native small-view/human readability,
physical/global input, listening, performance and active-match route acceptance
remain open. `DBG#1` remains open beyond twenty-three audited fields.

Respawn pen colour increment (2026-10-20), **verified**:
the twenty-first exact path, `presentation.respawn_redraw.pen_colour`, is live
for existing partial outlines. Its one-site base-alpha conversion preserves
the opaque BLACK default, captured resources and ordered stroke overlaps.
Progress changes path length rather than opacity; no event fade is introduced.

Both game/editor builds and the focused `debug_console`, `movement` and
`story_audio` CPU cases pass in both presets. Movement still measures the P3
95-unit tap and 207.375-unit held rise. Shared strict-token/reset/LF+CRLF
save/rebase/conflict/force checks include this field. A real dead Player is
prepared once through bounded Level updates, then timers/state/camera,
figure/cull bounds, match fingerprint, all stroke values and boundary queries
stay frozen across thirteen commands. Ordinary and forced merged saves reject
external `draw_from == fill_at`, preserving source/staged/live/text/baseline/
ledger state; reset still returns to the saved baseline.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -RespawnPenTuneColoursOnly`
passes on its first execution in both presets. Shared evidence uses 62
readbacks, eighteen CSV rows, eight 440x160 crops and three local Player
preparations. Four retained direct outlines cover both facings and two partial
path lengths alongside an actual frozen dead Player. An independent ordered
Manhattan-prefix/raw-nib reference, explicit overlap channel samples, opaque
parity and straight/squared-base-alpha controls pass. Nonblack zero alpha and
below/at-start boundaries are blank. Actual whole/filled figures remain
nonblank and colour-independent; Reduce Effects retains the same essential
outline. Snapshot and repeat-raster checks preserve the Player, geometry,
ordered strokes and global RNG between the counted preparations.

Debug adds nine actual tune/reset/save commands, 54 readbacks, eighteen rows
and two local Player preparations: totals of 116 readbacks, 36 rows, eight
crops and five preparations. Exact one-token private save/rebase, reset to
saved, unchanged actual content and preferences, and no queued match work pass.
Its two successful private seed Saves cover PlaySettings and Jump J; default
display settings remain unchanged. Release runs the consumer proof alone.
Neither native scope advances a Level, StateContext or input frame.

All 21 shared artifacts, including eight PPM crops, match byte for byte between
presets. The contact sheet was visually inspected; twelve frozen source hashes
still match. Evidence is under
`out/validation/crossfeed-20261020/respawn-pen-colour-v1/`, including
`build-consumer-result.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. No physical/global input, active-match
route, human readability, listening or performance acceptance is claimed.
`DBG#1` remains open beyond twenty-one audited fields.

Idle-drip keyline increment (2026-10-20), **verified**:
the twentieth exact path, `presentation.idle_drip.keyline`, is live for existing
attached beads and falling drops. The common colour reaches all five stable
profile records; only Sprayer, Sniper and Bucket remain enabled and receive
Level drips. Only the outline tint gains base-alpha premultiplication. There
is no event fade, and captured opaque paint, sheet/frame, geometry and timing
are unchanged.

Both game/editor builds and all six focused `debug_console`, `idle_drip` and
`menu_presentation` CPU cases pass across Debug/Release. Shared token/refusal,
reset, exact LF/CRLF save/rebase, conflict/force and unrelated external-edit
checks include the new field. Six preconstructed aged attached/falling effects
retain public state and all five borrowed profile records across commands;
an unrelated invalid Sniper interval rejects the complete merged candidate,
including with force, without changing source/staged/live/ledger state.
The initial Debug console test compilation found three shadowing locals under
`/WX`. Only those test names/references changed; the original Debug game/editor
build and idle-drip pass were retained, then failed/unfinished checks passed.
The original failure and both source freezes remain alongside the recovery.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -IdleDripTuneColoursOnly`
passes on its first execution in both presets. Shared evidence uses 46
readbacks, ten CSV rows, eight 380x260 crops and fifteen local cosmetic advance
calls. Independent raw rectangles/channels check six retained consumers across
all three tools, opaque-default parity, straight-RGB and squared-base-alpha
negative references, and nonblack zero alpha with all six opaque paint
interiors retained. Enabled/Reduce Effects draws are blank; restoration returns
the exact pixels, profile flags and public state. Frozen rendering preserves
public timers/drops/bounds and global RNG. Private cosmetic-stream invariance
is source-audited constness only, not an executed future-sequence proof.

Debug adds nine actual tune/reset/save commands, 60 readbacks, twenty rows
and fifteen local advances: totals of 106 readbacks, thirty rows, eight crops
and thirty local advances. Exact one-token private save/rebase, saved-baseline
reset and unchanged actual content/preferences/queued work pass; both private
preference seed Saves succeed. Release remains consumer-only. Neither native
scope updates a Level or StateContext.

All 22 shared artifacts, including eight PPM crops and the guard CSV, match
byte for byte between presets. The contact sheet was visually inspected and
twelve frozen source hashes still match. Evidence is under
`out/validation/crossfeed-20261020/idle-drip-colour-v1/`, especially
`build-consumer-reconciliation.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. No active-match route, physical/global
input, listening, human readability or performance acceptance is claimed.
`DBG#1` remains open beyond twenty audited fields.

Blocked-shot colour increment (2026-10-20), **verified**:
the eighteenth/nineteenth exact paths expose the existing highlight/keyline
pair. The draw helper premultiplies straight base RGBA once before the
unchanged quadratic fade. Both game/editor builds and the focused
`debug_console`, `blocked_shot` and `menu_presentation` CPU cases pass in both
presets. Retained aged cues cover every paint owner and the invalid-owner
bucket; exact LF/CRLF saves, conflicts/force, independent reset and invalid
complete merged-candidate rollback retain source/staged/live/ledger state.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -BlockedShotTuneColoursOnly`
passes on its first execution in both presets. Shared consumer evidence is
32 readbacks, eight CSV rows, eight 440x240 crops and eight controls. Eight
contacts span four cardinal normals at full/half life, with the existing
half-life fade of 0.75. Independent raw geometry/channels, opaque-default
legacy parity, straight-alpha/doubled-fade controls and stored opaque team-bead
interiors pass. Nonblack zero-alpha highlight/keyline removes their ticks and
outlines while retaining those beads. Snapshots cover stored contact tints,
position/normal/owner, age, all ten owner-repeat budget slots per instance,
per-cue/aggregate bounds and draw RNG.

Debug adds fifteen actual tune/reset/save commands, 60 readbacks and twenty
rows: totals of 92 readbacks, 28 rows and eight crops. Exact two-token private
source/staged save/rebase, reset to the saved baseline and unchanged actual
content/private preferences pass. Its two private preference seed Saves
succeed; no Save failure is expected or observed. Release runs the consumer
proof alone. One local half-age preparation per fixture means Debug totals
two local preparations and Release one; neither updates a Level or StateContext.

All 21 shared artifacts, including eight PPM crops, match byte for byte between
presets. The contact sheet was visually inspected, and all twelve frozen
production/CPU/native source hashes still match the validated files. Evidence
is under `out/validation/crossfeed-20261020/blocked-shot-colour-v1/`, including
`build-consumer-result.json`, `native/reconciliation.json` and
`final-source-reconciliation.json`. These synthetic contact checks establish
no new shot-launch classification, active-match route, physical/global input,
OS-focus delivery, listening, human readability or performance acceptance.
`DBG#1` remains open beyond nineteen audited fields.

Combat-colour increment (2026-10-20), **verified**:
the sixteenth/seventeenth exact paths expose the existing highlight/keyline
pair. `CombatFeedback` keeps its borrowed presentation record; its shared fill
helper now premultiplies the authored base alpha before applying the event
fade once. Opaque reviewed defaults and opaque team fills retain their pixels;
sheet/fill capture, cue admission, geometry and timing are unchanged.

Both game/editor builds pass. The focused `debug_console`, `combat_feedback`
and `menu_presentation` CPU cases pass in each preset. The seventeen-path
checks cover named/RGB/RGBA tokens and refusals, independent reset, exact
LF/CRLF saves, conflict/force and unrelated external edits. Preconstructed real
hurt/burst cues retain public timers, detached death rectangle and cull bounds
while colour commands adopt straight partial/nonblack zero-alpha channels.
An unrelated invalid combat timing constraint rejects the complete merged save
and retains source/staged/live/ledger state, even with force. Release
developer-tool exclusion remains intact.

Both focused native runs pass. The isolated matrix is
`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -CombatTuneColoursOnly`.
Four preconstructed full/half-life hurt and
elimination cues are compared over black/nonblack underlays with independent
raw-source rectangles and explicit RGBA channels. Opaque legacy parity,
straight-alpha/double-fade negative controls, opaque team interiors and frozen
public timers/death rectangles/bounds/draw RNG are checked. Each preset's shared
consumer scope uses 32 readbacks, eight CSV rows, eight 420x360 crops and eight
controls, plus two local effect-age preparations. It performs zero Level or
StateContext updates.

Debug adds fifteen actual validated tune/reset/save commands, 60 readbacks,
twenty rows and two local age preparations on borrowed consumers. Its exact
private two-token source/staged save and rebase, reset to the saved baseline,
actual content/tuning, private preferences (including the saved Jump binding)
and absence of queued match work are checked. Debug totals are 92 readbacks,
28 rows, eight crops and four local age preparations, with zero Level or
StateContext updates. Two private preference seed Saves succeed; no controlled
Save failure is exercised by this native scope.

All 21 shared capture files match byte for byte, including eight PPMs, their
converted crops and both contact sheets. The compact sheet was visually
inspected; this does not close human contrast/comfort acceptance. Evidence is
under `out/validation/crossfeed-20261020/combat-colour-v1/`, including
`build-consumer-result.json`, `native/runner-result.json` and
`native/reconciliation.json`. No active-match route, OS-focus delivery,
physical/global input, listening, human readability or uncontended performance
acceptance is claimed. `DBG#1` remains open beyond these seventeen fields.

Identity-background increment (2026-10-20), **verified**:
the fifteenth exact path uses the existing opaque/distinct badge validation and
the shared live world/HUD/menu renderer. Added CPU checks cover independent
ink/background reset, exact LF/CRLF save/rebase, conflicts/force and both semantic
merge directions: an independently valid external sibling colour must not make
the merged pair equal. A rejected merge preserves source bytes and the prior
live, staged and pending states, even with force. The native companion checks
paper pixels while retaining ink, geometry, visibility and draw state.

Both game/editor builds and focused Debug/Release console acceptance pass.
The first CPU compile caught shadowed test-local names under `/W4 /WX`; that
failed log is retained beside the corrected final receipts. Both native
Identity-only runs pass, with all 151 shared PPMs matching byte for byte.
The background companion adds 28 readbacks, five rows and four crops per
preset, with no simulation updates. Debug adds 40 readbacks, eight rows and
ten real command attempts (seven accepted, three semantic refusals), private
one-token save/rebase and reset to the saved baseline. Private play preferences
and saved Jump J remain unchanged. Team/Weapon/Results are source-audited users
of the shared renderer; their pages are not newly opened by this increment.
Evidence is under `out/validation/crossfeed-20261020/background-colour-v1/`,
including `native-reconciliation.json` and `consumer-final/`. Human
contrast/readability, physical-pad, listening and performance acceptance remain open.

Identity-ink increment (2026-10-20), **verified**: the fourteenth
exact path uses the existing opaque/distinct badge-colour validation. Added CPU
checks cover atomic semantic refusals, independent reset, exact LF/CRLF save and
rebase, token conflicts/force and unrelated external edits. A valid external
background edit that makes the merged ink invalid must restore the exact source
bytes and retain the previous live, staged and pending states, even with force.
Both builds pass and all 40 consumer cases have successful receipts per preset
in `out/validation/crossfeed-20261020/camera-final-v1/consumer-reconciliation.json`.
The retained Release DisplayTrial isolated-save seed failure passed with the
unchanged binary in a fresh private directory; later unfinished cases passed
separately. No save code changed and the transient failure is not diagnosed.

Both native Identity-only runs pass with 147 shared PPM frames/crops matching
byte for byte. The new ink scope uses 28 additional readbacks, five rows and
four crops per preset; Debug adds 40 readbacks, eight rows and ten real command
attempts (seven accepted, three semantic refusals), exact private one-token
save/rebase and saved-baseline reset. Preconstructed PlayerBadge/UiPlayerBadge
roster cells and actual frozen world/HUD badges pass literal opaque pixel
substitution, captured geometry/state, hidden/focus policy and RNG/fingerprint
checks. Team Select, Weapon Select and Results wiring is source-audited through
that shared renderer; those pages are not directly opened by this increment.
Run paths and counts are in `camera-final-v1/native-reconciliation.json`.
No physical-pad, listening, human readability or performance acceptance is claimed.

Validation of the seven-colour increment (2026-10-18): Debug and Release build
under `/W4 /WX` and pass all 36 consumer cases each. The Debug console case
checks raw string spans/escapes and first-occurrence keys, the strict allowlist,
safe named/RGB/RGBA values and refusals, fixed borrowed records, F9 preservation,
read/diff/reset, exact LF/CRLF save splices, conflicts/force and unrelated disk
edits, completion and inspect rows. Real Level checks retain recording/replay
edit restrictions, existing Label identity, premultiplied RGBA adoption, hidden
zero tint and unchanged match fingerprints/RNG. Release still excludes developer
tool RTTI.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -TuneColoursOnly` passes
on the NVIDIA GeForce RTX 5080 at hidden 1280x720. Eleven records and nine
captures cover all seven real consumers, Home/Mode capture and reopening/reset,
full resource rejection, queued reload refusal and rebuild/rebase, staged
fallback and saving. Content, user files, profiles and candidate TEMP/TMP are
private to the run. Reload now honours the same content-directory overrides as
`tune`; its normal source/staged defaults and source precedence are unchanged.
The runner explicitly refuses this Debug-only scope for Release before compiling
or launching it. Both game executables also pass isolated system-PATH startup,
per-monitor-v2 DPI awareness and graceful shutdown with the accepted audio bank.

Evidence is under `out/validation/tune-colours/`; the native run is
`a2271b60ffb04b71860a8629e3a40d7a`. No tests were skipped. These muted hidden
checks establish no physical-pad, listening, human readability or uncontended
performance acceptance.

Validation of the Team-colour increment (2026-10-19): both builds and all 37 CPU
cases per preset pass. The nine-path checks cover the new fields' named/RGB/RGBA
commands, exact tokens and scopes, invalid values, independent reset, LF/CRLF
splices, conflicts/force, unrelated external changes and structural aliases.
Independent review strengthened the save loop so each Team case introduces a
fresh unrelated colour; the final Debug console case was recompiled and passed
after that assertion-only change. The original full-suite source and receipts
are retained, and Release compiles this test body out.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -TeamTuneColoursOnly`
passes with actual tune/reset commands on one open two-player Team Select page.
Seven updates, three CSV records and two compact icon strips show a ready and an
unready seat retaining their colours before refresh, adopting both edits on the
next connected update and restoring the exact same-page baseline after reset.
Independent icon rasters, outside-strip pixels, captured artwork/fonts, sparse
roster, input snapshots, settings, save bytes and draw RNG are checked. The
dedicated Release guard refuses this scope before compilation or run creation;
Release developer-tool exclusion still passes CPU acceptance. Both isolated
system-path startups pass with the published audio bank and per-monitor-v2 DPI
awareness. Evidence is under `out/validation/team-tune-colours/`. These muted
hidden checks establish no physical-pad, listening, human readability or
uncontended performance acceptance.

Validation of the Weapon-colour increment (2026-10-19): both builds and the
complete Debug console CPU case in each preset pass. The eleven-path checks
cover independent Weapon reset, named/RGB/RGBA commands, exact LF/CRLF splices,
fresh unrelated external edits, conflicts/force and structural aliases. The
full 38-case suites from the preceding menu-label commit remain the baseline;
they were not rerun for these two Debug-only allowlist entries. Release
developer-tool exclusion still passes.

`scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -WeaponTuneColoursOnly`
passes on one open two-player Weapon Select page. Four actual tune/reset
commands, ten updates, six CSV records and two compact crops check frozen,
inactive and missing-pad retention, neutral reconnect adoption and exact reset.
Independent straight-alpha colours and premultiplied icon rasters check both
ready and unready tiles; artwork, labels, outside-card pixels, sparse roster,
input snapshots, settings, private source/staged content, save bytes and draw
RNG are preserved. The dedicated Release guard refuses this scope before
compilation or run creation. Both isolated system-path startups pass with the
published audio bank and per-monitor-v2 DPI awareness. Evidence is under
`out/validation/weapon-tune-colours/`. These muted hidden checks establish no
physical-pad, listening, human readability or uncontended performance acceptance.

Validation of the weapon-feedback increment (2026-10-19): both builds and all
38 CPU cases per preset pass. The thirteen-path checks cover named/RGB/RGBA
commands, independent reset, exact LF/CRLF save splices, conflicts/force, fresh
unrelated external edits and completion. The new keyline colour shares its
completion prefix with the existing numeric `keyline_width`; both remain
available. Release developer-tool exclusion still passes.

`scripts/check-painter.ps1 -Preset <preset> -SkipBuild -FeedbackTuneColoursOnly`
passes in both presets at hidden 1280x720. Each consumer-only run uses 56
readbacks, eight CSV records and eight 360x280 crops (2,419,320 PPM bytes).
Independent raw-source rasters check opaque defaults, RGB, half-alpha RGBA and
nonblack zero-alpha colours over black and coloured backgrounds. Opaque-default
legacy parity, deliberate straight-alpha and doubled-fade negative controls,
Reduce Effects, team centres and frozen geometry/timers/RNG are checked. The
eleven shared CSV/PPM/summary files are byte-identical between presets.

Debug additionally executes fifteen actual tune/reset/save commands against
preconstructed consumers, using 66 more readbacks and twenty CSV records.
Private two-token save/rebase, reset to saved values, preferences and source/
staged bytes are checked. Release runs the consumer proof alone. Neither path
updates a state or enters a match. Both isolated system-PATH startups pass with
the published audio bank and per-monitor-v2 DPI awareness.

Evidence is under `out/validation/feedback-tune-colours/`. The initial CPU
completion assertion and two native fixture assertions were corrected; their
sources, failed logs and receipts remain alongside the passing runs. Native
corrections sample the readiness bead's interior and read successful command
replies from the console log. No checks were skipped. These hidden checks
establish no physical-pad, listening, human readability or uncontended
performance acceptance. `DBG#1` remains open beyond the thirteen audited paths.

Alpha-consumer audit (2026-10-19): six already-supported fields still submitted
straight tint RGB to the engine's premultiplied blend: the three movement inks,
debug foreground/shadow and the captured Home background. Twenty-one drawing
submissions across fourteen client files now convert the base tint once. All
six authored defaults are opaque, so their existing channels and movement fades
are preserved. The presentation records, parser, thirteen-path allowlist and
save format are unchanged. The earlier seven-field native probe checked opaque
movement/debug text; its partial/zero-alpha pixel proof covered station labels.

Both builds, all 38 CPU cases per preset and both published-bank/DPI2/system-PATH
startups pass. `scripts/check-painter.ps1 -Preset <preset> -SkipBuild
-ExposedColourAlphaOnly` passes in both presets at hidden 1280x720. The shared
consumer proof uses 285 readbacks, 142 CSV rows and twelve native crops
(3,485,556 PPM bytes). Seventeen shared CSV/PPM/summary files are byte-identical
between presets, including all fifteen raw CSV/PPM files.

All thirteen real Home-background capture sites are opened directly: 52 page
openings check opaque defaults, half-alpha, nonblack zero-alpha, retained pages,
reopening and exact default restoration over two underlays. Independent raw
references preserve the existing scrim. Display confirmation uses only the
probe's guarded hidden window and the same windowed 1280x720 candidate, with
destructor rollback and no Keep action. Private preferences and save bytes stay
unchanged after the two successful seed writes.

Twenty preconstructed movement cues cover takeoff, landing, step and skid,
full/half life, and retained opaque/partial/zero-alpha contacted paint. An
independent integer puff mask and literal contact geometry check base-alpha
conversion before fade, opaque parity and deliberate straight/doubled-fade
negative controls. Actual ordinary DebugText matches nineteen literal rows and
raw font tints. Ten local effect-age updates prepare half-life fixtures; no
Player, Level or StateContext update is performed.

Debug adds eight validated private tune/reset commands and independent frozen/
live console, paint/status-panel and perf text references. Its additional 104
readbacks, 48 rows and four crops bring the totals to 389/190/16 (4,929,456 PPM
bytes). Fixed severity/caret/status/perf foregrounds, captured fonts, log/input
state, synthetic perf samples, private content/preferences and draw RNG are
preserved. Release excludes this console scope.

Evidence is under `out/validation/exposed-colour-alpha/`. Source review caught
and corrected the initial probe's one-pixel landing-outline clipping before
execution; its original source and runner remain archived. Both native presets
passed on their first execution. These checks establish no active-match route,
physical-device, listening, human readability or uncontended performance
acceptance. `DBG#1` remains open; this audit adds no new tunable fields.

## Foreground checks (`CI#1`)

**UNSCOPED.** `scripts/check-debug-console.ps1` remains an opt-in local check:
its real key events and focus-sensitive scripts need a reliable foreground
desktop, which the current hosted CI workflow does not establish. First assess
an attended/dedicated runner model with explicit focus ownership and retained
failure evidence. Scope CI integration only after that model is demonstrated;
a hidden launch or synthetic CPU input check does not qualify the foreground
workflow. Existing CPU acceptance and startup/editor CI checks remain in place.
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
unattended experiment. There is no new special, damage, health or jump boost.

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
