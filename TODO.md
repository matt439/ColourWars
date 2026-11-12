# TODO: open work

An index of open work, one row per item; each row points to the doc that owns the detail. The
rules for editing it are at the end, in [How to read this file](#how-to-read-this-file). A fresh
thread starts here: read **NOW**, then the owner of the row it picks up.

## NOW

**The only ordering in this file.** Keep this block to the current aim and the next few actions;
results go in their owners, and `git log` is the changelog.

**Aim: release preparation (the author, 2026-11-11), docs and packaging first.**
The [release guide](docs/releasing.md) owns this pass and the candidate checks.

Next actions that need no author call:

1. Candidate evidence for current `master`: `REL#1`, then `REL#2` (sandbox), now one command
   (`scripts/release-candidate.ps1`).
2. `PIN#2`: qualify engine master's behaviour changes on the trial build `PIN#1` rehearsed; the
   real pin stays.
3. `REV#7`: measure static-paint draw cost on a quiet machine.

The other `OPEN` rows can follow in any order. A push, pull request and publication need their
own instructions; the required `gate` and up-to-date `master` rule still apply.

**Author calls** (every `DECISION` row; the first group gates the release aim):

- Release: `FTR#1` first tag, `REL#6` versioning rule, `REV#12` hero image, `REV#26` stage order
  and fresh-save default (was `FTR#13`), `ROAD#11` play review (unblocks `REV#3`, `REV#21`,
  `REV#36`, `REV#56`), `ENG#36` pin bump before or after `v0.1.0`.
- Halves left by split rows: `REV#5` in-game notice, `REV#10` live paint-underfoot cue, `REV#63`
  crash dump; also `REV#13` (hide the panel from help or compile it out), `CI#1` trial push.
- Gameplay and presentation: `REV#1`, `REV#4`, `REV#14`, `REV#23`, `REV#25`, `REV#28`, `REV#38`,
  `REV#41`, `REV#42`, `REV#43`, `REV#46`, `REV#47`, `REV#50`, `REV#51`, `REV#54`, `REV#55`,
  `REV#57`, `REV#29`, `ROAD#13`, `ROAD#29`, `ROAD#40`, `POL#49`.
- Tools and distribution: `TOOL#5`, `REV#64`, `REV#67`, `REV#69`.
- Engine filing: `ENG#35`, `ENG#37`, `ENG#38`, `ENG#PARKED`.

The hardware, participant and listening checks waived on 2026-11-05 were not performed. Arena
art (`POL#16`) stays deferred; bot lift routing and non-XInput pads are deferred, and sponges
have an `OPEN` draft step.

## Release preparation

**Owner:** [docs/releasing.md, Release work items](docs/releasing.md#release-work-items-rel). The
release checks `ROAD#1`, `ROAD#2`, `POL#24` and `POL#25` were waived 2026-11-05, not performed.

| id | status | sev | size | item |
|---|---|---|---|---|
| `REL#1` | OPEN | MED | M | No candidate evidence exists for current master: build, package-check and start a clean candidate (audible and missing-bank), and record commit, pin, version, ZIP SHA256 and results. [Item](docs/releasing.md#release-candidate-evidence-rel1). |
| `REL#2` | OPEN | LOW | S | Windows Sandbox runs of the ZIP stop at engine v0.2.0; run `sandbox-release.ps1 -Run` on the `REL#1` candidate and record it under `ROAD#37` for Labrador's PI-14. [Item](docs/releasing.md#sandbox-run-of-the-candidate-rel2). |
| `REL#6` | DECISION | LOW | S | No versioning rule: when the version moves after a tag, what minor and patch mean, and whether 0.x releases are marked pre-release. [Item](docs/releasing.md#versioning-rule-rel6). |

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6. It records the 2026-10-09
switch to Paint.exe Maximalism and the accepted 2026-10-15 round-2 art rules. The pixel-kit
questions in the polish sub-docs are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#16` | DEFERRED | MED | XL | Extend the approved treatment through the accepted arenas (selected 2026-11-05); remaining UI art is a later polish pass. Waits on the author setting it as the next aim. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |
| `POL#35` | DEFERRED | LOW | S | Signage and captioned Paint-artefact jokes need the author's wording; with `POL#16`. [Terrain](docs/polish-terrain.md#stage-approval-and-remaining-environment-work-2026-10-20). |

## Feel and feedback

**Owner:** [docs/polish-plan.md §8](docs/polish-plan.md#8-further-improvements-to-prototype);
[Movement Course](docs/polish-movement-course.md). `POL#3`, `POL#18`, `POL#19`, `POL#38`,
`POL#39` and `POL#45` were accepted or waived 2026-11-05.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#49` | DECISION | LOW | S | Should a fresh save start with Reduce effects On when Windows' Animation effects is off? Today `reduce_effects` is fixed false; a first-run read would also be the client exhibit for engine `REV#60`. [Item](docs/polish-plan.md#reduce-effects-default-from-windows-pol49). |

## Comparable-game gaps

**Owner:** [docs/feature-roadmap.md, Comparable-game gaps](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27)
and [Backlog audit additions](docs/feature-roadmap.md#backlog-audit-additions-2026-11-12) (2026-11-12).
Features that Splatoon and couch party shooters have and ColourWars lacks, rated medium value
or above (2026-10-27). The owner records the gaps left out by choice and the excluded modes.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#11` | DECISION | MED | S | Sub weapons: all seven slices are done (2026-11-09), and the home-band gate is green again (2026-11-10). What remains is the author's attended play review. [Plan](docs/feature-roadmap.md#sub-weapons-road11). |
| `ROAD#13` | DECISION | LOW | M | More tool archetypes: [umbrella proposal](docs/feature-roadmap.md#umbrella-tool-proposal-road13) recorded. Author's call on Q1-Q7: umbrella vs brush (or splatling if REV#57 is taken), unblock REV#3 as slice 1, canopy input, slowdown, shape, name, umbrella with wall sub. Nothing is built before. |
| `ROAD#14` | UNSCOPED | LOW | L | Map pickups or power-ups (TowerFall chests, Smash items), behind a match toggle and with bot awareness. No longer blocked by `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#28` | DEFERRED | LOW | L | Cycling and paint-driven lifts are done. Left: deliberate bot lift routing. Deferred behind `REV#27`, because lifts stand only on the two trials that are out of Random, so routing has no consumer yet. [Boundary](docs/feature-roadmap.md#unscoped-paint-object-contracts-road28-road29). |
| `ROAD#29` | DECISION | MED | M | Growing sponges: [contract draft](docs/feature-roadmap.md#growing-sponge-contract-draft-road29) recorded (E1-E12 against 220d4809). Author's call on G1-G7: what grows it, two poses or proportional, what growth scores (rest area / grown / shown tiles), direction, occupancy, trial, bot routing. |
| `ROAD#23` | DEFERRED | MED | L | Non-XInput pads (PlayStation, Switch Pro). The engine ask is parked in `ENG#PARKED` and there is no client exhibit; resumes when such a pad is in hand or a store release is chosen (`REV#68`). [Input](docs/feature-roadmap.md#input). |
| `ROAD#24` | DEFERRED | LOW | M | Pad aim assist against the one mouse seat. Deferred until attended play shows pad seats consistently losing to the mouse seat (story B8); relates to `POL#3`, `POL#18` and `ROAD#5`. [Input](docs/feature-roadmap.md#input). |
| `ROAD#40` | DECISION | LOW | M | Should series rounds swap team sides (never, every round, or only when the stage repeats)? Today a team keeps its half for the whole series. If taken, it is a Mode-row value. [Item](docs/feature-roadmap.md#series-side-swap-road40). |
| `ROAD#41` | UNSCOPED | LOW | M | Grey, non-scoring practice targets in the workshop that pop and redraw with no damage readout; needs a scope section first. Related to `REV#55` and `REV#56`. [Item](docs/feature-roadmap.md#practice-targets-in-the-workshop-road41). |
| `ROAD#42` | UNSCOPED | LOW | M | Weapon Select could show each tool's shot shape, drawn from its tuning, beside the text (story A6). No problem has been observed; it may wait on `POL#16`. [Item](docs/feature-roadmap.md#shot-shapes-on-weapon-select-road42). |
| `ROAD#43` | UNSCOPED | LOW | L | Flood-fill paint object that fills one authored region for the team holding its trigger. Needs a contract draft on the lift precedent before an author call on the territory swing. [Item](docs/feature-roadmap.md#flood-fill-paint-object-road43). |
| `ROAD#44` | UNSCOPED | LOW | M | Circle collision shapes for the ball and grenade, once a pin bump brings engine master's circle narrow phase (`f6752ca7`). Measure the contact changes, then the author decides on hit feel. [Item](docs/feature-roadmap.md#circle-shapes-for-round-projectiles-road44). |

## Engine demonstration

**Owner:** [docs/review/gap-audit.md, Class 2](docs/review/gap-audit.md#class-2--gaps-in-the-role-as-the-engines-demonstration),
frozen as above, with live [menu and demonstration scopes](docs/feature-roadmap.md#per-player-menu-focus-gapd1).
These add to the hierarchies, never flatten them (AGENTS.md); D6 is `GAP#4.2`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#D2` | DEFERRED | MED | M | Additional widget/page bases need a concrete caller beyond OptionRow; scope only when an accepted page requires one. [Boundary](docs/feature-roadmap.md#other-demonstration-gaps). |

## Engine reports

**Owner:** [docs/review/engine-reports.md](docs/review/engine-reports.md), the live ledger. Check
engine master before filing, not the pin. The pin is master `e7fc50e` (`v0.3.0` plus vsync);
engine master was `3fe88f64`, 95 commits on, at the 2026-11-12 recheck. On 2026-11-05 the owed
reports were filed and 28 was closed as declined. A pin bump that answers one is when its
workaround is deleted.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#PARKED` | DECISION | LOW | S | Asks waiting for a vehicle: `window_size_changed`, desktop size (exhibit for engine `REV#83`), atomic JSON write, registry forgetting, install facts, listener/voice cap, display-awake owner, non-XInput reader (`ROAD#23`), colour transforms (was `GAP#F10`), engine `GAP#6`/`REV#60`. Author: post mouse-button names on labrador#122? [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `ENG#26`, `ENG#27`, `ENG#30`, `ENG#33`, `ENG#34` | DEFERRED | LOW | S | Filed 2026-11-05 as [labrador#122](https://github.com/matt439/Labrador/issues/122) (27 with the 26 rider), #123, #124 and #125 (the Bug). Waits on the engine. When one is answered, spend it at the next pin bump; for 34, also settle what CI's strict gates accept. [Ledger](docs/review/engine-reports.md#live) |
| `ENG#35` | DECISION | LOW | S | Drafted 2026-11-05 from the hygiene review: `AudioResources` has no non-throwing sound-group lookup, so `sound_groups.cpp` catches `out_of_range`. Checked at master `5cbc6b3`. The author decides whether to file. [Report](docs/review/engine-reports.md#35-audioresources-has-no-non-throwing-sound-group-lookup---draft-with-find-or-add-exhibit) |
| `ENG#36` | DECISION | LOW | M | Master `8362e2d` parses JSON iteratively; at the next pin bump (rehearsed by `PIN#1`) drop `save_max_nesting` from `reader_mutations`. Whether the bump lands before or after `v0.1.0` is the author's call. Never filed. [Bump](docs/review/engine-reports.md#preparing-the-next-pin-bump-2026-11-12) |
| `ENG#37` | DECISION | LOW | S | Long golden-image filenames break ordinary deep Windows clones. Drafted from `FTR#4`, reproduced against current master; the author decides whether to file. [Report](docs/review/engine-reports.md#37-long-render-golden-filenames-break-ordinary-windows-clones---draft-with-checkout-exhibit). |
| `ENG#38` | DECISION | LOW | S | Window presence reaches the game as one bool (activation edge, `StateContext::active()`), so Debug `debug_background.cpp` subclasses the HWND to tell minimised, hidden and power-suspended apart. Drafted against master `3fe88f64`; the author decides whether to file. [Report](docs/review/engine-reports.md#38-presence-reaches-the-game-as-one-bool---draft-with-window-subclass-exhibit) |

**Next pin bump (`PIN`)**

| id | status | sev | size | item |
|---|---|---|---|---|
| `PIN#2` | OPEN | LOW | M | Qualify master's behaviour changes on a trial build of engine master (`PIN#1` built and started it, 2026-11-12): pads read disconnected while inactive (Join page, `DBG#6`), nothing drawn while minimised, and the new throws. [Item](docs/review/engine-reports.md#qualify-masters-behaviour-changes-pin2). |
| `PIN#3` | DEFERRED | LOW | M | Once the pin has the engine's PNG loader, ship the PNG atlases and retire the texconv premultiply step, keeping the DDS for any atlas that does not match byte for byte. [Item](docs/review/engine-reports.md#png-atlases-and-texconv-pin3). |

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
| `TOOL#5` | DECISION | LOW | M | Diagnostics and the aim/reaction ladder are measured. Exact paid-shot/sub-use counts need a passive game-code observer, excluded by the original scope; the author decides whether to allow it. [Boundary](docs/gameplay-tuning.md#bot-diagnostics-tool5). |
| `CI#7` | DEFERRED | LOW | S | Tallied: paint-fx passed 7 of 7 scheduled nights (about 1 min of check per preset), and the 11-09 red night caught real regressions. After 2026-11-17's run, run check-paint-fx in ci.yml's checks legs; the rest stay nightly. [Hygiene](docs/code-hygiene.md#promote-stable-nightly-checks-ci7). |
| `CI#1` | DECISION | LOW | M | No reliable foreground runner model yet, so `check-debug-console` stays out of CI. Next step is a throwaway `workflow_dispatch` trial on a hosted runner, which needs the author's OK to push. [Constraint](docs/debug-console-record.md#foreground-checks-ci1), [workflow](.github/workflows/ci.yml). |

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
| `REV#1` | DECISION | MED | M | A player wholly outside the bounds throws `PlayerOutOfBounds` and exits the game; user stages (`ROAD#16`) are never checked for enclosure. Author picks: out-of-bounds becomes an elimination (pits become authorable) or the scan refuses unsealed stages. [Record](docs/review/2026-11-10-project-review.md#a-player-leaving-the-arena-ends-the-process-rev1). |
| `REV#3` | BLOCKED | LOW | M | Planted sprinklers and walls take hits by a shot's travel bounding box, tested after the tick's contacts: diagonal shots hit up to about 17 units wide, and a player inside the wall is hit before it. Waits on the `ROAD#11` play review. [Record](docs/review/2026-11-10-project-review.md#breakable-subs-are-struck-by-travel-bounding-box-rev3). |
| `REV#4` | DECISION | LOW | M | An exception in the live round or instant replay step exits the game; recovery was scoped out with `GAP#7.3` and the replay has no known Release cause. Author's call whether to contain it at `GameLevel`, after `REV#1` removes the reachable throw. [Record](docs/review/2026-11-10-project-review.md#no-containment-around-the-match-or-replay-step-rev4). |
| `REV#5` | DECISION | LOW | S | Save trouble reaches only stderr; a newer save is now copied aside before a downgrade write ([Save layout](docs/feature-roadmap.md#stage-records-and-the-sectioned-save-gap62)). Author's call: whether a one-line Title/Home notice from `presentation.json` shows the load notices (moves menu painter fixtures). [Record](docs/review/2026-11-10-project-review.md#save-recovery-notices-reach-only-stderr-rev5). |
| `REV#7` | OPEN | LOW | M | Static paint submits up to three sprites per owned tile per pane; draw cost was never measured. Next: run the `REV#38` four-bot Crossfeed script with `perf.capture` at round start and at 95% paint and compare. Merging runs would move painter pins. [Record](docs/review/2026-11-10-project-review.md#static-paint-draws-one-tile-at-a-time-rev7). |

**Release, README and repository**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#10` | DECISION | MED | S | Lesson half done 2026-11-12: the Movement Course refill lesson now says own paint refills faster and enemy paint stops it, pinned by `practice_workshop`. Left: the author's call on a live enemy-paint cue (paint meter or step sound), or no cue. [Scope](docs/releasing.md#current-preparation-scope), [record](docs/review/2026-11-10-project-review.md#paint-underfoot-has-no-player-cue-rev10). |
| `REV#12` | DECISION | MED | S | The author still picks the README hero image and clip from the retained captures, given `POL#16`'s arena art is unfinished; `REV#48`'s time-lapse could supply candidates. [Record](docs/review/2026-11-10-project-review.md#readme-has-no-screenshot-or-clip-rev12). |
| `REV#13` | DECISION | LOW | S | The F1/Back developer panel draws in Release and is listed in Controls help, but `frame_time_ms()` returns 0 there (`debug_director.h:212`), so it reads `render fps: n/a`. Author's call: hide it from help or compile it out of Release. [Record](docs/review/2026-11-10-project-review.md#developer-text-panel-ships-in-release-rev13). |
| `REV#14` | DECISION | MED | S | Default Random palette still rolls pink_green and blue_yellow, the two pairs exempt from the colour-vision bar (one roll in three). A per-pair in-random flag would fix it but reopens ROAD#8's choice and changes seeded draws. [Record](docs/review/2026-11-10-project-review.md#random-palette-draws-the-exempt-pairs-rev14). |
| `REV#16` | DEFERRED | LOW | S | Actions are pinned by major tag with no Dependabot, and the nightly's legs each rebuild. Each Dependabot pull request would need the owner to merge it and run the full gate; per-leg builds are a recorded choice. [Record](docs/review/2026-11-10-project-review.md#ci-action-pins-and-nightly-rebuilds-rev16). |
| `REV#18` | UNSCOPED | LOW | M | Level-editor manual/record split done 2026-11-12 ([record](docs/level-editor-record.md)). Left: moving closed rows out of feature-roadmap.md and code-hygiene.md (L, cosmetic). [Record](docs/review/2026-11-10-project-review.md#large-docs-mix-manual-and-records-rev18). |

**Bots, stages and records**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#19` | UNSCOPED | LOW | M | Bots chase any sighted enemy and ignore the clock, health and score. The sight ladder (2026-11-12) found that shorter sight cuts fights but does not raise paint, so there is no paint case for the four behaviour slices; they stay undesigned. [Record](docs/gameplay-tuning.md#sight-distance-ladder-rev19-2026-11-12) |
| `REV#20` | UNSCOPED | MED | L | One bot policy plays all five tools. Aim/reaction sensitivity is measured in `TOOL#5`; exact event counters need a decision, and per-tool roles remain unscoped. Sniper's low paint is intended. [Record](docs/review/2026-11-10-project-review.md#one-bot-policy-plays-all-five-tools-rev20). |
| `REV#21` | BLOCKED | LOW | M | Bots never target an enemy sprinkler and fire into an enemy paint wall, since BotWorld has no planted-sub query and bot sight ignores the wall. Bounded to 4-8 s per use. Waits on ROAD#11's attended play review. [Record](docs/review/2026-11-10-project-review.md#bots-do-not-answer-enemy-sprinklers-or-walls-rev21). |
| `REV#23` | DECISION | LOW | S | The bot's finishing window on a support is sized at the handicap refill rate, not the 1.25x own-paint rate that POL#30 ships on; the code calls it a sufficient minimum and ROAD#39's note left it undecided. Author decides: keep the safe bound or tighten. [Record](docs/review/2026-11-10-project-review.md#bot-finish-window-sized-without-own-paint-refill-rev23). |
| `REV#25` | DECISION | MED | M | Stage records are keyed by stage only, so solo has nothing graded to beat. Two shapes: per-stage Easy/Normal/Hard win marks or coverage medals at a fixed duration. Author's call: it sits beside no-stats, no-variant-leaderboard (ROAD#22) and solo-is-practice. [Record](docs/review/2026-11-10-project-review.md#solo-play-has-no-graded-stage-goal-rev25). |
| `REV#26` | DECISION | MED | S | Stage Select lists 13 stages in manifest order, opens a fresh save on King of the Hill with Random last, and marks no accepted arena apart from the Yard trials. Author picks: reorder and rename, an In Random mark, or a sectioned page (M). `FTR#13` folded in 2026-11-12. [Record](docs/review/2026-11-10-project-review.md#stage-select-does-not-mark-arenas-from-trials-rev26). |
| `REV#27` | DEFERRED | MED | XL | The gate, ferry and lifts appear only on single-purpose Yard trial blockouts; Random is four stages. A finished gate or ferry arena in Random is a candidate aim, the author's to set; deferred with `POL#16`. Bot lift routing is `ROAD#28`. [Record](docs/review/2026-11-10-project-review.md#no-random-arena-uses-the-gate-ferry-or-lifts-rev27). |

**Player-facing UX**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#28` | DECISION | MED | L | A missing pad with no spare leaves only Quit, which discards a series. Author's call: may a bot take a missing seat (from the pause, or between series rounds)? That relaxes the `ROAD#20` roster lock and needs tape and record rules. [Record](docs/review/2026-11-10-project-review.md#bot-takeover-for-a-missing-seat-rev28). |
| `REV#29` | DECISION | MED | M | Split-pane HUD text measured ([HUD](docs/polish-hud.md#split-pane-text-size-rev29-2026-11-12)): meter labels are 1.1% of window height (12 px at 1080p, 8 px at 720p), identity 1.6%, clock 2.1%, against a proposed 1.7% floor (16 arcmin, 55-inch at 2.5 m). Author picks: this floor, another, an Options size row, or none. A floor rebaselines painter HUD fixtures. |
| `REV#30` | DEFERRED | MED | M | Narrowed 2026-11-12 to a Practice row on Home for Movement Course (README already points newcomers there; stage order is `REV#26`). Home is an authored six-aspect layout; waits on `REV#26`. [Record](docs/review/2026-11-10-project-review.md#direct-entry-to-the-practice-course-rev30). |
| `REV#32` | UNSCOPED | LOW | M | Home has no play-again shortcut. GAP#6.1 already pre-fills every setup page and Join cannot be skipped, so it saves about four confirm presses for a conditional Home row and a second route into Join and Stage. [Record](docs/review/2026-11-10-project-review.md#play-again-with-last-setup-rev32). |
| `REV#36` | BLOCKED | LOW | M | Respawn grace is already a recorded POL#17 fallback, to add only on observed spawn pressure; the sprinkler is a reason to look for it. Blocked on ROAD#11's attended play review. Choosing among the team's spawns is new and undesigned. [Record](docs/review/2026-11-10-project-review.md#respawn-grace-and-spawn-choice-rev36). |

**Presentation and feedback**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#37` | UNSCOPED | LOW | M | Hit feedback is keyed to the victim. A 2026-11-12 tally found 1.7% of hit ticks land outside the shooter's pane (Sniper 21%) and none tiny. A Sniper-range shooter cue is undesigned; hit_confirm stays declined (POL#23). [Record](docs/polish-combat-feedback.md#shooter-side-confirmation-tally-rev37-2026-11-12) |
| `REV#38` | DECISION | MED | M | Heavily painted 4P reveals captured (2026-11-10): strips measure 2-3 px at 720p and 4 px at 1080p on Crossfeed, 1-2 and 2-3 px on Close Quarters. They show whose paint is where, not who won; the cards carry that. Author's call: a reveal-only minimum weight or body tint, or leave as is. [Capture](docs/polish-match-accents.md#paint-strip-weight-in-the-reveal-rev38), [record](docs/review/2026-11-10-project-review.md#paint-strips-are-hairline-thin-in-the-reveal-rev38). |
| `REV#39` | DEFERRED | MED | S | Post-reveal Results and series pages show only A/B letters in ink; no team colour. Keylined colour chips on the plates and rows fit the hidden-bar decision; colour-name winner copy is a design call. With POL#16. [Record](docs/review/2026-11-10-project-review.md#post-reveal-results-pages-carry-no-team-colour-rev39). |
| `REV#40` | DEFERRED | LOW | M | Persistent damage on the figure as accumulating enemy-colour smears; health shows only on the HUD meter. Needs authored smear art, so deferred with `POL#16`; may not read in quarter panes and erodes team-colour identity. [Record](docs/review/2026-11-10-project-review.md#show-damage-on-the-figure-as-enemy-paint-rev40). |
| `REV#41` | DECISION | LOW | S | Time-up card at the pull-back: the round ends on sound and the camera move, with no text; the closing clock already announces it. A banner over the scripted reveal beat is a fiction call for the author, likely declined. [Record](docs/review/2026-11-10-project-review.md#no-on-screen-time-up-signal-rev41). |
| `REV#42` | DECISION | LOW | M | Crossfeed and Close Quarters share one music loop and sting, the only collision in Random; Press Tower's reuse is deliberate. Decide whether Crossfeed gets its own arrangement, given POL#23 accepted and listening waived. [Record](docs/review/2026-11-10-project-review.md#crossfeed-shares-the-close-quarters-music-rev42). |
| `REV#43` | DECISION | LOW | S | Player-owned cues play centred at authored pitch for every seat; the wrapper already takes pan and pitch. Decide whether per-seat offsets are wanted, since the mix is accepted (POL#23) and can only be judged by ear. [Record](docs/review/2026-11-10-project-review.md#per-seat-pan-and-pitch-for-player-cues-rev43). |
| `REV#44` | UNSCOPED | LOW | M | Series end is the normal Results page plus a winner line and the win ledger. A round-by-round strip, a match-point announcement and series-wide awards were never designed. [Record](docs/review/2026-11-10-project-review.md#series-conclusion-is-the-ordinary-results-page-rev44). |
| `REV#45` | UNSCOPED | LOW | M | More paint-flavoured awards ahead of splats would extend `ROAD#12`, but none of the tallies is collected today. Never designed. [Record](docs/review/2026-11-10-project-review.md#wider-awards-set-and-a-consolation-call-out-rev45). |
| `REV#46` | DECISION | LOW | M | Roster strip by the clock with respawning players crossed out; the HUD has no roster element and `ROAD#18` keeps enemy state off it. Author rules first: none, or a teammates-only strip. [Record](docs/review/2026-11-10-project-review.md#team-status-strip-showing-who-is-respawning-rev46). |
| `REV#47` | DECISION | LOW | M | Respawn shows neither attacker nor tool. It would need a last-attacker record and a HUD element over a wait designed to be wordless, close to the kill attribution declined under `POL#30`; asked once, likely declined. [Record](docs/review/2026-11-10-project-review.md#tell-a-splatted-player-who-got-them-rev47). |

**New features from the comparison**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#48` | UNSCOPED | MED | M | Debug `replay.timelapse` done 2026-11-12. Left: the player-facing End row, not designed. It needs rulings on pacing, HUD, clock and sound, and on the row's place on the six-row End paper, then copy, validation, painter and a play check. [Owner](docs/feature-roadmap.md#whole-round-time-lapse-rev48). |
| `REV#49` | UNSCOPED | MED | M | Save the finished arena as a PNG from an explicit End or Results action. Capture and WIC writer exist only in Debug; needs a Release home, the reveal frame, a folder, copy and a check. Never designed. [Record](docs/review/2026-11-10-project-review.md#save-the-finished-picture-as-a-png-rev49). |
| `REV#50` | DECISION | LOW | M | Author's call: should an elimination leave a small burst of the attacker's paint (Splatoon)? Burst path and attacker credit exist, but it makes kills score territory, beside POL#30 and the balance accepted 2026-11-05. [Record](docs/review/2026-11-10-project-review.md#elimination-leaves-a-burst-of-attacker-paint-rev50). |
| `REV#51` | DECISION | LOW | XL | Author's call: trial own-colour wall slide and wall jump (Splatoon, Splasher)? Never ruled on under POL#30. Reopens accepted arenas, reach numbers, bot routing and editor reach aids; default-off trial at most. [Record](docs/review/2026-11-10-project-review.md#own-colour-wall-climb-or-wall-jump-rev51). |
| `REV#52` | DEFERRED | LOW | L | Paint-keyed bounce pad as a paint object beside ROAD#28 and ROAD#29: static, launches the team holding its top paint. New class, format kind, art, cue and trial stage. Deferred behind `ROAD#28`'s bot routing. [Record](docs/review/2026-11-10-project-review.md#paint-keyed-bounce-pad-object-rev52). |
| `REV#53` | UNSCOPED | LOW | L | Eraser stage object that returns tiles to neutral (Splasher water, de Blob). Breaks the paint-only-arrives invariant that wet strips, paint objects, bot finishing, underfoot and awards assume. Never designed. [Record](docs/review/2026-11-10-project-review.md#eraser-stage-object-that-clears-paint-rev53). |
| `REV#54` | DECISION | LOW | M | Coverage-over-time strip on Results after the reveal (sample both totals each second, replay-neutral). It sits against `POL#15`'s quick Results, so the author rules on that first. [Record](docs/review/2026-11-10-project-review.md#coverage-over-time-strip-on-results-rev54). |
| `REV#55` | DECISION | LOW | M | Author's call: untimed, unrecorded exploration of any arena for 1-4 players (Splatoon Recon)? A no-winner session is outside the 2026-11-03 options ruling; cheaper variant is solo practice on any stage. [Record](docs/review/2026-11-10-project-review.md#untimed-group-exploration-of-any-arena-rev55). |
| `REV#56` | BLOCKED | LOW | S | Movement Course has no lesson station for the Sub action: add one JSON lesson with its checks and text fit. Waits on ROAD#11's play review so it is written once. Practice targets are `ROAD#41`. [Record](docs/review/2026-11-10-project-review.md#sub-lesson-station-in-movement-course-rev56). |
| `REV#57` | DECISION | LOW | M | Author's call: give the sniper a hold-to-charge with a line opponents can see (Splatoon chargers)? Reopens a tool and bot ballistics accepted 2026-11-05, with no observed problem; if taken, rides with ROAD#13. [Record](docs/review/2026-11-10-project-review.md#sniper-charge-or-readable-warning-rev57). |
| `REV#58` | UNSCOPED | LOW | L | Rotation for five to eight people (Nidhogg 2 tournaments): profile names, who sits out, per-person standings. Needs a second ledger beside the team-win Session and a per-person winner rule. Never designed. [Record](docs/review/2026-11-10-project-review.md#party-rotation-for-more-than-four-people-rev58). |
| `REV#59` | DEFERRED | LOW | L | Bots-only attract match behind the title. Level and LevelBuilder refuse a setup with no human view, so it needs a spectator view and a new stack shape. Deferred with `POL#16`, which owns the colourless front end. [Record](docs/review/2026-11-10-project-review.md#attract-match-behind-the-title-rev59). |
| `REV#60` | DEFERRED | LOW | M | Fill the spare quadrant of three-human matches with territory-free content (clock, rosters), the row the ROAD#17 decline invited. Must repeat what panes show; needs a draw region and 3P painter cover. With POL#16. [Record](docs/review/2026-11-10-project-review.md#territory-free-use-of-the-spare-3p-quadrant-rev60). |

**Tooling, editor and distribution**

| id | status | sev | size | item |
|---|---|---|---|---|
| `REV#63` | DECISION | MED | M | Access violations and terminate leave no record. Slice: a filter and terminate handler writing one local dump. Author's call: a dump holds memory the privacy rules exclude. The Release PDB half landed as `REL#7` ([releasing](docs/releasing.md#release-symbols-rel7)); only the dump remains. [Record](docs/review/2026-11-10-project-review.md#no-crash-dump-and-no-release-symbols-rev63). |
| `REV#64` | DECISION | MED | L | The editor is left out of the ZIP and compiles absolute checkout paths, so shipping it means relocatable content lookup, hidden source flows, a guide and package checks. Author's call: it redrafts design-process P5, J4, F6 and F7. [Record](docs/review/2026-11-10-project-review.md#level-editor-is-not-shipped-to-players-rev64). |
| `REV#65` | BLOCKED | LOW | M | No Play Stage or Open My Stages folder item in the editor. Waits on REV#64: it pays only if the editor ships, and a Release launch argument cuts against the rule that Release takes none. [Record](docs/review/2026-11-10-project-review.md#editor-has-no-test-play-command-rev65). |
| `REV#66` | UNSCOPED | LOW | L | Editor has no multi-select, recent files, drag-and-drop open or autosave. Group move is the one with authoring value; a textured canvas contradicts the schematic scope. Scope with the next editor slice. [Record](docs/review/2026-11-10-project-review.md#editor-lacks-multi-select-and-recent-files-rev66). |
| `REV#67` | DECISION | LOW | M | The only channel is an unsigned GitHub ZIP, itself unpublished. An itch.io page with a butler step in publish is the worthwhile addition; signing and a checksum add little. Author's call, after the first tag. [Record](docs/review/2026-11-10-project-review.md#distribution-is-one-unsigned-github-zip-rev67). |
| `REV#68` | DEFERRED | LOW | L | No document scopes a Steam release (store page, Remote Play Together, prompts, Deck pass); a note beside `ROAD#23`. Deferred until after `FTR#1` and `REV#67` choose a channel. [Record](docs/review/2026-11-10-project-review.md#steam-release-is-not-scoped-rev68). |
| `REV#69` | DECISION | LOW | S | Nothing tells players that Parsec-style streaming can stand in for online play. One hedged HOW-TO-PLAY paragraph, but qualifying it needs two machines, a remote friend and pads: the author's hardware and call. [Record](docs/review/2026-11-10-project-review.md#stream-based-online-play-is-undocumented-rev69). |

## First-run review (2026-11-10)

**Owner:** [docs/review/2026-11-10-first-run-review.md](docs/review/2026-11-10-first-run-review.md),
a frozen record, so this file holds each row's status. It walked a newcomer from the GitHub page
to a first match, including a real build from a fresh clone; nothing was played on a pad. Its
[last section](docs/review/2026-11-10-first-run-review.md#already-tracked) lists findings that
already had rows. `FTR#13` was folded into `REV#26` on 2026-11-12; its id is retired.

| id | status | sev | size | item |
|---|---|---|---|---|
| `FTR#1` | DECISION | HIGH | S | No release or tag, so README's "no Git, Visual Studio or terminal" promise leads to an expiring CI artifact or a source build. The publish job and 0.1.0 are ready: tag `v0.1.0`, then make step 1 one link. The first tag is the author's. [Record](docs/review/2026-11-10-first-run-review.md#no-release-exists-ftr1). |
| `FTR#14` | UNSCOPED | LOW | S | The setup path is long and the save folder is never shown; `stages` is now made on the first save write ([User stages](docs/feature-roadmap.md#user-stages-road16)). Quick Play and an Open save folder item stay undesigned. [Record](docs/review/2026-11-10-first-run-review.md#the-setup-path-is-long-and-the-save-folder-is-hidden-ftr14). |

## Areas with no open rows

A new row for one of these gets its section back.

| area | owner | a row returns with |
|---|---|---|
| Levels and the editor | [polish-plan.md](docs/polish-plan.md) §3, §8; [level-editor.md](docs/level-editor.md) | a new arena or editor slice; `POL#21`, `POL#27`, `POL#30`, `POL#31` and `ED#5` were settled 2026-11-05 |
| Audio replacement | [polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set) | a missing cue hook or a mix defect; `POL#23` was accepted and `ROAD#3` waived 2026-11-05 |
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
| OOP showcase (`OOP#1`) | [oop-guide.md](docs/oop-guide.md) | a new hierarchy, extension point or registration the guide no longer describes; never a request to flatten one. Sub weapons and [Add a sub weapon](docs/oop-guide.md#add-a-sub-weapon) were added 2026-11-12 |
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
