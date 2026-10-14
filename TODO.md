# TODO: open work

An index of open work, one row per item; each row points to the doc that owns the detail. The
rules for editing it are at the end, in [How to read this file](#how-to-read-this-file). A fresh
thread starts here: read **NOW**, then the owner of the row it picks up.

## NOW

**The only ordering in this file.** Keep this block to the current aim and the next few actions;
results go in their owners, and `git log` is the changelog.

**Aim (2026-10-13):** finish the [backstory](docs/backstory.md) build, which the last two weeks of
commits delivered beat by beat, and settle the [Paint.exe art rules](docs/polish-plan.md#1-establish-the-art-and-display-rules)
that the characters, tools, environment and UI all wait on. The author's order is gameplay
style first, UI after.

The ordered actions are done (`STORY#3`, `ENG#PIN` with `ENG#LEDGER`, `DOC#1`–`DOC#4`, on
2026-10-14). The aim now waits on the author below; pick the next actions from the OPEN rows.

**Waiting on the author**, whatever the order above. Each is a `DECISION` row below:
- **Art:** confirm the Paint.exe round-2 picks (`POL#1`); this gates `POL#7`–`POL#9` and `POL#16`.
  Then decide whether to retire the workshop kit (`POL#2`).
- **Hands and hardware:** a physical-pad session (`ROAD#2`, `DBG#7`, the pad half of `POL#3`) and
  a clean Windows PC for the Release ZIP (`ROAD#1`). This machine has no pad.
- **Calls:** `POL#4`, `POL#5`, `POL#15`, `GAP#1.2`, `GAP#1.3`, `GAP#1.5`, `GAP#8.6`,
  `ENG#CAMERA`.

## Backstory build

**Owner:** [docs/backstory.md](docs/backstory.md#building-it), the Building-it table. Every other
beat is built; the table says which.

| id | status | sev | size | item |
|---|---|---|---|---|
| `STORY#1` | OPEN | MED | S | Integrate the [accepted pop](docs/audio-inventory.md#7-pol22-production-candidates) with `POL#23`: bank entry and tuning swap; the splat still plays the death yell. |
| `STORY#2` | OPEN | LOW | S | Integrate the [accepted sigh/chair](docs/audio-inventory.md#7-pol22-production-candidates) with `POL#23`: bank entries, `match.sounds` slots and leave-beat play calls. |

## Art direction, characters and the UI

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §1, §4 and §6; the 2026-10-09 switch to
Paint.exe Maximalism is not in it yet (`DOC#2`). The pixel-kit questions in the polish sub-docs
are superseded, not open.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#1` | DECISION | HIGH | L | Redo [§1](docs/polish-plan.md#1-establish-the-art-and-display-rules) for Paint.exe: confirm or replace the round-2 picks (Zoom 400% values, Tool Props terrain, airbrush shadows, black keyline, figure variant E with a head accent). Owns the in-world team channel (gap audit 2.3). |
| `POL#2` | DECISION | LOW | S | Retire the superseded workshop kit (the three `PlayerVisualMode`s, F8 cycle, `game/art/polish/kit`, the `check-workshop-*` scripts and cases) or keep it as a frozen study. [Remaining decisions](docs/polish-playable-kit.md#visual-observations-and-remaining-decisions). |
| `POL#7` | BLOCKED | HIGH | XL | Characters in the confirmed style: hands, grips, aim, idle, run, takeoff, air, land, firing, hurt, elimination, and a same-team cue readable at 4P. Waits on `POL#1`. [§4](docs/polish-plan.md#characters-and-distinctive-weapons). |
| `POL#8` | BLOCKED | HIGH | XL | Distinct designs for all five tools, with the secondary-motion table (pressure kick, hose, drum, gauge, slosh, mist tail). Waits on `POL#1`. [§4](docs/polish-plan.md#characters-and-distinctive-weapons). |
| `POL#9` | BLOCKED | HIGH | XL | Material and prop kit for the reference arena, background layers, landmarks, room life; collision silhouettes stay truthful. Waits on `POL#1` and `POL#17`. [§4](docs/polish-plan.md#environment-and-background). |
| `POL#16` | BLOCKED | MED | XL | Carry the style through menus, cards, stage preview, HUD, countdown, pause and results; extend poses and feedback to every weapon and arena. After `POL#1`. [§6](docs/polish-plan.md#6-finish-the-match-presentation-and-extend-the-treatment). |

## Feel and feedback

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §2, §4 and §8, with the
[Movement Course](docs/polish-movement-course.md) and [feedback prototype](docs/polish-feedback.md)
records. Hit feedback (gap audit 5.3) and the "this is you" marker (2.4) live here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#3` | OPEN | HIGH | M | Movement-feel playtest (§2 exit): keyboard/mouse now, a physical pad later (with `ROAD#2`); feedback on and off, aim sweeps, before/after clips. Then freeze jump, acceleration and gravity. [§2](docs/polish-plan.md#2-establish-movement-and-camera-feel). |
| `POL#4` | DECISION | MED | S | Route-trial answers: partial opposite stick braking, released pad aim snapping to facing; accept or fix the short-landing slide-off, 120-wide step walk-off, 60-unit overshoot, leftward drop-through landing. [Qualification](docs/polish-movement-course.md#qualification-still-required). |
| `POL#5` | DECISION | MED | S | Takeoff/landing puff timing and intensity, decided with poses and audio; nearly invisible at 4P 720p. [Values](docs/polish-feedback.md#authored-prototype-values). |
| `POL#10` | OPEN | HIGH | L | The rest of the feedback table: footsteps, skid, painted-surface flecks, hurt flash/flinch, elimination burst, muzzle flash, screen shake, empty-paint cue, menu focus motion. Sounds wait on `POL#23`; the cues are in [audio-inventory §4](docs/audio-inventory.md#4-the-new-cue-list). [§4](docs/polish-plan.md#movement-and-interaction-feedback). |
| `POL#11` | OPEN | HIGH | L | Per-weapon projectile and impact (all five share `projectile_3`), liquid splat shapes, droplets, wet highlight, repaint impact with exact ownership. [§4](docs/polish-plan.md#paint-and-feedback). |
| `POL#12` | OPEN | MED | M | Idle muzzle drip (its five rules), roller contact/settle, idle breathing, blinking and lean out of phase. [§4](docs/polish-plan.md#tool-personality-and-quiet-detail). |
| `POL#13` | OPEN | MED | M | A reduce-effects option that cuts optional shake, flashes and ambient detail but keeps useful cues; repeated-sound variants. [§4](docs/polish-plan.md#detail-across-the-world-interface-and-match). |
| `POL#14` | OPEN | MED | M | Read the tool and the fight: a readiness cue that tells cooldown from no paint, optional aim marker, a readable blocked-shot contact; one player-identity badge across card, figure, pane and results. [§8](docs/polish-plan.md#help-players-read-the-tool-and-the-fight). |
| `POL#15` | DECISION | LOW | M | An optional View arena step after the reveal (unpainted-face highlight, job-card framing): reconcile with the backstory reveal, which already shows the whole picture. [§8](docs/polish-plan.md#make-the-finished-paint-job-worth-looking-at). |
| `POL#18` | OPEN | MED | M | Couch play: a shared count-in after pause or reconnect that freezes every timer and eats stale edges; per-player stick deadzones with a preview; a prominent Rematch on the end menu. [§8](docs/polish-plan.md#make-couch-play-comfortable-between-actions-and-rounds). |
| `POL#19` | DEFERRED | LOW | L | Untimed learning workshop on the Movement Course; deferred until fresh-player observations exist. [§8](docs/polish-plan.md#let-players-learn-by-making-a-mess). |

## Levels and the editor

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §3 and §8 for the layouts;
[docs/level-editor.md](docs/level-editor.md) for the editor.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#17` | BLOCKED | HIGH | XL | Close Quarters reference blockout against the §3 requirements table (routes, spawns, paint budget, weapon roles, pacing, side fairness, recovery pocket). Waits on `POL#3` and `POL#4`. [§3](docs/polish-plan.md#3-redesign-the-levels-in-playable-blockouts). |
| `POL#21` | DEFERRED | MED | XL | King of the Hill and Turbulence blockouts after CQ is accepted; optional 1v1 CQ variant, a brief per layout, side-swap qualification. [Briefs](docs/polish-plan.md#initial-arena-briefs). |
| `ED#1` | OPEN | MED | M | Blockout aids: spawn rectangles from player tuning, a measured movement-envelope guide, exposed scoring faces and area by region matching the runtime buried-face rules. [§8](docs/polish-plan.md#give-the-arena-decisions-that-use-the-existing-rules). |
| `ED#3` | UNSCOPED | LOW | M | Resource names (sheet, frame, icon, colour, music) are checked only at game startup; the editor takes any non-empty name. |
| `ED#4` | UNSCOPED | LOW | S | Registering a stage is manual (`manifest.json` and `cmake/release_assets.cmake`); the editor does not help. |
| `ED#5` | OPEN | LOW | S | The manual GUI pass and the play-test of a saved stage have no recorded result. [Acceptance](docs/level-editor.md#acceptance-checks). |
| `ED#6` | OPEN | LOW | S | Movement Course placeholder: it borrows Turbulence's icon frame; give it its own icon. |

## Audio replacement

**Owner:** [docs/polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set). The
backstory's three sounds are `STORY#1`/`STORY#2`, not here. A silent fresh clone is correct and
never a row.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#23` | OPEN | HIGH | L | Integrate the accepted set: the [dispositions](docs/audio-inventory.md#2-legacy-inventory-replace-or-retire) and every [switchboard site](docs/audio-inventory.md#3-the-switchboard-where-cue-names-live), minding its traps; mix at 1/2/4 players. |
| `POL#26` | OPEN | LOW | S | The legacy countdown recording still runs 2.4 s into play; replace it with separate beats/GO in `POL#23`. Other playback defects are repaired. [Runtime follow-up](docs/audio-inventory.md#8-runtime-defect-follow-up-pol26). |
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
| `POL#25` | OPEN | MED | M | Measure frame pacing against 60 fps on a recorded reference machine and set the hardware baseline. |
| `GAP#7.6` | OPEN | LOW | S | Release identity: no VERSIONINFO, the stock `directx.ico`, no tags, no release job, no CI artifact upload. The ZIP and build stamp are done. |

## Game gaps

**Owner:** [docs/review/gap-audit.md](docs/review/gap-audit.md), Class 1, by its section numbers.
The audit is **frozen** (2026-08-11): it owns each finding and its suggested shape, not its
status, so these rows were checked against the code on 2026-10-13. The no-live-score and
random-pair findings (1.1, 2.1) are Splatoon borrowings and stay closed.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#1.3` | DECISION | MED | L | Solo has no opponent in Release: bots exist only in the Debug console. Ship a bot controller, or keep solo as practice. |
| `GAP#1.2` | DECISION | LOW | L | Only paint-timed rules exist, with no MatchRules seam; decide whether any further mode is wanted. |
| `GAP#1.5` | DECISION | LOW | S | No Random-teams row, no per-player handicap; the one-sided-roster refusal is done. |
| `GAP#3.2` | OPEN | LOW | M | Options: no restore-defaults, no revert after a resolution change, display and palette unreachable from pause, Back lands at the top, Exit quits unconfirmed. |
| `GAP#3.3` | OPEN | LOW | S | No Credits page acknowledging NOTICE's third-party work. |
| `GAP#4.2` | OPEN | MED | L | No action map or rebinding: bindings are constexprs in `player_input.h`/`menu_input.h`. The engine's `gamepad.h` waits on this client (Class 2 D6); `GAP#F2` waits on it. |
| `GAP#4.3` | OPEN | LOW | S | Trigger thresholds, the stick deadzone and the menu direction thresholds are header constants; move them to an input tuning block. |
| `GAP#5.1` | OPEN | MED | L | Hard cuts between screens; the level build blocks on the last menu frame; no music fades or ducking, and results and end screens have no track. |
| `GAP#5.4` | OPEN | LOW | S | No last-ten-seconds clock highlight; the DebugText fps line is `1/dt` of a fixed step, a constant. |
| `GAP#6.1` | OPEN | MED | M | The last match setup is not saved, so every launch walks the whole menu path again. |
| `GAP#6.2` | OPEN | MED | M | Results are never kept: per-stage records, a NEW BEST line, best % on stage select; save sections and a version key go with it. |
| `GAP#7.3` | OPEN | MED | M | One catch in `wWinMain`, captioned "startup failure", catches in-match throws too (`Player out of bounds` among them); no crash log. |
| `GAP#7.5` | OPEN | MED | S | No DPI awareness in the manifest; `_WIN32_WINNT` pinned to 0x0601. The engine half is a report. |
| `GAP#L9` | OPEN | LOW | M | No frame-rate or vsync setting, no render interpolation; `target_fps` is a literal 60. |
| `GAP#L11` | DEFERRED | LOW | L | No locale axis; waits on `GAP#8.4` and `GAP#8.7`. |

## Content pipeline

**Owner:** [docs/review/gap-audit.md](docs/review/gap-audit.md) Theme 8, frozen as above.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#8.6` | DECISION | LOW | S | Dead content is mirrored beside the exe (old textures, unused fonts, `levels/unused`, `.pub`/`.xcf` sources, misspelled `turebulence.pub`, `sounds/unused`). Keep, move to `art/`, or delete; then make staging an allow-list. |
| `GAP#8.2` | OPEN | LOW | M | Range checks are ad hoc: no shared content-value checks or cross-field checks; menu sounds unbounded. |
| `GAP#8.3` | OPEN | LOW | M | The level format has no doc, schema or `format` key. |
| `GAP#8.4` | OPEN | LOW | M | Player-visible text is C++ literals; weapon name and icon come from a switch with an `L"ERROR"` fallback (fold into weapon tuning). |
| `GAP#8.5` | OPEN | LOW | S | The C++/JSON line is uneven: menu widget geometry, 16 `sprite_sheet_1` literals, the wind factor and tool angles in C++. |
| `GAP#8.7` | OPEN | LOW | M | Fonts cannot be regenerated (no recipe for the 7 atlases); no text wrapping. |

## Engine demonstration

**Owner:** [docs/review/gap-audit.md, Class 2](docs/review/gap-audit.md#class-2--gaps-in-the-role-as-the-engines-demonstration),
frozen as above. These add to the hierarchies, never flatten them (CLAUDE.md); D6 is `GAP#4.2`.

| id | status | sev | size | item |
|---|---|---|---|---|
| `GAP#D1` | OPEN | MED | M | Give FocusGroup's per-slot cursors a client: a per-player menu page replacing team/weapon select's hand-rolled cursors; a misfit becomes a report. |
| `GAP#D2` | OPEN | MED | M | No toggle/cycle/slider rows beyond OptionRow, no progress bar, no modal, no list-page base. |
| `GAP#D3` | OPEN | MED | M | Check the engine's anti-tunnelling bound at tuning load and against real geometry; the sniper's 2000 u/s already exceeds it. |
| `GAP#D4` | OPEN | MED | M | Give `Scene::contacts()` a reader (natural home for hit feedback, `POL#10`) and use `labrador::Label` somewhere. |
| `GAP#D5` | OPEN | MED | M | Rotated and moving structures behind the builder, one of each in a shipped stage, so the SAT and moving-response paths get a client. |
| `GAP#D7` | OPEN | LOW | S | `projectile_type` uses SCREAMING enumerators; WeaponType ordering has no static_assert; dead `mine`/`curling`/`water` enumerators. |

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
| `ENG#CAMERA` | DECISION | LOW | S | Should the engine camera own the resolution-independent world height that `PlayerCamera` already fixes at 1080 units here? Only an ask, not a defect. [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `ENG#PARKED` | DEFERRED | LOW | S | Parked asks to ride with another report: the ignored `window_size_changed`, a desktop-size query, an atomic JSON write (F5, F9). [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `GAP#F2` | BLOCKED | LOW | S | The action-map/Gamepads report (per-slot lookup, trigger threshold, "what did this pad press"). Waits on `GAP#4.2`. |
| `GAP#F6` | UNSCOPED | LOW | S | Rumble (`set_vibration`, capabilities): unfiled, no exhibit. The FPS half is report 14. |
| `GAP#F8` | OPEN | MED | S | Draft a ledger report: no install/export/package config or content-staging helper; exhibits are the staging exclude list and `cmake/release_package.cmake`. |
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
| `CI#1` | OPEN | LOW | M | CI runs none of the window-driven checks (`check-debug-console`, `check-movement-feedback`, `check-workshop-*`). |

## Areas with no open rows

A new row for one of these gets its section back.

| area | owner | a row returns with |
|---|---|---|
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
hierarchy", "make it a variant") are misfiled (CLAUDE.md), and a silent fresh clone is correct.
