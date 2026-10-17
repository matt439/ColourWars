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
| `perf`, `perf.reset`, `perf.capture <frames> <label>` | print/clear the [perf](#perf) numbers or export a bounded rendered-frame capture |
| `tune <path> [value\|next\|prev\|+=x\|-=x\|on\|off\|toggle] [restart]` | print or set a number or boolean in `tuning.json` / `presentation.json` (see [Live tuning](#live-tuning)) |
| `tune.diff [tuning\|presentation]`, `tune.reset [path\|tuning\|presentation\|all]`, `tune.save [tuning\|presentation\|all] [force]`, `tune.find <word>` | list, revert, write back to the source files, search the paths |
| `bot <seat\|all\|off> <idle\|walk\|jump\|chase\|paint\|off>` | scripted input for a seat or every idle dummy; `bot` alone lists (see [Bots](#bots)) |
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
  variable (`toggle` for a bool, `next`/`prev` otherwise).
- A variable row shows `name  value`. Selecting a bool or a number steps it; a
  choice (`ammo`) opens a picker on its current value.
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
| `overlay.stations` | in the world: every [station](#stations-and-goto)'s name, the spawns included, above the place `goto` puts a player. The names are `labrador::Label` objects in the level's scene, made the first time the overlay is turned on and hidden by a transparent colour after that, so they draw wherever the world is drawn (the intro picture and the overview too) and under structures and players. Font, scale, offset and colour are `presentation.debug_overlay.station_label`; scale and offset tune live. No default key |
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

Every number and boolean in `tuning.json` and `presentation.json` can be read
and changed while the game runs, by the path its keys give it:
`tuning.player.gravity`, `tuning.weapons.sprayer.offset.x`,
`presentation.movement_feedback.enabled`; an array element is `[i]`. The names
are the files' own keys, scanned from the files, so they cannot drift from what
the game reads (the acceptance case replaces every value with a string in turn
and requires the game's reader to reject it by name). Tab completes a path one
segment at a time (`tune tuning.pl` -> `tune tuning.player.`), and then the
value (`next prev`, or `on off toggle`).

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
their addresses, and only the edited number changes.

**When it shows.** Each path has a scope, printed after the edit and marked with
`*` in the debug menu when it is not live:

| Scope | Paths | Applies |
|---|---|---|
| live | `tuning.input.*`, `tuning.player.*` (not `size`), `tuning.match.sounds.*.volume`, `tuning.weapons.*`, `tuning.projectiles.*` (except sizes and mist `spread_step`), `tuning.match.opening.*`, `tuning.match.reveal.*`, `presentation.movement_feedback/respawn_redraw/opening/debug_overlay/hud/camera.*` | at once |
| new shots | `tuning.projectiles.*.size`, `*.collision_size`, `tuning.projectiles.mist.spread_step` | the next projectile |
| rebuild | `tuning.player.size`, `tuning.match.*`, `tuning.paint.*`, `presentation.match.*`, anything else not listed | the next build: `restart`, or add `restart` to the tune |
| next screen | the rest of `presentation` (menus, pages, prompts) | when that page is next opened |

The shared `presentation.menu_style.layout.position/size/spacing` values use
the menu's 1920 by 1080 design coordinates and apply when a page is next opened.
For example, `tune presentation.menu_style.layout.spacing.y 140` changes the
default row spacing; pages with their own layout still use their overrides.
Position must be nonnegative, size positive and the first widget must fit within
the design area. Spacing may be zero and is bounded by the design size on each
axis. Named menu sheets and frames remain string settings edited in the JSON;
`tune` changes numbers and booleans.

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
order, indentation, comments-as-keys, number style and line endings - including a
file that mixes CRLF and LF - stay byte for byte. Before writing, each edited
token on disk must still be the value the game loaded; if someone changed it in
the meantime, that is a conflict: nothing in that file is written and the paths
are listed (`force` overwrites them). The file is replaced atomically, read back
through the game's reader and every startup check that reads it (resources and
every stage), so a save never leaves a file the next launch refuses; on failure
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
row. A value row shows its token; Left/Right run `tune <path> prev|next` (a
boolean toggles), echoed `[menu] tune ...`, and B/Escape climbs one level.

Not tunable: strings, colours, font and sheet names, and optional keys a file
leaves out (`frame_time` on most clips, `diffusion` blocks) - `tune.save` never
inserts keys.

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

It re-renders the stack into an unpresented frame and reads it back, because the
engine offers no way to read the frame it presents (engine report 10). That
redraw adds no sample to the perf numbers, and the frame after it is not timed
across the capture.

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
  says. **`grave`, `f10` and `f11` are reserved** (console, debug menu). A key the
  keyboard player uses (A, D, S, Space, Down, Escape, F1) binds with a warning:
  it does both.
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

This is engine report 9's workaround, and its cost is recorded there.

## Perf

`overlay perf` (or `overlay.perf on`, or a bind) shows, bottom left above the
paint panel, a graph of the last 120 frames' frame time (one bar a frame; green
up to 60 Hz, amber up to 30 Hz, red beyond; the white line is 16.7 ms; the graph
is two 60 Hz frames tall unless a frame was longer) over five lines:

```text
CPU ms    min    avg    max  (240 frames)
frame   14.68  16.68  18.80
update   0.06   0.11   0.17  x1.00 a frame
draw     0.14   0.23   0.37
objects 23  projectiles 0
```

Min, average and max over the last 240 rendered match frames, timed with
`QueryPerformanceCounter`: **frame** is the time between two match draws,
**update** the game's own `Level::update` calls in that frame (the seat filter -
bots, record, play - included; `x` is updates per frame, so x4 shows 4.00),
**draw** the match's draw from the top of `GameLevel::draw` to the debug panels
(refreshing the layout and recording the draw lists, with the scene's workers
joined). **objects** counts the scene's collision objects. `perf` prints the
lines; `perf.reset` starts again. It is measured whenever a match runs, overlay
or not, and only while the match is on top: a build, the menus, a page over the
match (pause, results) or a screenshot's capture is a gap, not a frame - the
first draw after one starts a new interval and records nothing (so does the
very first). Focus loss and covering pages now break the interval immediately,
including draws that occur before another simulation update.
Frame intervals include intervening submit/Present wall time indirectly.
**Not measured separately**: Present, GPU work or scan-out - `Application`
exposes no frame statistics and `Renderer` no timestamp queries (engine report
14). The panel says CPU for that reason.

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
is pending. Interrupted captures are marked incomplete. The
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
  (`debug_keys.*`, engine report 13) and the bind table (`debug_binds.*`); the
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
    `update_page`; the page is a `DebugPageTarget`, which is how the director
    starts a match, dismisses a page or resets its input. It also ticks
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
  F10 debug menu, staging and Apply, Escape into the pause menu, Alt. In order:
  phases 1-2 (`check_drive.cfg`: goto, status, restart+goto, cheats, the
  screenshot command; `check_menus.cfg`; a 1280x720 launch paused in the
  countdown for the panels and the debug menu); phase 3 (`-Phase3Only`: a
  four-seat paint-bot match, live tune, record and play to `MATCH`, the tuning
  tree); phase 4 (`-Phase4Only`: `+exec review_feedback_ab +quit` with an
  autoexec, 18 PNGs and nine checksums; `station_tour`; `bots_4p`; Shift+F5 on
  a bind; the `%TEMP%` sweep on a planted folder in an isolated TEMP; the live
  console; perf; binds and history across a restart). It saves screenshots
  throughout. `-SaveToSource` adds a
  `tune.save` on the real `game/content/tuning.json`: it refuses unless git
  sees the file clean, checks with git that exactly one value changed, and
  writes the saved original bytes back (a `git checkout` would not restore them:
  it renormalises a working copy that mixes CRLF and LF). The pad chord needs a
  pad and is covered on the CPU side only.
- The engine reports this forced are 9, 10, 11, 12, and the drafts 13 (key
  names) and 14 (frame statistics), in
  [engine-reports.md](review/engine-reports.md).

## Not done

- **Loops, conditions and variables in scripts** - deliberately. A script is
  data (which frames, which stations); anything that needs logic is a command.
- **A live console over the menus**, and **binds in the menus**: menus have no
  simulation to keep running, and F5 (`restart`) means nothing there.
- **Scripts that keep running while the window is in the background**: the
  match only updates while the window is active, and losing focus suspends the
  level (which would change a review's checksums anyway). A Debug-only "run
  while inactive" switch would not stop the suspend; not built.
- **Typing in the live console while the keyboard player keeps playing**: the
  whole keyboard seat is blanked, because the engine has no per-key consume
  (engine report 9).
- **GPU, present and submit time** in the perf overlay (engine report 14).
- **Strings, colours, fonts and absent keys in `tune`**, and inserting keys in
  `tune.save`.
- **Replays across builds or machines**, across step sizes, and across rounds
  (a recording stops at the end of the round).
- **A driven check of the pad chord and of a bot on an unplugged pad seat**:
  this machine has no pad; both are covered on the CPU side.
