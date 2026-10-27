# TODO: open work

An index of open work, one row per item; each row points to the doc that owns the detail. The
rules for editing it are at the end, in [How to read this file](#how-to-read-this-file). A fresh
thread starts here: read **NOW**, then the owner of the row it picks up.

## NOW

**The only ordering in this file.** Keep this block to the current aim and the next few actions;
results go in their owners, and `git log` is the changelog.

**Aim (clarified 2026-10-20):** research comparable combat levels, create an original new map,
then tune movement and all five weapons together against it. Preserve the existing arenas and CQ draft.
The author approved the current gameplay style;
it is a working baseline, open to further iteration, not final art or release acceptance.
See the [stage approval](docs/polish-plan.md#development-order-2026-10-17).

**Order** ([research and design criteria](docs/level-design-research.md)):
1. **Research before values:** use the recorded primary sources, concrete level examples and
   ColourWars-specific hypotheses to define required, optional and recovery routes.
2. **New map (done 2026-10-25):** the author accepted [Crossfeed](docs/polish-crossfeed.md#author-arena-acceptance-2026-10-25)
   as the reference arena. Preserve the earlier CQ experiment as a comparison.
3. **Now, the other arenas (`POL#21`):** apply the Crossfeed process to King of the Hill and
   Turbulence, keeping the current P3/five-tool values as the baseline. The King of the Hill
   blockout is [Press Tower](docs/polish-press-tower.md) (explicit-only, beside the preserved
   original; attended play is `POL#31`); Crossfeed is now in Random at the author's request.
   Turbulence's [static, mover and scored-slab comparisons](docs/polish-turbulence.md)
   are explicit-only. The scoped software qualification is complete: bounded
   fire/escape, routes/replay, local paint/cover roles, native composition and
   endings. Attended geometry choice, crossing comfort and opposed balance
   are now the next step (`POL#21`); current values remain provisional.
4. **Alongside, finish the coupled candidate (`POL#3`, `POL#27`):** five-tool balance, C2
   recognition and the pad half; Sniper supply and Mister lifetime remain hypotheses. The finish
   review and fresh-player observations (`POL#24`) still gate wider art production.

5. **Feeding the above:** the first headless five-tool report
   is recorded in the arena owners; attended decisions remain above. The default-off
   paint-underfoot prototype is available for `POL#30`. Runtime horizontal motion
   and public-loader replay, editor authoring and saved-stage round trips are qualified;
   the Turbulence mover comparison qualifies a local paint role. Fixed scored
   slabs now have core/editor/native, local Roller-role and public driven
   route/replay qualification; arena pressure and selection remain `POL#21`
   ([boundary](docs/feature-roadmap.md#horizontal-movement-capability-2026-10-26)).

UI artwork uses the approved gameplay style and remains outside the current blockout aim.
Rows outside this sequence (audio, release, engine, tooling) are picked from OPEN
when they don't compete with it.

**Implementation exhaustion audit, 2026-10-26.** The audit checked the remaining
owners beyond their row statuses. It found and completed the bounded Debug
background-script capability, reconciled stale geometry references and ran the
25 previously unrun existing Release cases. Remaining play/hardware reviews,
later art and unselected capability contracts retain their named gates below.
At that point, no further concrete independent implementation was selected by an owner.

**Author-requested next implementation (2026-10-27):** playable AI bot players
with Easy, Normal and Hard difficulty settings (`ROAD#10`). Setup, saved identity,
human-only panes and a shipping input driver are integrated. Static arena chains
include Turbulence's continuous ascent and local recovery. Moving/opposed local
combat and 200 natural-spawn team trials (120 mixed-tool over three seeds, 40
sustained 1v1 middle-retention trials) qualify presence; all 450 route gates pass.
The CQ home-band loop is fixed and slow tools spend shots only where they land.
The descent latch and refill retreat are decided, the scan cost is measured
(p99 about 1 ms per step) and bots ride and leave movers. Next is the attended
difficulty comparison; difficulty values remain provisional.
[Scope and acceptance](docs/feature-roadmap.md#ai-bot-players-road10).

**Waiting on the author**, whatever the order above. Each is a `DECISION` row below:

- **Turbulence play and selection:** choose among the explicit-only comparisons after
  both-side 1-4P pressure, crossing-comfort and regional-value play (`POL#21`).
- **Press Tower play:** both sides and 1-4P, the summit's value and the halves' feel (`POL#31`).
- **Reference-match review:** Crossfeed is accepted; its spawn pressure, C2 identity/rear-threat
  recognition and 1-4P clean/heavy readability go to the finish review (`POL#24`), with
  five-tool pressure/resource tradeoffs (`POL#27`) and C2/pad movement (`POL#3`).
- **Unborrowed Splatoon mechanics:** paint underfoot, a painting-charged special and a
  splat tally, each yes or no, with or after `POL#27` (`POL#30`).
- **Low-paint warning:** listen to the provisional cue in the 1/2/4P mix and look at the
  HUD tag and figure tank at 4P with sound muted (`POL#38`).
- **Cursor marker:** judge the pointer marquee in play at 1/2/4P (`POL#39`).
- **Handicaps:** whether Boost +1/+2 feel fair against bots in play (`ROAD#5`).
- **Display policy:** choose the recorded fractional, camera-snap or integer-cell/letterbox
  treatment (`POL#34`); current behavior is documented, with no candidate accepted.
- **Attended reviews:** the editor GUI and saved-stage play pass (`ED#5`), in-game audio
  listening and optional-cue choices (`POL#23`), visible uncontended performance captures
  and a supported hardware baseline (`POL#25`), and startup-window captures for Labrador
  (`GAP#5.1`).
- **Report filing:** the guarded resize request and example (`ENG#23`) are
  prepared and locally qualified, alongside the vibration-output draft (`ENG#25`).
  Filing needs separate author authorization.
- **Hands and hardware:** a physical-pad session (`ROAD#2`, `ROAD#9`, `DBG#7`, `POL#18`, the pad half of `POL#3`) and
  a clean Windows PC for the Release ZIP (`ROAD#1`). This machine has no pad.

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6. It records the 2026-10-09
switch to Paint.exe Maximalism and the accepted 2026-10-15 round-2 art rules. The pixel-kit
questions in the polish sub-docs are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#16` | DEFERRED | MED | XL | Extend the approved treatment through accepted arenas after `POL#21`; remaining UI art is a later polish pass. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |
| `POL#34` | DECISION | MED | M | Choose the recorded fractional, camera-snap or integer-cell/letterbox display policy after 720p/1080p/QHD motion and 1/2/4P readability review. [Record](docs/polish-plan.md#display-scaling-record). |
| `POL#35` | DEFERRED | LOW | S | Signage and captioned Paint-artefact jokes need the author's wording; with `POL#16`. [Terrain](docs/polish-terrain.md#stage-approval-and-remaining-environment-work-2026-10-20). |

## Feel and feedback

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §2, §4 and §8, with the
[Movement Course](docs/polish-movement-course.md) and [feedback prototype](docs/polish-feedback.md)
records. Hit feedback (gap audit 5.3) and the "this is you" marker (2.4) live here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#3` | DECISION | MED | M | Retain the author's positive P3 movement baseline. Attended C2 identity/rear-threat recognition, input-margin and physical-pad review remain; preserve legacy climbs. [Acceptance](docs/polish-crossfeed.md#author-arena-acceptance-2026-10-25), [course](docs/polish-movement-course.md). |
| `POL#18` | DECISION | MED | M | Physical-pad comfort review of resume timing and per-slot deadzones/response previews with two controllers; implementation and save association are complete. Needs pads (`ROAD#2`). [§8](docs/polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds). |
| `POL#19` | DEFERRED | LOW | L | Untimed learning workshop on the Movement Course; deferred until fresh-player observations exist. [§8](docs/polish-plan.md#let-players-learn-by-making-a-mess). |
| `POL#38` | DECISION | MED | S | Implemented: HUD tag, figure tank and a once-per-descent cue before exhaustion. Listen to the provisional `tool.drip` cue in the 1/2/4P mix (with `POL#23`) and check tag/tank readability muted at 4P. [Record](docs/polish-weapon-feedback.md#low-paint-advance-warning-pol38). |
| `POL#39` | DECISION | MED | S | A hollow marquee now marks the mouse pointer in matches. Judge it in play over pale, dark and painted scenes at 1/2/4P, including its one-frame trail. [Record](docs/polish-plan.md#mouse-cursor-visibility-pol39). |

## Levels and the editor

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §3 and §8 for the layouts;
[docs/level-editor.md](docs/level-editor.md) for the editor.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#27` | DECISION | HIGH | L | Attended five-tool balance/resource review remains; the author's acceptance session tried several tools without a complaint. Sniper reserve and Mister lifetime stay provisional hypotheses. [Evidence](docs/polish-crossfeed.md#author-arena-acceptance-2026-10-25). |
| `POL#30` | DECISION | MED | S | Decide paint underfoot, a painting-charged special and a splat tally, with or after `POL#27`; "no" is a valid answer. [Levers](docs/polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30). |
| `POL#21` | DECISION | MED | XL | Choose the explicit-only [Turbulence Yard](docs/polish-turbulence.md) geometry after attended both-side 1-4P pressure, crossing-comfort and regional-value play. Scoped software comparisons are qualified; optional CQ work follows selection. |
| `POL#31` | DECISION | MED | M | Attended Press Tower play on both sides and 1-4P: spawn pressure, the summit's value and whether the crown halves feel fiddly. [Open](docs/polish-press-tower.md#open). |
| `ED#5` | DECISION | LOW | S | Record an attended editor GUI pass and play a saved stage; document/hidden-window checks are already implemented. [Acceptance](docs/level-editor.md#acceptance-checks). |

## Audio replacement

**Owner:** [docs/polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set). The
accepted source set and live replacement bank are integrated. The remaining cue hooks and
mix still belong here; a missing-bank silent fallback is not a defect.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#23` | DECISION | HIGH | L | Listen to the in-game 1/2/4-player mix and decide whether optional detail earns a cue; world detail follows the reference arena. [Remaining work](docs/audio-inventory.md#9-runtime-migration-and-remaining-integration). |
| `ROAD#3` | BLOCKED | MED | M | Replacement-audio acceptance: gains (now Music/SFX sound-group volumes, report 6), mute, suspension, resumed loops, the multiplayer mix, a lost output device; then the audible release and the missing-bank fallback separately. Waits on `POL#23`. |

## Acceptance and release

**Owner:** [docs/feature-roadmap.md](docs/feature-roadmap.md#review-follow-up-ammunition-coverage-and-release-checks)
(its numbered hardware list) and [polish-plan §7](docs/polish-plan.md#7-acceptance-and-delivery).
The 2026-09-26 fix docs leave the same hardware checks open; they are frozen, so the rows live
here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#1` | DECISION | MED | S | Run the extracted Release ZIP on a clean Windows 10/11 x64 machine. Needs one. |
| `ROAD#2` | DECISION | MED | M | Physical-pad sessions: 2P and 4P on every stage and weapon; join, unplug/replug, pause, focus, fullscreen/resize, series draws, rematch, relaunch; a long soak past the display timers; one non-XInput pad. Needs pads. |
| `ROAD#4` | DEFERRED | LOW | M | A pad that returns in a different XInput slot cannot reclaim its player mid-match. Deferred until it proves inconvenient. [First increment](docs/feature-roadmap.md#first-increment-explicit-joining-and-reconnect-pause). |
| `POL#24` | DECISION | MED | M | The finish review and fresh-player observations on the Crossfeed reference match, including its spawn pressure and 1-4P clean/heavy readability; the gate before production widens. [Process](docs/polish-plan.md#7-acceptance-and-delivery). |
| `POL#25` | DECISION | MED | M | Arrange attended visible, uncontended captures and choose the supported hardware baseline; capture/analysis tooling is implemented. [Acceptance](docs/polish-frame-pacing.md#remaining-acceptance). |
| `GAP#7.6` | DEFERRED | LOW | S | Replace the stock `directx.ico` in the later UI/release-art pass using the approved style; the first tag and any release job are the author's. VERSIONINFO, the ZIP, the build stamp and the CI ZIP artifact are done. |

## Game gaps

**Owner:** [docs/review/gap-audit.md](docs/review/gap-audit.md), Class 1, by its section numbers.
The audit is **frozen** (2026-08-11): it owns each finding and its suggested shape, not its
status. Live implementation scopes are linked in the rows. The no-live-score and
random-pair findings (1.1, 2.1) are Splatoon borrowings and stay closed. The live
[match scope decisions](docs/feature-roadmap.md#match-scope-decisions-2026-10-15) own the
2026-10-15 calls on rulesets, solo practice, random teams and handicaps.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#7` | DEFERRED | LOW | M | Per-seat view shake, aim marker and Reduce effects, after the stick-profile pattern; after `POL#27`. [Scope](docs/feature-roadmap.md#per-seat-accessibility-toggles-road7). |
| `ROAD#8` | DEFERRED | LOW | S | More team palette pairs, each checked under colour-vision simulation; later UI art pass. [Scope](docs/feature-roadmap.md#more-team-palette-pairs-road8). |
| `ROAD#5` | DECISION | LOW | S | Per-seat Boost (Off/+1/+2: less damage taken, faster paint refill) is built on Weapon Select (mechanic chosen 2026-10-27); judge the provisional values in attended play against bots. [Scope](docs/feature-roadmap.md#handicaps). |
| `ROAD#9` | DECISION | MED | M | Review the four candidate rumble cues on physical pads, including comfort and distinctness; owner, settings and fake-sink qualification are complete. Needs `ROAD#2`. [Scope](docs/feature-roadmap.md#controller-rumble-road9). |
| `ROAD#10` | OPEN | HIGH | XL | Attended difficulty comparison and usefulness; decide whether bots hold and paint movers and whether world labels clamp to the pane; physical-pad check. Latch, refill retreat (margin 600), scan cost and mover identity are done; values remain provisional. [Scope](docs/feature-roadmap.md#ai-bot-players-road10). |
| `GAP#4.2` | UNSCOPED | MED | L | Other keyboard/pad actions and trigger choices need separate contracts; keyboard Jump and per-slot pad face-button Jump are complete. [Boundary](docs/feature-roadmap.md#pad-jump-face-button-slice-gap42). |
| `GAP#5.1` | DECISION | LOW | M | Blank startup window: capture a visible Release launch to first present, and a hidden one shown natively, for Labrador ([report 19](docs/review/engine-reports.md#19-no-public-later-show-convenience-for-a-hidden-application--needs-more-evidence-cacf112)). Hard cuts are a later UI-art slice. |
| `GAP#L9` | UNSCOPED | LOW | M | Scope presentation-rate/vsync settings separately from fixed 60 Hz simulation; the pin exposes no client vsync switch. [Boundary](docs/feature-roadmap.md#presentation-rate-gapl9). |
| `GAP#L11` | UNSCOPED | LOW | L | Define locale selection, fallback, font coverage and numeric conventions separately; text extraction is complete. [Boundary](docs/menu-presentation.md#remaining-text-slices-gap84), [font recipe](game/art/README.md#sprite-font-recipe-gap87). |

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
`GAP#F*`. The two live drafts remain unfiled. Minimal engine-focused reports with
reproducible examples are approved; recheck upstream and prepare each report for
Labrador's established issue route. These rows do not represent fixes already requested.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#PARKED` | DEFERRED | LOW | S | Asks to ride with another report: ignored `window_size_changed`, desktop-size query, atomic JSON write (F5, F9); runtime install facts (PI-11; report 15); listener/voice cap (report 6); audio-device loss, display-awake owner, a non-XInput reader. [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `ENG#25` | DECISION | LOW | S | Prepared vibration-output draft and qualified isolated XInput exhibit; author authorization and a fresh upstream recheck are required to file. Current task forbids filing. [Report](docs/review/engine-reports.md#25-no-public-controller-vibration-output--draft-with-isolated-adapter-exhibit). |
| `ENG#23` | DECISION | LOW | S | Prepared guarded resize request/exhibit; author authorization to file remains. Current task forbids filing. [Report](docs/review/engine-reports.md#23-resizing-a-hidden-window-activates-it--draft-with-activation-guard-exhibit). |
| `GAP#F10` | DEFERRED | LOW | S | `DrawList` colour transforms beyond multiply (greyscale, daltonize, hue remap); no exhibit yet. |

## Developer console

**Owner:** [docs/debug-console.md, Not done](docs/debug-console.md#not-done). Script control
flow and a console over the menus are deliberately not done and get no rows.

| id | status | sev | size | item |
|---|---|---|---|---|
| `DBG#1` | DEFERRED | LOW | M | Resume only when a concrete field/caller is selected; fifty-eight colour paths are qualified. String/font edits and missing-key insertion need separate scopes. [Scope](docs/debug-console.md#remaining-live-tuning-work-dbg1). |
| `DBG#2` | DEFERRED | LOW | L | Replays hold only on the same build, machine and step, within one round. |
| `DBG#4` | DEFERRED | LOW | S | Typing in the live console blanks the whole keyboard seat; typed keys are play keys and the engine declined input capture (report 9), so any per-key gate is client policy. [Not done](docs/debug-console.md#not-done). |
| `DBG#7` | DECISION | LOW | S | No driven check of the pad Back+Start chord or a bot on an unplugged seat. Run it in the `ROAD#2` session. |

## Tooling, CI and docs

**Owner:** the file named in each row.

| id | status | sev | size | item |
|---|---|---|---|---|
| `CI#1` | UNSCOPED | LOW | M | Find a reliable attended/foreground runner model before adding `check-debug-console` to CI; hosted focus remains unproven. [Constraint](docs/debug-console.md#foreground-checks-ci1), [workflow](.github/workflows/ci.yml). |

## Areas with no open rows

A new row for one of these gets its section back.

| area | owner | a row returns with |
|---|---|---|
| Backstory build | [backstory.md, Building it](docs/backstory.md#building-it) | a missing or regressed story beat; pop, sigh and chair integration are complete |
| Closed engine reports (1–22) | [engine-reports.md, Closed](docs/review/engine-reports.md#closed) | a pin bump that answers a live report and leaves a workaround to delete |
| Feature-roadmap increments 1–5 | [feature-roadmap.md](docs/feature-roadmap.md) | a sixth increment, or a regression from the `ROAD#2` session |
| 2026-09-26 review fixes | `docs/review/2026-09-26-*.md` (frozen) | a new review round; the hardware residue is `ROAD#2` |
| The OOP showcase | [oop-guide.md](docs/oop-guide.md) | a base class or weapon contract the guide no longer describes |
| Splatoon borrowings (no live score, random pairs) | gap audit 1.1, 2.1 | only the author reversing the decision |
| Startup content validation | gap audit 8.1, `game/content_validation.cpp` | a content kind added without validation |
| Content pipeline | [Text scope](docs/menu-presentation.md#remaining-text-slices-gap84), [art/content policy](game/art/README.md#runtime-content-separation-gap86) | a new authored text role or accepted runtime-content requirement; the scoped `GAP#8.4` extraction is complete |
| Code TODO/FIXME markers | `game/`, `editor/`, `scripts/`, `cmake/` | a new marker, which should be a row here instead |
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
editor, `ROAD` feature roadmap, `DBG` developer console, `DOC`/`CI`/`TOOL` the named file.

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
