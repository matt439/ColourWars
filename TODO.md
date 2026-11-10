# TODO: open work

An index of open work, one row per item; each row points to the doc that owns the detail. The
rules for editing it are at the end, in [How to read this file](#how-to-read-this-file). A fresh
thread starts here: read **NOW**, then the owner of the row it picks up.

## NOW

**The only ordering in this file.** Keep this block to the current aim and the next few actions;
results go in their owners, and `git log` is the changelog.

**Aim reached; no new aim yet (2026-11-05).** The 2026-10-20 aim was to research comparable
combat levels, make an original reference map, then tune movement and all five tools against
it. Crossfeed was accepted on 2026-10-25. On 2026-11-05 the author accepted every open
decision without the attended sessions:
- **Arenas.** The Turbulence slab trial and Press Tower replace the original Turbulence and King
  of the Hill in Random; the originals stay selectable ([Turbulence](docs/polish-turbulence.md),
  [Press Tower](docs/polish-press-tower.md)).
- **Values.** The current five-tool, movement, bot and Boost values are accepted. Paint underfoot
  now ships on ([`POL#30`](docs/polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30)).
  The series stage rule defaults to Rotate, user stages may opt into Random, display scaling
  stays fractional, enemies get no threat markers and no translation language is chosen.
- **Waived checks.** The hardware, participant and listening checks were waived by the author,
  not performed: pads, the clean PC, performance and startup captures, Study B, fresh players
  and the audio mix. Each owner says so.
- **Engine reports.** The owed reports are filed as
  [labrador#122-#125](docs/review/engine-reports.md#live), and 28 is closed as declined.

The next aim is the author's to set. Until then, rows are picked from the sections below.
The remaining queued hygiene rows (`HYG#81`, `HYG#88`-`HYG#92` and `CI#8`) are
complete (2026-11-07), recorded in [their owner](docs/code-hygiene.md#hygiene-review-2026-11-05).
The review's engine draft, `ENG#35`, still waits on the author's filing decision.
`ROAD#22`'s endless-paint slice is complete (2026-11-07), recorded in the
[roadmap](docs/feature-roadmap.md#endless-paint-road22). `ROAD#11` (sub weapons) is implemented
(2026-11-09): all seven slices are done, and the author's attended play review is the decision
left.
Art (`POL#16`) is unblocked but stays deferred. Bot lift routing, sponges and the controller
rows remain unscoped.
The 2026-11-10 project review added the `REV` rows
([Project review](#project-review-2026-11-10)). Twenty are `DECISION`, each a call beside
something accepted or waived on 2026-11-05; none is ordered here until the author sets an aim.

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6. It records the 2026-10-09
switch to Paint.exe Maximalism and the accepted 2026-10-15 round-2 art rules. The pixel-kit
questions in the polish sub-docs are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#16` | DEFERRED | MED | XL | Extend the approved treatment through the accepted arenas (selected 2026-11-05); remaining UI art is a later polish pass. Waits on the author setting it as the next aim. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |
| `POL#35` | DEFERRED | LOW | S | Signage and captioned Paint-artefact jokes need the author's wording; with `POL#16`. [Terrain](docs/polish-terrain.md#stage-approval-and-remaining-environment-work-2026-10-20). |

## Comparable-game gaps

**Owner:** [docs/feature-roadmap.md, Comparable-game gaps](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). Features that
Splatoon and couch party shooters have and ColourWars lacks, rated medium value
or above (2026-10-27). The owner records the gaps left out by choice and the
excluded modes.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#11` | DECISION | MED | S | Sub weapons: all seven slices are done (2026-11-09): grenade, sprinkler and paint wall as a Weapon Select pick, played by bots, balanced, with their own art and cues. What remains is the author's attended play review. [Plan](docs/feature-roadmap.md#sub-weapons-road11). |
| `ROAD#13` | UNSCOPED | LOW | XL | More tool archetypes (Splatoon brush, blaster, splatling, dualies, brella), each with art, audio and bot ballistics. No longer blocked by `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#14` | UNSCOPED | LOW | L | Map pickups or power-ups (TowerFall chests, Smash items), behind a match toggle and with bot awareness. No longer blocked by `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#28` | UNSCOPED | MED | L | Cycling and paint-driven lifts are done (2026-11-03, Lift and Paint Lift Trials; trial values accepted 2026-11-05). Left: deliberate bot lift routing, including a stuck check that discounts carried y; never scoped. [Slice](docs/feature-roadmap.md#paint-driven-lift-second-slice-road28) |
| `ROAD#29` | UNSCOPED | MED | XL | Growing sponges need occupied resize, changing-area scoring and paint-credit contracts before implementation. [Boundary](docs/feature-roadmap.md#unscoped-paint-object-contracts-road28-road29). |
| `ROAD#23` | UNSCOPED | MED | L | Non-XInput pads (PlayStation, Switch Pro). The reader is Labrador's, so the engine part is a report with an exhibit; Steam Input covers a Steam release. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#24` | UNSCOPED | LOW | M | Pad aim assist so pad players can hold their own against the one mouse player; relates to `POL#3`, `POL#18` and `ROAD#5`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |

## Engine demonstration

**Owner:** [docs/review/gap-audit.md, Class 2](docs/review/gap-audit.md#class-2--gaps-in-the-role-as-the-engines-demonstration),
frozen as above, with live [menu and demonstration scopes](docs/feature-roadmap.md#per-player-menu-focus-gapd1).
These add to the hierarchies, never flatten them (AGENTS.md); D6 is `GAP#4.2`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#D2` | DEFERRED | MED | M | Additional widget/page bases need a concrete caller beyond OptionRow; scope only when an accepted page requires one. [Boundary](docs/feature-roadmap.md#other-demonstration-gaps). |

## Engine reports

**Owner:** [docs/review/engine-reports.md](docs/review/engine-reports.md), the live ledger. Check
engine master before filing, not the pin. Gap-audit Class 3 items not yet in the ledger are
`GAP#F*`. The pin is master `e7fc50e` (`v0.3.0` plus vsync). On 2026-11-05 the owed reports were
filed after a recheck against master `6189072`, and 28 was closed as declined. A pin bump that
answers one is when its workaround is deleted.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#PARKED` | DEFERRED | LOW | S | Asks to ride with a report: ignored `window_size_changed`, desktop-size query, atomic JSON write (F5, F9); registry name forgetting (report 26, with 27); install facts (PI-11; report 15); listener/voice cap (report 6); mouse-button names; display-awake owner; non-XInput reader. [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `ENG#26`, `ENG#27`, `ENG#30`, `ENG#33`, `ENG#34` | DEFERRED | LOW | S | Filed 2026-11-05 as [labrador#122](https://github.com/matt439/Labrador/issues/122) (27 with the 26 rider), #123, #124 and #125 (the Bug). Waits on the engine. When one is answered, spend it at the next pin bump; for 34, also settle what CI's strict gates accept. [Ledger](docs/review/engine-reports.md#live) |
| `ENG#35` | DECISION | LOW | S | Drafted 2026-11-05 from the hygiene review: `AudioResources` has no non-throwing sound-group lookup, so `sound_groups.cpp` catches `out_of_range`. Checked at master `5cbc6b3`. The author decides whether to file. [Report](docs/review/engine-reports.md#35-audioresources-has-no-non-throwing-sound-group-lookup---draft-with-find-or-add-exhibit) |
| `ENG#36` | DEFERRED | LOW | S | The pinned engine parses JSON recursively, so `reader_mutations` stops a save's nesting at 64; master `8362e2d` parses iteratively. At the pin bump that includes it, drop `save_max_nesting` and give the save the deep depths. Never filed. [Report](docs/review/engine-reports.md#36-json-parsing-recurses-once-per-nesting-level---answered-on-master-before-it-was-drafted) |
| `GAP#F10` | DEFERRED | LOW | S | `DrawList` colour transforms beyond multiply (greyscale, daltonize, hue remap); no exhibit yet. |

## Developer console

**Owner:** [docs/debug-console.md, Not done](docs/debug-console.md#not-done). Script control
flow and a console over the menus are deliberately not done and get no rows.

| id | status | sev | size | item |
|---|---|---|---|---|
| `DBG#1` | DEFERRED | LOW | M | Resume only when a concrete field/caller is selected; fifty-eight colour paths are qualified. String/font edits and missing-key insertion need separate scopes. [Scope](docs/debug-console-record.md#remaining-live-tuning-work-dbg1). |
| `DBG#2` | DEFERRED | LOW | L | Replays hold only on the same build, machine and step; across rounds is done ([record](docs/debug-console-record.md#replays-across-rounds-dbg2)). |
| `DBG#4` | DEFERRED | LOW | S | Typing in the live console blanks the whole keyboard seat; typed keys are play keys and the engine declined input capture (report 9), so any per-key gate is client policy. [Not done](docs/debug-console.md#not-done). |

## Tooling, CI and docs

**Owner:** the file named in each row.

| id | status | sev | size | item |
|---|---|---|---|---|
| `CI#7` | DEFERRED | LOW | S | The first hosted trial passed 96 of 97 painter leaves and paint-fx in both presets; music flow is skipped without an audio endpoint. After a week of scheduled nightlies (from 2026-11-11), promote what is stable and short to `ci.yml`. [Hygiene](docs/code-hygiene.md#promote-stable-nightly-checks-ci7). |
| `CI#1` | UNSCOPED | LOW | M | Find a reliable attended/foreground runner model before adding `check-debug-console` to CI; hosted focus remains unproven. [Constraint](docs/debug-console-record.md#foreground-checks-ci1), [workflow](.github/workflows/ci.yml). |

## Project review (2026-11-10)

**Owner:** [docs/review/2026-11-10-project-review.md](docs/review/2026-11-10-project-review.md),
a frozen record, so this file holds each row's status. It compared the project with Splatoon,
couch arena games, other paint games and similar C++ projects; every row was checked against the
code and the recorded decisions, and nothing was built or played. Its
[last section](docs/review/2026-11-10-project-review.md#proposed-and-not-given-rows) lists what
was proposed and given no row.

**Code and robustness**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#1` | DECISION | MED | M | A player wholly outside the bounds throws `PlayerOutOfBounds` and exits the game; user stages (`ROAD#16`) are never checked for enclosure. Author picks: out-of-bounds becomes an elimination (pits become authorable) or the scan refuses unsealed stages. [Record](docs/review/2026-11-10-project-review.md#a-player-leaving-the-arena-ends-the-process-rev1). |
| `REV#3` | BLOCKED | LOW | M | Planted sprinklers and walls take hits by a shot's travel bounding box, tested after the tick's contacts: diagonal shots hit up to about 17 units wide, and a player inside the wall is hit before it. Waits on the `ROAD#11` play review. [Record](docs/review/2026-11-10-project-review.md#breakable-subs-are-struck-by-travel-bounding-box-rev3). |
| `REV#4` | DECISION | LOW | M | An exception in the live round or instant replay step exits the game; recovery was scoped out with `GAP#7.3` and the replay has no known Release cause. Author's call whether to contain it at `GameLevel`, after `REV#1` removes the reachable throw. [Record](docs/review/2026-11-10-project-review.md#no-containment-around-the-match-or-replay-step-rev4). |
| `REV#5` | DECISION | LOW | M | A reset save or section, a kept-aside copy and a newer save about to lose sections are reported only on stderr, which a Release player never sees. Author's call (left undecided in `HYG#22`): an in-game notice, and a copy aside before a downgrade write. [Record](docs/review/2026-11-10-project-review.md#save-recovery-notices-reach-only-stderr-rev5). |
| `REV#7` | UNSCOPED | LOW | M | Static paint submits up to three sprites per owned 4-unit tile per pane, about 900 tiles on a full Crossfeed floor; draw cost was never measured. Measure a fully painted 4P arena first; merging runs would move painter pins. [Record](docs/review/2026-11-10-project-review.md#static-paint-draws-one-tile-at-a-time-rev7). |

**Release, README and repository**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#10` | DECISION | MED | M | Paint underfoot (POL#30) ships on with no HUD, audio or text cue. One line in HOW-TO-PLAY and the refill lesson can ride with REV#9; whether to build an enemy-paint cue on the meter or step sound is the author's call. [Record](docs/review/2026-11-10-project-review.md#paint-underfoot-has-no-player-cue-rev10). |
| `REV#12` | DECISION | MED | S | README shows no screenshot, GIF or clip in 511 lines though docs/images holds captures. The author picks the hero image and clip, given POL#16's art is unfinished; trimming settings and CI prose is a separate M. [Record](docs/review/2026-11-10-project-review.md#readme-has-no-screenshot-or-clip-rev12). |
| `REV#13` | DECISION | LOW | S | The F1/Back per-player developer panel draws in Release and is listed in Controls help. Keep it as the Release frame-time readout, hide it from help, or compile it out: the author's call; no decision is recorded. [Record](docs/review/2026-11-10-project-review.md#developer-text-panel-ships-in-release-rev13). |
| `REV#14` | DECISION | MED | S | Default Random palette still rolls pink_green and blue_yellow, the two pairs exempt from the colour-vision bar (one roll in three). A per-pair in-random flag would fix it but reopens ROAD#8's choice and changes seeded draws. [Record](docs/review/2026-11-10-project-review.md#random-palette-draws-the-exempt-pairs-rev14). |
| `REV#15` | DEFERRED | LOW | S | No CONTRIBUTING, issue form or SECURITY file. A short CONTRIBUTING (OOP rule, engine changes go to Labrador) and a bug form asking for build-info.txt and last-error.log; rides with the first release (REV#11). [Record](docs/review/2026-11-10-project-review.md#no-contributor-or-issue-files-rev15). |
| `REV#16` | UNSCOPED | LOW | S | Actions are pinned by major tag with no Dependabot, and the nightly's nine legs each rebuild. All are first-party actions and runner minutes are free, so weight is small; per-leg builds on pull requests are a recorded choice. [Record](docs/review/2026-11-10-project-review.md#ci-action-pins-and-nightly-rebuilds-rev16). |
| `REV#18` | UNSCOPED | LOW | L | feature-roadmap, code-hygiene and level-editor docs interleave reference with closed records. Only a level-editor manual split (M) has a clear reader; the rest is L, cosmetic and risks anchors TODO rows link to. [Record](docs/review/2026-11-10-project-review.md#large-docs-mix-manual-and-records-rev18). |

**Bots, stages and records**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#19` | UNSCOPED | MED | L | Bots chase any sighted enemy at any range, stand still once in range, and observe no clock, health or score. Four possible slices (paint/fight weighting, final-seconds push, retreat, strafing); sits beside the accepted ROAD#10 and is unmeasured. [Record](docs/review/2026-11-10-project-review.md#bots-chase-sighted-enemies-and-ignore-the-clock-rev19). |
| `REV#20` | UNSCOPED | MED | L | One bot policy plays all five tools; the Hard Sniper bot holds 12-15% of painted area in 1v1 mirrors (paint share, not win rate). Per-tool bot roles as new subclasses were never designed; POL#27 and ROAD#10 were accepted with the table on record. [Record](docs/review/2026-11-10-project-review.md#one-bot-policy-plays-all-five-tools-rev20). |
| `REV#21` | BLOCKED | LOW | M | Bots never target an enemy sprinkler and fire into an enemy paint wall, since BotWorld has no planted-sub query and bot sight ignores the wall. Bounded to 4-8 s per use. Waits on ROAD#11's attended play review. [Record](docs/review/2026-11-10-project-review.md#bots-do-not-answer-enemy-sprinklers-or-walls-rev21). |
| `REV#23` | DECISION | LOW | S | The bot's finishing window on a support is sized at the handicap refill rate, not the 1.25x own-paint rate that POL#30 ships on; the code calls it a sufficient minimum and ROAD#39's note left it undecided. Author decides: keep the safe bound or tighten. [Record](docs/review/2026-11-10-project-review.md#bot-finish-window-sized-without-own-paint-refill-rev23). |
| `REV#25` | DECISION | MED | M | Stage records are keyed by stage only, so solo has nothing graded to beat. Two shapes: per-stage Easy/Normal/Hard win marks or coverage medals at a fixed duration. Author's call: it sits beside no-stats, no-variant-leaderboard (ROAD#22) and solo-is-practice. [Record](docs/review/2026-11-10-project-review.md#solo-play-has-no-graded-stage-goal-rev25). |
| `REV#26` | DECISION | MED | S | Stage Select lists 13 stages in manifest order with nothing marking accepted arenas from the seven Turbulence Yard entries, and the accepted slab arena is itself named a Trial. Author picks: reorder and rename, an In Random mark, or a sectioned page (M). [Record](docs/review/2026-11-10-project-review.md#stage-select-does-not-mark-arenas-from-trials-rev26). |
| `REV#27` | UNSCOPED | MED | XL | The gate, ferry and lifts appear only on single-purpose Yard trial blockouts; Random is four stages. A finished gate or ferry arena in Random is a candidate aim, never designed. Art is POL#16 and bot lift routing ROAD#28; the aim is the author's to set. [Record](docs/review/2026-11-10-project-review.md#no-random-arena-uses-the-gate-ferry-or-lifts-rev27). |

**Player-facing UX**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#28` | UNSCOPED | MED | L | A missing pad with no spare leaves only Quit, which discards a series. Let a bot take the seat from the pause, or swap driver in the same seat between series rounds; needs the ROAD#20 roster lock relaxed and tape and record rules. [Record](docs/review/2026-11-10-project-review.md#bot-takeover-for-a-missing-seat-rev28). |
| `REV#29` | UNSCOPED | MED | M | Split-pane HUD text has no size floor and no size option: scale is 0.5 in a 4P pane at 1080p and 0.333 at 720p, with only the keyline floored. Unobserved, since POL#24 readability was waived; a floor rebaselines the painter's HUD fixtures. [Record](docs/review/2026-11-10-project-review.md#hud-text-floor-or-size-option-rev29). |
| `REV#30` | UNSCOPED | MED | M | Movement Course has no entry point: it is stage 4 of 13 behind five setup pages, hinted only on Mode Select. Consider a Home Practice row and ordering Trial stages after the accepted arenas; Home is an authored six-aspect layout. [Record](docs/review/2026-11-10-project-review.md#direct-entry-to-the-practice-course-rev30). |
| `REV#31` | UNSCOPED | LOW | L | No toggle-to-fire, stick swap, d-pad movement or follow-movement aim; Pad Buttons remaps four actions only. Four per-pad options in the ROAD#7 pattern; toggle fire is the one cheap slice. Beside ROAD#24 and GAP#4.2. [Record](docs/review/2026-11-10-project-review.md#input-accommodations-beyond-rebinding-rev31). |
| `REV#32` | UNSCOPED | LOW | M | Home has no play-again shortcut. GAP#6.1 already pre-fills every setup page and Join cannot be skipped, so it saves about four confirm presses for a conditional Home row and a second route into Join and Stage. [Record](docs/review/2026-11-10-project-review.md#play-again-with-last-setup-rev32). |
| `REV#33` | UNSCOPED | LOW | S | Menus are keys and d-pad only; the keyboard seat's mouse does nothing in them. Smallest slice is left click proceeds, right click backs, but those are default Shoot and Sub, so arming must cover a firing click at the buzzer. [Record](docs/review/2026-11-10-project-review.md#mouse-input-in-menus-rev33). |
| `REV#34` | UNSCOPED | LOW | L | GAP#2.3 residue: HUD, results and palettes answer it, but painted surface is still told apart by hue alone, and POL#30 makes ownership a gameplay input. An optional per-team pattern in PaintTile; may clash with terrain hatching. [Record](docs/review/2026-11-10-project-review.md#non-colour-team-channel-on-paint-rev34). |
| `REV#35` | UNSCOPED | MED | M | A worn profile's name shows only on Join and the Pad rows; HUD identity, Results, Team and Weapon rows and the pause list say Player N. Extends ROAD#21. Needs fit in the 4P identity panel and Results rows that share width with awards. [Record](docs/review/2026-11-10-project-review.md#profile-name-on-hud-and-results-rev35). |
| `REV#36` | BLOCKED | LOW | M | Respawn grace is already a recorded POL#17 fallback, to add only on observed spawn pressure; the sprinkler is a reason to look for it. Blocked on ROAD#11's attended play review. Choosing among the team's spawns is new and undesigned. [Record](docs/review/2026-11-10-project-review.md#respawn-grace-and-spawn-choice-rev36). |

**Presentation and feedback**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#37` | UNSCOPED | MED | M | Shooter-side hit confirmation: feedback is victim-keyed, so a hit on a target outside or tiny in the shooter's pane is unconfirmed. Pane cue and shooter rumble never designed; the hit_confirm sound stays declined under POL#23. [Record](docs/review/2026-11-10-project-review.md#shooter-side-hit-and-splat-confirmation-rev37). |
| `REV#38` | DECISION | MED | M | Heavily painted 4P reveals captured (2026-11-10): strips measure 2-3 px at 720p and 4 px at 1080p on Crossfeed, 1-2 and 2-3 px on Close Quarters. They show whose paint is where, not who won; the cards carry that. Author's call: a reveal-only minimum weight or body tint, or leave as is. [Capture](docs/polish-match-accents.md#paint-strip-weight-in-the-reveal-rev38), [record](docs/review/2026-11-10-project-review.md#paint-strips-are-hairline-thin-in-the-reveal-rev38). |
| `REV#39` | DEFERRED | MED | S | Post-reveal Results and series pages show only A/B letters in ink; no team colour. Keylined colour chips on the plates and rows fit the hidden-bar decision; colour-name winner copy is a design call. With POL#16. [Record](docs/review/2026-11-10-project-review.md#post-reveal-results-pages-carry-no-team-colour-rev39). |
| `REV#40` | UNSCOPED | LOW | M | Persistent damage on the figure as accumulating enemy-colour smears; health shows only on the HUD meter. Never designed: needs authored art, may not read in quarter panes and erodes team-colour identity. [Record](docs/review/2026-11-10-project-review.md#show-damage-on-the-figure-as-enemy-paint-rev40). |
| `REV#41` | UNSCOPED | LOW | S | Time-up card at the pull-back: the round ends on sound and the camera move, with no text. The closing clock already announces it; a banner over the scripted reveal beat is a fiction call. Never designed. [Record](docs/review/2026-11-10-project-review.md#no-on-screen-time-up-signal-rev41). |
| `REV#42` | DECISION | LOW | M | Crossfeed and Close Quarters share one music loop and sting, the only collision in Random; Press Tower's reuse is deliberate. Decide whether Crossfeed gets its own arrangement, given POL#23 accepted and listening waived. [Record](docs/review/2026-11-10-project-review.md#crossfeed-shares-the-close-quarters-music-rev42). |
| `REV#43` | DECISION | LOW | S | Player-owned cues play centred at authored pitch for every seat; the wrapper already takes pan and pitch. Decide whether per-seat offsets are wanted, since the mix is accepted (POL#23) and can only be judged by ear. [Record](docs/review/2026-11-10-project-review.md#per-seat-pan-and-pitch-for-player-cues-rev43). |
| `REV#44` | UNSCOPED | LOW | M | Series end is the normal Results page plus a winner line and the win ledger. A round-by-round strip, a match-point announcement and series-wide awards were never designed. [Record](docs/review/2026-11-10-project-review.md#series-conclusion-is-the-ordinary-results-page-rev44). |
| `REV#45` | UNSCOPED | LOW | M | More paint-flavoured awards ahead of splats would extend ROAD#12, but none of the tallies is collected today. A consolation award for every player conflicts with the recorded one-each, order-is-the-design rule. Never designed. [Record](docs/review/2026-11-10-project-review.md#wider-awards-set-and-a-consolation-call-out-rev45). |
| `REV#46` | UNSCOPED | LOW | M | Roster strip by the clock with respawning players crossed out; the HUD has no roster element. Never designed, and it sits beside the ROAD#18 choice to keep enemy state off the HUD, so the author rules first. [Record](docs/review/2026-11-10-project-review.md#team-status-strip-showing-who-is-respawning-rev46). |
| `REV#47` | UNSCOPED | LOW | M | Respawn shows neither attacker nor tool. Needs a last-attacker record and a new HUD element over a wait designed to be wordless; close to the kill attribution declined under POL#30. Never designed. [Record](docs/review/2026-11-10-project-review.md#tell-a-splatted-player-who-got-them-rev47). |

**New features from the comparison**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#48` | UNSCOPED | MED | M | Whole-round time-lapse from one arena camera as an optional End row after the reveal (extends ROAD#19). Needs paced playback, a whole-arena layout in the rebuilt level and a HUD ruling. Never designed. [Record](docs/review/2026-11-10-project-review.md#whole-round-time-lapse-replay-rev48). |
| `REV#49` | UNSCOPED | MED | M | Save the finished arena as a PNG from an explicit End or Results action. Capture and WIC writer exist only in Debug; needs a Release home, the reveal frame, a folder, copy and a check. Never designed. [Record](docs/review/2026-11-10-project-review.md#save-the-finished-picture-as-a-png-rev49). |
| `REV#50` | DECISION | LOW | M | Author's call: should an elimination leave a small burst of the attacker's paint (Splatoon)? Burst path and attacker credit exist, but it makes kills score territory, beside POL#30 and the balance accepted 2026-11-05. [Record](docs/review/2026-11-10-project-review.md#elimination-leaves-a-burst-of-attacker-paint-rev50). |
| `REV#51` | DECISION | LOW | XL | Author's call: trial own-colour wall slide and wall jump (Splatoon, Splasher)? Never ruled on under POL#30. Reopens accepted arenas, reach numbers, bot routing and editor reach aids; default-off trial at most. [Record](docs/review/2026-11-10-project-review.md#own-colour-wall-climb-or-wall-jump-rev51). |
| `REV#52` | UNSCOPED | LOW | L | Paint-keyed bounce pad as a paint object beside ROAD#28 and ROAD#29: static, launches the team holding its top paint, needs none of the sponge contracts. New class, format kind, art, cue and trial stage. Never designed. [Record](docs/review/2026-11-10-project-review.md#paint-keyed-bounce-pad-object-rev52). |
| `REV#53` | UNSCOPED | LOW | L | Eraser stage object that returns tiles to neutral (Splasher water, de Blob). Breaks the paint-only-arrives invariant that wet strips, paint objects, bot finishing, underfoot and awards assume. Never designed. [Record](docs/review/2026-11-10-project-review.md#eraser-stage-object-that-clears-paint-rev53). |
| `REV#54` | UNSCOPED | LOW | M | Coverage-over-time strip on Results after the reveal: sample both totals each second, replay-neutral, and fit a two-colour strip into the validated paper layout. Sits against POL#15's quick Results. Never designed. [Record](docs/review/2026-11-10-project-review.md#coverage-over-time-strip-on-results-rev54). |
| `REV#55` | DECISION | LOW | M | Author's call: untimed, unrecorded exploration of any arena for 1-4 players (Splatoon Recon)? A no-winner session is outside the 2026-11-03 options ruling; cheaper variant is solo practice on any stage. [Record](docs/review/2026-11-10-project-review.md#untimed-group-exploration-of-any-arena-rev55). |
| `REV#56` | BLOCKED | LOW | S | Movement Course has no lesson station for the Sub action: add one JSON lesson with its checks and text fit. Waits on ROAD#11's play review so it is written once. A practice target dummy is separate and unscoped. [Record](docs/review/2026-11-10-project-review.md#sub-lesson-station-in-movement-course-rev56). |
| `REV#57` | DECISION | LOW | M | Author's call: give the sniper a hold-to-charge with a line opponents can see (Splatoon chargers)? Reopens a tool and bot ballistics accepted 2026-11-05, with no observed problem; if taken, rides with ROAD#13. [Record](docs/review/2026-11-10-project-review.md#sniper-charge-or-readable-warning-rev57). |
| `REV#58` | UNSCOPED | LOW | L | Rotation for five to eight people (Nidhogg 2 tournaments): profile names, who sits out, per-person standings. Needs a second ledger beside the team-win Session and a per-person winner rule. Never designed. [Record](docs/review/2026-11-10-project-review.md#party-rotation-for-more-than-four-people-rev58). |
| `REV#59` | UNSCOPED | LOW | L | Bots-only attract match behind the title. Level and LevelBuilder refuse a setup with no human view, so it needs a spectator view and a new stack shape. The colourless front end itself is POL#16. Never designed. [Record](docs/review/2026-11-10-project-review.md#attract-match-behind-the-title-rev59). |
| `REV#60` | DEFERRED | LOW | M | Fill the spare quadrant of three-human matches with territory-free content (clock, rosters), the row the ROAD#17 decline invited. Must repeat what panes show; needs a draw region and 3P painter cover. With POL#16. [Record](docs/review/2026-11-10-project-review.md#territory-free-use-of-the-spare-3p-quadrant-rev60). |

**Tooling, editor and distribution**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#61` | OPEN | LOW | S | `x64-asan` and `x64-analyze` presets and nightly legs are in; all 80 default cases pass under ASan locally. Next: triage this repository's eleven `/analyze` warnings. Not run hosted: confirm both legs on the pull request's nightly trial, then delete. No clang-tidy. [Owner](docs/code-hygiene.md#sanitizer-and-static-analysis-presets-rev61). |
| `REV#63` | DECISION | MED | M | Access violations and terminate leave no record, and Release keeps no PDB. Slice: a filter and terminate handler writing one local dump, plus a PDB artifact. Author's call: a dump holds memory the privacy rules exclude. [Record](docs/review/2026-11-10-project-review.md#no-crash-dump-and-no-release-symbols-rev63). |
| `REV#64` | DECISION | MED | L | The editor is left out of the ZIP and compiles absolute checkout paths, so shipping it means relocatable content lookup, hidden source flows, a guide and package checks. Author's call: it redrafts design-process P5, J4, F6 and F7. [Record](docs/review/2026-11-10-project-review.md#level-editor-is-not-shipped-to-players-rev64). |
| `REV#65` | BLOCKED | LOW | M | No Play Stage or Open My Stages folder item in the editor. Waits on REV#64: it pays only if the editor ships, and a Release launch argument cuts against the rule that Release takes none. [Record](docs/review/2026-11-10-project-review.md#editor-has-no-test-play-command-rev65). |
| `REV#66` | UNSCOPED | LOW | L | Editor has no multi-select, recent files, drag-and-drop open or autosave. Group move is the one with authoring value; a textured canvas contradicts the schematic scope. Scope with the next editor slice. [Record](docs/review/2026-11-10-project-review.md#editor-lacks-multi-select-and-recent-files-rev66). |
| `REV#67` | DECISION | LOW | M | The only channel is an unsigned GitHub ZIP, itself unpublished. An itch.io page with a butler step in publish is the worthwhile addition; signing and a checksum add little. Author's call, after the first tag. [Record](docs/review/2026-11-10-project-review.md#distribution-is-one-unsigned-github-zip-rev67). |
| `REV#68` | UNSCOPED | LOW | L | No document scopes a Steam release (store page, Remote Play Together, prompts, Deck pass); a note beside ROAD#23, whose grading does not move. Steamworks claims unchecked; nothing is released on any channel yet. [Record](docs/review/2026-11-10-project-review.md#steam-release-is-not-scoped-rev68). |
| `REV#69` | DECISION | LOW | S | Nothing tells players that Parsec-style streaming can stand in for online play. One hedged HOW-TO-PLAY paragraph, but qualifying it needs two machines, a remote friend and pads: the author's hardware and call. [Record](docs/review/2026-11-10-project-review.md#stream-based-online-play-is-undocumented-rev69). |
## Areas with no open rows

A new row for one of these gets its section back.

| area | owner | a row returns with |
|---|---|---|
| Hygiene review queue | [code-hygiene.md](docs/code-hygiene.md#hygiene-review-2026-11-05) | a new finding or regression in the completed slices; the last seven queued rows closed 2026-11-07 |
| Feel and feedback | [polish-plan.md](docs/polish-plan.md) §2, §4, §8; [Movement Course](docs/polish-movement-course.md) | a feel or feedback regression, or a reopened call; `POL#3`, `POL#18`, `POL#19`, `POL#38`, `POL#39` and `POL#45` were accepted or waived 2026-11-05 |
| Levels and the editor | [polish-plan.md](docs/polish-plan.md) §3, §8; [level-editor.md](docs/level-editor.md) | a new arena or editor slice; `POL#21`, `POL#27`, `POL#30`, `POL#31` and `ED#5` were settled 2026-11-05 |
| Audio replacement | [polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set) | a missing cue hook or a mix defect; `POL#23` was accepted and `ROAD#3` waived 2026-11-05 |
| Acceptance and release | [feature-roadmap.md](docs/feature-roadmap.md#review-follow-up-ammunition-coverage-and-release-checks), [polish-plan §7](docs/polish-plan.md#7-acceptance-and-delivery) | a release candidate needing a check the author has not waived; `ROAD#1`, `ROAD#2`, `POL#24` and `POL#25` were waived 2026-11-05, not performed |
| Game gaps (gap audit Class 1) | [docs/review/gap-audit.md](docs/review/gap-audit.md) (frozen) | a new finding; `ROAD#5`, `ROAD#7`, `ROAD#9`, `ROAD#10`, `GAP#4.2`, `GAP#5.1`, `GAP#L9` and `GAP#L11` were accepted or waived 2026-11-05 (no translation language chosen) |
| Backstory build | [backstory.md, Building it](docs/backstory.md#building-it) | a missing or regressed story beat; pop, sigh and chair integration are complete |
| Design process (`DES#1`-`DES#4`) | [design-process.md](docs/design-process.md) | a new story, persona or journey, or a check gap a later trace finds; `DES#4`, the code re-review, was the last (2026-11-03) |
| Closed engine reports (1–25, 28, 29, 31, 32) | [engine-reports.md, Closed](docs/review/engine-reports.md#closed) | a pin bump that answers a live report and leaves a workaround to delete |
| Feature-roadmap increments 1–5 | [feature-roadmap.md](docs/feature-roadmap.md) | a sixth increment, or a regression from the `ROAD#2` session |
| 2026-09-26 review fixes | `docs/review/2026-09-26-*.md` (frozen) | a new review round; the hardware residue is `ROAD#2` |
| Splatoon borrowings (no live score, random pairs) | gap audit 1.1, 2.1 | only the author reversing the decision |
| Startup content validation | gap audit 8.1, `game/config/content_validation.cpp` | a content kind added without validation |
| Content pipeline | [Text scope](docs/menu-presentation.md#remaining-text-slices-gap84), [art/content policy](game/art/README.md#runtime-content-separation-gap86) | a new authored text role or accepted runtime-content requirement; the scoped `GAP#8.4` extraction is complete |
| Code TODO/FIXME markers | `game/`, `editor/`, `scripts/`, `cmake/` | a new marker, which should be a row here instead |
| OOP showcase (`OOP#1`) | [oop-guide.md](docs/oop-guide.md) | a new hierarchy, extension point or registration the guide no longer describes; never a request to flatten one |
| Movement Course icon (`ED#6`, retired) | [level-editor.md](docs/level-editor.md#movement-course-icon-ed6-retired) | an accepted screen actually displays stage icons; current Stage Select uses a live preview |

---

## How to read this file

Rules for writing it: read them before editing a row, not before picking one.

**This file is an index, not a work order.** One row per open item: an id, a status, two grades,
one clause and a pointer. Why an item matters, its repro, its design and its acceptance belong in
the owner; a second copy here goes stale unnoticed.

1. **A row leaves this file in the same commit that finishes it.** Update the owner (for a live
   doc) and delete the row; no closed rows stay here. An emptied section becomes one line under
   [Areas with no open rows](#areas-with-no-open-rows).
2. **A live owner is the source of truth for status;** when the two disagree, this file is stale.
   The frozen reviews (`docs/review/gap-audit.md`, `docs/review/2026-09-26-*.md`) are records and
   are never edited to match: for their rows this file holds the status, and the commit that
   closes one says so.
3. **When part of an item lands, replace the clause with what's left;** don't append to it. Keep a
   row under about 350 characters.
4. **New work starts in an owner.** Add it to the doc it belongs to (the polish plan, the
   backstory, the ledger, the console's Not done), then give it a row here. Engine work belongs
   in Labrador; what this repository owes it is a report in the ledger.
5. **NOW changes only when its items finish or the author reorders them.**
6. **Keep readiness honest.** `OPEN` needs a concrete local next step in its owner. A human
   acceptance session is `DECISION`, a named dependency is `BLOCKED`, and a broad capability
   wishlist stays `UNSCOPED` or `DEFERRED`. Implementation does not close a human review gate.

**`id`:** a stable handle, not an order; a retired id is never reused. Gap-audit rows keep the
audit's own section number (`GAP#4.2`, `GAP#D3`, `GAP#F8`), ledger rows the report number
(`ENG#23`); the rest are `PREFIX#n` by owner: `STORY` backstory, `POL` polish plan, `ED` level
editor, `ROAD` feature roadmap, `DBG` developer console, `HYG` code hygiene,
`OOP` OOP guide, `DES` design process, `REV` the 2026-11-10 project review,
`DOC`/`CI`/`TOOL` the named file.

**`status`**, a closed list:
- `OPEN`: ready to work.
- `DECISION`: the next step needs the author: a call, files only they can supply, or their
  hardware (a pad, a clean PC). Always named in NOW.
- `BLOCKED`: waits on another row, named in the clause.
- `DEFERRED`: paused by priority; its owner's condition applies.
- `UNSCOPED`: noticed, never examined.

**`sev`**, the cost of leaving it open, not difficulty:
- `CRIT`: wrong or undefined behaviour today, or blocking the head of the queue.
- `HIGH`: a visible defect, or a missing piece the current aim depends on.
- `MED`: a real gap, but bounded or latent.
- `LOW`: housekeeping, doc debt, convenience.

**`size`:** engineer-hours: `S` ≤2 h · `M` 2–6 h · `L` 6–20 h · `XL` >20 h.

**Out of scope for rows:** findings that hold the game to the engine's T11 ("flatten the
hierarchy", "make it a variant") are misfiled (AGENTS.md), and a missing-bank silent fallback is correct.
