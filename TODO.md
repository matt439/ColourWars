# TODO: open work

An index of open work, one row per item; each row points to the doc that owns the detail. The
rules for editing it are at the end, in [How to read this file](#how-to-read-this-file). A fresh
thread starts here: read **NOW**, then the owner of the row it picks up.

## NOW

**The only ordering in this file.** Keep this block to the current aim and the next few actions;
results go in their owners, and `git log` is the changelog.

**Aim: after `v0.1.0`.** [`v0.1.0`](https://github.com/matt439/ColourWars/releases/tag/v0.1.0)
was published as a pre-release on 2026-11-13 ([record](docs/releasing.md#first-release-v010));
`master` is now `0.1.1` (`REL#6`).

The engine pin moved to master `1dc8533c` on 2026-11-13 (`ENG#36`, `PIN#2` and `PIN#3` done,
[record](docs/review/engine-reports.md#what-the-2026-11-13-bump-changed)); five atlases now ship
as PNG through the engine's loader ([`PIN#3`](docs/review/engine-reports.md#png-atlases-and-texconv-pin3)).

Next actions: the `OPEN` rows, in any order. The required `gate` and up-to-date `master` rule
still apply. The author's calls of 2026-11-13 on the decision rows are recorded in
[Project review calls](docs/feature-roadmap.md#project-review-calls-2026-11-13) and the owners;
they opened `REV#1`, `REV#13`, `REV#14`, `REV#63` and `POL#49`.

**Author calls** (every `DECISION` row):

- `ROAD#13`: whether to build umbrella slices 2-4 (`PIN#2` is done).
- `ENG#PARKED`: approve the drafted mouse-button comment for labrador#122 and the engine `REV#60` note.
- `ENG#39`: file the `FLT_MAX` round-trip draft as an issue, or fix it in the engine.

The hardware, participant and listening checks waived on 2026-11-05 were not performed. Arena
art (`POL#16`) stays deferred; bot lift routing, non-XInput pads and growing sponges are
deferred.

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6. It records the 2026-10-09
switch to Paint.exe Maximalism and the accepted 2026-10-15 round-2 art rules. The pixel-kit
questions in the polish sub-docs are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#16` | DEFERRED | MED | XL | Extend the approved treatment through the accepted arenas (selected 2026-11-05); remaining UI art is a later polish pass. Waits on the author setting it as the next aim. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |
| `POL#35` | DEFERRED | LOW | S | Signage and captioned Paint-artefact jokes need the author's wording; with `POL#16`. [Terrain](docs/polish-terrain.md#stage-approval-and-remaining-environment-work-2026-10-20). |

## Comparable-game gaps

**Owner:** [docs/feature-roadmap.md, Comparable-game gaps](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27)
and [Backlog audit additions](docs/feature-roadmap.md#backlog-audit-additions-2026-11-12) (2026-11-12).
Features that Splatoon and couch party shooters have and ColourWars lacks, rated medium value
or above (2026-10-27). The owner records the gaps left out by choice and the excluded modes.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#13` | DECISION | LOW | M | Umbrella answers recorded 2026-11-13 (Q1 umbrella, Q3-Q7 as recommended). Left: the author's call, now that `PIN#2` is done, whether to build slices 2-4, which reopens the five-tool balance and adds four unheard cues. Slice 1 is `REV#3`. [Proposal](docs/feature-roadmap.md#umbrella-tool-proposal-road13). |
| `ROAD#14` | UNSCOPED | LOW | L | Map pickups or power-ups (TowerFall chests, Smash items), behind a match toggle and with bot awareness. No longer blocked by `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#28` | DEFERRED | LOW | L | Cycling and paint-driven lifts are done. Left: deliberate bot lift routing. Deferred behind `REV#27`, because lifts stand only on the two trials that are out of Random, so routing has no consumer yet. [Boundary](docs/feature-roadmap.md#unscoped-paint-object-contracts-road28-road29). |
| `ROAD#29` | DEFERRED | LOW | M | Growing sponges parked 2026-11-13 behind `REV#27`: no G1-G7 picks until a Random arena wants a sponge, then resume from the [contract draft](docs/feature-roadmap.md#growing-sponge-contract-draft-road29)'s recommendations. |
| `ROAD#23` | DEFERRED | MED | L | Non-XInput pads (PlayStation, Switch Pro). The engine ask is parked in `ENG#PARKED` and there is no client exhibit; resumes when such a pad is in hand or a store release is chosen (`REV#68`). [Input](docs/feature-roadmap.md#input). |
| `ROAD#24` | DEFERRED | LOW | M | Pad aim assist against the one mouse seat. Deferred until attended play shows pad seats consistently losing to the mouse seat (story B8); relates to `POL#3`, `POL#18` and `ROAD#5`. [Input](docs/feature-roadmap.md#input). |
| `ROAD#41` | UNSCOPED | LOW | M | Grey, non-scoring practice targets in the workshop that pop and redraw with no damage readout; needs a scope section first. Related to `REV#56`. [Item](docs/feature-roadmap.md#practice-targets-in-the-workshop-road41). |
| `ROAD#42` | UNSCOPED | LOW | M | Weapon Select could show each tool's shot shape, drawn from its tuning, beside the text (story A6). No problem has been observed; it may wait on `POL#16`. [Item](docs/feature-roadmap.md#shot-shapes-on-weapon-select-road42). |
| `ROAD#43` | UNSCOPED | LOW | L | Flood-fill paint object that fills one authored region for the team holding its trigger. Needs a contract draft on the lift precedent before an author call on the territory swing. [Item](docs/feature-roadmap.md#flood-fill-paint-object-road43). |
| `ROAD#44` | OPEN | LOW | M | Circle collision shapes for the ball and grenade; the pin has the circle narrow phase (`f6752ca7`) since 2026-11-13. Measure the contact changes in a seeded replay, then the author decides on hit feel. [Item](docs/feature-roadmap.md#circle-shapes-for-round-projectiles-road44). |

## Engine demonstration

**Owner:** [docs/review/gap-audit.md, Class 2](docs/review/gap-audit.md#class-2--gaps-in-the-role-as-the-engines-demonstration),
frozen as above, with live [menu and demonstration scopes](docs/feature-roadmap.md#per-player-menu-focus-gapd1).
These add to the hierarchies, never flatten them (AGENTS.md); D6 is `GAP#4.2`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#D2` | DEFERRED | MED | M | Additional widget/page bases need a concrete caller beyond OptionRow; scope only when an accepted page requires one. [Boundary](docs/feature-roadmap.md#other-demonstration-gaps). |

## Engine reports

**Owner:** [docs/review/engine-reports.md](docs/review/engine-reports.md), the live ledger. Check
engine master before filing, not the pin. The pin is master `1dc8533c` (2026-11-13, 129
commits past `v0.3.0` plus vsync; no tag since `v0.3.0`). On 2026-11-05 the owed reports were
filed and 28 was closed as declined; on 2026-11-13 35 and 38 were parked, 36 spent at the bump
and 37 withdrawn. A pin bump that answers one is when its workaround is deleted.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#PARKED` | DECISION | LOW | S | Asks waiting for a vehicle: window presence (was 38), `window_size_changed`, desktop size (exhibit for engine `REV#83`), atomic JSON write, registry forgetting, install facts, listener/voice cap, display-awake owner, non-XInput reader (`ROAD#23`), colour transforms (was `GAP#F10`), engine `GAP#6`, sound-group lookup (was `ENG#35`). Author: approve the drafted mouse-button comment for labrador#122 and the engine `REV#60` note, then post them. [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `ENG#26`, `ENG#27`, `ENG#30`, `ENG#33`, `ENG#34` | DEFERRED | LOW | S | Filed 2026-11-05 as [labrador#122](https://github.com/matt439/Labrador/issues/122) (27 with the 26 rider), #123, #124 and #125 (the Bug). Waits on the engine. When one is answered, spend it at the next pin bump; for 34, also settle what CI's strict gates accept. [Ledger](docs/review/engine-reports.md#live) |
| `ENG#39` | DECISION | LOW | S | `FLT_MAX` written to JSON reads back one double step above it, so `gameplay_tuning` tests one float below. Drafted 2026-11-13, not filed. Author: issue route or engine fix. [Report](docs/review/engine-reports.md#39-a-float-written-as-flt_max-does-not-read-back---draft-not-filed) |

## Developer console

**Owner:** [docs/debug-console.md, Not done](docs/debug-console.md#not-done). Script control
flow and a console over the menus are deliberately not done and get no rows.

| id | status | sev | size | item |
|---|---|---|---|---|
| `DBG#1` | DEFERRED | LOW | M | Resume only when a concrete field/caller is selected; fifty-eight colour paths are qualified. String/font edits and missing-key insertion need separate scopes. [Scope](docs/debug-console-record.md#remaining-live-tuning-work-dbg1). |
| `DBG#2` | DEFERRED | LOW | L | Replays hold only on the same build, machine and step; across rounds is done ([record](docs/debug-console-record.md#replays-across-rounds-dbg2)). |
| `DBG#4` | DEFERRED | LOW | S | Typing in the live console blanks the whole keyboard seat; typed keys are play keys and the engine declined input capture (report 9), so any per-key gate is client policy. [Not done](docs/debug-console.md#not-done). |

## Code hygiene

**Owner:** [docs/code-hygiene.md, TODO audit follow-ups](docs/code-hygiene.md#todo-audit-follow-ups-2026-11-12).
The last seven rows of the [2026-11-05 review](docs/code-hygiene.md#hygiene-review-2026-11-05)
queue closed 2026-11-07.

| id | status | sev | size | item |
|---|---|---|---|---|
| `HYG#95` | OPEN | LOW | S | Baseline comparison landed (`report-analyze.ps1 -Baseline`, nightly passes it, keeps continue-on-error); still to see the hosted nightly job summary show the comparison, on the pull request's nightly trial or the first night after merge. [Item](docs/code-hygiene.md#read-the-nightly-analyze-count-against-a-baseline-hyg95). |
| `HYG#96` | OPEN | LOW | S | `close` job landed in nightly.yml (closes `Nightly sweep failed` on a green scheduled night; `workflow_dispatch` input `close_trial` runs only that path). Owed after a push: a dispatched trial closing a test issue (open #72 has the same title and would close too). [Item](docs/code-hygiene.md#close-the-nightly-issue-on-a-green-night-hyg96). |

## Tooling, CI and docs

**Owner:** the file named in each row.

| id | status | sev | size | item |
|---|---|---|---|---|
| `CI#7` | DEFERRED | LOW | S | Tallied: paint-fx passed 7 of 7 scheduled nights (about 1 min of check per preset), and the 11-09 red night caught real regressions. After 2026-11-17's run, run check-paint-fx in ci.yml's checks legs; the rest stay nightly. [Hygiene](docs/code-hygiene.md#promote-stable-nightly-checks-ci7). |

## Project review (2026-11-10)

**Owner:** [docs/review/2026-11-10-project-review.md](docs/review/2026-11-10-project-review.md),
a frozen record, so this file holds each row's status and, for a row promoted to `OPEN` by the
2026-11-12 audit, its first step. It compared the project with Splatoon, couch arena games, other
paint games and similar C++ projects; every row was checked against the code and the recorded
decisions, and nothing was built or played. Its
[last section](docs/review/2026-11-10-project-review.md#proposed-and-not-given-rows) lists what
was proposed and given no row. Here `UNSCOPED` means examined by the review but never designed.

**Code and robustness**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#3` | OPEN | LOW | M | Planted sprinklers and walls take hits by a shot's travel bounding box, tested after the tick's contacts: diagonal shots hit up to about 17 units wide, and a player inside the wall is hit before it. First step: sweep breakables in `Projectile::resolve_sweep` before player contacts ([roadmap](docs/feature-roadmap.md#umbrella-tool-proposal-road13)). [Record](docs/review/2026-11-10-project-review.md#breakable-subs-are-struck-by-travel-bounding-box-rev3). |

**Release, README and repository**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#10` | DEFERRED | MED | S | No live enemy-paint cue (2026-11-13); the HOW-TO-PLAY line and the refill lesson explain it. Reopens if players cannot tell why they slowed, as one choice between a cue (tank state first) and milder `POL#30` amounts. [Item](docs/polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30). |
| `REV#16` | DEFERRED | LOW | S | Actions are pinned by major tag with no Dependabot, and the nightly's legs each rebuild. Each Dependabot pull request would need the owner to merge it and run the full gate; per-leg builds are a recorded choice. [Record](docs/review/2026-11-10-project-review.md#ci-action-pins-and-nightly-rebuilds-rev16). |
| `REV#18` | UNSCOPED | LOW | M | Level-editor manual/record split done 2026-11-12 ([record](docs/level-editor-record.md)). Left: moving closed rows out of feature-roadmap.md and code-hygiene.md (L, cosmetic). [Record](docs/review/2026-11-10-project-review.md#large-docs-mix-manual-and-records-rev18). |

**Bots, stages and records**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#19` | UNSCOPED | LOW | M | Bots chase any sighted enemy and ignore the clock, health and score. The sight ladder (2026-11-12) found that shorter sight cuts fights but does not raise paint, so there is no paint case for the four behaviour slices; they stay undesigned. [Record](docs/gameplay-tuning.md#sight-distance-ladder-rev19-2026-11-12) |
| `REV#20` | UNSCOPED | MED | L | One bot policy plays all five tools. Aim/reaction sensitivity is measured (`TOOL#5`, closed 2026-11-13); exact event counters reopen only with per-tool roles, which remain unscoped. Sniper's low paint is intended. [Record](docs/review/2026-11-10-project-review.md#one-bot-policy-plays-all-five-tools-rev20). |
| `REV#21` | OPEN | LOW | M | Bots never target an enemy sprinkler and fire into an enemy paint wall, since BotWorld has no planted-sub query and bot sight ignores the wall. Bounded to 4-8 s per use. Released 2026-11-13 by ROAD#11's waived review. [Record](docs/review/2026-11-10-project-review.md#bots-do-not-answer-enemy-sprinklers-or-walls-rev21). |
| `REV#27` | DEFERRED | MED | XL | The gate, ferry and lifts appear only on single-purpose Yard trial blockouts; Random is four stages. A finished gate or ferry arena in Random is a candidate aim, the author's to set; deferred with `POL#16`. Bot lift routing is `ROAD#28`. [Record](docs/review/2026-11-10-project-review.md#no-random-arena-uses-the-gate-ferry-or-lifts-rev27). |

**Player-facing UX**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#28` | DEFERRED | MED | L | No bot takeover of a missing seat for now (2026-11-13). Trigger: a series lost to a pad that cannot return; then only the between-rounds version, which relaxes the `ROAD#20` roster lock. [Calls](docs/feature-roadmap.md#project-review-calls-2026-11-13), [record](docs/review/2026-11-10-project-review.md#bot-takeover-for-a-missing-seat-rev28). |
| `REV#29` | DEFERRED | MED | M | No split-pane HUD text floor (2026-11-13). Trigger: a player finds the text unreadable, or `POL#16` rebaselines the HUD fixtures; then an opt-in Options Normal/Large row, not a floor. [HUD](docs/polish-hud.md#split-pane-text-size-rev29-2026-11-12). |
| `REV#30` | DEFERRED | MED | M | Narrowed 2026-11-12 to a Practice row on Home for Movement Course (README already points newcomers there). `REV#26` put the course fifth, after the Random arenas (2026-11-13). Home is an authored six-aspect layout; deferred by priority. [Record](docs/review/2026-11-10-project-review.md#direct-entry-to-the-practice-course-rev30). |
| `REV#32` | UNSCOPED | LOW | M | Home has no play-again shortcut. GAP#6.1 already pre-fills every setup page and Join cannot be skipped, so it saves about four confirm presses for a conditional Home row and a second route into Join and Stage. [Record](docs/review/2026-11-10-project-review.md#play-again-with-last-setup-rev32). |
| `REV#36` | DEFERRED | LOW | M | Respawn grace is already a recorded POL#17 fallback, to add only on observed spawn pressure; the sprinkler is a reason to look for it. ROAD#11's review was waived 2026-11-13, so it waits on pressure seen in play. Choosing among the team's spawns is new and undesigned. [Record](docs/review/2026-11-10-project-review.md#respawn-grace-and-spawn-choice-rev36). |

**Presentation and feedback**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#37` | UNSCOPED | LOW | M | Hit feedback is keyed to the victim. A 2026-11-12 tally found 1.7% of hit ticks land outside the shooter's pane (Sniper 21%) and none tiny. A Sniper-range shooter cue is undesigned; hit_confirm stays declined (POL#23). [Record](docs/polish-combat-feedback.md#shooter-side-confirmation-tally-rev37-2026-11-12) |
| `REV#38` | DEFERRED | MED | M | Reveal paint strips stay as drawn (2026-11-13). Trigger: `POL#16` changes how paint is drawn, or a viewer at couch distance cannot see where the paint is. [Capture](docs/polish-match-accents.md#paint-strip-weight-in-the-reveal-rev38), [record](docs/review/2026-11-10-project-review.md#paint-strips-are-hairline-thin-in-the-reveal-rev38). |
| `REV#39` | DEFERRED | MED | S | Post-reveal Results and series pages show only A/B letters in ink; no team colour. Keylined colour chips on the plates and rows fit the hidden-bar decision; colour-name winner copy is a design call. With POL#16. [Record](docs/review/2026-11-10-project-review.md#post-reveal-results-pages-carry-no-team-colour-rev39). |
| `REV#40` | DEFERRED | LOW | M | Persistent damage on the figure as accumulating enemy-colour smears; health shows only on the HUD meter. Needs authored smear art, so deferred with `POL#16`; may not read in quarter panes and erodes team-colour identity. [Record](docs/review/2026-11-10-project-review.md#show-damage-on-the-figure-as-enemy-paint-rev40). |
| `REV#42` | DEFERRED | LOW | M | Crossfeed keeps Close Quarters' loop and sting; its own arrangement is deferred with `POL#16` (2026-11-13). Rotate never plays the two back to back; a Random first pick or Loser's pick can. [Audio](docs/audio-inventory.md#63-decisions-and-what-is-still-open). |
| `REV#44` | UNSCOPED | LOW | M | Series end is the normal Results page plus a winner line and the win ledger. A round-by-round strip, a match-point announcement and series-wide awards were never designed. [Record](docs/review/2026-11-10-project-review.md#series-conclusion-is-the-ordinary-results-page-rev44). |
| `REV#45` | UNSCOPED | LOW | M | More paint-flavoured awards ahead of splats would extend `ROAD#12`, but none of the tallies is collected today. Never designed. [Record](docs/review/2026-11-10-project-review.md#wider-awards-set-and-a-consolation-call-out-rev45). |

**New features from the comparison**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#48` | UNSCOPED | MED | M | Debug `replay.timelapse` done 2026-11-12. Left: the player-facing End row, not designed. It needs rulings on pacing, HUD, clock and sound, and on the row's place on the six-row End paper, then copy, validation, painter and a play check. [Owner](docs/feature-roadmap.md#whole-round-time-lapse-rev48). |
| `REV#49` | UNSCOPED | MED | M | Save the finished arena as a PNG from an explicit End or Results action. Capture and WIC writer exist only in Debug; needs a Release home, the reveal frame, a folder, copy and a check. Never designed. [Record](docs/review/2026-11-10-project-review.md#save-the-finished-picture-as-a-png-rev49). |
| `REV#52` | DEFERRED | LOW | L | Paint-keyed bounce pad as a paint object beside ROAD#28 and ROAD#29: static, launches the team holding its top paint. New class, format kind, art, cue and trial stage. Deferred behind `ROAD#28`'s bot routing. [Record](docs/review/2026-11-10-project-review.md#paint-keyed-bounce-pad-object-rev52). |
| `REV#53` | UNSCOPED | LOW | L | Eraser stage object that returns tiles to neutral (Splasher water, de Blob). Breaks the paint-only-arrives invariant that wet strips, paint objects, bot finishing, underfoot and awards assume. Never designed. [Record](docs/review/2026-11-10-project-review.md#eraser-stage-object-that-clears-paint-rev53). |
| `REV#56` | OPEN | LOW | S | Movement Course has no lesson station for the Sub action: add one JSON lesson with its checks and text fit. ROAD#11's review was waived 2026-11-13, so the subs stand as shipped. Practice targets are `ROAD#41`. [Record](docs/review/2026-11-10-project-review.md#sub-lesson-station-in-movement-course-rev56). |
| `REV#58` | UNSCOPED | LOW | L | Rotation for five to eight people (Nidhogg 2 tournaments): profile names, who sits out, per-person standings. Needs a second ledger beside the team-win Session and a per-person winner rule. Never designed. [Record](docs/review/2026-11-10-project-review.md#party-rotation-for-more-than-four-people-rev58). |
| `REV#59` | DEFERRED | LOW | L | Bots-only attract match behind the title. Level and LevelBuilder refuse a setup with no human view, so it needs a spectator view and a new stack shape. Deferred with `POL#16`, which owns the colourless front end. [Record](docs/review/2026-11-10-project-review.md#attract-match-behind-the-title-rev59). |
| `REV#60` | DEFERRED | LOW | M | Fill the spare quadrant of three-human matches with territory-free content (clock, rosters), the row the ROAD#17 decline invited. Must repeat what panes show; needs a draw region and 3P painter cover. With POL#16. [Record](docs/review/2026-11-10-project-review.md#territory-free-use-of-the-spare-3p-quadrant-rev60). |

**Tooling, editor and distribution**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#63` | OPEN | MED | M | Taken 2026-11-13 without a dump: unhandled-exception and terminate handlers write `last-error.log` with a crash phase, exception code, module (game, engine, system or other) and RVA for the shipped PDB; amends failure-reporting's Boundary. [Plan](docs/failure-reporting.md#crash-record-plan-rev63). |
| `REV#66` | UNSCOPED | LOW | L | Editor has no multi-select, recent files, drag-and-drop open or autosave. Group move is the one with authoring value; a textured canvas contradicts the schematic scope. Scope with the next editor slice. [Record](docs/review/2026-11-10-project-review.md#editor-lacks-multi-select-and-recent-files-rev66). |
| `REV#67` | DEFERRED | LOW | M | No signing or separate checksum (2026-11-13). An itch.io page with a butler publish step waits for the first non-pre-release tag or a wanted store page. [Calls](docs/feature-roadmap.md#project-review-calls-2026-11-13), [record](docs/review/2026-11-10-project-review.md#distribution-is-one-unsigned-github-zip-rev67). |
| `REV#68` | DEFERRED | LOW | L | No document scopes a Steam release (store page, Remote Play Together, prompts, Deck pass); a note beside `ROAD#23`. Deferred behind `REV#67`'s itch.io trigger. [Record](docs/review/2026-11-10-project-review.md#steam-release-is-not-scoped-rev68). |

## First-run review (2026-11-10)

**Owner:** [docs/review/2026-11-10-first-run-review.md](docs/review/2026-11-10-first-run-review.md),
a frozen record, so this file holds each row's status. It walked a newcomer from the GitHub page
to a first match, including a real build from a fresh clone; nothing was played on a pad. Its
[last section](docs/review/2026-11-10-first-run-review.md#already-tracked) lists findings that
already had rows. `FTR#13` was folded into `REV#26` on 2026-11-12, which closed 2026-11-13; its id is retired.
`FTR#1` closed 2026-11-13 with the `v0.1.0` release.

| id | status | sev | size | item |
|---|---|---|---|---|
| `FTR#14` | UNSCOPED | LOW | S | The setup path is long and the save folder is never shown; `stages` is now made on the first save write ([User stages](docs/feature-roadmap.md#user-stages-road16)). Quick Play and an Open save folder item stay undesigned. [Record](docs/review/2026-11-10-first-run-review.md#the-setup-path-is-long-and-the-save-folder-is-hidden-ftr14). |

## Areas with no open rows

A new row for one of these gets its section back.

| area | owner | a row returns with |
|---|---|---|
| Release preparation (`REL`) | [releasing.md, Release work items](docs/releasing.md#release-work-items-rel) | a new release-process gap; `REL#6`, the versioning rule, was decided 2026-11-13, and `ROAD#1`, `ROAD#2`, `POL#24` and `POL#25` were waived 2026-11-05 |
| Levels and the editor | [polish-plan.md](docs/polish-plan.md) §3, §8; [level-editor.md](docs/level-editor.md) | a new arena or editor slice; `POL#21`, `POL#27`, `POL#30`, `POL#31` and `ED#5` were settled 2026-11-05 |
| Audio replacement | [polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set) | a missing cue hook or a mix defect; `POL#23` was accepted and `ROAD#3` waived 2026-11-05 |
| Game gaps (gap audit Class 1) | [docs/review/gap-audit.md](docs/review/gap-audit.md) (frozen) | a new finding; `ROAD#5`, `ROAD#7`, `ROAD#9`, `ROAD#10`, `GAP#4.2`, `GAP#5.1`, `GAP#L9` and `GAP#L11` were accepted or waived 2026-11-05 (no translation language chosen) |
| Backstory build | [backstory.md, Building it](docs/backstory.md#building-it) | a missing or regressed story beat; pop, sigh and chair integration are complete |
| Design process (`DES#1`-`DES#4`) | [design-process.md](docs/design-process.md) | a new story, persona or journey, or a check gap a later trace finds; `DES#4`, the code re-review, was the last (2026-11-03) |
| Pin follow-up (`PIN`) | [engine-reports.md, PNG atlases](docs/review/engine-reports.md#png-atlases-and-texconv-pin3) | the next pin bump's preparation; `PIN#3` moved five atlases to PNG 2026-11-13 and `sprite_sheet_1` keeps its BC3 DDS |
| Closed engine reports (1–25, 28, 29, 31, 32) | [engine-reports.md, Closed](docs/review/engine-reports.md#closed) | a pin bump that answers a live report and leaves a workaround to delete |
| Feature-roadmap increments 1–5 | [feature-roadmap.md](docs/feature-roadmap.md) | a sixth increment, or a regression from the `ROAD#2` session |
| 2026-09-26 review fixes | `docs/review/2026-09-26-*.md` (frozen) | a new review round; the hardware residue is `ROAD#2` |
| Splatoon borrowings (no live score, random pairs) | gap audit 1.1, 2.1; [`ROAD#8`](docs/feature-roadmap.md#more-team-palette-pairs-road8) | only the author reversing the decision; `REV#14` (2026-11-13) took the two exempt pairs out of the roll and kept the random-pair model |
| Startup content validation | gap audit 8.1, `game/config/content_validation.cpp` | a content kind added without validation |
| Content pipeline | [Text scope](docs/menu-presentation.md#remaining-text-slices-gap84), [art/content policy](game/art/README.md#runtime-content-separation-gap86) | a new authored text role or accepted runtime-content requirement; the scoped `GAP#8.4` extraction is complete |
| Code TODO/FIXME markers | `game/`, `editor/`, `scripts/`, `cmake/` | a new marker, which should be a row here instead |
| OOP showcase (`OOP#1`) | [oop-guide.md](docs/oop-guide.md) | a new hierarchy, extension point or registration the guide no longer describes; never a request to flatten one. Sub weapons and [Add a sub weapon](docs/oop-guide.md#add-a-sub-weapon) were added 2026-11-12 |
| Feel and feedback (`POL`) | [polish-plan.md §8](docs/polish-plan.md#8-further-improvements-to-prototype); [Movement Course](docs/polish-movement-course.md) | a new feel or feedback defect; `POL#3`, `POL#18`, `POL#19`, `POL#38`, `POL#39` and `POL#45` were accepted or waived 2026-11-05, and `POL#49` (Reduce effects from Windows on a fresh save) was built 2026-11-13 |
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
`OOP` OOP guide, `DES` design process, `REV` the 2026-11-10 project review, `FTR` the
2026-11-10 first-run review, `REL` the release guide, `PIN` the ledger's next-pin-bump preparation,
`DOC`/`CI`/`TOOL` the named file.

**`status`**, a closed list:
- `OPEN`: ready to work.
- `DECISION`: the next step needs the author: a call, files only they can supply, or their
  hardware (a pad, a clean PC). Always named in NOW.
- `BLOCKED`: waits on another row, named in the clause.
- `DEFERRED`: paused by priority; its owner's condition applies.
- `UNSCOPED`: noticed, never examined. For the 2026-11-10 review rows it means examined there
  but never designed.

**`sev`**, the cost of leaving it open, not difficulty:
- `CRIT`: wrong or undefined behaviour today, or blocking the head of the queue.
- `HIGH`: a visible defect, or a missing piece the current aim depends on.
- `MED`: a real gap, but bounded or latent.
- `LOW`: housekeeping, doc debt, convenience.

**`size`:** engineer-hours: `S` ≤2 h · `M` 2–6 h · `L` 6–20 h · `XL` >20 h.

**Out of scope for rows:** findings that hold the game to the engine's T11 ("flatten the
hierarchy", "make it a variant") are misfiled (AGENTS.md), and a missing-bank silent fallback is correct.
