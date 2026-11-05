# Code hygiene review (2026-11-05)

A record of the code hygiene review run at the author's request on 2026-11-05,
against `master` at `8a04c998` with the engine pinned at master `e7fc50e`
(`v0.3.0` plus vsync). The question was what needs tidying across the whole
tree now that the 2026-10-30 architecture rows and the 2026-11-05 long-file
audit are done.

**This file is frozen.** It records the findings as they were made. Live status
is in [code-hygiene.md](../code-hygiene.md#hygiene-review-2026-11-05) (the
`HYG#58` to `HYG#92` rows and `CI#8`) and, for the engine's side, in
[the engine-report ledger](engine-reports.md) (`ENG#35`).
[TODO.md](../../TODO.md) indexes both.

## Method

Twenty-eight finders ran in parallel. Twenty-one each read one area in full:
three slices of `game/objects`, two of `game/states`, two of `game/debug`, two
of `game/config`, the small `game/` directories, the editor, seven slices of
`scripts/acceptance`, the painter scripts, the other scripts and the build and
CI files. Seven swept the whole tree for one concern each: dead code,
duplication, naming conventions, comment and row-id drift, data versus code,
latent correctness hazards, and live docs that no longer match the code.

Every finding then went to skeptics briefed to refute it. A low finding had one
skeptic, who checked it against the code. A medium or high finding had two: one
checked the facts, the other checked scope and whether the fix was improvement
or motion. A finding survived only if no skeptic refuted it or ruled it out of
scope. A completeness critic then named six under-covered areas, which were
swept and verified the same way. Last, a synthesis pass grouped the survivors
into themes.

555 raw findings came in, 533 were left after merging duplicates, 500 survived
and 33 were refuted or ruled out of scope. Where skeptics disagreed on severity
the lower grade was kept, so no finding is rated high: 78 are medium and 422 are
low. By kind, 178 are duplication, 55 dead code, 55 stale comments, 40
inconsistent idioms, 32 error handling, 31 latent bugs, 27 docs drift, 22
naming, 19 data in code and 15 vacuous checks.

