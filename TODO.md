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
2. **Qualify the new map (`POL#17`):** use the original [Crossfeed baseline](docs/polish-crossfeed.md)
   and its measured routes, paint budget, spawn approaches, firing breaks and individual panes
   to direct the coupled pass. Preserve the earlier CQ experiment as a comparison.
3. **Qualify the coupled candidate (`POL#3`, `POL#27`):** use the recorded Crossfeed comparison and
   resized Movement Course for keyboard/pad observations, camera warning and all five tool roles.
   Sniper supply and Mister lifetime remain hypotheses. Iterate geometry when an observation concerns
   a route; re-check 1-4P and heavy paint after changes.
4. **Then the other arenas:** `POL#21`, after reference-arena acceptance; the finish review
   and fresh-player observations (`POL#24`) still gate wider art production.

UI artwork uses the approved gameplay style and remains outside the current blockout aim.
Rows outside this sequence (audio, release, engine, tooling) are picked from OPEN
when they don't compete with it.

**Supporting implementation choices.** The current environment kit, small-view treatment
and static landmarks are approved for this stage. The retained parallax comparison
recommends keeping the shipped arenas static. The following supporting work can proceed locally
when it does not compete with the blockout work.
This is a selection guide, not another priority order; each owner defines a bounded first slice.

