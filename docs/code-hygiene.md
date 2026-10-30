# Code hygiene and maintenance

This is the live owner for `HYG` rows in [TODO](../TODO.md). The author asked
for source-file splits and comment audits on 2026-10-28. These are independent
maintenance slices alongside the gameplay backlog; they do not change its
arena/tuning order. Since 2026-10-29 it also owns the CI-gate rows `CI#2` to
`CI#5`; `CI#1`, the foreground debug-console check, stays with
[the debug console](debug-console-record.md#foreground-checks-ci1).

## Architecture review (2026-10-30)

On 2026-10-30 the author asked for an architecture review, since the project is
read as an example, and then asked for every finding to go into TODO and be
worked through. The findings are recorded, frozen, in
[the review](review/2026-10-30-architecture-review.md); this section owns their
status. Engine-side findings are ledger drafts `ENG#27` to `ENG#32` in
[engine-reports.md](review/engine-reports.md).

These rows are allowed to do what the [working rules](#scope-and-working-rules)
below forbid for a source split: rename a type, add an abstraction, move files
between directories and change a class's bases. They still preserve behaviour,
the game's inheritance-first style (no T11 findings), save and content formats,
and evaluation and RNG order unless the row says otherwise. Each row builds Debug
and Release and runs the acceptance cases its files touch; a row that moves files
across the tree runs the full default acceptance list. Work them in the order
listed. Refresh line numbers before starting; they are from `96c65373`.

### Match setup out of the menu layer (`HYG#28`)

**Done 2026-10-30.** `MenuLevelSettings` is now `MatchSetup` in
[game/match_setup.h](../game/match_setup.h) and `MenuPlayerSettings` is
`SeatSetup` in [game/seat_setup.h](../game/seat_setup.h), with no other
change to either type; `GameMenu`'s member followed as `match_setup_`. The seat
label left the class as the free function `player_label(const MatchSetup&, int,
const MenuText&)` in [game/states/player_label.h](../game/states/player_label.h).
The two Mode-row validators left `MainMenuModeSelect` as
`validate_mode_durations` and `validate_mode_stage_rules` in
[game/states/menu_content_validation.h](../game/states/menu_content_validation.h),
beside `validate_shell_content`, which runs `validate_game_content` and then
those two, in the order startup ran them before. `wWinMain`, `reload`, the
level-checking `tune` path and the probes that ran the startup checks call it
instead. `game/objects`, `game/ai`, `round_tape`, `last_match_setup`,
`series_stage_rule` and `content_validation` now include no `states/` header.
Both presets build under `/W4 /WX`. The full default acceptance list passed in
Release (74 cases) and in Debug without the four slow cases (71), and
`debug_console` and `menu_text` also passed in Debug. The Debug compile gate
(the sixteen opt-in cases, `check-painter.ps1 -CompileOnly` and
`check-frame-pacing.ps1 -CompileOnly`) compiled, and `check-startup.ps1` passed
on the Debug executable.

### A game library for the executable and the probes (`HYG#29`)

**Done 2026-10-30.** [game/CMakeLists.txt](../game/CMakeLists.txt) now builds
every game source but `main.cpp` and the two platform adapters into the
`colourwars_game` static library, `rumble_xinput.cpp` into
`colourwars_rumble_xinput` and `display_awake.cpp` into
`colourwars_display_awake` (each building against the game library's headers
and reusing its precompiled header), and links all three into the thin
`ColourWars` executable of `main.cpp`, `resource.rc` and the manifest.
`run-acceptance.ps1`, `check-movement-feedback.ps1` and `check-paint-fx.ps1`
link the game library and both adapters; `check-painter.ps1` and
`check-frame-pacing.ps1` link the game and display adapters and supply their own
rumble factory; `check-display-awake.ps1` links the game library alone, checks
with `lib /list` that it holds `game_states.cpp` and neither the entry point nor
an adapter, and records the three libraries in its receipt. No script globs
`CMakeFiles/ColourWars.dir` or filters object filenames any more, and a retired
source can no longer reach a probe. A link map showed every game-library member
with code in the executable in Debug, and in Release every member but the
Debug-only `game/debug` units, which compile to nothing there and carry no
initializers, so the executable's contents are unchanged. "No CTest by design"
stands; AGENTS.md and the README now say why the game's own probes are not
CTest tests (the runner builds them on request and owns selection, deadlines,
evidence and the timing summary). The Release ZIP's 72 entries are the same as
before. Both presets build under `/W4 /WX`. The full default acceptance list
passed in Release (74 cases) and in Debug without the four slow cases (71);
the sixteen opt-in cases and `check-painter.ps1 -CompileOnly` compiled in both
presets and `check-frame-pacing.ps1 -CompileOnly` in Debug;
`check-display-awake.ps1`, `check-movement-feedback.ps1` and
`check-startup.ps1` passed on Debug, and `check-acceptance-selection.ps1` and
`check-painter-selection.ps1` passed. `check-paint-fx.ps1` links and runs but
fails its "authored gameplay camera scale" check; HEAD's script on the
unchanged pre-split objects fails the same way, so that is older than this row
(its last pass on record is 2026-10-18).

### One acceptance case list and a shared probe header (`HYG#30`)

**Done 2026-10-30.** Every consumer acceptance case is declared once, in
[scripts/acceptance/cases.psd1](../scripts/acceptance/cases.psd1): its name,
group (default, `debug-only` for `balance_runner`, or `opt-in`), extra sources,
arguments (`game`, `repo`), the start rank that replaced `$longestFirst`,
`SlowInDebug` and `Utf8Output`. [run-acceptance.ps1](../scripts/run-acceptance.ps1)
reads it with `Import-PowerShellDataFile`, refuses unknown keys, groups and
arguments, and no longer names a case anywhere. Two switches replace the case
lists CI pasted: `-OptIn` selects the opt-in group (refused with `-Cases`) and
`-SkipSlowDebug` (refused outside `x64-debug`) leaves out the `SlowInDebug`
cases. [ci.yml](../.github/workflows/ci.yml) uses them, and selects exactly what
it did: the same sixteen opt-in cases in the same order (not fourteen, as
AGENTS.md said; it now says sixteen) and the same four Debug pull-request
exclusions. [check-acceptance-selection.ps1](../scripts/check-acceptance-selection.ps1)
checks both switches against the declarations and their refusals, and hashes
the declaration file.

[scripts/acceptance/support/probe.h](../scripts/acceptance/support/probe.h)
holds `acceptance::require` (both message types, throwing `std::runtime_error`
as every copy did) and `acceptance::run_probe(name, body)`, which prints
`FAIL <name>: <reason>` and returns 1 for any escaping exception. 80 files
dropped their own `require` for a `using acceptance::require;` (fixture
headers carry it for their folder), and 98 entry points, the runner's cases and
the painter, frame-pacing, movement-feedback, paint-fx and editor probes, now
return `run_probe`. Probes that printed a bare `FAIL:` or no label now name
themselves. Two keep their own code on purpose: `display_awake_lifecycle`'s
main writes failure evidence before returning, and `balance_runner`'s
self-check `require` prefixes `Self-check:`. No probe's checks changed.

Both presets build under `/W4 /WX`. The full default acceptance list passed in
Debug without the four slow cases (71). In Release 73 of 74 passed first time;
`debug_console` failed to compile because the script had put the new include
inside its fixture header's `#if COLOURWARS_DEV_TOOLS`. After that and ten
similar includes were moved out, `debug_console` and the ten bot probes passed
in Release, and `debug_console`, `menu_text`, `crossfeed`, `press_tower`,
`balance_runner` and those bot probes passed in Debug except the opt-in
`bot_team_trials`, which stopped on `bad allocation` while the machine was
short of resources (a shell started then could not fork); its checks are
unchanged and it passed in Release. The sixteen opt-in cases (`-OptIn -CompileOnly`) and
`check-painter.ps1 -CompileOnly` compiled in both presets and
`check-frame-pacing.ps1 -CompileOnly` in Debug. `check-display-awake.ps1`,
`check-movement-feedback.ps1` and `check-level-editor.ps1` passed (the editor
check failed once in each preset during that load, on a clipboard paste and a
locked file, and passed on rerun), and `check-acceptance-selection.ps1`
passed under Windows PowerShell 5.1; pwsh is not installed here.
`check-paint-fx.ps1` still fails its camera-scale check, as recorded under
`HYG#29`.

### A human architecture overview and accurate docs (`HYG#31`)

**Done 2026-10-30.** [architecture.md](architecture.md) is the human overview:
a directory map, the concerns mixed in the `game/` root, the intended
dependency direction with what holds and what does not at `272ecb75` (measured
from the include graph: `objects`/`ai` include each other, the root includes
`objects` and `states`, nothing enforces the contracts), the nine engine-free
contract headers, `game/carried`, `GameData`, every state-stack shape, `Level`
and `Scene`, the builder chain, the four startup files and the class chains
with every base. AGENTS.md's Architecture section is now a pointer and eight
agent rules; its claim that `GameData` reaches every object is corrected (the
shell's states, pages, `MenuContext`, `StagePreview` and `game/debug` hold it;
`GameLevel` unpacks it into `LevelBuilder`), and its OOP chains gained their
missing bases. The comment atop `game_states.h` now shows Options pushed over
the pause menu and the between-round pages. The OOP guide documents `Player`'s
two `DrawObject` subobjects and links the overview, as does the README (whose
line counts were refreshed). [docs/README.md](README.md) and
[scripts/README.md](../scripts/README.md) index both folders. The work record
of `docs/debug-console.md` (`DBG#1`, `#6` and `#8` to `#18`, and `CI#1`) moved
unchanged to [debug-console-record.md](debug-console-record.md), leaving a
1,724-line reference that gained short `script.background` and `stage_hash`
entries; inbound links were repointed. Both presets build under `/W4 /WX`;
`display_awake`, `remembered_setup` and `ammo` passed in each, and
`check_doc_links.py` passes.

### The precompiled header stops claiming a backend (`HYG#32`)

**Done 2026-10-30.** [game/pch.h](../game/pch.h) now carries the Windows
version floor, the lean `<Windows.h>` and the standard library headers, and no
backend: the WRL, D3D11, DXGI, DirectXMath, `dxgidebug` and 18 DirectXTK
includes are gone. The 32 dead `using namespace DirectX` lines and the two
"disambiguate from `DirectX::Mouse`" comments went with them.
[game/CMakeLists.txt](../game/CMakeLists.txt) no longer finds `directxtk` or
`xaudio2redist` or links `Microsoft::DirectXTK`; the engine finds and links its
own backends' DirectXTK privately, the executable still imports `DirectXTK.dll`
through it, and `vcpkg.json` keeps the port for the engine. `<wrl/client.h>` is
now local to `debug_screenshot.cpp` and `movement_feedback_visual.cpp`, the two
files that use `ComPtr`, and the latter includes `<dxgi1_3.h>` for
`DXGIGetDebugInterface1`. Only `collision_object_type.cpp` in the game had
compiled on the PCH's includes; it now includes `<stdexcept>` and `<string>`. Debug and Release
build the game and editor under `/W4 /WX`; the full default acceptance list
passes in Release and in Debug without the four slow cases; the opt-in, painter
and frame-pacing compile gates, `check-movement-feedback.ps1`,
`check-display-awake.ps1` in both presets and `check-startup.ps1` on the Debug
executable pass. `check-paint-fx.ps1` compiles and runs but stops on "matrix no
longer matches the authored gameplay camera scale", as its 06:36 run on the
pre-`HYG#32` tree already did; that is not this row's.

### The shipping match step out of debug code (`HYG#33`)

**Done 2026-10-30.** The match step is now the private `GameLevel::step_match`
in [game_states.cpp](../game/states/game_states.cpp), the old
`debug_update_level` body unchanged (background audio, rumble, the shipping
tape recorder around `Level::update`). `GameLevel::update` runs it in its own
loop bounded by four director hooks in
[debug_director.h](../game/debug/debug_director.h): `match_steps` (how many:
the clock's 0..4, one in Release), `before_match_step` (stops at a finished
round, else the step's latched or edge-free inputs and seat filter in a
`MatchStep`), `after_match_step` (perf time, update counts, recording
checkpoints, stops where a replay ended or a script waits) and
`end_match_steps` (audio sync on the paused clock), which split the old
`update_match` loop in the same order. In Release all four are inline and say
one step with no filter, so the step is called once a frame directly; no
director hook forwards it any more. `DebugMatchTarget` is now three interfaces
by client in [debug_targets.h](../game/debug/debug_targets.h):
`DebugMatchView` (read-only), `DebugMatchControl` (rebuild, adopt, leave,
audio, background) and `DebugMatchEdits` (overlays, cheats, place, respawn,
kill, seat labels), which it inherits; `seat_labels`, `rebuild_bots` and
`replay_header` take a `DebugMatchView`. The `debug_console` fakes take the
same loop through a fixture `update_match(director, match, ...)`, and
`painter_visual`'s practice replay reaches `step_match`. The Release paragraph
and hooks list of [the debug console](debug-console.md#release-behaviour) and
[the architecture overview](architecture.md) now describe this.
`director_checks.cpp` gained the `<objbase.h>` it needs for `CoInitializeEx`:
after `HYG#32` trimmed the precompiled header it no longer compiled in Debug
(the only build that compiles it), which `HYG#32`'s Debug run, made without
the slow cases, did not reach.

Both presets build under `/W4 /WX`. The full default acceptance list passed in
Release (74) and in Debug without the four slow cases (71); `debug_console`,
`menu_text`, `instant_replay` and `practice_workshop` passed in Debug.
`check-painter.ps1 -PracticeReplayOnly` passed in both presets,
`check-display-awake.ps1` and `check-startup.ps1` on the Debug executable, and
`check_doc_links.py` passes.

### One bot ballistics and RNG module (`HYG#34`)

**Done 2026-10-30.** `BotRandom` (splitmix64) and its seed/seat mix,
`BotRandom::seat_seed`, moved out of `debug/bot.h` into
[bot_random.h](../game/ai/bot_random.h); `MatchBot` keeps a `BotRandom` instead
of its own copy of the generator, and `MatchBotDriver` and `Bot::make` both seed
from `seat_seed`, so every bot draws the same sequence as before.
`match_bot_ballistics::horizontal_reach` is now defined in
[match_bot_ballistics.cpp](../game/ai/match_bot_ballistics.cpp) beside
`aim_direction`, and the Debug `BotDriver`'s engagement distance calls it
instead of its own uncapped copy. That is the one behavioural change: a
skirmish developer bot with a gravity tool now engages at the shipping
same-height ballistic cap, not from a full second of horizontal travel (said in
the commit and in [the debug console](debug-console.md#skirmish-measurement-bot)).
The bots' hard-coded `60` (step length, one-second loops, seconds-to-steps
budgets, the one-step velocity look-ahead) now read `SIMULATION_HZ` or
`SECONDS_PER_STEP` from [simulation.h](../game/simulation.h), each spelled to
give the same value bit for bit; the Debug behaviours' step counts stay as
written.

Both presets build under `/W4 /WX`. The full default acceptance list passed in
Release (74) and in Debug without the four slow cases (71, `balance_runner`
included); `debug_console` passed in Debug, and the opt-in `bot_trials`,
`bot_combat_trials`, `bot_scan_cost` and `bot_team_trials` passed in Release.
`check-startup.ps1` passed on the Debug executable and `check_doc_links.py`
passes.

### One record and replay model (`HYG#35`)

**Done 2026-10-30.** A Debug recording is now a
[RoundTape](../game/round_tape.h): `ReplayFile` holds `tape` (setup, cheats,
every step's inputs and dt, every suspend and resume) in place of its own
`settings`, `god`, `infinite_ammo`, per-seat `inputs` and `suspends`, and adds
only what checks a replay: the developer seed, content and stage hashes, tune
diff, update count, checkpoints, final fingerprint and `gap_updates`, the Level
update each gap was noted at. `ReplayRecorder` records through a
`RoundTapeRecorder` (at the dt the director hands it each step) and
`ReplayPlayer` plays through a `RoundTapePlayer`, keeping its neutral input once
the tape is exhausted. `inputs_by_seat()` gives the old seat-major view the
writer and the probes read.

The gap models now agree. The Debug recorder used to note suspends only, so a
replay never repeated what a resume does (re-read which jumps are still held)
and a jump held across a pause could diverge. A new
`DebugDirector::match_resumed` hook, called where `GameLevel` calls
`Level::prepare_resume`, notes resumes with their seat commands, and playback
repeats both through `RoundTape::apply_gap`
(`DebugMatchControl::debug_apply_gap` replaces `debug_suspend_level`): the
level's jump state only, no voice paused and resynced. Each gap is repeated at
its recorded Level update, as format 4 did, not at its tape step, because tape
steps count only the updates that read seats: the countdown sits at step 0 and
the zoom-out and reveal at the last step, and a suspend there (which clears a
mover's carrier, part of the fingerprint) would otherwise move. The adversarial
review found that case after a first version keyed playback to tape steps.

The file is **format 5**: one inputs array per participant, all the same
length, every step at the file's dt (the writer refuses otherwise), and an
ordered `gaps` list of `{ step, update }` with a resume's seat commands.
Formats 3 and 4 are refused with the version message; a replay holds only on
the build that recorded it (`DBG#2`). The tape's own seed is not written:
`seed` remains the developer seed the round's seed is drawn from, which
`HYG#39` replaces. [The debug console](debug-console.md#seed-record-and-play),
the instant replay's "How" and the vertical-lift draft's replay row in
[the roadmap](feature-roadmap.md) describe this.

The probes followed the new shape. Checks that read legacy format 3 now check
that 3 and 4 are refused; new checks refuse unequal seat lengths, a step at
another dt and gap updates missing or out of order, and the codec round trip
covers a resume. `practice_replay_checks.h` reads the new public
`RoundTape::gaps()` instead of reaching the private member.

Both presets build under `/W4 /WX`. The full default acceptance list passed in
Release (74 cases) and in Debug without the four slow cases (71), and
`debug_console` passed in Debug (record/play on a real `Level` with a suspend
at update 300 after the countdown). The sixteen opt-in cases compiled in Debug,
`turbulence_slab_routes --replay` passed when run by hand with its private trial
files, `check-painter.ps1 -CompileOnly` compiled in both presets, its
`-PracticeReplayOnly`, `-RememberedSetupOnly`, `-KeyboardJumpOnly` and
`-RumbleOnly` runs passed in Debug, `check-startup.ps1` passed on the Debug
executable and `check_doc_links.py` passes.

### One owner for weapon and projectile names (`HYG#36`)

**Done 2026-10-30.** [weapon_type.h](../game/objects/weapon_type.h) now holds
`WEAPON_NAMES`, the one spelling of each selectable weapon (`sprayer` to
`bucket`, then `random` for `WeaponType::random_primary`), with
`TUNED_WEAPON_COUNT`, `weapon_name`, `weapon_from_name` and
`for_each_tuned_weapon(visit)`, which calls `visit(type, name)` in enum order;
[projectile_type.h](../game/objects/projectile_type.h) holds `PROJECTILE_NAMES`,
`projectile_name`, `projectile_from_name` and `for_each_projectile`. The
hand-written lists are gone: tuning's projectile parser (its refusal now lists
the table), splash animations, impact sounds, weapons and projectiles in
[tuning.cpp](../game/tuning.cpp) and their four checks in
`tuning_validation.cpp`; the tool-motion frame check in
`content_validation.cpp`; the idle drip, muzzle size, tool motion and grip
readers and the weapon-select choice keys and descriptions in the presentation
readers (`presentation_read::weapon_choice_keys` is gone);
`last_match_setup.cpp`'s two arrays, whose reader still falls back to the
sprayer for an unknown name or `random`; the Debug `MatchPlanner`'s list, parser
and printer, which still prints `other` for anything but a tuned weapon; and the
balance runner's tool parser. Every visit keeps the order the calls were
written in, which was enum order, so diagnostics are raised in the same order.
The other probes keep their own literal names on purpose: they check the files'
spellings independently, or only label a failure. Both presets build under
`/W4 /WX`; the full default acceptance list passed in Release (74 cases) and in
Debug without the four slow cases (71, `balance_runner` included).

### Menu page geometry in one place (`HYG#37`)

Make page geometry a pure function of the presentation records in one module
that owns the design size, and have the reader, the validator and the pages call
it; move `DESIGN_RESOLUTION` out of `states/menu_page.h`. Split presentation
validation by page rather than by size where the shared geometry allows.

**The module, done 2026-10-30.** [menu_geometry.h](../game/menu_geometry.h) at
the `game/` root owns `menu_consts::DESIGN_RESOLUTION` (moved unchanged;
`menu_page.h` includes it, so no probe changed) and the reader's caps by name
(`paper_max_width` 1760, `setup_paper_max_width` 1840, `scan_paper_max_width`
1880, `paper_footer_margin` 40, `overlay_max_y` 900, `overlay_paper_max`
1600x900). Its functions are the page rectangles: `design_area`, `centred` and
`centred_x`, `footer_paper` (centred above `footer_y`), `screen_paper` (centred
on the design: Exit, Display, Pause, End, Results), `paper_interior` (inside
three keylines and the shade), `paper_text_right`, `footer_band` and
`footer_rule`, `setup_art` and `display_art`, `settings_action_width` and
`results_fill_box`. The reader, every presentation validator,
`menu_content_validation`, `stage_scan_diagnostics` and the pages call them; no
bare 1920, 1080 or cap literal is left in the reader or validators except in
message text. `MenuPage::calculate_center_position` forwards to `centred`;
Pause, End and Results build their paper at `screen_paper` instead of moving it
with `set_position_at_center`, the same rectangle exactly; `stage_preview`
frames its camera on `design_area`; `Validation::check_menu_item` takes the
paper rectangle instead of a size and a content height. Where the reader or a
validator associated a sum differently from the page (interior bottoms, right
edges, the Results fill's right edge, the controls value width), it now uses the
page's form, which can move a limit by one ulp for fractional data and changes
nothing for today's whole-number values. Diagnostic order and text are
unchanged. Both presets build under `/W4 /WX`; the full default acceptance list
passed in Release (74) and in Debug without the four slow cases, `menu_text`
passed in Debug and `check-painter.ps1 -CompileOnly` compiled in Release.

**Drift the shared geometry exposed.** Mapping every formula found places where
the three sides disagree; the module keeps each as it was, so these are separate
changes:

- D1, a visible question for the author: every page draws the footer band from
  `footer_y` (960) to the bottom, including over a match, while Pause and End
  papers (900 tall, centred on the design) reach y 990 and Results (860) 970,
  so the band covers their bottom 30 and 10 pixels. Nothing relates those boxes
  to `footer_y`. Whether that is intended is the author's call (`POL#45`).
- D2: the Pad sticks validator still checks rows pad, move, aim, rumble, apply
  at 4 and back at 5, but the page has nine rows (Back at 8), so its status
  overlap check tests the wrong card.
- D3: End row 0's labels are fitted to the whole paper, ignoring the shade that
  every other End row subtracts.
- D4: the practice Pause layout (five rows at 0.8 spacing, the Tool label) is not
  validated.
- D5: three meanings of "inside the paper" (interior, face, whole box) are used
  for the same kind of check on different pages.
- D6-D10: Display's setup shade checked only for Keep/Revert; the setup shade
  never checked against a setup box; `footer_y` range-checked in `main_menu`'s
  reader but read by the controls reader too; Credits' fit checked only on the
  page; static stage navigation and hints missing checks the numbered ones get.

Left: correct D2-D4 and D6-D10 in the validator, then the order-preserving split
by page (keep the nine phase functions in `Validation::validate` as the owners of
order, move each phase's per-page blocks into per-page files).

### Presentation includes and the content layer (`HYG#38`)

Trim the 24-line include block copied into 13 `presentation*.cpp` files to what
each uses, so `build_info.h` and the `states/` badge header appear only where
referenced. Move the badge geometry the roster validator needs beside
`PlayerIdentityPresentation`, and stop `presentation.h` dragging the menu
records into gameplay objects.

### An explicit match random stream (`HYG#39`)

Replace the global `srand`/`rand` seeding of a round with a `MatchRandom` value
passed into `LevelBuilder::build_level`, with independent sub-streams for
palette, bot seeds and the monitor persona; `RoundTape` stores the seed it
hands in. Seeded team pairs and colours may change; record the new `+seed`
mapping where the screenshot loop documents it.

### `Level` composed of owned parts (`HYG#40`)

Group the 28-parameter constructor into a few parameter records built by
`LevelBuilder`; extract owned collaborators (the intro/opening presenter,
horizontal motion, practice, the HUD/draw layer, match audio) with their own
state; give `game/ai` a narrow world-query interface that `Level` implements,
instead of `bot_*` members. Update the guide's description of `Level`.

### `GameLevel`'s suspension rules in one object (`HYG#41`)

Replace the four focus/pause booleans and their branches with one owned
suspension object that answers whether audio may play and whether to count in,
ask the director once per callback for its gate, and make the layout restore
and Debug attachment scopes created in `init()`.

### Structure queries instead of casts (`HYG#42`)

**Done 2026-10-30.** Structures, players and projectiles now derive from the
new abstract [Collider](../game/objects/collider.h), a game-side
`labrador::CollisionObject`. Its final `on_contact` asks the other party to
`touch` this one, and that party's `touch` calls `on_structure_contact`,
`on_player_contact` or `on_projectile_contact`. `Player` and `Projectile` no
longer decode the tag of what they touched, and the painted structures no longer
cast it. `Collider` answers the roles `structure()`, `projectile()` and
`paintable()`. `Collider::of` is the one conversion from the engine's type and
refuses anything else. That cross-cast is now `ENG#29`'s exhibit in the
[ledger](review/engine-reports.md). [Structure](../game/objects/structure.h)
answers `jump_through`, `ramp`, `ramp_rises_right`, `flat`, `slab_geometry`,
`top_paint_team`/`top_paint_colour`, `holds_flat_top_paint`, `passable_for` and
`cover_point`, and the roles `mover()` and `gate()`. With those, `Player`,
`Projectile`, `Level`, `LevelObjectBuilder`, content validation and the stage
preview have no `dynamic_cast` to a structure leaf left. Of the 33 in
`game/objects`, only `Collider::of`'s and a terrain-shape cast remain. The base
tag tests are virtuals now. `Weapon::has_aim_marker`,
`advance_travel_motion` and `motion_pose` are overridden by `WeaponRoller`.
`Projectile::resolve_travel`, `launch_hits_players` and `duds_on_bare_cover`
are overridden by `ProjectileRolling`, which also answers both contact handlers.
The translating structure's redundant roller test is gone. Some tag tests
remain: those for the sprayer and mister, which have no class of their own; the
bucket and sniper ones beside them; `is_swept`, the tunnelling contract's table;
and the tag-only classifications (`is_structure`/`is_projectile` in the bot
queries, idle drips, the any-slab test in `resolve_sweep`). None of these casts,
and they keep the per-step cost where it was. `lethal_provenance` now checks
that a tag-only proxy is refused rather than damaging by tag. The OOP guide
gained a double-dispatch row, and its projectile and scored-object recipes now
say to answer these queries. Both presets build under `/W4 /WX`. The full
default list passed in Release (74 cases), including the bot trials' fingerprints
and the movement, paint, slab, gate and moving-structure cases. In Debug
without the four slow cases, 68 passed in the working tree. `moving_structure`
and `bot_runtime` did not compile there, and `balance_runner` did not run,
because of `HYG#35`'s uncommitted `ReplayFile` work in that tree. All three,
with `lethal_provenance`, passed in a scratch worktree holding only this
change. `check-startup.ps1` passed on the Debug executable.

### Menu page bases named by role (`HYG#43`)

**Done 2026-10-30.** The five settings pages (Controls / audio, Pad sticks,
Keyboard keys, Pad buttons, Profiles) now derive from `SettingsPanelPage` in
[game/states/settings_panel_page.h](../game/states/settings_panel_page.h)
instead of `PauseMenuPage`. It has no `finish()`: its only way out is `close()`,
which pops like Back, and its `debug_dismiss` closes (Keyboard keys still
refuses during a capture). It holds the panel scaffolding the five had copied:
the opening slot and its input (`opening_slot`, `opening_slot_input`, formerly
the pause page's `pausing_player`), the panel style and look, `build_panel`
(grid, background, paper card and both widget trees), `add_text`, `fit_text`,
the status-line box, `covers_screen` and the paper's own focus card.
`PauseMenuPage` and its `finish` now serve only `PauseMenuInitial` and
`PauseMenuConfirmation`. `MainMenuPage` is a thin main-menu-frame base (its
look, `connected_slots` and starting a debug match), and `RosterSetupPage`
under it holds the roster machinery (seat sounds, the missing-controller wait,
host row cursor, profile Boost, the between-rounds series) for Player Count,
Join, Team, Weapon and Stage Select; Title, Home, Credits, Options and Mode
Select no longer inherit it. `MenuPage` lost `shows_brand`, which all 18 pages
overrode to false, with the brand drawing and font it guarded, and
`animates_focus_backing`, which none overrode. `menu_text.brand` and
`menu_style.chrome.brand_font` and `brand_position` stay in the format and in
validation; the painter menu-text probe, whose test page was the only thing
that drew the brand, no longer compares a brand crop. The OOP guide,
architecture overview, AGENTS.md, debug-console and menu-presentation docs
follow. Both presets build under `/W4 /WX`. Release ran the full default list
in the working tree (74 passed). Debug ran in a scratch worktree holding only
this change, since `HYG#35`'s uncommitted replay work stops some probes
compiling in the working tree: 71 passed without the four slow cases, and
`menu_text` and `debug_console` passed. There, `check-painter.ps1 -CompileOnly`
compiled in both presets, and the Release `-MenuTextOnly`, `-KeyboardJumpOnly`,
`-PausedOptionsOnly`, `-StickReadoutsOnly`, `-RumbleOnly` and
`-OptionsDefaultsOnly` matrices passed. `check-startup.ps1` passed on the
Debug executable.

### Menu owner and navigation (`HYG#44`)

**Done 2026-10-30.** `MenuContext` borrows a `MenuOwner`
([game/states/menu_owner.h](../game/states/menu_owner.h)) in place of its four
owner slots: `update_menu_music`, and `suspend_menu_music`, `rescan_stages` and
`stage_scan` with do-nothing defaults. `GameMenu` and `GameLevel` implement it
privately and pass themselves to their context; `GameLevel` answers only the
music clock, so a match still lends no stage folder. A context with no owner (the
probes, the developer tools' detached context) stays silent and finishes a page
exit at once, as an unset music slot did. The fifth slot, the pending page exit,
stays a one-shot `std::function`: it is the page's continuation, not the owner's.
Every page change inside the main menu's frame is now a `MainMenuRoute` followed
by `MainMenuPage::go_to`, and `make_main_menu_page` in
[game/states/main_menu_route.cpp](../game/states/main_menu_route.cpp), which
replaces `GameMenu::build_page` and draws the setup flow as one graph, is the one
place those pages are built. `MainMenuScreen` gained `join`; the End menu's
between-rounds re-picks build through the same factory. Probes that set slots
use stand-in owners: small local ones in `story_audio` and `user_stages`, and
`ScriptedMenuOwner` in
[scripted_menu_owner.h](../scripts/acceptance/painter_visual/scripted_menu_owner.h)
for the painter checks, attached through `MenuContext::set_owner`. Both presets
build under `/W4 /WX`. The full default acceptance list passed in Release (74
cases) and in Debug without the four slow cases, except `bot_runtime` and
`moving_structure`, which did not compile against uncommitted `ReplayFile`
changes already in the working tree and touched by nothing here.
`check-painter.ps1 -CompileOnly` compiled in Release (in Debug only
`practice_replay_checks.h` failed, on the same `ReplayFile` changes), the
Release `-HomeExitOnly`, `-HomeReturnFocusOnly`, `-StageScanOnly`,
`-StagePaperOnly`, `-EndMenuOnly`, `-EndContinuationOnly`,
`-JoinStageMotionOnly` and `-RememberedSetupOnly` matrices passed, and
`check-startup.ps1` passed on the Debug executable.

### Device-to-command in one place (`HYG#45`)

**Done 2026-10-30.** `SeatCommandBuilder`
([game/objects/seat_commands.h](../game/objects/seat_commands.h)), owned by
`GameLevel`, is now the one place device slots become seats: each participant
takes its `player_num()` slot's command, a seat with no slot gets neutral input,
and a keyboard/mouse seat's aim is resolved there through its pane and camera.
`GameLevel` refreshes the level's layout before building, so a resized window's
camera is the one aimed through, as when `Level::update` did it. `Level::update`,
`prepare_resume` and the round tape's resume gap take one command per
participant; `Level` lost its `Mouse*`, `resolve_mouse_aim` and the slot reads,
and `LevelBuilder` its mouse argument. A participant's intro skip reaches the
level as `request_intro_skip`, honoured by the next stepping update exactly
where the slot check ran, and `GameLevel::draw` hands the cursor marker the
pointer through `set_pointer`. `PlayerInputData` is the seat command; the pause
and intro-skip signals moved to `SlotInput`
([slot_input.h](../game/objects/slot_input.h)), which derives from it and is
what `PlayerInput`, the pause latch, the resume count-in and the Debug clock and
filters read. The seat's F1 overlay toggle stays in the command, since
`Player` applies it and tapes replay it. Debug replays no longer write a seat's
pause (64) and skip (256) flags, which no player ever read, and still read
files that have them; the format stays version 4. The keyboard defaults are now
stated once in `player_input_consts`, which `InputBindings` reads, and the pad
defaults once in `PadProfile`: `fixed_action_bindings` holds only the controls no
save changes, and `default_action_bindings` is `effective_action_bindings` of a
default `InputBindings`. Probes that script a level by slot map through
`acceptance::slot_seats` or `update_from_slots`
([slot_seats.h](../scripts/acceptance/support/slot_seats.h)), which use the
builder without a mouse; a temporary audit of every default case found the five
whose slots differed from their seats. The architecture overview and the OOP
guide follow. Both presets build under `/W4 /WX`. The full default acceptance
list passed in Release (74 cases) and in Debug without the four slow cases (71),
and `debug_console`, `crossfeed` and `press_tower` also passed in Debug. The Debug
opt-in, painter and frame-pacing compile gates and the Release painter compile
passed. In Release the `-Countdown`, `-ResultPercentage`, `-PracticeReplay`,
`-Monitor`, `-Identity`, `-LimbTuneColours`, `-ProgramLevelVisibility`,
`-Resume`, `-CqBlockout`, `-CursorTuneColours`, `-Hud`, `-Supply`, `-Closing`,
`-Drift`, `-Quiet`, `-Crossfeed`, `-PressTower`, `-TurbulenceBlockout`,
`-GatePermission`, `-FerryDirection`, `-SurfaceFinish`, `-RouteWarning`,
`-Landmark`, `-MiniatureTerrain` and `-RevealTuneColours` painter matrices
passed (the resume matrix now hands the level the pointer before its raw
render, as `GameLevel::draw` does), and `check-startup.ps1` passed on the Debug
executable.

### One owner for the level format (`HYG#46`)

**Done 2026-10-30.** [game/level_format.h](../game/level_format.h), still
engine-free and header-only, is now the level format's one owner: beside the
version it holds the key names (`level_format::key`), the bounds keys, the
object types (`object_type`), the collision tags as `level_format::Collision`
with their spellings, which tags each object type may carry (`may_carry`), the
material table (`materials`, `sits_on`, `allowed_materials`), the fixed slab's
material and refused fields, the four-spawn minimum and the own-half spawn
rule, and it names the station, gate, motion and slab contracts as the format's
parts. The game reads it in `LevelLoadedInfo` (which `static_assert`s the spawn
minimum against the pad count), `LevelObjectBuilder` (its tag table and
per-type allowed lists are gone) and `TerrainMaterial::make`, which now only
builds the look the table admits; the editor reads it in `level_json` (its
copy of the material table and `allowed_materials` are gone), the slab,
motion and blockout helpers, the document, the canvas and the mechanism
guide. Each side keeps its own messages and check order, so no level is
accepted or refused differently; the one wording change is that the editor
now names a fixed slab's refused `slab` fields in the game's order when two
are present. [level.schema.json](../game/level.schema.json) moved from `docs/`
to beside it; the three schema checks and the live docs follow. Reading a
level once into a typed record was considered and not done:
`LevelLoadedInfo`'s reads go through `JsonValue`, which names the file and key
path in every refusal, and its object entries must stay JSON for
`LevelObjectBuilder`, the one path that both validates and builds them, so a
record would add a second, unlocated reader rather than remove one. Both
presets build under `/W4 /WX`. The full default acceptance list passed in
Release (74 cases). In Debug without the four slow cases, 69 passed and
`moving_structure` and `bot_runtime` did not compile against the uncommitted
`ReplayFile` changes already in the working tree, which nothing here touches.
`check-level-editor.ps1` passed in Debug and Release (a first Debug run failed
the DPI probe's thread keyboard-state check once and passed on a rerun), and
`check-startup.ps1` passed on the Debug executable.

### One stage catalog (`HYG#47`)

**Done 2026-10-30.** [game/stage_catalog.h](../game/stage_catalog.h) owns the
ordered stage list and the level registry and is their only writer:
`StageDefinition` moved there from `stage_loading.h`, the manifest's `"level"`
kind calls `add`, and `UserStageFolder` and `load_user_stages` take a
`StageCatalog` in place of the registry and list pair. `GameData` holds one
`const StageCatalog*` (`set_stage_catalog`) in place of the two pointers, and
`stages()` and `level_infos()` now forward to it, read-only. The rescan is
unchanged in what it does: it drops and rereads the user stages, a vanished
stage's definition stays registered, and a read that throws puts the starting
list back, as building beside the live list did. `StageCatalog::definition`
answers by name from the list, and the Debug station lookup uses it. Probes
that offer their own lists from one shared registry build a catalog per list
with `acceptance::catalog_of` ([scripts/acceptance/support/stage_catalog.h](../scripts/acceptance/support/stage_catalog.h)),
which rereads each definition from `LevelLoadedInfo::source_path`, since a
definition cannot be copied. `ENG#26` in the ledger now asks only for the memory
back. Both presets build under `/W4 /WX`. In a clean worktree of the tip plus
this change, the full default acceptance list passed in Release (74 cases) and
in Debug without the four slow cases (71), `debug_console` and `menu_text`
passed in Debug, the Debug compile gate (the opt-in cases,
`check-painter.ps1 -CompileOnly` and `check-frame-pacing.ps1 -CompileOnly`)
compiled, and `check-startup.ps1` passed on the Debug executable.

### Save and the active profile (`HYG#48`)

**Done 2026-10-30.** [`SettingsSession`](../game/settings_session.h) now owns
what belongs to the run: which pad wears which profile and the effective play
settings and input bindings, the save's with each worn profile laid over its
pad, rebuilt in place so borrowed references stay current. It borrows the
`Save`; `wWinMain` declares it beside the save and `GameData` lends it as
`settings_session()`, a seventh pointer. Every reader of the settings in effect
and the Controls, Pad buttons, Profiles and setup pages moved to it, as did the
writes that take effective values (`set_play_settings_and_save`,
`set_input_bindings_and_save`), wearing, adding a profile from a pad, deleting
one (the wearers follow the shift) and carrying Boosts back. The `Save` keeps
the file: `saved_play_settings` and `saved_input_bindings` (each pad as
itself), the profile list, rename and Boost, and memory-only setters the
session writes through once. Write order, checks and the effective
references' identity are unchanged, and the file format is not touched. A
malformed section now prints one stderr line naming the save, the section's
key and the reader's error before it resets; sections that recover field by
field print nothing. [save_section.h](../game/save_section.h) states the
migration rule: no migration pass, each section reads every older layout in
place, a version only adds, a key whose meaning changes gets a new name, and
no section reads `"save_version"`. `match_records` gained
`check_section_reset_report`; probes that build a `GameData` hold a session
beside their `Save`, and those that test the file alone read the `saved_`
accessors. The architecture overview, the OOP guide and the roadmap follow.
Both presets build under `/W4 /WX`. In a clean worktree of the tip plus this
change, the full default acceptance list passed in Release (74 cases) and in
Debug without the four slow cases (71), and `debug_console` and `menu_text`
passed in Debug. A first `input_bindings` run failed on a probe that reassigns
its `Save` under a live session; those probes now reload through the session.
`moving_structure` once refused a stale temporary receipt folder whose name
reused the process id and passed on a rerun. The opt-in and painter compile
gates compiled in both presets, `check-frame-pacing.ps1 -CompileOnly`
compiled in Debug, the Release `-ControlsMenuOnly`, `-KeyboardJumpOnly`,
`-OptionsDefaultsOnly` and `-StickSettingsOnly` painter matrices passed, and
`check-startup.ps1` passed on the Debug executable.

### `const` means draw-safe (`HYG#49`)

**Done 2026-10-30.** `game/` has no `mutable` member left. The methods that
play, stop, pause or resume a sound are no longer const: `Weapon`'s and
`Player`'s sound methods (with the redraw pen and fire-loop flags), `Level`'s
`suspend`, `interrupt_players`, `pause_audio`, `resume_music` and player-sound
fan-out, `MovementAudio::play`, `MenuPage::play_wave` and the carried
`SoundBankObject`'s play and effect calls, as are `GameLevel::stop_rumble`
and `leave_match`, `Player::update_weapon_position` and the Debug
`filter_match_inputs`, which consumes the one-frame keyboard blank.
`Level::prepare_resume` stops a tool's loop through the new
`Player::stop_tool_sounds`, since `primary_weapon()` stays read-only, and Debug
playback's injected suspend goes through `DebugMatchControl::debug_suspend_level`
rather than the read-only `debug_level()`. `PlayerInput::player_inputs` and the
Debug menu model's `rows` are non-const, because a read refreshes their copied
mappings. `Projectile::note_paint_contact` is non-const; a surface reaches it
through `Projectile::answer_paint_contact`, the one `const_cast` in `game/`,
because the engine hands a contact's other party over const. That is engine
report 33 in [the ledger](review/engine-reports.md#owed-from-the-architecture-review-2026-10-30).
[The OOP guide](oop-guide.md#follow-one-shot-from-input-to-score) states the
rule. Behaviour, evaluation order and formats are unchanged. Both presets
build under `/W4 /WX`. In a clean worktree of the tip plus this change, the
full default acceptance list passed in Release (74 cases; `moving_structure`
first refused a stale receipt folder, as under `HYG#48`, and passed on a
rerun) and in Debug without the four slow cases (71), `debug_console` and
`menu_text` passed in Debug, and the opt-in and painter compile gates compiled
in both presets, with `check-frame-pacing.ps1 -CompileOnly` in Debug.

### One audio idiom (`HYG#50`)

**Done 2026-10-30.** Everything in `game/` that makes a noise now reaches its
bank as the carried `SoundBankObject`. `Level`, `Player` and `Weapon` gained it
as a base, and `MovementAudio` and `PaintAudio` derive from it, so no game
object holds a raw `labrador::SoundBank*` any more; `MovementAudio` now takes
the resources and the bank name. A sound that keeps playing and has to stop is
a `MatchVoice` ([match_voices.h](../game/objects/match_voices.h)): a small
`SoundBankObject` owning one effect instance, which does nothing when built
without a name. `Level` holds its sixteen voices as `MatchVoice` members. Its
constructor registers them, then each player's tool loop and redraw pen
(`Player::add_voices`, `Weapon::add_voices`), in one `MatchVoices` list, in the
order the hand-written lists used. `~Level`, `pause_audio` and `resume` walk
that list. The three sixteen-line lists are gone, and so are
`Level::pause_player_sounds` and `resume_player_sounds`,
`Player::pause_sounds` and `resume_sounds`, and the virtual
`Weapon::pause_sounds` and `resume_sounds`. Two voices keep their own rule as
overrides: `Player::RedrawVoice` pauses only while it sounds and, on resume,
stops if the redraw has ended; `Level::MusicVoice` resumes only from a pause,
at `music_gain()`. Each voice gets the same bank calls in the same order as
before, on an audible bank or a silent one. The time-up `stop_player_sounds`,
`Player::stop_sounds`, `stop_tool_sounds` and `Weapon::stop_sounds` stay,
because they are gameplay stops. The menu layer's `ScreenMusic`,
`ConnectionAudio` and `ResumeAudio` were already `SoundBankObject` components.
`GameLevel`'s results music stays its own; its suspension rules belong to
`HYG#41`. One order changes: `Level`, `Player` and `Weapon` now resolve their
bank as that base is built, so a missing bank throws before their other
resources do rather than after. Startup validation refuses such content
either way. The six probes that called the removed pause and resume methods
now register the object with a `MatchVoices` instead.
[The OOP guide](oop-guide.md#inheritance-and-ownership-answer-different-questions)
states the rule, and `docs/architecture.md` and `AGENTS.md` list the new bases.
Both presets build under `/W4 /WX`. In a clean worktree of the tip plus this
change, the full default acceptance list passed in Release (74 cases) and in
Debug without the four slow cases (71), and `check-startup.ps1` passed on the
Debug executable.

### Directories by role (`HYG#51`)

Last, because it moves files every other row touches. Give the `game/` root role
folders (match types, configuration with a presentation subfolder, persistence,
input, the engine-free shared contracts with an include check like the engine's
`check_engine_includes`), give `game/debug` subsystem folders, and decide what
`game/carried` becomes. Update `game/CMakeLists.txt`, the editor and the docs.

### Release package rules runnable locally (`CI#6`)

**Done 2026-10-30.** The Release ZIP inspection is now
[check-release-package.ps1](../scripts/check-release-package.ps1), the same
rules unchanged: no development files or unreviewed audio, the sixteen required
files, the bank's hash against `scripts/audio/runtime-bank.json` and none of the
sixteen developer-console RTTI names. It extracts into `-OutputDirectory`
(default `out/validation/release-package`, relative paths following the
caller's location), replacing only the `ColourWars-windows-x64` folder it
extracts, so it reruns locally after `--target package`. CI's inspection step
calls it with `$RUNNER_TEMP/ColourWars-release`, the folder the startup steps
already read. [export-msvc-environment.ps1](../scripts/ci/export-msvc-environment.ps1)
replaces the job's `Find vcvars64` step: it runs `vcvars64.bat` once and writes
every variable it adds or changes to `GITHUB_ENV`, so the ten `call
"%VCVARS%"` lines are gone and every later step has the environment those
steps built (including whatever `VCPKG_ROOT` vcvars leaves, as before). Locally
the package check passed twice on a fresh Release ZIP (34 checks, 66 files) and
refused a copy with a stray `.pdb`; the export script, run with
`-EnvironmentFile`, wrote 42 variables including `INCLUDE`, `LIB` and `Path`.
The hosted run is the first test of the CI wiring.

## Open shortlist (2026-10-29)

The author approved adding every proposal from the 2026-10-29 verified
brainstorm to the index. Each entry below is **open, not started**. Evidence
was re-read against the tree on that date; refresh line numbers before
starting. None of them closes a human gate, and any listening, feel, art or
attended-play judgment it touches stays a `DECISION` row.

### Unreadable save preservation (`HYG#22`)

**Done 2026-10-29.** `Save::load_save_data` now hands a save that fails as a
whole to `keep_unreadable_save` ([save.cpp](../game/save.cpp)) before keeping
the defaults. It copies the file beside itself as
`save_data.unreadable-<n>.json`, taking the first free `n` (up to 999), through
`std::filesystem::copy_file` with no overwrite, so an earlier copy is never
replaced. It prints one stderr line naming the save, the reader's error and the
copy; a failed copy prints its own line instead and changes nothing else. The
missing-file and legacy paths are unchanged. `match_records`'
`check_unreadable_save` loads a truncated and an invalid save in turn. It
checks that each leaves the defaults and the file untouched, prints exactly one
line naming both paths, and leaves a byte-identical copy; that the second does
not touch the first copy; and that a later `save_to_file` writes the real path
as version 5 and leaves both copies. The check failed against the old `Save`
before the change. The roadmap's save-layout record now says version 5 and
lists versions 4 and 5 and the nested `profiles` section. Both presets pass the
default acceptance suite. Still not decided: an in-game notice for a reset save,
and whether copies are ever pruned.

The record as filed:

A save that exists but will not parse is silently
replaced. `Save::load_save_data` catches the whole-file failure and keeps
`SaveData()` with no message ([save.cpp](../game/save.cpp) lines 433-441), and
the next match start writes that default state over the file:
`GameLevel` calls `remember_match_setup_and_save` on every match start
([game_states.cpp](../game/states/game_states.cpp) lines 237-239), which always
writes. A recorded round writes again (`record_result_and_save`, save.cpp
lines 568-579). The engine's `read_json_file` throws on open, read and parse
failures alike (`external/labrador/engine/assets/json.cpp` lines 66-89). The
existing protection is per section only: `load_from_json` resets one malformed
section (save.cpp lines 367-379), which the `match_records` case's
`check_section_isolation` covers; nothing covers a file that fails as a whole.

Scope: when an existing save fails to load, copy it beside itself under a name
that is never reused or overwritten, best effort, then print one stderr line
naming both paths before keeping defaults. A failed copy prints its own line
and changes nothing else. The missing-file and legacy-save paths are
unchanged. In the same change, correct the save-layout record in
[the roadmap](feature-roadmap.md#stage-records-and-the-sectioned-save-gap62),
which still says `"save_version": 3` and lists versions up to 3, against
[save.h](../game/save.h) lines 33-44: version 5, versions 4 (`"profiles"`) and
5 (`"last_match.stage_rule"`) listed, `profiles` nested, and the display and
play preferences flat.

First step: add the failing case to an isolated-save acceptance case
(`match_records` or `settings`) before touching `Save`.

Acceptance, CPU only: an invalid-JSON save and a truncated one each leave
defaults, one stderr line, and a byte-identical side copy; a
second corrupt load does not overwrite the first copy; a later
`save_to_file` writes the real path and leaves the copy. Both presets pass the
default acceptance suite.

Not decided here: any in-game notice or dialog for a reset save, and whether
side copies are ever pruned.

### Participant parameter names in `Level` (`HYG#26`)

**Done (2026-10-29).** `HYG#8` corrected the comments about participants and
panes but no identifiers (see [its record](#source-comment-contract-audit-hyg8)).
This commit renamed the identifiers, and nothing else: every [level.h](../game/objects/level.h)
parameter that indexes a roster participant in `players_` is now `participant`
in declaration and definition: `player`, `view_camera`, `idle_drip`,
`aim_marker_visible`, `player_identity_visible`, `player_identity_emphasis`,
`player_identity_bounds` and their `_for` forms, and `place_player`,
`respawn_player` and `kill_player`. The comment over the developer-tools block
lost its "`view` here indexes roster participants" caveat. Three loop and
buffer locals that walked `players_` under the name `view` (the death snapshot
in `update_level_logic`, `refresh_identity_texts` and `apply_player_inputs`'s
`by_view`) became `participant`/`by_participant`; a review follow-up renamed
the fourth, the figure-act loop in `level_intro.cpp`'s `update_acting`.

`view` is kept where it really is a human pane: `participant_for_view`,
`teammate_markers`, `hud_scale`, `update_teammate_markers`' loop and the pane
local in `refresh_player_cameras`. `draw_acting`'s unnamed `view` is the
Scene's view index and stays. `view_camera` keeps its name: it is the
cosmetic camera for that participant's pane, and its public name was out of
scope.

Verified: the diff changes identifiers and one comment only; both presets
build under `/W4 /WX`; the review follow-up ran the default acceptance suite,
which passes in Debug (73 cases) and Release. Release's `moving_structure`
first failed on a temp folder left by an earlier run with the same process id
and passed on rerun.

Not decided here: any rename outside `Level`, or of the public accessors
`view_count`, `participant_for_view` and `view_for_participant`.

### Stale README and AGENTS structure facts (`HYG#27`)

**Done (2026-10-29).** README now gives `game/` as ~69k lines with ~16k in
`game/debug/` and `editor/` as ~5.5k (tracked `.cpp`/`.h`, blank lines and
comments included, rounded), and lists the gate and ferry Turbulence Trials as
explicit-only. AGENTS.md, with the author's approval, names the three audio
backends and the `xinput`/`web` input axis and the Windows defaults this
repository gets. The record below is the finding as filed.

Two structural descriptions had drifted.
[README.md](../README.md) lines 176-177 give `game/` as ~34k lines with ~14k of
Debug tools and `editor/` as ~2k; tracked `.cpp`/`.h` files measured on
2026-10-29 are about 69k in `game/`, 15.7k in `game/debug/` and 5.5k in
`editor/`, counted with blank lines and comments. README lines 35-38 list the
explicit-only stages but omit the Turbulence Ferry and Gate Trials, both
`"include_in_random": false`
([ferry](../game/content/levels/turbulence_ferry_trial.json) line 543,
[gate](../game/content/levels/turbulence_gate_trial.json) line 507).
[AGENTS.md](../AGENTS.md) says the engine has two audio backends, but the
pinned engine offers `xaudio2`, `webaudio` and `null`
(`external/labrador/engine/CMakeLists.txt` line 325), and a separate
`LABRADOR_INPUT_BACKEND` axis, `xinput` or `web` (line 368), which AGENTS.md
does not mention.

Scope: correct those README lines and the AGENTS.md backend paragraph to what
the pinned engine and the tree say, without new prose about the web build.
Keep the counting rule stated or rounded so the figures do not need a refresh
for every slice. The AGENTS.md edit is project instruction text, so the author
approves it at commit time; the README edit can land without it.

First step: recount with the same rule as the source-size snapshot below and
re-read the backend `STRINGS` lines at the current pin.

Acceptance: each corrected figure and list matches a command recorded in the
commit message; the CI change-detection checks still classify the commit as
docs-only.

Not decided here: the AGENTS.md wording (the author's), and whether README
keeps line counts at all.

### Compile gate for opt-in probes (`CI#2`)

**Done 2026-10-29.** CI used to compile only what it ran: the fourteen opt-in
acceptance cases (`bot_trials` through `turbulence_slab_tools`, the entries of
`$availableCases` in [run-acceptance.ps1](../scripts/run-acceptance.ps1) that
are neither in the default list nor `balance_runner`), the painter and the
frame-pacing harness were never built there, and probes kept off CI drift (the
2026-10-28 painter run found stale fixtures nobody had noticed, the MattPC
paragraph under [Player profiles](feature-roadmap.md#player-profiles-road21)).
Those were runtime failures; this gate catches only the compile-breaking kind.

The `Compile gate for opt-in probes` step in
[ci.yml](../.github/workflows/ci.yml) runs in both preset jobs straight after
Build, and uploads `out/validation/compile-gate/` JSON and logs as
`compile-gate-<preset>`, `if: always()`. It runs:

- `run-acceptance.ps1 -SkipBuild -CompileOnly -Parallel 4 -Cases <the
  fourteen>`, with its own `-OutputDirectory` so its `acceptance-summary.json`
  never stands in for the real acceptance run's (`CI#4`). The case list is
  written out in the step; a new opt-in case must be added there too, as its
  comment says.
- `check-painter.ps1 -SkipBuild -CompileOnly`, new here. It compiles
  `painter_visual.cpp` twice, the normal and then the scripted
  (`COLOURWARS_PAINTER_SCRIPTED_PADS`) gamepad reader, to
  `painter_visual-normal-pads.exe` and `painter_visual-scripted-pads.exe`, then
  returns through the runner's `finally`, writing `result.json` with scope
  `compile-only`, outcome `compiled` and one `compiled_variants` entry per
  reader. That receipt keeps only the run's identity (preset, revisions,
  worktree status, profile); a failed compile writes the same scope with
  outcome `failed` and the readers it did compile. It launches nothing, so it
  makes no capture or GPU claim.
  `CompileOnly` ends in `Only` but is not a focused matrix: the one-leaf guard
  skips it, it refuses a focused selector before any output is created, and a
  normal run compiles its one reader under its old file names.
- `check-frame-pacing.ps1 -SkipBuild -CompileOnly` in the Debug job only. The
  harness accepts only `x64-debug` (its `ValidateSet`, `/MDd` and Debug
  libraries), like `balance_runner`, so the Release job compiles sixteen
  targets and the Debug job seventeen (fourteen cases, two painter readers,
  frame pacing).

[check-painter-selection.ps1](../scripts/check-painter-selection.ps1) keeps
`CompileOnly` out of its focused-switch inventory and qualifies it: both
presets route to the two readers (normal first) with no selector or native
flag, a normal-reader and a scripted-reader matrix each keep their one
compile, an explicit `-CompileOnly:$false` leaves routing alone, and
`CompileOnly` with a selector is refused without output (two selectors still
give the focus conflict first).

Verified on MattPC in a vcvars64 shell under pwsh 7, the step's commands
in both presets after an up-to-date build: all fourteen cases `compiled`, both
painter readers compiled, frame pacing `compiled` (Debug), with no probe
started. Then, with `bot_trials.cpp` given a `static_assert(false)`,
`painter_visual.cpp` an `#error` only under the scripted-pad define and
`frame_pacing.cpp` a `static_assert(false)`, each command exited 1 in both
presets with the compiler's error; the painter compiled its normal reader and
failed on the scripted one, showing both are built. The faults were made in
place in tracked files and restored from scratch copies, with their SHA-256
checked afterwards, because the runners only accept the checkout their
configured build came from. `check-painter-selection` and
`check-acceptance-selection` pass in Windows PowerShell 5.1 and pwsh 7, and
`scripts/ci` unit tests pass. A hosted run waits for an authorised push.

Not decided here: running any opt-in case in CI, a scheduled workflow, or a
GPU runner.

### Registration and failure-reporting checks in CI (`CI#3`)

**Done 2026-10-29.** [ci.yml](../.github/workflows/ci.yml) runs both
checkers after level-editor acceptance in both jobs, with `-SkipBuild` and an
explicit `-OutputDirectory` under `out/validation/`. The registration step
first builds `ColourWarsRegisterStage`, which is `EXCLUDE_FROM_ALL`. One
upload, `registration-failure-reporting-<preset>` (`if: always()`, seven
days), keeps the receipts, logs and dialog/stderr text by fixed-depth globs.
It does not take the registration run's scratch repository, its junction or
its staged game: those are about 150 MB a run, and `**` would walk the
junction.

Measured on MattPC under pwsh 7 in a vcvars64 shell, after an up-to-date
build. A poller sampled the foreground window's owner every 50 ms for the
whole of each run:

| Checker | Preset | Seconds | Outcome | Foreground |
|---|---|---|---|---|
| `check-stage-registration` | Debug | 22.9 | passed, startup `passed` | unchanged (Notepad) |
| `check-stage-registration` | Release | 20.6 | passed, startup `passed` | unchanged (Notepad) |
| `check-failure-reporting` | Debug | 1.3 | passed, both cases | unchanged (Notepad) |
| `check-failure-reporting` | Release | 1.0 | passed, both cases | unchanged (Notepad) |

Neither checker depends on focus or input. Registration starts a game only
through `check-startup.ps1`, hidden. CI already runs that script in both
jobs, and Release keeps its no-hardware-adapter allowance. The failure
checker finds its own process's `#32770` dialog, reads it with
`WM_GETTEXT` and closes it with a posted `WM_CLOSE`, so it needs no
keystroke or foreground window. Unlike the editor's Cancel check
([level-editor.md](level-editor.md), runs 290 and 291), neither checker
asserts focus or activation. Receipts:
`out/validation/ci3-timing/`. The CI step commands, run as written in both
presets afterwards, also passed and put their evidence under
`out/validation/stage-registration/` and
`out/validation/failure-reporting/`. The upload globs matched 102 files,
about 200 KB in all. A hosted run waits for an authorised push.

Not known until a hosted run: whether the runner's session shows the
dialog of a hidden-start process the way MattPC does. If the step fails
there, start with the failure checker's dialog search.

### Acceptance timing summary (`CI#4`)

**Done 2026-10-29.** [run-acceptance.ps1](../scripts/run-acceptance.ps1)
times each case's compile and run separately inside `$runCase` and writes
`acceptance-summary.json` beside the run's evidence
(`out/acceptance/<preset>/` by default). It holds the preset,
`compile_only`, `parallel`, the run's outcome, the shell, the wall seconds
from the first case scheduled, and one entry per selected case with
`compile_seconds`, `run_seconds`, `total_seconds` and `outcome`, sorted
longest first. Outcomes are `passed`, `compiled` (`-CompileOnly`),
`compile-failed`, `run-failed` (including a timeout), `cancelled`,
`runner-failed`, and `not-run` for a case the run never reached, such as
those after a serial failure; a serial case stopped mid-run is `cancelled`.
An old file is deleted as soon as case selection passes, and a new one is
written in the runner's `finally` once cases are scheduled, so a failed or
interrupted run leaves a fresh summary and one that stops in tool checks or
the build leaves none, never a stale one. Nothing is created before then,
which keeps the selection checker's no-artifact rule. The per-case console line now also shows the compile/run
split. [ci.yml](../.github/workflows/ci.yml) uploads it after consumer
acceptance in both jobs, `if: always()`, as `acceptance-summary-<preset>`.

Verified on MattPC: a Windows PowerShell 5.1 `-Parallel 4 -Cases
ammo,text_wrap,settings,paint,rumble` run and a pwsh 7 `-Parallel 4
-CompileOnly` run of the same five each wrote a valid summary with all five
cases (`passed` and `compiled`). A serial `-RunTimeoutSeconds 1 -Cases
movement_course,ammo` run recorded `run-failed` then `not-run` with outcome
`failed`. `check-acceptance-selection` and `check-acceptance-process` pass in
both shells. A hosted run waits for an authorised push.

Still open and not this row's: re-ordering `$longestFirst` follows from the
uploaded data in a later change. The Debug pull-request exclusions are gone:
pull requests now run every case, because a merge that carries the tree a pull
request passed is not built again.

### Local Markdown link check (`CI#5`)

**Done 2026-10-29.** [check_doc_links.py](../scripts/ci/check_doc_links.py)
runs in the always-on lint job after change selection. It reads tracked
Markdown outside `external/` and checks inline links, images and reference
definitions outside code. Each target must reach a file, and an anchor into a
Markdown file must reach a GitHub heading slug (duplicates numbered) or an
`<a id>`/`<a name>`. It allows targets under `out/`, `#` inside an existing
file name, external schemes, and missing files named by a document that opens
a line with "Historical" or "Frozen" in its first twelve. That rule matches
the four retired workshop studies and nothing else; a passing mention of
history is not a declaration. Those records' anchors into documents that still
exist are still checked, which is how `polish-refine.md` line 131 was caught.
Its first run on the tree found exactly the two stale `#retained-route-trials`
anchors. Both now point at `#route-trials-2026-10-20-provisional-profile`, and
the tree's 1,352 local links, 433 with anchors, resolve.
[test_check_doc_links.py](../scripts/ci/test_check_doc_links.py) covers slugs,
each allowance, a broken-anchor fixture, missing files, code spans and fences,
the historical rule and the `external/` exclusion. All 24 tests in
`scripts/ci` pass, and change selection is untouched, so a docs-only change
still skips the Windows jobs. The first hosted run (PR #26) failed on
five links into `external/labrador` that resolve at the pinned engine but
not in the lint job, which had never checked the submodule out; the lint
checkout now takes the pinned engine (`submodules: true`, not recursive).

The record as filed: nothing checked relative links. `scripts/ci/` holds only
`lint.py` and its tests, run by the always-on lint job
([ci.yml](../.github/workflows/ci.yml) lines 21-36). A survey of tracked
Markdown outside the engine on 2026-10-29 found nine unresolved local targets,
two of them real: [polish-foundation.md](polish-foundation.md) line 94 and
[polish-refine.md](polish-refine.md) line 131 link
`polish-movement-course.md#retained-route-trials`, a heading now titled
"Route trials (2026-10-20 provisional profile)" (line 208). The rest are
evidence under `out/`, sources a historical document says were retired
(`polish-style-kit.md`), and files with `#` in their names
(`game/audio/paint_exe/PROVENANCE.md`).

Scope: `scripts/ci/check_doc_links.py` with unit tests, run in the lint job
over tracked Markdown outside `external/`. It resolves files and GitHub
heading anchors; it allows targets under `out/`, documents that declare
themselves historical, and `#` in file names. Repoint the two anchors in the
same change.

First step: the checker and tests against fixtures, then a clean run on the
tree.

Acceptance: `python3 -m unittest discover -s scripts/ci -p test_*.py` passes,
including a broken-anchor fixture; the checker reports nothing on the tree
after the two repoints; a docs-only change still skips the Windows jobs.

Not decided here: checking external URLs, or links inside `external/labrador`.

## Startup-check evidence paths (`HYG#21`)

**Done (2026-10-28).** `check-startup.ps1` and
`check-failure-reporting.ps1` now resolve explicit output paths from the
caller's PowerShell filesystem location. These finish the thirteen checkers
identified in the output-path audit. Default roots, run hierarchies and each
checker's initialization order remain unchanged.

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Both startup-related checkers still call `Path.GetFullPath(OutputDirectory)`. | Selected as `HYG#21`: finish the established caller-relative contract with two imports and two substitutions. Private preflight copies can qualify paths without starting a game. |
| 2 | The pending editor alignment feature has an unresolved Release font-retirement assertion. | Continue diagnosis separately; diagnostic passes do not clear its original qualification gate. |

Exact private copies reproduced both old scripts placing relative output
under the process directory. Startup used an inert executable and a synthetic
`Add-Type` failure after output creation, before its native helper loaded.
Failure-reporting really compiled/loaded its existing C# helper, then stopped
at an absent private build cache before any helper method or game launch.
That helper has no static constructor or field initializer. Receipts record
the real compilation and its private temporary files explicitly.

Fresh PS5 and pwsh candidate hosts each pass eight cases: relative
Unicode/space paths, absolute/default controls and non-filesystem refusal.
Receipt/file boundaries, three source hashes, private input bytes and caller
environment/location/stack are preserved. No CMake, C++ compiler, Git/CIM query
or game/native probe ran. Independent review confirms exactly two imports
and two output-expression substitutions, with all remaining script text
unchanged. `run-balance` keeps its existing repository-relative policy.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`hyg21-baseline/fd0ff88b0be949c38f882adc07956bec`, candidate PS5
`hyg21-candidate/c27cf4be40c04ee3be3143a8cddf6fa6` and pwsh
`hyg21-candidate/d9f5140ccb814165b85ac840fd0b8f9b`.

## Tool-runner evidence paths (`HYG#20`)

**Done (2026-10-28).** Continued the established caller-relative path contract
for `check-acceptance-process.ps1`, `check-stage-registration.ps1` and
`check-debug-console.ps1`. These three converted explicit output paths
with process-relative `Path.GetFullPath`.

The current ranked scope is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Three remaining runners have private preflight stops before native helpers, compilers or UI/process work. | Selected as `HYG#20`: reuse the existing filesystem resolver, preserving defaults, run layout and failure receipts. Three imports and three substitutions; independent of editor DPI diagnosis. |
| 2 | Startup has another caller-relative output conversion and can stop at a private `Add-Type` fault after output creation. | A separate bounded follow-up with an inert executable and explicit fault boundary. |
| 3 | Failure-reporting compiles a C# interop helper before output resolution. | Qualify its own initialization order separately; disclose helper compilation rather than claiming a compiler-free check. |

The scripts use `Resolve-CheckFileSystemPath` for their output values, retaining
acceptance-process's GUID root, registration's preset-bearing default but
direct explicit root, and debug-console's flat output directory. Defaults and
preflight order stay intact; `run-balance` keeps its repository-relative rule.

Exact private copies reproduced misplaced relative output in all three old
scripts before implementation. Fresh PS5 and pwsh candidate hosts each pass
12 cases: relative Unicode/space paths, absolute/default controls and explicit
non-filesystem refusal. An absent compiler stops acceptance-process; empty
PATH plus `-SkipBuild` stops registration at its required-CMake check; an inert
executable and synthetic throwing debug driver stop before the real helper,
tuning reads, profile cleanup or process/UI work. Exact receipt/file boundaries,
five source hashes, private input bytes and caller environment/location/stack
are preserved. No Git/CIM, CMake/compiler, native helper or probe executable
ran. The existing denied queries remain stopped.

Independent review confirms exactly three imports and three output-expression
substitutions, with all remaining script text unchanged. Evidence under
`out/validation/implementation-loop-20261028/`: baseline
`hyg20-baseline/683fad45e43b467dabbe2b71a77b896e`, candidate PS5
`hyg20-candidate/f8ae203df8ea451ba8a739c205730a63` and pwsh
`hyg20-candidate/b71d92d89f9148f5944fe9011a2c2861`.

## Probe-wrapper evidence paths (`HYG#19`)

**Done (2026-10-28).** Extended `HYG#17`'s caller-relative filesystem contract
to `check-movement-feedback.ps1`, `check-paint-fx.ps1`,
`check-display-awake.ps1` and `check-frame-pacing.ps1`. Their explicit
`OutputDirectory` values used `Path.GetFullPath`, so distinct PowerShell
and process directories placed evidence under the wrong caller directory.

The ranked follow-up shortlist is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | Four build/probe wrappers retain the same process-relative output conversion. Each can stop at missing-tool preflight before Git, compiler or native work. | Selected as `HYG#19`: reuse the existing filesystem resolver, preserving defaults, preset/GUID suffixes, preflight order and normal receipts. Four imports and four substitutions; low risk. |
| 2 | Five other wrappers had the same conversion, but different startup/helper boundaries. | Three are completed by `HYG#20`; startup and failure-reporting retain separate initialization requirements. |
| 3 | Vertical lifts (`ROAD#28`) would add a new gameplay affordance. | Current loader, carry, projectile, shadow, bot and replay paths explicitly model horizontal motion. Define the coordinated physics contract before promoting that larger capability; an axis toggle is insufficient. |

Each wrapper now dot-sources `check-path.ps1` and resolves its output value
through `Resolve-CheckFileSystemPath`. Existing default roots, run hierarchies,
options and preflight order remain unchanged. `run-balance` retains its
documented repository-relative policy.

Exact private copies reproduced misplaced relative output in all four old
scripts. Fresh PowerShell 5.1 and pwsh candidate hosts each pass 16 cases:
relative Unicode/space paths, absolute and default-root controls, and explicit
non-filesystem refusal for every wrapper. Empty private PATH stops each
runner at missing-CMake preflight, before Git, metadata/CIM, compilation,
native helpers or GPU work. Literal roots, preset/GUID layout, existing failed
receipt boundaries, five source hashes, private input files and caller
environment/location/stack remain correct. Independent review passes. The
denied worktree-status and global process queries remain stopped.

Evidence under `out/validation/implementation-loop-20261028/`: baseline
`hyg19-baseline/15f709722f8940a798475e02c95cca95`; candidate PS5
`hyg19-candidate/8efa89e5510c42acbea157d5dab619de` and pwsh
`hyg19-candidate/59468bc27ab042829558f504b1b3e9a9`. This qualification
establishes path/preflight behavior, with no new runtime or hardware claim.

## Painter initialization cleanup (`HYG#18`)

**Done (2026-10-28).** The first `HYG#17` native attempt exposed a separate
cleanup defect: painter pushes the repository location, then constructs its
evidence record before entering the `try` that restores the caller. An
initialization error therefore leaves PowerShell in the repository. The outer
qualification wrapper restored its own caller, but the runner did not.

The current bounded shortlist is:

| rank | evidence and candidate | value, scope and risk |
|---|---|---|
| 1 | The retained failed native receipt reports `checker_caller_location_restored=false` after evidence initialization threw. | Selected as `HYG#18`: guard only initialization with a catch that pops the runner's location and rethrows the original error. Small failure-cleanup change; preserve normal cleanup and failed-initialization receipt semantics. |
| 2 | Nine other validation wrappers still use process-relative explicit output paths. | Extend the established filesystem resolver in a later bounded scope, after tracing each wrapper's path base and preflight order. |

The evidence-record load and Debug rumble supplement now have a narrow
`try`/`catch`. Failure restores the single pushed location and rethrows the
original error. Successful initialization continues into the existing normal
`try`/`finally`, which still owns environment/process cleanup and its own
single pop. Failed initialization still writes no incomplete result record.
Git queries, settings, renderer behavior and public options are unchanged.

An ignored private fault-injection harness reproduced the old-source location
and extra-stack-frame leak. Fresh PowerShell 5.1 and pwsh candidate hosts pass
both initialization-failure and normal-cleanup controls. Exact private copies
of the runner use a synthetic evidence helper that throws before metadata
collection; local dummy tool commands throw if invoked. Original exception
identity/type/id/message, caller location/stack, process directory, environment,
private inputs and five source hashes are preserved. The successful synthetic
helper followed by a missing private cache retains the existing failed receipt
and single restoration. No Git, compiler, CMake, native helper or GPU runs.
The actual denied worktree-status query remains stopped.

Independent review and all 145 focused painter-selection cases pass in each
host. Evidence under `out/validation/implementation-loop-20261028/`: baseline
`hyg18-baseline/aa54ae3c8ed84152b9f27272fdb89d1b`; candidate PS5
`hyg18-candidate/0800c8425bd34f24ae33b8c4152aeb30` and pwsh
`hyg18-candidate/46237d47052d4f00a4a94b90d6f92fca`; selection checks
`hyg18-selection/ps5/106c335c33c545169e4430b30f0034af` and
`hyg18-selection/pwsh/83fe48e2719b4be3897fb2ed46f6d16f`.

## Caller-relative validation paths (`HYG#17`)

**Done (2026-10-28).** Fresh source review found that explicit relative
`OutputDirectory` values used `Path.GetFullPath`, whose base could differ from
PowerShell's location after `Set-Location`. Painter `TrialPath` had the same
issue. Existing selection checks aligned the two locations or passed absolute
paths, hiding the discrepancy.

This bounded shortlist was recorded before implementation:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Painter and acceptance input/evidence paths can resolve beneath the process directory instead of the caller's PowerShell location. | Selected as `HYG#17`: define caller-relative filesystem paths for these entry points and their selection checkers. Private preflight can prove location, quoting and hashes without a compiler or native probe. |
| 2 | Starting a left drag during a middle-button pan leaves both gestures active; releasing the middle button cancels the left drag. | A bounded editor usability candidate, recorded in the authoring shortlist; establish button ownership and native controls before implementation. |

One small shared path resolver now serves `check-painter.ps1`,
`run-acceptance.ps1`, `check-painter-selection.ps1` and
`check-acceptance-selection.ps1`. Explicit relative paths use the caller's
PowerShell filesystem location; absolute paths and default roots retain their
meaning. It resolves literal, potentially not-yet-existing filesystem paths through
the provider-aware
[PowerShell path API](https://learn.microsoft.com/en-us/dotnet/api/system.management.automation.pathintrinsics.getunresolvedproviderpathfrompspath),
then retains native full-path normalization and rejects other providers before
filesystem access. Selection/trial diagnostic order, preset/GUID suffixes,
quoting and global process-directory state are preserved. `run-balance` has an
explicit repository-relative contract and keeps it.

The actual private baseline reproduced misplaced output in both runners and
a wrong painter trial hash from different same-named process-directory bytes.
The existing selection checks now cover separate provider/process locations,
relative paths with spaces and Unicode, absolute controls, all three trial
selectors' bytes/hash/quoted arguments and restored caller state. Fresh
PowerShell 5.1 and pwsh hosts each pass 145 painter and 24 acceptance cases,
including the checkers' own relative evidence roots. Six resolver/API cases
and all six changed scripts' syntax pass in both hosts. No compiler, native
process helper or GPU runs during this CPU qualification.

The first native attempt stopped before compilation when Git worktree-status
collection hit a denied global ignore file. That query remains stopped: an
explicit painter `-OmitWorktreeStatus` option skips it and records a null status
plus an omission flag. HEAD/source hashes remain available. No Git settings
were changed and no clean status was manufactured. The Release native preview
leaf passes with this option: 68 readbacks, 26 rows and 16 crops under the
relative caller-selected root, with caller state restored and the omission
verified in its receipt. Default metadata behavior remains unchanged.

Independent review and six-file source-immutability checks pass. Evidence under
`out/validation/implementation-loop-20261028/`: failing private baseline
`hyg17-baseline/85f23b68914e44108d3f6e396f6451cb`, final four-host CPU batch
`hyg17-qualification/967a47bcf11643eba088bbe5a8c5f3aa`, native batch
`hyg17-native/1f97aa8d62eb480787300d0710db3bf8` (leaf
`e067d5a5840546299ce439f626526c7d`). Four more wrappers are qualified by
`HYG#19` and three by `HYG#20`; startup and failure-reporting need separate
scoping. The observed initialization location
leak is resolved by `HYG#18` above, without retrying the denied metadata query.

## Follow-up shortlist (2026-10-28)

The author authorized autonomous selection once ready work is exhausted or
blocked. This ranked shortlist was recorded during editor qualification;
its evidence describes the code before the associated fixes:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Unicode folder matching: `editor/user_stage_save.cpp` folds paths with locale-sensitive `towlower`, while the save guard uses Windows ordinal comparison. | Check non-ASCII case aliases, including a not-yet-created My Stages folder. Small scope; coordinate after the editor changes land. |
| 2 | Bounded painter builds: `scripts/check-painter.ps1` invokes `cmake --build` without a job count; the acceptance/editor runners already cap build concurrency. | Directly supports five projects sharing the PC. One argument change; low risk and independent of the active editor investigation. Selected as `HYG#9`. |
| 3 | Acceptance process timeouts: `scripts/run-acceptance.ps1` has no per-case compiler/probe deadline. | Useful for unattended loops, but needs generous limits for slow Debug cases and cleanup restricted to owned children. Medium scope/risk; retain for later scoping. |

After `HYG#11`, a fresh source scan found five other local check wrappers
without a CMake build limit. The next ranked shortlist is:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Complete build caps: display-awake, failure-reporting, frame-pacing, movement-feedback and paint-fx wrappers still have uncapped `cmake --build` calls. | Selected as `HYG#12`: five argument-only changes, low risk, directly useful on the shared PC. |
| 2 | Acceptance subset CLI: `Cases` treats `ammo,text_wrap` as one unknown case under `pwsh -File`, while `ExcludeCases` explicitly handles comma lists. | Small usability fix; scope selection normalization and preflight validation before implementation. |

### Focused painter preflight (`HYG#16`)

**Done (2026-10-28).** Selected the second candidate from `HYG#15` after
compatibility qualification. All 100 current focused switches had native
mappings, but the separate manual count and late argument chain allowed a
future added switch to fall through to the full matrix. A scratch copy adding
only `FutureUnmappedOnly` reproduced that fallthrough at missing-CMake
preflight before the fix; no compiler or native probe started.

The count now derives from actual bound `*Only` switch values that are
present; explicit false values remain inactive. TrialPath and Debug-only
checks retain their diagnostic order. The existing selector chain follows
`Quote-Path`, with unchanged branches/operands and an initially empty
selector-only argument array. An active selector with no native argument
fails before output/tool setup. The original quoted content/capture paths
are prepended once at the existing launch point. Independent source review
confirms all 100 ordered clauses, parameters and trial/Debug rules are
preserved. Reader, capacity and native dispatch policies remain unchanged.

The actual-runner preflight checker passes 138 checks in both PowerShell 5.1
and pwsh: all 100 discovered selectors, default/false/conflict cases, three
trial paths and Debug-only refusals. The synthetic unmapped selector now fails
before output/tool creation, while false preserves ordinary routing. Private
relative-path fixtures align both provider and process directories and restore
both; receipts confirm unchanged source and caller environment.
Final receipts under `hyg16-selection/` are
`final-ps5/b73e0742b7bf4beb883ef2ceadff432d` and
`final-ps7/f576d6cc5d8a4adfbb48904dd63bb7ee`.

The native Release stage-preview-background leaf also passes under 5.1:
68 readbacks, 26 rows and 16 native 64 by 32 crops, including real menu reopen
transitions. It verifies the final quoted launch prefix and caller restoration;
it does not run all 100 native matrices or establish visual/hardware acceptance.
Evidence is `hyg16-native/x64-release/6ea8af81f7d34ba69aee5400892f150c`
under `out/validation/implementation-loop-20261028/`.

That launch also exposed an inherited `PSModulePath` selecting PowerShell 7's
Utility module in host 5.1, leaving `Get-FileHash` absent despite `#Requires`.
Runner and checker now import their host's own standard Utility manifest
without reload, installation or environment changes. The same failing host
environment then passes native compilation/execution. CI runs both preflight
hosts and retains receipts; hosted CI has not been run locally.

### Painter PowerShell compatibility (`HYG#15`)

**Done (2026-10-28).** A fresh painter review recorded these findings before
the compatibility fix:

| rank | candidate and evidence | value, scope and risk |
|---|---|---|
| 1 | Twelve expressions in eleven focused verifiers use `SHA256.HashData` and `Convert.ToHexString`, unavailable under Windows PowerShell 5.1. The runner explicitly supports that host, but these calls happen after native work. | Selected as `HYG#15`: one compatible hashing helper and narrow call substitutions; preserve exact bytes, uppercase hexadecimal and every oracle. |
| 2 | Focused-selection wiring repeats 100 switches across the one-leaf guard and argument chain. All current leaves are wired, but a future missed branch could run the full matrix. | Lower priority: scope a fail-closed preflight later; no registry rewrite or speculative discovery CLI. |

The actual retained stage-preview-background verifier first failed at
`HashData` under PowerShell 5.1, without native execution or evidence writes.
One shared `SHA256.Create()` / `ComputeHash()` helper now disposes its algorithm
and returns identical uppercase, hyphenless hex. The twelve substitutions retain
the eight local wrapper names and every input array/slice.

Actual retained Track and Scribble replays also confirmed five missing
`Math.Clamp` calls. Their admitted operands are finite doubles with fixed
0..255 bounds. Inline `Max(0.0, Min(255.0, expression))` now preserves
each expression exactly once, its existing rounding, alpha division and byte
casts. Independent review confirms the same quantization for admitted values;
no numeric admission, literal pin, schema or comparison was weakened.

Four independent Python `hashlib` vectors pass in fresh 5.1 and pwsh hosts:
empty, ASCII, all byte values and a binary boundary sequence. Each returns one
uppercase digest without changing input bytes. All eleven actual affected
verifiers and their PNG completion phases pass under each host, on scratch
copies with fresh child processes. Hash manifests confirm every original
retained capture/log/receipt is unchanged. Replay batches under
`out/validation/implementation-loop-20261028/` are
`hyg15-replay-Desktop-3fcaef5a41ff4d0d921ab7deb776ce87` and
`hyg15-replay-Core-ddf1fc3453e24606a66012c6bf198d79`.

The native HUD-health leaf passes under 5.1 Release and pwsh Debug: each checks
54 readbacks, 12 rows/crops and 36,864 RGB pixels; Debug additionally checks
nine real tuning commands, 36 readbacks and private Save/rebase/reset behavior.
Native evidence under `hyg15-native/<preset>/` has IDs
`6f8feabd3b9647ebb8723325d1ffecdb` (Release) and
`91f743e4b16641e59b75b3a43d1de942` (Debug).

The initial native preflight also found this session had not loaded the
standard `Get-FileHash` function. An explicit Utility requirement initially
addressed that preflight; `HYG#16` subsequently pinned module loading to the
host's own manifest after finding a cross-edition inherited module path.
Neither change installs a module or supplies a hash shim.
Fresh direct-runner preflight in both hosts passes fixture hashing before the
expected missing-tool refusal, with no compilation. All thirteen touched
scripts parse in both hosts and independent review passes. These checks do
not establish hardware or attended visual acceptance.

### Editor build-profile isolation (`HYG#14`)

**Done (2026-10-28).** The next ranked candidate from the
[editor review](level-editor.md#raw-nul-json-admission-ed12) was an observed
tooling defect: `check-level-editor.ps1` redirected `LOCALAPPDATA` before
configure, so vcpkg lost the caller's existing registry cache. During `ED#11`,
this triggered a fresh registry fetch that failed at the unavailable GitHub
connection despite installed dependencies. It was selected after both metadata
preservation slices were qualified.

The profile assignment now follows build/cache preflight and precedes
the first acceptance executable. Build tools inherit the caller's environment;
the document/native checks and every editor launch retain the private profile.
`SkipBuild`, arguments, concurrency, nested guidance isolation and the outer
`finally` restoration are unchanged. Dependency-root and install-policy
settings are unchanged by the source fix.

The baseline captured the actual configure-call environment, then returned an
explicit failure before executing CMake; it confirmed the redirected profile.
The final one-off receipt records two real forwarded CMake invocations per
preset, both inheriting the caller's profile, followed by complete passing
document/native/window checks and caller restoration. The build used installed
dependencies with manifest installation temporarily off, then restored the
original `ON` cache settings; the failed network fetch was not retried.
Independent ordering review and parsing pass in PowerShell 5.1 and pwsh.

Profile receipts are under `out/validation/implementation-loop-20261028/hyg14-profile/`:
baseline `baseline/8d625ab173d94d2388a31f2cd61aeed3`, final
`candidate/8ac69dfe92b84b7588d25cb97de0a339`. Full editor evidence is under
`out/acceptance/<preset>/level-editor/`: Debug
`8efc0067637248eeb2ff6b426bbfb32c`, Release
`46b5227ad30748b8862add8a01be105e`. These source/tooling checks do not establish
attended editor, gameplay or hardware acceptance.

### Acceptance subset CLI (`HYG#13`)

**Done (2026-10-28).** After completing the build caps, the second candidate
now normalizes each explicit `Cases` value by splitting commas, trimming each
name and applying invariant lowercase. It preserves array arguments and order,
default Debug/Release selections, unknown-name and duplicate refusals,
Debug-only tool restrictions, and exclusion behavior. Empty selected names,
including empty comma fields, remain errors. No discovery/list option or
default-case expansion is needed for this defect.

The new selection checker first reproduced the comma/whitespace failures on
the old runner. All 22 checks then pass in fresh PowerShell 5.1 and pwsh hosts:
the actual selected list/order, defaults, arrays, normalization, empty/unknown
names, duplicate precedence, Debug-only restrictions and exclusions. An actual
current-host `-File` child verifies command-line binding. These checks use
private scratch directories and empty PATH, stopping at the required
missing-CMake preflight; no native helper, compiler or CMake starts. Caller
PATH/location are restored and the receipts include source hashes.

Final receipts under `out/validation/implementation-loop-20261028/hyg13-selection/`:
PowerShell 5.1 `ps5/b631eb4adade4c1992a1907ebb8bee0e`, pwsh
`ps7/ae967210690e4a3696968c998ac2b865`. A real native `-File` invocation using
`-Cases " ammo , TeXt_WrAp "` also passes ammunition/text wrapping in Debug
under 5.1 and Release under pwsh. Independent source review and whitespace
checks pass. CI runs selection qualification in both shells alongside process
qualification and retains their receipts/logs; hosted CI has not run locally.

### Remaining local build caps (`HYG#12`)

**Done (2026-10-28).** Each of the five wrappers above now passes
`--parallel 1`, matching the existing acceptance, painter and editor wrappers.
The argument-only changes preserve
targets, configure arguments, `SkipBuild` branches and probe execution. The
stage-registration wrapper already has an explicit two-job limit; hosted CI
keeps its separately chosen concurrency.

Independent review and parsing of all five scripts pass in PowerShell 5.1 and
pwsh. A scan of all script `--build` calls finds no remaining uncapped local
wrapper. No new test repeats the literal flag, and this source change provides
no new gameplay, hardware, rendering or performance acceptance.

### Bounded painter builds (`HYG#9`)

**Done (2026-10-28).** The painter wrapper now uses `--parallel 1`, matching
`run-acceptance.ps1` and `check-level-editor.ps1`. This bounds CMake/Ninja
build fan-out when five projects share the PC. Configure, visual-probe
execution, scope switches and `-SkipBuild` behavior are unchanged.

PowerShell parsing and scoped whitespace checks pass. Review confirmed the
only command change is the explicit job limit; no new test repeats that
literal argument, and no game/runtime behavior changed.

### Windows folder case matching (`HYG#10`)

**Done (2026-10-28).** A new regression case first failed on the original
comparison: not-yet-created Unicode case aliases were rejected. The helper
now uses `CompareStringOrdinal` with case ignored, as the save guard already
does. Absolute/lexical normalization and trailing separators are unchanged;
no locale is changed and no filesystem identity lookup was added.

The checks exercise both directions of e-acute and a-umlaut case pairs before
creation, then independently verify existing aliases through the filesystem.
They retain ASCII/dot/separator behavior, distinct/sibling/nested refusals,
ASCII filename and shipped-name rules, source/My Stages guidance, actual
save/reopen/ordinary Save and original replacement spelling.

Independent review, all three editor targets and complete editor checks pass
in Debug and Release. Evidence: Debug `ef93762bdefe4e70bdc6d0de24e3b914`,
Release `c71f6a2d9fbe4077a32ac99b6059d1cf`, under
`out/acceptance/<preset>/level-editor/`. The original failing run is retained
in `out/validation/implementation-loop-20261028/hyg10-baseline-run.log`.
Discovery destinations, junction/symlink policy and human gates are unchanged.

### Acceptance probe deadlines (`HYG#11`)

**Done (2026-10-28).** A hung acceptance executable previously held its serial
run or parallel slot indefinitely, and its buffered output was written only
on exit. `RunTimeoutSeconds` (default 3,600) now bounds the executable
phase after successful compilation. The runner preserves case order, `Parallel`,
`CompileOnly`, UTF-16 compiler response files and the serial UTF-8 tool scope.

The Windows-specific process helper supports Windows PowerShell 5.1 and pwsh.
It launches the probe suspended, assigns it to a private
kill-on-close [Job Object](https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects),
then resumes it. Explicit inherited output handles keep parallel cases separate.
It retains merged output incrementally in a raw sidecar and finalizes the existing
UTF-8 log with its captured encoding. PowerShell polls in short interruptible
steps inside `try/finally`; disposal/cancellation cleans only owned jobs and
queued probes do not launch after cancellation. Launch failure, timeout and
uncertain cleanup fail the case. A containment denial cancels the run instead
of launching uncontained or repeatedly retrying that denied action.

Compiler and initial CMake execution remain outside this slice. `/MP`
enables [synchronous PDB writes](https://learn.microsoft.com/en-us/cpp/build/reference/fs-force-synchronous-pdb-writes?view=msvc-170)
through MSPDBSRV; current flags do not establish exclusive ownership of such
a server if supplied by compiler environment options. A compiler job could
therefore capture shared work. This is an ownership uncertainty, not proof
that the current invocation starts one. Compiler invocation is preserved, and
already-running compiles finish during parallel cancellation. A core cancellation
token also skips queued `CompileOnly` compiles without loading the native helper.

Independent review and syntax parsing pass in both shells. The isolated native
fixture passes eleven real-process checks in each: success/nonzero/missing
launch, Unicode/quoted arguments, legacy decoding, large merged output,
flushed partial output on timeout, owned sleeping-child cleanup, an unrelated
owned sentinel surviving, normal-parent-exit child cleanup, parallel log
separation, queued cancellation and serial pipeline interruption. The interruption
check exercises PowerShell pipeline cancellation and `finally`, not injected
console input. Known process handles replace global process inventory.
An additional guard extracts the actual runner's `runCase` script block and
checks canceled `CompileOnly` work never calls its compiler delegate.

Final receipts under `out/validation/implementation-loop-20261028/hyg11-process/`:
PowerShell 5.1 `ps5/8a785e248f9947939621af5259d47d39`, pwsh
`ps7/91ec7c2faf4f42118ccd9c647b7f959a`. Each records source hashes and case facts.
Actual runner checks also pass: Debug ammunition/text wrapping with two slots
under 5.1, Release equivalents serially under pwsh, and Debug `balance_runner`'s
UTF-8 path. Both fresh shells compile into Unicode/spaced output paths without
loading the helper or running the executable; response files remain UTF-16 LE.
Caller PATH, location and console output encoding are restored.

A one-second Debug `menu_text` runtime deadline produces the expected named
timeout, retaining both logs and restoring the caller environment. That case
emits nothing before its first completed check, so its retained logs are empty;
the isolated fixture separately proves flushed partial-output retention. The
interrupted `menu_text` run is not a gameplay acceptance pass. The sixteen CI
change-detection regressions pass. CI now runs the isolated fixture in both
shells and retains receipts/logs; hosted CI itself has not run locally.

## Scope and working rules

File length identifies candidates to inspect, not a maximum or a reason to cut
a cohesive implementation. Preserve the game's inheritance, public signatures,
names, layouts and behavior. Engine and vendored code are outside this pass.
Keep historical review documents as written.

For a split, move whole definitions and their private helpers together wherever
possible. Preserve evaluation, callback, RNG, diagnostic and floating-point
operation order. Do not combine a split with bug fixes, renaming, broad
formatting, a new abstraction model or gameplay changes. An extraction that
needs new private parameters must say so; it is not a literal source move.

Register new game translation units in [game/CMakeLists.txt](../game/CMakeLists.txt),
with the existing PCH, include and warning conventions. The
[acceptance runner](../scripts/run-acceptance.ps1) already compiles a case's
same-named folder of `.cpp` files. Header-based visual/document fixtures have
their own inclusion context; preserve it or wire an explicit replacement.

Review the moved definition bodies and narrow build/include changes, then build
Debug and Release and run the existing checks named in each scope. Do not add
tests that merely mirror a move. Comment-only work needs caller/implementation
review and a comment-only diff, not a new runtime test suite. Keep structural
changes and behavioral defects in separate commits when implementation lands.

## Source-size snapshot (2026-10-28)

These counts include blank lines and comments in tracked authored files. Refresh
the affected files before starting; this is audit evidence, not a permanent size
gate. The earlier figure/animation and other source splits already exist.

| file | lines | audit result |
|---|---:|---|
| [presentation_validation.cpp](../game/presentation_validation.cpp) | 25 | Ordered coordinator and nine family sources; `HYG#1` complete |
| [player.cpp](../game/objects/player.cpp) | 1,848 | Retain the coupled movement/contact/carrier core; figure code is already separate |
| [input_bindings.cpp](../scripts/acceptance/input_bindings.cpp) | 56 | Four topic sources and shared fixtures; `HYG#5` complete |
| [evidence-record.ps1](../scripts/check-painter/evidence-record.ps1) | 354 | Four explicit topic helpers; `HYG#6` complete |
| [painter_visual.cpp](../scripts/acceptance/painter_visual.cpp) | 1,347 | Retain the explicit include/setup/dispatch entry; no registry redesign selected |
| [level.cpp](../game/objects/level.cpp) | 966 | Horizontal-motion subsystem extracted (380 lines), `HYG#2` complete |
| [tune_colour_document_checks.h](../scripts/acceptance/debug_console/tune_colour_document_checks.h) | 17 | Ordered topic coordinator; `HYG#7` complete |
| [tuning.cpp](../game/tuning.cpp) | 907 | Retain cohesive key-by-key parsing and validation |
| [level_document.cpp](../editor/level_document.cpp) | 949 | Retain the shared document transaction; `ED#7` save protection complete |
| [projectile.cpp](../game/objects/projectile.cpp) | 287 | Contacts/sweeps extracted (587 lines), `HYG#3` complete |
| [weapon.cpp](../game/objects/weapon.cpp) | 855 | Retain the current firing/lifecycle core; motion already has its own file |
| [save.cpp](../game/save.cpp) | 602 | Profile overlay extracted (209 lines), `HYG#4` complete |

## Ordered presentation validation (`HYG#1`)

**Done (2026-10-28).** `Presentation::validate_resources` is now a 25-line
ordered coordinator over nine validation families. The private nested worker
borrows presentation/resources and carries concrete already-computed setup,
stage, controls and roster geometry/font contexts. It performs no eager
resource lookup or measurement. Shared helper and metrics definitions live
with those families; `sound_references` and `validate_stage_counts` moved as
whole methods. The largest cohesive family remains 737 lines.

The public API and member layout are unchanged. Independent source comparison
reconstructs all nine original phase statement streams after explicit
receiver/context transfers, plus sixteen helper bodies, three metrics and the
two catalog methods. Lookup/calculation order, floating-point operations and
the Stage/Results/roster interleaving remain intact. The qualified `ROAD#25`
paper-face boundary is the retained baseline.

Debug and Release builds, `menu_presentation`, `weapon_catalog`,
`stage_selection`, loader/user-stage checks and hidden accepted-bank startup
pass. The instrumented existing `menu_text` case compares the preserved
original source against the split with current headers, compiler and generated
build identity: **3,709 exact first diagnostics match in each preset**. This
includes five multiple-invalid-field probes across resource and interleaved
page families. Only scratch filename prefixes are normalized. All existing
menu-text assertions also pass in both baseline and candidate executions.

## Level horizontal motion (`HYG#2`)

**Done (2026-10-28).** Moved the horizontal admission helpers and all eight
motion query, admission/budget, prepare, commit, zero-step and snapshot methods
together into `game/objects/level_horizontal_motion.cpp` (380 lines). The
current `level.cpp` is 966 lines; its update coordinator and call order stay
in place. Public signatures, member layout, diagnostics and pre-step paint
ownership timing are unchanged.

Independent source comparison confirmed that the helper block and all moved
method bodies are identical, including their float/double arithmetic. The
remaining implementation differs only by the move and includes. Debug and
Release builds and the existing `moving_structure`, `paint_ferry`,
`fixed_slab`, `movement` and `instant_replay` checks pass. Debug retains the
manual/public-loader checkpoint, paint-credit and terminal-RNG replay checks;
the shipping seven-arena replay passes in both configurations.

## Projectile contacts and sweeps (`HYG#3`)

**Done (2026-10-28).** Moved the private sweep/relative-impact helpers and
all eleven definitions from `resolve_launch_obstruction` through
`flush_paint_audio` into `game/objects/projectile_contacts.cpp` (587 lines).
Construction, diffusion, movement, lifetime and accessors remain in the
287-line `projectile.cpp`; the contact helpers remain private to their new
translation unit.

Independent block comparisons confirm identical definitions and retained
sweep ordering, launch exemptions, relative moving-surface geometry,
endpoint deduplication and impact/paint/audio callback order. This only moves
source and adjusts includes/registration. Debug and Release builds and
`blocked_shot`, `tunnelling`, `moving_structure`, `fixed_slab`, `paint` and
`instant_replay` pass, including Debug's public-loader replay checks.

## Save profile overlay (`HYG#4`)

**Done (2026-10-28).** Moved `copy_slot`, the play-preference/binding setters,
effective-preference accessors and profile/wearing/Boost operations into
`game/save_profiles.cpp` (209 lines). The 602-line `save.cpp` retains JSON
sections, migration, loading and atomic writing. Display settings stay with
the resolution conversion used by serialization; they have no profile overlay.

Independent comparison confirms identical helper and method bodies, including
effective-reference identity, per-slot association, mutation/write order and
failure semantics. The public API and save schema are unchanged. Debug and
Release builds and the existing `player_profiles`, `settings`,
`input_bindings`, `handicap` and `remembered_setup` checks pass, including
isolated real write/replacement failures, active-setting recovery and reloads.

## Input-binding acceptance topics (`HYG#5`)

**Done (2026-10-28).** The 56-line `input_bindings` entry retains `main` and
the single fake `GamepadReader`/state seam. Four topic sources hold independent
domain/oracle checks, actual reader/resume checks, save/schema/relaunch checks
and keyboard actions. `fixtures.h` shares only the required fixtures, literal
oracle tables and declarations; the acceptance runner discovers the sources.

Exact source-group comparisons preserve all assertion/oracle literals,
function bodies, invocation order and isolated profile lifetimes. Shared names
now use `input_bindings_acceptance`; defaults live on declarations and shared
constant tables have inline linkage. No production helper replaces an oracle.

The existing `input_bindings` case passes in Debug and Release. In each
configuration, separate `relaunch-write` and `relaunch-read` processes also pass
against the same retained private profile, preserving the real persisted-J
reader and unchanged-save checks. No player data or hardware is used.

## Painter evidence metadata (`HYG#6`)

**Done (2026-10-28).** The 354-line entry retains its ordered base record,
all 98 explicit precedence conditions and the four final fixups. Their bodies
are dot-sourced from coherent arena (15 branches), colour (31), menu/text (44)
and action/HUD (8) helpers under `scripts/check-painter/evidence-record/`.
The unique selector parameters do not shadow existing inputs; no moved body
uses the helper's script path. There is no registry or painter-matrix change.

Independent inspection and PowerShell token comparison retain every condition,
body token, literal string, ordered key and dot-sourced failure boundary. Base
and final-fixup statements are byte-identical. All five scripts parse. The
baseline/candidate harness passes 800 ordered metadata comparisons: every one
of 99 focused switches in both presets, adjacent/first-winner precedence,
PracticeReplay overrides and combined final fixups. Only game revision, engine
revision and working-tree metadata are normalized.

Representative native `FerryDirectionOnly`, `OptionsDefaultsOnly`,
`CrackTuneColourOnly` and `ResumeOnly` leaves pass in Debug and Release,
covering every extracted group and retaining their existing scope receipts.
No full painter matrix or new timing claim was needed for this source move.

## Colour-document fixture families (`HYG#7`)

**Done (2026-10-28).** The 17-line `check_colour_document` entry coordinates
six topic helpers in four headers: splice/alias behavior, selectors/feedback,
opening artwork, and HUD/figure/identity colours. The interleaved additions
retain their original order: generic, feedback, opening art, HUD,
crack/acting, then limbs/identity. All headers keep the original anonymous
namespace and `last_key` inclusion context.

Reconstructing the original function body from those ordered helper bodies
reproduces all 1,038 lines unchanged after physical newline normalization.
Literal JSON, source-span assertions, conflict/reset checks and byte-preserving
fixtures are unchanged; no production accessors or registration were added.
Independent review and the existing `debug_console` case pass in Debug and
Release, including Release's executable exclusion checks.

## Source-comment contract audit (`HYG#8`)

**Done (2026-10-28).** Corrected the three bounded comment groups against
current implementations and callers. Source edits are comments only; the
executable tokens, APIs, member layouts and behavior are unchanged. An
independent caller review agreed with the corrections. No new runtime tests
were added for this comment-only slice.

Completion evidence, by edited declaration or claim:

| comment location | verified implementation/caller |
|---|---|
| `PlayerBuilder::build_players` declaration and local identity comment | `player_builder.cpp` keeps roster order, increments `human_view` only for humans, passes participant to sound and `paint_owners::index(setting.num, participant_index)` to the weapon |
| `SeatSetup::num`, `MatchSetup::device_slot` and `Player::player_num` | `match_setup.cpp` assigns human slots, gives bots `-1` and remaps only the device; `Level::apply_player_inputs` reads stable slots; `results_menu.cpp` maps participants through `paint_owners::index` |
| `paint_owners.h` owner-domain comment and `Player` constructor's sound comment | `PlayerBuilder` uses slot-based human paint credit and participant-derived bot credit; the constructor receives a separate participant sound index |
| `Level` participant/pane, feedback and acting comments | `Level` constructs `view_participants_` only for humans; `take_rumble_events` retains `player_num()`; `level_dev_tools.cpp` indexes players by participant; `level_draw.cpp::refresh_identity_texts` falls back to bot difficulty or human device; `level_intro.cpp::update_acting` and `draw_acting` index `acts_` alongside `players_` |
| `Level::update`, `DebugDirector::update_match`, `GameLevel` adapter declarations and `game_level_dev_tools.cpp` heading | `game_states.cpp::update` calls the director in both builds; the Release inline hook forwards to `debug_update_level`, which supplies `RoundTapeRecorder`; `game_level_replay.cpp::step_replay` supplies `RoundTapePlayer` |
| `Level` shipping setup/replay adapter comment | `game_states.cpp` calls `set_pad_devices` during build and update; `game_level_replay.cpp::start_replay` restores pad mappings and cheats |
| `DebugDirector` Release overview | `debug_director.h` retains ordinary update forwarding and `opening_mode` pass-through; `debug_director.cpp::run_launch_arguments` retains the Release argument warning |
| `RoundTape` invalidation overview | `GameLevel::debug_place`, `debug_respawn` and `debug_kill` call `spoil`; `RoundTape::set_cheats` spoils after a recorded step; `tune_commands.cpp` applies live edits without tape invalidation; the `ROAD#19` owner records that exception and Debug-injected suspends |
| `GameData::stages` and `MatchSetup::set_stage` | `stage_loading.cpp` appends accepted user definitions after manifest stages in file-name order; `MainMenuStageSelect::set_level_settings` chooses the selected stage's registry name |
| `LevelDocument::unresolved_resources` and `LevelLoadedInfo::validate_content` icon comments | both still check `icon_frame` in `sprite_sheet_1`; `MainMenuStageSelect::update_stage_select_visuals` selects `StagePreview`, not the legacy icon |

The accurate `stage_loading.h` rescan, borrowed-registry and no-match lifetime
comments were retained: `GameMenu::init` lends the callback to Stage Select,
whose main-menu entry rescans, while between-round entry does not. Missing
definitions remain registered but unreachable from the stage list. Historical
reviews and the engine's design decisions were left untouched.
