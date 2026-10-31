# Design process: personas, user stories and journeys

Opened **2026-10-29** at the author's request. ColourWars has grown by
features, polish slices and attended reviews. Its intended players and what
each needs from a session are spread across the polish plan, the feature
roadmap, the gap audit and the arena records. Nothing states them once and
traces the work to them. This doc owns that formal pass and the `DES#` rows.

## What exists to draw on

Persona and story evidence is already recorded, but none of it is written as
personas or stories:

- Couch play and its constraints: local split screen for 1-4 players, one
  mouse player at most, pads on the other seats, and bots on idle seats
  ([feature-roadmap.md](feature-roadmap.md), `ROAD#10`, `ROAD#24`).
- Fresh-player observation is already a gate: the finish review (`POL#24`)
  and the Movement Course practice workshop (`POL#19`).
- Comfort and accessibility: per-seat toggles (`ROAD#7`), handicaps
  (`ROAD#5`) and reduced effects ([polish-reduced-effects.md](polish-reduced-effects.md)).
- Stage authors: the editor and My Stages ([level-editor.md](level-editor.md),
  `ROAD#16`).
- Deliberate Splatoon borrowings, which a story must not report as gaps
  (gap audit 1.1 and 2.1).
- The game's second audience: Labrador developers reading it as the engine's
  full-OOP client (AGENTS.md, [oop-guide.md](oop-guide.md)).

## Deliverables

1. **Personas** (3-6). Each one gives the player's context (who is on the
   couch, which device, experience with shooters), goals, frustrations and
   what a good session looks like. Every claim cites the doc or code it rests
   on. A persona with no evidence is marked as a hypothesis.
2. **User stories** in "As a ..., I want ..., so that ..." form, grouped by
   journey stage. Each story carries acceptance criteria and the feature, row
   or acceptance case that serves it today, or "none".
3. **Journeys** for first launch to first match, a returning group's evening
   (series, rematch, settings), solo practice, and authoring and playing a
   user stage. Each step points to the screen or state that serves it.
4. **Traceability and gaps.** A story with no serving feature becomes a
   candidate row in its owner doc. A feature with no story is listed, not
   removed.

## What this does not decide

The personas and priorities are the author's to accept. The draft records
evidence and options, not a direction. The pass does not reorder NOW, change
tuning, reopen closed Splatoon borrowings or close any attended gate.

## Draft personas, stories and journeys (`DES#1`)

**Drafted 2026-10-30; waits on the author (`DECISION`).** Deliverables 1-3
are below: an evidence inventory, six personas, 49 stories in seven journey
stages and four journeys. The draft describes the game at this revision and
changes no code, content, tuning or order. Every persona claim and story
cites an inventory entry (`E1`-`E24`) or code, or is marked **(H)**: a
hypothesis nobody has observed. "Attended" in a mapping means the serving
feature is built but its human review is still an open `DECISION` row; the
mapping does not close it. Stories are numbered by stage (`A1`, `B3`); the
numbers are handles for `DES#2`, not priorities.

