# Code hygiene and maintenance

This is the live owner for `HYG` rows in [TODO](../TODO.md). The author asked
for source-file splits and comment audits on 2026-10-28. These are independent
maintenance slices alongside the gameplay backlog; they do not change its
arena/tuning order. Since 2026-10-29 it also owns the CI-gate rows `CI#2` to
`CI#8`; `CI#1`, the foreground debug-console check, stays with
[the debug console](debug-console-record.md#foreground-checks-ci1).

## Hygiene review (2026-11-05)

On 2026-11-05 the author asked for a large code hygiene review and for its
findings to go into TODO the same way as the architecture review's. The
findings are recorded, frozen, in
[the review](review/2026-11-05-code-hygiene-review.md); this section owns their
status. 500 verified findings are grouped into the rows below. The one engine-side
finding is ledger draft `ENG#35` in [engine-reports.md](review/engine-reports.md).

The rows keep the rules the [architecture rows](#architecture-review-2026-10-30)
work under. They may rename, add a small shared helper and move code between
files. They preserve behaviour, the inheritance-first style (no T11 findings),
save and content formats, and evaluation and RNG order, unless the row says
otherwise. A row that changes behaviour (`HYG#58`-`HYG#60` and parts of others)
says so in its commit. Each row builds Debug and Release and runs the acceptance
cases its files touch. A row that only edits probes runs those probes in both
presets, and a row that only edits scripts runs the matching `check-*` script
or selection checker. Each row names its sites; a site that stands for a group
says so ("and siblings", "about twenty"), and the row starts with a grep for the
rest. Refresh line numbers from `8a04c998` before starting. Work the defects (`HYG#58`-`HYG#60`) first, then in
the listed order. Several rows are independent and can be taken in any order;
the rows that depend on another say so.

### Controls percentages on the real rows (`HYG#58`)

**Done 2026-11-05; behaviour change.** `settings_rows` in
[menu_geometry.h](../game/config/presentation/menu_geometry.h) names the value
rows of Controls (Music 4, SFX 5, the three toggles 6-8) and Sticks (Pad 0,
Move 1, Aim 2, Rumble 3, the three comfort rows 4-6). Both pages place their
rows from it, and `validate_settings_percentages` now fit-checks Sticks Move/Aim
and Controls Music/SFX instead of rows 1-4, so the SFX value is checked;
`validate_settings_toggles` takes its rows from the same names, and both stale
comments are gone. The `menu_text` percentage fixture places its Music and SFX
`OptionRow`s on the named rows (it used rows 3 and 4), and the painter's
percentage leaf takes its first value row from them.
[verify-percentage-text.ps1](../scripts/check-painter/verify-percentage-text.ps1)
builds both its expectation and its message ("Controls rows 4/5 and Sticks rows
1/2") from one table.

No rejection variant was added. A percentage that fails only on the SFX row
cannot be authored: values in one column share the font, width limit and help
column, so only the row's y differs, and a fitted value is at most a row high.
Anything that pushes the SFX value past the paper or the status line also
pushes the labels on rows 6-10 past it, which the reader (eleven rows above
`status_position`) and phase 1 reject first. Tried locally: a status line
through the SFX value is refused by the reader before validation runs. The
missing check was a latent gap, not a reachable failure.

Both presets build under `/W4 /WX`. `menu_text`, `menu_presentation`,
`stick_settings`, `settings`, `locales` and `menu_navigation` passed in Debug
and Release, `check-startup.ps1` passed on both executables, and
`check-painter.ps1 -PercentageTextOnly` passed in both presets. The verifier was
also run by hand against that run's evidence with one Controls row changed, and
threw the new message.

A follow-up commit cleared the sibling sites review found. The
[Controls row-label table](menu-presentation.md#controls--audio-and-pad-sticks-row-labels)
gave the pre-Profiles rows (Music 1, Apply/Back 6/7 and 3/4) and left out the
three Controls page actions, Rumble and the Sticks comfort rows; it now lists
all thirteen roles on their real rows, names `settings_rows`, and says which
actions are fitted. In `menu_text`'s `controls_checks.cpp` the fitted-label
fixture placed Controls' settings on rows 3-7 and now takes every row from
`settings_rows`, and the fitted-action fixture's Apply rows (8 and 4) are now
Pad Buttons 8, Controls 9 and Sticks 7. `menu_text` passed in Debug and Release
after it.

### Save and settings correctness (`HYG#59`)

**Done 2026-11-05; behaviour change.** Four commits:

- `Save::file_exists` ([save.cpp](../game/save/save.cpp)) asks
  `std::filesystem::exists`, and a failed query counts as present. A save that is
  there but will not open (locked, denied) now fails in the reader and goes to
  `keep_unreadable_save`, which copies it to `save_data.unreadable-<n>.json`,
  instead of being treated as missing and replaced with the defaults.
- An unloadable legacy save (`./save/save_data.json` beside the executable, read
  only when there is no per-user save) prints one stderr line naming it and the
  reader's error. It is still left in place, and the defaults are still written
  to the per-user path.
- `ControlsSettingsMenu::apply_settings` lays its five fields (both volumes and
  the three shared comfort switches) over the session's current settings through
  the new static `ControlsSettingsMenu::applied_settings`, as Pad sticks does. The
  back-fill list of child fields is gone.
- `StickSettings` gained a defaulted `operator==`, and `PlayerProfile`'s is now
  defaulted. Equality is unchanged today; a new field is compared automatically.

`match_records` gained two checks. `check_unopenable_save` holds the save open
for delete from another handle, which the CRT's open refuses and a file copy
does not, so the save is present, unopenable and still copyable. It requires the
defaults, one stderr line naming the save and the copy, a byte-identical copy and
an unchanged save. `check_legacy_save` runs a copy of the probe in a private
directory, because every probe shares the executable's `./save/`, against a
corrupt legacy save and the same file held open. It requires the stderr line, an
untouched legacy file and a per-user save of this version. `settings` gained
`check_controls_apply`. It commits child-page changes to sticks, rumble, comfort
and `aim_sensitivity` (a field no page edits, standing in for one a back-fill list
would miss) under an older Controls copy. After Apply it requires both sets of
changes, in the session and after a reload. Each check failed against the code it
replaced.

Both presets build under `/W4 /WX`. `settings`, `match_records`,
`player_profiles`, `remembered_setup`, `display_trial`, `input_bindings`,
`stick_settings` and `menu_navigation` passed in Debug and Release, and
`debug_console` and `menu_text` in Release. `check-startup.ps1` passed on both
executables. On MattPC, `check-painter.ps1 -ControlsMenuOnly` and `-RumbleOnly`
passed in both presets and `-StickSettingsOnly` in Debug and Release; those leaves
drive the real Controls and Pad sticks pages, including a Pad sticks change kept
by Controls Apply. The probe's `same_input` is `HYG#60`'s; the probes that
compare stick settings field by field were left as they are.

### Checks that can fail (`HYG#60`)

**Done 2026-11-05**, except three fixture items that no check notices (see
below). The game's behaviour is unchanged; the checks changed, and
the Session checks now run in Release too. Three commits:

- `bot_driver`'s mover hold requires a drop before it requires one after the
  hold, so a bot that never drops reports that, not an early drop.
- `menu_navigation`'s `InputVariant::write` makes the substitution, and refuses
  a missing key or unreadable `tuning.json`, outside the rejection `try`
  (`rejects`).
- The background supplement and rumble-owner summaries drop the constant
  `native_motor_calls` (the fixtures have recording motors only), and
  `recorded_updates` is the tape's own count; `verify-rumble.ps1` no longer
  reads the dropped field.
- Painter fixtures take their layout from one `players_layout` in
  `painter_fixtures.h`. Three players now get `three_player` in `countdown`,
  `setup_motion` and `team_setup`; the other sixteen fixtures give the same
  layouts as before.
- `PlayerInputData` and `SlotInput` have defaulted `operator==`, which
  `input_bindings` uses in place of its field list (`same_input`).
- The crossfeed fixture builds its objects with the gate and ferry cues and
  names its stage through `stage_title`, as `LevelBuilder` does. The route
  warning HUD bounds measure the level's own identity text and the locale's
  `health` label instead of literals.
- `debug_console`'s shipped-script lint resolves every `tune`, `tune.reset`,
  `tune.diff` and `tune.save` target against `tuning.json` and
  `presentation.json` as `TuneDocument` reads them, and checks `weapon`'s seat
  and weapon name. Each tune command keeps its own grammar from
  `tune_commands.cpp`: `tune` takes a file-prefixed leaf, `tune.reset` a leaf,
  a file or `all`, `tune.diff` a file or `all`, and `tune.save` a file, `all`
  or `force`, then only `force`.
- The shipping Session checks (end-page continuation, result and score text,
  `next_session`) moved from `debug_console`'s Debug-only `session_checks.cpp`
  to a new default case, `session`, which runs in both presets.
- `level_editor_dpi` addresses Align origin by `edit_align_origin`, and the
  clipboard check requires its collision entry before dereferencing it.
- `check-level-editor.ps1` reads the command ids from `editor/editor.h`
  (`Get-EditorCommandIds`); it and the save-guidance check send menu commands
  by id and refuse an id the menu lacks. The steps that toggle Blockout aids are
  named `*-blockout-toggle-ready`.
- [process-births.ps1](../scripts/process-births.ps1) counts the processes a
  check starts through a Job Object. `check-acceptance-selection.ps1` and its
  sibling `check-painter-selection.ps1` record and require that count, and the
  helper-loaded flag, from measurement; the comma-binding child may start only
  its host and that host's conhost.
- `prepare_review.py` compares each master with its own project's record
  (music `measurements.json`, foley `PROVENANCE.json`, interface `cues.json`);
  only interface cues carried a hash before, so music and foley compared with
  themselves.

Each check failed once against a broken input that was then reverted:
a mover hold multiplied by 1000 in `match_bot.cpp` ("A bot held on a moving
support past its bounded hold"); a renamed key in the rejection list ("Input
tuning field must exist"); a flipped `skip_intro` in the repeated sample
("Repeated reader access consumed or changed a sampled action edge"); a renamed
`tune` path and a misspelt weapon in shipped scripts ("no tunable
tuning.player.gravity_renamed", "weapon 4 sprayr in a 4-player script"), then,
after the per-command grammar, a bare `tune tuning` and a
`tune.save tuning.player.gravity` ("tune cannot take tuning", "tune.save cannot
take tuning.player.gravity") while `tune.diff all`, `tune.save force` and
`tune.save tuning force` passed; a
staged "Rematch!" in Release ("A finished single round or solo practice offers
Rematch"); an entry search that finds nothing ("Clipboard fixture lacks a
collision entry"); the Align menu item removed ("Align origin menu availability
does not follow the selected entry"); an id the menu lacks ("Editor menu has no
command 116"); a process started in each runner's preflight ("started 1
process(es)"); and a changed foley record hash ("Master hash differs from its
project's record: player.step.01").
Against background-supplement evidence copied from a passing run,
`verify-rumble.ps1` threw once `recorded_updates` read 419.

Three items do not meet the row's done criterion: no check notices them. The
layout, crossfeed and route-warning changes make the fixtures match production,
but putting each old input back still passes. With `four_player` for three
players, `-CountdownOnly` passed in both presets, and `-SetupMotionOnly` and
`-TeamSetupOnly` passed in Release. With the gate and ferry cues left out and the
old literal identity label, `-CrossfeedOnly`, `-RouteWarningOnly`,
`-GatePermissionOnly` and `-FerryDirectionOnly` passed in Release. The fixtures
are now right, but a later drift in them would still go unnoticed.

Both presets build under `/W4 /WX`. `bot_driver`, `menu_navigation`,
`input_bindings`, `session`, `instant_replay` and `debug_console` passed in
Debug and Release. On MattPC, `check-painter.ps1` passed `-CountdownOnly`,
`-SetupMotionOnly`, `-TeamSetupOnly`, `-CrossfeedOnly`, `-RouteWarningOnly`,
`-RumbleOnly`, `-GatePermissionOnly` and `-FerryDirectionOnly` in both presets.
`check-level-editor.ps1` passed in both presets, and both selection checkers
passed in pwsh and Windows PowerShell 5.1. `prepare_review.py`'s `collect`
passed and rewrote its outputs unchanged.

### Status text after the 2026-11-05 decisions (`HYG#61`)

**Done 2026-11-05.** Text only; the game's behaviour is unchanged. Every named
site now says what was decided on 2026-11-05: the bot policy and Boost values
in `tuning.h` and `tuning.json` were accepted (`ROAD#10`, `ROAD#5`), paint
underfoot ships on (`POL#30`, at the amounts `POL#36` built), report 28 was
declined, `ED#5` was accepted without its session, and the listening,
clean-PC, pad and finish checks (`ROAD#1`-`ROAD#3`, `POL#24`) were waived, not
performed. Sibling sites done with them: the bot comments in `tuning.h` and
`tuning.json`, `audio_pipeline.py`'s build refusal, the Paint.exe production
READMEs (`paint_exe/README.md`, `foley/README.md`, `music/PRODUCTION.md`),
foley's `review_status` in `build.py` and in the `PROVENANCE.json` it wrote
(edited in place, not re-rendered), `check-finish-details.ps1`'s header, the
`crossfeed_coupled`, `review_bots_boost` and `review_finish_details` headers,
and the "prototype" and "trial amounts" wording in `match_fingerprint`, the
`debug_console`, `paint_underfoot`, `handicap` and `moving_structure` probes.

A review of that commit found sites it missed, fixed in a follow-up: the opening
paragraph of `paint_exe/README.md` and `interface/README.md`, the third
production README; the `POL#23` and `ROAD#3` lines in `polish-movement-course.md`
and `backstory.md`; the "remain in their existing TODO rows" lines in
`polish-hud.md`, `polish-countdown.md`, `polish-match-accents.md` and
`polish-paint-effects.md`, which now say what was accepted and what was waived;
and the `stage_scan` `_comment` in `presentation.json`, whose action/help wording
the author accepted with `ROAD#32`.

`prepare_review.py` now says the accepted set ships and, only while the masters
match `acceptance.json`, what became of `POL#23` and `ROAD#3`; a changed set
still reads as candidates. Running it rewrote `cues.json`'s status, the
provenance header and its 103 per-wave status lines, and the review page's
status paragraph, and nothing else. It also re-encoded the three audition Oggs
with this machine's FFmpeg; those bytes are not reproducible, so they were put
back and are not part of the change.

Left as found: `acceptance.json`'s `remaining` field, the author's 2026-10-14
record as given; dated increment logs whose owner already carries a 2026-11-05
status note (audio inventory §9, `ROAD#10` and the `ED#5` checkpoints in
feature-roadmap.md and level-editor.md, and the polish plan's dated
increment notes under its 2026-11-05 calls); and the probes' PASS summaries that
say attended acceptance "remains open", which are output rather than comments
and go to `HYG#87`.

Both presets build under `/W4 /WX`, and the build's wave-bank check passed in
both. `paint_underfoot`, `handicap`, `moving_structure`, `debug_console`,
`menu_navigation`, `gameplay_tuning`, `settings`, `bot_driver` and
`instant_replay` passed in Debug and Release. The audio unit tests and
`audio_pipeline.py validate` passed, the Release ZIP passed
`check-release-package.ps1`, and `check-startup.ps1` passed on both executables
and the extracted ZIP. `check-finish-details.ps1` needs a foreground desktop and
was not run; only its comment changed.

The follow-up changed text and one JSON `_comment` only. Both presets rebuilt,
and `user_stages`, `menu_presentation` and `menu_text`, which read
`presentation.json`, passed in Debug and Release; the Markdown links resolve.

### Comments and reference docs that drifted (`HYG#62`)

**Done 2026-11-05.** Comments and docs only; the game's behaviour is unchanged.
Each named site now says what the code does:

- `GameData`'s class comment names its holders (states, menu pages,
  `MenuContext`, `StagePreview`, the developer tools) and says nothing in
  `objects/`, `ai/` or `carried/` names it.
- `Level::update` says the shipping bots drive first unless the seat filter
  replaces bot inputs, and the filter sees every input last; `main.cpp` adds
  `tune` to what mutates tuning and presentation; `level_inputs.h` says the
  four records are the constructor's (with `StageAudio` inside `LevelStage`);
  `save.cpp` says the first save fails before any page could report it, which
  is why it gets a dialog.
- `debug-console.md` lists all sixteen RTTI names (`DebugBackgroundInput` was
  missing) and `SkirmishBot` in the bot hierarchy, and says Release passes
  `Level::update` only the round tape as `seats` (not "always null").
  `README.md`'s tree has today's line counts (~78k, ~17k, ~7k), what RapidJSON
  reads and what `cmake/` holds (no wave-bank recipe; staging and checks).
  `level-format.md` says three of the four archived levels had their bank and music renamed and all use `level_name`,
  names the two drives (they are described below the line, not above), and
  counts thirteen registered stages. `docs/README.md` says what
  `gameplay-tuning.md` covers and that Press Tower and Turbulence's slab trial
  were accepted. `menu-presentation.md` names the real key,
  `main_menu.backgrounds.home`, at all three sites, and so does its sibling in
  `polish-main-menu.md`. `oop-guide.md` lists the optional `low` cue, and
  `architecture.md`'s invariants, re-checked, carry a date instead of a SHA.

Left as found: the dated `b96bad6`/`8e91aab` stamps in `feature-roadmap.md`,
which record what one past check covered rather than claim a present state.

Both presets build under `/W4 /WX`. `settings`, `level_format`, `bot_driver` and
`menu_presentation` passed in Debug and Release, and the Markdown links resolve.

### One atomic file writer and one debug file-stem rule (`HYG#63`)

**Done 2026-11-05; behaviour change.** A replay is no longer truncated in place,
a failed screenshot no longer leaves a partial PNG, and `exec` and
`perf.capture` refuse reserved Windows names.

- `replace_file_atomically(path, bytes)`
  ([atomic_file.h](../game/save/atomic_file.h)) creates the parent, writes a
  `CREATE_NEW` sibling named `<file>.<process>-<n>.tmp`, flushes it, moves it
  over the file with replace and write-through, and deletes it on any failure.
  The save (now serialised to a string first), the fatal-error record,
  `debug_files::write_file`, `tune.save` (including its `.bak` fallback),
  `ReplayFile::write` (which no longer ignores a `create_directories` failure)
  and the screenshot writer (which now encodes the PNG into memory) all go
  through it.
- The editor's one copy is `PreparedFile` and `replace_prepared_file`
  ([prepared_file.h](../editor/prepared_file.h)) in `colourwars_level_document`.
  It is split into prepare and replace because document Save and stage
  registration both recheck the destination in between. Six editor-linked probe
  cases gained an `editor_prepared_file_adapter.cpp`, as they already compile
  `level_document.cpp`.
- `debug_file_name` ([debug_file_name.h](../game/debug/script/debug_file_name.h))
  has `stem_character`, `valid_stem` and `max_stem` beside `reserved_stem`. The
  two refusing checks (`exec`'s `debug_script::file_name` and `perf.capture`)
  use `valid_stem`; the three repairing ones (`replay_name`, the screenshot name
  and `screenshot.dir`) use `stem_character`.
- `replay_names` lists with `increment(error)`.

The unique temporary changed how probes fail a save on purpose. Sixteen sites
made a directory at the fixed `save_data.json.tmp`, which no longer blocks
anything. They now hold the old save open, sharing read and write but not
delete, through `acceptance::ReplacementLock` ([replacement_lock.h](../scripts/acceptance/support/replacement_lock.h)):
`display_trial`, `input_bindings` (twice), `remembered_setup` and twelve painter
leaves. The three probes that ran a write both ways (blocked temporary, then
locked save) now run only the locked one, because the writer's temporary can no
longer be named from outside. The 50-odd checks that a fixed `*.tmp` was absent
would now pass vacuously; they ask `acceptance::temporary_beside`, which finds
any `<file>.*.tmp`. New checks: a locked replay and a locked PNG refuse a
write of different bytes, keep the earlier file whole and leave no temporary;
`exec CON`, `nul.cfg` and `Com1` and `perf.capture` with `CON`, `nul`, `Com1`
and `lpt9` are refused. The lock shares writing on purpose: the old in-place
writers (a truncating `ofstream`, WIC's `InitializeFromFilename`) open through
it and change the bytes, while the replacing move is refused, so the two
locked-write checks fail against the old code. The first cut of the lock denied
write sharing, which refused the old writers too and made those checks
vacuous. A replay whose folder cannot be created was refused before as well
(its stream failed to open), so its check guards the same outcome; only the
ignored `create_directories` error is new, and the commit message's claim of a
new refusal was wrong.

Handed to [`HYG#84`](#probe-save-isolation-and-json-fixtures-hyg84):
`settings.cpp`'s hand-made lock and the dead `save_data.json.tmp` removals in
three fixture destructors. `ReplacementLock` is the lock that row names, and it
moves into its `isolated_save.h`.

Both presets build under `/W4 /WX`. `settings`, `player_profiles`,
`match_records`, `remembered_setup`, `input_bindings`, `display_trial`,
`fatal_report`, `debug_console`, `moving_structure`, `fixed_slab` and `session`
passed in Debug and Release, as did the opt-in `turbulence_movers` and
`turbulence_slopes`; `turbulence_slab_routes` and `turbulence_slab_tools`, which
need the private trial, compiled in both. `check-level-editor.ps1` (with its
save-guidance check) and `check-startup.ps1` passed in both presets. On MattPC,
`check-painter.ps1` passed `-ControlsMenuOnly`, `-SettingsMenuOnly`,
`-DisplayTrialOnly`, `-KeyboardJumpOnly`, `-OptionsDefaultsOnly`,
`-PausedOptionsOnly`, `-PercentageTextOnly`, `-RumbleOnly` and
`-StickSettingsOnly` in both presets: the twelve ported save-failure leaves.
After the lock gained write sharing, a scratch program confirmed that under it
a truncating `ofstream` and WIC's `GENERIC_WRITE` stream both rewrite the file
while the replacing move is refused (and that under read-only sharing all three
are refused). Both presets rebuilt; `debug_console`, `display_trial`,
`input_bindings`, `remembered_setup`, `match_records` and `settings` and the
same nine painter switches passed again in Debug and Release.

### Diagnostics and impossible states (`HYG#64`)

**Done 2026-11-05; behaviour change.** The failure dialog names the real record
path, and two impossible menu states throw instead of drawing `ERROR`/`Error`.

- `Level::update` throws `PlayerOutOfBounds`
  ([player_out_of_bounds.h](../game/objects/player_out_of_bounds.h)), a
  `std::runtime_error` with the same message, and `FatalReport::capture`
  records `player_out_of_bounds` by type. A plain `runtime_error` with those
  words no longer gets the reason. The engine's device-failure `strcmp` stays
  at `fatal_report.cpp:117`, where report 30 cites it.
- `FatalReport::dialog_text` takes the path written (empty when nothing was
  saved), and the dialog says `Diagnostic metadata saved to <path>.` instead of
  the fixed `%LOCALAPPDATA%` spelling.
- The spawn diagnostic says a player built outside `out_of_bounds` stops the
  match with a player-out-of-bounds failure; its sibling in
  `station_contract.h` (the editor's words) and its comment say the same. The
  paintable-role error says a `structure_paintable` collision object has no
  paintable role.
- The seven `std::exception("...")` throws (`level_end_info.cpp`,
  `player_figure.cpp`, `player.cpp`, three in `projectile.cpp`,
  `team_colour.cpp`) are `std::logic_error`, or `std::invalid_argument` for
  `TeamColour::team_colour`'s parameter, each naming the enum's value.
- Stage Select's out-of-range slot throws `std::out_of_range` and Pause's unknown
  confirmation `std::logic_error`; neither is reachable (the slot's preview
  already calls `stages->at` first).

`fatal_report` now checks the type, that a look-alike `runtime_error` is not
classified, that a nested one is classified at its own cause, and that the
dialog ends with the given (Unicode) path or the could-not-save line.
`check-failure-reporting.ps1` expects the isolated profile's real path.
`failure-reporting.md` says both.

Both presets build under `/W4 /WX`. `fatal_report`, `level_format`,
`menu_navigation`, `stage_selection`, `session`, `gameplay_tuning`,
`user_stages`, `bot_driver`, `movement`, `paint`, `results_awards` and
`team_palette` passed in Debug and Release, as did `check-failure-reporting.ps1`
(its dialog body names the isolated `last-error.log`) and
`check-level-editor.ps1`.

### Fixed arrays and lifetime hazards (`HYG#65`)

**Done 2026-11-06.** No behaviour change for any input the game can produce;
the editor's text conversion now refuses a string longer than `INT_MAX`
instead of truncating its length.

- `input_slots::per_pad(value)` ([input_slots.h](../game/input/input_slots.h))
  fills an array of `pad_count`. It replaces the four-element literals in
  `PlayerIdentityHud::pad_devices_`, the entry boosts in `game_states.cpp`,
  `SettingsSession::worn_`, and the siblings the grep found:
  `MatchSetup::devices_` and `PlaySettings::pad_rumble`. Review found one more,
  `MatchPlanner::ConnectedPads`, now an array of `pad_count` too. The
  acceptance fakes that stand in for the engine's four-slot reader
  (`rumble_adapter.cpp`) and the frozen legacy menu mapping in
  `action_bindings.cpp` keep their literal four on purpose. `shuffle_teams`' seat
  order is `std::iota` instead of `{ 0, 1, 2, 3 }`.
- `Level::update_level_logic`'s `was_dead` is a `std::vector<bool>` of
  `players_.size()`, not a fixed four.
- `MenuDirectionRepeat`'s held, stick and stick-value arrays are sized by
  `menu_direction_count`.
- `bot_team_trials`' crossing arrivals are filled with -1 to the contact
  list's size instead of nine literals.
- `terrain_role_name`'s table has a `static_assert` that its last entry is
  set, which a missing name would leave null.
- `DisplaySettingsChoice` defaults to the save's own defaults (`s_1920_1080`,
  windowed, random palette).
- `Editor`, `ScreenMusic` and `ResumeAudio` delete their copy constructor and
  copy assignment.
- `bind_commands.cpp` checks `settings_session()`, the pointer it
  dereferences, instead of `save()`. The same mismatch in `tune_target.cpp`'s
  `can_check_levels` and `DebugDirectorImpl::sync_background_audio` is fixed
  the same way.
- `editor/win32_text.h`'s `wide` and `narrow` take their length through a
  checked `win32_length`. `stage_scan_diagnostics.cpp` already range-checks;
  `debug_tokenizer.cpp`'s unchecked copy is the local `narrow` that
  [`HYG#79`](#use-the-engines-text-path-and-environment-helpers-hyg79) replaces.

Left alone: probe expectations written as four-element literals
(`menu_navigation`, `roster_checks`, `settings`) state the expected value
rather than fill a size, so they stay.

Both presets build under `/W4 /WX`. `menu_navigation`, `player_identity`,
`settings`, `stick_settings`, `display_trial`, `terrain`, `input_bindings`,
`action_bindings`, `bot_roster`, `team_selection`, `handicap`,
`player_profiles`, `camera_framing`, `moving_structure`, `story_audio`,
`session` and `debug_console` passed in Debug and Release; the opt-in
`bot_team_trials` compiled in both and passed in Release.
`check-level-editor.ps1` passed in both presets.

### Match-domain rules and settings bounds in one place (`HYG#66`)

**Done 2026-11-06; behaviour change.** A saved deadzone above 90 now loads as
90 instead of falling back to the inherited value.

- [match_rules.h](../game/match/match_rules.h) holds `max_players` (4),
  `min_versus_players` (2), `valid_player_count`, `player_count_range`,
  `layout_for_humans` (which throws outside 1-4), `best_of_choices` and
  `valid_best_of`; `session_rounds(const MatchSetup&)` in `match_setup.h` is
  the solo-is-a-single-match rule. `bot_difficulties`, `valid_bot_difficulty`,
  `bot_difficulty_key` and `bot_difficulty_from_key` sit beside the enum in
  `player_control.h`, and `series_stage_rules` and `valid_series_stage_rule`
  beside theirs in `series_stage_rule.h`.
- They replace the layout copies in `main_menu_players.cpp` (both arrays, and
  `start_join` no longer takes a layout per button), `game_states.cpp`,
  `level_builder.cpp`, `last_match_setup.cpp` and `MatchPlanner::layout_for`,
  which is gone (the three `debug_console` checks that called it now call
  `layout_for_humans`), and the human-count copies in the `team_selection`,
  `bot_runtime`, `cursor_marker`, `instant_replay` and `teammate_markers`
  probes; the difficulty tables in `last_match_setup.cpp`,
  `replay.cpp`, `match_plan.cpp` (completion list, check and conversion),
  `tuning.cpp` (the `bots` entry names and `GameTuning::bot`), `match_setup.cpp`,
  `debug_status_text.cpp` and `Presentation::Validation`; best-of in
  `match_setup.cpp`, `session.cpp`, `last_match_setup.cpp`,
  `MenuText::stage_best_of_mode` and the stage summary validator; the four
  round-count copies (`game_states.cpp` twice, `game_level_dev_tools.cpp`
  twice); the stage-rule lists in the Mode page, `menu_content_validation.cpp`
  and `last_match_setup.cpp`; and the 4 and 2 in `match_setup.cpp`,
  `last_match_setup.cpp`, the three replay validators, `match_plan.cpp`'s
  `players` check, `MenuText`'s pause title and roster line, and the Players
  page's series focus. Messages that named "1-4" now format it.
- [play_settings.h](../game/config/play_settings.h) names
  `StickSettings::inherit`, `max_deadzone` and `deadzone_step`, and
  `PlaySettings::min_aim_sensitivity`, `max_aim_sensitivity`, `max_volume` and
  `volume_step`. The save's play reader, `Save::checked_play_settings`,
  `PlayerProfiles::validate`, the Sticks and Controls pages and the gains read
  them. [play_settings_json.h](../game/save/play_settings_json.h) has the one
  reader and writer for a `StickSettings` and a `SeatComfort`, which the play
  section's `pad_sticks` and `pad_comfort` and each `profiles` entry share; the
  deadzone reader goes through `normalized_deadzone`. `settings` now expects 91
  and 500 to load as 90, and `player_profiles` a saved 95 as 90.

Left as found: the Debug console's `players`, `seat`, `weapon`, `goto`,
`respawn`, `kill` and `bot` parsers, whose help text and completions spell
1-4 with the check; the presentation validators' per-player loops over
four-row geometry tables (`presentation_validation_setup.cpp`, `_results`,
`_pause`), which belong with
[`HYG#70`](#presentation-reader-and-validator-helpers-hyg70); the
Join page's difficulty stepping and the Mode page's two series buttons, which
are page behaviour rather than tables; probes' own difficulty names and the
`balance_runner` option parser, which state expectations or a tool's command
line; and the layouts `bot_roster` and `balance_runner`'s `build` set from the
whole seat count, bots included, which are fixture choices rather than the
one-pane-per-human rule.

Both presets build under `/W4 /WX`. The full default acceptance list (78
cases, `-Parallel 4`) passed in Debug and Release, `check-startup.ps1` passed
in both, and `check-painter.ps1`'s `ControlsMenuOnly`, `StickSettingsOnly`,
`RememberedSetupOnly`, `StageSummaryOnly`, `MenuTextOnly` and
`StickReadoutsOnly` passed in both.

### The level-format contract owns its numbers and names (`HYG#67`)

**Done 2026-11-06; behaviour change.** Stage registration now refuses a
manifest or stage basename longer than 64 characters (it took up to 250), and
the motion contract's refusals say "Motion" rather than "Horizontal motion",
since lifts reach them too.

- [fixed_slab_paint_geometry.h](../game/contracts/fixed_slab_paint_geometry.h)
  names `slab_span`, `slab_rise` and `slab_thickness`, and
  `canonical_slab_paint_polygon` checks against them. `FixedSlabGeometry`'s
  three constants alias them; `fixed_slab_paintable.cpp`,
  `LevelObjectBuilder::stage_context`, the editor's `slab_authoring.cpp`, the
  mirror's slab reflection and `blockout_aids` (its slab edge and paint-depth
  bound) use them. Each value folds to the same float or double as the literal
  it replaces.
- The station, gate and motion contracts share one initialised `Rectangle`,
  the new [rectangle_contract.h](../game/contracts/rectangle_contract.h), and
  `horizontal_motion_contract::Spec` initialises every member. `standing()`
  keeps its own pad, and its comment says why: its scale leaves out
  `path.bottom`, which `corridor_pad()` includes, so using it could widen the
  corridor and refuse a level admitted today. The contract's seven motion
  refusals are worded "Motion ..."; `moving_structure`'s three constructor
  checks look for the travel refusal by its new words.
- [level_format.h](../game/contracts/level_format.h) holds the paintable-face
  names (`face::*`, `paintable_face_names`) and the scenery greys
  (`scenery_grey_names`, `is_scenery_grey`), and the new
  [drawable_text_contract.h](../game/contracts/drawable_text_contract.h) holds
  the drawable range, the layout controls and the punctuation table (its code
  points written as `\u` escapes, as the switch had them, so the dashes and
  the three invisible spaces stay distinct in source). The game's
  `drawable_text` reads that table in place of its switch, and
  `LevelObjectBuilder` reads the face names. The editor's `level_json` reads all
  three tables (its grey-name string, UTF-8 punctuation list and face lists are
  gone) and builds the unknown-material message from the material table;
  `horizontal_authoring`, `slab_authoring`, `blockout_aids` and the mirror use
  `key::*` and `collision_named` for the format's words; the paint-mechanism
  guide and the paint-drive default read `heading_name`; and registration's
  `valid_basename` is `user_stage_contract::usable_name`, its refusal quoting
  `name_rule()`. The game still checks a scenery colour by resolving it, so
  `level_format` now holds `scenery_grey_names` to the engine's colour table.

Left as found: `level_document.cpp`'s own key literals (about 140, most in the
File > New template string and the `AddMember` calls) and the canvas's
`paintable_faces/...` and station `name` property paths. The template string
went when File > New began reading `template.json` in
[`HYG#78`](#editor-shell-and-document-hyg78); the `AddMember` literals and the
property paths moved on to [`HYG#79`](#use-the-engines-text-path-and-environment-helpers-hyg79),
the next editor row. Also left: `level_structures.cpp`'s
"Horizontal motion ..." admission refusals, which name the game's
`HorizontalMotion` type rather than the contract and which the
`moving_structure` probe matches by those words.

Both presets build under `/W4 /WX`, editor included. The full default
acceptance list passed with `-Parallel 4` (78 cases in Release, 79 in Debug);
`level_format` reports the new scenery-grey check. `check-level-editor.ps1`,
`check-stage-registration.ps1` and `check-startup.ps1` passed in Debug and
Release.

### `Player` move-state and contact predicates (`HYG#68`)

**Done 2026-11-06; no behaviour change.**

- [player.h](../game/objects/player.h) has a free `supported(PlayerMoveState)`
  beside the enum, a public `Player::grounded()` in place of the private
  `is_on_ground()`, and private `on_flat_ground()` and `can_act()` (alive with
  health left). `Player` and its figure use them at about twenty sites, and `Level`'s
  idle drip, `LevelBotWorld::body`, the Debug bot driver (its `on_ground`
  switch is gone) and thirteen probe helpers use `grounded()` or `supported()`.
- `end_contacts` and `choose_carrier` share `seen_from` (a contact from the
  player's side) and `top_normal`.
- The velocity chain tests rest once, with `are_equal`, and the stick's
  direction by its exact sign. The old chain's `are_equal(x_input, 0)` could
  only be reached with an exact zero, so the branches are the same.
- The constructor's three sound indices share `sound_identity` (the roster
  participant, else the input slot, at least 0). The redraw cue takes it modulo
  the four-entry table, which is what the old `% 4` did for the keyboard
  slot, and still refuses a participant past the table.

Verified by running the full default acceptance list in Release
before and after on the same machine: all 78 run logs, with their movement
measurements, timing pins and fingerprints, match once temporary paths, dates
and process ids are masked. Both presets build under `/W4 /WX`. The full
Debug list (79 cases, the slow four included) passed, as did
`check-movement-feedback.ps1` in both presets, `check-painter.ps1 -CompileOnly`
in Release (it compiles the three edited painter headers) and
`-RouteWarningOnly` in Debug.

Left as found: `alive && health > 0` written through the public accessors in
`Level` (idle drip, camera follow), `level_structures` and `player_identity_hud`,
because `can_act()` is private; the alive-only checks in `player_figure.cpp`,
which ignore health; `movement_course`'s flat-only probe helper and
`ramp_support_checks`' helper, which leaves out drop-through platforms; and
the single-state `== on_ground` checks, which test exactly that state.

### Player-facing copy out of C++ (`HYG#69`)

**Done 2026-11-06; no change to any English line.** A locale can now reorder
the record lines and replace the Random mark; `qps-ploc` brackets both.

- `menu_text.records` are templates: `new_best` and `stage_best` take
  `{percentage}`, `played` takes `{count}`, and `best_win` and `best_coverage`
  take `{percentage}` and `{played}` (the formatted played line). Startup
  formats each once and refuses a missing or repeated token.
  `MenuRecordText::new_best_line`, `stage_best_line`, `played_line` and
  `best_line` build them for Stage Select's `record_line`, the sibling the grep
  found in Results' `record_text`, and both validators. The Random mark is
  `menu_text.stage.random_mark`, checked at startup for its glyphs in the
  announcement font and for its unscaled title-shadow union fitting
  `preview_size`, so overlong copy fails at load under its own key; the page
  still repeats the containment check. The `menu_text` OptionRow fixture that
  shrank the preview to 2 pixels for the Random arena name now uses a 200-pixel
  preview and a taller name area instead.
- `OptionRow` starts its value empty.
- The Sticks page, its validator and the four probe copies of its layout
  measure the help-font line on `menu_text.help.aim_cutoff` instead of `"Move"`.
  In the shipped `lato_black_24` both measure 38, the line spacing, in English
  and `qps-ploc`, so nothing moves. Only a font whose caption glyphs are taller
  than its line spacing differs: the caption then clears the aim box by its own
  height, so `menu_text`'s 29-high `G` overflow case is gone and its
  track-touching fixture moved from 447 to 455.
- `MenuPage::set_prompts` builds a vector of prompts as given; the Debug menu
  writes `A/Enter` and `B/Esc` itself. Shipped pages already passed copy.
- `input_bindings.cpp` keeps only legends: `input_key_legend` and
  `pad_control_legend` (empty for Space, None and Start) replace
  `input_key_label` and `pad_control_label`, the mouse names lost their labels,
  and `desktop_control_label` is gone (`controls_text::desktop_label` checks the
  control through `desktop_control_name`). The `menu_text` case now checks that
  every word comes from the copy instead of matching English twins.
- [localisation.md](localisation.md#not-yet-localisable) lists `P1`-`P4`, which
  name the pencil-cell badge and stay with it, and the window title, set
  before the locale is chosen.

Left as found: the Debug menu's `"Queued"` measure and its English, which are
developer text; the HUD identity separator and the results `A`/`B` marks,
recorded choices (frozen review). The painter's Stage paper check now expects
literal English record lines rather than gluing the copy itself, and its Results
record check builds its lines through `new_best_line` and `stage_best_line`
(review follow-up: it had appended the number to the template, so its boxes
were measured on a wider string than the game draws).
[menu-presentation.md](menu-presentation.md) describes the Sticks captions
measuring `aim_cutoff`, not `Move`. The follow-up passed the full default
Release list, Debug `menu_text`, `menu_presentation`, `locales`,
`stage_selection`, `settings`, `user_stages`, `stick_settings` and
`debug_console`, and in both presets `check-startup.ps1` (plain and
`qps-ploc`) and `check-painter.ps1 -ResultsMenuOnly` and `-StagePaperOnly`.

Both presets build under `/W4 /WX`. The full default acceptance list (78
cases, `-Parallel 4`) passed in Release; in Debug, `input_bindings`,
`keyboard_jump_capture`, `action_bindings`, `menu_text`, `locales`,
`match_records`, `stage_selection`, `stick_settings`, `series_stage_rule`,
`handicap`, `menu_presentation`, `settings`, `user_stages`, `results_awards`,
`player_identity` and `debug_console` passed. `check-startup.ps1` passed in
both presets, plain and with `-Locale qps-ploc`. `check-painter.ps1`'s
`StickSettingsOnly`, `StickReadoutsOnly`, `ControlsMenuOnly`, `MenuTextOnly`,
`StagePaperOnly`, `ResultsMenuOnly` and `KeyboardJumpOnly` passed in both
presets.

### Presentation reader and validator helpers (`HYG#70`)

**Done 2026-11-06; no layout change, and no accepted content is rejected.**
`pause_menu.practice_row_spacing` (96, what `0.8` times the 120 spacing
gave) is a new optional key: a `presentation.json` without it gets
`0.8 * layout.spacing.y` from the reader, so it loads as before. The first
commit made the key required, which broke the row's criterion; the review
follow-up made it optional. Some diagnostics are worded differently.

- [presentation_read.h](../game/config/presentation/presentation_read.h) has
  `finite_rectangle` (finite edges, positive size) and `overlaps` (touching
  edges allowed). They replace `Validation::finite_stage_rectangle`, the
  lambdas in `check_briefing_card`, `validate_stage_counts` and the Random
  navigation check, `check_status`'s and the opening glass's spelled-out
  overlaps, and eleven spelled-out finite-and-positive checks in the text,
  stage, setup and stick-readout validators.
- The stick readouts measure through `append_label_text`, so their pen walk
  is the labels' one; `Validation::check_settings_row` is the one row check
  Keyboard Jump, Pad Buttons and Profiles share.
- `presentation_read_match.cpp` has `check_plaque` (gate permission, ferry
  direction) and one `read_pen_points`, which takes the respawn canvas's last
  nib position when it has one; `presentation_read_menu.cpp` has
  `read_disabled_ink` for the main and Pause art.
- `format_label_template` takes a `multiline` flag and is the one template
  formatter: the stick readouts, the Random hint, the Display countdown and the
  Pause player title (whose reader and formatter each spelled their token check
  out) use it, and `read_menu_block` shares its blank-line rule.
- Practice Pause reads `pause_menu.practice_row_spacing` in the page and the
  validator; the `menu_text` probe's paper-edge and full-height fixtures use it,
  and one paper-edge fixture sets it to 100, not 96, so a validator spacing the
  rows at `0.8` of the layout spacing fails it. `menu_presentation` checks the
  fallback for a missing key, an exact custom value, and refusals for zero,
  negative, too-large and non-number values. No CPU probe can see the page's
  row spacing: the painter's Pause checks draw plain Pause only.
- The menu reader formats its size limits from `DESIGN_RESOLUTION` and the
  `menu_geometry` constants; the focus-motion bounds are
  `MenuFocusMotionPresentation::max_duration`, `max_arrow_travel` and
  `max_selection_inset_fraction`, which the reader and `MenuFocusMotion` both
  use and format.
- Stage-scan copy errors say `menu_text.stage_scan`; the font, layout and fit
  errors keep `main_menu.stage_scan`. The respawn canvas and figure-slip
  cross-checks name their position in the file. `user_stages` checks a blank
  heading's prefix next to its font, layout and fit refusals, and
  `menu_presentation` checks the canvas, slip-stroke and shared-stroke
  refusals' positions.
- `read_music_mix`, `read_camera`, `read_movement_feedback`,
  `read_combat_feedback` and `read_match` replace the constructor's inline
  reads.
- `SELECTABLE_WEAPON_COUNT` in `weapon_type.h` sizes `WEAPON_NAMES`, the
  Weapon Select choices and the descriptions; `CHOICES` and `DESCRIBED_WEAPONS`
  are gone.
- The HUD comment sits on `HudPresentation`, `menu_geometry.h` says where
  its users are, and `check_status`'s comment no longer counts three pages.
- The roster-row tables and the per-player loops in the setup, Results and
  Pause validators take `max_players`.

Left as found: the Stage Select page's own overlap test
(`main_menu_stage_select.cpp`), which goes with the page helpers in
[`HYG#74`](#menu-page-helpers-hyg74); Mode's `card.intersects` for the
practice hint, the engine's predicate, whose edge rule may differ from
`overlaps`; `validate_stage_counts`' `size_of`, a `Presentation` copy of
`fixed_menu_size`; Keyboard Jump's action labels, which fit their column
without the row checks `check_settings_row` would add; `controls_text`'s
`replace_tokens`, whose optional tokens differ from the formatter's; the slip
nib check in the constructor, which runs once both sections are read; and the
Results ledger's `target <= 3`.

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Debug (79 cases, the slow four included) and in
Release (78), where `menu_text` first failed on its full-height fixture, whose
practice rows had followed its 3-pixel spacing; it passed once the fixture set
`practice_row_spacing` to the same 2.4. `check-startup.ps1` passed in both
presets and with `-Locale qps-ploc` in Release, and `check-painter.ps1`'s
`PauseMenuOnly`, `StickReadoutsOnly`, `MenuTextOnly`, `WeaponCatalogOnly` and
`StageScanOnly` passed in both presets. After the review follow-up, both
presets built again and `menu_presentation`, `menu_text`, `user_stages`,
`figure_slips` and `practice_workshop` passed in both, as did
`check-startup.ps1`; with the validator put back to `0.8` of the spacing (not
committed), `menu_text` failed on the 100-pixel fixture.

### Presentation and content keys that mislead (`HYG#71`)

**Done 2026-11-06; a content-format change, with nothing drawn differently.**
Three keys moved and two Debug `tune` paths changed with them; a
`presentation.json` or `tuning.json` written for the old layout no longer loads,
and a user stage whose objects or `icon_frame` name a deleted `sprite_sheet_1`
frame is refused at load.

- `require_known_keys` now runs on each presentation object with an optional
  member: `camera`, `main_menu`, `menu_text` and `pause_menu` (whose optional
  members are the framing keys, the two `stage_scan` blocks and
  `practice_row_spacing`), both `stage_scan` blocks themselves, and a figure
  slip stroke's edit, where `after` or `remove` chooses the edit. A misspelt
  optional key now stops startup. The root comment says which keys are optional,
  and `content_value.h` says where the check runs.
- `menu_style.box_size` is gone from the file, the reader and `MenuStyle`.
- `main_menu.options_value_colour` is `debug_overlay.menu_value_colour`
  (`DebugOverlayPresentation`), which the Debug menu reads. It is a string
  outside `tune`'s colour allowlist, so it was not a `tune` path before the
  move and is not one now; `debug_console`'s allowlist check keeps it so.
- `hud.player_identity.background_colour` is
  `main_menu.stage_select.preview_background` (`StageSelectPresentation`). Its
  `tune` path, scope (next screen) and every `debug_console` and
  `painter_visual` probe that named it moved with it, including the Debug menu
  walk and the synthetic span documents; the painter evidence text names the new
  path.
- `sprite_sheet_1.json` keeps the twelve frames something names (`pixel`,
  `square_white_4`, the six team-select frames, the three stage icons and
  `ramp_right_200_100`); its nine strips and 29 other frames are gone. The
  `level_editor` probe's "a strip is not a frame" checks use `painter`'s `run`
  strip instead of `player_right_walk`.
- The controls reminder's look is `match.controls_text` in presentation
  (`ControlsTextPresentation`), not tuning: `CountdownCards`, `PracticeLessons`
  (which no longer takes `GameTuning`), `validate_controls_reminders` and
  `validate_stage_text` (neither takes `GameTuning` now) read it, and its font
  is checked with the other presentation fonts. Its `tune` path is
  `presentation.match.controls_text.*`, still rebuilt with the level.
- The `_comment`s on `menu_style`, `results_menu.team_mark` (now inside the
  block) and `tuning.json`'s `paint` describe the block.

Left as found: `_prompt_bar_comment`, which sits beside its block rather than in
it but describes it; and `require_known_keys` on the presentation objects whose
members are all required, where a misspelling already fails as a missing key
(the level files are still unchecked, as `content_value.h` says).

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Release (78 cases). In Debug, 77 passed on the first
run and `debug_console` failed, its Debug menu walk still looking for the
preview backing under `hud`; after that fix `debug_console` and the one case the
failure left unrun, `balance_runner`, passed. `check-startup.ps1` passed in both
presets and with `-Locale qps-ploc` in Release, and `check-level-editor.ps1`
passed in both. `check-painter.ps1`'s `HudHealthTuneColourOnly`,
`HudTrackTuneColourOnly`, `HudTimerTuneColourOnly`, `HudLabelTuneColoursOnly`,
`StagePreviewBackgroundTuneColourOnly`, `StagePaperOnly`, `MenuTextOnly`,
`CountdownOnly` and `PracticeReplayOnly` passed in both presets. Seven of them
first failed with capture or save writes refused under a long evidence path;
they passed rerun with a short `-OutputDirectory`.

After review, `menu_presentation` checks each new `require_known_keys` site
(`camera`, `main_menu`, `menu_text`, `pause_menu`, both `stage_scan` blocks,
both kinds of slip stroke edit, `match.controls_text` and its `shadow_offset`):
a misspelt key is refused with the file and object named, a `_` key still
loads, and a misspelt `remove` is refused. It, `user_stages` and
`debug_console` passed in both presets; with the `camera` check disabled (not
committed), it failed.

### Tuning and content-validation readers (`HYG#72`)

**Done 2026-11-06; a content-format change, with nothing played differently.**
`tuning.json` has a new required key, `player.animations.run_speed` (600, the
literal it replaces), so a file written without it no longer loads. A file with
two faults may name a different one first, because the paint-underfoot checks
now run together after the weapons block is read. Some diagnostics are worded
differently.

- `GameTuning::weapon` and `projectile` name the file they were read from and
  `TUNED_WEAPON_COUNT`, not `./tuning.json` and "five"; the sibling
  `WeaponBuilder` refusal counts the same way. `Rumble`'s profile and event
  refusals format `maximum_rumble_pulse_seconds` and `rumble_event_count`.
- `tuning.h`'s file comment names the Debug `tune` and `reload` commands, and
  "Match phase durations" sits on `MatchTuning`, not the opening, without the
  countdown presentation that `HYG#71` moved out. `tunnelling_limits.h` says
  that paint underfoot multiplies the player's horizontal speeds and that no
  projectile is a mover.
- `tunnelling_limits.cpp` formats through `content_value::number_text` and has
  no roller entry, which could never refuse (zero travel against a positive
  `collision_size`); the `static_assert` comment says why it needs none.
- `content_validation.cpp`'s two stage-name refusals are one
  `shipped_stage_name`, and the supply strip checks the idle origin once,
  outside its loop.
- `stage_scan_diagnostics::alphabet()` builds the printable range from
  `drawable_text::first_drawable` and `last_drawable`, for the layout and the
  stage-scan validator, and `display_text` tests the same constants. The grep's
  two other content checks, `controls_text`'s `replace_tokens` and
  `format_label_template`, use them too.
- `validate_paint_underfoot` holds every rule that compares the block with
  another (the player's horizontal rates, `weapons.ammo_regen_rate`, each
  handicap's refill), in place of `validate_underfoot_rates` and two blocks in
  the constructor; `read_paint_underfoot` keeps the block's own checks.
  `PaintTuning::splash_animation` is gone: `PaintSplashClips::initial` is the
  spray entry of `by_type`.
- `Player::calculate_animation_state` reads `PlayerTuning::run_speed`; its Debug
  `tune` path is `tuning.player.animations.run_speed`, live.
  [gameplay-tuning.md](gameplay-tuning.md#accepted-ranges) lists it with the
  positive fields, and `gameplay_tuning` refuses a zero.
- `locale.cpp` builds the index-less base in one named place,
  `base_without_index`, and the fallback default and `compose` reach the base
  through `Locales::base()`. The tag and name still also appear in
  `locales.json`, which a build without the index cannot read.

Left as found: the printable-ASCII checks on console input
(`debug_text_field.cpp`, `script_commands.cpp`), which bound what a person may
type rather than what content may draw; and docs that say "five weapons" about
the game.

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Release (78 cases) and in Debug (79, the slow four
included); it covers `gameplay_tuning`, `paint`, `paint_underfoot`,
`tunnelling`, `handicap`, `locales`, `user_stages`, `menu_text` and
`debug_console`. `check-startup.ps1` passed in both presets.

### Dead code in the game (`HYG#73`)

**Done 2026-11-06; a content-format change, with nothing played differently.**
`rotation_origin` is gone from the five projectiles in `tuning.json`, from
`read_projectile` and `ProjectileDetails`, so a tuning file that still names it
is now refused as an unknown key. `game/carried/rotation_origin.h` went with it:
its only other user, `Weapon::calculate_sprite_origin`, had no callers, and
[architecture.md](architecture.md#gamecarried) now counts six carried types.
Everything else listed below is deleted as written, with these notes:

- `Weapon` also lost `offset()`, `nozzle_offset()` and `projectile_builder()`,
  `check_if_shooting_and_ammo_update` lost its player centre and velocity, and
  `update_movement_and_rotation` (and `WeaponRoller`'s override) its velocity.
- `relative_impact` lost the motion segment it only read for `global_entry`.
- `build_projectiles` and `build_mist_projectiles` lost rotation, origin, flip
  and layer depth; no caller passed them, so every shot keeps the defaults.
- Dropping `audio_resources` from `StructurePaintable` reached its two
  subclasses and the sibling `FixedSlabPaintable`, which also dropped its unused
  `preferences`. That left `LevelObjectBuilder`'s audio unread, so its
  constructor lost it too, at its four game callers and the probes.
- `level.h` had seventeen stale includes, not ten. Probes that reached
  `labrador::Mouse`, `Monitor` or `Silhouette` through it now include them.
- `game_commands.cpp` also lost `content_root.h`, `text_encoding.h`,
  `directory_consts.h`, `replay.h` and `tune_document.h`, and
  `debug_director_hooks.cpp` lost `application.h`. `main_menu.cpp` lost
  `drawable_text.h`; `pause_menu.h` and `end_menu.h` lost `state_context.h` and
  `colour.h`; `save.h` lost `<iostream>`, `<chrono>`, `<array>`, `vector2i.h`
  and `directory_consts.h`, which `save.cpp` now includes itself. The CMake
  comment on `source_paths.h` names its real users.
- The validate-only calls are written `std::ignore = ...`, the game's usual
  form: the palette check in `save.cpp`, the `(void)` siblings in
  `match_bot.cpp` and `locale.cpp`, and the three in `level_object_builder.cpp`
  (slab geometry, collision type, record gate). The `(void)` in
  `display_awake.h` and `rumble_gamepads.cpp` discard a best-effort platform
  call's result rather than validate, and the probes under `scripts/acceptance/`
  are not the game, so both are left.

Left: `Player`'s constructor still takes the `primary` and `secondary` weapon
types and reads neither. Removing them touches about thirty probe constructions
and was not in the row.

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Release (78 cases) and in Debug (79, the slow four
included; `skirmish_bot` and `balance_runner` passed on a rerun after their
missing `mouse.h`). The opt-in cases, `check-painter.ps1 -CompileOnly` and
`check-paint-fx.ps1 -CompileOnly` compiled in both presets, and
`check-frame-pacing.ps1 -CompileOnly` in Debug. `check-startup.ps1`,
`check-movement-feedback.ps1` and `check-display-awake.ps1` passed in both.

The row as filed:

Remove these, updating builders and probe fixtures:

- `rotation_origin` from `tuning.json`, the reader and
  `game/carried/rotation_origin.h` if nothing else uses it
  (`projectile_details.h:10`);
- `Player::primary_type_`;
- `HorizontalCarryStep::current_top` and `HorizontalPaintImpact::global_entry`;
- the empty arms in `do_jump` and the redundant pitch check
  (`translated_airbrush_shadow.cpp:27`);
- `to_projectile_type`, `projectile_name` and `PaintTile::draw_surface_rigid`;
- `Weapon::nozzle_size` and the unused `Weapon` accessors and update parameters;
- `build_projectiles`' trailing parameters and the duplicate `Projectile::type()`;
- `DebugText::render_resources_`;
- the `audio_resources` constructor parameter (`structure_paintable.cpp:56`);
- the ten stale includes in `level.h`;
- `MenuPage::calculate_center_position` and `float_resolution`, and the unused
  `ProfilesMenu` accessors;
- `hook_frames_`, and `text_wrap`'s `caller` parameter;
- the dead includes in `main_menu.cpp`, `pause_menu.h`, `end_menu.h`,
  `game_commands.cpp` (with `source_paths.h`; fix the CMake comment),
  `debug_director_hooks.cpp` and `save.h`.

Write the validate-only calls one way (`save.cpp:577`), and merge
`sound_groups.cpp`'s two anonymous namespaces.

### Menu-page helpers (`HYG#74`)

**Done 2026-11-06; nothing drawn or played differently.**

- `menu_geometry::fit_scale` (one axis or both; a size or limit that is not
  positive does not constrain) fits the two `OptionRow` labels and values,
  Credits, `SettingsPanelPage::fit_text`, the Controls and Sticks action rows,
  Sticks' association line, the Options status, Stage Select's name, record,
  hint and scan status, and the refusals page's progress line. Controls'
  help reuses `fit_text`.
- `finite_rectangle` and `overlaps` moved from `presentation_read` to
  `menu_geometry`, which `presentation_read` re-exports, and `fits` (finite,
  non-empty and inside) replaced the Join, Team, Weapon and Stage Select paper
  checks; Stage Select's briefing/preview test is `overlaps`.
- `released_action` in `menu_input.cpp` arms and releases both the pad and
  the keyboard actions.
- `menu_cycle.h` has `wrap_step` and `step_tool`: Options' resolution and team
  palette, Mode's duration and series stage, Stage Select, Weapon Select (with
  Random), the practice Tool row (`practice_tool_after`, kept for its probe),
  the refusals file list and a profile name's letters.
- `MenuPage::pop_page` replaces `SettingsPanelPage::close`,
  `UserStageRefusalsMenu::close` and `MainMenuExitConfirmation::cancel`, and
  `PauseMenuPage::debug_dismiss` replaces the two pause pages' copies.
- Controls and Sticks fit their status through `fit_text` and
  `status_limit`, like Keyboard Jump, Pad Buttons and Profiles. The added
  height limit cannot bind, since the validator keeps a status line inside
  the paper.
- The End menu's re-pick is one call; Results has one `fill_width`, both bars
  updated in `update_fill_box`, `continue_pressed` returns a `bool`, and
  `level_end_info()` returns a reference. The title page reads its inputs
  once. The read-only roster copies in `game_states.cpp` (two),
  `player_commands.cpp`, `replay_commands.cpp`, the replay writer, the
  director's seat labels and the status page's two (match lines and paint
  line) are references.

Left as found: the presentation validators' own fit formulas and
`!finite_rectangle(b) || !inside.contains(b)` pairs, where a non-positive
limit gives a non-positive scale that their later checks refuse;
`fit_scale` would give 1 there. `MainMenuOptions::close`, which resets the
focus motion and may return Home instead of popping; and the replay reader's
`claimed` roster, a copy taken before the loop changes the teams.
Where `fit_text` or a fit had no limit check, a non-positive limit now leaves
the text at full size instead of a zero or negative scale; the validators
refuse such content first.

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Release (78 cases) and in Debug (79, the slow four
included); it covers `menu_navigation`, `menu_text`, `menu_presentation`,
`practice_workshop`, `user_stages` and `debug_console`. `check-painter.ps1`'s
`MenuTextOnly`, `PauseMenuOnly`, `ResultsMenuOnly`, `EndMenuOnly`,
`MainMenuOnly`, `JoinPaperOnly`, `TeamPaperOnly`, `WeaponPaperOnly`, `StagePaperOnly`,
`ControlsMenuOnly`, `StickSettingsOnly`, `KeyboardJumpOnly`, `StageScanOnly`,
`ResolutionTextOnly`, `DurationTextOnly` and `PercentageTextOnly` passed in
both presets; in Release `MenuTextOnly` and `ControlsMenuOnly` first failed
with capture and save writes refused under a long evidence path, and passed
rerun with a short `-OutputDirectory`.

The row as filed:

Add `menu_geometry::fit_scale` and `fits`, `wrap_step` and `step_tool`,
`MenuPage::pop_page` and one action-arming helper. Then convert:

- the about seven fit-to-box copies (`option_row.cpp:26`);
- the four paper-bounds checks (`main_menu_weapon_select.cpp:401`);
- the pad and keyboard arming (`menu_input.cpp:145`);
- the wrap idioms (`main_menu_options.cpp:376`) and tool stepping
  (`pause_menu.cpp:155`);
- the three close bodies (`settings_panel_page.cpp:94`) and the two
  `debug_dismiss` bodies (`pause_menu.cpp:130`);
- the hand-rolled status fitting (`controls_settings_menu.cpp:166`);
- `end_menu.cpp:169`, the two results fills (`results_menu.cpp:410`) and its
  unread index (`:454`);
- the `LevelEndInfo` copy (`results_menu.h:23`), `main_menu.cpp:204` and the
  copied roster vectors (`game_states.cpp:204`);
- handed on by `HYG#70`: Stage Select's spelled-out briefing/preview overlap
  (`main_menu_stage_select.cpp:525`), which the presentation validators now
  test with `presentation_read::overlaps`.

### Shared drawing helpers (`HYG#75`)

**Done 2026-11-06; nothing drawn differently.**

- `CrudeDrawing::paper_card` (a static template over any fill, and a
  protected overload through `fill`) draws the countdown, reveal, HUD paper
  and closing-clock cards and, found while converting, a fifth copy: the
  menu paper in `menu_panel.cpp`. `keyed_block` replaces both `draw_block`
  bodies; `WeaponFeedback` and `BlockedShotFeedback` now derive from
  `CrudeDrawing` and keep `draw_block` as one-line wrappers, so their call
  sites did not move.
- `AirbrushShadow::tiles`, `lay` and `first_tile` serve `AirbrushShadow`,
  `FixedSlabShadow` and `TranslatedAirbrushShadow`'s pitch; `PaintTile::wet_band`
  serves the plain and rigid wet bands.
- `game/objects/primitive_frame.h` names the built-in `sprite_sheet_1`/`pixel`
  texel for the badge, teammate marker, gate and ferry cues, viewport
  dividers, the debug console, debug menu and the presentation validator.
  `level_loaded_info.cpp:368` still names `sprite_sheet_1` for a stage icon,
  a different frame, and is left as found.
- `TerrainMaterial::swatch` replaces `FixedSlabPaintable`'s second resolve;
  `StructurePaintable::team_colours()` replaces the copies in the ferry and,
  also found, the team gate. `RevealArt` uses `render_resources()`.
- `ruler_minor_period`/`ruler_major_period` name the ruler's 16 and 80;
  `cell_key` draws from `BotRandom` (the same splitmix64) instead of a second
  copy; `CursorMarkerPresentation::reference_lines` names the `1080`;
  `cosmetic_seed.h` holds the seed mix `QuietCharacter` and `IdleDrip` share.
  The pencil-cap comment describes `at_start`.

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Release (78 cases) and Debug (79); it covers
`weapon_feedback`, `blocked_shot`, `terrain`, `menu_presentation`, `menu_text`
and `debug_console`. 32 `check-painter.ps1` leaves passed in both presets:
`Countdown`, `RevealTuneColours`, `Monitor`, `Hud`, `HudPaperTuneColours`,
`HudClosingMaterials`, `HudClosingShade`, `Closing`, `HudTimerTuneColour`,
`MainMenu`, `SetupPaper`, `JoinPaper`, `TeamPaper`, `WeaponPaper`,
`StagePaper`, `PauseMenu`, `MenuMotion`, `FeedbackTuneColours`,
`BlockedShotTuneColours`, `SurfaceFinish`, `TurbulenceSlab` and
`TurbulenceMover` (with the tracked trial files as `-TrialPath`),
`MiniatureTerrain`, `CqBlockout`, `Crossfeed`, `PressTower`,
`GatePermission`, `FerryDirection`, `Identity`, `Quiet`,
`IdleDripTuneColours` and `CursorTuneColours` (each `...Only`).
`check-startup.ps1` passed in both presets.

The row as filed:

Add `CrudeDrawing::paper_card` and `keyed_block`, the `AirbrushShadow` tile
helpers, named primitive-frame constants (or presentation's sheet and fill), and
a `TerrainMaterial` swatch accessor. Then delete these copies:

- the four paper cards (`countdown_art.cpp:45`) and the two `draw_block`s
  (`weapon_feedback.cpp:59`);
- the wet band (`paint_tile.cpp:159`) and the shadow tiles
  (`fixed_slab_shadow.cpp:13`);
- the literal frame names (`team_gate_structure_paintable.cpp:44`,
  `teammate_marker.cpp:22`) and the second swatch resolve
  (`fixed_slab_paintable.cpp:65`);
- the copied team colours (`translating_structure_paintable.h:105`) and the
  `RenderResources` pointer (`reveal_art.h:19`);
- the ruler periods (`terrain_material.cpp:226`) and the second `splitmix64`
  (`terrain_material_drawings.cpp:33`);
- the `1080` literal (`level_draw.cpp:173`) and the copied seed mix
  (`quiet_character.h:24`).

Fix the pencil-cap comment (`terrain_material_drawings.cpp:341`). Rendered
pixels must not change, so the painter leaves that draw these must pass
unchanged on MattPC.

### Weapon, projectile and opening logic (`HYG#76`)

**Done 2026-11-06.** One behaviour change, for a build fault only:
`TerrainMaterial::make` now throws for a material the level format's table
names but no look draws, where it used to draw a paint tin. Names outside the
table were already refused.

- `Weapon::play_one_shot` is the Bucket and Sniper overrides' one body.
  `handle_shoot_sound` now takes `(fired_this_update, shooting_this_update)`,
  the order its caller passes them in.
- `game/objects/aim_facing.h`'s `aim_faces_left` replaces `Weapon::facing_left`
  and the idle drip's copy; the roller and the base weapon set their flip from
  it in one line each. The idle drip's disabled branch calls `clear()`.
  Found in review, the bot's predicted nozzle (`match_bot_territory.h`'s
  `top_launch_lands`) and the figure's hands (`figure_attachments.h`) call it
  too, so neither can disagree with the weapon's flip. The two probes that
  restate the test as an oracle (`bot_territory.cpp`,
  `idle_drip/level_checks.h`) are left for the probe rows.
- `paint_owners::slot_or_fallback` serves `PaintAudio` and, also found,
  `BlockedShotFeedback`, whose `owner_index` is gone. `Projectile::team()` is
  now public and replaces the layer-to-team mapping in `StructurePaintable`
  and, also found, `FixedSlabPaintable`; the layer guard stays.
- `HorizontalMotionSpec::pose_at` and `axis_part` replace
  `HorizontalMotion::at_offset`, both local `axis_part`s, the pose in
  `level_structures.cpp` (which still checks the box) and the replay
  reader's copy.
- `opening_times` and `reveal_times` in `match_opening.cpp` give the beat, the
  sounds, the click age and both lengths one set of edges, each the last plus
  its span. The sound edges used to add `preview * 2` and `hover * 2`; at the
  shipped tuning every edge is bit-identical either way. The figures' `alone`
  test (`reveal_length() - react`) is left as found: for a draw it is not the
  same edge.
- `level_format::material_name` names the six materials for the table, the
  fixed-slab rule, `TerrainMaterial::make`, the drawings' constructors and,
  also found, the editor's new-object defaults. Refusals spell the tag with
  `level_format::collision_name`, so a fixed slab now says
  `STRUCTURE_FIXED_SLAB` instead of "this collision type".

Left as found: the probes that restate the reveal timeline by hand
(`story_checks.h`, `reveal_checks.h`, `monitor_checks.h`,
`result_percentage_checks.h`, `level_checks.cpp`), for the probe rows; and
`level_document.cpp`'s File > New template, which still spelt `"ground"`;
[`HYG#78`](#editor-shell-and-document-hyg78) replaced it with `template.json`.

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Release (78 cases) and Debug (79); it covers
`weapon_feedback`, `blocked_shot`, `idle_drip`, `story_audio`, `terrain`,
`moving_structure` and `instant_replay`. `check-level-editor.ps1` and 11
`check-painter.ps1` leaves passed in both presets: `Monitor`, `Countdown`,
`Closing`, `RevealTuneColours`, `SurfaceFinish`, `MiniatureTerrain`,
`IdleDripTuneColours`, `FerryDirection`, `WeaponCatalog`, `Crossfeed` and
`BlockedShotTuneColours` (each `...Only`). The review follow-up rebuilt both
presets and passed `bot_territory`, `bot_ballistics`, `skirmish_bot`,
`idle_drip`, `weapon_feedback`, `figure_slips` and the painter's
`WeaponCatalogOnly` in each.

The row as filed:

- Add `Weapon::play_one_shot` for the identical Bucket and Sniper overrides
  (`weapon_bucket.cpp:27`, `weapon_sniper.cpp:28`), and fix the inverted
  parameter name (`weapon.cpp:357`).
- Add one `aim_faces_left` for the roller (`weapon_roller.cpp:73`) and the idle
  drip (`idle_drip.cpp:107`), and call `clear()` in the idle drip (`:72`).
- Add `paint_owners::slot_or_fallback` (`paint_audio.cpp:38`), and use
  `Projectile::team()` instead of the copied mapping
  (`structure_paintable.cpp:150`).
- Add `HorizontalMotionSpec::pose_at` and `axis_part` (`level_structures.cpp:47`),
  and the opening time helpers (`match_opening.cpp:429`).
- Make `TerrainMaterial` use the `level_format` names (`terrain_material.cpp:40`)
  and throw on an unknown material (`:123`) instead of drawing a paint tin. That
  is a behaviour change for malformed content only; say so in the commit.

### Bot geometry, replay and Debug bots (`HYG#77`)

**Done 2026-11-06; behaviour change.** Two changes, both small:
`SECONDS_PER_STEP` is now the step the game actually takes, so the shipping
bots' jump model steps at 0.0166666 s instead of 0.016666668 s; and a Debug
replay whose seat names a team other than `a`, `b` or `none` is refused instead
of read as `none`.

- [simulation.h](../game/contracts/simulation.h) derives `SECONDS_PER_STEP` the
  way the engine's `StepTimer` does (10,000,000 ticks a second, 1/60 truncated
  to 166666 ticks), and `main.cpp` `static_assert`s that it equals
  `StepTimer`'s step. The three spellings are now one: `match_bot_ballistics`'s
  two `StepTimer` expressions (already this value) and the Debug `wait`'s
  `1.0f / 60.0f` fallback read it. `MatchBotLocomotion::jump`, the
  anti-tunnelling check's default, the motion contract's two-step period floor,
  `HorizontalMotion::validate_dt` and `LevelStructures`' budget move from 1/60
  to it; the shipped content passes each at either value.
- `MatchBot::crosses_interior` is the navigation and ramp code's one sweep, and
  `MatchBotLocomotion::held_jump_steps` the one hold count that `decide` and
  `steer_ascent`'s takeoff set and `jump()` models. The death branch calls `replan_traversal`, then clears
  what a replan keeps. Bounds compare with `RectangleF::operator==` in the ramp
  code and, also found, in `clear_route` and `same_support`. `height_across`
  is gone.
- [replay.cpp](../game/debug/replay/replay.cpp) writes and reads a seat's team
  through one `team_words` table and refuses an unknown word, naming the seat.
  The finished player fills seats with `Bot::neutral()`. `match_plan.cpp`'s two
  comments now say what the code does. `fnv1a` and `hex64` moved to
  [game/debug/replay/hash.h](../game/debug/replay/hash.h), and `replay.cpp`'s
  identical `motion_hex64` is gone.
- `Bot::observes_tool` (true for `SkirmishBot`) replaces `BotDriver`'s name
  comparison. `BotSeatView::step` is gone, with `view_of`'s `step` parameter and
  `BotDriver::step_`. The Debug bots' numbers are named in `bot.cpp`; values and
  RNG order are unchanged.

Probes: `debug_console`'s codec check refuses a team `c`, `skirmish_bot` checks
that only the skirmish bot observes the tool, and the probes that set
`view.step` no longer do. The probes that step a match at `1.0f / 60.0f`, the
other value, stay pinned to it: `debug_console` (most of its checks),
`display_awake_lifecycle`, `instant_replay`, `keyboard_jump_capture`,
`quiet_character`, `resume_countdown`, `story_audio`, `terrain` and
`painter_visual`'s result-percentage fixture. None of them runs a mover, the
only code that refuses a longer step (`validate_dt`). About forty more spell
the shipping step through `StepTimer`; moving them to `SECONDS_PER_STEP` is
probe tidying for [`HYG#87`](#probe-tidy-ups-hyg87). Left as found:
`level_loaded_info.cpp`'s `StageSemanticHash`, shipping code with its own FNV
framing, and the probes' private FNV loops (`practice_replay_checks.h`,
`turbulence_slab_routes`).

Both presets build under `/W4 /WX`. The full default acceptance list
(`-Parallel 4`) passed in Debug (79 cases) and Release (78). After a last,
value-identical edit, the eight bot cases, `skirmish_bot`, `crossfeed`,
`press_tower`, `cq_blockout`, `paint_ferry` and `debug_console` passed again in
Release. The twelve `Quick` opt-in cases (the bot route and trial probes and
`turbulence_movers` among them) passed in both presets, and every opt-in case
compiled in both (`-OptIn -CompileOnly`). `check-painter.ps1 -CompileOnly` and
the `PracticeReplayOnly`, `BotPlayersOnly` and `CrossfeedOnly` leaves passed in
both presets, and `check-startup.ps1` passed on both executables.

The row as filed:

Bot geometry:

- Share `crosses_interior` (`match_bot_ramp.cpp:23`) and one held-jump step
  count (`match_bot.cpp:367`).
- Have the death branch call `replan_traversal` (`:415`).
- Use `RectangleF` equality (`match_bot_ramp.cpp:11`).
- Define one step length in `simulation.h:9` (two values exist today). Keep the
  value the shipping step uses, and record any probe that pins the other.
- Delete `height_across`.

Replay and the Debug bots:

- Refuse an unknown team word (`replay.cpp:585`).
- Fix the format-4 and "as before the table" comments (`match_plan.cpp:357`,
  `:98`).
- Move FNV-1a and `hex64` from `tune_document.h:87` to a hash header.
- Use `Bot::neutral()` (`replay.cpp:1141`), and add `Bot::observes_tool` instead
  of comparing names (`bot_driver.cpp:110`).
- Name the bot constants (`bot.cpp:129`), and drop `BotSeatView::step`.

### Editor shell and document (`HYG#78`)

**Done 2026-11-06; behaviour change.** File > New opens a different level, a
combo selection no longer applies pending property text, a spawn or station
box follows source tuning's player size, and a failed registration says
"Registration failed:" once.

- [editor/CMakeLists.txt](../editor/CMakeLists.txt) compiles
  [template.json](../editor/template.json) into a generated
  `new_level_template.h` (editing it reconfigures; the build refuses a template
  the raw string cannot hold), and `LevelDocument::create` parses it, dropping
  only its `_comment`. New therefore has the template's two walls, ceiling,
  `centre` station, spawns at y 1075 and `"include_in_random": false`, where
  the inline string had a floor alone, spawns at y 1000 and no Random setting.
  `level_editor`'s resource check now requires New to equal the template.
- [editor_window.cpp](../editor/editor_window.cpp) has one `{label, type}`
  palette table and one `{label, size}` snap table; the snap list selects the
  entry equal to `snap_`. `WM_COMMAND` reaches `command()` only for a menu or
  accelerator (no control) or `BN_CLICKED`; before, `HIWORD` 1 also matched
  `CBN_SELCHANGE`, so choosing a palette entry applied pending text. The DPI
  probe's empty-key check sends a palette `CBN_SELCHANGE` with text pending
  and requires it still pending.
- `labelled` is the three add/duplicate/paste loops' and the mirror's one way
  to return the added entry with its label; `same_entry` moved beside `Entry`.
  `LevelDocument::Impl::player_size` (tuning's size, or 52 by 120 without
  tuning) replaces the mirror's and the symmetry report's two copies of that
  fallback, and still throws on unreadable tuning there. A spawn or station
  rectangle reads `marker_size`, which never throws: it gives 52 by 120 when
  tuning is unreadable and parses tuning again only when the file's write time
  changes, so the editor's refresh and paint neither fail on a bad tuning file
  nor reread it per entry. `level_editor`'s station checks load New with a
  70 by 140 player and with an unparseable tuning file and require those
  sizes and the fallback. `bounds_labels` is held to `bounds_keys`' length.
- `apply_stage_registration`'s two rollback errors start with the cause;
  `ColourWarsRegisterStage` adds the prefix. `level_editor`'s registration
  checks refuse a message that repeats it.
- The shell sources build once, as `colourwars_editor_shell`, which the editor
  and `colourwars_editor_dpi_acceptance` link.

Probes that took New's old layout as a blank: the Random-setting check writes
New without `include_in_random`, the station palette check deletes `centre`
first, the mover float-binade check finds its object by position instead of
index 1, the slab seam check erases the walls and ceiling (a new
`erase_template_walls` fixture), and the DPI metadata fixture names
`left_wall`, New's first object, instead of `floor`.

Handed on: `level_document.cpp`'s `AddMember` key literals for new palette
objects and the canvas's `paintable_faces/...` and station `name` property
paths, which the `HYG#67` note handed here with File > New. They are not in
this row's sites, so they go to
[`HYG#79`](#use-the-engines-text-path-and-environment-helpers-hyg79), the next
row in the editor's sources. The inline template's literals, and the `"ground"`
the `HYG#76` note handed here, went with it.

Both presets build under `/W4 /WX`. `check-level-editor.ps1` (document,
DPI, gesture, pending and hidden GUI checks) and `check-stage-registration.ps1`
passed in Debug and Release, as did the acceptance cases that compile the
editor document: `level_format`, `moving_structure`, `fixed_slab` and
`turbulence_blockout`, and the `Quick` opt-in `turbulence_movers`,
`turbulence_slopes` and `turbulence_slab_stage`. The review follow-up
(`marker_size`) rebuilt both presets and passed `check-level-editor.ps1` and
`level_format`, `moving_structure`, `fixed_slab` and `turbulence_blockout` in
each.

The row as filed:

- Make File > New read `template.json` (`level_document.cpp:285`).
- Use one `{label, type}` palette table (`editor_window.cpp:189`) and one snap
  table (`:719`).
- Dispatch `WM_COMMAND` only for menus and `BN_CLICKED` (`:724`), so a combo
  selection does not apply pending text.
- Share the added-entry label loop (`level_document.cpp:964`).
- Read the player size from tuning (`:482`).
- Print "Registration failed:" once (`stage_registration.cpp:820`).
- Build the shell sources once as a library that the DPI probe links
  (`editor/CMakeLists.txt:55`).

### Use the engine's text, path and environment helpers (`HYG#79`)

**Done 2026-11-06; behaviour change, for non-ANSI paths only.** Two editor
messages, a bad aid-data file and an unreadable motion `tuning.json`, used
`path.string()`, which throws a conversion error on a name the ANSI code page
cannot spell; they now name the path in UTF-8. Everything else reads and
writes as before.

- `labrador::narrow` replaces `debug_tokenizer.cpp`'s `WideCharToMultiByte`
  copy (both turn a lone surrogate into U+FFFD), `replay_commands.cpp`'s
  `narrow_build_label`, its copy building the console banner in
  `debug_director_impl.cpp` (found in review), and a sibling in
  `text_wrap.cpp`; the last three wrote `?` for a non-ASCII unit, and all
  read ASCII (the build label, drawable text). `labrador::widen` replaces `ProfilesMenu`'s byte copy; profile names
  are A-Z, 0-9 and spaces.
- `labrador::path_to_utf8` replaces the screenshot line's `u8string` copy in
  `debug_director_hooks.cpp` and its two generic-form siblings, `reload`'s
  `utf8` (`game_commands.cpp`) and tune's `shown`, which pass
  `generic_wstring()` so they keep their forward slashes.
- [editor/path_text.h](../editor/path_text.h) holds `path_text` (UTF-8 for
  messages) and `utf8_path` (its inverse). It replaces the `shown` copies in
  `user_stage_save.cpp`, `register_stage_main.cpp`, `stage_registration.cpp`
  and `prepared_file.cpp`, `LevelDocument::load`'s inline copy,
  `save_guidance.cpp`'s `stage_name`, the `utf8_path` in
  `stage_registration.cpp`, the conversions in `content_catalog.cpp` and
  `save_guidance.cpp`'s `widen`, the two `path.string()` messages and
  `editor_window.cpp`'s `narrow(folder.wstring())`. The editor has no engine,
  so it keeps its own header.
- `DebugArgs::parse_double` replaces tune's copy; it and `parse_float` share
  one template.
- [game/config/environment.h](../game/config/environment.h)'s
  `environment_variable()` replaces the two `_dupenv_s` readers
  (`COLOURWARS_MUTE`, `COLOURWARS_LOCALE`).
- `level_document.cpp` spells every level-format key with `level_format::key::*`
  (114 literals, the new palette objects' `AddMember` keys among them) and its
  face loops with `paintable_face_names` and `face::bottom`; the canvas's
  station `name` and `paintable_faces/...` paths use `key::` and `face::`.

Left as they are: the two wide `LOCALAPPDATA` readers
(`directory_consts.cpp`, `user_stage_save.cpp`), which return paths, not text;
`user_stage_save.cpp`'s `stem_of`, whose `?` for a non-ASCII unit is how such a
name fails the name rule; a motion record's `"type"`, and the coordinate and
small fields `level_format.h` says are written where they are read; and
`stage_registration.cpp`'s `"name"` and `"display_name"` literals, which are
outside the document and canvas. Tune's unchecked `parse_double` call stays
with [`HYG#80`](#debug-console-commands-hyg80).

Both presets build under `/W4 /WX`. `debug_console`, `locales`, `text_wrap`,
`menu_text`, `player_profiles`, `menu_navigation`, `settings`, `level_format`,
`moving_structure`, `fixed_slab` and `turbulence_blockout` passed in Debug and
Release, as did `check-level-editor.ps1`, `check-stage-registration.ps1` and
`check-startup.ps1` (plain and `-Locale qps-ploc`).

The row as filed:

- `labrador::narrow` for the local `narrow` (`debug_tokenizer.cpp:25`) and
  `narrow_build_label` (`replay_commands.cpp:37`);
- `labrador::widen` for `ProfilesMenu`'s byte copies (`profiles_menu.cpp:10`);
- `labrador::path_to_utf8` for the three path conversions in
  `debug_director_hooks.cpp:499`;
- one editor `path_text.h` for the five editor copies (`user_stage_save.cpp:19`),
  and no `path.string()` in messages (`blockout_aids.cpp:22`);
- a `DebugArgs::parse_double` for tune's copy;
- an `environment_variable()` helper for the two `_dupenv_s` readers
  (`sound_groups.cpp:149`).

Handed here by [`HYG#78`](#editor-shell-and-document-hyg78) (first from
`HYG#67`), as the next row in the editor's sources: spell `level_document.cpp`'s
`AddMember` key literals for new palette objects with `level_format`'s `key::*`
names, and the canvas's `paintable_faces/...` and station `name` property paths
likewise.

### Debug console commands (`HYG#80`)

Fix the messages and help text:

- `screenshot` names a file it does not write (`game_commands.cpp:334`).
- The F-key help is fixed text that goes stale after `bind`
  (`debug_registry.cpp:422`).
- The `binds.cfg` warning omits `binds.reset` (`bind_commands.cpp:77`), and the
  inverted `moved` variable hides a silent failure (`:59`).
- The doc-comment examples and counts have drifted (`debug_director_impl.h:115`,
  `tune_scope.h:19`).

Share the duplicated helpers:

- the arity check (`debug_registry.cpp:294`) and the queued predicate
  (`debug_director_hooks.cpp:423`);
- the behaviour list (`player_commands.cpp:371`) and `respawn`/`kill` (`:68`);
- the severity prefix (`debug_log.cpp:24`), `upper()` (`bind_commands.cpp:31`) and
  `format_number` (`game_commands.cpp:28`);
- the overlay names (`view_commands.cpp:71`) and `OverlayHandles::resolve`
  (`debug_console_view.cpp:77`);
- the restart-flag check (`debug_menu_model.cpp:101`);
- one way to register a command with menu traits (`bind_commands.cpp:23`).

Also:

- Make `reload` validate the text it applies (`game_commands.cpp:418`).
- Move `DebugStatus` to its header (`debug_variable.cpp:9`).
- Use `DebugPerf::max_capture_frames` (`script_commands.cpp:413`).
- Name the tune file indices (`tune_commands.cpp:23`), and check `parse_double`'s
  result (`:396`).

### The `SCREAMING_SNAKE` constant rule (`HYG#81`)

AGENTS.md reserves `SCREAMING_SNAKE` for macros (`AGENTS.md:403`). About 59
constants use it anyway, for example `SIMULATION_HZ`, `DESIGN_RESOLUTION`,
`WEAPON_NAMES` and the `directory_consts.h` strings. The author decides first:
amend the rule to admit constants, or rename. If renaming, do file-local
constants first, then one header per commit, and make `directory_consts.h`'s
per-translation-unit `const static std::string`s `inline constexpr` character
arrays. That last change is worth doing whichever way the rule goes.

### Names that mislead (`HYG#82`)

Mechanical renames, one per commit:

- the key-domain helpers' "Jump" (`input_bindings.cpp:285`) and
  `KeyboardJumpMenu`/`KeyboardJumpCapture` (the save key `keyboard_jump` stays);
- the settings panels' `player` parameter to `opening_slot`
  (`profiles_menu.h:15`);
- `match_owns_pad_slot` to `match_owns_input_slot` (`pause_menu.h:19`);
- the `_1_player` members (`main_menu.h:322`);
- `duration_font_` (`main_menu_mode_select.cpp:128`);
- `SettingsPanelPage::text_` (`settings_panel_page.h:64`);
- `Validation::presentation_` (`presentation_validation.h:67`);
- the editor's `horizontal_authoring`, now that it covers lifts.

In the same row, move `MenuInputAction` from `states/` to
`game/input/menu_input_action.h` (`action_bindings.h:7`), and drop the exception
from docs/architecture.md.

### Probe stages and assets from the manifest (`HYG#83`)

In `support/stage_catalog.h`, add `manifest_level_paths` and `shipped_stage`. In
a new `support/content_world.h`, add `load_cpu_assets(content, render, audio)`,
which reads `manifest.json`. Then replace these copies:

- the hand-picked stage lists behind "every" claims (`story_audio.cpp:163`,
  `level_editor/content_checks.cpp:106`, `countdown_checks.h:423`);
- the stale "four shipped stages" guard (`terrain.cpp:77` and tunnelling);
- the private manifest readers (`locales.cpp:75`);
- the repeated lists (`miniature_terrain_checks.h:441`, `instant_replay.cpp:25`,
  `user_stages.cpp:149`, `stage_selection.cpp:58`);
- the 28 hard-coded `movement_course` descriptors (`bot_setup_checks.h:226`) and
  the hard-coded music cue (`music_flow_checks.h:469`);
- the about twenty Worlds that copy the manifest's sheets and fonts
  (`aim_marker.cpp:48`, `weapon_catalog.cpp:57`, `tune_menu_checks.cpp:74`);
- the identical `menu_text` and `menu_presentation` Worlds
  (`menu_presentation.cpp:65`), and `moving_structure`'s copy of the
  paint-surface helpers.

Exact-count pins stay with `stage_selection`. A check whose coverage grows may
find new failures; fix them in the same row, or record them as their own rows.

### Probe save isolation and JSON fixtures (`HYG#84`)

Add `support/isolated_save.h`. It holds a non-copyable GUID-rooted
`LOCALAPPDATA` override with `remove_all`, `save_path()`, `save_bytes()` and a RAII
`ReplacementLock`. Replace the about twenty copies (`display_trial.cpp:26`,
`settings.cpp:20`, `remembered_setup.cpp:40`, `menu_motion_checks.h:7`,
`controls_menu_checks.h:7` and others) and the 64 spelled-out save paths. Add
`support/json_fixture.h`, with `read_document`, `write_document` and
`rejects_variant` using checked I/O and one scratch policy. Port the edit-a-copy
fixtures and read/write helpers (`combat_feedback.cpp:342`, `view_shake.cpp:553`,
`series_stage_rule.cpp:361`, `story_audio.cpp:59`, `locales.cpp:59`,
`practice_workshop.cpp:337`, `menu_presentation.cpp:233`).

From `HYG#63`: `acceptance::ReplacementLock` and `temporary_beside` already
exist in `support/replacement_lock.h`; fold them into `isolated_save.h`. Port
`settings.cpp`'s hand-made `locked` handle to it, and drop the
`save_data.json.tmp` removals in the `settings`, `match_records` and
`player_profiles` destructors, which no writer creates any more.

### Level-driving and route fixtures (`HYG#85`)

After `HYG#83`. Move `one_bot_roster`, `owned_top_paint`,
`check_final_input_replay` and the trial rosters into
`bot_navigation/fixtures.h`. Delete the copies in `bot_trials.cpp:137`,
`bot_arena_routes.cpp:89` and `:119`, `team_gate.cpp:411` and the other bot
cases. Add `support/player_course.h` for the six `Course` fixtures that restate
`Level`'s tick order (`tool_motion.cpp:281`). Add `support/route_run.h` for the
Map, Services and Run fixture copied across `crossfeed`, `cq_blockout` and
`press_tower` (`crossfeed/fixtures.cpp:53`). Collapse crossfeed's five
steering copies (`opposed_loop_checks.cpp:36`), the seat-to-team expression
(`bot_team_trials.cpp:316`), the region classifier
(`turbulence_blockout/tool_checks.cpp:15`) and `movement_feedback_visual.cpp:729`.
The trial fingerprints and counts must match before and after.

### `painter_visual` fixtures (`HYG#86`)

Add the following to `painter_fixtures.h`:

- `TuneColourSession`, the about-90-line scaffold copied into about twenty
  `*_tune_colour` headers (`blocked_shot_tune_colour_checks.h:275`);
- `add_probe_font`, the nine shifted-font copies
  (`competitive_winner_checks.h:272`);
- `CanvasProbeRoot` and `stack_frame` (`end_menu_checks.h:3`,
  `main_menu_checks.h:3`);
- the opening and HUD style fingerprints, with explicit exclusions, so a new
  field is not silently dropped (`acting_tune_colour_checks.h:4`,
  `power_light_tune_colour_checks.h:5`);
- `pixel_bounds` and `srgb_luminance` (`countdown_checks.h:19`);
- `controlled_spray` (`action_combat_checks.h:136`).

Also:

- Replace the `const_cast` seeding (`controls_menu_checks.h:31`).
- Share the Options reference renderers (`options_defaults_checks.h:152`) and
  the help copy (`controls_help_checks.h:8`).
- Make `StageScanStderrCapture` safe (`stage_scan_checks.h:12`).

Port leaves in batches. Each batch runs its leaves on MattPC with identical
pins.

### Probe tidy-ups (`HYG#87`)

- Delete the `srand` calls that have seeded nothing since `HYG#39`
  (`press_tower_checks.h:49`, `aim_marker.cpp:320`, `player_identity.cpp:328`,
  `level_checks.cpp:207`, `home_loop_checks.cpp:82`, `paint_surfaces/fixtures.h:94`).
- Remove the dead probe code: the unused `Mouse` members, the SettingsSession
  locals (`remembered_setup.cpp:214`), the `Totals` fields (`team_gate.cpp:439`),
  the no-op slot claim (`instant_replay.cpp:54`), the silenced parameters, the
  unreachable `#else` throws (`painter_visual.cpp:812`), the unused includes,
  crossfeed's `swept_surface` and the unbuilt
  `turbulence_slab_stage/tools/register.cpp`.
- Tidy the `debug_console` case:
  - one `{path, accessor}` colour table with named indices
    (`fixtures.h:411`, `tune_colour_sweep_checks.h:126`);
  - shared reserved-name, tick and `command()` helpers;
  - the `DebugPerf` and `ReplayFile` constants;
  - checked lookups;
  - one declaration index, and the stale comments.
- Share the `level_editor` refusal, lookup and file helpers. Return a value from
  `motion_of` (`drive_checks.cpp:33`). Require the clipboard restore and
  `DeleteFileW`.
- Fix the idioms: RAII `captured_stderr` (`match_records.cpp:389`), one
  `story_audio` include convention, internal linkage for file-local helpers, the
  BOM in `team_palette.cpp` and the fixture folders `fatal_report` leaves.
- Fix the fused words (`exceeded500`), the `#`-less row ids (`ROAD34`, `POL36`,
  `CI1`) and the stale "draft" headers. `HYG#61` already wrote `POL#36` in
  `moving_structure/replay_checks.cpp`.
- Reword the PASS summaries that still say attended or human acceptance
  "remains open" (from `HYG#61`): `cq_blockout.cpp`, `crossfeed.cpp`,
  `crossfeed/pressure_checks.cpp`, `press_tower.cpp`, `fixed_slab.cpp`,
  `turbulence_blockout.cpp`, `turbulence_slopes.cpp`,
  `balance_runner/self_checks.cpp` and the painter's `cq_blockout_checks.h`
  and `crossfeed_checks.h`.

### Painter verifier evidence helpers (`HYG#88`)

Add `scripts/check-painter/evidence.ps1` with `Assert-PainterPpm`,
`ConvertTo-PainterFinite`, `Get-PainterSaveCounts`, key-set and CSV asserts and
one PPM-to-PNG conversion. Port the about forty verifiers
(`verify-hud-closing-shade.ps1:4`, `verify-competitive-winner.ps1:119`,
`verify-desktop-tune-colour.ps1:6`). That restores the payload-length check
`verify-feedback-tune-colours.ps1:30` lost, and settles one case rule.

Also:

- Drop the hand-added totals (`verify-desktop-tune-colour.ps1:66`).
- Derive the selector token instead of the 100-branch chain
  (`check-painter.ps1:194`).
- Rename `$profile` (`:307`).
- Check the git exit codes (`evidence-record.ps1:13`).
- Treat `check-path.ps1` as required (`check-painter-selection.ps1:265`).
- Fix the contact sheet's rumble filter (`contact-sheet.ps1:145`).

Run `check-painter-selection.ps1` and a full painter sweep on MattPC.

### Shared PowerShell probe-pipeline and art helpers (`HYG#89`)

Add dot-sourced helpers:

- `probe-build.ps1`, for the CMakeCache, compile-flag and library pipeline that
  six scripts copy (`check-movement-feedback.ps1:33`; it has drifted on the
  rapidjson root and Xinput);
- `check-diagnostics.ps1`, for the stderr and optional-bank scans
  (`check-startup.ps1:257`);
- one window enumerator (`:77`);
- one art PNG writer (`build-painter-art.ps1:17`, `build-terrain-art.ps1:17`).

Also:

- Require Windows PowerShell in the `build-*-art` scripts
  (`drawing-references.ps1:4`).
- Use the CI query in `build.ps1:43`, and `vswhere` in
  `capture-study-b.ps1:78`.
- Fix the messages: `check-display-awake.ps1:80`,
  `check-failure-reporting.ps1:307`, `check-debug-console.ps1:51` and
  `driver.ps1:1`.
- Correct `scripts/README.md:6`'s OutputDirectory claim.

Run each touched `check-*` script in both presets.

### Art build scripts and art docs (`HYG#90`)

- Read the team colours from `presentation.json` in the painter lineup
  (`build-painter-art.ps1:516`) and the icon (`icon.json:14`).
- Name `slips.png` in the status line (`:593`), and fix the row-check comment
  (`:176`).
- Stop emitting the unread `idle` frame (`:431`), and move the locomotion timing
  into `painter.json` (`:419`).
- Make `build_icon.py --check` decode-based, or drop its CI claim
  (`build_icon.py:6`).
- Correct the art READMEs: the hue count (`painter/README.md:145`), the pen
  table (`:107`), the owned directories (`game/art/README.md:276`), the source
  rows (`:262`) and the terrain props (`:244`).

Regenerated art must be byte-identical, or the commit says why not.

### Python audio and study tools (`HYG#91`)

- Add one shared `write_wave` (`foley/build.py:104`; the three writers clip
  differently).
- Name the ADPCM geometry and the peak ceiling once in `audio_pipeline`
  (`audio_pipeline.py:235`, `prepare_review.py:228`).
- Fix the overwritten test fixture (`test_audio_pipeline.py:166`).
- Make `--only` refuse unknown ids (`music/render.py:342`).
- Report the publisher's failures on stderr (`publish_runtime_bank.py:69`).
- Share the capture limit (`analyze_frame_pacing.py:71`).
- Normalise Study B's tie-break (`study-b/analyse.py:111`).
- Fix the pose error (`make-references.py:146`).
- Add a `scripts/studies/_paths.py` for the four copies of the privacy guard.

The tracked bank must be unchanged.

### Debug `.cfg` scripts (`HYG#92`)

- Add the two stations `station_tour.cfg` skips. It claims every station.
- Extract `station_tour_begin` and `station_tour_end`.
- Collapse the seven `review_turbulence_*` copies into one family script with
  wrappers.
- Factor the movement and tools profiles the `jump_*` and `crossfeed_*`
  scripts restate.
- Fix `review_finish_details.cfg:38`'s scope comment.

The shipped-script lint (`debug_console`) must still pass.

### Build, CI and repository housekeeping (`CI#8`)

- Rename the vcpkg manifest to `colourwars`, drop the hand-repeated version and
  the unused `doctest` (`vcpkg.json:2`, `:11`). Keep the cache key in step.
- Move the audio tests after Build, so their failure does not skip every check
  (`ci.yml:120`).
- Write the manifest-to-runtime-asset rule once (`cmake/release_assets.cmake:10`).
  Fix the "four directories" and rapidjson comments (`game/CMakeLists.txt:344`,
  `:245`).
- Drop the retired kit from `.gitattributes`, and the duplicate `out/` ignore
  (`.gitignore:383`).
- Share the Windows job setup between `ci.yml` and `nightly.yml`
  (`nightly.yml:67`), and keep a manual dispatch from cancelling the scheduled
  run (`:24`).
- Drop the dead `testPresets` (`CMakePresets.json:31`) and the `review-*` trigger
  (`ci.yml:8`).
- Add `retention-days` to the startup upload (`:397`).
- Make the four rapidjson includes use angle brackets (`save.cpp:12`).
- Fix `lint.py`'s docstring (`:57`), and derive `PACKAGED_DOCS` from the package
  rule (`:15`).
- Read each document once in `check_doc_links.py:119`.

This touches `gate`'s jobs, so the pull request says it weakens nothing (AGENTS.md).

## Architecture review (2026-10-30)

On 2026-10-30 the author asked for an architecture review, since the project is
read as an example, and then asked for every finding to go into TODO and be
worked through. The findings are recorded, frozen, in
[the review](review/2026-10-30-architecture-review.md); this section owns their
status. Engine-side findings are ledger reports `ENG#27` to `ENG#33` in
[engine-reports.md](review/engine-reports.md); `ENG#29` was withdrawn on
2026-11-03 and its client-side change, `HYG#52`, is done.

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
[game/match/match_setup.h](../game/match/match_setup.h) and `MenuPlayerSettings` is
`SeatSetup` in [game/match/seat_setup.h](../game/match/seat_setup.h), with no other
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
`colourwars_game` static library, `rumble_gamepads.cpp` into
`colourwars_rumble_gamepads` (`rumble_xinput.cpp` and
`colourwars_rumble_xinput` until `ENG#25`) and `display_awake.cpp` into
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
`SECONDS_PER_STEP` from [simulation.h](../game/contracts/simulation.h), each spelled to
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
[RoundTape](../game/match/round_tape.h): `ReplayFile` holds `tape` (setup, cheats,
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

The file was **format 5**: one inputs array per participant, all the same
length, every step at the file's dt (the writer refuses otherwise), and an
ordered `gaps` list of `{ step, update }` with a resume's seat commands.
Formats 3 and 4 are refused with the version message; a replay holds only on
the build that recorded it (`DBG#2`). `HYG#39` made it **format 6**: `seed` is
now required and is the round seed the level was built from, not the optional
developer seed it used to be drawn from, so format 5 is refused too.
[The debug console](debug-console.md#seed-record-and-play),
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
[tuning.cpp](../game/config/tuning.cpp) and their four checks in
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

**The module, done 2026-10-30.** [menu_geometry.h](../game/config/presentation/menu_geometry.h) at
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
- D5: three meanings of "inside the paper" (interior, face, whole box) are used
  for the same kind of check on different pages.
- D4's row rule, a visible question for the author: practice Pause puts its
  five rows 96 apart (0.8 of 120), but a shipped item's focus card is 113 tall
  (77 + 2 x 18), so neighbouring cards overlap by 17 pixels. The validator
  checks practice rows against the paper only; the row rule would refuse the
  shipped file. Filed with D1 under `POL#45`.

**D2-D4 and D6-D10, done 2026-10-31.** The validator now checks what the pages
draw. Pad sticks: its nine rows (seven option rows, Apply at 7, Back at 8);
Back's card at row 8 is the status obstacle, and the stale Apply-at-3 and
Back-at-4 checks are gone. Every Sticks row sits above a Controls row checked
the same way, and the Controls items must end above the shared status line, so
no content fails only the corrected rows. End's first row fits the face short
of `end_menu.art.shadow_offset`, like its others. Practice Pause: Resume, Tool,
Restart, Options and Quit at 0.8 spacing on the Pause paper, and
`menu_text.practice_tool` for each real tool (glyph coverage stays with
`content_validation`). Display's heading, question and countdown use the face
the setup shade leaves, as Keep and Revert did. `read_main_menu` refuses a setup
paper (Options, setup, Team, Weapon, Stage) that is no larger than
`setup_shadow_offset` plus six keylines, and the Display check refuses a
`pause_menu.box_size` no larger than the setup shade plus six Pause keylines,
since Pause's own box check sees only Pause's shade. `main_menu.credits.text`
must leave a positive fitting area on the Credits face above Back. After the
summaries, the static `menu_text.stage.random_navigation` must fit the Stage
interior, every briefing row, status and hint keeps its expanded briefing card
inside the Stage paper and off the preview (`check_briefing_card`, shared with
the summaries), and then the union of every row and every alternative must keep
one card off the preview, because the page draws one card around all the rows
it shows: a row widening it and another deepening it can reach a preview set
diagonally from the briefing. That union is stricter than any one screen (it
joins solo and versus, locked and browse); the shipped layout passes because the
preview's vertical span lies inside the briefing's. Probes whose accepted edges
encoded the old answers moved to the new edges, and each new check has a variant
only it rejects (`menu_text`, `menu_presentation`) except two: the Pause-box
check, whose failing boxes (under 144 pixels) Pause's own heading and row fits
refuse first with the shipped fonts, and the combined card, which only an authored preview diagonal from the
briefing can reach and which no probe layout builds yet. `footer_y`'s range
check stays at the top of `read_main_menu`, so its order and text are unchanged;
D8 needed no move, since `read_main_menu` runs before `read_controls_settings`
in `presentation.cpp`, so the controls reader only ever sees a checked value.

**The split by page, done 2026-10-31.** `Validation::validate` and its phase
functions stay the only owners of diagnostic order: the eight that check pages
now live in [presentation_validation.cpp](../game/config/presentation/presentation_validation.cpp)
as lists of calls, and `validate_artwork_resources` (phase 4, assets, no page)
keeps its walk in `presentation_validation_assets.cpp`. Each call is one page's
block from that phase, moved verbatim into a private `Validation` member in its
page's file and called where the block stood, so every input throws in the same
order. A page spans phases, so a file holds several steps:

- `presentation_validation_menu.cpp`: what every page shares and the pages with
  no other file. The chrome (menu resources, brand, headings, prompts; phase 1),
  Home's actions and Credits (1), the Title page and Home's subtitle (3), and
  the status-line helpers `padded_back`, `check_status` and
  `status_back_cards`, which measures the Options, Controls, Pad Buttons and
  Sticks Back cards together before any status line, as before.
- `presentation_validation_options.cpp`: actions (1), values and status lines
  (3), palette timing (5), resolution values (8).
- `presentation_validation_settings.cpp`: Controls and Sticks actions through
  `check_controls_item` (1); toggles, Controls help, Keyboard Jump, Pad Buttons,
  Profiles, Sticks help and each page's status lines, with `check_help_fit` (3);
  stick readouts (6); the Sticks pad value (7); percentages and the Sticks
  `shared` template (8).
- `presentation_validation_pause.cpp`: Pause and practice Pause, Exit's Yes and
  No, Display's Keep and Revert, and the four questions (1); Exit's heading and
  Display's heading, countdown and paper (5); connection lines and missing
  lists (6).
- `presentation_validation_end.cpp`: actions and headings (1), Session labels
  (9).
- `presentation_validation_setup.cpp`: Mode and Player Count rows and the Join
  heading (1); Team and Weapon headings and papers (5); bot labels, Join rows
  and the Team and Weapon label rows, one player at a time across both pages
  as before (6); Mode's duration templates (8).
- `presentation_validation_stage.cpp`: briefing, records, then Random counter
  and hints (2, either side of Results' step); heading (5); summaries and the
  expanded card (8). The count checks stay in
  `presentation_validation_catalog.cpp`.
- `presentation_validation_results.cpp`: panel and records (2), contributions
  and awards (7), headings, Session lines and percentages (9).

`presentation_validation_roster.cpp` is gone; its blocks went to setup, pause,
settings and results. The steps take the `SetupGeometry`, `StageGeometry` and
`ControlsGeometry` they need; the phase-2 and phase-5 steps fill their fields
of the geometry by reference, and the later Stage Select step reads the
briefing rows the earlier one left. Three lambdas that several pages' steps
shared became members (`check_controls_item`, `check_help_fit`, which now takes
the help area, and `check_status` with its `padded_back`); one loop over the
Mode duration templates and the Sticks `shared` template became two loops in
the same order. The Results contribution rows, which phase 6 used to declare
and phase 7 fill, are now a local of the Results step, so neither phase passes
them. The four rewritten files and the four new ones include only
what they use. Both presets build under `/W4 /WX`; in Release `menu_text`,
`menu_presentation`, `weapon_catalog`, `story_audio`, `stage_selection`,
`display_trial`, `practice_workshop`, `stick_settings`, `menu_navigation`,
`series_stage_rule`, `user_stages`, `movement`, `turbulence_blockout` and
`debug_console` passed. `HYG#37` is complete.

### Presentation includes and the content layer (`HYG#38`)

Trim the 24-line include block copied into eight `presentation*.cpp` files to
what each uses (`HYG#37`'s page files already include only theirs), so
`build_info.h` and the `states/` badge header appear only where referenced.
Move the badge geometry the roster-label checks (setup and results) need
beside `PlayerIdentityPresentation`, and stop `presentation.h` dragging the
menu records into gameplay objects.

**Done 2026-10-31.** The eight sources that carried the copied block
(`presentation.cpp`, `presentation_menu_text.cpp`, both readers and the
assets, catalog, metrics and text validators) now include only what they use:
`build_info.h` is left only in `presentation_validation_menu.cpp`, which draws
the build label, and `objects/player_badge.h` only in
`presentation_menu_text.cpp`, which formats `PlayerBadge::label`. The metrics
file includes its own `presentation_validation_metrics.h` rather than the
validator. The badge rectangle is now
`PlayerIdentityPresentation::ui_badge_before(label)`, the same arithmetic
`UiPlayerBadge::before` had, and `before` forwards to it, so the pages and the
probes that call it did not change. The setup and results checks call the
record, and no `presentation*` source includes a `states/` header.

The in-match records moved unchanged from `presentation.h` into
[presentation_match.h](../game/config/presentation/presentation_match.h): HUD, identity, camera,
feedback, figure, respawn, opening, debug overlay, music mix and the team
palette. `presentation.h` includes it with the two menu headers and keeps the
`Presentation` class. Twenty `game/objects` headers, `level.h` among them, and
four sources include `presentation_match.h` instead. `level.h`, `level_builder.h` and
`player_builder.h` forward-declare `Presentation`. `level_end_info.cpp`
includes `presentation_menu_text.h` for the `MenuResultsText` it formats.
`Level` keeps the reveal's results text as a copy, now held by
`std::unique_ptr<const MenuResultsText>`, so the snapshot taken with the
reveal font is unchanged and `level.h` needs only a forward declaration. The
nine `Level`/builder sources that call `Presentation` getters include
`presentation.h` directly.

Of the 165 files in `game/objects` and `game/ai`, 70 reached
`presentation_menu.h` and `presentation_menu_text.h` through their includes
before this change and 10 reach a menu header now, none of them headers and
none in `game/ai`: nine reach both through `presentation.h`, and
`level_end_info.cpp` reaches only `presentation_menu_text.h`.
`level.cpp`, `level_draw.cpp`, `level_opening.cpp`, `level_practice.cpp` and
`level_end_info.cpp` use menu records themselves: the menu style's fonts, the
pause menu, the bot roster label and the results text. `level_audio.cpp`,
`level_builder.cpp`, `level_dev_tools.cpp`, `level_intro.cpp` and
`player_builder.cpp` read only in-match records, but through the `Presentation`
class, which holds the menu records by value. Handing `Level` and the builders
the records they read is left to `HYG#40`, which regroups `Level`'s inputs.
Thirteen acceptance probes that used `Presentation` through `level.h`
(`aim_marker`, `blocked_shot`, `cursor_marker`, `idle_drip`,
`movement_contact`, `paint`, `player_identity`, `practice_workshop`,
`quiet_character`, `teammate_markers`, the balance runner, the skirmish
observation checks and the paint-surface fixtures) now include it. Both
presets build under `/W4 /WX`. Every default case compiles in both presets,
along with the opt-in cases and both painter probes in Release. In Release,
`menu_text`, `menu_presentation`, `player_identity`, `weapon_catalog`,
`story_audio`, `team_palette`, `aim_marker`, `teammate_markers`,
`results_awards`, `instant_replay`, `cursor_marker`, `low_paint`, `paint`,
`blocked_shot`, `idle_drip`, `quiet_character`, `movement_contact`,
`practice_workshop`, `team_selection`, `stage_selection` and
`menu_navigation` passed. `HYG#38` is complete.

### An explicit match random stream (`HYG#39`)

Replace the global `srand`/`rand` seeding of a round with a `MatchRandom` value
passed into `LevelBuilder::build_level`, with independent sub-streams for
palette, bot seeds and the monitor persona; `RoundTape` stores the seed it
hands in. Seeded team pairs and colours may change; record the new `+seed`
mapping where the screenshot loop documents it.

**Done 2026-10-31.** A build now draws only from the
[MatchRandom](../game/objects/match_random.h) it is given, and never reads or
moves `rand()`. `MatchRandom` holds the round seed and makes one `BotRandom`
(splitmix64) stream per use: the state of each is the first output of
`(tag << 32) | seed`, with tag 1 for the palette and 2 for the persona. The
shipping bots get the seed itself, as the balance runner already seeded its
bots. A Random palette is `next() % pairs` and the persona's rolls are
`unit()` and `next() % n`, all reduced in 64 bits, and every persona draw is a
named local because argument order is unspecified. So the same seed builds the
same match on any standard library, and whether one use draws never moves
another.

- **The build.** `build_level(settings, palette, opening, random, practice)`
  passes the stream to `TeamColourTools::team_colours` and to `Level`, whose
  `bot_seed` parameter became `MatchRandom random`. The rand-based
  `generate_random_team_colour(palette)` and `MonitorPersona::roll(look)` are
  gone. The two-argument `team_colours` is fixed-pair only and refuses Random
  by name. The weapon page's preview never took that path.
- **The round seed.** `GameLevel` still draws it from `rand()`, as two
  sequenced locals, and asks the new `DebugDirector::round_seed(drawn)` hook,
  whose Release stub returns `drawn`. In Debug a replay being played returns
  its recorded seed and `seed n` returns `n`. Either override also calls
  `srand(n)`, so the menus' Random picks after every seeded build repeat. The
  two `srand` calls in `before_level_build` are gone. The menus' team shuffle,
  random weapon and random stage stay on `rand()`.
- **The instant replay.** It builds from `MatchRandom(tape.seed())` and from
  `round_palette_`, the preference the round was built with. Before this, it
  read the save's palette, which Paused Options can change mid-round while
  keeping the arena, so a replay could show the new colours.
- **`ReplayFile`.** It is **format 6**. `seed` is required and is the round
  seed, so a recording made without `seed` now replays its Random palette.
  The header starts its tape from it, and format 5 is refused. `status` shows
  `round seed n`.

`+seed n` now means "every round is built from seed n". The seed-to-pair table
is in [the debug console](debug-console.md#seed-record-and-play), and
`team_palette` pins it for seeds 0-7, 211 and 4242. For the probes:

- **The shipping-bot probes swapped their seeds.** Each now names the bot seed
  its old `srand` gave, so their bots are bit-identical: `41731` became `5242`
  (the `bot_navigation` fixture, `bot_trials` and `bot_scan_cost`'s level),
  `9127` became `29843`, `30011` became `32505`, `52807` became `8644`,
  `61403` became `3946`, and `bot_runtime`'s `8301` became `27146`.
  `bot_scan_cost`'s timed driver keeps `41731`. `bot_team_trials`' Close
  Quarters exclusion now names seed `3946`.
- **The other build sites.** Every other `build_level` site passes
  `MatchRandom(k)` for its old `srand(k)`, or `MatchRandom()` where nothing was
  seeded. The `srand` stays only where a probe reads a terminal `rand()` after
  the run: `paint_surfaces`, `moving_structure`, `turbulence_movers`,
  `turbulence_slab_routes` and `pad_buttons`.
- **New checks.** `instant_replay` checks that a seed's palette is the same
  with and without an intro and shipping bots, and that a full build leaves
  `rand()` untouched. `debug_console` records without `seed`, checks that the
  file holds the drawn round seed, and checks that playback builds from it and
  reports MATCH.

The baseline was taken on the unchanged tree. Against it, the `MEASURE` and
`PASS` lines of `paint_ferry`, `team_gate`, `handicap`, `bot_territory`,
`bot_navigation`, `bot_gate_routes`, `bot_runtime` and `bot_ascents` are
identical apart from the relabelled seeds. So are the rows of
`bot_trials.csv` and `bot_combat_trials.csv`. The full default list passed in
Release (74 cases). Debug passed with `-SkipSlowDebug` (71 cases), and
`debug_console` and `crossfeed` passed when run on their own. The opt-in group
and both painter probes compiled. `HYG#39` is complete.

### `Level` composed of owned parts (`HYG#40`)

**Done 2026-10-31.** `Level` is now the composition root of owned parts, in
nine commits (stages 1 to 8, with stage 5 in two halves). It keeps
the match state, every timer, the roster and pane maps, the scene and the order
of `update`, `update_state`, `update_level_logic`, `draw`,
`draw_player_overlay` and `refresh_layout`; each part keeps one concern's
state and drawing, borrows the tuning and presentation and reads them per call
(so `tune` stays live, and `game/debug/tune/tune_scope.cpp` names the new readers),
and is created at the point its members were. Every public `Level` member
stays, as a forwarder where its body moved; `players_` and `timer_` keep their
names for `movement_feedback_visual.cpp`, `cursor_marker_frames` stays a
public static, and `level.h` keeps every include it had, so no probe changed
beyond the two planned edits below. The bodies moved unchanged.

The constructor takes four records from
[level_inputs.h](../game/objects/level_inputs.h) instead of 27 parameters:
`LevelWorld` (the scene, its players and the pane dividers, handed over),
`LevelServices` (the borrowed services and live tuning, presentation,
preferences and bindings), `LevelStage` (name, camera bounds, `StageAudio`,
stations, lessons) and `LevelMatch` (team colours, opening, duration, roster,
random stream). `LevelBuilder` fills them inside the `try` that names the stage
in an error; its own constructor is unchanged. The four probes that build a
`Level` by hand use designated initialisers.

[LevelStructures](../game/objects/level_structures.h), a value member
(`level_horizontal_motion.cpp` became `level_structures.cpp`), owns what
`Level` borrowed at admission: the translating structures and their supports,
the team gates and the projectile targets, collected in the old order inside
`build_opening_frame`; `TranslatingStructurePaintable` befriends it instead of
`Level`. The bots query the world through [BotWorld](../game/ai/bot_world.h),
an abstract `game/ai` class that returns only values (a `BotBody` per
participant with `tool_ready`/`tool_empty`, the gates' permissions, paint
surfaces, obstacles, supports, ramps and the two sight lines).
[LevelBotWorld](../game/objects/level_bot_world.h) implements it, borrowing
the scene, `players_` and `LevelStructures` by reference because the targets
are admitted after it is built. `MatchBotDriver` no longer derives from
`SeatInputFilter`: `drive(seats, world)` keeps the same loops and RNG draws, so
`game/ai` includes neither `level.h`, `weapon.h` nor the team gate. The four
probes that drove it call `drive(seats, level.bot_world())`; the balance
runner wraps it in a probe-local filter.

The window-space and HUD layers followed. [PaperCard](../game/objects/paper_card.h)
holds the overlay sheet, its fill and the one card scale and card draw that
every card uses; [PracticeLessons](../game/objects/practice_lessons.h) the
lessons and their cards; [CursorMarkerOverlay](../game/objects/cursor_marker_overlay.h)
the mouse pointer and its marquee (all three value members);
[TeammateMarkers](../game/objects/teammate_markers.h) the edge chips and their
per-pane fade ages; [PlayerIdentityHud](../game/objects/player_identity_hud.h)
the badge, the emphasis, the bot labels, seat labels, pad mapping and HUD
identity texts, created empty before the roster loop so each bot label is
still made before the no-human-view throw. [MatchAudio](../game/objects/match_audio.h),
declared after the scene, owns the one `MatchVoices` list, the music as its
`MusicVoice`, the end sting, the match cues, the room tone and the story
voices, and resolves and registers them in the old order; `Level` still
registers each player's voices after them, decides when every cue plays, and
keeps `~Level` in `level_audio.cpp`, which stops the list before any member
goes. `PaintAudio` and `BlockedShotFeedback` stay in `Level`, declared before
the scene, because players point at both. [CountdownCards](../game/objects/countdown_cards.h)
owns the countdown, resume and replay cards, the stage title and the controls
reminders, built in two steps at the old points; `Level` keeps the state checks
and the beat timing.

[MatchOpening](../game/objects/match_opening.h), owned through a `unique_ptr`
that is null when the match has no intro, took the opening, the reveal and the
figures' acting from `level_intro.cpp` into `match_opening.cpp`: the
`Monitor`, `Silhouette`, `ScreenCursor`, `FigureActing` and `RevealArt`, the
coverage text, the hover, click and close waves, the opening and reveal clocks,
the picture bounds, the end info and winner, the acts and their beats. It is
created where those members were, rolling the persona and making them in the
old order, and greys the figures as before; `build_opening_frame` still works
out the picture bounds at the same point and hands them over. It borrows
`MatchAudio` for the room tone and story voices. `Level` keeps `state_`,
`intro_countdown_`, `glass_in_started_`, the skip request and every timer,
hands them over as an `OpeningClock` on each call, and applies each transition
the part returns where it was written before: `update_opening` returns `stay`,
`countdown` (a skip) or `intro_countdown`, `update_reveal` returns whether the
reveal is over, and `Level::skip_intro` calls `skip` and then sets the state
before the countdown branch reads it. `start_reveal` is called only when the
opening exists, so the scene is still counted only then; `update_acting` runs
only with an opening, where the null `acting_` check returned before. The draw
takes a callback that draws `Level`'s countdown at the old point, and the
intro's views are added in the same order. `revealed()` is now whether the
opening exists. `LevelState` moved into `level_state.h`, which `level.h`
includes, so the part need not include `level.h`. `Level` keeps its
`SoundBankObject` base although nothing plays through it now: resolving the
stage's bank there is the first check a `Level` makes, before the roster
checks, and dropping it would report a bad roster ahead of a bad bank.

Every stage built both presets under `/W4 /WX`, ran the acceptance cases,
script checks and painter switches its files touch, and compared what the plan
asked of it (`check-paint-fx.ps1` failing only on its known camera-scale
check). For the last
commit: Release passed all 74 default cases and Debug 71 with `-SkipSlowDebug`
plus `crossfeed`, `press_tower`, `menu_text` and `debug_console`; the opt-in
group, both painter probes and the frame-pacing probe compiled;
`check-movement-feedback.ps1` and `check-display-awake.ps1` passed;
`check-startup.ps1` passed on both executables, and with the bank hidden
under `-RequireNoAudio` and for a `+stage` match launch, which reported the
silent bank once and exited 0. `bot_trials.csv`, `bot_combat_trials.csv` and
the `bot_team_trials` reports are byte-identical to the stage-3 evidence, and
the 2- and 4-player `run-balance.ps1 -Bot match` CSVs on `crossfeed` are
byte-identical to the baseline taken before stage 1. On MattPC the painter's
`-MonitorOnly`, `-RevealTuneColoursOnly`, `-ResultPercentageOnly`,
`-CountdownOnly`, `-LandmarkOnly`, `-SilhouetteTuneColoursOnly`,
`-SilhouetteBodyTuneColourOnly`, `-ActingTuneColoursOnly`,
`-GlassTuneColourOnly`, `-TeamSetupOnly` and `-PausedOptionsOnly` matrices
passed, and the full run passed 716 checks, as it did after stage 7.
`HYG#40` is complete.

### `GameLevel`'s suspension rules in one object (`HYG#41`)

**Done 2026-11-02.** `GameLevel`'s four booleans (`covered_`,
`pause_menu_open_`, `focus_interrupted_`, `background_continuing_`) and its
`ResumeCountdown` are now one owned
[MatchSuspension](../game/states/match_suspension.h). It records each reason
through named transitions (`cover`/`uncover`, `open_pause_menu`/
`close_pause_menu`, `lose_focus`/`regain_focus`, `begin_count_in`,
`background_changed`, and `reset` for a new level). It answers the questions
the callbacks used to spell out each time: `audio_may_play`, `music_ducked`
and `ducked_music_plays`, `rumble_paused`, `may_run_unfocused`,
`suspends_level`, `live`, and whether a focus loss is owed a count-in
(`take_focus_interruption`). `GameLevel` still makes the audio, rumble and
`Level` calls, in the order it did before. Each callback reads the window's
focus and the director's clock and background policy once, into a
`SuspensionGate`. It reads them again only after a developer hook that may
change them: `before_match_update`, the build hooks and the match steps.
`sync_rumble`, `update_music`, `update_replay`, `drain_rumble` and
`begin_resume_countdown` take the caller's gate. `init()` now creates the two
scopes the destructor used to end by hand. One is
[ScreenLayoutRestore](../game/states/screen_layout_restore.h); the other is
`DebugMatchAttachment` in `debug_director.h`, which detaches again if
`attach_match` throws. The destructor ends them in the old order, after it
stops rumble. One behaviour changes deliberately: a `GameLevel` destroyed
without `init()` no longer sets the layout to one player or detaches a match
it never attached. A three-lens adversarial review compared the old and new
code for each callback. Its three candidate differences were refuted by
both of their verifiers: each was on an exception path out of `init()`, and
that path ends the process. The new CPU case `match_suspension` covers the
reasons, the focus hold, the count-in and background mode. Both presets build
under `/W4 /WX`. The full default list passed in Release, and in Debug with
`-SkipSlowDebug` plus `crossfeed`, `press_tower`, `menu_text` and
`debug_console`. After the final attachment change, the cases that drive
`GameLevel` (`debug_console`, `instant_replay`, `display_awake`,
`cursor_marker`) passed again in both presets, and `check-startup.ps1` passed
on both executables.

### Structure queries instead of casts (`HYG#42`)

**Done 2026-10-30.** Structures, players and projectiles now derive from the
new abstract [Collider](../game/objects/collider.h), a game-side
`labrador::CollisionObject`. Its final `on_contact` asks the other party to
`touch` this one, and that party's `touch` calls `on_structure_contact`,
`on_player_contact` or `on_projectile_contact`. `Player` and `Projectile` no
longer decode the tag of what they touched, and the painted structures no longer
cast it. `Collider` answers the roles `structure()`, `projectile()` and
`paintable()`. `Collider::of` is the one conversion from the engine's type and
refuses anything else. That cross-cast was `ENG#29`'s exhibit in the
[ledger](review/engine-reports.md) (withdrawn 2026-11-03; now `HYG#52`). [Structure](../game/objects/structure.h)
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

**Done 2026-10-30.** [game/contracts/level_format.h](../game/contracts/level_format.h), still
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
are present. [level.schema.json](../game/contracts/level.schema.json) moved from `docs/`
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

**Done 2026-10-30.** [game/config/stage_catalog.h](../game/config/stage_catalog.h) owns the
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

**Done 2026-10-30.** [`SettingsSession`](../game/config/settings_session.h) now owns
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
field print nothing. [save_section.h](../game/save/save_section.h) states the
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

**Done 2026-11-02.** It came last, because it moves files that every other row
touches. 97 of the 111 files at the `game/` root moved into six role folders.
`contracts/` holds the nine engine-free headers and `level.schema.json`, and
`match/`, `config/`, `config/presentation/`, `save/` and `input/` hold the
rest. The root keeps only the shell. 69 of `game/debug`'s 79 files moved into
`commands/`, `console/`, `input/`, `script/`, `replay/`, `tune/` and
`capture/`. The seam headers, `dev_tools.h` and the director's implementation
stay at its root, so nothing outside `game/debug` changed what it includes.
[docs/architecture.md](architecture.md#the-repository) gives the map and the
two contested placements: `play_settings.h` and `settings_session` are in
`config/`, not `save/`, and the Debug bots are in `replay/`. The map was drawn
from the include graph. For each of the two areas, two independent proposals
were made and a judge checked the code to choose between them. Nothing else
changed: every include, CMake source entry, probe, script path, live-doc link
and source comment was rewritten from one table. The frozen reviews keep the
old paths. `game/carried` keeps its name. It is `ENG#28`'s exhibit, and it is
deleted when the engine answers that report, so renaming it would only move
the exhibit.

The new `colourwars_check_contract_includes` target runs
[cmake/check_contract_includes.cmake](../cmake/check_contract_includes.cmake).
It refuses any include in `game/contracts/` other than a bare standard header
or another header in that folder, and it names each offending line. It runs on
every build because `colourwars_game` and the editor's
`colourwars_level_document` both depend on it. It was tested on a copy of the
folder with four planted includes (another game header, an engine header, a
missing contract and `<windows.h>`), and it refused all four.

Both presets build under `/W4 /WX`, and so do the editor's acceptance targets
and `ColourWarsRegisterStage`. The full default list passed in Release (75
cases) and in Debug (76, the slow four included). Both opt-in groups compile,
and so do `check-painter.ps1 -CompileOnly` (both presets) and
`check-frame-pacing.ps1 -CompileOnly`. `check-display-awake.ps1` and
`check-startup.ps1` passed on both presets. `check-level-editor.ps1` passed in
Release. In Debug it failed once at the close-cancel dialog step and passed on
both re-runs; no source it builds changed except for include paths.

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

### Promote stable nightly checks (`CI#7`)

**Deferred until a week of scheduled nightlies, from 2026-11-11.**
[nightly.yml](../.github/workflows/nightly.yml) reached `master` on 2026-11-04
and first fires on its schedule at 16:17 UTC on 2026-11-05. Its first hosted
trial passed 96 of 97 painter leaves and paint effects in both presets. The
other leaf, Resume, and the plain matrix stopped at music flow, because the
runner has no audio endpoint; since `99240203` the job sets
`COLOURWARS_NO_AUDIO_ENDPOINT` and the painter skips only its music flow. Once
a week of scheduled runs is in, the legs that stayed green and are short enough
for a pull request move to [ci.yml](../.github/workflows/ci.yml); the rest stay
nightly.

### A typed projectile list in `Level` (`HYG#52`)

**Done 2026-11-03.** Added that day, when the author withdrew engine report 29
([Closed](review/engine-reports.md#closed)): its exhibit was this game's own
choice, not something the API forced. A performance and clarity cleanup with
no player-visible effect.

[Level](../game/objects/level.h) now keeps the `Projectile*` that `Scene::add`
returns, in two lists: `projectiles_`, the admitted shots in scene order, and
`pending_projectiles_`, those added this tick. `resolve_projectile_movement`,
the drain after `scene_->resolve()` and `count_projectiles` walk
`projectiles_`, so the three `collision_objects()` walks and their
`Collider::of` cross-casts and tag tests per object are gone. Each walk sees
the same shots as before: the admitted ones, in scene order, never this tick's.

Every `end_tick()` in `Level` now goes through `end_scene_tick()`. That calls
`retire_projectiles()` first, while the pointers it drops are still valid: it
erases from `projectiles_` each shot with
`for_deletion() || !scene_->in_bounds(*p)`, which is the coming `end_tick()`'s
retire set, since its bounds sweep flags an object outside the scene and every
`Projectile` records the flag. Then it appends this tick's shots, as
`end_tick()` appends its pending list. A shot added this tick is not pruned,
because `end_tick()` admits it whatever its flags and the next one sweeps it
(`scene.h`: "retired by the first end_tick() after its insertion"). The
prune runs immediately before `end_tick()`, after the idle drips, rather than
in the drain loop, so nothing between the two can flag a shot unseen.
`check_projectiles()` follows `end_tick()`. In Debug it throws
`std::logic_error` unless `projectiles_` is exactly the scene's projectiles in
scene order, its size equals the projectile-tag count the overlay used to
read, and nothing is left pending. It is kept as the guard, not removed after
the first run. The predicate never disagreed with `end_tick()`, so report 29
stays withdrawn.

The guard found one disagreement, and it was a probe's. `moving_structure`'s
two Roller checks added a shot straight to a built `Level`'s scene and called
its `end_tick()`, which a `Level` cannot see. `Level::admit_projectile` is the
seam for that now: between updates it adds a shot made outside a weapon and
admits it at once, as the station labels are admitted. The two checks call
it, and the third hand `end_tick()`, which retired the spent Roller, is a
zero-step `tick()`. No game code adds a shot any other way. `collider.h` no
longer points at report 29; `Collider::of` stays for a contact's other party
(report 33) and for the scene walks that remain. Behaviour, evaluation order,
RNG streams and formats are unchanged.

Both presets build under `/W4 /WX`. The full default acceptance list passed in
Release (76 cases) and in Debug (77, with the slow cases), including the bot trials' replay fingerprints and
`debug_console`'s record and play, which replay matches bit for bit with the
Debug guard running. `check-startup.ps1` passed on the Debug executable.

## Open shortlist (2026-10-29)

The author approved adding every proposal from the 2026-10-29 verified
brainstorm to the index. Each entry below is **open, not started**. Evidence
was re-read against the tree on that date; refresh line numbers before
starting. None of them closes a human gate, and any listening, feel, art or
attended-play judgment it touches stays a `DECISION` row.

### Unreadable save preservation (`HYG#22`)

**Done 2026-10-29.** `Save::load_save_data` now hands a save that fails as a
whole to `keep_unreadable_save` ([save.cpp](../game/save/save.cpp)) before keeping
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
`SaveData()` with no message ([save.cpp](../game/save/save.cpp) lines 433-441), and
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
[save.h](../game/save/save.h) lines 33-44: version 5, versions 4 (`"profiles"`) and
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
| [presentation_validation.cpp](../game/config/presentation/presentation_validation.cpp) | 25 | Ordered coordinator and nine family sources; `HYG#1` complete |
| [player.cpp](../game/objects/player.cpp) | 1,848 | Retain the coupled movement/contact/carrier core; figure code is already separate |
| [input_bindings.cpp](../scripts/acceptance/input_bindings.cpp) | 56 | Four topic sources and shared fixtures; `HYG#5` complete |
| [evidence-record.ps1](../scripts/check-painter/evidence-record.ps1) | 354 | Four explicit topic helpers; `HYG#6` complete |
| [painter_visual.cpp](../scripts/acceptance/painter_visual.cpp) | 1,347 | Retain the explicit include/setup/dispatch entry; no registry redesign selected |
| [level.cpp](../game/objects/level.cpp) | 966 | Horizontal-motion subsystem extracted (380 lines), `HYG#2` complete |
| [tune_colour_document_checks.h](../scripts/acceptance/debug_console/tune_colour_document_checks.h) | 17 | Ordered topic coordinator; `HYG#7` complete |
| [tuning.cpp](../game/config/tuning.cpp) | 907 | Retain cohesive key-by-key parsing and validation |
| [level_document.cpp](../editor/level_document.cpp) | 1,168 | Reflection family (`ED#24`/`ED#25`) extracted (300 lines); shared transaction, save included, retained; `HYG#54` complete |
| [projectile.cpp](../game/objects/projectile.cpp) | 287 | Contacts/sweeps extracted (587 lines), `HYG#3` complete |
| [weapon.cpp](../game/objects/weapon.cpp) | 855 | Retain the current firing/lifecycle core; motion already has its own file |
| [save.cpp](../game/save/save.cpp) | 602 | Profile overlay extracted (209 lines), `HYG#4` complete |

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
`game/save/save_profiles.cpp` (209 lines). The 602-line `save.cpp` retains JSON
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

## Long-file audit (2026-11-05)

A fresh pass over the 36 longest authored code files (about 600 lines and up,
`external/`, data JSON and `scripts/studies/` excluded). One reader mapped
each unit and proposed keep or split. Each proposed split then went to two
independent skeptics, one checking cohesion and one checking whether the plan
works as a mechanical move, with a brief to kill it if the split was motion
rather than improvement. Five splits survived (`HYG#53`-`HYG#57` below), and
one commit removed six dead includes (`save.cpp`, `level.cpp`,
`tune_commands.cpp`, `user_stages.cpp`). The rulings below are recorded so a
later pass starts from them rather than re-deriving them.

| file | lines | ruling |
|---|---:|---|
| `game/objects/player.cpp` | 1,910 | Keep. The growth since 2026-10-28 is POL#46 figure resolution that belongs in the constructor; a contacts/movement split would separate the producer and consumer of the per-tick support flags |
| `editor/level_document.cpp` | 1,470 | Reflection family split out (`HYG#54`); the file transaction (load/save/Win32) stays with `create` and `mutate`, as ruled 2026-10-28 |
| `scripts/acceptance/painter_visual.cpp` | 1,386 | Keep, 2026-10-28 ruling stands: check bodies already live in topic headers |
| `game/debug/replay/replay.cpp` | 1,148 | Keep: one type's on-disk format |
| `scripts/acceptance/bot_team_trials.cpp` | 1,022 | Keep: one harness whose passes share streams and counters |
| `scripts/acceptance/debug_console/replay_checks.cpp` | 927 | Codec/file checks split out (`HYG#55`) |
| `game/config/tuning.cpp` | 892 | Keep, 2026-10-28 ruling stands |
| `game/config/presentation/presentation_read_menu.cpp`, `presentation_read_match.cpp` | 891, 876 | Keep: one section-to-record reader each |
| `editor/editor_window.cpp` | 880 | Split killed by both skeptics: one Win32 shell, and the 60-line clipboard leaves the reader's problem unchanged |
| `game/config/presentation/presentation_menu_text.cpp` | 874 | Split killed by both skeptics: each template's token set and its formatter must change together |
| `game/objects/level.cpp` | 878 | Keep, plus a dead-include fix: composition root and one tick order |
| `scripts/acceptance/level_format.cpp`, `movement_feedback_visual.cpp`, `user_stages.cpp` | 867, 850, 841 | Keep (dead include removed from `user_stages.cpp`) |
| `game/debug/replay/replay_commands.cpp`, `debug_director_hooks.cpp` | 853, 835 | Keep: already DebugDirectorImpl family sources |
| `scripts/acceptance/debug_console/tune_colour_sweep_checks.h` | 847 | Colour-pair checks split out (`HYG#56`) |
| `game/objects/weapon.cpp` | 842 | Keep, 2026-10-28 ruling stands |
| `scripts/acceptance/crossfeed/opposed_loop_checks.cpp`, `turbulence_slab_routes/route_checks.cpp`, `debug_console/director_checks.cpp` | 836, 826, 790 | Keep: shared fixture or one simulation's captured state |
| `game/debug/script/script_commands.cpp` | 797 | Split killed on a 1-1 tie: moving the perf block would part `perf.capture`'s registration from its body, and `draw_status` is mostly not perf |
| `game/debug/tune/tune_commands.cpp` | 796 | Keep, plus a dead-include fix |
| `game/objects/level_object_builder.cpp`, `match_opening.cpp` | 760, 698 | Keep: one decode path; one timeline |
| `scripts/check-painter.ps1` | 757 | Two leaf evidence checks split out (`HYG#57`) |
| `game/states/game_states.cpp` | 745 | Keep: GameLevel's families already have their own sources |
| `scripts/check-debug-console.ps1` | 701 | Split killed by both skeptics: one ordered attended script with guarded phases |
| `game/states/main_menu.cpp` | 697 | Mode Select page split out (`HYG#53`) |
| `game/config/presentation/presentation_match.h`, `presentation_validation_settings.cpp` | 686, 680 | Keep: an aggregate catalogue; already a `HYG#1` family source |
| `editor/editor_canvas.cpp`, `stage_registration.cpp`, `game/config/locale.cpp`, `game/save/save.cpp`, `scripts/check-acceptance-process.ps1` | 669-616 | Keep (dead include removed from `save.cpp`) |

## Main-menu Mode Select page (`HYG#53`)

**Done (2026-11-05).** Moved the whole `MainMenuModeSelect` region - its
constructor, the static setup-row builders and fit checks
(`make_setup_row` through `check_stage_rule_row` and
`practice_hint_position`), the duration and series-stage refresh/cycle pairs,
`update_page` and `init` - into
`game/states/main_menu_mode_select.cpp` (330 lines), in its original order.
It was the one large leaf page still beside the page bases; every other
main-menu page already had its own file. The 372-line `main_menu.cpp` keeps
`MainMenuPage`, `RosterSetupPage`, Title and Home. The file had no anonymous
namespace or file-local helper, `main_menu.h` is unchanged, and the only
other edits are the new file's preamble, its CMake registration and dropping
the two includes (`series_stage_rule.h`, `ui_text_drop_shadow.h`) that only
Mode Select used. The unused `drawable_text.h` include was left for a
separate change.

A line comparison confirms the moved block is identical to the original
lines, including the `main_menu.mode_select_spacing` and `menu_text.values`
diagnostics. Debug and Release builds pass, as do `menu_presentation`,
`menu_navigation`, `remembered_setup` and `series_stage_rule` in both presets
and `menu_text` in Release.

## Level-document reflection family (`HYG#54`)

**Done (2026-11-05).** Moved the `ED#24`/`ED#25` reflection rules out of
`editor/level_document.cpp` into `editor/level_document_mirror.cpp` (300
lines), in their original order: the anonymous `set_text`,
`advance_half_cycle` and `mirror_object`; `LevelDocument::mirror_copy`; the
anonymous `paint_ferry`, `strip_unmirrored` and `same_shape`; and
`LevelDocument::symmetry`. The file had grown about 520 lines since the
2026-10-28 snapshot, and roughly 270 of them were this one rule set with its
own private helpers and its own sections in
[the level editor guide](level-editor.md). The type is not split and no
public signature or layout changes.

Two things move with it, because the reflection bodies reach into the
document's private state. `struct LevelDocument::Impl` and its inline
`mutate` template move verbatim to the new editor-private header
`editor/level_document_impl.h`, which only the two level-document sources
include; it carries a `using namespace level_json;` so the `Impl` body stays
unqualified. The four helpers the reflection family shares with the rest of
the file - `entry_json`, `entry_array`, `set_number` and `move_shape` - change
from internal to external linkage: their definitions stay in place in
`level_document.cpp`, rewrapped from the first anonymous namespace into
`colourwars_editor::level_document_detail`, which the header declares and
brings into scope. The other helpers in that namespace stay anonymous and in
source order. No other editor source defines those names.

The file transaction stays: `load`, `create` and the `ED#7` save with its
`save_target`, `same_save_target` and `check_saved_source` guards are the
other half of `mutate`'s history contract, as the 2026-10-28 snapshot ruled,
and `<windows.h>` stays with them. The clipboard and `unresolved_resources`
also stay; each shares a helper with the edit core. The moved lines are
identical to the originals. Debug and Release builds pass, as do
`check-level-editor.ps1` in both presets (mirror copy and symmetry included),
`check-stage-registration.ps1` and the `user_stages` case in Debug.

The six acceptance cases that compile the real document through one-line
`editor_*_adapter.cpp` files (`fixed_slab`, `moving_structure` and the four
`turbulence_*` editor cases) each gained an `editor_mirror_adapter.cpp`; a
source added to `colourwars_level_document` needs the same adapter there.

## Replay codec and file checks (`HYG#55`)

**Done (2026-11-05).** Split `scripts/acceptance/debug_console/replay_checks.cpp`
(927 lines) along the two jobs its own header comment named. The codec and
file job - `check_replay_codec` with its private `same_inputs` and `same_gaps`
and the `check_bot_replay_metadata` declaration it calls - moved whole and in
order to `replay_codec_checks.cpp` (283 lines): the bit-exact input round
trip, run-length coding, the `DBG15` `replay_name` matrix and the
`ReplayFile` write/read round trip with its version, length, step-size, gap
and layout refusals in a temp folder. It builds no `Level` or director. The
671-line `replay_checks.cpp` keeps `check_replay_cpu_match`, the one ordered
scenario on a real CPU `Level`, with `count_of_lines`, `retaped`,
`suspend_steps` and `drive`. That function stays whole: its locals run from
the seeded recording to the unseeded seed, and lifting a piece such as the
`DBG16` catalog block would need new parameters.

No helper crosses the cut, so each anonymous namespace stays fully used under
`/W4 /WX`. The new file copies the original include block verbatim (any
trimming, such as the apparently unused `bot.h`, is a separate change); the
only other edits are the two file comments and the `fixtures.h` heading that
now names `replay_codec_checks.cpp` for `check_replay_codec`. The runner
compiles every source in the case folder, so nothing registers it, and
`debug_console.cpp` calls the two checks in the same order. A line comparison
confirms both files' moved lines match the originals. The Debug build passes
and the `debug_console` case compiles in Debug.

## Colour-pair tune checks (`HYG#56`)

**Done (2026-11-05).** Split
`scripts/acceptance/debug_console/tune_colour_sweep_checks.h` (847 lines),
whose comment named only the sweep and the identity tunes, along its two kinds
of check. `check_identity_colour_tunes` and `check_hud_paper_colour_tunes`, the
coupled opaque, distinct pairs that each run in fresh LF/CRLF scratch contexts,
moved whole and in order, with the comment between them, to
`tune_colour_pair_checks.h` (373 lines). The 476-line sweep header keeps
`check_colour_tunes`, the sequential walk of every audited colour path over one
shared scratch document. That function stays whole: its loops share `words`,
`expected`, `initial` and the borrowed pointers in order, so cutting it would
need new parameters.

The new header has the same inclusion context as its siblings: no includes of
its own, included by `tune_checks.cpp` immediately after the sweep header,
inside the same anonymous namespace. Call order, scratch folders and
diagnostics are unchanged. Concatenating the two bodies in include order
reproduces the original lines exactly, mixed tab/space indentation included;
the only other edits are the two file comments.
The runner compiles the case folder through `tune_checks.cpp`, so nothing
registers the header. The `debug_console` case compiles in Debug and passes.

## Painter practice-replay and rumble evidence (`HYG#57`)

**Done (2026-11-05).** `scripts/check-painter.ps1` (757 lines) stays one
coordinator, but two leaves kept their post-run evidence inline against the
rule its own header states: a leaf with post-run evidence gets a guarded
`check-painter/verify-<leaf>.ps1` dot-source. Both blocks were added after
`19d3fc91` created that family. The `PracticeReplayOnly` receipt check moved
whole to `check-painter/verify-practice-replay.ps1` (32 lines) and the Debug
`RumbleOnly` background-supplement check to `check-painter/verify-rumble.ps1`
(21 lines), each with the family's two-line header and its bodies at their
original 8-space indentation. The entry, now 708 lines, keeps the guards
`if ($PracticeReplayOnly)` and `if ($RumbleOnly -and $debug)` unchanged and
dot-sources each helper at the position of the block it replaces, not in the
verifier list, so failures still come in the same order: practice receipt,
remembered-setup relaunch, save diagnostics and fixture hash, background
receipt, trial hash, then the other verifiers.

The moved bodies read only `$captures`, `$debug` and `$result` and use no
script path or `return`, so dot-sourcing keeps their meaning. A line
comparison confirms both bodies match the originals. The selection checker's
fault copy stops before either block, so it needs no change. All three
scripts parse, `check-painter-selection.ps1` passes, `-CompileOnly` compiles
in both presets, and the native `-PracticeReplayOnly` and `-RumbleOnly` leaves
pass in Debug and Release with their `practice_replay_receipt` and Debug
`background_receipt` recorded.
