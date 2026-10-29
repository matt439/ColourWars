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

**Independent implementation (2026-10-29).** The 2026-10-28 shortlists are
complete. Pick OPEN rows by section while attended reviews wait, with
the next-bump preparation (`HYG#24`, `HYG#25`) before any Labrador minor tag.
After each slice, record the next concrete gap in its owner.
This does not change the arena/tuning order above. `ROAD#28`'s OPEN step is a
contract draft only; vertical lifts and growing sponges (`ROAD#29`) do not count
as ready work. Functional trial cues do not widen the deferred production-art pass.

**Bots (`ROAD#10`):** Easy/Normal/Hard setup and the shipping driver are integrated,
including mover painting and pane-clamped labels. Next is the attended difficulty
comparison and physical-pad check; values remain provisional.
[Scope and acceptance](docs/feature-roadmap.md#ai-bot-players-road10).

**Waiting on the author**, whatever the order above. Each is a `DECISION` row below:

- **Turbulence play and selection:** choose among the explicit-only comparisons after
  both-side 1-4P pressure, crossing-comfort and regional-value play (`POL#21`).
- **Press Tower play:** both sides and 1-4P, the summit's value and the halves' feel (`POL#31`);
  the shipping-bot evidence is in its owner.
- **Bots and paint-object trials:** compare Easy/Normal/Hard usefulness (`ROAD#10`)
  and play the explicit Ferry/Gate Trials from both sides at 1-4P (`ROAD#30`).
- **Reference-match review:** Crossfeed is accepted; its spawn pressure, C2 identity/rear-threat
  recognition and 1-4P clean/heavy readability go to the finish review (`POL#24`), with
  five-tool pressure/resource tradeoffs (`POL#27`) and C2/pad movement (`POL#3`).
- **Unborrowed Splatoon mechanics:** paint underfoot and a painting-charged special, each
  yes or no, with or after `POL#27` (`POL#30`). The results awards answered the splat tally.
- **Low-paint warning:** listen to the provisional cue in the 1/2/4P mix and look at the
  HUD tag and figure tank at 4P with sound muted (`POL#38`).
- **Cursor marker:** judge the pointer marquee in play at 1/2/4P (`POL#39`).
- **Practice workshop:** watch a fresh player try Movement Course solo practice, then keep
  or cut it and settle the lesson wording (`POL#19`).
- **Handicaps:** whether Boost +1/+2 feel fair against bots in play (`ROAD#5`).
- **User stages:** play a stage made with Save to My Stages, and decide whether user stages
  may opt into Random, whether the briefing hint is enough of a marker, and the
  provisional scan-details wording (`ROAD#16`).
- **Series stage rule:** play Same stage, Rotate and Loser's pick, choose the default and
  settle the draw, explicit-only and bot-loser calls (`ROAD#20`).
- **Keyboard Keys:** rebind on a keyboard and confirm the slice's provisional calls (`GAP#4.2`).
- **Overview map and match variants:** whether a mid-match map may show territory
  (`ROAD#17`), and whether variants count as modes (`ROAD#22`).
- **Off-screen markers:** whether enemies get threat markers, and a readability look at the
  teammate markers at 1/2/4P (`ROAD#18`).
- **Display policy:** choose the recorded fractional, camera-snap or integer-cell/letterbox
  treatment (`POL#34`); current behavior is documented, with no candidate accepted.
- **Attended reviews:** the editor GUI and saved-stage play pass (`ED#5`), in-game audio
  listening and optional-cue choices (`POL#23`), visible uncontended performance captures
  and a supported hardware baseline (`POL#25`), and startup-window captures for Labrador
  (`GAP#5.1`).
- **Report filing:** the registry-removal draft (`ENG#26`) needs separate author
  authorization. `ENG#23` and `ENG#25` are answered on engine master and need no filing.
- **Hands and hardware:** a physical-pad session (`ROAD#2`, `ROAD#7`, `ROAD#9`, `ROAD#21`, `DBG#7`, `POL#18`, the pad half of `POL#3`) and
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
| `POL#19` | DECISION | LOW | S | Built: menu solo practice on the Movement Course is untimed, with station lessons and a pause Tool row. Watch a fresh player use it, then keep or cut it and settle the lesson wording. [Workshop](docs/polish-movement-course.md#practice-workshop). |
| `POL#38` | DECISION | MED | S | Implemented: HUD tag, figure tank and a once-per-descent cue before exhaustion. Listen to the provisional `tool.drip` cue in the 1/2/4P mix (with `POL#23`) and check tag/tank readability muted at 4P. [Record](docs/polish-weapon-feedback.md#low-paint-advance-warning-pol38). |
| `POL#39` | DECISION | MED | S | A hollow marquee now marks the mouse pointer in matches. Judge it in play over pale, dark and painted scenes at 1/2/4P, including its one-frame trail. [Record](docs/polish-plan.md#mouse-cursor-visibility-pol39). |
| `POL#43` | OPEN | LOW | M | Finish-review detail A/B: `review_finish_details.cfg` takes seeded 4P Crossfeed frames with every detail on, then once per presentation `enabled` flag off (ten flags, restart where the scope needs it), and ends with `tune.reset presentation`. Equal checksums across variants are checked, not assumed. Prepares `POL#24` and accepts nothing. [Finish-review A/B](docs/polish-plan.md#finish-review-detail-ab-script-pol43) |
| `POL#44` | OPEN | LOW | S | `cursor_marker` CPU acceptance case: make `Level::cursor_marker_visible` public (access only) and draw from a pure frame-rectangle accessor. Check each visibility condition, whole-pixel frames with an open centre at 720/1080/1440, and every presentation refusal. The look stays the `POL#39` DECISION. [Cursor-marker case](docs/polish-plan.md#cursor-marker-acceptance-case-pol44) |

## Levels and the editor

**Owner:** [docs/polish-plan.md](docs/polish-plan.md) §3 and §8 for the layouts;
[docs/level-editor.md](docs/level-editor.md) for the editor.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#27` | DECISION | HIGH | L | Attended five-tool balance/resource review remains; the author's acceptance session tried several tools without a complaint. Sniper reserve and Mister lifetime stay provisional hypotheses. [Evidence](docs/polish-crossfeed.md#author-arena-acceptance-2026-10-25). |
| `POL#30` | DECISION | MED | S | Decide paint underfoot (with its no-regen-on-enemy-paint and bot-refill switches) and a painting-charged special, with or after `POL#27`; "no" is a valid answer. The splat tally is answered by the results awards. [Levers](docs/polish-plan.md#decide-which-splatoon-mechanics-to-borrow-pol30). |
| `POL#21` | DECISION | MED | XL | Choose the explicit-only [Turbulence Yard](docs/polish-turbulence.md) geometry after attended both-side 1-4P pressure, crossing-comfort and regional-value play. Scoped software comparisons are qualified; optional CQ work follows selection. |
| `POL#31` | DECISION | MED | M | Attended Press Tower play on both sides and 1-4P: spawn pressure, the summit's value and whether the crown halves feel fiddly. [Open](docs/polish-press-tower.md#open). |
| `ED#5` | DECISION | LOW | S | Record an attended editor GUI pass and play a saved stage; document/hidden-window checks are already implemented. [Acceptance](docs/level-editor.md#acceptance-checks). |
| `POL#42` | OPEN | MED | M | Shipping-bot route case on turbulence_blockout and natural-spawn team trials on the blockout and mover trial, so the variants compare like for like. Evidence for `POL#21`/`POL#27`, not a selection; the slab trial waits on `ROAD#38`. [Bot evidence](docs/polish-turbulence.md#shipping-bot-routes-and-team-trials-pol42) |
| `ED#24` | OPEN | MED | M | Add Edit > Mirror copy (Ctrl+Shift+D): one validated undo step reflected across the out_of_bounds centre, swapping ramp handedness and faces, flipping slab rise, mirroring movers with phase +0.5, mapping spawn A to B and `_a` to `_b`; ferries refused. [Mirror copy](docs/level-editor.md#mirror-copy-across-the-arena-centre-ed24). |
| `ED#25` | OPEN | LOW | M | After `ED#24`: add a symmetry line to the blockout-aids summary listing unmatched mirror partners and left/right scoring area, splitting centre-straddling objects; the Ferry Trial heading is a known authored asymmetry. Reports only. [Symmetry report](docs/level-editor.md#symmetry-report-in-the-blockout-aids-ed25). |
| `ED#26` | OPEN | LOW | M | Show and edit stations in the editor (`EntryKind::station`, named marker, palette entry, validated `name` row and drag, using the shared rules). Lesson blocks stay preserved metadata; places no station. [Station editing](docs/level-editor.md#station-editing-ed26). |
| `ED#27` | OPEN | LOW | M | Add Edit > Copy and Paste of one object between documents through `CF_UNICODETEXT` JSON, appended as one undo step with full validation; the inspector field keeps its own Ctrl+C/V. No multi-selection. [Copy and paste](docs/level-editor.md#copy-and-paste-between-documents-ed27). |

## Audio replacement

**Owner:** [docs/polish-plan.md §5](docs/polish-plan.md#5-replace-the-entire-audio-set). The
accepted source set and live replacement bank are integrated. The remaining cue hooks and
mix still belong here; a missing-bank silent fallback is not a defect.

| id | status | sev | size | item |
|---|---|---|---|---|
| `POL#23` | DECISION | HIGH | L | Listen to the in-game 1/2/4-player mix and decide whether optional detail earns a cue; world detail follows the reference arena. [Remaining work](docs/audio-inventory.md#9-runtime-migration-and-remaining-integration). |
| `ROAD#3` | BLOCKED | MED | M | Replacement-audio acceptance: gains (now Music/SFX sound-group volumes, report 6), mute, suspension, resumed loops, the multiplayer mix, a lost output device; then the audible release and the missing-bank fallback separately. Waits on `POL#23`. |
| `ROAD#36` | OPEN | LOW | S | After the pin passes Labrador `2b3b97b`, device recovery stops persistent voices without an event, and ScreenMusic and the arena loop treat that as finished, leaving music silent. Replay a looping, not-leaving music voice stopped from outside; prove it with a `story_audio` case that stops the effect directly. Listening stays ROAD#3. [Audio inventory](docs/audio-inventory.md#looping-music-after-device-recovery-road36) |

## Acceptance and release

**Owner:** [docs/feature-roadmap.md](docs/feature-roadmap.md#review-follow-up-ammunition-coverage-and-release-checks)
(its numbered hardware list) and [polish-plan §7](docs/polish-plan.md#7-acceptance-and-delivery).
The 2026-09-26 fix docs leave the same hardware checks open; they are frozen, so the rows live
here.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#1` | DECISION | MED | S | Run the extracted Release ZIP on a clean Windows 10/11 x64 machine. Needs one. |
| `ROAD#2` | DECISION | MED | M | Physical-pad sessions: 2P and 4P on every stage and weapon; join, unplug/replug (also into another slot, reclaimed with Start), pause, focus, fullscreen/resize, series draws, rematch, relaunch; a long soak past the display timers; one non-XInput pad. Needs pads. |
| `POL#24` | DECISION | MED | M | The finish review and fresh-player observations on the Crossfeed reference match, including its spawn pressure and 1-4P clean/heavy readability; the gate before production widens. [Process](docs/polish-plan.md#7-acceptance-and-delivery). |
| `POL#25` | DECISION | MED | M | Arrange attended visible, uncontended captures and choose the supported hardware baseline; capture/analysis tooling is implemented. [Acceptance](docs/polish-frame-pacing.md#remaining-acceptance). |
| `GAP#7.6` | DEFERRED | LOW | S | Replace the stock `directx.ico` in the later UI/release-art pass using the approved style; the first tag and any release job are the author's. VERSIONINFO, the ZIP, the build stamp and the CI ZIP artifact are done. |
| `ROAD#37` | OPEN | LOW | M | Record Labrador `PI-14`'s 2026-10-28 Windows Sandbox run of the Release ZIP here with its limits, then add a `.wsb` driver that writes a receipt (ZIP hash, exit code, log). Not a clean physical PC; `ROAD#1` stays DECISION. [Scope](docs/feature-roadmap.md#windows-sandbox-run-of-the-release-zip-road37) |

## Game gaps

**Owner:** [docs/review/gap-audit.md](docs/review/gap-audit.md), Class 1, by its section numbers.
The audit is **frozen** (2026-08-11): it owns each finding and its suggested shape, not its
status. Live implementation scopes are linked in the rows. The no-live-score and
random-pair findings (1.1, 2.1) are Splatoon borrowings and stay closed. The live
[match scope decisions](docs/feature-roadmap.md#match-scope-decisions-2026-10-15) own the
2026-10-15 calls on rulesets, solo practice, random teams and handicaps.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#7` | DECISION | LOW | S | Review the per-pad View shake, Aim marker and Reduce effects overrides on 1-4P panes with pads; the settings, save and CPU checks are complete. Needs `ROAD#2`. [Scope](docs/feature-roadmap.md#per-seat-accessibility-toggles-road7). |
| `ROAD#5` | DECISION | LOW | S | Per-seat Boost (Off/+1/+2: less damage taken, faster paint refill) is built on Weapon Select (mechanic chosen 2026-10-27); judge the provisional values in attended play against bots. [Scope](docs/feature-roadmap.md#handicaps). |
| `ROAD#9` | DECISION | MED | M | Review the four candidate rumble cues on physical pads, including comfort and distinctness; owner, settings and fake-sink qualification are complete. Needs `ROAD#2`. [Scope](docs/feature-roadmap.md#controller-rumble-road9). |
| `ROAD#10` | DECISION | HIGH | M | Compare Easy/Normal/Hard usefulness in attended play and check physical pads. Mover painting and pane-clamped labels are implemented; difficulty values remain provisional. [Scope](docs/feature-roadmap.md#ai-bot-players-road10). |
| `GAP#4.2` | DECISION | LOW | S | Keyboard Keys rebinds Move, Jump, Drop, Shoot (key or click) and an extra Pause; pad buttons are done. Left: try it and confirm the provisional calls (refuse not swap, clicks only for Shoot, fixed Esc/Down, A-Z/Space). [Owner](docs/feature-roadmap.md#keyboard-actions-slice-gap42). |
| `GAP#5.1` | DECISION | LOW | M | Blank startup window: capture a visible Release launch to first present, and a hidden one shown natively, for Labrador ([report 19](docs/review/engine-reports.md#19-no-public-later-show-convenience-for-a-hidden-application--needs-more-evidence-cacf112)). Hard cuts are a later UI-art slice. |
| `GAP#L9` | OPEN | LOW | S | Record the inspection at the pin and Labrador master: `Present(1, 0)` and Vulkan FIFO, `target_fps` only, no client `step_position()` caller; add an `overlay.perf` receipt at this display's 59 Hz. Then set DEFERRED with its condition or split the row. No refresh-rate policy choice. [Slice](docs/feature-roadmap.md#presentation-rate-inspection-slice-gapl9) |
| `GAP#L11` | UNSCOPED | LOW | L | Define locale selection, fallback, font coverage and numeric conventions separately; text extraction is complete. [Boundary](docs/menu-presentation.md#remaining-text-slices-gap84), [font recipe](game/art/README.md#sprite-font-recipe-gap87). |
| `ROAD#38` | OPEN | MED | L | Bots treat fixed scored slabs as solid boxes. Measure first: a `bot_slab_routes` case on `turbulence_slab_trial` counts slab-grounded steps, slab-top paint and crossover arrivals against the mover trial; add a slab-aware standing segment only if the counts are near zero. [Scope](docs/feature-roadmap.md#fixed-scored-slabs-for-bots-road38) |
| `ROAD#39` | OPEN | LOW | S | Cover Boost on paint underfoot on a test tuning copy: own and enemy multipliers times `paint_refill` at every level (0 stays 0), off-paint and dead cases, and a boosted bot's observed `paint_refill`. Off stays bit-exact; no tuning change. [Scope](docs/feature-roadmap.md#boost-on-paint-underfoot-road39) |

## Comparable-game gaps

**Owner:** [docs/feature-roadmap.md, Comparable-game gaps](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). Features that
Splatoon and couch party shooters have and ColourWars lacks, rated medium value
or above (2026-10-27). The owner records the gaps left out by choice and the
excluded modes.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ROAD#11` | BLOCKED | MED | L | Sub weapons: a second, paint-hungry tool per player (Splatoon subs); `Player` already takes and ignores a secondary. Waits on `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#13` | BLOCKED | LOW | XL | More tool archetypes (Splatoon brush, blaster, splatling, dualies, brella), each with art, audio and bot ballistics. Waits on `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#14` | BLOCKED | LOW | L | Map pickups or power-ups (TowerFall chests, Smash items), behind a match toggle and with bot awareness. Waits on `POL#27`. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#28` | OPEN | MED | M | Vertical lifts need carry, ceiling/crush, admission and sweep contracts. The step is a contract draft: a file:line inventory of horizontal-only sites, engineering answers following existing practice, gameplay calls as author options. The capability stays unscoped until the owner accepts the draft. [Draft](docs/feature-roadmap.md#vertical-lift-contract-draft-road28) |
| `ROAD#29` | UNSCOPED | MED | XL | Growing sponges need occupied resize, changing-area scoring and paint-credit contracts before implementation. [Boundary](docs/feature-roadmap.md#unscoped-paint-object-contracts-road28-road29). |
| `ROAD#30` | DECISION | MED | M | Play the Ferry/Gate Trials from both sides at 1-4P; judge route value, held-open clarity and provisional values. Arena selection remains `POL#21`. [Scope](docs/feature-roadmap.md#paint-object-trial-play-road30). |
| `ROAD#16` | DECISION | LOW | S | User stages and scan details are implemented. Left: attended saved-stage play, Random opt-in and hint-only marker choices, and provisional scan-details wording. [Owner](docs/feature-roadmap.md#user-stages-road16). |
| `ROAD#17` | DECISION | LOW | M | Overview map in the unused three-player quadrant (Splatoon turf map). Shows territory mid-match, against gap audit 1.1: decide first. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
| `ROAD#18` | DECISION | LOW | S | Off-screen teammate markers are in. Left: decide whether enemies get threat markers (an information-model change; see `POL#3` rear threats), and an attended readability look at 1/2/4P. [Owner](docs/feature-roadmap.md#teammate-markers-road18). |
| `ROAD#20` | DECISION | LOW | S | The Series stage rule is in (Same stage, Rotate, Loser's pick on Mode). Left: play the three, choose the default, and confirm the provisional calls for draws, explicit-only stages and bot-only losers. [Owner](docs/feature-roadmap.md#series-stage-rule-road20). |
| `ROAD#21` | DECISION | LOW | S | Player profiles are in and their MattPC painter checks pass. Left: try profiles with real pads (`ROAD#2`). [Owner](docs/feature-roadmap.md#player-profiles-road21). |
| `ROAD#22` | DECISION | LOW | M | Match variants (low gravity, endless paint, tool roulette), mostly tuning overrides. Decide whether they count as modes, which are ruled out. [Gap](docs/feature-roadmap.md#comparable-game-gaps-2026-10-27). |
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
`GAP#F*`. `ENG#23` and `ENG#25` are answered on untagged master and wait on the next
minor tag; `ENG#26` remains an unfiled draft. Minimal engine-focused reports with
reproducible examples are approved; recheck upstream and prepare each report for
Labrador's established issue route. These rows do not represent fixes already requested.

| id | status | sev | size | item |
|---|---|---|---|---|
| `ENG#PARKED` | DEFERRED | LOW | S | Asks to ride with a report: ignored `window_size_changed`, desktop-size query, atomic JSON write (F5, F9); install facts (PI-11; report 15); listener/voice cap (report 6); mouse-button names; display-awake owner; non-XInput reader. Device loss answered on master (`2b3b97b`). [Not here](docs/review/engine-reports.md#what-is-deliberately-not-here). |
| `ENG#25` | BLOCKED | LOW | S | Answered, narrowed, on untagged master by `cadc962` (`Gamepads::vibrate`; not in v0.2.0); no filing needed. Waits on the next Labrador minor tag, then replace the native XInput transport and close the report. [Report](docs/review/engine-reports.md#25-no-public-controller-vibration-output---draft-with-isolated-adapter-exhibit) |
| `ENG#26` | DECISION | LOW | S | Rechecked 2026-10-29 at master `b96bad6`: `Registry` still cannot remove a name (`3a14f11` changed comments only). The ask narrows to forgetting the name, since `add(name, nullptr)` already empties a slot. Author authorization is needed to file. [Report](docs/review/engine-reports.md#26-registry-cannot-forget-a-name---draft-with-rescan-exhibit) |
| `ENG#23` | BLOCKED | LOW | S | Answered on untagged engine master by `42bc478` (not in v0.2.0); filing no longer needed. Waits on the next Labrador minor tag, then delete `SettingsProbeActivationGuard` and its five uses and close the report. [Report](docs/review/engine-reports.md#23-resizing-a-hidden-window-activates-it--draft-with-activation-guard-exhibit) |
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
| `TOOL#3` | OPEN | MED | M | Give the four Turbulence trials the blockout's region bands, then rerun the 25-pair x 5-seed five-tool matrix at Hard on Crossfeed, Press Tower and the Turbulence family, and file per-band held paint as descriptive evidence for `POL#21`/`POL#27`. [Turbulence bands and rerun](docs/gameplay-tuning.md#turbulence-bands-and-hard-five-tool-rerun-tool3-2026-10-29) |
| `CI#2` | OPEN | MED | M | Add a CI compile gate in both presets: `run-acceptance -SkipBuild -CompileOnly` over the 14 opt-in cases, a new `check-painter -CompileOnly` (both pad readers) and `check-frame-pacing -CompileOnly`; no probe or GPU. Hosted run waits for an authorized push. [Owner](docs/code-hygiene.md#compile-gate-for-opt-in-probes-ci2) |
| `CI#3` | OPEN | LOW | M | Time `check-stage-registration` and `check-failure-reporting` with `-SkipBuild` in both presets (duration, focus, dialogs), then add CI steps with evidence upload or record each exclusion in its owner doc. [Owner](docs/code-hygiene.md#registration-and-failure-reporting-checks-in-ci-ci3) |
| `CI#4` | OPEN | LOW | S | Have `run-acceptance` write `acceptance-summary.json` (per-case compile and run seconds, outcome, longest first) and upload it from CI, so `$longestFirst` and the Debug PR exclusions rest on data. [Owner](docs/code-hygiene.md#acceptance-timing-summary-ci4) |
| `CI#5` | OPEN | LOW | S | Add `scripts/ci/check_doc_links.py` with tests to the always-on lint job (allowing `out/`, historical docs, `#` in file names) and repoint the two broken `#retained-route-trials` anchors. [Owner](docs/code-hygiene.md#local-markdown-link-check-ci5) |

## Code hygiene and maintenance

**Owner:** [docs/code-hygiene.md](docs/code-hygiene.md), its [Open shortlist (2026-10-29)](docs/code-hygiene.md#open-shortlist-2026-10-29) and, for the next-bump preparation, the [engine-report ledger](docs/review/engine-reports.md).

| id | status | sev | size | item |
|---|---|---|---|---|
| `HYG#24` | OPEN | MED | M | Trial-build against engine master in a scratch worktree (pin stays) and record each break class with error counts: mattmath namespace and MattMath.lib script paths, constant renames, removed members, the RectangleI include, removed types, /utf-8. [Next bump](docs/review/engine-reports.md#trial-build-against-engine-master-hyg24) |
| `HYG#25` | OPEN | MED | L | After `HYG#24`: copy the seven types `730dd99` deletes (MovingObject, SoundBankObject, AnimationObject, CameraTools, RotationOrigin, TextDropShadow, UiTextDropShadow) into game/, shape unchanged; record the rungs that move. Never flatten. [Next bump](docs/review/engine-reports.md#carry-the-seven-removed-engine-types-into-the-client-hyg25) |
| `HYG#26` | OPEN | LOW | S | Rename-only commit: `Level` parameters named `view` that index roster participants become `participant`; `view` stays only where a human pane is indexed. No behaviour change. [Owner](docs/code-hygiene.md#participant-parameter-names-in-level-hyg26) |

## The OOP showcase

**Owner:** [docs/oop-guide.md](docs/oop-guide.md). These describe the hierarchies as they are; never flatten them (AGENTS.md).

| id | status | sev | size | item |
|---|---|---|---|---|
| `OOP#1` | OPEN | MED | M | Bring the guide up to date: new projectile kinds go before the `ProjectileType::count` sentinel; add `TeamGateStructurePaintable`, `TranslatingStructurePaintable`, `FixedSlabPaintable` (Structure + PaintableObject) and the `CrudeDrawing` family; add an "add a scored paint object" checklist. Describe the classes as they are; don't flatten them. [Guide drift](docs/oop-guide.md#guide-drift-since-the-last-pass-oop1) |

## Design process

**Owner:** [docs/design-process.md](docs/design-process.md): personas, user stories and journeys, opened 2026-10-29 at the author's request.

| id | status | sev | size | item |
|---|---|---|---|---|
| `DES#1` | OPEN | MED | M | Draft 3-6 personas, user stories with acceptance criteria, and journeys (first launch, a returning group's evening, solo practice, user stages), each citing existing docs/code or marked a hypothesis and mapped to what serves it. The author's acceptance follows as a DECISION. [Draft](docs/design-process.md#draft-personas-stories-and-journeys-des1) |
| `DES#2` | BLOCKED | LOW | M | Waits on `DES#1`: trace stories to features, rows and acceptance cases; file unserved stories as candidates in their owners and list features no story explains. [Traceability](docs/design-process.md#traceability-and-gap-pass-des2) |

## Areas with no open rows

A new row for one of these gets its section back.

| area | owner | a row returns with |
|---|---|---|
| Backstory build | [backstory.md, Building it](docs/backstory.md#building-it) | a missing or regressed story beat; pop, sigh and chair integration are complete |
| Closed engine reports (1–22, 24) | [engine-reports.md, Closed](docs/review/engine-reports.md#closed) | a pin bump that answers a live report and leaves a workaround to delete |
| Feature-roadmap increments 1–5 | [feature-roadmap.md](docs/feature-roadmap.md) | a sixth increment, or a regression from the `ROAD#2` session |
| 2026-09-26 review fixes | `docs/review/2026-09-26-*.md` (frozen) | a new review round; the hardware residue is `ROAD#2` |
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