The draft's acceptance (every persona and story cites a source or is marked a
hypothesis; every story has acceptance criteria and a mapping or "none") was
checked row by row when it was written. What the author's review decides is
listed under [Open points for the author](#open-points-for-the-author). Until
then the personas and stories are a proposal, and `DES#2` stays blocked.

### Evidence inventory

| Id | Source | What it establishes |
|---|---|---|
| E1 | [README](../README.md), AGENTS.md "What this is" | Local split screen for up to four players, two teams; the score is painted surface; Windows only. |
| E2 | [Input devices and seats](feature-roadmap.md#input-devices-and-seats) | XInput pads only; one keyboard seat, which aims with the mouse; a second keyboard player is a design question nobody has asked for. |
| E3 | [Explicit joining and reconnect pause](feature-roadmap.md#first-increment-explicit-joining-and-reconnect-pause) | The device that leaves Player Count is P1 and confirms the roster; others claim seats; a disconnect pauses the match; a pad in another slot reclaims with Start (`ROAD#4`). |
| E4 | [Balanced random teams](feature-roadmap.md#balanced-random-teams), gap audit 1.5 | X or R shuffles teams; a one-sided confirmation is refused. |
| E5 | [AI bot players](feature-roadmap.md#ai-bot-players-road10) | The author asked on 2026-10-27 for bot teammates and opponents at Easy/Normal/Hard; bot-free solo practice must stay; usefulness is unattended (`ROAD#10`). |
| E6 | [Match scope decisions](feature-roadmap.md#match-scope-decisions-2026-10-15) | Timed painting is the only ruleset; Single Match, Best of 3 and Best of 5; solo is practice with coverage results. |
| E7 | [Gap audit](review/gap-audit.md) 1.3 | "Solo is the most likely first session, and the only way to learn the controls before friends arrive" (a frozen record). |
| E8 | Gap audit 1.1 and 2.1 | The withheld score and the random authored palette pair are deliberate Splatoon borrowings; what survived was per-player attribution and an accessibility route around the roll. |
| E9 | [Remembered match setup](feature-roadmap.md#remembered-match-setup-gap61) | The last normally started setup is restored at launch; Random intent is not remembered. |
| E10 | [Sessions and paint contribution](feature-roadmap.md#fifth-increment-sessions-and-paint-contribution), [results awards](feature-roadmap.md#results-awards-road12) | Series, draws replayed, per-player contribution and at most one call-out per player; sessions are not saved. |
| E11 | [Comparable-game gaps](feature-roadmap.md#comparable-game-gaps-2026-10-27) | Value was rated by "how much a couch group would notice the feature"; re-picks between rounds, profiles, instant replay, aim assist (`ROAD#24`) and what is deliberately not tracked. |
| E12 | [Polish plan §7](polish-plan.md#7-acceptance-and-delivery) | The finish review requires fresh-player observations of responsiveness, readability, tool identity and what feels unfinished (`POL#24`). |
| E13 | [Practice workshop](polish-movement-course.md#practice-workshop), [the proposal](polish-plan.md#let-players-learn-by-making-a-mess) | Keep it only if a fresh player can join a match knowing how to move, recover paint and choose a tool without being told (`POL#19`). |
| E14 | [Per-seat accessibility toggles](feature-roadmap.md#per-seat-accessibility-toggles-road7), [reduced effects](polish-reduced-effects.md) | A motion-sensitive player could not opt out alone on a shared screen; each pad now overrides View shake, Aim marker and Reduce effects, and reduced effects keep every state signal. |
| E15 | [Palette accessibility](feature-roadmap.md#second-increment-palette-accessibility), gap audit 2.1 | Pink/green is a red-green confusion pair; Options offers fixed pairs; the HUD spells out the team. |
| E16 | [Handicaps](feature-roadmap.md#handicaps) | Per-seat Boost (Off/+1/+2), chosen by the author 2026-10-27; values unattended (`ROAD#5`). |
| E17 | [Pause controls/settings](feature-roadmap.md#fourth-increment-pause-controlssettings), [action bindings](feature-roadmap.md#action-bindings-gap42), [player profiles](feature-roadmap.md#player-profiles-road21) | A controls reference, per-pad deadzones, music/SFX volume, rebinding, and named profiles that carry a person's settings between pads. |
| E18 | [Make couch play comfortable](polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds) | The resume count-in, so a resume or focus return does not surprise anyone (`POL#18`). |
| E19 | [Backstory: Building it](backstory.md#building-it), [countdown](polish-countdown.md) | The opening and reveal, skippable with A, B or Start; solo practice skips both; the countdown shows a controls card per device present. |
| E20 | [Level editor](level-editor.md#play-the-level), [User stages](feature-roadmap.md#user-stages-road16) | A player who saves a stage should play it without a rebuild; a broken file costs one stage; user stages are never Random (provisional). |
| E21 | [OOP guide](oop-guide.md), AGENTS.md "What this game is for" | A second audience: Labrador developers reading the game as the engine's full-OOP client. |
| E22 | [Polish plan: the indie standard](polish-plan.md#what-the-indie-game-standard-means-in-practice) | Moving and painting in an empty room should be enjoyable; the finish must hold through pause, death, respawn and rematch. |
| E23 | `game/states/main_menu.h`, `game/states/game_states.h`, `game/states/main_menu_players.cpp:497` | The menu path Title, Home, Mode, Player Count, Join, Team, Weapon, Stage; one player skips Team Select; a match is a `GameLevel` pushed above `GameMenu`. |
| E24 | [TODO.md](../TODO.md), NOW "Waiting on the author" | Which of the above are still attended gates. |

Nothing in the repository records a playtest with people other than the
author, or their shooter experience, age or how often a group meets. Every
claim of that kind below is therefore **(H)**.

### Personas

Each persona is a role a person takes in a session, not a demographic; one
person can be P1 on Friday and P3 on Sunday. P4 is a facet any seat can carry.

**P1, the host.** *Context:* owns the Windows PC (E1) and starts the session;
the device that leaves Player Count is P1 and confirms the roster (E3). Often
the one keyboard-and-mouse seat, since there is only one (E2) **(H: which
device the host uses)**. Knows the game better than the guests **(H)**.
*Goals:* get from launch to a match quickly, ideally with last time's setup
(E9); fair teams without an argument (E4); full seats when the group is short
(E5); several rounds in a row (E6, E10). *Frustrations the record names:*
re-entering setup at every launch (`GAP#6.1`, answered); a pad dropping out
mid-match (E3, answered). *A good session:* **(H)** an evening of rounds in
which nobody reaches for a manual and nobody restarts the program.

**P2, the couch guest.** *Context:* handed a pad, may never have played; the
finish review exists to watch this person (E12). Shooter experience varies
**(H)**. *Goals:* know what wins (painted surface, revealed at the end; E8,
E19), which figure is theirs (gap audit 2.4), what each tool does, and the
controls before the first shot (E19). *Frustrations:* **(H)** being outaimed
by the one mouse player (E11 gives it as `ROAD#24`'s reason, not as an
observation); **(H)** not knowing why they lost. *A good session:* joins a
match knowing how to move, recover paint and choose a tool without another
person explaining the game (the test in E13, applied to the whole game).

**P3, the solo player.** *Context:* alone at the PC; the audit called solo
the most likely first session (E7). *Goals:* learn movement and tools without
a clock (E13); a real opponent when nobody else is there (E5); something to
beat next time, such as a stage record (`GAP#6.2`). *Frustrations the record
names:* solo once announced a 100% victory against nobody (E7, answered by
practice results, E6). *A good session:* **(H)** leaves practice, then wins or
loses a match against a bot that played by the same rules (E5).

**P4, the comfort-sensitive seat.** *Context:* any seat whose player is
motion-sensitive (E14), has a colour-vision deficiency (E15), is weaker than
the others (E16), or finds the default stick response or rumble
uncomfortable (E17). *Goals:* change their own seat without changing anyone
else's (E14's reason), and lose no information when effects are reduced
(E14). *Frustrations the record names:* a palette nobody chose, whose re-roll
cost the match (E15, answered by fixed pairs); settings that followed the pad
slot rather than the person (E17, answered by profiles). *A good session:*
**(H)** sets it once on a profile and never thinks about it again.

**P5, the stage author.** *Context:* builds an arena in `ColourWarsEditor.exe`
(E20). Today that is mainly the game's author; a player making stages for
their own group is the case `ROAD#16` was built for (E20) **(H: that such
players exist)**. *Goals:* edit, save and play without a rebuild; learn why a
stage was refused; keep records on their stage. *Frustrations the record
names:* the manifest/CMake/rebuild cycle and a broken stage stopping the game
(E20, answered). *A good session:* edits, saves to My Stages, opens Stage
Select in the running game and plays the new version (E20).

**P6, the Labrador developer.** *Context:* reads the source, not the screen
(E21). *Goals:* see the full-OOP end of the engine's dial at work, follow one
shot from input to score, add a weapon or paint object without breaking a
contract (E21), and find where the engine API was awkward (the
[report ledger](review/engine-reports.md)). *Frustrations:* **(H)** a client
refactored toward the engine's own style, which would delete the
demonstration (AGENTS.md). *A good session:* finds a concept in the OOP
guide's table and the code where the guide says. P6 has stories but no
journey; deliverable 3 names only player journeys.

### User stories

Each row reads "As *persona*, I want *story*, so that *outcome*". **Served
by** names the feature or row and, where one exists, the retained
`scripts/acceptance/` case; "none" means nothing serves it today.

#### A. Launch and setup

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| A1 | P1: to reach a match from launch with a pad or the keyboard, so the group starts quickly | Title, Home, Mode, Count, Join, Team, Weapon and Stage are each reachable with either device; Title says when nothing is connected | `MainMenuTitle` to `MainMenuStageSelect`; `menu_navigation`, `team_selection` | E3, E23 |
| A2 | P1: last time's setup restored, so a returning group skips re-entering it | Mode and Count focus start on the last choices; teams, weapons and stage still need confirming; a missing stage falls back without failing | `GAP#6.1`; `remembered_setup` | E9 |
| A3 | P2: to claim a seat with my own pad without renumbering anyone, so my seat stays mine | A device takes at most one seat; leaving frees it without renumbering; joining stops at the chosen count | `MainMenuJoin`; `team_selection` | E3 |
| A4 | P1: fair teams, so nobody argues about them | X or R shuffles to 1v1, 2v1 or 2v2; confirming with every seat on one team is refused with a message | `GAP#1.5`; `team_selection` | E4 |
| A5 | P1: to fill empty seats with bots at a chosen difficulty, so a short group still has a match | P1 turns an unclaimed row into an Easy, Normal or Hard bot; bots use the same movement, tools, paint and scoring; their label shows number and difficulty | `ROAD#10` (attended: difficulty usefulness); `bot_roster`, `bot_runtime`, `bot_driver` | E5 |
| A6 | P2: to know what each tool does before I pick it, so I pick one that suits me | Weapon Select names and describes all five tools in text that fits; the choice can change before Stage Select | Weapon Select descriptions; `weapon_catalog`, `text_wrap`; whether the text is enough is `POL#24` | E12, E23 |
| A7 | P1: to see the arena and its record before locking it, so the choice is informed | Stage Select draws the live arena and its best record; Random draws only from Random stages | Stage Select, `GAP#6.2`; `stage_selection`, `match_records` | E1, E20 |
| A8 | P2: the controls before my first shot, so I do not learn them by dying | The countdown shows a controls card for each device in the match; Controls / audio carries a full reference, also from Pause | Countdown cards, Controls / audio; `menu_text`; observation is `POL#24` | E17, E19 |
| A9 | P1: to skip the opening once everyone has seen it, so rematches start fast | A, B or Start (Space, Escape or Enter) skips; a skip press is never a pause; a rematch starts at the preview | `MatchOpening::skip` (`game/objects/match_opening.cpp`), `latch_intro_pause` (`game/states/pause_menu.cpp`, called from `game_states.cpp`); `debug_console` (skip, latch), `action_bindings` (skip keys) | E19 |

#### B. In the match

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| B1 | P2: to know which figure is mine, so I never steer the wrong one | My pane's HUD names my seat, device and team; my figure carries a badge readable over paint; Reduce effects keeps it | HUD identity, `PlayerBadge`; `player_identity` | E3, E15, gap audit 2.4 |
| B2 | P2: to know where my teammates are when they are off my pane, so we can work together | An off-screen teammate is marked at my pane's edge in their direction, bots included | `ROAD#18` (attended: readability); `teammate_markers` | E11 |
| B3 | P2: to learn who won only at the end, so the reveal is the payoff | No coverage figure is shown during play; the reveal shows both teams' coverage | The reveal (`game/objects/match_opening.cpp`); deliberate (E8) | E8, E19 |
| B4 | P4: warning before my paint runs out, even with the sound off, so I am not caught dry | A HUD tag and the tank change before exhaustion, and a cue sounds once per descent | `POL#38` (attended); `low_paint` | E24 |
| B5 | P1: a dropped pad to stop the match, so nobody loses a player to a cable | Disconnect pauses and names the missing seat; Resume waits for its return; a pad in another slot reclaims with Start | `ROAD#4`; `menu_navigation` (physical pads: `ROAD#2`) | E3 |
| B6 | Any seat: resuming not to surprise anyone, so nobody dies on the first frame | Resume and focus return count in before play; held fire or jump needs a release | `POL#18` (attended); `resume_countdown` | E18 |
| B7 | P1 on the mouse: to find my pointer over busy paint, so I aim where I mean | A marker follows the pointer in matches over pale, dark and painted scenes | `POL#39` (attended); `cursor_marker` | E24 |
| B8 | P2 on a pad: aim that holds its own against the mouse seat, so the device does not decide the match | **(H)** In mixed-device play, pad seats are not consistently outscored for aim alone | none (`ROAD#24` is `UNSCOPED`); Boost (`ROAD#5`) is a partial lever | E2, E11 |

#### C. Between rounds and across a series

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| C1 | P1: to play a best-of series, so one round is not the whole evening | Mode offers Single, Best of 3 and Best of 5; Results show the round, wins and target; a draw awards no win and is replayed | `Session` (`game/session.h`), Results, End menu; `series_stage_rule` | E6, E10 |
| C2 | P2: to see what I did, not only which team won | Results list each player's held coverage and at most one award; a lone player gets none | `ROAD#12`; `results_awards` | E8, E10 |
| C3 | P1: to change weapons or arena between rounds, so the series stays fresh without losing its score | Change Weapons and Change Level open under Next Round; teams stay locked; Back returns to the End menu | `ROAD#20`; `series_stage_rule` | E11 |
| C4 | P1: a series rule that moves the stage on, so nobody has to choose every round | Same stage, Rotate or Loser's pick on Mode; a draw replays its stage; Loser's pick lets only the losing seats choose | `ROAD#20` (attended: which default); `series_stage_rule` | E11 |
| C5 | Any seat: to watch the round's last seconds again, so the finish can be argued over | Watch Replay plays the final seconds with the round's panes and sound, as often as wanted, then returns to the End menu | `ROAD#19`; `instant_replay` | E11 |
| C6 | P1: a quick rematch, so the evening keeps moving | Play Series Again resets the score; a rematch starts at the preview; Restart replays only the current round | Session flow, intro skip; `series_stage_rule` | E10, E19 |
| C7 | P1: to end the evening cleanly, so next time starts where this one left off | Exit to Main Menu, then Home Exit with a confirmation; the save keeps settings, records and the setup | End menu, `MainMenuExitConfirmation`; `settings`, `match_records`, `remembered_setup` | E9, E23 |

#### D. Settings and comfort

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| D1 | P4: to turn off shake and effects for my pane only, so I can play without changing anyone else's screen | Per-pad Off/Shared/On for View shake, Aim marker and Reduce effects; other panes unchanged; no state signal lost | `ROAD#7` (attended); `view_shake`, `aim_marker`, `settings`, `player_identity` | E14 |
| D2 | P4: to avoid a palette I cannot read, so I can tell the teams apart | Options offers fixed pairs beside Random; the choice is saved; the HUD spells out the team | Team palette option; `team_palette` | E15 |
| D3 | P4: a handicap that keeps me in the game, without changing what wins | Boost Off/+1/+2 per seat on Weapon Select; less damage taken and faster refill; score stays painted surface | `ROAD#5` (attended); `handicap` | E16 |
| D4 | P4: stick response that suits my pad, so movement and aim feel right | Per-pad movement and aim deadzones with a live preview; Apply or Back | `POL#18` (attended, with pads: `ROAD#2`); `stick_settings` | E17 |
| D5 | Any seat: to rebind my controls, so they suit my hands | Pad buttons and the keyboard's six actions are rebindable; a control in use is refused, not swapped | `GAP#4.2` (attended: provisional calls); `action_bindings`, `input_bindings`, `keyboard_jump_capture` | E17 |
| D6 | P4: my settings to follow me to whichever pad I pick up | A named profile carries deadzones, rumble, comfort toggles, buttons and Boost; Join cycles profiles | `ROAD#21` (attended: real pads, `ROAD#2`); `player_profiles` | E17 |
| D7 | Any seat: music, effects and rumble at a level I can stand | Separate music and SFX volume; a per-pad rumble choice | Controls / audio, `ROAD#9` (attended); `settings`, `rumble` | E17 |
| D8 | P1: to change resolution without being stuck on a bad mode | A display change opens a Keep/Revert trial; nothing is saved until Keep | `DisplaySettingsTrial`; `display_trial` | E23 |

#### S. Solo practice

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| S1 | P3: to learn movement without a clock, so I can repeat what I need | 1 Player on the Movement Course is untimed; each station shows its lesson while I stand in it | `POL#19` (attended: keep or cut); `practice_workshop`, `movement_course` | E13 |
| S2 | P3: to try every tool on the same wall and start clean, so I can compare them | The practice pause has a Tool row that rebuilds the course clean with the chosen tool | `POL#19` (attended); `practice_workshop` | E13 |
| S3 | P3: an opponent when nobody else is here | One human with one to three bots on either team | `ROAD#10` (attended); `bot_runtime` | E5 |
| S4 | P3: something to beat next time | A finished round records under its stage; Stage Select and Results show the best | `GAP#6.2`; `match_records` | E10 |
| S5 | P3: a timed solo run on a real arena that reports coverage honestly | Solo on any stage ends with coverage and no opponent, and skips the opening and reveal | Match scope decisions; `paint` covers coverage | E6, E19 |
| S6 | P3, after practice: to know I am ready for a group | **(H)** After the workshop I can move, recover paint and pick a tool in an ordinary match | `POL#19`'s keep test (attended) | E13 |

#### F. Authoring and playing a user stage

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| F1 | P5: to save from the editor straight into the game's stage folder | Save to My Stages names a valid file in the per-user folder and refuses a shipped name | `ROAD#16`; `level_editor`, `user_stages` | E20 |
| F2 | P5: to play my stage without rebuilding or restarting the game | The game rescans when Stage Select opens; the stage appears marked "Built by you." | `ROAD#16` (attended: saved-stage play); `user_stages` | E20 |
| F3 | P5: a broken stage to cost one stage and tell me why | A refused file is left out with a reason on stderr and in Stage Select's issue details | `ROAD#16`, `ROAD#32`; `user_stages` | E20 |
| F4 | P5: to check my arena's fairness before playing it | The editor's blockout aids show paint-mechanism guides and a symmetry report | `ED#9`, `ED#25`; `level_editor` | E20 |
| F5 | P5: my stage to keep its records | Records are keyed by the file name and survive later registration | `ROAD#16`; `user_stages`, `match_records` | E20 |
| F6 | P5: my finished stage in Random | **(H)** An opted-in user stage can be drawn by Random | none: user stages are never Random (provisional, `ROAD#16`) | E20 |
| F7 | P5: to give my stage to a friend | **(H)** A stage file copied into another player's folder plays there | Copying by hand works (E20); no in-game sharing | E20 |

#### G. Reading the code

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| G1 | P6: to find each OOP concept in real code | Every concept in the OOP guide's table links to code that shows it | [OOP guide](oop-guide.md#where-the-concepts-live); no case (review) | E21 |
| G2 | P6: to follow one shot from input to score | The guide's walk-through names each class and call in order | [Follow one shot](oop-guide.md#follow-one-shot-from-input-to-score) | E21 |
| G3 | P6: to add a weapon without breaking a contract | The guide lists the contracts; the catalogue check refuses a broken tool | [Add a weapon](oop-guide.md#add-a-weapon-without-losing-the-contracts); `weapon_catalog` | E21 |
| G4 | P6: to see where the engine API was awkward | Each workaround is a ledger report with its exhibit | [Engine reports](review/engine-reports.md) | E21 |

### Journeys

Each step names the screen or state that serves it; classes are in
`game/states/` unless named otherwise.

**J1, first launch to first match** (P1 with P2 guests; P3 takes J3).

| Step | Screen or state | Stories |
|---|---|---|
| Launch; the save is read before the window exists | `wWinMain` (`game/main.cpp`), `Save` | C7 |
| Title shows the build, and "Connect a controller" when nothing reads as connected | `MainMenuTitle` | A1 |
| Home: Play, Options, Credits, Exit | `MainMenuHome` | A1 |
| Optional: palette, display, Controls / audio | `MainMenuOptions`, `DisplaySettingsTrial`, `ControlsSettingsMenu` | D2, D7, D8, A8 |
| Mode: Single, Best of 3 or 5, Match length, Series stage rule | `MainMenuModeSelect` | C1, C4 |
| Player Count; the device that leaves it is P1 | `MainMenuPlayerCount` | A3 |
| Join: claim seats, turn rows into bots, cycle profiles | `MainMenuJoin` | A3, A5, D6 |
| Team Select: choose or shuffle; both teams needed | `MainMenuTeamSelect` | A4 |
| Weapon Select: tool descriptions, Boost | `MainMenuWeaponSelect` | A6, D3 |
| Stage Select: live preview, record, Random | `MainMenuStageSelect` | A7 |
| Opening (skippable), countdown with controls cards | `GameLevel` above `GameMenu`; `game/objects/match_opening.cpp` | A8, A9 |
| Play | `GameLevel`, `Level` | B1-B8 |
| Reveal, then Results | `match_opening.cpp`, `ResultsMenuPage` | B3, C2 |
| End: Watch Replay, continue, change, exit | `EndMenuPage` | C5, C6, C7 |

*Unobserved:* whether a guest reads the countdown card and the weapon
description in time; that is the fresh-player observation in `POL#24`.

**J2, a returning group's evening** (P1, P2, P4).

| Step | Screen or state | Stories |
|---|---|---|
| Launch: Mode and Count open on last time's choices | `GameMenu` restoring `"last_match"` | A2 |
| Join: each person picks their profile; empty seats become bots | `MainMenuJoin` | D6, A5 |
| Team, Weapon (with Boost) and Stage confirmed from the remembered picks | Setup pages | A4, D3, A7 |
| The opening, skipped | Intro skip | A9 |
| Mid-match: a pad drops and is reclaimed; someone pauses to change volume | Reconnect wait, Pause, Options under Pause | B5, B6, D7 |
| Round ends: results, awards, series score | `ResultsMenuPage` | C1, C2 |
| Between rounds: replay, change weapons or arena, or let the rule pick | `EndMenuPage` | C3, C4, C5 |
| Series won: Play Series Again or change teams | `EndMenuPage` | C6 |
| Leave: Exit to Main Menu, Home Exit | `EndMenuPage`, `MainMenuExitConfirmation` | C7 |

*Unobserved:* whether the evening felt good is the attended `ROAD#2`,
`ROAD#10`, `ROAD#20` and `POL#18` work; nothing here observes it.

**J3, solo practice** (P3).

| Step | Screen or state | Stories |
|---|---|---|
| Mode: Single Match (with its practice hint); Count: 1 Player | `MainMenuModeSelect`, `MainMenuPlayerCount` | A1 |
| Team Select is skipped for one player | `game/states/main_menu_players.cpp:497` | A1 |
| Weapon, then the Movement Course | `MainMenuWeaponSelect`, `MainMenuStageSelect` | A6, S1 |
| Untimed workshop: a lesson per station, pause Tool row, Restart, Quit | `GameLevel` in practice; lessons in `movement_course.json` | S1, S2 |
| Or a timed solo run on an arena: coverage results, no opening or reveal | `GameLevel`, `ResultsMenuPage` | S4, S5 |
| Or Count 2-4 with bots for a real opponent | `MainMenuJoin` | S3, A5 |

*Unobserved:* whether the workshop prepares a fresh player (S6) is the
`POL#19` observation.

**J4, authoring and playing a user stage** (P5).

| Step | Screen or state | Stories |
|---|---|---|
| Open the editor; New, or Open a shipped stage to copy | `ColourWarsEditor.exe` | F1 |
| Edit; check the paint guides and the symmetry report | Editor canvas, blockout aids | F4 |
| File > Save to My Stages | `game/user_stage_contract.h` rules | F1 |
| In the running game, open Stage Select: it rescans | `UserStageFolder`, `MainMenuStageSelect` | F2 |
| A refused file: the issue count, Down for details | `UserStageRefusalsMenu` | F3 |
| Play it; records are keyed by the file name | `GameLevel`, `MatchRecords` | F2, F5 |
| Edit, save, reopen Stage Select: the new version plays | Rescan on open | F2 |
| Share it, or put it in Random | none | F6, F7 |

### Open points for the author

The review is the author's `DECISION`; the draft takes no position on these:

1. **The persona set.** Keep the six, fold P4 into P1-P3 as a facet, or drop
   P6 (it has no journey and its stories describe the code, not play).
2. **The hypotheses.** Every **(H)** above is unobserved. The largest are who
   P2 is (shooter experience, age, how often a group meets), the host's
   device, and whether players other than the author make stages (P5).
3. **The unserved stories.** B8 (pad aim against the mouse seat), F6 (user
   stages in Random) and F7 (sharing) map to "none". Accepting one makes it a
   `DES#2` candidate; rejecting it records the absence as intended, as the
   comparable-game gaps' "Not tracked, by choice" list does.
4. **Priorities.** The draft ranks no persona or story. If the author names a
   primary persona, `DES#2` can order its candidates by it; NOW does not
   change.

## Traceability and gap pass (`DES#2`)

**Blocked on the author's acceptance of `DES#1`.** Its draft is above; the
pass starts from the stories the author accepts, not from the draft. Map the
accepted stories to features, TODO rows and
`scripts/acceptance/` cases. File each unserved story as a candidate in the
owner doc it belongs to, with its own row. List features that no story
explains. Acceptance: one table covers every story, and each new candidate
follows TODO.md rule 4.