The guardrails from [AGENTS.md](../../AGENTS.md) applied. No finding holds the
game to Labrador's T11, and none was raised. Engine changes go to the ledger.
Tuning values are out of scope. So are file splits, which the
[long-file audit](../code-hygiene.md#long-file-audit-2026-11-05) settled the same
day. Line numbers are as of `8a04c998`.

## What is sound

- **The OOP shape is intact.** No reviewer found a hierarchy that needs to be
  undone, and none was proposed.
- **The engine-free contracts still include only the standard library and each
  other.** The findings in `game/contracts/` are about values inside them.
- **The rename is nearly complete.** Outside the frozen reviews, the only old
  name left is `"artattack"` in `vcpkg.json`. No `mattmath::` or ChromaClash
  identifier remains.
- **No TODO, FIXME or HACK markers** in `game/`, `editor/` or `scripts/`.
- **Includes are written from the repository root** throughout. The only drift
  is four quoted rapidjson includes.
- **The acceptance infrastructure holds.** Each case is declared once in
  `cases.psd1`. Probes share `acceptance::require` and `run_probe`. The
  selection and process checkers cover the runner's rules.
- **Content fails loudly at startup.** The tuning reader rejects unknown keys.
  Locales, stage names and font coverage are checked before play.
- **Saves are written atomically**, and an unreadable save is kept aside
  (`HYG#22`). The one gap is below.
- **Player-facing copy is almost all in `presentation.json`.** It uses token
  templates and font-coverage checks. Only a handful of strings leak into C++.
- **Debug code is gated by `_DEBUG` alone**, with no layout-dependent members,
  and the Release RTTI check enforces it.
- **The earlier rows are in effect.** Most new findings are leftovers at the
  edges of finished work (stale `srand` calls, dead includes, unread fields)
  rather than regressions.
- **The CI design is sound:** one required gate, passed-tree reuse and the
  engine-pin check. The findings there are housekeeping.

## Findings by theme

### 1. Defects

- **Controls percentages are validated on the wrong rows.**
  `presentation_validation_settings.cpp:651` fit-checks rows `{1, 2, 3, 4}`. Music
  and SFX moved to rows 4 and 5 (`controls_settings_menu.cpp:91-94`) when Keys,
  Pad Buttons and Profiles were inserted. So the SFX value is never checked, and
  the comment still says "rows two and three". The painter verifier
  `verify-percentage-text.ps1:26` checks rows 4 and 5 but names rows 3/4 and 2/3
  in its messages.
- **A save that exists but cannot be opened is treated as missing.**
  `Save::file_exists` (`save.cpp:422`) returns false when `_wfopen_s` fails for any
  reason, such as a locked file or denied access. `load_save_file` then writes
  defaults over it without calling `keep_unreadable_save`.
- **An unreadable legacy save is dropped silently** (`save.cpp:447`), unlike every
  other read failure.
- **Controls Apply saves a stale snapshot.** `ControlsSettingsMenu::apply_settings`
  (`controls_settings_menu.cpp:189`) starts from the `PlaySettings` taken at
  `init()`. It back-fills a hand-kept list of the child pages' fields, so a new
  field set by a child page would be reverted.
  `StickSettingsMenu::apply_settings` merges onto the current settings instead.
- **Hand-written equality.** `PlayerProfile::operator==` (`player_profiles.h:25`)
  and the probe's `same_input` compare field lists that will miss a new field.

### 2. Checks that cannot fail or check the wrong thing

Fifteen acceptance checks and verifiers would pass whatever happened:

- `bot_driver.cpp:260`: the second mover-hold `require` cannot be reached, and the
  first reports the wrong failure.
- `menu_navigation.cpp:313`: the probe's own substitution failure is caught and
  counted as a `GameTuning` rejection.
- `debug_background_checks.h:479`: `native_motor_calls` is a hard-coded 0.
- `countdown_checks.h:86`: the three-player case uses the four-player layout.
- `crossfeed_checks.h:78`: the fixture has drifted from `LevelBuilder`. It omits
  the gate and ferry cues.
- `script_checks.cpp:433`: the shipped-script lint never resolves `tune` paths or
  weapon names, so a renamed key in a `.cfg` passes.
- `session_checks.cpp:215`: the shipping Session copy is checked only in Debug.
- `align_origin_checks.h:9`: computes a command id as `grid_size + 1`.
- `check-level-editor-save-guidance.ps1:76`: drives menus by position, and its
  "explicit refresh" step toggles Blockout aids.
- `check-acceptance-selection.ps1:116`: three receipt flags are constants.
- `prepare_review.py:67`: compares most masters' hashes with themselves.

### 3. Status text that outlived the 2026-11-05 decisions

Code comments, shipped content and docs still describe rows settled on
2026-11-05 as open trials or pending reviews (`POL#23`, `POL#27`, `POL#30`,
`POL#36`, `ROAD#1`-`ROAD#3`, `ROAD#5`, `ROAD#10`, `ED#5`, `ENG#28`). Examples:
`tuning.h:261`, the `_comment`s in `tuning.json` (`:29`, `:187`) and
`prepare_review.py:50`. The last one generates the shipped `PROVENANCE.md`, which
says integration and runtime acceptance are pending. `architecture.md:182` says
report 28 is open again; it was declined. Separately, about seventeen comments
and doc lines have drifted from the code. The worst is `game_data.h:15`, which
says `GameData` is shared with objects. That is the opposite of the
architecture rule.

### 4. Rules written in several places

- **File writing.** Six temp-then-rename writers have drifted: fixed `.tmp`
  names, different durability flags and different cleanup
  (`debug_files.cpp:25`, `tune_commands.cpp:52`, `save.cpp`, `fatal_report.cpp`,
  `editor/stage_registration.cpp:516`). `replay.cpp:1027` truncates in place. The
  screenshot writer leaves partial PNGs. The debug file-stem rule exists in five
  copies that disagree on reserved names; `perf.capture` accepts `CON`.
- **The match domain.** Human count to `ScreenLayout` appears in about six copies.
  Bot difficulty names are in five tables. Best-of 1/3/5 is written three times,
  the session round count four times and the `SeriesStageRule` list by hand. The
  four-player limit is a literal (`match_setup.cpp:29`, `replay.cpp:519`).
- **The fixed slab.** The footprint 160/80/24 is written as literals in the
  contract, the runtime and the editor (`fixed_slab_paint_geometry.h:27`,
  `fixed_slab_paintable.cpp:16`, `level_object_builder.cpp:557`,
  `slab_authoring.cpp:44`), although `canonical_slab_paint_polygon` accepts only
  that span.
- **Player predicates.** "Grounded", "flat ground" and "can act" are spelled out
  at about twenty sites, because `Player::is_on_ground` is private
  (`player.h:557`, `player.cpp:575`).
- **Settings bounds.** The 90% deadzone cap, the 5% step, sensitivity 1-5 and
  volume 0-100 are written in several places. The save readers drop out-of-range
  values while `normalized_deadzone` clamps them.
- **The editor and the level format.** The editor keeps its own material list,
  scenery colours, punctuation table, face names, stage-name rule and level-format
  key literals (`level_json.cpp:140-493`, `horizontal_authoring.cpp:96`,
  `stage_registration.cpp:106`), although `HYG#46` made `level_format` their owner.
- **Fixed arrays.** Arrays sized by `pad_count` or `TerrainRole::count` are filled
  from four-element literal lists. `was_dead` is a fixed `std::array<bool, 4>`
  indexed by participant (`level.cpp:368`).

### 5. Presentation and content data

- The reader and validators duplicate their helpers. The glyph pen walk, three
  settings-row lambdas, about eight finite and overlap predicates, plaque
  contrast and `disabled_ink` are each written more than once
  (`presentation_validation_settings.cpp:193`, `:479`;
  `presentation_read_match.cpp:347`, `:669`). Limits are hard-coded in messages
  instead of formatted from `menu_geometry`. Some errors name the wrong section.
- Keys mislead. The root comment says every key is required, but `camera.*` and
  `stage_scan` are optional, and a misspelt optional key is silently ignored
  (`presentation.json:6`). `menu_style.box_size` is read and never used.
  `options_value_colour` and `hud.player_identity.background_colour` style other
  screens than their names say. About 25 `sprite_sheet_1` frames are unreferenced.
- Some player-facing copy is still in C++: the `P1`-`P4` labels, the record-line
  word order, the `OptionRow` placeholder `L"null"`, a measured English word on
  the Sticks page and a prompt-rewrite table only the Debug menu reaches. Two
  impossible states draw `ERROR` or `Error` on screen instead of failing
  (`main_menu_stage_select.cpp:313`, `pause_menu.cpp:551`). Seven throws use the
  MSVC-only `std::exception(const char*)`.

### 6. Dead code

Write-only and unused code: `rotation_origin` is authored in `tuning.json` and
parsed, then never read (`projectile_details.h:10`). `Player::primary_type_`,
`HorizontalCarryStep::current_top` and `HorizontalPaintImpact::global_entry` are
written and never read. `Weapon::nozzle_size` and other accessors have no
callers. `projectile_name`, `to_projectile_type` and
`PaintTile::draw_surface_rigid` are unused. So are the constructor's
`audio_resources` and `build_projectiles`' trailing parameters, ten stale
includes in `level.h` and `MenuPage::calculate_center_position`. Also unused:
`hook_frames_`, `MatchBotLocomotion::height_across`, `BotSeatView::step` and
several probe members, locals and includes. `turbulence_slab_stage/tools/register.cpp` is a one-shot
tool that nothing builds.

### 7. Probe and script fixtures copied with drift

- About twenty copies of `LOCALAPPDATA` save isolation, with drifted cleanup. The
  private save path is spelled out 64 times.
- About twenty probe Worlds hand-copy the manifest's sprite sheets and fonts.
- Stage lists are hand-picked under claims like "every authored sound"
  (`story_audio.cpp:163`) and "all authored stage names"
  (`countdown_checks.h:423`), and they leave out stages. Private manifest readers
  have drifted from `acceptance::manifest_catalog`.
- Bot-route and course fixtures are copied across the bot probes, `crossfeed`,
  `cq_blockout` and `press_tower`. Six `Course` fixtures restate `Level`'s tick
  order.
- `painter_visual` copies a 90-line tune-colour scaffold about twenty times and a
  shifted-font fixture nine times. Its `*_fixed_style` fingerprints list fields by
  hand.
- About forty painter verifiers hand-roll PPM, number, Save-line and CSV checks.
  One has lost its payload-length check (`verify-feedback-tune-colours.ps1:30`).
  Six scripts copy the probe compile pipeline, which has drifted
  (`check-movement-feedback.ps1:33`).
- Probes still call `srand` before builds that have not read `rand()` since
  `HYG#39`.
- Debug `.cfg` scripts are byte-identical copies, and `station_tour` claims every
  station but skips two.

### 8. Smaller themes

- Menu pages repeat fit-to-box (about seven copies), paper-bounds checks,
  wrap-around cycling and close handling.
- Drawings repeat the paper card, keyed blocks, the wet band and shadow tiles.
  `splitmix64` is hand-written twice.
- Weapon and projectile code duplicates one-shot sound overrides, team mappings
  and opening timelines. `TerrainMaterial::make` treats any unknown material as a
  paint tin (`terrain_material.cpp:123`).
- The bots copy `crosses_interior` and the held-jump step count. The simulation
  step is written three ways with two values (`simulation.h:9`).
- Debug console messages are wrong in places. The screenshot message names a
  file that is not written. `reload` reads each file twice. Commands are
  registered four ways.
- Local copies of `labrador::narrow`, `widen` and `path_to_utf8`. `path.string()`
  throws on non-ANSI paths in the editor.
- Editor: File > New has drifted from `template.json`, and the palette and snap
  tables are parallel arrays.
- Diagnostics: a game-owned exception is classified by `strcmp` on its message
  (`fatal_report.cpp:113`, separate from report 30's engine message). The spawn
  diagnostic says players are "retired"; they are not.
- Lifetime: `DisplaySettingsChoice` is uninitialised. `Editor`, `ScreenMusic` and
  `ResumeAudio` are copyable resource owners. A null guard checks `save()` and
  then dereferences `settings_session()`.
- Names that mislead: `KeyboardJumpMenu` serves all keys, and
  `match_owns_pad_slot` covers the keyboard. `MenuInputAction` lives in `states/`
  and drags the input layer there.
- The convention rule and the code disagree. AGENTS.md reserves `SCREAMING_SNAKE`
  for macros, but about 59 constants use it.
- Build and CI: `vcpkg.json` is still named `artattack` and installs the unused
  doctest. The pre-build audio test hides every later check when it fails. The
  runtime-asset rule exists three times. Other leftovers: dead `testPresets`, a
  `review-*` trigger and a duplicate `out/` ignore.
- Art and audio tools: the lineup and icon hard-code 5 of 11 team colours. Three
  WAV writers clip differently. Study B's tie-break can raise `TypeError`.

### Engine report owed

One, drafted in the ledger, not here: `labrador::AudioResources` has no
non-throwing sound-group lookup, so `sound_groups.cpp:21` catches
`std::out_of_range` as control flow. Checked against Labrador master `5cbc6b3`
the same day: only the throwing `resolve_sound_group` exists.

## Refuted

Thirty-three findings did not survive. Those worth recording, so a later pass
does not raise them again:

- *The HUD identity separator and the results `A`/`B` marks are copy in C++.*
  Both are recorded choices. `presentation_match.h:211-213` keeps the separator
  and line break in code, and `docs/menu-presentation.md` treats the A/B marks as
  canonical team identities, like digits.
- *`#define private public` in `movement_feedback_visual.cpp` should go.* It is
  deliberate, commented, and the public accessors do not cover what it reads.
- *The crossfeed trial-count and exchange-ledger requires are vacuous.* The ledger
  is not built from the same deltas, and the counts guard future early exits.
- *Pad-reader guards in `check-painter.ps1` can never fire.* They are drift
  assertions.
- *`game/carried/` keeps API no client calls.* The folder keeps the engine
  types' shape unchanged by documented policy, and that did not lapse at the
  pin bump.
- *Painter selector names are spelled out four times.* Already ruled on in the
  long-file audit.
- *`using namespace` in `level_document_impl.h`.* The done `HYG#54` row records
  it as deliberate in the editor's private header.
- *The window title should read "ColourWars".* "Colour Wars" is the display
  name, as on the title screen's wordmark. Only its absence from the
  localisation list stands, under `HYG#69`.
