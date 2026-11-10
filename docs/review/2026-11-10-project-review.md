# Project review, 2026-11-10

A review of the whole project for improvements and new features, made by
comparing it with similar games and similar projects. This file is a record,
frozen as written. It owns the `REV` rows in [TODO.md](../../TODO.md), and under
that file's rule 2 the row, not this record, holds a finding's current status.

## Method

The review ran in three passes, all read-only. Nothing was built or run, so no
finding here rests on play, a measurement or a listening pass.

1. **Discovery.** Nine reviewers each took one lens. Five read the repository:
   gameplay and content, player-facing UX, code health, tooling and release, and
   presentation. Four compared it with other work: the Splatoon series,
   local-multiplayer arena games (TowerFall, Duck Game, Boomerang Fu and others),
   other paint and territory games (Splasher, INK, de Blob), and small C++ games
   on their own engines as projects. They returned 118 candidates, about 83
   distinct findings once duplicates were merged.
2. **Verification.** Seven verifiers each tried to refute one theme's findings:
   they reopened the cited files, searched for the thing claimed absent, and
   checked TODO.md, the roadmap, the polish plan, the gap audit and the other
   owners for an existing row or a recorded decision. None was refuted outright.
   Most ratings were lowered, 8 findings matched a decision the author had
   already made and 5 were already tracked.
3. **Write-up.** The 69 that survived are the entries below, each carrying the
   verifier's corrected facts and the strongest objection to acting on it.

Each entry names its evidence by path and line as they stood at `26acaa39`.
Sizes and severities use TODO.md's scales. A status of `DECISION` means the
next step is the author's call, usually because the finding sits beside
something accepted or waived on 2026-11-05.

## Summary

No serious defect was found. One entry is rated `HIGH`: the README's download
instructions lead nowhere, because no release exists and the fallback it names
no longer carries a ZIP (`REV#11`). The weakest points are around the game
rather than in it: it cannot be downloaded, the README shows nothing of it, and
the shipped instructions predate sub weapons. Inside the game the larger gaps
are bot behaviour (`REV#19`, `REV#20`), the paint mechanics that appear only on
trial stages (`REV#27`), and a player leaving the arena ending the process
(`REV#1`).

## Code and robustness

### A player leaving the arena ends the process (REV#1)

`game/objects/level.cpp:430-437` throws `PlayerOutOfBounds` once a player's box lies wholly
outside the scene bounds (`external/labrador/engine/scene/scene.cpp:113-119` tests
intersection). Nothing catches it before `wWinMain`; `GameLevel::step_match`
(`game/states/game_states.cpp:481-495`) has no try, so the game exits with the failure dialog.
Finished rounds are already saved; the round in progress and the in-memory series are lost.
`LevelLoadedInfo::validate` (`game/objects/level_loaded_info.cpp:167-236`) checks only spawns
and stations, and `LevelObjectBuilder::validate_objects`
(`game/objects/level_object_builder.cpp:698-741`) checks nothing about enclosure. Gap-audit L8
and GAP#7.3 recorded this when every arena was a sealed box; ROAD#16 user stages now let
unreviewed geometry reach it. Splatoon and Smash Bros. treat leaving the stage as a splat.
`Player::kill` and `Player::respawn` exist (`game/objects/player.cpp:1635`, `:1667`).

The fix is either an elimination with the normal respawn, or a scan refusal of unsealed user
stages. The first changes a `Level` rule under every exact-pinned replay and makes pits
authorable; the second needs a definition of sealed that movers, gates and one-way platforms
complicate. The author picks.

### User stage geometry and folder have no caps (REV#2)

Level bounds (`game/objects/level_loaded_info.cpp:426-444`) and object rectangles
(`game/objects/level_object_builder.cpp:89-100`) are checked only for finite values and positive
size. `StructurePaintable::generate_paint_tiles`
(`game/objects/structure_paintable.cpp:348-368`) makes one `PaintTile` per 4 units of face, and
its float-to-int cast is undefined once the quotient passes `INT_MAX`. `load_user_stages`
(`game/config/stage_loading.cpp:94-162`) builds every `.json` with no file-size or count cap, at
startup as well as at Stage Select, and `validate_objects` builds moving or slab stages twice.
The only limit in the contracts is the 64-character name
(`game/contracts/user_stage_contract.h:31`). A stall needs about four extra digits on a width,
so this is a rare typo or a deliberate file; the README invites only the player's own files.

A fix puts extent, object-count and paint-length limits in `game/contracts/level_format.h`,
refused with a reason by the scan, the editor and the schema alike. The objection: there is no
adversary on a couch game, and a cap set too low refuses a legitimate large stage. Fuzzing under
ASan is a separate CI job and should not ride with this.

### Breakable subs are struck by travel bounding box (REV#3)

`Level::strike_breakables` (`game/objects/level.cpp:504-520`) spends an enemy shot on a planted
sprinkler or wall when the shot's `travel_bounds()` intersects it. That box is the axis-aligned
union of the previous and current rectangles (`game/objects/projectile.cpp:270-279`), so a
diagonal sniper jet can be eaten up to about 17 units clear of a corner; slower shots miss by a
few units at most. The test also runs after `scene_->resolve()`
(`game/objects/level.cpp:388-401`) and skips shots already marked for deletion, and planted subs
have an empty mask (`game/objects/projectile_planted.cpp:69-72`). The case that occurs in play
is a player standing inside the 32-unit wall's footprint: a shot reaching that player in the
same tick hits them and the wall never counts it. Splatoon's Splash Wall blocks along the shot's
path.

A fix tests the swept rectangle and makes planted breakables sweep participants in
`Projectile::resolve_sweep`, with a diagonal near-miss check in the `sub_weapons` case. It is
not in ROAD#11's left-for-later lists. The objection: hit generosity is feel, the sweep is under
bit-exact replays and painter pins, and ROAD#11's play review has not happened.

### No containment around the match or replay step (REV#4)

`GameLevel::step_replay` (`game/states/game_level_replay.cpp:105-115`) and `step_match`
(`game/states/game_states.cpp:481-495`) call `Level::update` with no try, and `wWinMain` is the
only handler. For the instant replay there is no known Release cause:
`game/match/round_tape.h:18-25` says a tape holds on this build, machine and step and names only
Debug sources of divergence, and the shipping tape is replayed in the process that recorded it.
The live-round half is the unbuilt recovery of GAP#7.3, which
`docs/failure-reporting.md:111-118` scoped out when that row closed. The one throw a player can
reach mid-match is `PlayerOutOfBounds`, which is REV#1. Rollback-netcode games checksum state
per frame to detect desync; that motivated the proposed tape hash.

A fix would catch at `GameLevel`, discard the level and return to the End menu with the series
intact. The objections: a catch mid-update leaves a half-stepped scene, a broad catch hides the
`logic_error` checks Debug relies on, and a tape hash adds a second determinism contract for a
failure nobody has seen. It reopens a recorded boundary, so it is the author's call, best taken
after REV#1.

### Save recovery notices reach only stderr (REV#5)

The game is a `wWinMain` program and nothing attaches a console, so a Release player never sees
what `game/save/save.cpp` reports: an unreadable save kept aside (`:345`, `:351`), a section
reset (`:388-389`), a legacy read failure (`:423-424`), and a save from a newer build whose
unknown sections the next write drops (`:368-370`) with no copy taken first. The only dialog is
the first-write failure (`:432-439`). The discarded result in `record_result_and_save`
(`:555-566`) is deliberate and commented, and the silent audio-bank fallback is correct by rule.

A fix has `Save` keep its load notices, shows one line on Title or Home from
`presentation.json`, and copies the file aside before a downgrade write. A session log, as Godot
keeps, is ruled out: `docs/failure-reporting.md:44-68` keeps one record and writes no paths to
disk. HYG#22 in `docs/code-hygiene.md` already ends with the in-game notice still not decided,
and it was not among the 2026-11-05 calls. The objection: the paths are rare, and the notice is
new copy and layout that moves menu painter fixtures.

### Small per-frame waste on the match hot path (REV#6)

None of this is measured, and with 20 to 41 collision objects per shipped arena it is negligible
today. `Level::refresh_views` (`game/objects/level.cpp:704-747`) walks every projectile target
through `Collider::of`, a `dynamic_cast` (`game/objects/collider.cpp:8-18`), up to four times a
frame: from `GameLevel::seat_commands` (`game/states/game_states.cpp:499`), at the start and end
of `Level::update` (`level.cpp:147`, `:161`) and from draw (`game_states.cpp:518`).
`Projectile::resolve_travel` runs an `any_of` with the same cast over all targets per swept
projectile per tick (`game/objects/projectile_contacts.cpp:227-232`), and `resolve_sweep` a
second for fixed slabs (`:389-390`). Per-tick vectors are built at `level.cpp:368`, `:444` and
`:169-179`, in `game/match/round_tape.h:84`, and by `MatchBotDriver` copying obstacles, supports
and ramps into each bot's view (`game/ai/match_bot_driver.cpp:128-130`, `:202-204`).

