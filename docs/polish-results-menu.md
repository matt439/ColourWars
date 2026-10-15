# Paint.exe results page

Implemented on 2026-10-15, part of `POL#16`. Results now use the same pale
stepped paper, dark ink, black keyline and quiet edge shade as the end and
pause pages. The team-labelled percentage plates share that drawing. The
heading, winner/coverage line, series ledger and continuation prompt center
from actual font measurements; contribution rows retain their roster badges
and device labels. The brand line stays hidden above the ended match.

`MenuPaperArtwork` in `game/states/menu_panel.cpp` draws the panel and score
plates from the existing solid texel. The bar uses plain rectangles with a
dark border and a neutral unpainted track. Paper leaves point filtering active
for the texture layer: linear sampling of the one-texel frame picked up
neighbouring atlas colours at the bar's edges. The page selects linear
filtering separately for text. The obsolete `MenuPanel` and the three legacy
results-frame resource requirements are removed.

Editable drawing values live in `presentation.json.results_menu.art`, with
the neutral track in `fill.track_colour`. Centered text uses the named `*_y`
positions; contributions, score geometry and timing remain in the same block.
Normal builds reproduce the drawing without an atlas-generation step.

The page retains the existing scoring and flow:

- Competitive bars show each team's share of painted surface. Solo practice
  shows actual arena coverage, with no bar, mark or percentage for an absent
  team. Player contributions remain percentages of the whole arena, credited
  by input owner rather than roster position.
- Ordinary results fill before revealing numbers, contribution rows and the
  winner/coverage line. A match whose story already revealed the result keeps
  the bar hidden and omits the repeated winner line; draws retain their line.
- Series keep their round number, win ledger and continuation/completion text.
  Enter/A proceeds only after the existing delay, then the match opens its
  existing end menu. Escape does not bypass the results page.
- The drawing is static, including under Reduce effects. Audio bindings and
  relative gains retain their existing behavior.

## Validation

Debug and Release build under `/W4 /WX` and each pass all 22 consumer
acceptance cases. Content variants check the paper fields, bounded positive
geometry, text ordering, fill borders and track colour, plus reveal-time
boundaries. The D3D11 hardware matrix passes in both configurations on the
NVIDIA GeForce RTX 5080, checking:

- Six page states at 720p, 1080p, QHD, 4:3, 16:10 and ultrawide. Measured text,
  score plates, contribution rows and badges fit within the paper; native
  lettering retains dark-ink/pale-paper contrast.
- Competitive share and solo coverage bar widths, all three supported
  palettes at 720p, absent-team suppression, neutral solo track pixels and
  no repeated score bar after the story reveal.
- Sparse, reordered input-owner credits: changing one owner's score changes
  only that roster row. An unpainted competitive draw keeps its even split.
- Partial fill advances within the score track; early Enter and Escape cannot
  proceed, while fresh Enter after the authored gate closes the page. The
  retained partial-fill capture shows the numbers and details still hidden.
- Repeated frozen drawing, draw randomness, static Reduce effects and exact
  resize round trips.

The shared-paper pause/confirmation and end-menu hardware matrices also pass
in both builds after the filtering fix. Their actual keyboard navigation,
disabled and safe-default actions, frozen/Reduce effects drawing and resize
checks retain their existing acceptance coverage. Evidence is in
`regression/<preset>/` below the results evidence root.

The retained native 720p views show [already-revealed scores](
images/polish-results-menu/revealed-720p.png), [a completed series](
images/polish-results-menu/series-720p.png), [solo arena coverage](
images/polish-results-menu/solo-720p.png) and [the partial fill before numbers](
images/polish-results-menu/fill-720p.png). The page probe uses a plain backing
to isolate its drawing; these are actual `ResultsMenuInitial` pages, driven
through `StateContext` and public keyboard input.

The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte,
including the unchanged accepted bank. Isolated Debug and extracted Release
startup pass with a system-only PATH and graceful shutdown. A separate
extracted copy passes without its optional bank, with the expected single
silent-fallback diagnostic. No player's save is opened.

Evidence is under `out/acceptance/` and `out/validation/results-menu/`, with
build/acceptance/visual logs in `out/validation/results-menu-*.log`. Successful
results runs are Debug `179e84522b594122a139aeb60bf517a8` and Release
`259b04e3eec14cc7ac272ab924945b19`. Package/startup evidence is in
`package-0ffeabbdc4354ea2ac1ff62b951b84e9/` below that evidence root.

Rerun from an x64 Visual Studio Developer shell:

```powershell
scripts/run-acceptance.ps1 -Preset x64-debug
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -ResultsMenuOnly
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -PauseMenuOnly
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -EndMenuOnly
```

Use `x64-release` for the Release checks. Remaining menus, selection cards and
stage preview stay open under `POL#16`. Reference-arena composition, movement
feel, physical controllers and listening acceptance retain their TODO owners.
