# TODO: open work

An index of open work, one row per item; each row points to the doc that owns the detail. The
rules for editing it are at the end, in [How to read this file](#how-to-read-this-file). A fresh
thread starts here: read **NOW**, then the owner of the row it picks up.

## NOW

**The only ordering in this file.** Keep this block to the current aim and the next few actions;
results go in their owners, and `git log` is the changelog.

**Aim (2026-10-15):** the [backstory](docs/backstory.md#building-it) build is complete, including
its accepted sounds. Build gameplay artwork to the accepted
[Paint.exe art rules](docs/polish-plan.md#1-establish-the-art-and-display-rules). The author's
order is gameplay style first, UI after.

**Order** ([development order](docs/polish-plan.md#development-order-2026-10-17)):
1. **Lock the gameplay art style:** `POL#7` painter and tools, `POL#14` player and shot cues,
   `POL#9` material/prop/background kit, in the current arenas.
2. **Then levels with movement and weapons:** the Close Quarters blockout (`POL#17`), jump/movement
   tuning (`POL#3`) and weapon tuning (`POL#27`) together, the kit fitted to it, then the other
   arenas (`POL#21`).

UI artwork follows the gameplay style. Rows outside this sequence (audio, release, engine,
tooling) are picked from OPEN when they don't compete with it.

**Waiting on the author**, whatever the order above. Each is a `DECISION` row below:
- **Hands and hardware:** a physical-pad session (`ROAD#2`, `DBG#7`, `POL#18`, the pad half of `POL#3`) and
  a clean Windows PC for the Release ZIP (`ROAD#1`). This machine has no pad.

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6. It records the 2026-10-09
switch to Paint.exe Maximalism and the accepted 2026-10-15 round-2 art rules. The pixel-kit
questions in the polish sub-docs are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#7` | OPEN | HIGH | M | Human review of character/team/tool readability in 1P and 4P play, in the current arenas; re-check in Close Quarters (`POL#17`). The author's 2026-10-18 build (no teammate accents), its poses, sleeves, grips and all five tools are implemented. [Painter](docs/polish-painter.md), [tools](docs/polish-tools.md). |
| `POL#9` | OPEN | HIGH | L | Tool Props materials, shadows, backdrop and quiet cloud drift are in all four arenas. Left: human review clean/painted, every pair, 1P/4P; landmarks, further room life, parallax; CQ's own backdrop with `POL#17`. [Kit](docs/polish-terrain.md), [§4](docs/polish-plan.md#environment-and-background). |
| `POL#16` | OPEN | MED | XL | Carry the style through accepted arenas after their blockouts; human presentation/readability review remains. Gameplay style first, UI after. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |

## Feel and feedback

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §2, §4 and §8, with the
[Movement Course](docs/polish-movement-course.md) and [feedback prototype](docs/polish-feedback.md)
records. Hit feedback (gap audit 5.3) and the "this is you" marker (2.4) live here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#3` | DEFERRED | MED | M | Movement tuning, done with the blockout (`POL#17`) once the art style is locked; values stay provisional. Keyboard/mouse: jump a bit floaty and high (`jump_*` candidates to try). Pad half with `ROAD#2`. [Playtest](docs/polish-movement-course.md#keyboardmouse-playtest-2026-10-17). |
| `POL#14` | OPEN | MED | M | Human review of the implemented player badges, aim/readiness and blocked-shot cues, in the current arenas; re-check in Close Quarters (`POL#17`). [§8](docs/polish-plan.md#help-players-read-the-tool-and-the-fight). |
| `POL#18` | DECISION | MED | M | Physical-pad comfort review of resume timing and per-slot deadzones/response previews with two controllers; implementation and save association are complete. Needs pads (`ROAD#2`). [§8](docs/polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds). |
| `POL#19` | DEFERRED | LOW | L | Untimed learning workshop on the Movement Course; deferred until fresh-player observations exist. [§8](docs/polish-plan.md#let-players-learn-by-making-a-mess). |

## Levels and the editor

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §3 and §8 for the layouts;
[docs/level-editor.md](docs/level-editor.md) for the editor.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#17` | DEFERRED | HIGH | XL | Close Quarters reference blockout against the §3 requirements table (routes, spawns, paint budget, weapon roles, pacing, side fairness, recovery pocket), tuned with `POL#3`. After the art style is locked. [§3](docs/polish-plan.md#3-redesign-the-levels-in-playable-blockouts). |
| `POL#27` | DEFERRED | HIGH | L | Weapon tuning (range, rate, damage, paint output, supply) for all five tools, done with the blockout (`POL#17`) and movement (`POL#3`); values stay provisional. [Order](docs/polish-plan.md#development-order-2026-10-17), [§3](docs/polish-plan.md#design-requirements). |
| `POL#21` | DEFERRED | MED | XL | King of the Hill and Turbulence blockouts after CQ is accepted; optional 1v1 CQ variant, a brief per layout, side-swap qualification. [Briefs](docs/polish-plan.md#initial-arena-briefs). |
| `ED#4` | UNSCOPED | LOW | S | Registering a stage is manual (`manifest.json` and `cmake/runtime_assets.cmake`); the editor does not help. |
| `ED#5` | OPEN | LOW | S | The manual GUI pass and the play-test of a saved stage have no recorded result. [Acceptance](docs/level-editor.md#acceptance-checks). |
| `ED#6` | OPEN | LOW | S | Movement Course placeholder: it borrows Turbulence's icon frame; give it its own icon. |

## Audio replacement

**Owner:** [docs/polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set). The
accepted source set and live replacement bank are integrated. The remaining cue hooks and
mix still belong here; a missing-bank silent fallback is not a defect.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#23` | OPEN | HIGH | L | Assess [optional audio detail and the mix](docs/audio-inventory.md#9-runtime-migration-and-remaining-integration) at 1/2/4 players. |
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
| `POL#25` | OPEN | MED | M | Qualify [visible, uncontended frame pacing](docs/polish-frame-pacing.md) against 60 fps on a recorded reference machine and set the supported hardware baseline. |
| `GAP#7.6` | BLOCKED | LOW | S | Release identity: the stock `directx.ico` waits on the gameplay style lock (`POL#7`, `POL#14`, `POL#9`); the first tag and any release job are the author's. VERSIONINFO, the ZIP, the build stamp and the CI ZIP artifact are done. |

## Game gaps

**Owner:** [docs/review/gap-audit.md](docs/review/gap-audit.md), Class 1, by its section numbers.
The audit is **frozen** (2026-08-11): it owns each finding and its suggested shape, not its
status, so these rows were checked against the code on 2026-10-13. The no-live-score and
random-pair findings (1.1, 2.1) are Splatoon borrowings and stay closed. The live
[match scope decisions](docs/feature-roadmap.md#match-scope-decisions-2026-10-15) own the
2026-10-15 calls on rulesets, solo practice, random teams and handicaps.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#5` | DEFERRED | LOW | M | Per-player handicaps: physical multiplayer observations (`ROAD#2`) must establish the need and mechanic. [Decision](docs/feature-roadmap.md#handicaps). |
| `GAP#4.2` | OPEN | MED | L | No action map or rebinding: bindings are constexprs in `player_input.h`/`menu_input.h`. The engine's `gamepad.h` waits on this client (Class 2 D6); `GAP#F2` waits on it. |
| `GAP#5.1` | BLOCKED | LOW | M | Screen changes are hard cuts: the transition look is UI art after the gameplay style lock (`POL#7`, `POL#14`, `POL#9`). Level builds [take under 2.5 ms in Release](docs/polish-plan.md#level-build-pause-measured-2026-10-17), so no loading card. The blank startup window waits on `ENG#19`. |
| `GAP#6.1` | OPEN | MED | M | The last match setup is not saved, so every launch walks the whole menu path again. |
| `GAP#L9` | OPEN | LOW | M | No frame-rate or vsync setting, no render interpolation; `target_fps` is a literal 60. |
| `GAP#L11` | DEFERRED | LOW | L | No locale axis; waits on `GAP#8.4`. Fonts regenerate from [a recipe](game/art/README.md#sprite-font-recipe-gap87) and text can wrap. |

## Content pipeline

**Owner:** [docs/review/gap-audit.md](docs/review/gap-audit.md) Theme 8, frozen as above.
The live [art/content policy](game/art/README.md#runtime-content-separation-gap86) owns
the accepted `GAP#8.6` scope.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#8.4` | OPEN | LOW | M | Other setting values, composed stage navigation/count hints, best-of mode and roster copy, connection/readout text, player-specific Pause titles, session continuation text and composed match/results text remain C++ literals. [Boundary](docs/menu-presentation.md#shared-headings-and-complete-control-legends). |

## Engine demonstration

**Owner:** [docs/review/gap-audit.md, Class 2](docs/review/gap-audit.md#class-2--gaps-in-the-role-as-the-engines-demonstration),
frozen as above. These add to the hierarchies, never flatten them (CLAUDE.md); D6 is `GAP#4.2`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#D1` | OPEN | MED | M | Give FocusGroup's per-slot cursors a client: a per-player menu page replacing team/weapon select's hand-rolled cursors; a misfit becomes a report. |
| `GAP#D2` | OPEN | MED | M | No toggle/cycle/slider rows beyond OptionRow, no progress bar, no modal, no list-page base. |
| `GAP#D5` | OPEN | MED | M | Rotated and moving structures behind the builder, one of each in a shipped stage, so the SAT and moving-response paths get a client. |

## Engine reports

**Owner:** [docs/review/engine-reports.md](docs/review/engine-reports.md), the live ledger. Check
engine master before filing, not the pin. Gap-audit Class 3 items not yet in the ledger are
`GAP#F*`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#6` | OPEN | MED | S | File report 6: no audio bus, master gain, listener or voice cap. [Report](docs/review/engine-reports.md#6-no-audio-bus-master-gain-listener-or-voice-cap--draft-with-mixing-exhibit). |
| `ENG#9` | OPEN | MED | S | File report 9: no application-level per-frame hook or input consume scope (`DBG#4` waits on it). [Report](docs/review/engine-reports.md#9-no-application-level-per-frame-hook-or-input-capture--draft-with-console-exhibit). |
| `ENG#10` | OPEN | LOW | S | File report 10: no read-back of the presented frame. [Report](docs/review/engine-reports.md#10-no-way-to-read-back-the-frame-that-was-presented--draft-with-screenshot-exhibit). |
| `ENG#11` | OPEN | LOW | S | File report 11: F10 and a lone Alt enter the system menu with no opt-out. [Report](docs/review/engine-reports.md#11-f10-and-a-lone-alt-belong-to-the-system-menu-with-no-opt-out--draft-low-priority). |
| `ENG#12` | OPEN | MED | S | File report 12: JSON is read-only, path-only, with no keys or source spans. [Report](docs/review/engine-reports.md#12-json-is-read-only-path-only-and-opaque--draft-with-live-tuning-exhibit). |
| `ENG#13` | OPEN | LOW | S | File report 13: `Key` has no names. [Report](docs/review/engine-reports.md#13-key-has-no-names--draft-low-priority). |
| `ENG#14` | OPEN | LOW | S | File report 14: no frame statistics or GPU timing (`DBG#5` waits on it). [Report](docs/review/engine-reports.md#14-no-frame-statistics-or-gpu-timing--draft-with-perf-exhibit). |
| `ENG#PARKED` | DEFERRED | LOW | S | Parked asks to ride with another report: the ignored `window_size_changed`, a desktop-size query, an atomic JSON write (F5, F9). [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `GAP#F2` | BLOCKED | LOW | S | The action-map/Gamepads report (per-slot lookup, trigger threshold, "what did this pad press"). Waits on `GAP#4.2`. |
| `GAP#F6` | UNSCOPED | LOW | S | Rumble (`set_vibration`, capabilities): unfiled, no exhibit. The FPS half is report 14. |
| `ENG#15` | OPEN | MED | S | File report 15: repeated runtime packaging and content staging; game asset/release policy stays here. [Report](docs/review/engine-reports.md#15-runtime-packaging-and-content-staging-are-repeated-in-the-client--draft-with-release-exhibit). |
| `ENG#16` | OPEN | LOW | S | Recheck master and file report 16: windowed Apply shows a deliberately hidden window. [Report](docs/review/engine-reports.md#16-windowed-apply-shows-a-deliberately-hidden-window--draft-with-probe-exhibit). |
| `ENG#17` | OPEN | LOW | S | File report 17: the Win32 window sizes its frame at system DPI and ignores `WM_DPICHANGED` once the client is per-monitor aware; answers OCA-66. [Report](docs/review/engine-reports.md#17-the-win32-window-ignores-dpi-once-the-client-opts-in--draft-with-manifest-exhibit). |
| `ENG#18` | OPEN | LOW | S | Recheck master and file report 18: a `Label` in a `Scene` cannot be hidden (only a fully zero tint) or drawn over collision objects. [Report](docs/review/engine-reports.md#18-a-label-in-a-scene-can-be-neither-hidden-nor-raised--draft-with-station-label-exhibit). |
| `ENG#19` | OPEN | LOW | S | Recheck master and file report 19: the window is shown before the manifest loads, with no later show call. [Report](docs/review/engine-reports.md#19-the-window-is-shown-before-the-client-has-loaded-anything--draft-with-startup-timing-exhibit). |
| `GAP#F10` | DEFERRED | LOW | S | `DrawList` colour transforms beyond multiply (greyscale, daltonize, hue remap); no exhibit yet. |

## Developer console

**Owner:** [docs/debug-console.md, Not done](docs/debug-console.md#not-done). Script control
flow and a console over the menus are deliberately not done and get no rows.

| id | status | sev | size | item |
|---|---|---|---|---|
| `DBG#1` | OPEN | LOW | M | `tune` handles numbers only, and `tune.save` cannot insert a missing key. |
| `DBG#2` | DEFERRED | LOW | L | Replays hold only on the same build, machine and step, within one round. |
| `DBG#4` | BLOCKED | LOW | S | Typing in the live console blanks the whole keyboard seat. Waits on `ENG#9`. |
| `DBG#5` | BLOCKED | LOW | S | The perf overlay has no GPU, present or submit time. Waits on `ENG#14`. |
| `DBG#6` | DEFERRED | LOW | M | Scripts stop when the window loses focus; no run-while-inactive switch. |
| `DBG#7` | DECISION | LOW | S | No driven check of the pad Back+Start chord or a bot on an unplugged seat. Run it in the `ROAD#2` session. |

## Tooling, CI and docs

**Owner:** the file named in each row.

| id | status | sev | size | item |
|---|---|---|---|---|
| `CI#1` | OPEN | LOW | M | The foreground-dependent `check-debug-console` remains local; CI has no reliable focus workflow for it. [.github/workflows/ci.yml](.github/workflows/ci.yml). |

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
| Code TODO/FIXME markers | `game/`, `editor/`, `scripts/`, `cmake/` | a new marker, which should be a row here instead |

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
