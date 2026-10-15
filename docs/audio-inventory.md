# Audio inventory and sound direction

Status: inventory taken on 2026-10-14 at `2f052b9`, for
[polish plan §5](polish-plan.md#5-replace-the-entire-audio-set) and TODO row
`POL#20`. The author accepted the sound direction in
[section 5](#5-sound-direction-accepted) on 2026-10-14; their other answers are in
[section 6.3](#63-decisions-and-what-is-still-open). The complete candidate set
produced later the same day is recorded in [section 7](#7-pol22-production-candidates).
The author accepted the complete set on 2026-10-14; that response, rather than
the technical checks, closes POL#22. The 2026-10-15
[runtime migration](#9-runtime-migration-and-remaining-integration) replaces the
existing game bindings and closes STORY#1, STORY#2 and POL#26. POL#23 retains
the remaining new cue hooks and gameplay mix; ROAD#3 retains actual listening
and release acceptance.

This page lists every legacy wave, bank entry, loop instance and cue reference,
and gives each one a replace-or-retire entry. It then lists every cue the
finished game needs, proposes a sound direction, and gives the provenance record
that each replacement asset must carry. `POL#22` produces the set and `POL#23`
integrates it. Both work from this page.

**Sections 1–3 preserve the pre-migration inventory and its file/line references.**
Their dispositions and traps describe the legacy set; section 9 records what
is now integrated and what remains. They are not directions to revive old files.

The legacy set cannot be shipped. The earlier [NOTICE](../NOTICE) said four of
the twenty-four waves are commercial music tracks. None of the twenty remaining
waves has a recorded source or licence either (see
[provenance](#24-provenance-of-the-legacy-set)). Polish plan §5 (lines 412-415)
accepts no existing track, sample, loop or cue as final content, so every row
below retires the legacy item. The only open question for each one is what
replaces it.

At inventory time the local waves were at `game/content/sounds/sound_bank_1/`,
with `sound_bank_1.xwb` beside the definition; both were git-ignored.
Durations, formats and levels below were read from those local files. No file
was copied into the replacement set. The legacy bank is no longer a runtime or
packaging input.

## 1. The legacy bank in one paragraph

`game/content/sounds/sound_bank_1.json` lists 24 waves (lines 4-27). It sets
`create_effect_instance_for_each_wave` (line 29), which gives 24 effect
instances named after the waves, and adds 24 named loop instances (lines
30-127). The engine reads all of these at
`external/labrador/engine/assets/sound_bank_loader.cpp:50-97`. The source
folder holds exactly the 24 listed waves, plus `XWBTool.exe` and `cmd.txt`.
The built bank has 24 named entries in the same order. Each entry is
uncompressed PCM and is byte-identical to its source's `data` chunk, so the
bank is the same size as its sources: 154,388,016 B against 154,386,974 B. The
bank has no loop regions, and no source has a `smpl` loop chunk. Five songs
account for 147 MB of the bank. Every wave is 16-bit stereo 44.1 kHz except
`Whoosh2`, which is mono 48 kHz. Peak levels range from -23.2 dBFS to 0.0
dBFS, and data volumes as low as 0.03 make up for the difference.

## 2. Legacy inventory: replace or retire

**Kind** is how the runtime uses the item. **W** is a fire-and-forget wave
(`play_wave`, which returns no handle and cannot be paused or stopped,
`external/labrador/engine/audio/sound_bank.h:123-126`). **E** is a single named
effect voice. **L** is an effect played looped. "Replace with" names a new cue
from [section 4](#4-the-new-cue-list). In every row the legacy wave itself is
retired.

### 2.1 Bank waves

| # | Wave (`sound_bank_1.json` line) | Source format, length | Every runtime use | Kind | Disposition |
|---|---|---|---|---|---|
| 0 | `BowlingBallRoll` (:4) | PCM 16/2/44.1k, 0.930 s, peak -23.2 dBFS | Roller loop: instances `roller_a0..b3` (:64-95), named in `tuning.json:168-169`. It is also `weapons.roller.shoot_sound` (`tuning.json:166`), but that name is never played; see [trap 1](#33-traps-a-replacement-must-know) | L | Replace with `roller.loop`, `roller.start`, `roller.stop` |
| 1 | `caramelldansen` (:5) | 174.2 s, 30.7 MB, 717 ms trailing below -60 dBFS | Turbulence music, `levels/turbulence.json:7` | L (music gain) | Retire (commercial track). Replace with `music.turbulence` |
| 2 | `CartoonBoing` (:6) | 1.007 s | `player.sounds.jump` (`tuning.json:102`), played at `player.cpp:742` | W | Replace with `player.jump` |
| 3 | `DeathImpactYellSingle` (:7) | 0.569 s | `player.sounds.death` (`tuning.json:101`), played on the alive-to-dead edge at `player.cpp:467` | W | Replace with `player.pop` (`STORY#1`). Map it to `death`, not to `respawn`: `backstory.md:78` puts the pop "as the paint comes off", and `TODO.md:36` calls it the "Respawn pop" only loosely |
| 4 | `dragostea_din_tei` (:8) | 214.1 s, 37.8 MB | King of the Hill music, `levels/king_of_the_hill.json:7`. It is also the music for three unloaded stages: `levels/unused/base.json:6`, `levels/unused/structure_test_old.json:6` and `levels/unused/test_1.json:6` | L (music) | Retire (commercial track). Replace with `music.king_of_the_hill`. Remove the three unused references, or the stages themselves (`GAP#8.6`) |
| 5 | `DrumRoll` (:9) | 4.888 s | `results_menu.sounds.fill` (`presentation.json:192`). Looped at `results_menu.cpp:319` only when no reveal ran; stopped at :39 and :63 | L | Replace with `results.fill`. That path runs only in solo practice or debug ([trap 4](#33-traps-a-replacement-must-know)) |
| 6 | `holly_dolly` (:10) | 174.5 s, 30.8 MB | Close Quarters music, `levels/close_quarters.json:7`. Movement Course music at volume 0.0 until 2026-10-14, when the stage became silent. The editor's new-level default, `editor/level_document.cpp:212` | L (music) | Retire (commercial track). Replace with `music.close_quarters`. Change the editor default in the same commit |
| 7 | `LaserGun` (:11) | 0.307 s | `weapons.bucket.shoot_sound` (`tuning.json:204`), played once per throw at `weapon_bucket.cpp:29-35` | W | Replace with `bucket.throw` |
| 8 | `party_horn` (:12) | 1.045 s | `results_menu.sounds.winner` (`presentation.json:191`), played once at `results_menu.cpp:64`, and on a draw too | W | Replace with `results.winner` and `results.draw` |
| 9 | `slide_whistle_up` (:13) | 1.992 s | `match.sounds.zoom_out` (`tuning.json:28`). Started at time-up (`level.cpp:194-195`), hard-stopped when `match.zoom_out` (2.0 s, `tuning.json:5`) ends (`level.cpp:207`). Paused and resumed with the level (`level.cpp:107-122`) | E | Replace with `match.time_up` and `match.glass_out` |
| 10 | `smash_countdown` (:14) | 5.375 s | `match.sounds.countdown` (`tuning.json:27`). Played once when the countdown starts (`level.cpp:155-156`) | W | Retire. The name suggests audio taken from another game, and the wave is longer than the 3.0 s countdown (`tuning.json:4`). Replace with `match.count_beat` and `match.go` |
| 11 | `SplittingFireWood` (:15) | 0.124 s | `player.sounds.damage` (`tuning.json:100`), throttled by `damage_sound_delay` 0.25 s (`tuning.json:97`), played at `player.cpp:445` | W | Replace with `player.hurt` |
| 12 | `Steamboat` (:16) | 122.7 s, 21.6 MB | **None.** It was the level music until `64c501b` (2023-12-23). Only the stale cue sheet names it (`sounds/sound list.txt:48`, `sound_bank_1/cmd.txt:9`) | none | Retire with no replacement. Delete it from `waves` so the recipe stops compiling it (`cmake/build_wave_bank.cmake:49-56`) |
| 13 | `ThunderCrack` (:17) | 3.347 s | `player.sounds.respawn` (`tuning.json:103`), played at `player.cpp:949` | W | Replace with `player.respawn` (and `player.redraw`, new) |
| 14 | `Top_Of_The_Morning` (:18) | 148.0 s, 26.1 MB, peaks at 0.0 dBFS | `main_menu.music` at volume 0.03 (`presentation.json:111`). Looped by `MenuPage::play_music` (`menu_page.cpp:294-298`) from every main-menu page's `init()`, and stopped at `main_menu_stage_select.cpp:56` and, for a debug start, `main_menu.cpp:22` | L (music) | Retire. Its source is unknown, and it may be the fourth commercial track. Replace with `music.menu` |
| 15 | `UI_Clicks01` (:19) | 0.122 s | `menu_style.sounds.confirm` (`presentation.json:50`); `pause_menu.window_open_sound` (`presentation.json:145`); `match.sounds.click` (`tuning.json:30`, reveal click at `level_intro.cpp:360`) | W | Replace with three cues: `ui.confirm`, `ui.pause_open`, `match.reveal_click` |
| 16 | `UI_Clicks14` (:20) | 0.179 s | `menu_style.sounds.direction` (`presentation.json:49`); `match.sounds.hover` at 0.6 (`tuning.json:29`, reveal hover at `level_intro.cpp:356`) | W | Replace with two cues: `ui.move` and `match.reveal_hover` |
| 17 | `UI_Clicks17` (:21) | 0.186 s | `menu_style.sounds.cancel` (`presentation.json:51`) | W | Replace with `ui.cancel` |
| 18 | `UIerror2` (:22) | 0.643 s. Embedded author: John Lencioni, 2002 | `main_menu.sounds.error` (`presentation.json:113`), played at `main_menu_team_select.cpp:59` | W | Replace with `ui.error` |
| 19 | `VulcanMachineGunSeries` (:23) | 6.788 s | Mister loop: instances `mister_a0..b3` (:96-127), named in `tuning.json:188-189`. It is also `weapons.mister.shoot_sound` (`tuning.json:184`), which is never played | L | Replace with `mister.loop`, `mister.start`, `mister.stop` |
| 20 | `WaterSplashing` (:24) | 0.166 s | `paint.splash_sound` at 0.1 (`tuning.json:75`), played at `structure_paintable.cpp:109-113`, at most once per projectile contact per structure, with no global limit | W | Replace with `paint.splash`: 4-6 variants and a rate limit |
| 21 | `WaterSprayIntoBucket` (:25) | 13.306 s, peak -18.6 dBFS | Sprayer loop: instances `sprayer_a0..b3` (:32-63), named in `tuning.json:136-137`. It is also `weapons.sprayer.shoot_sound` (`tuning.json:132`), which is never played | L | Replace with `sprayer.loop`, `sprayer.start`, `sprayer.stop` |
| 22 | `Whoosh2` (:26) | **mono 48 kHz**, 0.277 s. Embedded author: Peter Comley, 2005 | `main_menu.sounds.ready` (`presentation.json:114`), played at `main_menu_players.cpp:287` and `main_menu_stage_select.cpp:70` | W | Replace with `ui.ready` |
| 23 | `WoodRattle` (:27) | 0.624 s | `weapons.sniper.shoot_sound` (`tuning.json:152`), played once per shot at `weapon_sniper.cpp:30-37` | W | Replace with `sniper.shot` |

### 2.2 Effect instances

| Item | Where | Use | Disposition |
|---|---|---|---|
| 24 per-wave instances from `create_effect_instance_for_each_wave` | `sound_bank_1.json:29`; created at `sound_bank_loader.cpp:60-67` | Only six are ever resolved as effects: `slide_whistle_up`, `Top_Of_The_Morning`, `DrumRoll`, `holly_dolly`, `dragostea_din_tei` and `caramelldansen`. The other 18 are never used | Retire the flag. List each new music, loop and stoppable cue as a named instance |
| `sprayer_a0..a3`, `sprayer_b0..b3` | `sound_bank_1.json:32-63`; `tuning.json:136-137` | One private loop voice per player | Replace: rebuild against `sprayer.loop` |
| `roller_a0..b3` | `:64-95`; `tuning.json:168-169` | as above | Replace: rebuild against `roller.loop` |
| `mister_a0..b3` | `:96-127`; `tuning.json:188-189` | as above | Replace: rebuild against `mister.loop` |

Only four of each weapon's eight instances can sound in one match.
`sound_index` is the view index (`player_builder.cpp:52, 82`), which is unique
across both teams, so view *i* uses `_a<i>` or `_b<i>` and never both. Four per
weapon would be enough. Changing that is a schema decision, because
`tuning.cpp:311-328` requires exactly four per team.

### 2.3 Legacy files and names outside the live bank

| Item | Where | What it is | Disposition |
|---|---|---|---|
| `unused/wave_bank_1.json` | `game/content/sounds/unused/` | An older six-wave definition (`UIerror2`, `UI_Clicks14`, `UI_Clicks17`, `UI_Clicks01`, `Top_Of_The_Morning`, `Whoosh2`, lines 4-9) and a lowercase instance, `top_of_the_morning` (:13-19), whose `volume`, `pitch` and `pan` fields the current loader ignores. Nothing references it, but the build stages it beside the exe. The folder holds no waves | Retire (delete). That stops it being revived as a second bank. Part of `GAP#8.6` |
| `sound list.txt` | `game/content/sounds/` | The 2023 cue sheet. It names `WoodPlankFlicks` (:22) as the fill, though the shipping fill is `DrumRoll`, and it has placeholders `Title ?????` (:10) and `WINDOW_OPEN_SOUND ????` (:15). It lists `Steamboat` (:48) and sorts the weapons into looping and single-shot (:52-59). Staged beside the exe | Retire (delete). This page supersedes it |
| `WoodPlankFlicks` | `sound list.txt:22` only | A cue name with no wave anywhere | Retire. `results.fill` covers the role |
| `cmd.txt` | `game/content/sounds/sound_bank_1/` | XWBTool usage notes naming the absent `Explo1.wav` and `Explo2.wav` (:1-4), and a hand-written 24-wave build line (:9) that would rebuild the legacy bank | Retire (delete). `cmake/build_wave_bank.cmake` is the recipe |
| `XWBTool.exe` | `game/content/sounds/sound_bank_1/` (tracked; MIT, `NOTICE:31`) | The bank compiler | Keep, unless `POL#22` changes the tool. It moves with the bank folder if the bank is renamed |
| `sound_bank_1.xwb`, local and staged | `game/content/sounds/`, `out/build/x64-debug/game/sounds/`, `out/build/x64-release/game/sounds/` (all identical) | The built legacy bank | Retire. Delete all three before the first replacement build, because when sources are missing the recipe reuses a bank that is not older than its definition (`build_wave_bank.cmake:17-23`) |
| Music in `levels/unused/` | `base.json:6`, `structure_test_old.json:6`, `test_1.json:6`. `level_0.json` has no `sound` block and could not load | Unloaded stages | Retire with the stages, or point them at the new music (`GAP#8.6`) |
| Movement Course placeholder note | `levels/movement_course.json:6` | Said the music was muted pending replacement | Retired 2026-10-14: the note now says the stage is deliberately silent |
| Review fixtures named after the legacy set | `out/review-2026-09-26-data/` (git-ignored, `.gitignore:374`): 24 placeholder waves of 8,864 B each under `authored/sounds/sound_bank_1/`, a 213,460 B `package/ColourWars-windows-x64/sounds/sound_bank_1.xwb`, a 22 B `bank-fix-trlmi5xc/stage/another_private_bank.xwb`, and five `sound_bank_1.json` files (two of 1,821 B, two of 1,802 B and a 28 B stub) | Synthetic stand-ins from the 2026-09-26 review's packaging checks. They are not the legacy audio (the `.xwb` differs from the real bank from byte 25), but they carry the legacy file names | Retire (delete the folder) before `POL#23`, so that no stray file named `sound_bank_1.xwb` or `WaterSplashing.wav` can be mistaken for, or copied into, the replacement set |
| Release ZIPs already built | `out/build/x64-release/packages/ColourWars-windows-x64.zip` and the `out/validation/*/…/ColourWars-windows-x64/sounds/` trees | Checked: they hold only `sound_bank_1.json`, no `.xwb` or `.wav` | Nothing to retire. They are rebuilt by `POL#23`; the CI ZIP check (`ci.yml:90-92`) keeps guarding them until it is changed |

### 2.4 Provenance of the legacy set

None of this is verified. It is recorded so that the NOTICE revision in
`POL#23` can be accurate.

- **Commercial songs.** `caramelldansen`, `dragostea_din_tei` and `holly_dolly`
  are named after well-known commercial songs. NOTICE says there are four
  commercial tracks, but it does not say whether the fourth is `Steamboat` or
  `Top_Of_The_Morning`, and the author does not know (2026-10-14). Both are
  retired either way, so NOTICE can keep the count without naming the fourth.
- **Countdown.** `smash_countdown` is probably audio taken from a game. NOTICE's
  "four music tracks" statement does not cover it.
- **Embedded authors.** Two waves name one: `UIerror2` (John Lencioni, Sound
  Forge 4.5, 2002) and `Whoosh2` (Peter Comley, Sony Sound Forge 8.0, 2005).
- **The rest.** The other sound effects come in three naming styles: CamelCase
  descriptive names, a numbered `UI_ClicksNN` pack, and snake_case
  `party_horn`/`slide_whistle_up`. That points to at least three unrecorded
  sources.
- **NOTICE sizes.** NOTICE:51-52 and README.md:309-310 give the sources as
  148 MB and the bank as 154 MB. Both are about 154 MB (147 MiB), because the
  bank is raw PCM. Correct the figures when the text is revised.

### 2.5 How completeness was checked

Every name in `waves`, every `"wave"` and `"name"` in `sound_effect_instances`
(24 waves, 24 instances, each instance on one of three waves), every `.wav`,
`.xwb`, `.mp3`, `.ogg` and `.xwm` in the tree (outside `.git` and the engine's
own samples and tests), and every hit for the 24 wave names, `sound_bank_1`,
`play_wave`, `play_effect`, `stop_effect`, `pause_effect`, `resume_effect`,
`set_effect_volume`, `resolve_wave`, `resolve_effect`, `play_music`,
`music_gain` and `sfx_gain` across `game/`, `editor/`, `scripts/`, `cmake/`,
`.github/`, the content JSON and the top-level and `docs/` text was matched
against sections 2 and 3. The engine's own sample and test banks
(`external/labrador/samples/audio/`, `external/labrador/tests/audio/`) belong to
Labrador and are out of scope.

## 3. The switchboard: where cue names live

A replacement has to update every site in this section. No C++ file selects a
wave by literal name. Data chooses every cue, and the one exception is the
editor default.

### 3.1 Data keys

| File | Key (line) | Resolved as | Resolved at | Validated at |
|---|---|---|---|---|
| `tuning.json` | `match.sounds.countdown` (:27) | effect, in **the level's** bank (since 2026-10-15) | `level.cpp` | `tuning.cpp:534`, once per stage (`content_validation.cpp:33-34`) |
| | `match.sounds.zoom_out` (:28) | effect | `level.cpp:65-66` | `tuning.cpp:535` |
| | `match.sounds.hover` (:29), `click` (:30) | wave | `level.cpp:80-81`, only when the opening runs | `tuning.cpp:536-537` |
| | `paint.sound_bank`, `paint.splash_sounds`, `capture_sound`, `dud_sound`, `impact_sounds` (2026-10-15) | waves | Level-owned `PaintAudio`; surfaces report contact results | `GameTuning::validate_resources`, every variant and projectile kind |
| | `player.sound_bank` (:98), `player.sounds.*` (:100-103) | wave | `player.cpp:62-71` | `tuning.cpp:468-475` |
| | `weapons.<w>.sound_bank` (:131, :151, :165, :183, :203) | bank | `weapon.cpp:49` | |
| | `weapons.<w>.shoot_sound` (:132, :152, :166, :184, :204) | wave | `weapon.cpp:52-53` | `tuning.cpp:498` |
| | `weapons.<w>.loop_sounds.team_a/team_b` (:136-137, :168-169, :188-189) | effect | `weapon.cpp:50-51, 324-344` | `tuning.cpp:500-509` |
| `presentation.json` | `menu_style.sound_bank` (:47) | bank for all menus | `menu_page.cpp:11` | |
| | `menu_style.sounds.direction/confirm/cancel` (:49-51) | wave | `menu_page.cpp:16-18` | `presentation.cpp:842-844` |
| | `main_menu.music` (:111) | effect | `main_menu.cpp:17`; also by name at `controls_settings_menu.cpp:91-96` | `presentation.cpp:847` |
| | `main_menu.sounds.error/ready` (:113-114) | wave | `main_menu.cpp:15-16` | `presentation.cpp:845-846` |
| | `pause_menu.window_open_sound` (:145) | wave | `pause_menu.cpp:67-68` | `presentation.cpp:848` |
| | `results_menu.sounds.winner/fill` (:191-192) | wave / effect | `results_menu.cpp:15-16` | `presentation.cpp:849-850` |
| `levels/*.json` | `sound.sound_bank_name`, `music_name`, `music_volume` (`close_quarters.json:6-8`, `king_of_the_hill.json:6-8`, `turbulence.json:6-8`; `movement_course.json` names only the bank). `music_name` and `music_volume` are optional as a pair: a stage with neither is silent | effect | read at `level_loaded_info.cpp:255-266`, passed through `level_builder.cpp:97-99`, resolved at `level.cpp:62-64` | `level_loaded_info.cpp:170-179` |
| `sound_bank_1.json` | `waves` (:4-27), `sound_effect_instances` (:30-127) | the bank | `sound_bank_loader.cpp:50-97` | |
| `editor/level_document.cpp:212` | new-level `sound` block (`holly_dolly`, 0.3) | written into new stages | | `editor/level_json.cpp:253-258` checks only that the names are non-empty (`ED#3`) |

Sites that read no cue name of their own but still play or resolve the cues
above:

| Site | What it does with audio | What the replacement must keep in mind |
|---|---|---|
| `game/debug/debug_menu_page.cpp:103, 246, 251, 256, 264` | The Debug-only debug menu (`DebugMenuPage : MenuPage`, `debug_menu_page.h:22`) plays `menu_style` direction, confirm and cancel | `ui.move`, `ui.confirm` and `ui.cancel` are heard here too; nothing to change unless the debug menu wants its own cues |
| `game/states/stage_preview.cpp` and `level_object_builder.cpp` | Stage previews build real `StructurePaintable`s. Since 2026-10-15 surfaces report contact results; Level owns paint audio, so previews resolve no paint voices and play none | Every paint cue is checked at startup; match construction resolves the same choices in `PaintAudio` |
| `game/debug/tune_scope.cpp:26`; `tune_target.cpp:31, 56, 90` | `tuning.match.sounds.*.volume` is live-tunable; a tuned candidate is re-validated against the loaded banks | New `match.sounds.*` slots inherit live tuning. `tune_checks.cpp` (section 3.2) pins the list |
| `game/play_settings.h:8-12`; `save.cpp:79-80, 174-175, 309-310`; `controls_settings_menu.cpp:58-59, 84-97`; `menu_page.cpp:280-298`; every `* preferences_->sfx_gain()` / `music_gain()` in the play calls above | The player's 0-100 music and SFX volumes. Every wave and effect is multiplied by one of the two; Apply re-sets the menu music's volume by name (`controls_settings_menu.cpp:91-96`) | Every new cue must be routed through the right gain (music or SFX). Room tone and stings need a deliberate choice of which. `scripts/acceptance/settings.cpp:88-140` pins the stored values |
| `game/states/game_states.cpp:159-160, 186-203, 270-284, 305, 422-440`; `game/debug/debug_director_hooks.cpp:291, 432, 600-601`; `game_commands.cpp:292`; `replay_commands.cpp:242, 335` | The match's audio lifecycle: the old level is destroyed before the new one is built because its destructor stops music in the shared bank; suspend on cover or focus loss, resume on return; the debug clock's `debug_sync_audio` | Any new looping or stoppable cue (`player.redraw`, `room.tone`, weapon `start`/`stop`) must join `Level::suspend`/`resume`/`pause_audio` (`level.h:72-86`, `level.cpp:91-122`). `scripts/acceptance/debug_console/director_checks.cpp:97-106` counts the syncs |
| `game/main.cpp:107`; `content_validation.cpp:9-34`; `resource_validation.h:37-47` | Startup validation of every name above against the loaded banks | Unchanged in shape; extend it for every new slot (section 4) |

Validation can be fooled in a fresh clone. A silent bank accepts any name
(`external/labrador/engine/audio/sound_bank.cpp:36-44, 60-66`), so a misspelt
cue fails only on a machine that has the real bank.

### 3.2 Build, packaging, checks and documents

| Site | What changes when the bank does |
|---|---|
| `game/content/manifest.json:36-44` | The bank entry, marked `"optional": true` with a comment about undistributable audio (:39) |
| `cmake/build_wave_bank.cmake` | Waves come only from `waves` (:37-56); the bank is built only when every source is present (:58-68); an existing bank not older than the definition is reused when inputs are missing (:17-23); the tool runs as `XWBTool -f -y -o` (:91). An instance whose wave is missing from `waves` is silently added by the loader (`sound_bank_loader.cpp:93-96`, despite the comment at :90-92), so `waves` does not fully control what gets built |
| `game/CMakeLists.txt:180-192, 201-218` | `BANK_NAME=sound_bank_1`; the staging loop also mirrors `sound list.txt` and `unused/` |
| `cmake/stage_wave_bank.cmake:7-17` | Copies the bank, or removes a stale staged copy |
| `cmake/release_assets.cmake:27, 49-54, 70-71` | The release ships only the definition, and the build stops unless every bank is optional |
| `.github/workflows/ci.yml:90-92, 111-118` | Rejects `.xwb`, `.wav` and `.mp3` in the ZIP; the "A clone with no audio still starts" step |
| `scripts/check-startup.ps1:23-25, 185-206`; `check-movement-feedback.ps1:188`; `check-workshop-playable.ps1:178`; `check-workshop-style.ps1:196` | Require the bank to be absent, or accept the exact "no audio" stderr line |
| `scripts/acceptance/*`: `ammo.cpp:103`, `movement.cpp:224`, `movement_course.cpp:1557`, `paint.cpp:39`, `workshop_player.cpp:335`, `workshop_terrain.cpp:154`, `debug_console/replay_checks.cpp:198` | Register `SoundBank::silent()` under the literal `"sound_bank_1"` |
| `scripts/acceptance/debug_console/tune_checks.cpp:100, 207, 234, 261` | Pin which sound keys can be tuned live |
| `.gitignore:369-372` | Ignores `*.wav`, `*.xwb` and `*.mp3`. Change this to suit the chosen source workflow |
| `NOTICE:41-56`, `README.md:255-265, 305-325`, `CLAUDE.md:130` and `AGENTS.md:130` ("Audio: a fresh clone is silent"), `docs/feature-roadmap.md:79, 87`, `docs/backstory.md:152, 158` | Text that describes the legacy set or its absence |
| `README.md:59, 76-83, 203` (`-RequireNoAudio`); `docs/feature-roadmap.md:17, 30, 98-108, 194-196`; `TODO.md:23, 36-37, 88`; `docs/level-editor.md:45, 100`; `docs/oop-guide.md:21, 26, 169-187`; `docs/polish-movement-course.md:14-15`; `docs/debug-console.md:316, 367` | Further text that names the legacy bank (`sound_bank_1.json`, `holly_dolly`), the "death sound", the silent ZIP, or how to add a wave. Revise each in the `POL#23` commit |
| `scripts/acceptance/movement_feedback_visual.cpp:517-518`, `workshop_playable_visual.cpp:317-318`, `workshop_style_visual.cpp:444-445`; `scripts/check-debug-console.ps1:572` | Visual probes set both volumes to 0 and load the real banks; the console check writes a save with both at 100. Unaffected by the bank's contents, listed so a search for audio finds no unexplained site |

### 3.3 Traps a replacement must know

1. **The looping weapons' `shoot_sound.name` is never played.** For the
   sprayer, roller and mister, `tuning.json:132, 166, 184` name a wave that is
   validated and resolved but never played. `Weapon::handle_shoot_sound`
   (`weapon.cpp:279-297`) plays only the loop instance, and only the
   *volume* comes from tuning. You hear the wave named by the instance in the
   bank JSON, so renaming the tuning entry changes nothing, and nothing checks
   that the two names agree. Either make that name `<weapon>.start`, or remove
   it from the schema.
2. **The countdown is still one oversized baked wave.** It is 5.375 s against
   the 3.0 s countdown, so the tail still overlaps live play. `POL#23` replaces
   it with individual beats and GO. Since 2026-10-15 it is a named effect,
   validated as an effect, paused/resumed with the level (including gain
   changes), and stopped when the level ends or is destroyed. Restarting can
   no longer leave an orphan countdown playing over the next match.
3. **The time-up transition now rings out.** Since 2026-10-15 the end of the
   camera zoom no longer hard-stops its effect. It remains pause-aware and
   is stopped when the level is destroyed. `POL#23` still replaces the legacy
   whistle with the time-up/glass cues; the picture stays silent while the
   person is present.
4. **Results after a reveal now play the fanfare.** Since 2026-10-15 the first
   results update plays it once, after the person has left. The fill loop is
   intentionally absent there: the reveal already showed the percentages and
   the results page has no bar-fill animation. Solo/debug without a reveal
   retain their animated bars, fill loop and completion fanfare. The legacy
   fanfare still serves draws too; `POL#23` supplies the distinct draw cue.
5. **The pause-open cue belongs to entering pause.** Since 2026-10-15 returning
   from confirmation (Back, No, or a lost pad during restart confirmation)
   recreates the initial page without replaying the open cue. Back/No still
   play cancel. Opening a new pause session still plays the open cue; the
   controls/audio page returns to the existing pause page.
6. **Menu sounds have no volume field.** Direction, confirm, cancel, error,
   ready and pause-open are plain strings (`presentation.h:71-73, 171-172,
   186`) and always play at 1.0 × SFX. Their gain must be built into the
   wave, or the schema must change (`GAP#8.2`).
7. **A stage needs its `sound` block for the bank, but not music.** Since
   2026-10-14 `music_name` and `music_volume` may be left out together, and the
   stage is silent (`LevelLoadedInfo::has_music`; `Level` leaves `music_`
   unresolved and checks it at every call). The Movement Course is the one
   silent stage. The level editor accepts the same rule.
8. **Things that keep playing when the window loses focus.** Menu music and
   the results fill loop do not pause on focus loss; only minimising does
   (`external/labrador/engine/app/application.cpp:389-403`). In a match,
   focus loss reaches `GameLevel::on_deactivated` (`game_states.cpp:199-203`),
   and `Level::pause_audio` (`level.cpp:107-112`) pauses the music, the
   time-up cue and the weapon loops, but not fire-and-forget waves.
9. **Weapon loops call play every tick.** They call `play_effect` on every
   held tick (`weapon.cpp:286-291`), which reaches `SoundEffectInstance::Play`
   (`external/labrador/engine/audio/xaudio2/audio_device.cpp:179-187`), and
   rely on DirectXTK treating Play on a playing instance as a no-op. That has not been verified here. Every stop is
   a hard `stop_effect(..., true)`, so a loop's tail is cut off.
10. **Nothing limits voices.** The engine has no bus, master gain or voice cap
    (`ENG#6`, `docs/review/engine-reports.md`). Splashes arrive every tick from
    the sprayer and roller, so any limit has to be in the client.
11. **The first 0.25 s is damage-silent.** The damage timer starts at 0 and
    advances only while the player is alive (`player.cpp:486`).
12. **Debug speed does not scale audio** (`game/debug/game_commands.cpp:265`).
    Do listening passes at 1×.
13. **A named effect instance is one voice, shared by every owner.** The
    menu music, each level's music, the time-up cue and the results fill are
    single instances in one bank, so a second `play` restarts the same voice
    and any `stop` silences it for everyone. That is why `GameLevel` destroys
    the old level before building the new one (`game_states.cpp:159-160`). A
    cue that must overlap itself (two players' `player.redraw`, a sting over
    its own tail) needs one instance per voice, as the weapon loops already
    have.

## 4. The new cue list

Every cue the finished game needs. **Tier 1** is the first polished reference
match: Close Quarters, movement, sprayer, roller, UI, match flow and the
backstory beats. **Tier 2** is the remaining weapons, arenas and screens, needed
before final acceptance. **Tier 3** is quiet detail, assessed one cue at a time.
Polish plan §5 (lines 430-431) allows a visual idle accent to stay silent.

The trigger locations and line numbers below were captured during inventory.
The current implementation status is in [section 9](#9-runtime-migration-and-remaining-integration);
the hook column records the work identified before that migration.

**Hook:** *exists* means a play call was already there, so only data changes.
*Slot* means there is an event point in code but no sound call, so a tuning slot
and a `play_wave` call must be added. *Add* means there is no event yet, so new
detection logic is needed. Every new slot must be added to startup validation
(`tuning.h:204-209`, `resource_validation.h:37-47`).

### 4.1 Music

| Id | Trigger | Loop | Hook | Tier | Replaces |
|---|---|---|---|---|---|
| `music.menu` | Main-menu pages | yes | exists: `main_menu.music`; started at `main_menu.cpp:198, 329, 478` and elsewhere | 1 | `Top_Of_The_Morning` |
| `music.close_quarters` | Countdown start (`level.cpp:154`) | yes | exists: level `music_name` | 1 | `holly_dolly` |
| `music.king_of_the_hill` | as above | yes | exists | 2 | `dragostea_din_tei` |
| `music.turbulence` | as above | yes | exists | 2 | `caramelldansen` |
| `music.results` | Results and end screens | yes or a sting | add: `results_menu.cpp` resolves no music (`GAP#5.1`) | 2 | — |
| `music.end_sting` | Time-up, instead of the hard stop at `level.cpp:192` | no | add | 2 | — |

### 4.2 Match flow and the backstory

| Id | Trigger | Loop | Hook | Tier | Replaces |
|---|---|---|---|---|---|
| `match.count_beat` | Each 3-2-1 change (`level.cpp:158-166`) | no | add: one edge per beat, so the sound follows `match.countdown` | 1 | `smash_countdown` |
| `match.go` | `start_timer_ <= 0` (`level.cpp:169-172`) | no | add | 1 | `smash_countdown` |
| `match.time_up` | `timer_ <= 0` (`level.cpp:183-196`) | no | exists (`match.sounds.zoom_out`) | 1 | `slide_whistle_up` |
| `match.glass_out` | The pull back into the monitor | no | exists, the same site. Make it fit 2.0 s or let it ring out (trap 3) | 1 | `slide_whistle_up` |
| `match.glass_in` | The push through the glass (`level_intro.cpp:89-97`) | no | add | 2 | — |
| `match.reveal_hover` | The cursor reaches a team: reveal `level_intro.cpp:354-357`; opening preview, `previewing_a/b` edges at :57-58 | no | exists (reveal); slot (opening, `update_opening` :148-167 needs the before/after pattern) | 1 | `UI_Clicks14` |
| `match.reveal_click` | The winner is clicked (`level_intro.cpp:358-361`); the same instant fills the slot and shows the coverage (`backstory.md:97`, `level_intro.cpp:435-455`), so one cue covers all three | no | exists (`match.sounds.click`) | 1 | `UI_Clicks01` |
| `match.reveal_close` | Draw: the program closes without saving | no | add: the same click fires today (:358) | 2 | — |
| `match.celebrate`, `match.slump` | The reveal's react window (`level_intro.cpp:292-294, 489`) | no | add | 1 | — (`party_horn` in spirit) |
| `match.colour_in` | Figures fill with team colour as the opening ends (`level_intro.cpp:155-162`): the backstory's "figures come alive" beat (`backstory.md:61-62`) | no | slot | 2 | — |
| `match.skip` | Intro skipped (`level_intro.cpp:140-146`) | no | slot | 2 | — |
| `match.undo` | The opening's undo (`level_intro.cpp:51, 62`) | no | add | 2 | — |
| `match.last_ten` | Each second at 10 s or less (`level.cpp:183-184`) | no | add (`GAP#5.4`); must not give away the score | 2 | — |
| `room.sigh` | Opening leave beat, `pose.sigh` (`level_intro.cpp:64-66`) | no | add: a `match.sounds.sigh` slot (`STORY#2`) | 1 | — |
| `room.chair` | `pose.stand` (`level_intro.cpp:67`); reused when the person sits and leaves in the reveal (:320-330) | no | add (`STORY#2`) | 1 | — |
| `room.tone` | While the monitor is shown, before the push and in the reveal | yes | add: it must obey `pause_audio` | 2 | — |
| `room.footsteps` | `pose.leave` (`level_intro.cpp:68`) | no | add | 3 | — |
| `room.breath`, `match.waver`, `match.cheat` | Person settles; the cursor wavers (`level_intro.cpp:339-342`); frozen figures cheat | no | add | 3 | — |

### 4.3 Player and movement

| Id | Trigger | Loop | Hook | Tier | Replaces |
|---|---|---|---|---|---|
| `player.jump` | `Player::start_jump`, every jump kind (`player.cpp:742`) | no | exists | 1 | `CartoonBoing` |
| `player.land` | Airborne to supported (`player.cpp:274-282`); scale by `support_impact_speed_` | no | slot | 1 | — |
| `player.step` | Gait contact while supported and moving | no | add: a distance accumulator in `update_movement` (`player.cpp:564-676`) or contact frames in the run cycle; 4-6 variants, rate-limited | 1 | — |
| `player.skid` | Reversal at speed (`player.cpp:607-616`) | no | slot: threshold plus latch; nothing on small corrections | 1 | — |
| `player.hurt` | Projectile damage (`player.cpp:443-447`) | no | exists | 1 | `SplittingFireWood` |
| `player.pop` | Elimination: the paint comes off (`player.cpp:467`) | no | exists (`player.sounds.death`) | 1 | `DeathImpactYellSingle` (`STORY#1`) |
| `player.redraw` | The respawn redraw, driven from the dead branch (`player.cpp:488-498`) | yes | add: one effect instance per player; must pause with the match | 2 | — |
| `player.respawn` | Back in play (`player.cpp:949`) | no | exists | 1 | `ThunderCrack` |
| `player.step_painted` | Step or landing on painted tiles | no | add: needs a support-tile ownership query | 2 | — |
| `player.bonk`, `player.drop` | Ceiling hit (`player.cpp:361`); drop-through (`player.cpp:653-659`) | no | slot | 3 | — |
| `player.hit_confirm` | The shooter's projectile hurt someone | no | add: there is no shooter-side event. Shared speakers make it ambiguous | 3 | — |

### 4.4 Weapons and paint

| Id | Trigger | Loop | Hook | Tier | Replaces |
|---|---|---|---|---|---|
| `sprayer.loop` | Fire held and affordable | yes | exists (`weapon.cpp:279-297`) | 1 | `WaterSprayIntoBucket` |
| `sprayer.start`, `sprayer.stop` | Loop start and stop edges | no | add: `handle_shoot_sound` keeps no edge state. Stop must not fire on death or teardown (`weapon.cpp:298-305`, `level.cpp:91-96`) | 1 | — |
| `roller.loop` | Fire held | yes | exists | 1 | `BowlingBallRoll` |
| `roller.start`, `roller.stop`, `roller.lift`, `roller.settle` | Edges; leaving and meeting the ground while rolling (`player.cpp:274-299`) | no | add | 1 | — |
| `weapon.empty` | Fire held without enough paint (`weapon.cpp:232-234, 262-263`) | no | slot: an edge latch, rate-limited while held | 1 | — |
| `weapon.recovered` | Affordability returns, or the tank fills (`weapon.cpp:248-256`) | no | slot | 1 | — |
| `mister.loop`, `mister.start`, `mister.stop` | as for the sprayer | loop yes | loop exists; edges add | 2 | `VulcanMachineGunSeries` |
| `sniper.shot` | Shot fired (`weapon_sniper.cpp:30-37`) | no | exists | 2 | `WoodRattle` |
| `sniper.ready` | Cooldown ends while affordable (`POL#14`) | no | slot (`weapon.cpp:232-234`) | 2 | — |
| `bucket.throw` | Throw (`weapon_bucket.cpp:29-36`) | no | exists | 2 | `LaserGun` |
| `paint.splash` | A projectile paints at least one tile (`structure_paintable.cpp:109-113`) | no | exists: add variants and a per-structure or global rate limit | 1 | `WaterSplashing` |
| `paint.capture` | A tile changes team, as opposed to a friendly repaint | no | add: `PaintTile::paint` returns nothing (`paint_tile.cpp:61-70`) | 2 | — |
| `paint.dud` | A projectile retired on bare geometry (`projectile.cpp:266-272`) | no | add | 2 | — |
| `paint.impact.<projectile>` | A splash per projectile kind (`POL#11`) | no | add: splash is in `PaintTuning` (`tuning.h:129-130`), not per projectile | 2 | — |
| `weapon.blocked` | A shot blocked at cover (`projectile.cpp:181-188`) | no | slot (`POL#14`) | 3 | — |
| `bucket.slosh`, `tool.drip` | Movement slosh; idle drip (`POL#12`) | no | add. Silent by default; off under reduce-effects (`POL#13`) | 3 | — |
| `world.ambience.<arena>`, `world.detail` | "Rooms at rest" (polish plan :391): a turning fan, distant machinery, an occasional pipe droplet, per arena | bed yes; details no | add: needs a per-level ambience key beside `music_name` and a pause-aware instance (trap 13). Assess each detail on its own; a visual accent may stay silent | 3 | — |

### 4.5 Menus and between rounds

| Id | Trigger | Loop | Hook | Tier | Replaces |
|---|---|---|---|---|---|
| `ui.move` | Focus or value change: 19 sites, 17 outside the Debug-only menu, e.g. `main_menu.cpp:234`, `pause_menu.cpp:185`, `end_menu.cpp:50`, `controls_settings_menu.cpp:148` | no | exists | 1 | `UI_Clicks14` |
| `ui.confirm` | Proceed or select | no | exists | 1 | `UI_Clicks01` |
| `ui.cancel` | Back or quit | no | exists | 1 | `UI_Clicks17` |
| `ui.error` | Refused, e.g. a one-team roster (`main_menu_team_select.cpp:59`) | no | exists | 1 | `UIerror2` |
| `ui.ready` | A seat joined; a stage marked READY | no | exists. Consider per-seat pitch (`play_wave` takes pitch) | 1 | `Whoosh2` |
| `ui.pause_open` | Pause pushed (`pause_menu.cpp:342`) | no | exists: stop it firing again on returns (trap 5) | 1 | `UI_Clicks01` |
| `ui.unready` | A seat leaves (`main_menu_players.cpp:272-276`) | no | exists as cancel | 3 | — |
| `ui.pad_lost`, `ui.pad_back` | Controller lost or back (`pause_menu.cpp:215-227`) | no | slot | 2 | — |
| `ui.count_in` | A shared count-in after pause or reconnect (`POL#18`) | no | add; reuse `match.count_beat` | 2 | — |
| `results.fill`, `results.winner`, `results.draw` | The non-reveal results path (`results_menu.cpp:60-65, 319`) | fill yes | exists. A draw still plays the fanfare today | 2 | `DrumRoll`, `party_horn` |
| `end.series` | Series complete (`end_menu.cpp:61, 78`) | no | add | 3 | — |
| `ui.volume_preview` | The SFX volume row changes (`controls_settings_menu.cpp:147-153`) | no | slot: today `ui.move` plays at the *old* gain because the setting is unapplied, so the player cannot hear the level being chosen | 3 | — |

Two pieces of mix work have no wave of their own: crossfades and ending stings
in place of `stop_effect(..., true)`, and music ducking under pause instead of
the hard pause. Both need a client music director (`GAP#5.1`).
`set_effect_volume` (`sound_bank.h:136`) is enough to build one.

## 5. Sound direction: accepted

**Accepted by the author on 2026-10-14**, including the rule that the picture is
silent while the person is present. The technical targets in 5.2 are still
proposals for `POL#22` to confirm in production. It follows
the fiction in [backstory.md](backstory.md): a paint program on someone's old
PC, crude mouse-drawn figures, one empty palette slot, and a person seen only as
a bezel, cursor and silhouette.

### 5.1 Two worlds and the glass between them

- **The room** is outside the screen. Real, dry, close foley, slightly muffled:
  the chair, a breath and a sigh, footsteps, a mouse click, a CRT whine or PC
  fan. It is heard only in the opening, the reveal and the draw.
- **The picture** is everything the drawings do.
  - Paint is tactile liquid: wet slaps, drips, pressure hiss, and the roller's
    tacky drum.
  - The figures are mouse drawings, so their bodies sound drawn: a marker dab
    for a footstep, a paper flump for a landing, a sped-up marker scribble for
    the redraw, and a wet bubble pop when the paint comes off.
  - The tools are real mechanisms played slightly cartoonish: the sprayer's
    pump and valve, the roller's drum and creaking handle, the sniper's
    pressure release, the mister's atomiser fizz, and the bucket's slosh and tin
    clank. Each weapon must be identifiable with your eyes shut.
  - The sounds are hand-made, not synthetic. Crude art does not call for
    chiptune (polish plan §5, lines 434-435).
- **The glass.** The countdown push and the time-up pull back are audible
  crossings, such as a low-pass sweep or a soft whump. They replace the slide
  whistle.

**When the person is present, the picture is silent**, the way toys freeze when
a person is in the room (backstory). There is no match music in the opening or
the reveal. Today's code already starts the music at the countdown and stops it
at time-up. The figures' cheating gets only the quietest sounds. The picture's
sound returns with the winners' celebration and the losers' slump after the
click.

**UI sounds belong to the invented program**: short, dry clicks and blips with
their own character. Never use or imitate a real operating system's or
program's sounds. This extends the backstory's rule against tracing a real
program.

**Music** should sound like a small home-made ensemble that fits the crude
drawings: hand percussion, claps, toy or upright piano, ukulele or guitar,
whistle, kazoo-style leads. It should drive the matches and sound like an idle
bedroom desktop on the menu. Each arena gets its own track with a clean loop and
an ending sting. Results get a short loop or sting.

**Mix order**, loudest first: useful action cues (empty, hurt, elimination,
countdown beats), then weapon voices, then movement, then music, then
decorative detail. Decorative cues default to off or nearly off.

### 5.2 Technical targets

| Target | Proposal | Grounds |
|---|---|---|
| Container | One `.xwb` built by XWBTool with entry names (`-f`) | `resolve_wave` needs the names; `build_wave_bank.cmake:91` |
| Encoding | **MS-ADPCM** for the music. PCM or MS-ADPCM for short effects. **Not xWMA** | The game's `xaudio2` backend keeps DirectXTK's reader, which plays ADPCM and xWMA (`external/labrador/engine/audio/xaudio2/audio_device.cpp:25-30`). The engine's own reader, used by `webaudio`, decodes PCM and MS-ADPCM and refuses xWMA and XMA by name (`external/labrador/engine/audio/xwb_file.cpp:228-237`). ADPCM keeps a browser build of the game possible. XWBTool packs the waves it is given, so encoding happens earlier in the pipeline; confirm the encoder in `POL#22` |
| Sample rate | One rate for every wave. Proposal: 44.1 kHz | 23 of the 24 legacy waves use it; `Whoosh2` was the outlier |
| Channels | Mono for effects unless the stereo image matters; stereo for music and room tone | Effects are not panned today (no game call passes pan) |
| Size | Music loops of 60-120 s. Whole bank under about 40 MB | The legacy bank is 154 MB, 147 MB of it songs, which is past GitHub's per-file limit (NOTICE:51-52). MS-ADPCM is about a quarter of 16-bit PCM |
| Silence | Under 10 ms before the onset. Tails end on their own decay, with no padding | Legacy waves carry up to 717 ms of trailing padding below -60 dBFS (`caramelldansen`) |
| Loops | Author a loop region (`smpl` chunk) in every looping source, and check that the first and last samples join without a click | No legacy wave has one; the mister wave's first and last samples are about 700 (left) and 2,400 (right) steps apart, so it clicks when it loops. The engine reader reads the XWB loop region (`xwb_file.cpp:462-463`) and the `webaudio` backend loops on it (`webaudio/audio_device.cpp:115-117`). DirectXTK's handling is still to be confirmed |
| Peaks and loudness | Peaks no higher than -1 dBFS. Master each class to one reference so the data volumes sit near 1.0 and only shape the mix | Legacy peaks range from -23 to 0 dBFS, made up for by volumes of 0.03 to 1.0. Menu sounds have no volume field at all (trap 6) |
| Variation | 4-6 variants for steps, landings and splashes; pitch jitter where it helps | `play_wave` takes pitch (`sound_bank.h:125-126`); `POL#13` |
| Length to fit | The time-up cue at 2.0 s or less, or allowed to ring out. Countdown beats as separate cues | `tuning.json:4-5`; traps 2 and 3 |
| Banks | Decide whether music gets its own bank | The match sounds resolve against the level's bank (`content_validation.cpp:33-34`), so splitting needs a small code change |

## 6. Production and provenance

### 6.1 The record each asset carries

Every replacement wave gets one record, kept in the repository beside the bank
definition (for example `game/content/sounds/PROVENANCE.md`), before it enters
`waves`.

| Field | Content |
|---|---|
| Bank name | The wave's name in `waves`, and the cue ids it serves |
| Creator / source | The person, or the library and the exact asset id or URL |
| Licence or permission | The licence name and version, or a written permission and where it is kept |
| Use covered | Covers bundling in the release ZIP and the built bank: yes or no |
| Source redistribution | May the editable source and the `.wav` be committed to this repository: yes or no |
| Attribution | The exact credit text, if one is required, and where it must appear (NOTICE, README, in-game credits) |
| Modifications | Trim, edit, loop points, gain, encoding |
| Production source | Path to the editable project (DAW session, recording) and the export settings |
| Date | When it was recorded or obtained |

An asset with an empty licence field does not go into `waves`.

### 6.2 What happens next

- **`POL#22`** has produced Tiers 1–3, with a provenance record for every wave,
  editable sources and technical qualification ([section 7](#7-pol22-production-candidates)).
  The author accepted the complete set on 2026-10-14 with no requested revisions,
  closing this item. The review page includes offline comparisons; these are
  not actual gameplay acceptance.
- **`POL#23`** has migrated the existing bindings, explicit runtime voices,
  bank validation, staging and release packaging to the accepted replacement
  set. It retains the new cue hooks and gameplay mix listed in
  [section 9](#9-runtime-migration-and-remaining-integration).
- **`ROAD#3`** accepts the result: gains, mute, focus loss, pause and resume,
  resumed loops, the multiplayer mix, then the audible release and the
  missing-bank fallback, each checked separately.

### 6.3 Decisions and what is still open

Answered by the author on 2026-10-14:

- **The sound direction in section 5 is accepted**, including the silent picture
  while the person is present.
- **Source waves are committed to this repository.** Every new asset therefore
  needs redistribution rights for its source as well as its use (the
  "Source redistribution" field in 6.1 must be yes). The accepted masters and
  derived bank are committed; a fresh clone has audio. The silent-clone text
  has been replaced in README, NOTICE, CLAUDE.md and AGENTS.md; the optional
  missing-bank fallback remains supported.
- **The fourth commercial track is unknown.** NOTICE keeps "four" without naming
  it; both candidates are retired either way.

- **The Movement Course is silent.** It is the movement test stage, where
  footsteps and landings should be heard alone. Done the same day (trap 7).

Nothing is still open here.

The sourced pop, sigh and chair-creak (`STORY#1`, `STORY#2`) were accepted with
the set in section 7 and integrated on 2026-10-15.

## 7. POL#22 production candidates

**2026-10-14: production complete and author accepted; POL#22 closed.**
[Open the listening page](../game/audio/paint_exe/index.html), starting with Tier 1,
then the same scripted scene at one, two and four players. The
[production README](../game/audio/paint_exe/README.md) gives reproduction commands
and the acceptance record. The files are under `game/audio/paint_exe/`, outside
staged content. The table below records that production milestone; the later
live-bank migration is recorded in section 9.

| Deliverable | Retained result |
|---|---|
| Coverage | 103 PCM WAV masters cover all 85 concrete cue groups in §4: 49 Tier 1, 37 Tier 2, 17 Tier 3. Projectile impacts expand to all five tools; world beds cover the three arenas, never the Movement Course. |
| Physical cues | Recorded marker/paper, water/bubbles, brush/spray/mechanisms and room foley. Six step and splash variants; four landing and painted-step variants. The 64 physical cues have reproducible excerpt/layer/filter/gain recipes. |
| Music | Original Codex-authored ukulele/upright-piano/hand-percussion arrangements using recorded CC0 samples. Menu 87.27 s, Close Quarters 76.19 s, Hill 85.71 s, Turbulence 83.48 s; three corresponding ending stings and a results coda. Editable scores and rendered event records are retained. |
| Interface/match | 31 short arrangements of recorded switches, paper, taps, claps and acoustic plucks; no OS sounds. `ui.count_in` deliberately shares the countdown beat. |
| Provenance | 71 unchanged, hashed source recordings; exact creator/source/licence/redistribution fields in [sources.json](../game/audio/paint_exe/sources.json), and [one record per wave](../game/audio/paint_exe/PROVENANCE.md) beside the candidate bank definition. All recorded sources are CC0; original code/arrangements use the repository MIT licence. |
| Review | Individual cue players, automatic loop repeat and fixed-gain 1/2/4-player offline mixes. The timeline keeps picture sound out of the room interval and reactions after the person leaves. The author accepted the set after the review request; no automated browser playback or detailed listening observations are claimed. |
| Bank | An isolated, named XWB with 103 entries, 22,772,676 bytes; no legacy input or timestamp reuse. Output is under `out/audio-production/`, never the live bank. |

The production checks exercised the technical proposals in §5.2: one bank,
44.1 kHz, PCM16 effects and MS-ADPCM stereo music. Four music loops meet the
60–120 s target. Every looping master has an authored full-file `smpl` region;
the accepted PCM music masters use 500-frame boundaries. The original encoded
bank used FFmpeg 8.1.2 with **512-byte stereo ADPCM blocks (500 frames)**. Its
missing `smpl` was restored before packing; the bundled tool preserved the
metadata and DirectXTK forwarded the loop bounds.

Those checks did not create an XAudio2 voice. The 2026-10-15 runtime probe
rejected the 500-frame ADPCM format (`0x88960001`). The derived runtime encoding
is corrected to **512 frames per 524-byte stereo block**; the 103 accepted PCM
masters are unchanged. The original snapshots below remain a record of their
technical checks, not evidence that their encoded bank played through XAudio2.

The [retained technical record](../game/audio/paint_exe/technical-checks.json)
checks every source/master hash, all cue groups, peaks below -1 dBFS, format,
onset/tail, loop metadata and XWB entry/sample identity. The ADPCM music is
decoded and checked too. The final build has zero technical listening flags;
The production qualification passed 11 pipeline tests, including a real encode/pack and corrupt-metadata
rejection. All 103 masters reproduce byte for byte. These checks do not establish
musical fit, timbre, pleasant loop joins or four-player intelligibility.

The chair creak's retained source is explicitly a CC0 HQ MP3 preview; its
lossless Freesound original requires login. The source format and exact URL
are recorded. This source choice is included in the accepted set. No
repository-author recording is claimed for the sourced pop, sigh or chair.

**Author acceptance:** in response to the explicit request to name revisions or
accept the complete set for POL#23 integration, with the listening page and
1/2/4-player mixes linked, the author replied **"sounds good"** on 2026-10-14.
All 103 masters from `104cfd0`, covering 85 cue groups including the three story
cues, are accepted; no revisions were requested. The
[acceptance record](../game/audio/paint_exe/acceptance.json) binds this response to
their exact hashes. Regenerated catalogues show acceptance only while those
masters match. Original technical snapshots retain their pre-approval status.

POL#22 is closed. The later runtime migration closes the story integration rows;
POL#23 retains remaining new cue hooks and mix. Tier 3 remains optional with the
existing default-off direction. ROAD#3 still tests the actual game, mix and
release; accepting the assets does not close those checks.

## 8. Runtime defect follow-up (POL#26)

2026-10-15: countdown voice ownership, pause confirmation returns, the time-up
transition tail and the post-reveal fanfare are repaired. The runtime migration
also replaces the oversized legacy countdown recording with separate
`match.count_beat` and `match.go` cues, closing POL#26. ROAD#3 retains listening,
multiplayer and hardware acceptance.

Lifecycle validation before the bank migration: `scripts/run-acceptance.ps1 -Preset x64-debug` and `x64-release`
both passed, including the Debug real-Level opening/reveal/replay checks and
Release developer-tool exclusion. `scripts/check-startup.ps1` passed against
the Debug executable with the local bank, an isolated save and graceful
shutdown (`out/validation/pol26-debug/`). These checks exercise construction,
validation and state flow; they do not listen to or assert the emitted audio.

## 9. Runtime migration and remaining integration

**2026-10-15: the existing live audio bindings use the accepted Paint.exe set.**
The runtime definition and tracked derived bank are
`game/content/sounds/paint_exe.json` and `paint_exe.xwb`. The bank contains all
103 accepted masters, encoded as PCM16 effects and MS-ADPCM music, with named
music/weapon/transition voices. Music is reblocked from FFmpeg's output into
512-frame/524-byte stereo blocks for XAudio2; an encoded loop may gain at most
511 frames of tail padding. This changes the derived encoding, not the accepted
PCM masters. The corrected bank is 22,765,884 bytes; its SHA-256, encoded music
hashes, padding and decoded measurements are retained in
[runtime-bank.json](../scripts/audio/runtime-bank.json). The build validates it before staging it; the
normal Release ZIP includes this bank, provenance and CC0 terms. The bank can
be omitted for the existing optional silent fallback, which is checked
separately. Private legacy waves and banks cannot supply either path.

The old generated `out/review-2026-09-26-data/` fixtures remain inactive archival
output. They are outside the explicit bank inputs, staging and release list;
the accepted artifact receipt excludes them from runtime publication. Their
legacy names do not make them migration inputs.

| Binding | Integrated replacement |
|---|---|
| Menu and arena music | `music.menu`, `music.close_quarters`, `music.king_of_the_hill`, `music.turbulence`; Movement Course has no music |
| Player | `player.jump`, `player.hurt`, `player.pop` through the existing `death` slot, and `player.respawn` |
| Movement (2026-10-15) | Four impact-scaled landing variants, six distance-driven footstep variants, and a latched high-speed reversal skid, all from completed contacts |
| Redraw and painted support (2026-10-15) | Four independent redraw voices follow the visible pen phase; four wet variants replace dry steps/landings on exposed painted top tiles |
| Weapons and paint | Sprayer, roller and mister start cues play once per held-fire edge, with independent per-player loop voices; `sniper.shot`, `bucket.throw`, and the first `paint.splash` variant |
| Weapon feedback (2026-10-15) | Sprayer/roller/mister stop edges; all five tools' empty/recovered cues; sniper cooldown-ready cue; roller lift/settle on completed ground contacts |
| Paint feedback (2026-10-15) | Six rotating splashes, all five tools' impacts, neutral/enemy capture and retiring bare-geometry duds, bounded per paint owner across all surfaces |
| Menus and results | Direction, confirm, cancel, error, ready, pause-open, results fill, winner and distinct draw use their replacement cues |
| Match start/end | Repeated `match.count_beat` edges and one `match.go`; separate time-up and glass-out voices replace the whistle |
| Closing clock and room (2026-10-15) | One `match.last_ten` per displayed positive second at ten or below; a quiet `room.tone` loop before the push and throughout the reveal |
| Opening and reveal | Reveal hover/click use the replacement cues; the accepted sigh/chair cues have tuning slots and pause-aware phase-edge playback |
| Opening/reveal detail (2026-10-15) | Opening swatch hovers, undo, colour-in, skip and glass-in; distinct draw-close and one winning/losing reaction per participating team after the person leaves |

The pop's data swap closes STORY#1. The sigh/chair bank entries, tuning slots
and play calls close STORY#2. The per-beat countdown/GO replaces the 5.375-second
recording that used to overlap active play, closing POL#26. All three closures
describe integration; their audible balance and timing still need ROAD#3.

The weapon-feedback increment preserves shot costs, cadence and regeneration.
A fatal hit now suppresses firing in the interval before the next update marks
the player dead, so that interval cannot emit new weapon feedback. An empty
trigger hold plays `weapon.empty`, then at most one reminder per authored
`empty_repeat_delay` (0.65 seconds). Regaining a full shot's cost plays
`weapon.recovered` once. `sniper.ready` follows a paid shot's cooldown when
paint is available, independently of whether fire is held; it is quiet at
spawn. Recovery uses firing's full-shot affordability test; readiness also
observes its strict cooldown boundary, including when infinite-ammo testing
silently refills the tank. Loop stop cues mark ordinary release or exhaustion;
pause, death, time-up and teardown stop voices without inventing a release.
Roller ground cues use completed contacts, including buffered takeoffs, and suppress the
first settling contact after spawn or respawn. Ground seams stay quiet.
New feedback edges and their timers do not advance on zero-delta updates.

`weapons.<tool>.feedback_sounds` owns cue names, gains and the repeat delay;
each wave resolves at construction and startup validation checks the authored
names. Gains use the current SFX preference. These are integration candidates;
their gains and the combined multiplayer mix still need listening acceptance.

The movement increment uses `player.movement_sounds` for cue choices, gains,
stride distance, minimum step interval, landing impact scale and skid limits.
`MovementAudio`, owned by each player, observes completed contacts and never
changes movement or consumes the gameplay RNG. Initial spawn/respawn settling
and terrain seams stay quiet. Landing strength scales with resolved impact
speed, clamped between the authored minimum gain and full strength. A buffered
landing-and-jump can play its landing without inventing a footstep.

Footsteps require consecutive supported ticks and actual horizontal travel,
clipped to the requested displacement so collision separation cannot count as
walking. Each player emits at most one step per update and one per authored
minimum interval; idle, airborne, wall pressure and placement discard partial
strides. Skids require a fresh opposing-input edge above the speed threshold,
resolved grounded travel and an elapsed repeat delay. Each player's variants
rotate independently, with a different initial offset for each seat. No new
movement cue or audio timer advances on a nonpositive update. All gains use
the current SFX preference, including mute; pause freezes the simulation and
creates no catch-up cues. These quiet initial gains remain listening candidates.

The opening/reveal increment follows timeline edges, including both preview
hovers when a long update crosses them. Undo follows the previews, colour-in
starts the countdown, and glass-in starts during its authored final `push`
span. A skip stops the bypassed room, undo, colour-in and glass-in voices,
plays its own cue once and continues the split-screen countdown. A draw shows
no swatch previews, so its reveal plays neither hover nor winner-click: it
uses the distinct close cue and has no celebration or slump. A winning reveal
plays each participating team's reaction once after the person has left,
independently of how many figures or panes it has.

New choices and gains live under `match.sounds`; startup validation checks all
names and volumes. The longer cues use explicit effect voices owned by the
level: pause suspends them, resume refreshes their SFX gain without replaying
them, and teardown stops them. Solo and intro-off matches have none of these
story cues. Nonpositive updates refresh views without advancing the match or
emitting new cues. The quiet authored gains remain listening candidates.

The paint increment collects each projectile's result across both collision
callbacks before requesting sound. A contact that reaches a paint tile plays
the tool's impact and one splash variant. A tile changing from neutral or enemy
paint also requests capture feedback; friendly repaint keeps the original
player's scoring credit and emits no capture. Rollers aggregate all surfaces
in their contact batch without losing their multi-contact painting or damage.
A flying projectile retired on flat geometry without touching an exposed paint
tile requests a dud. Enemy-player hits, lifetime/out-of-bounds retirement and
rollers resting on bare geometry do not invent duds. Launch obstructions are
flushed before the player's existing discarded-shot cleanup.

`paint.splash_sounds`, `impact_sounds`, `capture_sound` and `dud_sound` own all
choices and gains. Startup validation checks every entry. Level owns the
limiter, which advances once before simulation contacts even if a shot's owner
has died. Each paint owner shares one `min_impact_interval` (0.10 seconds) across
all tools, splashes and duds on every surface, plus an independent
`min_capture_interval` (0.25 seconds). Suppressed contacts are discarded; long
updates create no sound backlog. Splash variants rotate independently, starting
at a different offset per owner, without consuming the gameplay RNG. Pause and
nonpositive updates create no cues or cooldown progress. All requests use the
current SFX gain, including mute; the short waves have the same fire-and-forget
lifecycle as movement cues. The quiet gains are listening candidates, not a
qualified multiplayer mix.

The redraw/painted-support increment uses four explicit `player.redraw_sounds`
effect instances, selected by roster/view index rather than pad slot. Sparse
pads, keyboard/mouse and device-less debug seats therefore retain independent
voices. The pen loops only between the redraw presentation's `draw_from` and
`fill_at` boundaries; hidden workshop redraws and whole-figure reveal poses stay
quiet. Fill, respawn, time-up and teardown stop the voice. Pause/resume preserves
its position and refreshes the current SFX gain, including mute. A long tick
that passes the whole drawing interval creates no catch-up loop.

Painted footsteps and landings select `player.movement_sounds.step_painted`
instead of layering over the dry cue. Either team's paint qualifies; paint
ownership and scoring are read without changing them. At the contact phase's
end the player reads `Scene::contacts()` as well as the dispatched top-support
callbacks, then samples the midpoint of its final foot overlap on each
still-supporting surface. Adjacent floors remain candidates even when separation
against one made the other's callback redundant; paint on either supporting
overlap selects wet feedback independently of scene insertion order. This sees
same-tick painting regardless of callback order and rejects a support moved away
from by later separation. Only exposed top tiles qualify: painted walls,
undersides, clipped intervals, plain ramps and jump-through platforms stay dry.
Tile intervals are half open, giving tile seams a consistent owner. Wet variants
rotate per player without gameplay RNG; they share the existing stride and step
interval budget, and wet landings retain impact-scaled strength. Spawn settling,
wall pressure, airborne travel, fatal contacts and nonpositive ticks retain their
existing suppression. Authored gains remain listening candidates.

The clock and room audio increment uses `match.sounds.last_ten` and `room_tone`
for effect names and gains. Each current displayed positive second at ten or
below sounds once, including the first active tick of a short match. A long
update sounds only its current second; one that reaches time-up emits no late
clock cue. Time-up stops the clock voice before its own cue. Neither cue
depends on coverage or reveals the score.

The room loop starts on the first positive full/quick opening update, survives
the monitor countdown and stops when the glass push starts. Skipping stops it
without starting an unheard loop. The pull back into the monitor starts it
again, and finishing the winning or drawn reveal stops it. Intro-off and solo
practice have no room loop. Both voices belong to Level: pause preserves their
positions, resume refreshes the current SFX gain without replay, and teardown
stops them. Nonpositive updates emit nothing. The quiet room gain and clock
gain remain listening candidates; accepted masters and bank bytes are unchanged.

**POL#23 remains open.** Putting an accepted wave into the bank does not create
a gameplay trigger. Complete the required new cues from section 4, assess Tier
3 detail individually and keep it off or nearly off by default:

- Movement: optional bonk/drop/hit-confirm details.
- Paint/tool detail: optional blocked/slosh/drip cues.
- Room: optional footsteps/breath/waver/cheat.
- Menus/music: pad lost/back, reconnect count-in (`POL#18`), distinct unready
  and volume-preview hooks, series cue; results music, arena ending stings and
  transition fades/ducking (`GAP#5.1`); optional world ambience/detail.
- Mix: useful action cues above weapon voices, movement, music and decorative
  detail; limits on repeating impacts/steps; actual one-, two- and four-player
  audition with music/SFX gains, mute, focus, suspension and resumed loops.

ROAD#3 then qualifies listening from the extracted Release ZIP and the missing-bank
fallback separately. Offline listening-page acceptance and automated construction
checks do not establish the in-game mix, physical-pad behaviour or an audible
release check.

Migration validation: full Debug and Release consumer acceptance pass, including
the focused `story_audio` null-recorder check for cue edges, zero-delta updates,
countdown/time-up once on a short match, gain changes, skip/quick-rematch/solo
and weapon held starts/pause/release/death. All 18 audio tests pass (12 pipeline,
six runtime/publication). The 51-file Release ZIP was inspected: the corrected
bank, provenance and CC0 terms are included; source waves/previews, private
banks, tools and debug symbols are excluded. Extracted Release startup with the
real XAudio2 bank and its separately omitted-bank fallback both pass with a
system-only PATH, isolated save and graceful close. Isolated Debug missing-bank
and real-bank startup also pass. Evidence is under `out/validation/audio-migration/`,
including `release/package-inspection.json`. These checks establish API
requests, asset integrity and startup, not listening or multiplayer mix quality.

Weapon-feedback validation (2026-10-15): Debug and Release builds and all 13
consumer acceptance cases pass. The expanded `story_audio` recorder checks all
five real factory weapons, paid volleys, bounded held warnings across brief
affordability recoveries, recovery edges, the strict sniper cooldown boundary,
four independent loop voices, ordinary versus lifecycle stops, roller ground
contacts, current gains/mute, infinite-ammo readiness and the fatal-health
guard. Content variants reject missing cue names, invalid volumes and
nonpositive repeat delays. The six audio publication checks pass. Isolated
Debug and Release startup pass with the real bank and, separately, without it;
each closes gracefully. Consumer logs are under `out/acceptance/`; startup
evidence is under `out/validation/weapon-audio/`. These checks establish cue
requests and lifecycle behaviour; ROAD#3 retains listening and multiplayer mix.

Movement-audio validation (2026-10-15): Debug and Release builds and all 13
consumer acceptance cases pass. The expanded `story_audio` recorder checks
variant cycling, short/long-fall gains, strides and repetition limits, four
independent players, mute, spawn/respawn settling, seams, walls, airborne motion,
buffered landing-and-takeoff, drop-through platforms, both ramp directions,
fatal contact ordering and nonpositive updates. Content variants reject empty
arrays, every missing variant, invalid gains and nonpositive limits. Isolated
Debug and Release startup pass with the real bank and, separately, without it;
each closes gracefully. Evidence is under `out/validation/movement-audio/`
and `out/acceptance/`. These checks establish cue requests and preserved gameplay;
listening intensity and the multiplayer mix remain ROAD#3.

Opening/reveal-audio validation (2026-10-15): Debug and Release builds and all
13 consumer acceptance cases pass. The expanded `story_audio` recorder checks
the new cue names and volume ranges, swatch/undo/colour/glass/skip edges,
positive versus nonpositive updates, full/quick/skipped/off/solo matches,
real painted A/B wins versus draws, four-player team reaction counts and
chronological cue order across long reveal updates. It also checks current
gains/mute, skip cancellation, pause/resume without replay and teardown.
The six audio publication checks pass. Isolated Debug and Release startup pass
with the real bank and, separately, without it; each closes gracefully.
Evidence is under `out/validation/opening-audio/`, the adjacent build/acceptance
logs and `out/acceptance/`. These checks establish cue requests and lifecycle
behaviour; ROAD#3 retains listening and multiplayer mix acceptance.

Paint-audio validation (2026-10-15): Debug and Release builds and all 13
consumer acceptance cases pass. The expanded `story_audio` recorder checks
every cue and authored gain/limit, all five factory projectile kinds, both
callback orders, neutral/enemy capture versus friendly credit, roller
aggregation, swept/launch duds and quiet lifetime retirement. It checks six
variant rotation, four paint-owner budgets, device-less/fallback owners,
current gains/mute, independent capture limits, long and nonpositive updates,
the real Player's discarded launch shot and a Level hit after its owner dies.
Isolated Debug and Release startup pass with the real bank and, separately,
without it, each with a system-only PATH and graceful close. Evidence is under
`out/validation/paint-audio/` and `out/acceptance/`. These checks establish cue
requests and preserved gameplay; ROAD#3 retains listening and multiplayer mix.

Redraw/painted-support validation (2026-10-15): Debug and Release builds and
all 13 consumer acceptance cases pass. The expanded `story_audio` recorder
checks redraw content rejection, independent one-/two-/four-player roster
voices (including device-less seats), live pen boundaries and gains, mute,
pause/resume, fill, natural/forced respawn, hidden poses/workshop art, time-up,
teardown and nonpositive countdown stability. Painted-support checks cover
neutral/friendly/enemy ownership, exposed versus disabled/clipped/non-top
faces, tile boundaries, same-tick callback order, final-footprint separation,
reversed dry/wet floor insertion order and fractional floor heights. Wet cues
retain impact strength, buffered-jump/fatal-contact suppression, shared dry/wet
stride and repetition limits, and independent variant cycling. The six audio
publication checks pass; the accepted bank and masters are unchanged. Isolated
Debug and Release startup pass with the real bank and, separately, without it,
each with a system-only PATH and graceful close. Evidence is under
`out/validation/redraw-painted-audio/` and `out/acceptance/`. These checks
establish cue requests and lifecycle behaviour; listening and multiplayer mix
remain in POL#23 and ROAD#3.

Clock and room audio validation (2026-10-15): Debug and Release builds and all
13 consumer acceptance cases pass. The expanded `story_audio` recorder checks
displayed seconds ten through one, short one-/two-/four-player matches,
same-second suppression, long ticks without backlogs, time-up cancellation,
current gains/mute, pause/resume and teardown. Room checks cover full/quick
openings at two and four players, construction and nonpositive updates,
countdown/push boundaries, immediate/in-progress skips, reveal completion,
intro-off/solo, loop flags and lifecycle. Coarse clock-only ticks reset player
placement so unrelated accumulated falls do not obscure their audio assertions.
The six audio publication checks pass. Isolated Debug and Release startup pass
with the real bank and, separately, without it, each with a system-only PATH
and graceful close. Evidence is under `out/validation/match-room-audio/`, the
adjacent build/acceptance logs and `out/acceptance/`. These checks establish cue
requests and lifecycle behaviour; listening and multiplayer mix remain in
POL#23 and ROAD#3.