| id | next implementation slice | owner |
|---|---|---|
| `DBG#1` | Audit further colour consumers; twenty existing colour paths now tune/reset/save | [Console](docs/debug-console.md#remaining-live-tuning-work-dbg1) |

**Waiting on the author**, whatever the order above. Each is a `DECISION` row below:
- **Attended reviews:** the editor GUI and saved-stage play pass (`ED#5`), in-game audio
  listening and optional-cue choices (`POL#23`), visible uncontended performance captures
  and a supported hardware baseline (`POL#25`).
- **Hands and hardware:** a physical-pad session (`ROAD#2`, `DBG#7`, `POL#18`, the pad half of `POL#3`) and
  a clean Windows PC for the Release ZIP (`ROAD#1`). This machine has no pad.

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6. It records the 2026-10-09
switch to Paint.exe Maximalism and the accepted 2026-10-15 round-2 art rules. The pixel-kit
questions in the polish sub-docs are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#16` | DEFERRED | MED | XL | Extend the approved treatment through accepted arenas after `POL#17`/`POL#21`; remaining UI art is a later polish pass. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |

## Feel and feedback

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §2, §4 and §8, with the
[Movement Course](docs/polish-movement-course.md) and [feedback prototype](docs/polish-feedback.md)
records. Hit feedback (gap audit 5.3) and the "this is you" marker (2.4) live here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#3` | OPEN | MED | M | Qualify selected shorter airtime and C2 framing with all five tools: keyboard comfort, recognition, rear-threat awareness and input margins. Late 3/4P Course landing exposure is corrected geometrically. Preserve legacy 200-unit climbs; height feedback and pad review remain open. [Framing comparison](docs/polish-crossfeed.md#narrow-pane-framing-comparison-2026-10-20), [course](docs/polish-movement-course.md). |
| `POL#18` | DECISION | MED | M | Physical-pad comfort review of resume timing and per-slot deadzones/response previews with two controllers; implementation and save association are complete. Needs pads (`ROAD#2`). [§8](docs/polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds). |
| `POL#19` | DEFERRED | LOW | L | Untimed learning workshop on the Movement Course; deferred until fresh-player observations exist. [§8](docs/polish-plan.md#let-players-learn-by-making-a-mess). |

## Levels and the editor

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §3 and §8 for the layouts;
[docs/level-editor.md](docs/level-editor.md) for the editor.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#17` | OPEN | HIGH | XL | Qualify the original [Crossfeed map](docs/polish-crossfeed.md) against the [research criteria](docs/level-design-research.md); preserve existing maps and the CQ experiment. Iterate directed routes, spawn pressure, return pacing, all five tools and 1-4P/heavy-paint panes with the coupled tuning pass. Human reference-arena acceptance remains open. [§3](docs/polish-plan.md#3-redesign-the-levels-in-playable-blockouts). |
| `POL#27` | OPEN | HIGH | L | Qualify the five-tool candidate with movement (`POL#3`): adversarial lanes, resource pressure and human balance. Sniper reserve and Mister lifetime hypotheses remain open. [Opening comparisons](docs/polish-crossfeed.md#aimed-and-delayed-sniper-openings-2026-10-20), [requirements](docs/polish-plan.md#design-requirements). |
| `POL#21` | DEFERRED | MED | XL | King of the Hill and Turbulence blockouts after reference-arena acceptance; optional 1v1 CQ variant, a brief per layout, side-swap qualification. [Briefs](docs/polish-plan.md#initial-arena-briefs). |
| `ED#5` | DECISION | LOW | S | Record an attended editor GUI pass and play a saved stage; document/hidden-window checks are already implemented. [Acceptance](docs/level-editor.md#acceptance-checks). |

## Audio replacement

**Owner:** [docs/polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set). The
accepted source set and live replacement bank are integrated. The remaining cue hooks and
mix still belong here; a missing-bank silent fallback is not a defect.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#23` | DECISION | HIGH | L | Listen to the in-game 1/2/4-player mix and decide whether optional detail earns a cue; world detail follows the reference arena. [Remaining work](docs/audio-inventory.md#9-runtime-migration-and-remaining-integration). |
| `ROAD#3` | BLOCKED | MED | M | Replacement-audio acceptance: gains, mute, suspension, resumed loops, the multiplayer mix; then the audible release and the missing-bank fallback separately. Waits on `POL#23`. |

## Acceptance and release

**Owner:** [docs/feature-roadmap.md](docs/feature-roadmap.md#review-follow-up-ammunition-coverage-and-release-checks)
(its numbered hardware list) and [polish-plan §7](docs/polish-plan.md#7-acceptance-and-delivery).
The 2026-09-26 fix docs leave the same hardware checks open; they are frozen, so the rows live
here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#1` | DECISION | MED | S | Run the extracted Release ZIP on a clean Windows 10/11 x64 machine. Needs one. |
| `ROAD#2` | DECISION | MED | M | Physical-pad sessions: 2P and 4P on every stage and weapon; join, unplug/replug, pause, focus, fullscreen/resize, series draws, rematch, relaunch; plus a long multiplayer soak. Needs pads. |
| `ROAD#4` | DEFERRED | LOW | M | A pad that returns in a different XInput slot cannot reclaim its player mid-match. Deferred until it proves inconvenient. [First increment](docs/feature-roadmap.md#first-increment-explicit-joining-and-reconnect-pause). |
| `POL#24` | BLOCKED | MED | M | The finish review and fresh-player observations on the reference match; the gate before production widens. Waits on the reference arena (`POL#17`). |
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
| `ROAD#5` | DEFERRED | LOW | M | Per-player handicaps: physical multiplayer observations (`ROAD#2`) must establish the need and mechanic. [Decision](docs/feature-roadmap.md#handicaps). |
| `GAP#4.2` | UNSCOPED | MED | L | Define the next bounded action/pad rebinding slice; keyboard Jump is implemented and validated. Rebinding artwork remains a later UI pass. [Scope](docs/feature-roadmap.md#action-bindings-gap42). |
| `GAP#5.1` | BLOCKED | LOW | M | The blank startup window waits on `ENG#19`. Hard-cut transitions are a later UI-art slice using the approved style. Level builds [take under 2.5 ms in Release](docs/polish-plan.md#level-build-pause-measured-2026-10-17), so no loading card. |
| `GAP#L9` | UNSCOPED | LOW | M | Scope presentation-rate/vsync settings separately from fixed 60 Hz simulation; the pin exposes no client vsync switch. [Boundary](docs/feature-roadmap.md#presentation-rate-gapl9). |
| `GAP#L11` | UNSCOPED | LOW | L | Define locale selection, fallback, font coverage and numeric conventions separately; text extraction is complete. [Boundary](docs/menu-presentation.md#remaining-text-slices-gap84), [font recipe](game/art/README.md#sprite-font-recipe-gap87). |

## Engine demonstration

**Owner:** [docs/review/gap-audit.md, Class 2](docs/review/gap-audit.md#class-2--gaps-in-the-role-as-the-engines-demonstration),
frozen as above, with live [menu and demonstration scopes](docs/feature-roadmap.md#per-player-menu-focus-gapd1).
These add to the hierarchies, never flatten them (CLAUDE.md); D6 is `GAP#4.2`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#D2` | DEFERRED | MED | M | Additional widget/page bases need a concrete caller beyond OptionRow; scope only when an accepted page requires one. [Boundary](docs/feature-roadmap.md#other-demonstration-gaps). |
| `GAP#D5` | DEFERRED | MED | M | Add rotated/moving builder structures and useful arena placements during `POL#17`/`POL#21` blockouts; qualify collision, support and paint. [Boundary](docs/feature-roadmap.md#other-demonstration-gaps). |

## Engine reports

**Owner:** [docs/review/engine-reports.md](docs/review/engine-reports.md), the live ledger. Check
engine master before filing, not the pin. Gap-audit Class 3 items not yet in the ledger are
`GAP#F*`. The fourteen drafts remain unfiled. Minimal engine-focused reports with
reproducible examples are approved; recheck upstream and prepare each report for
Labrador's established issue route. These rows do not represent fixes already requested.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#6` | OPEN | MED | S | Review the prepared shared Music/SFX attenuation request and refresh upstream before filing; its synthetic null-audio Debug/Release exhibit passes and remains unfiled. Listener/voice-budget proposals remain outside this slice. [Report](docs/engine-reports/audio-attenuation.md). |
| `ENG#9` | OPEN | MED | S | Review the prepared optional post-poll observer request and refresh upstream immediately before filing; its synthetic Debug/Release exhibit passes. Input routing/capture remains separately scoped and unanswered. [Report](docs/engine-reports/input-observer.md). |
| `ENG#10` | OPEN | LOW | S | Review the narrowed, unfiled host-capture convenience request and tested final-State alternative; refresh upstream before filing. Redraw is not unavoidable, and no displayed-frame outcome is qualified. [Report](docs/engine-reports/frame-readback.md). |
| `ENG#11` | OPEN | LOW | S | Review the prepared optional bare-system-menu suppression request and refresh upstream before filing; synthetic Debug/Release F10/Alt requests reproduce and remain unfiled. [Report](docs/engine-reports/system-menu-keys.md). |
| `ENG#12` | OPEN | MED | S | Review the prepared named in-memory JSON parsing request and refresh upstream before filing; its synthetic CMake Debug/Release exhibit passes and remains unfiled. Object enumeration/source-span portions remain open. [Report](docs/engine-reports/json-candidate-parsing.md). |
| `ENG#13` | OPEN | LOW | S | Review the prepared canonical `Key`-name request and refresh upstream before filing; its synthetic Debug/Release exhibit passes and remains unfiled. [Report](docs/engine-reports/key-names.md). |
| `ENG#14` | OPEN | LOW | S | Review the prepared CPU frame-statistics request and refresh upstream before filing; its manual public-renderer example passes Debug/Release, while Application phases/GPU timing remain unmeasured. [Report](docs/engine-reports/cpu-frame-statistics.md). |
| `ENG#PARKED` | DEFERRED | LOW | S | Parked asks to ride with another report: the ignored `window_size_changed`, a desktop-size query, an atomic JSON write (F5, F9). [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `GAP#F6` | UNSCOPED | LOW | S | Rumble (`set_vibration`, capabilities): unfiled, no exhibit. The FPS half is report 14. |
| `ENG#15` | OPEN | MED | S | Review the prepared, unfiled development-staging convenience request and refresh upstream before filing; its synthetic Debug/Release phases pass. Runtime-install scope stays separate. [Report](docs/engine-reports/runtime-staging.md). |
| `ENG#16` | OPEN | LOW | S | Review the prepared hidden-window Apply bug report and refresh upstream before filing; the public Application call reproduces in Debug/Release. [Report](docs/engine-reports/hidden-window-apply.md). |
| `ENG#17` | OPEN | LOW | S | Review the prepared DPI-sizing request and refresh upstream before filing; Debug/Release hidden 96-DPI baselines and rectangle queries pass. Scaled/transition acceptance and OCA-66 remain open. [Report](docs/engine-reports/window-dpi.md). |
| `ENG#18` | OPEN | LOW | S | Review the prepared Label visibility convenience; refresh upstream before filing. Null Debug/Release passes; unfiled. Order/retirement/font questions stay separate. [Report](docs/engine-reports/label-visibility.md). |
| `ENG#19` | OPEN | LOW | S | Review the prepared, unfiled later-show convenience request and refresh upstream before filing; hidden creation passes four Debug/Release cases. First visible game frame and `GAP#5.1` remain open. [Report](docs/engine-reports/startup-visibility.md). |
| `ENG#20` | OPEN | LOW | S | Review the active view-context request; refresh upstream before filing. Source audit and null Debug/Release illustration pass; unfiled. Art policy and Scene/order integration stay separate. [Report](docs/engine-reports/view-context.md). |
| `ENG#21` | OPEN | LOW | S | Review the prepared action-evaluation request (formerly `GAP#F2`) and refresh upstream before filing; its synthetic Debug/Release exhibit passes. [Report](docs/engine-reports/action-evaluation.md). |
| `GAP#F10` | DEFERRED | LOW | S | `DrawList` colour transforms beyond multiply (greyscale, daltonize, hue remap); no exhibit yet. |

## Developer console

**Owner:** [docs/debug-console.md, Not done](docs/debug-console.md#not-done). Script control
flow and a console over the menus are deliberately not done and get no rows.

| id | status | sev | size | item |
|---|---|---|---|---|
| `DBG#1` | OPEN | LOW | M | Extend colour tuning beyond twenty audited fields; general string/font edits and missing-key insertion remain separate slices. [Scope](docs/debug-console.md#remaining-live-tuning-work-dbg1). |
| `DBG#2` | DEFERRED | LOW | L | Replays hold only on the same build, machine and step, within one round. |
| `DBG#4` | BLOCKED | LOW | S | Typing in the live console blanks the whole keyboard seat. Waits on `ENG#9`. |
| `DBG#5` | BLOCKED | LOW | S | The perf overlay has no GPU, present or submit time. Waits on `ENG#14`. |
| `DBG#6` | DEFERRED | LOW | M | Scripts stop when the window loses focus; no run-while-inactive switch. |
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
| Closed engine reports (1–5, 7, 8) | [engine-reports.md, Closed](docs/review/engine-reports.md#closed) | a pin bump that answers a live report and leaves a workaround to delete |
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
(`ENG#12`); the rest are `PREFIX#n` by owner: `STORY` backstory, `POL` polish plan, `ED` level
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
hierarchy", "make it a variant") are misfiled (CLAUDE.md), and a missing-bank silent fallback is correct.
