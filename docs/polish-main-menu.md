# Paint.exe title and home pages

Implemented on 2026-10-15, part of `POL#16`. The title and home pages now use
the same pale stepped paper, dark ink, black keyline and quiet edge shade as
the match menus. A solid neutral surround replaces the textured backdrop on
these two pages. The title, subtitle, start action and build label center from
actual font measurements. Home keeps its existing heading and four actions;
Play has a persistent paper card, and a detached ink arrow follows focus.

`MenuPaperArtwork` draws both panels and their action cards from the existing
solid texel. The paper belongs to the texture layer, with point sampling;
lettering retains the text layer's linear sampling. These pages suppress the
shared brand line and legacy focus backing while retaining the common controls
footer. Other main-menu pages keep their existing artwork until their own pass.

Editable colours, pen width, shade and card padding live in
`presentation.json.main_menu.art`. `title_box_size` and `home_box_size` set the
panels centered above the shared controls footer. The title's `*_y` values set
its vertical rhythm; `home_position`, `home_spacing` and
`home_subtitle_position` set Home's layout.
Startup rejects malformed paper, unbounded geometry and anchors outside their
panels. Actual font and card fitting is checked with the renderer. Normal
builds reproduce the drawing without an atlas-generation step.

The drawing is static, including under Reduce effects. Existing copy,
keyboard/pad prompts, title Start, Home Back, Play, Options, Credits and Exit
retain their behavior, as do music and effects bindings and gains. The keyboard
seat continues to make the title's controller hint unnecessary; its authored
position and font bounds are retained for that existing hint.

## Validation

Debug and Release build under `/W4 /WX` and each pass all 22 consumer
acceptance cases. Content variants reject malformed paper, invalid panel and
footer geometry, missing/nonfinite anchors and overlapping anchor order, while
retaining legal panel boundaries. The D3D11 hardware matrix passes in both
configurations on the NVIDIA GeForce RTX 5080, checking the actual pages
through `StateContext` and public keyboard input:

- Title Start, Home Back, all four Home focus cards/arrows, Play/Options/Credits
  destinations and Exit closing only the probe's own hidden window. Navigation
  supplies the existing neutral input required to arm each new page.
- Measured lettering and card separation at 720p, 1080p, QHD, 4:3, 16:10 and
  ultrawide, including the panel and build label clearing the controls footer.
- Native ink/paper contrast, independent comparisons against forced-linear
  title/Home lettering, and a forced-point negative control at 720p.
- Frozen redraws, draw randomness, static Reduce effects and exact resize
  round trips without changing focus.

The retained native 720p views show the [title](images/polish-main-menu/title-720p.png),
[Home with Play focused](images/polish-main-menu/home-720p.png) and
[Home with Options focused](images/polish-main-menu/options-720p.png).

The 53-file Release ZIP contains all 26 reviewed runtime assets byte for byte,
including the unchanged accepted bank. Isolated Debug and extracted Release
startup pass with a system-only PATH and graceful shutdown. A separate
extracted copy passes without its optional bank, with the expected single
silent-fallback diagnostic. No player's save is opened.

Evidence is under `out/acceptance/` and `out/validation/main-menu/`, with build,
acceptance and visual logs in `out/validation/main-menu-*.log`. Successful
renderer runs are Debug `01bdcf75043b4130909a578d8a9cc0e3` and Release
`429ebc53b8d84633a991ef3e7eeaa636`. Package evidence is in
`package-9105a56f9af248f8ba33f0f2143709a3/` below that evidence root. These scripted
checks and native image inspection do not establish physical-controller,
human movement/readability or listening acceptance.

Rerun from an x64 Visual Studio Developer shell:

```powershell
scripts/run-acceptance.ps1 -Preset x64-debug
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -MainMenuOnly
```

Use `x64-release` for the Release checks. Options/Credits, match setup,
selection cards and stage preview remain under `POL#16`. Reference-arena
composition, movement feel, physical controllers and listening acceptance
retain their TODO owners.
