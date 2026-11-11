# Documentation

What is in `docs/`, grouped by what you would come for. Open work is indexed in
[TODO.md](../TODO.md), and each row there points to the doc that owns it. The
[README](../README.md) covers playing and troubleshooting. The
[development guide](development.md) covers building and checks, and the
[release guide](releasing.md) covers preparing and publishing a Release ZIP.

## Players

| Doc | What |
|---|---|
| [HOW-TO-PLAY.txt](HOW-TO-PLAY.txt) | extracting and starting the game, controllers and bots, controls, settings and removal; also included beside the executable |
| [If something goes wrong](../README.md#if-something-goes-wrong) | help with startup, downloads, controllers and building; where to report an error |
| [level-editor.md](level-editor.md), [level-format.md](level-format.md) | building and sharing a stage of your own; the editor currently needs a source build |
| [backstory.md](backstory.md) | why the arena is an unsaved picture in a paint program |

Everything below is maintainer material: code, development tools, design
records and reviews. The open-work index is [TODO.md](../TODO.md).

## Reading the code

| Doc | What |
|---|---|
| [CONTRIBUTING.md](../CONTRIBUTING.md) | contribution workflow, the game/engine boundary and reporting a bug |
| [architecture.md](architecture.md) | the directory map, dependency direction, services, state stack, builders and startup data |
| [oop-guide.md](oop-guide.md) | the class hierarchies as an OOP example, one shot from input to score, adding a weapon or a scored object |
| [failure-reporting.md](failure-reporting.md) | how `wWinMain` reports an escaping exception |
| [code-hygiene.md](code-hygiene.md) | owner of the `HYG` and `CI` maintenance rows, including the architecture review's |

## Using the tools

| Doc | What |
|---|---|
| [development.md](development.md) | source builds, acceptance, packaging, engine pin, audio and repository history |
| [releasing.md](releasing.md) | the current release-preparation scope, clean candidate checks and publication gates |
| [debug-console.md](debug-console.md) | the Debug-only console, debug menu, launch arguments, live tuning, bots, record and play, scripts: the reference |
| [debug-console-record.md](debug-console-record.md) | how each `DBG` row was done or why it waits, and `CI#1` |
| [level-editor.md](level-editor.md) | `ColourWarsEditor.exe`, the desktop level editor |
| [level-format.md](level-format.md), [level.schema.json](../game/contracts/level.schema.json) | the level file format and its schema |
| [gameplay-tuning.md](gameplay-tuning.md) | `tuning.json`: its key rules, tool angles and accepted ranges, the headless balance runner (`TOOL#1`-`TOOL#3`) and the two-bot team measurement (`REV#22`) |
| [menu-presentation.md](menu-presentation.md) | `presentation.json`'s menu geometry, artwork and text |
| [localisation.md](localisation.md) | locales: the overlay files, what they may change, fallback, font coverage and numbers |

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
| [drawing-mistakes-research.md](drawing-mistakes-research.md) | whether the art should carry deliberate mouse-drawing mistakes, and the human tests that decide which |

## Arenas

| Doc | What |
|---|---|
| [polish-crossfeed.md](polish-crossfeed.md) | Crossfeed, the accepted reference arena |
| [polish-press-tower.md](polish-press-tower.md) | Press Tower, the accepted King of the Hill arena |
| [polish-turbulence.md](polish-turbulence.md) | Turbulence, its accepted slab trial and the mover, gate, ferry and lift comparisons |
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
[2026-11-05 code hygiene review](review/2026-11-05-code-hygiene-review.md), the
[2026-11-10 project review](review/2026-11-10-project-review.md), which owns the
`REV` rows, the
[2026-11-10 first-run review](review/2026-11-10-first-run-review.md), which owns the `FTR` rows,
the three 2026-09-26 fix records and the
[hidden-window-resize](review/hidden-window-resize/README.md) request and exhibit (`ENG#23`).

`images/` holds the screenshots and clips the docs embed.
