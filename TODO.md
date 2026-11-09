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
[roadmap](docs/feature-roadmap.md#endless-paint-road22). `ROAD#11` (sub weapons) is the work in hand: the
paint grenade, bots throwing it, the sprinkler, the paint wall, picking a sub on Weapon Select and
rebinding Sub on the Controls pages, and bots using all three subs with a balance pass, are
complete (2026-11-09); final art and sound cues are next.
Art (`POL#16`) is unblocked but stays deferred. Bot lift routing, sponges and the controller
rows remain unscoped.

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
| `ROAD#11` | OPEN | MED | XL | Sub weapons: grenade, sprinkler and paint wall as a loadout pick beside the tool, costing half the paint tank (all decided 2026-11-08), in seven slices. Slices 1 to 6 are done (2026-11-09): the grenade end to end, bots throwing it, the sprinkler, the paint wall, the sub picked on Weapon Select, saved, planned and replayed, with its HUD mark and a rebindable Sub action on Keyboard Keys and Pad Buttons, and bots using all three subs, with a balance pass. Next, slice 7: final art and sound cues. [Plan](docs/feature-roadmap.md#sub-weapons-road11). |
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
`OOP` OOP guide, `DES` design process, `DOC`/`CI`/`TOOL` the named file.

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
