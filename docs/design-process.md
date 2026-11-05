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

**Drafted 2026-10-30; accepted with amendments by the author on
2026-11-03.** Deliverables 1-3
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
checked row by row when it was written. The author's calls on it are under
[Open points for the author](#open-points-for-the-author); the text fixes they
asked for (P5, P6, B8, F6, F7 and J4) are made in place below. The personas
and stories are now the accepted set `DES#2` starts from. `DES#2` re-traced every
story against the tree; where its [table](#the-table) differs from a **Served
by** cell below, the table is current.

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
| E19 | [Backstory: Building it](backstory.md#building-it), [countdown](polish-countdown.md) | The opening, skippable with A, B or Start (the reveal plays in full); solo practice skips both; the countdown shows one controls card per kind of device present (keyboard and mouse, pad). |
| E20 | [Level editor](level-editor.md#play-the-level), [User stages](feature-roadmap.md#user-stages-road16) | A player who saves a stage should play it without a rebuild; a broken file costs one stage; a user stage joins Random only when its file asks (`ROAD#16`, decided 2026-11-05). |
| E21 | [OOP guide](oop-guide.md), AGENTS.md "What this game is for" | A second audience: Labrador developers reading the game as the engine's full-OOP client. |
| E22 | [Polish plan: the indie standard](polish-plan.md#what-the-indie-game-standard-means-in-practice) | Moving and painting in an empty room should be enjoyable; the finish must hold through pause, death, respawn and rematch. |
| E23 | `game/states/main_menu.h`, `game/states/game_states.h`, `MainMenuJoin::update_page` (`game/states/main_menu_players.cpp`) | The menu path Title, Home, Mode, Player Count, Join, Team, Weapon, Stage; one player skips Team Select; a match is a `GameLevel` pushed above `GameMenu`. |
| E24 | [TODO.md](../TODO.md), NOW "Waiting on the author" | Which of the above are still attended gates. |

Nothing in the repository records a playtest with people other than the
author, or their shooter experience, age or how often a group meets. Every
claim of that kind below is therefore **(H)**.

### Personas

Each persona is a role a person takes in a session, not a demographic; one
person can be P1 on Friday and P3 on Sunday. P4 is a facet any seat can carry,
kept as a named persona so its shared requirement is traced as one. P6 is a
non-player persona. P2 is the primary persona and P1 the second (the author,
2026-11-03).

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
else's (E14's reason; per pad, since the one keyboard seat follows the shared
Options switches, `ROAD#7`), and lose no information when effects are reduced
(E14). *Frustrations the record names:* a palette nobody chose, whose re-roll
cost the match (E15, answered by fixed pairs); settings that followed the pad
slot rather than the person (E17, answered by profiles). *A good session:*
**(H)** sets it once on a profile and never thinks about it again.

**P5, the stage author.** *Context:* builds an arena in `ColourWarsEditor.exe`
(E20). The editor is built from source and is not in the Release ZIP
(`cmake/release_package.cmake` installs only `ColourWars`; the README's "Your
own stages" says so), so today P5 is the game's author. A player making
stages for their own group is the case `ROAD#16` was built for (E20) **(H:
that such players exist)**, and it cannot happen until the editor ships. *Goals:* edit, save and play without a rebuild; learn why a
stage was refused; keep records on their stage. *Frustrations the record
names:* the manifest/CMake/rebuild cycle and a broken stage stopping the game
(E20, answered). *A good session:* edits, saves to My Stages, opens Stage
Select in the running game and plays the new version (E20).

**P6, the Labrador developer** (non-player). *Context:* reads the source, not the screen
(E21). *Goals:* see the full-OOP end of the engine's dial at work, follow one
shot from input to score, add a weapon or paint object without breaking a
contract (E21), and find where the engine API was awkward (the
[report ledger](review/engine-reports.md)). *Frustrations:* **(H)** a client
refactored toward the engine's own style, which would delete the
demonstration (AGENTS.md). *A good session:* finds a concept in the OOP
guide's table and the code where the guide says. P6 has stories but no
journey; deliverable 3 names only player journeys. Its stories are traced to
docs only (the OOP guide and the report ledger) and take no part in ordering
player features.

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
| A8 | P2: the controls before my first shot, so I do not learn them by dying | The countdown shows one controls card for each kind of device in the match (keyboard and mouse, pad), shared by all panes; Controls / audio carries a full reference that names the rebound keys and pad buttons, also from Pause | Countdown cards, Controls / audio; `menu_text`; observation is `POL#24` | E17, E19 |
| A9 | P1: to skip the opening once everyone has seen it, so rematches start fast | A, B or Start (Space, Escape or Enter) skips; a skip press is never a pause; a rematch starts at the preview | `MatchOpening::skip` (`game/objects/match_opening.cpp`), `latch_intro_pause` (`game/states/pause_menu.cpp`, called from `game_states.cpp`); `debug_console` (skip, latch), `action_bindings` (skip keys) | E19 |

#### B. In the match

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| B1 | P2: to know which figure is mine, so I never steer the wrong one | My pane's HUD names my seat, device and team; my figure carries a badge readable over paint; Reduce effects keeps it | HUD identity, `PlayerBadge`; `player_identity` | E3, E15, gap audit 2.4 |
| B2 | P2: to know where my teammates are when they are off my pane, so we can work together | An off-screen teammate is marked at my pane's edge in their direction, bots included | `ROAD#18` (attended: readability); `teammate_markers` | E11 |
| B3 | P2: to learn who won only at the end, so the reveal is the payoff | No coverage figure is shown during play; when a team wins, the reveal shows both teams' coverage; a draw (equal or no paint) closes without one, and Results still lists both shares | The reveal (`game/objects/match_opening.cpp`); deliberate (E8) | E8, E19 |
| B4 | P4: warning before my paint runs out, even with the sound off, so I am not caught dry | A HUD tag and the tank change before exhaustion, and a cue sounds once per descent | `POL#38` (attended); `low_paint` | E24 |
| B5 | P1: a dropped pad to stop the match, so nobody loses a player to a cable | Disconnect pauses and names the missing seat; Resume waits for its return; a pad in another slot reclaims with Start | `ROAD#4`; `menu_navigation` (physical pads: `ROAD#2`) | E3 |
| B6 | Any seat: resuming not to surprise anyone, so nobody dies on the first frame | Resume and focus return count in before play; held fire or jump needs a release | `POL#18` (attended); `resume_countdown` | E18 |
| B7 | P1 on the mouse: to find my pointer over busy paint, so I aim where I mean | A marker follows the pointer in matches over pale, dark and painted scenes | `POL#39` (attended); `cursor_marker` | E24 |
| B8 | P2 on a pad: aim that holds its own against the mouse seat, so the device does not decide the match | **(H)** In mixed-device play, pad seats are not consistently outscored for aim alone | none; traced to `ROAD#24` (`UNSCOPED`, no new row); Boost (`ROAD#5`) is a partial lever | E2, E11 |

#### C. Between rounds and across a series

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| C1 | P1: to play a best-of series, so one round is not the whole evening | Mode offers Single, Best of 3 and Best of 5; Results show the round, wins and target; a draw awards no win and is replayed | `Session` (`game/match/session.h`), Results, End menu; `series_stage_rule` | E6, E10 |
| C2 | P2: to see what I did, not only which team won | Results list each player's held coverage and at most one award; a lone player gets none | `ROAD#12`; `results_awards` | E8, E10 |
| C3 | P1: to change weapons or arena between rounds, so the series stays fresh without losing its score | Change Weapons and Change Level open under Next Round; teams stay locked; Back returns to the End menu | `ROAD#20`; `series_stage_rule` | E11 |
| C4 | P1: a series rule that moves the stage on, so nobody has to choose every round | Same stage, Rotate or Loser's pick on Mode; a draw replays its stage; Loser's pick lets only the losing seats choose | `ROAD#20` (attended: which default); `series_stage_rule` | E11 |
| C5 | Any seat: to watch the round's last seconds again, so the finish can be argued over | Watch Replay plays the final seconds with the round's panes and sound, as often as wanted, then returns to the End menu | `ROAD#19`; `instant_replay` | E11 |
| C6 | P1: a quick rematch, so the evening keeps moving | Play Series Again resets the score; a rematch starts at the preview; Restart replays only the current round | Session flow, intro skip; `series_stage_rule` | E10, E19 |
| C7 | P1: to end the evening cleanly, so next time starts where this one left off | Exit to Main Menu, then Home Exit with a confirmation; the save keeps settings, records and the setup | End menu, `MainMenuExitConfirmation`; `settings`, `match_records`, `remembered_setup` | E9, E23 |

#### D. Settings and comfort

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| D1 | P4: to turn off shake and effects for my pane only, so I can play without changing anyone else's screen | Per-pad Off/Shared/On for View shake, Aim marker and Reduce effects; other panes unchanged; no state signal lost; the keyboard seat follows the Options switches (`ROAD#7`), so a pad that must keep an effect sets it On | `ROAD#7` (attended); `view_shake`, `aim_marker`, `settings`, `player_identity` | E14 |
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
| S5 | P3: a timed solo run on a real arena that reports coverage honestly | Solo on any stage without lessons ends with coverage and no opponent (a stage with lessons is S1's untimed practice); every solo start skips the opening and reveal | Match scope decisions; `paint` covers coverage | E6, E19 |
| S6 | P3, after practice: to know I am ready for a group | **(H)** After the workshop I can move, recover paint and pick a tool in an ordinary match | `POL#19`'s keep test (attended) | E13 |

#### F. Authoring and playing a user stage

| Id | As ..., I want ..., so that ... | Acceptance criteria | Served by | Basis |
|---|---|---|---|---|
| F1 | P5: to save from the editor straight into the game's stage folder | Save to My Stages names a valid file in the per-user folder and refuses a shipped name | `ROAD#16`; `level_editor`, `user_stages` | E20 |
| F2 | P5: to play my stage without rebuilding or restarting the game | The game rescans when Stage Select opens outside a series (between rounds the list holds still); the stage appears marked "Built by you." | `ROAD#16` (attended: saved-stage play); `user_stages` | E20 |
| F3 | P5: a broken stage to cost one stage and tell me why | A refused file is left out with a reason on stderr and in Stage Select's issue details | `ROAD#16`, `ROAD#32`; `user_stages` | E20 |
| F4 | P5: to check my arena's fairness before playing it | The editor's blockout aids show paint-mechanism guides and a symmetry report | `ED#9`, `ED#25`; `level_editor` | E20 |
| F5 | P5: my stage to keep its records | Records are keyed by the file name and survive later registration | `ROAD#16`; `user_stages`, `match_records` | E20 |
| F6 | P5: my finished stage in Random | **(H)** An opted-in user stage can be drawn by Random | A user stage whose file says `"include_in_random": true` joins Random (`ROAD#16`'s Random call, decided 2026-11-05); `user_stages` | E20 |
| F7 | P5: to give my stage to a friend | **(H)** A stage file copied into another player's folder plays there | Copying the file by hand, with the README's "Your own stages"; `ROAD#16`; `user_stages`. In-game sharing is deliberately absent while the editor stays out of the ZIP | E20 |

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
| Series won: Play Series Again | `EndMenuPage` | C6 |
| Leave: Exit to Main Menu, Home Exit | `EndMenuPage`, `MainMenuExitConfirmation` | C7 |

*Unobserved:* whether the evening felt good is the attended `ROAD#2`,
`ROAD#10`, `ROAD#20` and `POL#18` work; nothing here observes it.

**J3, solo practice** (P3).

| Step | Screen or state | Stories |
|---|---|---|
| Mode: Single Match (with its practice hint); Count: 1 Player | `MainMenuModeSelect`, `MainMenuPlayerCount` | A1 |
| Join: the activating device already holds the one seat; Continue skips Team Select | `MainMenuJoin::update_page` (`game/states/main_menu_players.cpp`) | A1, A3 |
| Weapon, then the Movement Course | `MainMenuWeaponSelect`, `MainMenuStageSelect` | A6, S1 |
| Untimed workshop: a lesson per station, pause Tool row, Restart, Quit | `GameLevel` in practice; lessons in `movement_course.json` | S1, S2 |
| Or a timed solo run on an arena without lessons: coverage results, no opening or reveal | `GameLevel`, `ResultsMenuPage` | S4, S5 |
| Or Count 2-4 with bots for a real opponent | `MainMenuJoin` | S3, A5 |

*Unobserved:* whether the workshop prepares a fresh player (S6) is the
`POL#19` observation.

**J4, authoring and playing a user stage** (P5). The editor is built from
source and is not in the Release ZIP, so a ZIP player can follow this journey
from Stage Select on, with a file someone else made.

| Step | Screen or state | Stories |
|---|---|---|
| Open the editor (a source build); New, or Open a shipped stage to copy | `ColourWarsEditor.exe` | F1 |
| Edit; check the paint guides and the symmetry report | Editor canvas, blockout aids | F4 |
| File > Save to My Stages | `game/contracts/user_stage_contract.h` rules | F1 |
| In the running game, open Stage Select: it rescans | `UserStageFolder`, `MainMenuStageSelect` | F2 |
| A refused file: the issue count, Down for details | `UserStageRefusalsMenu` | F3 |
| Play it; records are keyed by the file name | `GameLevel`, `MatchRecords` | F2, F5 |
| Edit, save, reopen Stage Select from the main menu: the new version plays | Rescan on open (not between rounds) | F2 |
| Share it: copy the file into a friend's stages folder | By hand, per the README's "Your own stages"; no in-game sharing | F7 |
| Put it in Random | Set `include_in_random` to true in the editor before saving (`ROAD#16`, 2026-11-05) | F6 |

### Open points for the author

The draft took no position on these. **The author decided them on
2026-11-03,** accepting the draft with the amendments below; as "What this
does not decide" says, that reorders nothing and closes no gate.

1. **The persona set.** Keep the six, fold P4 into P1-P3 as a facet, or drop
   P6 (it has no journey and its stories describe the code, not play).
   *Decided:* keep all six. P4 stays a named facet, because D1-D4, D6 and B4
   share one requirement (change my own seat without changing anyone else's)
   that `DES#2` should trace as one. P6 is labelled non-player: it is left out
   of `DES#2`'s ordering of player features and its stories are traced to
   docs only. *Reopen* to drop P6 if the pass should cover players only and
   leave the engine-client role to AGENTS.md and the OOP guide, or to fold P4
   if `DES#2`'s table becomes unwieldy with a facet persona.
2. **The hypotheses.** Every **(H)** above is unobserved. The largest are who
   P2 is (shooter experience, age, how often a group meets), the host's
   device, and whether players other than the author make stages (P5).
   *Decided:* keep every (H) and strike none. Each goes to the attended
   session that can settle it: who P2 is, and whether a good session needs
   no manual, to `POL#24`; the host's device and B8 to `ROAD#2`; S6 and P3's
   good session to `POL#19` and `ROAD#10`; P5's player authors to
   `ROAD#16`/`ED#5`, blocked while the editor is unshipped. POL#46's Study B
   is not one of them: its participants judge screenshots and never play.
   *Reopen* if a later study kit gains a background questionnaire on play
   habits, which could then be a P2 source.
3. **The unserved stories.** B8 (pad aim against the mouse seat) and F6
   (user stages in Random) map to "none"; F7 (sharing) is served by copying
   the file by hand. Accepting an unserved story makes it a `DES#2`
   candidate, unless an owner row already holds it, in which case `DES#2`
   traces it to that row and files nothing new. Rejecting one records the
   absence as intended here, not in the comparable-game gaps' "Not tracked,
   by choice" list, which belongs to that comparison. *Decided:* all three
   accepted.
   - **B8** is traced to `ROAD#24`, which stays `UNSCOPED` and `LOW`. The
     `ROAD#2` pad session, and any `POL#24` session that mixes the mouse seat
     with pads, notes whether pad seats lose fights to the mouse seat.
     *Reopen:* if they consistently do at 2P or 4P, raise and scope
     `ROAD#24`; if the mouse seat is an accepted host advantage, or play is
     expected to be mostly all-pad, reject B8 and close `ROAD#24` with it.
   - **F6** is left to `ROAD#16`'s Random call, with a lean to keeping
     "never Random" for now: today only the author authors, and can pick a
     stage by hand. *Reopen* towards the opt-in if the editor ships, if the
     `ROAD#16`/`ED#5` saved-stage play shows finished stages worth rotating,
     or if groups collect enough user stages that picking each is a chore.
     The author's call on 2026-11-05 was the opt-in: a user stage whose file
     says `"include_in_random": true` joins Random.
   - **F7** is served by hand-copying and the README; in-game sharing is
     deliberately absent while the editor stays out of the ZIP. *Reopen* as
     a candidate if the editor ships in the ZIP or players other than the
     author start making stages; the smallest slice would be an "open My
     Stages folder" item.
4. **Priorities.** The draft ranks no persona or story. If the author names a
   primary persona, `DES#2` can order its candidates by it; NOW does not
   change. *Decided:* P2, the couch guest, is primary and P1 second, since
   the attended gates that judge the finish ask what a fresh player
   understands without help. *Reopen* for P3 if the ZIP mostly reaches people
   playing alone, or for P1 if playtests show guests cope and hosts struggle
   with setup.

The whole acceptance would be redrafted if the main audience turns out not to
be a couch group, or if a planned release ships the editor, which changes P5,
J4, F6 and F7 materially.

## Traceability and gap pass (`DES#2`)

**Done 2026-11-03.** One pass over all 49 accepted stories, each mapped against
the tree as it stands: the serving feature in code, its TODO row and that row's
current status, and the `scripts/acceptance/` cases that check it (names from
[cases.psd1](../scripts/acceptance/cases.psd1)). The table below supersedes the
stories' **Served by** column where the two differ; that column is kept as the
accepted draft, and the differences are under [Corrections](#corrections-to-served-by).

**How to read it.** *Served*: built, and nothing open holds it. *Attended*:
built, and an open `DECISION` row holds its human review (the mapping closes no
gate). *Held*: unserved, and an existing owner row already holds the call.
*Docs*: a P6 story, traced to docs only and not ordered. On 2026-11-05 the
author accepted every open `DECISION` row as built; the attended reviews an
*Attended* result names were not held, and the hardware and participant ones
were waived by the author, not performed, so *Attended* now reads as built and
accepted without that review. A row named "done" is
closed and no longer in TODO. "Painter" means the check runs only under
`check-painter.ps1` on MattPC, not in the default case list. A **check gap** is a
CPU-checkable criterion that no case asserts; they are collected under
[Check gaps](#check-gaps-des3) as `DES#3`.

### The table

| Id | Persona | Serving feature | Row | Cases | Result |
|---|---|---|---|---|---|
| A1 | P1 | `MainMenuTitle` to `MainMenuStageSelect` ([main_menu.h](../game/states/main_menu.h)); Title's no-device label | none | `menu_navigation`, `menu_text` (the hint rule, `DES#3`); painter drives the pages | Served; the hint cannot show in the shipped game (see `DES#3`) |
| A2 | P1 | The save's `last_match` ([last_match_setup.h](../game/save/last_match_setup.h)), restored by `GameMenu` | `GAP#6.1` done | `remembered_setup`; painter for Mode/Count focus | Served |
| A3 | P2 | `MainMenuJoin`; `MatchSetup::claim_input_slot`/`release_input_slot` | `ROAD#4` done | `bot_roster` (incl. the full-count refusal, `DES#3`), `menu_navigation` | Served |
| A4 | P1 | `MatchSetup::shuffle_teams`; Team Select's one-sided refusal | `GAP#1.5` done | `team_selection`; painter for the refusal message | Served |
| A5 | P1 | Join's bot rows, `set_player_bot`, the `game/ai` driver, `player_label` | `ROAD#10` | `bot_roster`, `bot_runtime`, `bot_driver`, `menu_text` | Attended |
| A6 | P2 | Weapon Select choices and descriptions | `POL#24` | `weapon_catalog`, `text_wrap` | Attended |
| A7 | P1 | `StagePreview`, `MainMenuStageSelect::record_line`, Random eligibility | `GAP#6.2` done | `stage_selection`, `match_records`; painter for the record line | Served |
| A8 | P2 | Countdown controls cards; Controls / audio | `POL#24` | `menu_text` (incl. rebound pad buttons, `DES#4`); painter for one card per device kind | Attended |
| A9 | P1 | `MatchOpening::skip`, `latch_intro_pause` | none | `debug_console`, `action_bindings` | Served |
| B1 | P2 | `PlayerIdentityHud`, `PlayerBadge` | gap audit 2.4 answered; readability in `POL#24` | `player_identity` (incl. the HUD line's words, `DES#4`); painter for the HUD line | Served |
| B2 | P2 | `TeammateMarkers` | `ROAD#18` | `teammate_markers` | Attended |
| B3 | P2 | No live score; the reveal | deliberate (gap audit 1.1) | `story_audio` (reveal beats); painter for the coverage display | Served, by design |
| B4 | P4 | Low-paint tag, tank and `tool.drip` | `POL#38` | `low_paint` | Attended |
| B5 | P1 | Disconnect pause, `reclaim_missing_seat` | `ROAD#4` done; pads `ROAD#2` | `menu_navigation`, `debug_console`, `story_audio`; painter for the naming text | Served |
| B6 | any | Resume and focus count-in | `POL#18` | `resume_countdown`, `match_suspension` | Attended |
| B7 | P1 | `CursorMarkerOverlay` | `POL#39` | `cursor_marker` | Attended |
| B8 | P2 | none | `ROAD#24` (`UNSCOPED`); observed in `ROAD#2`/`POL#24` | none | Held |
| C1 | P1 | `Session`, Results, End menu | none | `series_stage_rule`, `debug_console` | Served |
| C2 | P2 | Results contributions and awards | `ROAD#12` done | `results_awards` | Served |
| C3 | P1 | Change Weapons / Change Level under Next Round | `ROAD#20`'s re-pick slice done | `series_stage_rule`; painter for rows, lock and Back | Served |
| C4 | P1 | Series stage rule on Mode | `ROAD#20` | `series_stage_rule`; painter for losing-seats-only | Attended |
| C5 | any | Watch Replay | `ROAD#19` done | `instant_replay` (three viewings of one tape); painter `-PracticeReplayOnly` for the End-menu loop (`DES#3`); painter `-EndMenuOnly` for Watch Replay after Back from a re-pick (`DES#4`) | Served |
| C6 | P1 | Play Series Again, rematch at the preview, Restart | none | `series_stage_rule`, `debug_console` (`next_session`, `DES#3`), `story_audio` (`round_opening`, `DES#4`); painter `-BotPlayersOnly` for Restart and Play Series Again on a real `GameLevel` | Served |
| C7 | P1 | End to Main Menu, `MainMenuExitConfirmation`, the save | none | `settings`, `match_records`, `remembered_setup`; painter for the confirmation | Served |
| D1 | P4 | Per-pad View shake, Aim marker, Reduce effects | `ROAD#7` | `view_shake`, `aim_marker`, `settings`, `player_identity` | Attended |
| D2 | P4 | Options team palette | none | `team_palette`, `settings`, `display_trial`; painter for the HUD team text | Served |
| D3 | P4 | Boost Off/+1/+2 | `ROAD#5` | `handicap` (scoring never reads it) | Attended |
| D4 | P4 | Pad sticks page | `POL#18`, `ROAD#2` | `stick_settings`, `settings`; painter for the preview and Apply/Back | Attended |
| D5 | any | Keyboard keys, Pad buttons | `GAP#4.2` | `action_bindings`, `input_bindings`, `keyboard_jump_capture` | Attended |
| D6 | P4 | Player profiles | `ROAD#21` | `player_profiles` | Attended |
| D7 | any | Music/SFX volume, per-pad rumble | `ROAD#9` | `settings`, `rumble`, `story_audio` | Attended |
| D8 | P1 | `DisplaySettingsTrial` | none | `display_trial` | Served |
| S1 | P3 | Untimed Movement Course practice and its lessons | `POL#19` | `practice_workshop` (`movement_course` proves only the routes) | Attended |
| S2 | P3 | Practice Pause's Tool row (`PauseMenuInitial::cycle_tool`, `GameLevel`'s change-tool rebuild) | `POL#19` | `practice_workshop` (the row's wrap and the change-tool rebuild, `DES#3`) | Attended |
| S3 | P3 | One human with bots | `ROAD#10` | `bot_runtime`, `bot_roster` | Attended |
| S4 | P3 | Stage records | `GAP#6.2` done | `match_records`; painter for Stage Select's line | Served |
| S5 | P3 | Solo opening off ([game_states.cpp](../game/states/game_states.cpp)), `LevelEndInfo`'s solo coverage | none | `paint`, `menu_text`, `match_records`; `check-display-awake.ps1` for the opening | Served |
| S6 | P3 | **(H)** | `POL#19`, `ROAD#10` | none | Attended |
| F1 | P5 | Save to My Stages | `ROAD#16`, `ED#5` | `user_stages`; the editor's acceptance (`check-level-editor.ps1`) | Attended |
| F2 | P5 | Stage Select's rescan, "Built by you." | `ROAD#16`, `ED#5` | `user_stages`; painter `-StageScanOnly` for the page-open trigger | Attended |
| F3 | P5 | Refused files, scan details | `ROAD#32` done; wording `ROAD#16` | `user_stages`; `check-startup.ps1 -UserStageFixture` for stderr | Attended |
| F4 | P5 | Mechanism guides, symmetry report | `ED#9`, `ED#25` done; `ED#5` | the editor's acceptance | Served |
| F5 | P5 | Records keyed by file name | `ROAD#16` | `match_records` (generic stage keys) | Served by construction |
| F6 | P5 | A user stage's `"include_in_random": true` ([stage_loading.cpp](../game/config/stage_loading.cpp)) | `ROAD#16`'s Random call, decided 2026-11-05 | `user_stages` | Served |
| F7 | P5 | Hand-copying, the README's "Your own stages" | `ROAD#16`, `ED#5` | `user_stages` | Attended, **(H)** |
| G1 | P6 | [OOP guide](oop-guide.md#where-the-concepts-live) | none | none (a reading review) | Docs |
| G2 | P6 | [Follow one shot](oop-guide.md#follow-one-shot-from-input-to-score) | none | none | Docs |
| G3 | P6 | [Add a weapon](oop-guide.md#add-a-weapon-without-losing-the-contracts) | none | `weapon_catalog` | Docs; the guide was stale and is fixed in this pass |
| G4 | P6 | [Engine reports](review/engine-reports.md) | the `ENG#` rows | none | Docs; `GAP#F*` and `ENG#PARKED` are the deliberate holding places |

**Totals:** 21 served (B3 by design, F5 by construction), 22 attended,
2 held (B8, F6) and 4 docs. The P4
facet's shared requirement, changing my own seat without changing anyone
else's (D1-D4, D6, B4), traces as one, but its storage differs by story
(corrected by `DES#4`). D1, D4 and D6 are per pad slot in the save
(`view_shake`, `aim_marker`, `player_identity`, `stick_settings`,
`player_profiles`, `settings`); the keyboard seat follows the shared switches.
D3's Boost is per seat on Weapon Select and is carried to a pad by its profile
(`handicap`, `player_profiles`). D2's palette is one shared Options choice,
readable through fixed pairs and the HUD team text (`team_palette`). B4's
low-paint signal is the same for every seat (`low_paint`). Its comfort is the
`ROAD#2` pad session.

### Candidates

**No new player-feature candidate.** Every story but B8 and F6 has a serving
feature, and those two are held where the author placed them: B8 by `ROAD#24`,
F6 by `ROAD#16`'s Random call. F7 is served by hand-copying. So TODO gains no
feature row from this pass, and the ordering by P2 then P1 applies to the check
gaps below.

The one stale serving feature was P6's: [the OOP guide's Add a weapon
steps](oop-guide.md#add-a-weapon-without-losing-the-contracts) still described
per-weapon registration lists in `tuning.cpp`, `tuning_validation.cpp` and
`presentation_read_menu.cpp` and the menu visuals in `MainMenuWeaponSelect`.
Those now come from `WEAPON_NAMES`, `for_each_tuned_weapon` and
`main_menu.weapon_select.choices`. The steps are rewritten in this pass, and the
last one now names `weapon_catalog`, which pins the existing choices. As a docs
fix it takes no row.

### Check gaps (`DES#3`)

**Done 2026-11-03.** Filed by `DES#2` the same day: five CPU-checkable
criteria that no case asserted. Each is now asserted, and each check was
mutation-tested (the guarded line broken, the require seen to fire, the line
restored). Four of the five could not be reached as filed, because
`GameLevel` and the menu pages need a graphics device. So the rule each one
decides is now a named function that the shipped code calls and the case
calls too, and where the call site mattered, a painter check drives a real
`GameLevel`. Painter-only coverage counts as checked (above).

1. **A3 (P2):** `bot_roster` asserts that `MatchSetup::claim_input_slot`
   refuses a fresh device once the chosen count is full, with four seats and
   a bot, two humans, and two seats with a bot. A claim below the count must
   still land, and a refused claim must leave the setup unchanged.
2. **C6 (P1):** the End menu's continuation is `next_session`
   ([session.h](../game/match/session.h)), which `debug_console` checks for
   best-of-3, best-of-5 and a single match: Next Round and Replay Draw keep
   the ledger, and Play Series Again and Rematch start a fresh one of the same
   length. Painter `-BotPlayersOnly` plays a best-of-5 to completion on a real
   `GameLevel` (through the new `GameLevel::session()`). Pause Restart after a
   won round must leave round and wins as they were, and Play Series Again
   must reset to round 1, 0-0. Not on a real `GameLevel`: the solo Rematch's
   best-of-1 argument and Play Chosen Stage, which `next_session` covers only
   on the CPU.
3. **A1 (P1):** the Title hint's rule is `title_shows_no_controller`
   ([main_menu.h](../game/states/main_menu.h); it replaced
   `MainMenuPage::connected_slots`), and `menu_text` checks it: shown with no
   slot connected, hidden with any one slot. The case also pins down what the
   game actually does. `MenuInput` always reports the keyboard-and-mouse slot
   as connected, so **"Connect a controller" can never show** in the shipped
   game. Whether the hint should stay is a design question, not a check gap.
   The wiring in `MainMenuTitle::update_page` (one line) is covered only by
   reading it.
4. **C5 (any seat):** Watch Replay's offer is `offers_watch_replay`
   ([round_tape.h](../game/match/round_tape.h)), which `GameLevel::open_end_menu`
   calls. `instant_replay` plays three viewings of one tape on every stage,
   each rebuilt and matched bit for bit to the final paint. Painter
   `-PracticeReplayOnly` drives the loop on a real `GameLevel`. A finished
   replay must return to the End menu by itself, Watch Replay must play again,
   a confirm press part-way must leave to the End menu, and a fourth viewing
   must start. Not covered: Back or Pause as the leave press, the multi-frame
   preparing phase (the painter's tapes are shorter than the replay window),
   and practice refusing a playable tape.
5. **S2 (P3):** the Tool row's step is `practice_tool_after`
   ([pause_menu.h](../game/states/pause_menu.h)), and `practice_workshop`
   checks that it wraps both ways over the five tools and never offers Random.
   The change-tool rebuild is now `Level::tool_change_place` and
   `Level::enter_after_tool_change`, which the case runs for each tool: the
   rebuilt course is active, unpainted, holds the chosen tool and shows the
   lesson where the player stood, and a living player is back in place while
   a dead one starts at the spawn. The handler's order in
   `GameLevel::on_pause_menu_closed` (three lines) is covered only by reading
   it.

F5 (user-stage records) and D3 (Boost never changes the score) hold by
construction, with a shared stage key and no scoring read of the handicap, and
are left as notes, not gaps.

### Corrections to Served by

Where the stories' column above disagrees with the tree:

- **A1:** `team_selection` checks the shuffle, not reachability; `menu_text`
  carries the Title clause.
- **A3:** `team_selection` only builds fixtures with `claim_input_slot`; the
  checks are `bot_roster` (device conversion) and `menu_navigation` (seat
  reclaim).
- **A5:** add `menu_text`, whose bot-label checks cover number and difficulty.
- **B5:** add `debug_console` (an unplugged seat pauses) and `story_audio`
  (lost and back cues).
- **B6:** add `match_suspension`, which checks that focus return is owed one
  count-in.
- **C1, C6:** add `debug_console`, whose session checks cover the draw ledger
  and the End-menu choices.
- **C3:** `ROAD#20`'s open part is the stage rule; the re-pick C3 needs is done.
- **C4:** `ROAD#20` also leaves the draw, explicit-only and bot-loser calls,
  not only the default.
- **D2:** add `settings` and `display_trial`, which check that the palette is
  saved and commits with Keep.
- **D4:** add `settings` (per-pad deadzone persistence).
- **D7:** add `story_audio`, which checks that the volumes reach their sound
  groups.
- **S1:** `practice_workshop` is the case; `movement_course` runs route trials.
- **S3:** add `bot_roster`.
- **S5:** "Match scope decisions" names no feature; the features are the solo
  opening-off rule and `LevelEndInfo`'s solo coverage.
- **F1, F4:** `level_editor` is the editor's own acceptance probe
  (`scripts/acceptance/level_editor.cpp`, built as `colourwars_editor_acceptance`
  and run by `check-level-editor.ps1`), not a `cases.psd1` case.
- **F2, F7:** `ED#5` (play a saved stage) holds them beside `ROAD#16`.
- **F3:** `ROAD#32` is done; the open part is `ROAD#16`'s wording.

### Features no story explains

Listed, not removed (deliverable 4). None is a defect. Several are the
premise and art direction, which the stories take for granted, and most of the
rest are developer tools.

- **Player-facing:** the Credits page and the shipped NOTICE and licence
  files; Mode Select's Match length row; Options' Restore Defaults; the End
  menu's Change Teams, Change Weapons and Change Level after a single match or
  a finished series; the setup pages' missing-controller wait (Team, Weapon
  and Stage Select hold on the missing-player prompt, and Back returns to
  Player Count, or between rounds to the End menu); leaving a versus match
  from Pause with Quit and its confirmation; paint underfoot (shipped on
  since 2026-11-05, `POL#30`); Turbulence Yard and its paint-object and mover stages (the Ferry,
  Gate, Lift, Paint Lift, Mover and Slab Trials); which surfaces score (slabs,
  ramps, one-way floors); the respawn redraw; combat, blocked-shot and movement
  feedback; menu and match music, the results coda and cues; the error dialog
  and `last-error.log`;
  `HOW-TO-PLAY.txt`; the save's per-user location, migration and version
  handling; the display held awake during a match; the hybrid-GPU request; and
  the F1/Back per-player debug panel, which ships in Release.
- **Content and premise:** the backstory opening and reveal, the drawing
  mistakes and figure slips, the quiet character and idle drip, and the
  Paint.exe terrain, backdrops and crude shapes.
- **Developer-only:** the Debug console, launch arguments, debug menu,
  overlays, F-key controls, cheats, live tuning, scripted bots, seeded record
  and play, screenshots, scripts and binds; Release ignoring developer
  arguments; startup content validation; the silent-bank fallback; build
  identity; the balance and bot-trial harnesses; the level editor's core
  editing, special-object authoring, file safety and mirror copy; and
  `ColourWarsRegisterStage`.

Stories could be written for several of the player-facing ones: Match length
and Restore Defaults under setup and settings, respawning in the match, and
the error dialog and updating the game at launch. That widens the accepted
set, so it is the author's call, not this pass's.

## Code re-review (`DES#4`)

**Done 2026-11-03,** at the author's request to re-review the personas and
stories and make the app align with them. `DES#2` traced each story to a
feature and a case by name. This pass read the code behind each acceptance
criterion clause by clause, re-checked every persona and evidence claim, and
walked the four journeys page by page. Each claimed mismatch was put to an
independent skeptic, who tried to refute it against the tree. Of 41 claims, 31
survived (some were the same finding from two angles) and the completeness
pass added two more. The personas and stories themselves still stand; no
persona, story or decision was changed, only criteria that overstated what the
accepted behaviour is.

**Code that did not do what its story says, now fixed:**

- **C5 (and C3):** between rounds, Back from Change Weapons or Change Level
  rebuilt the End menu without Watch Replay, though the round's tape was still
  playable. The re-pick route now carries the offer
  (`MainMenuRoute::replay`, `RosterSetupPage::round_replay`), including the
  stage page reached through readied tools. Painter `-EndMenuOnly`
  (`series_repick_checks.h`) checks all three paths.
- **A8:** the Controls / audio reference named the default pad buttons after a
  pad rebind. Its pad lines are now `{pad_jump}`, `{pad_shoot}` and
  `{pad_pause}`, which name the pads' shared choice or say "per pad" when the
  slots' profiles differ ([menu-presentation.md](menu-presentation.md)).
  Startup validation fits every pad label; `menu_text` checks both cases.
- **F5:** the game's user-stage names accepted `COM0` and `LPT0`, which the
  registration helper refuses, so such a stage could not be registered
  unchanged with its records. The contract now refuses them too
  (`user_stages`), and the README states the reserved names.

**Criteria with no assertion, now checked:** B1's HUD line words
(`Level::player_identity_text`, `player_identity`) and C6's "a rematch starts at
the preview" (`round_opening` in `opening_mode.h`, `story_audio`).

**Docs that no longer matched the tree, now corrected:** the README's
reconnect rule (it still said to quit when a pad comes back in another slot;
the game reclaims with Start); E19 (only the opening skips; the reveal plays in
full); A8 and E19 (one card per kind of device, not per device); B3 (a draw's
reveal shows no figures); D1, P4 and the `DES#2` totals (the keyboard seat
follows the shared switches; the P4 stories differ in where they are stored);
S5 (a solo stage with lessons is untimed practice); F2 and J4 (Stage Select
does not rescan between rounds, by design); J2 and J3 (no Change Teams story; a
solo start still passes through Join); E23 and J3's stale line citation; the
F1/F4 correction's target name; the "Features no story explains" list; and,
in [the OOP guide](oop-guide.md), the shot walk-through's results step and the
add-a-weapon and add-a-projectile steps (the match presentation's per-weapon
tables, which are now sized from `TUNED_WEAPON_COUNT`, and `PROJECTILE_NAMES`).
The open-points decisions that send B8 and the P1/P2 hypotheses to attended
sessions are now in those sessions' checklists: `ROAD#2`'s list in
[feature-roadmap.md](feature-roadmap.md#review-follow-up-ammunition-coverage-and-release-checks),
`ROAD#24`'s reopen trigger, and the finish review's step 5 in
[polish-plan.md](polish-plan.md#7-acceptance-and-delivery).

**Left for the author.** A1's "Title says when nothing is connected" still
cannot happen, because the keyboard always reads as connected (`DES#3` item 3).
Either the hint counts pads only and reads as information ("No controller
found - keyboard and mouse ready"), or the hint and the clause go. Nothing
else from this pass waits on a decision, and it reorders nothing in NOW.
