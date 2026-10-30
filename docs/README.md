# Documentation

What is in `docs/`, grouped by what you would come for. Open work is indexed in
[TODO.md](../TODO.md), and each row there points to the doc that owns it. The
[README](../README.md) covers building, running and the release ZIP.

## Reading the code

| Doc | What |
|---|---|
| [architecture.md](architecture.md) | the directory map, dependency direction, services, state stack, builders and startup data |
| [oop-guide.md](oop-guide.md) | the class hierarchies as an OOP example, one shot from input to score, adding a weapon or a scored object |
| [failure-reporting.md](failure-reporting.md) | how `wWinMain` reports an escaping exception |
| [code-hygiene.md](code-hygiene.md) | owner of the `HYG` and `CI` maintenance rows, including the architecture review's |

## Using the tools

| Doc | What |
|---|---|
| [HOW-TO-PLAY.txt](HOW-TO-PLAY.txt) | the player's guide shipped at the root of the Release ZIP: extracting, SmartScreen, joining and bots, controls, settings, removal |
| [debug-console.md](debug-console.md) | the Debug-only console, debug menu, launch arguments, live tuning, bots, record and play, scripts: the reference |
| [debug-console-record.md](debug-console-record.md) | how each `DBG` row was done or why it waits, and `CI#1` |
| [level-editor.md](level-editor.md) | `ColourWarsEditor.exe`, the desktop level editor |
| [level-format.md](level-format.md), [level.schema.json](../game/level.schema.json) | the level file format and its schema |
| [gameplay-tuning.md](gameplay-tuning.md) | `tuning.json`: what each block controls |
| [menu-presentation.md](menu-presentation.md) | `presentation.json`'s menu geometry, artwork and text |

## Design and direction

| Doc | What |
|---|---|
| [feature-roadmap.md](feature-roadmap.md) | features, release and hardware acceptance; scopes most `ROAD` rows |
| [polish-plan.md](polish-plan.md) | the presentation, game-feel, audio and arena plan and its accepted art rules; scopes the `POL` rows |
| [design-process.md](design-process.md) | personas, user stories and journeys (`DES` rows) |
| [backstory.md](backstory.md) | the fiction: a grey, unsaved picture with one palette slot |
| [audio-inventory.md](audio-inventory.md) | the sound inventory and direction behind the replacement bank |
| [level-design-research.md](level-design-research.md) | combat-map research that led to Crossfeed |
| [classic-paint-research.md](classic-paint-research.md) | what the old Microsoft paint programs could draw, for the art rules |

## Arenas

| Doc | What |
|---|---|
| [polish-crossfeed.md](polish-crossfeed.md) | Crossfeed, the accepted reference arena |
| [polish-press-tower.md](polish-press-tower.md) | Press Tower, the King of the Hill blockout |
| [polish-turbulence.md](polish-turbulence.md) | Turbulence and its mover, slab, ferry and gate trials |
| [polish-close-quarters.md](polish-close-quarters.md) | Close Quarters, the first playable blockout |
| [polish-movement-course.md](polish-movement-course.md) | the Movement Course and its stations |

## Polish increments

Each records one implemented presentation or feel increment: what changed,
how it was checked and what is still open.

| Area | Docs |
|---|---|
| Camera, movement and frame pacing | [foundation](polish-foundation.md), [movement feedback](polish-feedback.md), [movement contact](polish-movement-contact.md), [view shake](polish-view-shake.md), [frame pacing](polish-frame-pacing.md), [reduced effects](polish-reduced-effects.md) |
| Figures, tools and paint | [painter](polish-painter.md), [tools](polish-tools.md), [paint effects](polish-paint-effects.md), [weapon feedback](polish-weapon-feedback.md), [combat feedback](polish-combat-feedback.md), [blocked shots](polish-blocked-shots.md), [aim marker](polish-aim-marker.md), [terrain](polish-terrain.md) |
| Match presentation | [HUD](polish-hud.md), [countdown](polish-countdown.md), [monitor scenes](polish-monitor.md), [closing and reveal accents](polish-match-accents.md) |
| Menus | [main menu](polish-main-menu.md), [pause menu](polish-pause-menu.md), [results](polish-results-menu.md) |
| Retired workshop studies | [style candidate](polish-style-reference.md), [native-pixel kit](polish-style-kit.md), [playable comparison](polish-playable-kit.md), [refinement](polish-refine.md) |

## Reviews

[review/](review/) holds records, frozen as written, except
[engine-reports.md](review/engine-reports.md), the live ledger of reports this
client owes Labrador, and [gap-audit.md](review/gap-audit.md), the 2026-08-11
audit of missing functionality that still owns the `GAP` rows. The rest are the
[2026-10-30 architecture review](review/2026-10-30-architecture-review.md), the
three 2026-09-26 fix records and the
[hidden-window-resize](review/hidden-window-resize/README.md) request and exhibit (`ENG#23`).

`images/` holds the screenshots and clips the docs embed.