The fix in scope is caching the mover and fixed-slab flags per level; the role lookup through
`Collider` is the OOP design and stays. The objection: the performance captures were waived
(POL#25), so no baseline can show a gain, and reusing buffers under bit-exact replays is churn
with a small regression risk.

### Static paint draws one tile at a time (REV#7)

`StructurePaintable::draw_paint` (`game/objects/structure_paintable.cpp:86-121`) draws each
tile's splash and surface, and `PaintTile::draw_surface` (`game/objects/paint_tile.cpp:70-95`)
submits a surface sprite, an edge sprite and a wet highlight per owned 4-unit tile, in every
pane; `update` walks every tile each step (`structure_paintable.cpp:55-60`). Crossfeed is 3800
wide, so a full floor face is about 900 tiles. The fixed slab merges only its opaque base, and
for a visual reason: quantising each rotated tile separately exposes primer at the joins
(`game/objects/fixed_slab_paintable.cpp:153-156`). It still draws the wet band per tile, so
reusing that path saves one or two of three sprites per tile at most. 3D Splatoon-likes such as
UnySplat paint into a texture instead.

No draw-cost measurement exists, and the performance captures were waived (POL#25). The first
step is to measure a fully painted four-player arena with the perf overlay and do nothing if
paint does not show. The objection: merging bands changes rasterisation at tile joins, which
costs a painter fixture re-qualification on MattPC across 97 leaves for a gain nobody has shown.

### Stale SeatSetup comment about the grenade (REV#8)

The comment on `SeatSetup::sub` at `game/match/seat_setup.h:12-14` still says only code and
probes choose a sub until Weapon Select's loadout row, that the save, the Debug plan and replays
do not carry it, and that every menu match is the grenade. ROAD#11 slice 5 landed on 2026-11-09
(`docs/feature-roadmap.md:2395-2410`): `game/states/loadout_choice.h` exists, the save reads and
writes each last-match player's `sub` (`game/save/last_match_setup.cpp:50-52`, `:394`), and the
Debug plan has a `sub` seat field (`game/debug/replay/match_plan.cpp:104`, `:166`). The header
was last touched in the slice 3 commit `d9533214`.

The fix is to say that the grenade is the default and that Weapon Select, the save, the Debug
plan and replays carry the choice. There is no objection beyond its being a comment: it is worth
doing alongside the next ROAD#11 change, not as a commit of its own.

## Release, README and repository

### Player docs predate sub weapons (REV#9)

`ROAD#11` gave every player a sub on 2026-11-09, but the two player-facing documents were not
updated. `docs/HOW-TO-PLAY.txt:42-48`, which ships in the Release ZIP and was last committed on
2026-10-30, is a six-row controls table (Move, Jump, Aim, Shoot, Drop through, Pause) with no
Sub row. Neither it nor `README.md` contains sub, grenade or sprinkler, and `README.md:129`
still says Pad Buttons chooses Jump, Shoot and Pause. The in-game copy is right
(`game/content/presentation.json:122`, `:134`, `:156`, `:158`).
`game/content/levels/movement_course.json:58-130` has ten lessons and none on the sub, and the
`ROAD#11` section of `docs/feature-roadmap.md` records no decision to leave either out. Splatoon
teaches the sub in its opening tutorial and its test range.

The fix is a Sub row and a short loadout paragraph in both files. A SUB course station is a
separate M piece, because the stage carries acceptance route pins. The objection is that the
countdown card and Controls help already name Sub in game, so few players depend on the text
file.

### Paint underfoot has no player cue (REV#10)

Paint underfoot ships on under `POL#30`: `game/content/tuning.json:177-185` gives own paint 1.10
speed and 1.25 refill, and enemy paint 0.85 speed and no refill. Nothing tells the player. In
`game/` the word underfoot appears only in the bots, config, debug, tuning and
`game/objects/player.cpp`; no HUD, draw, audio or `presentation.json` file reads it, and
`docs/HOW-TO-PLAY.txt` does not mention it. `docs/design-process.md:616-617` lists it among
features no story explains. The lesson at `game/content/levels/movement_course.json:131` says
the tank refills when you stop shooting, which is incomplete rather than false; whether the
course contains enemy paint was not checked. `game/objects/movement_audio.cpp` shows no own or
enemy branch on a search, but was not read line by line. Splatoon shows the character sinking in
enemy ink with its own sound.

The safe slice is one line in HOW-TO-PLAY and the lesson text. A HUD or audio cue is the
author's presentation call. `POL#30`'s own reopen trigger at `docs/polish-plan.md:1976-1977` is
players not knowing why they slowed, but the amounts are provisional and a cue built now may be
reworked.

### README download route is dead (REV#11)

`README.md:10-15` sends players to the latest release and, until the first release is out, to
the `ColourWars-windows-x64` artifact on a passing `master` run. Neither exists. The public
repository has no releases and no tags, and the last successful `master` push runs carry no
artifacts: a push whose tree already passed on its pull request skips the Windows jobs
(`.github/workflows/ci.yml:59-61`), and the ZIP upload sits inside the Release checks leg
(`.github/workflows/ci.yml:378-385`, 14-day retention). The ZIP therefore exists only on
pull-request runs, which the README's filter excludes. `README.md:388-392` repeats the claim.
TowerFall, INVERSUS and most itch.io couch games offer a one-click download.

The local fix is to reword both passages to point at pull-request runs or `build.cmd`. The
publish job is written (`.github/workflows/ci.yml:399` onward) and the version is 0.1.0, but
cutting `v0.1.0` is the author's call under AGENTS.md, and the pad, clean PC and audio checks
were waived rather than performed, with `ROAD#11`'s play review still open. A changelog file is
not proposed: `TODO.md` says `git log` is the changelog.

### README has no screenshot or clip (REV#12)

`README.md` has no image in 511 lines, although the game's pitch is visual. `docs/images/` holds
32 entries, including `docs/images/polish-crossfeed/4p-clean-advance-720p.png` and
`4p-heavy-advance-720p.png`, and eight tracked mp4 files. The repository also has no topics and
no homepage set. No doc or row records a choice to keep the README text-only. Mindustry,
SuperTuxKart, Veloren and OpenRCT2 open their READMEs with a screenshot or GIF.

The fix is a hero screenshot and a short clip at the top; GitHub does not play an mp4 from the
repository, so the clip needs a GIF or an uploaded asset. Moving the settings reference
(`README.md:129-138` and around it) and the CI detail out of the README is a separate M
restructuring and a matter of taste; its exact line ranges were not checked.

The objection is that `POL#16` is deferred, so any hero image shows art the author considers
unfinished, and the existing captures are review evidence rather than chosen shots. An image is
cheap to replace later. Which image represents the game publicly is the author's choice.

### Developer text panel ships in Release (REV#13)

The per-player developer text panel is reachable in Release on pad Back or F1.
`game/objects/player_input.cpp:72` and `:128` set the toggle from `GameAction::debug`,
`game/input/action_bindings.cpp:98-101` binds it to the debug key and the pad's Back button,
`game/objects/player.cpp:1125` toggles it, and `game/objects/level_draw.cpp:148-152` draws it
with no `_DEBUG` gate. `game/content/presentation.json:122` lists Debug: Back / View and Debug:
F1 in the player-facing Controls help. This is known and maintained, not a defect:
`docs/design-process.md:605-625` lists it under features no story explains,
`docs/debug-console.md:1491` calls it Release's legacy text, and `GAP#5.4` relabelled its
readout on 2026-10-15 (`docs/polish-frame-pacing.md:51`). No keep-or-cut decision is recorded.

A change would either compile the action out of Release, or keep it and drop it from the help or
put it behind an Options switch. The objection is that it is the only frame-time readout a
Release player can show when reporting a performance problem, and removing it moves acceptance
text pins for little gain. It needs the author's keep-or-cut call.

### Random palette draws the exempt pairs (REV#14)

Random is the default team palette (`game/save/save.h:16`) and
`game/objects/team_colour.cpp:28-35` draws uniformly over all six authored pairs. Two of them,
pink_green and blue_yellow, are exempted by name from the project's colour-vision bar:
`docs/feature-roadmap.md:1929-1939` records the bar and the exemptions, and
`scripts/acceptance/team_palette.cpp:232` holds the legacy list. So one default roll in three
gives a pair the project has measured as failing.

This sits beside a settled matter. Gap audit 2.1 (`docs/review/gap-audit.md:130-139`) proposed a
filter on which pairs the roll may draw; the author answered with fixed Options choices and
three passing pairs (`ROAD#8`, accepted 2026-10-27) and kept the legacy pairs as shipped.
`TODO.md` says random pairs return only if the author reverses the decision. Whether the default
draw should include the exempt pairs is not recorded either way.

A fix would be a per-pair content flag that keeps the legacy pairs as fixed choices but removes
them from the roll. The objections are that these are the game's original colours, and that
changing the seeded draw invalidates recorded replays and any pins on the roll.

### No contributor or issue files (REV#15)

The repository tracks no CONTRIBUTING, SECURITY, issue template, pull request template or
CODEOWNERS file; `.github/` holds only `actions/` and `workflows/`. The engine has
`external/labrador/CONTRIBUTING.md`, `CHANGES.md`, `SECURITY.md` and `.editorconfig`. The real
rules are in `AGENTS.md`, which is addressed to agents. No doc records a decision about
contributor files. A changelog file is not part of this entry: `TODO.md` says `git log` is the
changelog, and the publish job uses generated notes.

The defensible slice is a short CONTRIBUTING.md that states the OOP rule and that engine changes
go to Labrador, plus a bug form asking for `build-info.txt` and `last-error.log`. Mindustry and
OpenRCT2 use issue forms that ask for version and logs.

The objection is strong: nobody outside the author contributes, there is no release to file bugs
against, and the files would duplicate AGENTS.md and become another thing to keep true. It is
worth doing only alongside a first release, which is the author's call (`REV#11`).

### CI action pins and nightly rebuilds (REV#16)

The workflows reference actions by major tag: `actions/checkout@v7`, `upload-artifact@v7` and
`download-artifact@v8` at `.github/workflows/ci.yml:35`, `:87`, `:134` and `:418`, and
`actions/cache@v6` at `.github/actions/windows-setup/action.yml:18`. There is no
`.github/dependabot.yml`. All four are GitHub's own actions, so the supply-chain case for SHA
pinning is weak. The cache covers only vcpkg archives (`action.yml:18-22`), and
`.github/workflows/nightly.yml:38-50` runs nine legs, six on `x64-release`, each configuring and
building for itself (`nightly.yml:57-70`). Per-leg builds are a recorded choice for pull
requests (`ci.yml:67-69`); the nightly is not covered by that comment, though the same reasoning
applies. `CI#7` covers nightly promotion only. The `vcpkg.json` baseline claim and Dependabot's
vcpkg support were not checked.

A change would be a Dependabot file for github-actions and, at most, a shared Release build for
the nightly. The objections are that each Dependabot pull request runs the full five-leg gate
for actions that rarely break, and that sharing a build tree by artifact is fragile and saves
nothing on a public repository.

### No editorconfig in the repository (REV#17)

The repository tracks no `.editorconfig` and no `.clang-format`, while the engine ships
`external/labrador/.editorconfig`. The naming conventions exist only as prose in `AGENTS.md`,
and `scripts/ci/lint.py` checks whitespace errors on changed lines. Line endings are already
normalised: `.gitattributes` has two live rules, `* text=auto` and `*.cmd text eol=crlf`.
`game/objects/team_colour.cpp` is tab-indented; the tabs claim for
`game/directory_consts.cpp:110` was not checked. No doc records a decision on formatter
configuration.

The fix is an `.editorconfig` that matches current practice, copied from Labrador's if its
settings agree with the game's files, which should be confirmed first. A `.clang-format` is not
proposed: it risks reformatting churn across a large hand-formatted tree for a solo author with
an established style.

The objection is that the only contributors are the author and agent sessions that already
follow the surrounding style, so the file helps a reader who does not yet exist.

### Large docs mix manual and records (REV#18)

Three owner docs are each several documents in one: `docs/feature-roadmap.md` is 4,832 lines,
`docs/code-hygiene.md` 4,700 and `docs/level-editor.md` 1,679. In the level editor doc the
Editing section (line 49) opens with an authoring follow-up shortlist that runs to line 655
before Materials, and lines 891-1327 are the `ED#7` to `ED#27` backlog records, so a stage
author cannot read it as a manual. There are 31 `docs/polish-*.md` files among 50 top-level
docs. The split of `docs/debug-console.md` from `docs/debug-console-record.md` is the precedent
(`docs/README.md:22`, `docs/code-hygiene.md:2479`). Godot and Bevy keep the manual apart from
proposals and changelogs.

The one slice with a concrete reader is a level-editor manual split from its records, about M.
Moving closed rows out of the roadmap and hygiene docs is L and mostly cosmetic.

The objection is that these are working records navigated by `TODO.md` pointers and anchors. A
mass move touches a very large number of anchors, and the link checker catches missing targets
but not wrong ones.

## Bots, stages and records

### Bots chase sighted enemies and ignore the clock (REV#19)

`game/ai/match_bot.cpp:294` sets the goal to the nearest visible enemy whenever the bot is not
refilling, at any distance up to `sight_distance` 1600 (`game/content/tuning.json:11`); the
comment at `291-293` says this is deliberate so short-range tools reach combat. Inside 0.55 of
engage distance plus braking the bot stops moving (`match_bot.cpp:408-411`), jumps are
navigation only (`419-421`), and the only retreat is while refilling (`298-304`).
`MatchBotObservation` (`game/ai/match_bot.h:18-85`) carries no match time, own health or score.
Only enemies in clear line of sight count. That a human beats bots by painting elsewhere is an
inference and was not measured. Splatoon's last-seconds paint push and Smash CPU levels
motivated it. A fix is four slices: a paint-versus-fight weighting, a final-seconds paint rule,
a health retreat, and movement while engaging. Objection: `ROAD#10` accepted the bots as they
stand on 2026-11-05, and each rule needs new tuning, requalified bot cases and a rerun balance
matrix.

### One bot policy plays all five tools (REV#20)

The bot policy branches on tool in two places: `deliberate()` (`game/ai/match_bot.cpp:65-69`,
keyed on `shoot_interval`, not tool identity) and `roller_throw` (`264-268`), plus per-tool
ballistics. `docs/gameplay-tuning.md:659-672` records the Hard Sniper bot's share of painted
area at 12.0 to 14.9% on seven stages against 52 to 63% for Sprayer and Mister, from 875
two-player Hard mirror matches of 60 s. That is paint share, not a win rate, and the same doc
says a Sniper trades paint for eliminations. The matrix ran one difficulty, so no comparison
with Easy/Normal/Hard follows. Bots draw Random tools as humans do
(`game/states/main_menu_weapon_select.cpp:123-133`). The roller-with-sub figure belongs to the
`ROAD#11` play review. Splatoon 3 bots and Smash CPUs play per-weapon scripts. A feature would
add per-tool roles beside `MatchBot` as subclasses, which adds a hierarchy. Objection: `POL#27`
and `ROAD#10` were accepted with this table on record, and the low share may be the tool's
intended trade.

### Bots do not answer enemy sprinklers or walls (REV#21)

`BotWorld` (`game/ai/bot_world.h:48-73`) exposes bodies, gate permissions, paint surfaces,
obstacles, supports, ramps, top paint and two sight queries, and nothing about projectiles.
`LevelBotWorld::traversal_line_of_sight` (`game/objects/level_bot_world.cpp:178-189`) tests only
`structures_.projectile_targets()`, so a standing paint wall, a `ProjectilePlanted` with an
emptied mask (`game/objects/projectile_planted.h:6-15`), does not block bot sight, and
`Level::strike_breakables` (`game/objects/level.cpp:504-520`) spends the bot's shots on it.
Nothing in `game/ai` targets an enemy sprinkler. The exposure is short: the wall stands 4 s and
takes 15 hits, the sprinkler 8 s and 3 hits. `ROAD#11` slice 6 covered when bots use subs, not
how they answer them. Splatoon players shoot sprinklers on sight. A fix adds a breakables query
to `BotWorld`, sprinkler targeting when no enemy is engaged, and the wall as cover in sight.
Objection: it widens the `HYG#40` value-only interface for a 4 to 8 second effect, and is better
raised after the `ROAD#11` play review.

### Bots choose territory without teammate awareness (REV#22)

`MatchBotObservation` (`game/ai/match_bot.h:18-85`) lists enemies only, `choose_territory`
(`game/ai/match_bot.cpp:128-172`) costs candidates by horizontal distance plus twice vertical
distance from the bot alone, and no teammate or ally term exists in `game/ai`. Whether two
same-team bots duplicate work was not checked: each bot has its own `BotRandom` sequence and
sampled territory, tiles one bot paints leave the other's candidates at the next replan, and
spawn points differ, so overlap may be small. The record says only that existing outcomes do not
establish team coordination (`docs/feature-roadmap.md:792-793`) and that the matrix was 2P only
(`docs/gameplay-tuning.md:680-684`). Splatoon's spread-from-spawn habit and Rocket League bot
rotation motivated it. The first step is a measurement: painted over paintable share for two-bot
teams on the existing headless runner. Only if that shows waste does a cost penalty near an
ally's goal follow. Objection: two-bot teams are a minority roster in a four-seat couch game.

### Bot finish window sized without own paint refill (REV#23)

The bot observation carries only the handicap's `paint_refill`
(`game/ai/match_bot_driver.cpp:147`, `game/ai/match_bot.h:39-40`), while standing on own paint
refills 1.25 times faster (`game/content/tuning.json:181`, `game/objects/player.cpp:708-712`).
The value is read in one place, `MatchBot::open_finish` (`game/ai/match_bot.cpp:104-126`), which
sizes the finishing window on a support, and its comment states the base rate as a chosen
sufficient minimum. The refill hold itself ends on observed ammo reaching `reserve_resume`
(`match_bot.cpp:510-511`), so refill retreats are not lengthened; only the window's upper bound
is conservative, and a ride's hold is capped separately (`123-125`). The `ROAD#39` closing note
(`docs/feature-roadmap.md:1467-1472`) leaves the question undecided, and `POL#30` has since
shipped underfoot on. A change would pass the underfoot rate into the estimate. Objection:
tightening risks a bot leaving a support before its reserve recovers, and the handicap
acceptance case pins the 158, 138 and 125 step windows bit for bit, for a fraction of a second
saved. The decision is whether to keep the safe minimum.

### Random stage pick can repeat the last arena (REV#24)

`MainMenuStageSelect::pick_random_stage` (`game/states/main_menu_stage_select.cpp:320-324`) maps
`rand()` onto `random_stage_indices` (`game/objects/stage.cpp:4-26`) with no exclusion of the
previous stage. The pool is four (Close Quarters by shipped default, Crossfeed, Press Tower and
the Turbulence slab trial), so a repeat is about one in four. It arises only across separate
Single Match or series starts on Random, because the default Rotate rule already moves through
the Random stages between rounds (`game/match/series_stage_rule.h:38-49`). Reproducibility is
not affected: the Debug `seed` command and `DebugDirector::round_seed` reseed `rand()` so menu
picks repeat (`game/debug/debug_director.h:140-146`), and Release has no seed. Smash and Mario
Kart avoid immediate repeats; TowerFall deals from a shuffled set. A fix excludes the
last-played stage when the pool has more than one entry, or deals from a refilling bag.
Objection: it adds a last-played input to a pure function that the `stage_selection` acceptance
case pins, for a minor annoyance rather than a defect.

### Solo play has no graded stage goal (REV#25)

`StageRecord` (`game/save/match_records.h:12-25`) stores versus played, wins by side, draws,
best winning share, practice played and best coverage, with no difficulty, duration, variant,
tool or bot field. The roadmap lists stage-only keying among defaults chosen without an author
call so they can be revisited (`docs/feature-roadmap.md:3964-3968`), and persona P3 wants
something to beat next time (`docs/design-process.md:134-137`). A timed solo run with coverage
already exists (story S5). TowerFall's medal-graded Trials and per-difficulty Quest clears
motivated two shapes of one proposal: per-stage beaten Easy, Normal and Hard marks, or bronze,
silver and gold coverage thresholds at a fixed duration, shown on Stage Select and Results. No
doc mentions either, so the idea is untracked rather than declined. It brushes three recorded
choices: no separate variant leaderboard (`ROAD#22`, `feature-roadmap.md:2216-2218`), no
lifetime stats or unlocks (`2494-2502`), and solo remains practice (`22-31`). It also needs a
save version bump, copy in every locale, painter pins, and thresholds that are provisional
numbers. The author decides whether it is a mode.

### Stage Select does not mark arenas from trials (REV#26)

The manifest lists 13 levels (`game/content/manifest.json:31-45`) and Stage Select offers them
in that order (`game/objects/stage.h:23-24`). `game/content/presentation.json:282-294` names
seven of them Turbulence Yard or a Turbulence Yard trial; the other six include the Movement
Course workshop and the two superseded originals. `Stage` has only `include_in_random` and
`user` (`stage.h:14-21`), and `game/states/main_menu_stage_select.cpp` has no grouping, filter
or Random mark. One Yard entry, the Slab Trial, is itself an accepted Random arena, so Trial in
a name does not mean workshop. Smash lets players see and toggle Random stages, TowerFall groups
arenas by set, and Splatoon never lists test maps. The 2026-11-05 call was that originals and
comparisons stay selectable (`docs/polish-turbulence.md:19-24`), which a change of order or
marking does not contradict. The cheapest forms are a manifest reorder with a rename of the slab
trial, or an In Random mark; a sectioned page is M and needs painter re-pins. A level-file group
field would touch the engine-free `level_format` contract and the editor. The author picks the
form.

### No Random arena uses the gate ferry or lifts (REV#27)

The Random pool is four stages: `crossfeed`, `press_tower` and `turbulence_slab_trial` set
`include_in_random` true and `close_quarters` takes the shipped default
(`game/objects/stage.h:19`); the other nine set it false. Gate objects occur only in
`turbulence_gate_trial.json`, ferries only in `turbulence_ferry_trial.json` and lifts only in
the two lift trials; plain horizontal movers reach Random through the slab trial. Every Yard
file has a single visual entry, so they are blockouts. Splatoon ships its lifts and sponges
inside rotation stages, and INVERSUS builds each map on one tactical question. Parts are already
tracked: art through the accepted arenas is `POL#16`, bot lift routing is `ROAD#28`, and
`POL#21` and `POL#31` settled the arena aim on 2026-11-05. What is untracked is authoring a
finished arena around the gate or ferry and adding it to Random; the gate or ferry is cheaper
first since lifts wait on `ROAD#28`. Objection: an arena is the most expensive unit of work
here, and TODO.md says the next aim is the author's to set, so this is a candidate aim rather
than a defect.

## Player-facing UX

### Bot takeover for a missing seat (REV#28)

The missing-pad pause disables Resume and Restart until every seat returns
(`docs/feature-roadmap.md:1484-1488`, `README.md:96-99`). The only recovery is the `ROAD#4`
reclaim, where Start on a pad no seat reads takes the first missing seat
(`game/states/pause_menu.h:27-33`, `game/states/pause_menu.cpp:40-47`). The keyboard seat cannot
reclaim. When no working pad is left, or the person has gone, Quit is the only exit and it
discards a series (`README.md:60-61`). Rocket League fills a departed slot with a bot; Boomerang
Fu and Brawlhalla add and remove bots between matches.

A feature would add a pause row handing the seat to a bot, reversible by the Start reclaim, and
the same conversion on the End menu between series rounds. `MatchBotDriver` is built once from
the roster at match start (`game/ai/match_bot_driver.h:16-23`), and a mid-round change also
touches round tapes, instant replay, Boost write-back and records.

The roster lock (`docs/feature-roadmap.md:2168-2169`) is stated for team wins, which a same-seat
change does not move, but relaxing it is the author's call. A between-rounds-only slice would
deliver most of the value for less.

### HUD text floor or size option (REV#29)

HUD scale is the smaller of pane width over 1920 and pane height over 1080
(`game/objects/interface_gameplay.cpp:97-99`, `game/content/presentation.json:759,832`), so a
four-player pane is 0.5 at 1080p and 0.333 at 720p. Meter labels use `lato_black_24` and the
identity line `lato_black_36` (`presentation.json:756,826`). Only the keyline has a
one-device-pixel floor (`docs/polish-hud.md`, One-pixel keyline floor). No player-facing HUD or
text size setting exists. Drawn pixel sizes were not measured. Human readability was waived
under `POL#24`, not performed (`docs/polish-hud.md:98-102`), so this is unobserved rather than a
known defect. Split-screen console games commonly ship a HUD scale, and the Xbox Accessibility
Guidelines ask for text scaling.

A fix would be a text floor in device pixels, or an Options row (Normal, Large) multiplying the
HUD scale in three and four pane layouts. Menus are full-screen, so the case there is weaker.

The objection: a floor breaks the strictly proportional boxes the painter pins pixel-exactly,
and a larger HUD covers more of an already small pane. It wants a look on a couch first.

### Direct entry to the practice course (REV#30)

Home has four rows: Play, Options, Credits and Exit (`game/states/main_menu.cpp:326-349`). The
only pointer to practice is Mode Select's hint, "Solo practice: choose Single Match"
(`game/states/main_menu_mode_select.cpp:288-292`). A solo player then walks Mode, Count, Join,
Weapon and Stage, and browses to `movement_course`, fourth of thirteen levels with seven
`turbulence_*` variants after it (`game/content/manifest.json:29-45`,
`docs/design-process.md:310-322`). The save has no first-run flag. The workshop itself
(`POL#19`) exists and is accepted; only the entry is missing. `README.md:62-63` still calls the
course experimental. Splatoon opens on a playable tutorial, Smash Ultimate has Training in its
menus and TowerFall has top-level Trials.

A feature would add a Home row that starts the course with the pressing device and the default
tool, optionally focused on first launch, and might group the Trial stages after the accepted
arenas.

The objection: Home is an authored layout validated at six aspects, so a fifth row means layout,
art, copy and painter fixtures, and reordering stages touches the accepted Stage Select order.
The countdown's controls cards already teach the basics.

### Input accommodations beyond rebinding (REV#31)

`PadControl` has no d-pad and `PadAction` is jump, shoot, pause and sub, with the comment that
movement, aim and the stick drop stay on the sticks (`game/input/input_bindings.h:11-23`). Shoot
is read as held on pad and keyboard (`game/objects/player_input.cpp:63,118`), and the d-pad is
bound only to menu directions (`game/input/action_bindings.cpp:121-124`). One-stick play can
already shoot: the tool points the way the player faces until the first aim input, then keeps
the last aimed angle (`game/objects/weapon.cpp:541-556`). What is missing is a follow-movement
mode after that and any vertical aim. The header comments record scope, not a decision against
the options. TowerFall plays on one stick; Celeste's Assist Mode and the Xbox Accessibility
Guidelines treat hold-to-toggle as a baseline.

Each option would be a per-pad row following `ROAD#7`'s Off, Shared, On pattern, with save
fields, profile carry, copy, Controls tokens and acceptance cases. It sits beside `ROAD#24` and
`GAP#4.2`.

The objection: the Pad Buttons page is recorded as having room for one more row, and no known
player needs these. Toggle-to-fire alone has a clear case.

### Play again with last setup (REV#32)

Home has no quick-play row (`game/states/main_menu.cpp:326-349`) and no such path exists. The
saving is smaller than it looks. `GAP#6.1`, which is done, already opens Mode and Count on last
time's choices and carries each seat's controller kind, team, tool, Boost, bot difficulty and
variant (`docs/design-process.md:187,297-299`, `docs/feature-roadmap.md:149-150,1415,2209`), so
the returning walk is a run of confirm presses on pre-filled pages. Join cannot be skipped,
because devices must claim seats and profiles are worn per run and not saved
(`docs/feature-roadmap.md:3396-3398`). The shortcut would save roughly the Mode, Count, Team and
Weapon confirmations. Smash Ultimate keeps rulesets and returns to character select; TowerFall
returns a group to the same lobby.

A feature would be a Home row shown when a remembered setup exists, summarising it and going to
Join and then the Stage confirmation.

The objection: a conditional row on an authored Home layout needs copy fit for a variable
summary string and painter fixtures, and a second route into Join and Stage must hold every
setup invariant: team validity, the missing-stage fallback and bot seats.

### Mouse input in menus (REV#33)

`MenuInput` feeds the keyboard-and-mouse slot from keys only
(`game/states/menu_input.cpp:120,204,219`), and the default menu directions are keys and d-pad
(`game/input/action_bindings.cpp:121-124`). The engine-report ledger notes in passing that menus
never took the mouse (`docs/review/engine-reports.md:2347`). No row and no decision either way
was found; the keyboard contract only says menus keep fixed keys
(`docs/feature-roadmap.md:4387-4389`). The motivation is the PC expectation that menus accept
clicks, which Splatoon, being console-only, gives no precedent for.

The value is low: there is at most one such seat, and its other hand is already on the keys that
drive menus. The smallest slice is left click as proceed and right click as back on the keyboard
seat, with no hit-testing. Hover focus on rows through the UI camera is the larger part.

The objection: left click is the default Shoot and right click the default Sub, so a firing
click at the buzzer could confirm Results or End menu rows unless the arming logic covers mouse
buttons. Click without hover may also read as broken pointing.

### Non colour team channel on paint (REV#34)

`PaintTile` draws one splash and one frame tinted by team colour, with a black edge and wet band
and no per-team frame or pattern (`game/objects/paint_tile.cpp:63-93`). `GAP#2.3`
(`docs/review/gap-audit.md:150-153`) proposed a per-team style. Since then three fixed palettes,
Team A and B on each HUD and labelled results have landed (`docs/feature-roadmap.md:1518-1530`),
and `ROAD#8` checked the pairs under colour-vision simulation
(`docs/feature-roadmap.md:1921-1942`). Only the surface half remains, and no record closes or
declines it: `docs/polish-plan.md:1839-1842` calls sparse surface markings optional, to be
inspected on heavily painted terrain first. With paint underfoot shipped on (`POL#30`), whose
paint a player stands on is now a gameplay input; the tuning lines for that were not checked.
Splatoon stops at Colour Lock, so this would go beyond the model game.

A trial would be an optional per-team pattern or sparse mark drawn where the tint is applied,
behind an Options switch and judged on the colour-vision preview sheet.

The objection: terrain already uses hatching as its own surface language, the flat fill and the
reveal are built on plain colour, and every paint pixel probe would need new baselines.

### Profile name on HUD and Results (REV#35)

`player_label()` builds "Player N / device" or the bot label from `MatchSetup` and never
consults a profile (`game/states/player_label.cpp:5-13`). Results contribution rows use it
(`game/states/results_menu.cpp:331`), as do Team and Weapon Select and the missing-pad pause,
and the in-match identity text takes no profile input. The roadmap shows the name on the Join
row and the Pad rows only (`docs/feature-roadmap.md:3404-3415`). Only lifetime stats and
cosmetics are not tracked by choice (`docs/feature-roadmap.md:2496-2499`), so this display-only
step, which extends `ROAD#21`, is undecided. Smash name tags and Duck Game put a chosen identity
on screen during and after the match. Whether it is a prerequisite for a party rotation was not
assessed.

The work is passing the name into `Level` as plain text, since `game/objects` may not name
`GameData`, and fitting the widest name in the four-player identity panel, Results rows, Team
and Weapon rows and the pause list, each with startup fit validation and fixtures.

The objection: a ten-character name is wider than "Player 2", and a Results row that reaches its
award drops the award. Replacing the device label also loses the cue telling a guest which pad
is theirs.

### Respawn grace and spawn choice (REV#36)

`respawn_delay` is 3.0 (`game/content/tuning.json:198`), a player returns to the single
`respawn_position_` fixed at construction from the team-local spawn index
(`game/objects/player.cpp:114,1670`, `game/objects/player_builder.cpp:10-22,46-70`), and damage
immunity is reachable only through the Debug cheats path. The immunity half is already designed:
`docs/polish-crossfeed.md:112-120` records a damage-immune grace as a tuning value, default 0,
with paint still landing and ending on the first shot, to be added only if attended `POL#17`
play shows spawn pressure that geometry cannot fix. That play was waived on 2026-11-05 and none
was observed, so the trigger has not fired. Since then the sprinkler (8.0 s lifetime,
`tuning.json:501-509`) has landed, and user stages carry no authored spawn cover. Splatoon 3
lets a player aim their landing and arrive briefly armoured.

These are reasons to watch for spawn camping in `ROAD#11`'s attended review, not evidence of a
defect. Choosing among the team's spawns during the redraw is new and would need input, bot
policy and a HUD affordance.

The objection: pre-emptive immunity changes accepted balance values and every seeded bot trial
and replay fingerprint, and hides what the spawn trials measure.

## Presentation and feedback

### Shooter side hit and splat confirmation (REV#37)

All authored hit feedback is keyed to the victim: ticks and burst
(`docs/polish-combat-feedback.md:10-22`), shake (`docs/polish-view-shake.md:42-44`) and the four
rumble pulses (`game/content/tuning.json:36-42`). Owner credit is recorded passively and by its
own comment changes no feedback (`game/objects/player.cpp:1000-1013`). The shooter is not left
with nothing: ticks and burst are world-space and drawn in every view, and the hurt and pop
sounds reach the shared speakers. The gap is a target outside or tiny in the shooter's pane, and
a private channel. Splatoon gives the shooter a hit sound, a splat callout and rumble. A feature
would be a pulse on the aim marker in the shooter's pane and `hit_dealt` and `splat_dealt`
rumble pulses. The `player.hit_confirm` wave (`game/content/sounds/paint_exe.json:98`) is
unplayed by recorded choice under `POL#23` (`docs/audio-inventory.md:895-902`), not oversight.
Objection: rumble was accepted unfelt, there is no pad on the development machine, and a splat
cue adds the kill emphasis `POL#30` steered away from.

### Paint strips are hairline thin in the reveal (REV#38)

Paint depth is `paint.thickness`, 12.0 world units (`game/content/tuning.json:111`), and strips
are built from it (`game/objects/structure_paintable.cpp:253`, `361-364`). The reveal fits the
whole picture into the monitor glass at 0.8 of screen width
(`game/content/presentation.json:1203`, `game/objects/match_opening.cpp:549-560`). By
arithmetic, not measurement, that is about 3 px at 720p on Crossfeed and about 2 px on a
6,100-wide arena. `docs/images/polish-match-accents/reveal-720p.png` shows a 1 to 2 px line, but
that fixture has almost no paint laid, so it is weak evidence. The reveal also tints the picture
during the swatch hovers and shows colour-coded percentage cards, so the beat does not rest on
the strips alone. Splatoon's overhead shot reads because ink is area fill. The `ROAD#17` decline
rests on this view. First step: capture a heavily painted four-player reveal, which nobody has
produced. A fix would be a minimum on-screen weight or a body tint in the pulled-back states.
Objection: the reveal would then show a picture that differs from the one fought over.

### Post reveal Results pages carry no team colour (REV#39)

After a story reveal the Results page hides the fill bar, track and A/B marks
(`game/states/results_menu.cpp:377-388`). What remains is ink on paper: `A 60.0%`, `B 40.0%` and
rows reading Team A and Team B (`docs/images/polish-results-menu/revealed-720p.png`,
`series-720p.png`; copy at `game/content/presentation.json:229-246`). The winner line is omitted
after a reveal except for draws (`docs/polish-results-menu.md:29-31`), but the series line
stays. Named colours and pair labels already exist (`presentation.json:14-35`). Hiding the bar
is a recorded author decision in `docs/backstory.md`, so restoring it is not on offer; keylined
colour chips on the plates and rows are compatible with it, and the A/B letter stays as the
non-colour channel from `GAP#2.3`. Splatoon, Smash and TowerFall lead results with team colour.
The mapping was shown one screen earlier on the reveal cards, and non-story paths keep coloured
bars. Objection: the plain page is the accepted treatment, validated at six resolutions, and
colour-name copy must fit six pairs and the localisation route. It rides with `POL#16`.

### Show damage on the figure as enemy paint (REV#40)

Health is drawn only as the per-pane HUD meter (`game/objects/interface_gameplay.cpp:147-161`).
Nothing in the player draw path reads health, and there is no low-health cue. Momentary damage
does show on the figure, as the hurt pose and four ticks
(`docs/polish-combat-feedback.md:10-15`); what is missing is persistent state. The backstory
already says a splatted figure has been painted over in the other side's colour
(`docs/backstory.md:74-76`), and Splatoon shows damage as enemy ink on the character with no
health bar. A feature would be authored smear masks drawn in proportion to missing health and
clearing as it regenerates. Figures are small in quarter panes, so the smears would be a handful
of pixels and may not read, and each player already has a meter. Objection: clothing colour is
how players tell the teams apart, enemy-coloured smears erode that channel, and the art adds to
the deferred `POL#16` pass.

### No on screen time up signal (REV#41)

At `timer_ <= 0` the level switches to `zoom_out`, ends the music, plays the end sting and the
`time_up` and `zoom_out` sounds and starts the reveal, with no text or card
(`game/objects/level.cpp:283-311`). Countdown copy has only `get_ready`, `resuming` and `replay`
(`game/content/presentation.json:352-356`), and no doc discusses a time-up card. The gap is
narrow: the last ten seconds have a closing-clock panel and a per-second cue
(`level.cpp:276-281`, `game/objects/interface_gameplay.h:112`), and the pull-back through the
glass is itself a full-screen signal with two sounds. Splatoon's FINISH banner and Smash's TIME
freeze mark the exact frame input stops counting. A feature would be one string beside the
countdown copy and a short shared paper card in the countdown's drawing language, with a static
form under Reduce effects. Objection: the backstory's reveal starts exactly at the pull-back and
nobody addresses the figures, so a banner is a game-show device laid over a scripted beat.

### Crossfeed shares the Close Quarters music (REV#42)

There are three arena loops (`docs/audio-inventory.md:550`).
`game/content/levels/crossfeed.json:16-18` uses `music.close_quarters` and its sting, and both
stages are in Random, so they share one loop and one sting. That is the only real collision.
Press Tower's use of `music.king_of_the_hill` (`game/content/levels/press_tower.json:18-20`) is
a recorded choice (`docs/polish-press-tower.md:9-14`), and King of the Hill is out of Random, so
that track belongs to one stage there; the Turbulence slab trial likewise replaces its original.
Splatoon, Smash and TowerFall give each stage its own music. A fix would be one new arrangement
and sting for Crossfeed through the existing score pipeline under `game/audio/paint_exe/`; the
bank is 22.9 MB against a 40 MB target. It needs the author's decision because the audio set was
accepted under `POL#23` with listening waived (`ROAD#3`): a new track would enter the bank
unheard, and it changes the 106-wave bank, its provenance and the release asset checks.

### Per seat pan and pitch for player cues (REV#43)

The sound wrapper exposes pitch and pan (`game/carried/sound_bank_object.h:50-61`), and the
inventory records that effects are not panned because no game call passes pan
(`docs/audio-inventory.md:466`). It also floated per-seat pitch for `ui.ready` (line 392). No
decision against panning was found. With four players on one speaker pair, jump, hurt, pop and
tool loops from every seat sound alike; split-screen racers commonly bias a player's sounds
toward their pane. A change would be tuning data: a small pitch offset per roster seat for
player-owned cues, and in three and four player layouts a modest pan toward the owning pane's
column. It is the author's decision because the mix was accepted as it stands (`POL#23`) with
listening waived (`ROAD#3`), and `TODO.md` reopens audio only for a missing hook or a mix
defect, which this is not. Objection: offsets detune cues mastered against the music, the
masters are mono, panning fails in 2P and on a soundbar, and nobody would hear the result.

### Series conclusion is the ordinary Results page (REV#44)

A completed series shows the normal Results page with the series winner line, plays a one-shot
series cue (`game/states/end_menu.cpp:218-220`) and heads the End menu Series Complete
(`game/content/presentation.json:216-219`). The page does carry a tally: a win ledger reading
`Wins: A 2 - B 0 | First to 2` and the round number
(`docs/images/polish-results-menu/series-720p.png`; `series_score` and `round_heading`). What is
absent is a round-by-round strip, a match-point announcement on a deciding round and series-wide
awards; awards are per round (`docs/feature-roadmap.md:2524-2560`). Smash's series results and
TowerFall's end-of-session awards are the comparison. A feature would be a row of per-round
chips on the final page, a match-point line on the stage-entry card and the four awards computed
across the series. Objection: the ledger already says who won and by how much, and series awards
need tallies carried across rounds through re-picks and replays for a moment that happens once
per session.

### Wider awards set and a consolation call out (REV#45)

There are four awards (`game/match/match_award.h:6-14`), at most one per player and never passed
to a runner-up (`docs/feature-roadmap.md:2526-2545`), so some players in a four-player match get
none. TowerFall hands out many awards and a consolation prize; its external pages were not
checked, and the finding does not depend on them. The two halves differ. More paint-flavoured
awards ahead of splats are compatible with the `ROAD#12` design and untracked. An
everyone-gets-one rule conflicts with it: the roadmap says the order is the design, which exists
so that splats are named last (the `POL#30` answer). The proposed tallies (time on enemy paint,
longest life, sub-weapon paint, paint lost to repaint) are not collected today; each needs
passive collection in `Player` or the paintable structures, routing through `LevelEndInfo`, copy
that passes the Results width validation and an updated `results_awards` case. Objection: more
awards dilute the paint-first ordering, and the page already drops the award beside long device
labels.

### Team status strip showing who is respawning (REV#46)

The HUD draws the panel, meter, sub mark, timer and closing clock, with no roster element
(`game/objects/interface_gameplay.h:30-112`), and dead teammates and all enemies are never
marked (`docs/feature-roadmap.md:3811-3826`). Splatoon's top bar shows every player's icon,
crossed out while splatted. A feature would be a strip of roster badges by the clock, each
crossed out while that player waits to respawn, reusing the player badge and the pane-scaled
HUD. It carries no position, so the `ROAD#18` threat-marker decline does not literally cover it,
but it sits close: that decision's ground is that the other team's state is found by looking and
listening (`docs/feature-roadmap.md:3864-3871`), and an enemy-down mark is new information about
the other team on every pane. On a shared screen the pop sound and the burst in another pane
already tell an attentive player. Objection: a crossed-out enemy badge is a live kill feed in
all but name, and it adds a fourth element to quarter-size panes.

### Tell a splatted player who got them (REV#47)

Nothing in the respawn presentation names the attacker or the tool
(`game/objects/respawn_redraw.h:8-40`); there is no respawn text in the HUD or level draw
sources. The fatal projectile's owner is credited (`game/objects/player.cpp:1000-1013`), but as
a per-owner count for the awards, not a record of the last attacker and tool, so that record
would need adding. There is no respawn card and no countdown digit to extend: the author decided
on 2026-10-12 that no number is drawn while a player waits (`docs/backstory.md`, Building it),
and the wait is the camera hold on the death spot followed by the undo-history redraw. Splatoon
shows Splatted by with the opponent's name and weapon. A feature would therefore be a new HUD
element showing the attacker's roster badge and tool or sub icon. Objection: the wait was
designed as a wordless fiction beat, and an attacker badge adds the kill attribution the author
declined for the results page under `POL#30`.

## New features from the comparison

### Whole round time lapse replay (REV#48)

Watch Replay shows only the last `match.replay` seconds (`game/content/tuning.json:55`, 8.0) in
the round's own panes, although the RoundTape holds every step of the round. Nothing in
`TODO.md` or `docs/` mentions a time-lapse. Splatoon 3's Battle Replays re-create a whole battle
from a top-down camera; TowerFall's end-of-round replay is the model `ROAD#19` already borrowed.
The existing catch-up is not a watchable pass: `preparation_budget{12}` in
`game/states/game_level_replay.cpp:18-21` is 12 ms of wall time per frame and prepares a round
in about a second. Multi-step playback exists, so the feature consists of deliberate pacing (a
fixed step multiple per frame), a single whole-arena camera in the rebuilt level (it rebuilds
with the round's split layout, `game_level_replay.cpp:42-49`), a ruling on HUD and clock during
the pass, and a new End row with copy, painter and validation updates
(`docs/feature-roadmap.md:3287-3300`). It runs after the reveal, so the `ROAD#17` decline does
not apply. Objection: the End menu was already resized for six rows, `POL#15` wanted fewer steps
between rounds, and the payoff is unproven without play.

### Save the finished picture as a PNG (REV#49)

Nothing a group paints survives a match except a best-coverage number, though the theme is
choosing which colour to save. The capture and encoder exist only in Debug: `capture_stack` and
`write_png` (WIC, atomic write) in `game/debug/capture/debug_screenshot.h:13-40`, with bodies
under `#if COLOURWARS_DEV_TOOLS` (`debug_screenshot.cpp:4`). `Renderer::read_back_buffer` is
public engine API (`external/labrador/engine/render/d3d11/renderer.cpp:1012`), so this is
game-only and owes no engine report. It is not declined: `ROAD#19` says only that saving a clip
was not taken up (`docs/feature-roadmap.md:2157`), `POL#15` declined a separate View arena step
(`docs/polish-plan.md:2334-2341`) and `docs/backstory.md:178` rejects Save As copies as the
rematch device. TowerFall saves replay GIFs and Splatoon has a photo mode. The work is a Release
home for the two functions, capturing the reveal frame rather than the whole state stack, an
output folder, a menu row, copy in `presentation.json` and a check; the null backend refuses
readback, so a headless probe cannot cover the capture. Objection: it writes to the user's disk
from Release and adds an End row, so it must be an explicit action, never an auto-save.

### Elimination leaves a burst of attacker paint (REV#50)

An elimination changes no tile. The death branch in `game/objects/player.cpp:1062-1081` sets
state, animation, rumble, `combat_feedback_.eliminate(rectangle_)`, view shake and sound only.
In Splatoon a splatted player bursts into the attacker's ink. The parts exist:
`Level::burst_projectiles` (`game/objects/level.h:435`) serves the grenade, and
lethal-projectile credit already identifies the attacker for the `ROAD#12` awards. `POL#30`
(`docs/polish-plan.md:1955-2005`) ruled on underfoot, special and splat tally only, so this is
neither tracked nor declined. A slice would spawn a small burst in the attacker's colour at the
death rectangle, with a tuning block, a rule for a fall that has no attacker, a replay
determinism check and acceptance; bots would be left untouched. The objection is the decision it
needs: it makes a kill score territory directly, against the recorded stance that eliminations
make space to paint and the reasoning behind the tally decline
(`docs/polish-plan.md:1997-2002`), and the five-tool balance accepted on 2026-11-05 was set
without it while the `ROAD#11` play review is still owed.

### Own colour wall climb or wall jump (REV#51)

Side faces are painted and scored but play no part in movement. Only top faces count
(`game/objects/structure_paintable.h:85-87`, side and bottom paint never counts), nothing in
`game/` climbs, clings or wall-jumps, and `docs/level-design-research.md:28` lists no wall climb
as a map constraint. Splatoon's squid form swims up own-ink walls and Splasher's sticky paint
allows wall running. `POL#30` notes that nothing recorded whether the absence was a decision and
then ruled on three other mechanics, so wall traversal is unruled rather than declined. This is
a new movement mechanic, not a missing feature: Crossfeed, Press Tower, the Turbulence slab, the
jump and reach numbers, bot routing and the editor's reach aids were all built and accepted on
2026-11-05 assuming none. Even a bounded slice (slower fall against an own-colour face plus one
wall jump) needs a side-face ownership query, bot awareness and an arena recheck. The decision
it needs is whether to trial it at all; if so, as a default-off trial in the manner of `POL#36`.

### Paint keyed bounce pad object (REV#52)

The paint-object family is the ferry, the gate and the lifts. The remaining row, `ROAD#29`
sponges, is unscoped and waits on resize, crush and changing-area contracts
(`docs/feature-roadmap.md:2624-2628`, `:2838-2841`). No bounce pad, trampoline or spring appears
anywhere in `docs/` or `TODO.md`. A static pad that launches only the team holding its top
paint, keyed on `top_paint_team`, avoids those contracts because its scored area is fixed.
Splasher's bouncy paint motivated it; reviewers there found free bouncing hard to control, which
argues for a fixed pad with one launch vector. It is still a new final class, a level-format
object kind with validation and editor support, art and a cue, an explicit trial stage, a new
vertical-impulse path in `Player`, and bots that do not know it, the gap `ROAD#28` still carries
for lifts. The roadmap promotes a slice only once its owner records the choices and acceptance
(`:2843-2845`). Objection: three paint objects already live only in trial stages with unscoped
bot routing, and a fourth adds content before any is in an accepted arena.

### Eraser stage object that clears paint (REV#53)

Paint only ever arrives. `PaintTile::paint` (`game/objects/paint_tile.cpp:191-200`) assigns the
incoming team and no paint source passes `PlayerTeam::none`; the eraser in
`game/contracts/level_format.h:212-237` is a terrain material name only, and `docs/` has no
unpaint or wipe concept. Splasher's water and de Blob's ink strip colour. A stage object that
returns tiles to neutral on a timer or along a path would keep the middle of a match contested.
The costs are wide: tiles carry wet age, splash and runoff strips that assume paint only
arrives; the ferry, gate and lifts read ownership counts that would now fall; bots' finishing
logic counts unpainted tiles (`docs/feature-roadmap.md:1131`); underfoot support changes; and
the awards' laid and retaken tallies need a rule. Whether PaintCredit already ignores
transitions to none was not checked. With a trial stage, art and a cue it is large. Objection:
it breaks the invariant that paint is monotonic away from neutral, which scoring, awards, bots
and paint objects rely on, for a speculative effect; `docs/backstory.md:178` also dropped Undo
to grey, though as the rematch device.

### Coverage over time strip on Results (REV#54)

The reveal gives a group one final number per team and nothing about how the round went. No
coverage history is recorded: a search of `game/` for history or timeline finds only unrelated
files such as `match_records.h`. The withheld-score rule (`docs/feature-roadmap.md:2505-2521`)
covers play before the reveal, so a strip shown afterwards does not conflict with it. No
comparable game shows this; it follows from the withheld-score design. The feature would sample
`paint_total()` once a second, store the samples in a replay-neutral way, and draw a two-colour
strip on Results. Most of the work is layout: the Results paper is tightly validated, awards
must clear the widest row by 24 units and rows are painter-checked
(`docs/feature-roadmap.md:2550-2560`), and the strip needs copy and colours in
`presentation.json`. It would also give some evidence for the `POL#30` reopen triggers, a minor
benefit with the attended sessions waived. Objection: Results was decided to stay quick and
uncluttered under `POL#15`, and awards show no values by design, so a chart adds number-like
information to a page that deliberately carries little.

### Untimed group exploration of any arena (REV#55)

Untimed play exists for one player on a stage with lessons and nowhere else. `Level::practice()`
is `!lessons_.empty()` (`game/objects/level_practice.cpp:13-16`), lessons exist only in
`movement_course.json`, and practice is reached only by menu solo on such a stage
(`game/states/game_states.cpp:267-275`); solo on any other stage is a timed run, and every
multi-player round is timed and recorded (`docs/feature-roadmap.md:3974`). Splatoon's Recon lets
a group walk any stage with no opponents. The practical audience is a group vetting a `ROAD#16`
user stage, for whom a record on that stage is low stakes, and Debug launch arguments already
serve the author. The feature would be an untimed option with no clock, reveal or record, left
from Pause; a cheaper variant lets practice be chosen for any stage in solo. It needs its own
ruling: the author's decision says an option adds no end condition or result path
(`docs/feature-roadmap.md:35-43`), and a session with no winner is outside that. Objection: the
gain over playing a round and ignoring the result is small.

### Sub lesson station in Movement Course (REV#56)

The ten lesson stations in `game/content/levels/movement_course.json:56-131` (drop through,
short hop, held jump, running jump, low ceiling, steps, paint a high face, recovery, practice,
empty and refill) include none for the Sub action, and the seven `ROAD#11` slices record no
course change. Subs are taught elsewhere: each has a name and description on Weapon Select
(`game/content/presentation.json:341-349`), the Controls page lists Sub for pad and keyboard
(`:122`), the countdown reminders name it (`:360-363`) and the HUD has a readiness mark and cost
tick. The fix is one JSON lesson plus the practice_workshop and movement_course checks and
text-fit validation. A practice target in the manner of Splatoon's test range is a separate,
larger item and is not scoped here: a damageable non-scoring figure with a damage readout is a
new object, HUD element and copy, and it would put damage numbers into a game that shows none.
Objection: `ROAD#11` still awaits the author's play review, so a lesson written before subs are
accepted may be rewritten.

### Sniper charge or readable warning (REV#57)

The sniper fires a straight jet every second with nothing an opponent can read first.
`game/content/tuning.json:297-316` gives it `shoot_interval` 1.0 and `starting_vel_length` 2000
with no charge keys, and `game/objects/weapon_sniper.*` contains no charge, telegraph or sight;
the shooter does get a `sniper.ready` cue. Splatoon's chargers must be held to charge and show a
laser line that opponents see. A change would be a short hold-to-charge whose line draws in
every pane, as behaviour in C++ with durations in tuning, plus bot ballistics. This is a
mechanic proposal, not a defect: the five-tool values were accepted on 2026-11-05, enemy threat
markers were declined the same day, and no readability problem has been observed because the
attended sessions were waived. The H6 criterion in `docs/level-design-research.md` cited by the
original finding was not re-read. If it belongs anywhere it is inside `ROAD#13`. The decision it
needs is whether to reopen an accepted tool and change its input model on speculation.

### Party rotation for more than four people (REV#58)

Four seats is a hard limit: `max_players = 4` carries a static_assert against the four-player
layout (`game/match/match_rules.h:14-20`), `Session` is a per-team win ledger for a fixed roster
(`game/match/session.h:8-10`), and records stay per stage, not per person
(`docs/feature-roadmap.md:3410`). No rotation, tournament, winner-stays or sit-out format
appears in `docs/` or `TODO.md`. Nidhogg 2 runs eight-player tournaments on a two-player game.
The feature would take up to eight `ROAD#21` profile names, name who plays and who sits out each
round, and show standings per person. It is not declined by name; the nearest decline, a
lifetime stats screen, is not the same as a ledger within one session. It cuts against recorded
design twice: the series locks teams and roster because the ledger counts team wins
(`docs/feature-roadmap.md:2167-2169`), and random team pairs make the winner a team, so
winner-stays-on needs a per-person rule. Objection: a group can pass pads by hand at no cost,
and the feature adds a second ledger and new pages for a case not observed, the attended
sessions having been waived.

### Attract match behind the title (REV#59)

Two things sit here. The front end's lack of colour and figures is already `POL#16`:
`backgrounds.title` and `home` are flat B0B0A8 (`game/content/presentation.json:511-514`) and
the remaining UI art waits on the author setting it as the next aim. The attract match is new;
`docs/` has no mention of attract, demo mode or kiosk. Smash Bros. and arcade-lineage party
games run a demo behind the title; no indie comparable was shown to. The shell cannot host it
today. `StagePreview` (`game/states/stage_preview.h:14-35`) is a UiWidget holding bare
GameObjects and CollisionObjects, not a `Level`, with no players or simulation. `Level` throws
without a human view (`game/objects/level.cpp:68-69`) and `LevelBuilder` refuses `human_count()
< 1` (`game/objects/level_builder.cpp:40-44`), so an all-bot match needs a spectator view. A
live match under the menu also needs a new stack shape beside the rule that a match is pushed
above `GameMenu`, a menu audio answer and a startup cost budget. Objection: that is a spectator
camera and a human-less `Level` nothing else requires, for a local game with no storefront, and
`POL#16` would address the first impression more directly.

### Territory free use of the spare 3P quadrant (REV#60)

The three-human layout leaves the fourth quadrant unused (`docs/feature-roadmap.md:2146-2148`).
The `ROAD#17` decline ends by saying that a territory-free use of that quadrant (clock, team
colours, the person's desk) would be its own row (`docs/feature-roadmap.md:2519-2521`), and
`TODO.md` has no such row, so this is invited but untracked, and not declined. No comparable
game motivated it. The same paragraph gives a reason that still partly applies: what players
could see would depend on how many pads are plugged in, so anything shown there must repeat what
the panes already carry, such as a shared clock or the rosters with profile names. That caps its
value. The work is a fourth viewport or draw region in a three-human layout, presentation data
and painter coverage at three players. It fits naturally inside `POL#16`'s deferred UI pass and
should ride with it. Objection: only three-human matches benefit and the content must be
non-essential by the author's own reasoning, so it is finish, not function.

## Tooling, editor and distribution

### No sanitizer or static analysis leg (REV#61)

Nothing outside `external/` references `/fsanitize`, `/analyze`, clang-tidy, cppcheck or ASan;
the only hit for "analyze" is the frame-pacing analyser at `scripts/check-frame-pacing.ps1:164`.
`CMakePresets.json:15-24` has two presets, and the engine's only hardening flag is `/sdl`
(`external/labrador/cmake/settings.cmake:19`). The 97 cases in `scripts/acceptance/cases.psd1`
link the real game library headless, which is the workload SDL, Godot and Box2D run under ASan.
Labrador states its own absence in `external/labrador/SECURITY.md:68-71`, so the engine half is
not this repository's to add.

A fix is an `x64-asan` preset and a non-required leg in `nightly.yml` running the default cases,
with `/analyze` as a report first. The objection: MSVC ASan must also instrument the engine and
vcpkg libraries and needs its runtime DLL, `/analyze` under `/WX` needs a baseline that
`labrador_settings` owns, and the payoff is speculative because the probes already pass under
Debug's checked iterators.

### No mutation probe for outside file readers (REV#62)

No generative or mutation test exists: "fuzz" has no code hits, and the negative checks are
hand-written fixtures such as `scripts/acceptance/match_records.cpp:236-252` and `358-388` and
`scripts/acceptance/settings.cpp:111`. `README.md:172-193` invites players to drop stage files
into the user stages folder, and `docs/level-format.md:101-102` warns that a JSON number can
overflow a game float although the schema accepts it. SDL, stb and Godot fuzz their loaders.

The cheap form is one opt-in case in `cases.psd1` that mutates the shipped level and save files
for a fixed seed count and requires a return or a refusal, never a crash or hang. The replay
codec is Debug-only and stays out of scope. The reader line ranges the finding cited were not
checked.

The objection: the likeliest findings, unbounded nesting depth and memory in rapidjson, are
already disclosed in `external/labrador/SECURITY.md:68-77` and would be ledger reports, and the
only stage author today is the game's author.

### No crash dump and no Release symbols (REV#63)

`docs/failure-reporting.md:111-118` scopes structured exceptions, `noexcept` termination and
exceptions crossing Win32 callbacks out of that change; it is a boundary statement, not a ruling
against crash capture. No `SetUnhandledExceptionFilter`, `set_terminate`, `MiniDumpWriteDump` or
`WerRegister` call exists in `game/`, `editor/` or the engine. Release sets no `/Zi` or
`/DEBUG`, and `.github/workflows/ci.yml:377-385` uploads only the ZIP, so no PDB is kept. Godot
and Crashpad users write a dump or backtrace on a hard crash.

The minimal slice is a filter and a terminate handler writing one local dump beside
`last-error.log`, and a PDB artifact keyed by revision. Callback containment is Labrador's and
needs an exhibit first.

The decision it needs: a minidump holds memory that the privacy rules at
`docs/failure-reporting.md:42-46` deliberately exclude from the record, and with no release or
tag published there are no field crashes yet, so this pays only from the first tag.

### Level editor is not shipped to players (REV#64)

`cmake/release_package.cmake:25` installs only the game, and `README.md:362-363` says the editor
is not in the Release ZIP, so the player-facing user stages feature has no player-facing tool.
TowerFall, Duck Game and Ultimate Chicken Horse ship theirs. Packaging the executable as built
would not work: `editor/CMakeLists.txt:47-49` compiles absolute checkout paths that
`editor/editor_window.cpp` uses at lines 35, 133, 320-321, 327 and 472.

The work is content lookup beside the executable, hiding registration and source-level flows
without a checkout, dialogs defaulting to My Stages, a player guide, and package and startup
checks.

`docs/design-process.md:153-158` and `255` record the absence as a standing condition, and
`401-403` say shipping means redrafting P5, J4, F6 and F7. No document decides either way. The
objection: the couch guest is the decided persona, stage-authoring players are still a
hypothesis, and the editor was accepted without attended review (ED#5), so this creates a second
supported product before any release exists.

### Editor has no test play command (REV#65)

The editor's whole menu is File, Edit, View and Help (`editor/editor_window.cpp:153-180`), and
`editor/` contains no `CreateProcess` or `ShellExecute` call. LDtk and Tiled offer a run-game
command, and TowerFall's editor tests a level with one button. The loop today is shorter than it
looks: the game rescans the stages folder whenever Stage Select opens outside a series, so with
the game left running it is Ctrl+M, switch windows and reopen Stage Select. The editor is
schematic by design (`docs/level-editor.md:3-8`). Whether the Debug `+stage` argument accepts a
My Stages id was not checked.

A feature would be a Play Stage item that saves and launches the game on that stage, which needs
a launch argument in a Release build that deliberately takes none (`README.md:282`). The cheaper
item, Open My Stages folder, is the smallest sharing slice at `docs/design-process.md:389-392`.

The objection: it pays only if the editor ships (`REV#64`), and adding a Release argument is the
author's call.

### Editor lacks multi select and recent files (REV#66)

The editor has no multi-selection (`docs/level-editor.md:1307` says so, and `editor/` has no
marquee or selection-set code), no textures on the canvas (schematic by design,
`docs/level-editor.md:3-8` and `676`), and no recent-files list, drag-and-drop open or autosave:
the menus at `editor/editor_window.cpp:153-180` have none, and no `WM_DROPFILES` or
`DragAcceptFiles` handling exists. Edit already has Duplicate, Mirror copy, Copy and Paste for
single objects. Tiled and LDtk treat marquee select, live tiles, recent projects and
backup-on-save as baseline.

Of the list, multi-select with group move is the one with real authoring value; recent files and
autosave are conveniences; a textured preview contradicts the stated schematic scope and would
duplicate the game's renderer in a GDI tool.

The objection: the editor was accepted as built on 2026-11-05 (ED#5), its only user is the
author, who built the shipped arenas with it, and the Tiled comparison assumes an audience it
does not have.

### Distribution is one unsigned GitHub ZIP (REV#67)

CPack produces a ZIP only (`cmake/release_package.cmake:85-92`), the publish job creates only a
GitHub Release (`.github/workflows/ci.yml:399-435`), and `README.md:19-21`,
`docs/HOW-TO-PLAY.txt:19` and the release notes coach players through SmartScreen. No document
mentions itch.io, butler, winget or code signing. Mindustry and most local-multiplayer indies
push to itch.io from CI. The channel is unstarted rather than weak: no release or tag exists,
and `README.md:12-15` says so.

The pieces are a butler step in the publish job, which does not touch `gate`, with the page,
account, secret and store copy as author work; a SHA-256 beside the asset, which adds little
over GitHub's own digest; and signing, which is low value because SignPath becomes the named
publisher and neither route gives instant SmartScreen reputation.

The decision it needs: the first tag is the author's call and is not yet made, and a storefront
page is a public-presence choice, not an engineering gap.

### Steam release is not scoped (REV#68)

`ROAD#23` and `docs/feature-roadmap.md:2074-2083` and `2482-2485` say Steam Input covers
non-XInput pads if the game ships on Steam, and no document scopes a Steam release. Online play
was out of scope for the 2026-10-27 comparison (`docs/feature-roadmap.md:2092-2094`), which is a
scope note for that comparison and not a ruling against stream-based play. TowerFall and
INVERSUS rely on Steam Input and list Remote Play Together.

Scoping would cover the partner fee and store page, Remote Play, button prompts and a Steam Deck
pass. The Steamworks claims behind it (pad emulation for XInput-only titles, automatic Remote
Play Together eligibility, Deck behaviour under Proton with XAudio2) were not checked.
`ROAD#23`'s grading does not move, because it would depend on a decision nobody has made.

The objection: nothing is released on any channel, the project is free and MIT, and choosing
Steam would remove the pressure that makes `ROAD#23` an engine report with an exhibit, which is
what the first-client role cares about.

### Stream based online play is undocumented (REV#69)

No document, `README.md` or `docs/HOW-TO-PLAY.txt` mentions Parsec, Remote Play, Steam Deck or
Proton. The limits that matter are recorded: XInput only and one keyboard seat
(`docs/feature-roadmap.md:2074-2088`), with online play out of scope for the 2026-10-27
comparison (`2092-2094`), which excludes netcode and not a how-to paragraph. TowerFall,
Boomerang Fu and Duck Game are commonly played this way.

Remote Play Together and Deck Verified need a Steam release (`REV#68`) and are moot today. What
remains is a Parsec check with the current ZIP and one short, hedged paragraph in
`docs/HOW-TO-PLAY.txt`. The technical points behind it, guest pads appearing as XInput and an
overlay triggering the focus-loss pause, were not verified.

The decision it needs: the check takes two machines, a remote friend and pads, this machine has
none, and attended pad sessions were waived on 2026-11-05. Documenting an untested third-party
tool as a way to play promises behaviour nobody has checked.

## Proposed and not given rows

Reviewers proposed these too. Each matched a decision the author has already
recorded, or a row that already exists, so none has a `REV` row. A later gap
list should read them as intended.

Already decided:

| proposal | where it was decided |
|---|---|
| Refuse unknown keys in level files | `docs/level-format.md`: unknown keys are ignored metadata by design, and the editor preserves them |
| Hysteresis on the team gate's majority rule | `docs/feature-roadmap.md`, `ROAD#30`, accepted as built 2026-11-05 |
| A padded pseudo-locale to expose length and glyph problems | `docs/localisation.md`, `GAP#L11`, closed 2026-11-05 with no language chosen |
| A keyboard domain wider than A-Z and Space | `docs/feature-roadmap.md`, keyboard actions slice (`GAP#4.2`), decided 2026-11-05 |
| Directional hurt ticks | `docs/feature-roadmap.md`, `ROAD#18`: threats get no marker of any kind, 2026-11-05 |
| A final-minute music lift | `docs/feature-roadmap.md`, "Not tracked, by choice" |
| Series autobalance for the trailing team | `docs/feature-roadmap.md`, "Not tracked, by choice" |
| A browser build on the engine's wasm presets | `AGENTS.md`, "What this is": the other backends are the engine's business |

Already tracked:

| proposal | row |
|---|---|
| Planted subs riding movers and reacting to their surface | `ROAD#11`, left for later by plan |
| More tool archetypes, split into single-tool slices | `ROAD#13` |
| Visual screen transitions | `POL#16`, the residue of `GAP#5.1` |
| More match variants, stackable options and a blank-canvas variant | `ROAD#22`, revisited only when the author selects one |
| Cross-build replays or a state hash as a regression test | `DBG#2` |

One candidate, stage files carrying author and version metadata with an import
path in the game or editor, was not verified separately. It belongs with
shipping the editor (`REV#64`) and with `docs/design-process.md` F7.
