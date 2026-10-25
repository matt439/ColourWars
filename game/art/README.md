# Art sources and runtime content

The accepted Paint.exe art rules are in the
[polish plan](../../docs/polish-plan.md#1-establish-the-art-and-display-rules).
The author's [accepted round-2 study](references/style-board/index.html)
([original artifact](https://claude.ai/artifact/9J5kQgRMvVtXPe6r6cJ7vP))
is the reference for Zoom 400% values, Tool Props terrain, airbrush shadows,
black paint keylines and figure E with teammate head accents; the figure has
since been rebuilt from parts with no teammate mark ([painter](painter/README.md)).
The complete export, including all five rounds of JSON and 112 local images,
is retained unchanged in `references/style-board/`. Serve that directory over
HTTP to use the board's JSON-backed tabs; direct image files remain viewable
without running the board. The export still requests its display fonts from
Google Fonts. Later-round proposals in the export do not change the accepted
choices recorded in the polish plan.
The workshop kit, comparison modes, atlas and dedicated probes were retired on
2026-10-15. Its historical concept image and prompt remain in `polish/`; the
[study records](../../docs/polish-playable-kit.md) and Git history retain the
review evidence. The production painter and five-tool atlas below now implement
the accepted drawing language; composition review remains in the polish plan.

## Painter artwork

The [painter source](painter/README.md) begins the accepted gameplay character
pass. Its editable coarse-cell drawing, deterministic System.Drawing/texconv
recipe and all-palette pose preview live outside runtime content. Rebuild it with
`powershell -ExecutionPolicy Bypass -File scripts/build-painter-art.ps1`.
Only `game/content/textures/painter.{json,dds}` is staged and packaged.
The [implementation and review record](../../docs/polish-painter.md) states
what this increment covers and which character work remains open.

## Weapon artwork

The [five-tool source](weapons/README.md) is the editable Paint.exe production
drawing: integer vectors, neutral hardware and explicit team-paint materials.
It supplies a pressure sprayer, precision nozzle, round decorator's roller,
garden atomiser and dented paint tin. Each has idle, two motion and lifted poses.
The original image-generated `weapon_paint_source.png` and
`weapon_paint_prompt.txt` remain historical authoring material.

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-weapon-art.ps1
```

This rasterises and packs the five tools, copies the legacy
utility frames from `sources/textures/sprite_sheet_1.png`, and regenerates
`sources/textures/weapon_paint.png` here and
`game/content/textures/weapon_paint.{json,dds}`, plus the authoring
[lineup](weapons/lineup.png). This uses Windows System.Drawing and the retained
`tools/texconv.exe`.
No image service or Python dependency is needed to reproduce the runtime atlas.

Gameplay frames have four texels per world unit. Their aim pivots are at the
left centre in source texels; the source canvases align the barrels to that line
and the roller drum with its existing paint contact. Gameplay dimensions,
offsets, ammunition and projectile behaviour remain in `tuning.json`.

The packer separates exact paint-material IDs from neutral hardware.
Every pose has a base frame and a grayscale `<frame>_paint` overlay with
identical dimensions and origin. The overlay is tinted with the player's actual
team colour at draw time. The roller only colours its sleeve while firing;
its arm and handle stay neutral. Fixed-palette menu previews use the same team
colours, while Random stays neutral until the match chooses its palette.

The sprayer and mister reservoirs additionally use full-canvas supply strips,
with neutral empty interiors in the body. Bottom-filled liquid rows follow
actual ammunition; permanent wet nozzles retain their team tint. The
[supply record](../../docs/polish-tools.md#reservoir-paint-supply) describes
their quantity levels, native-size evidence and reproduction checks.

Complementary layer alpha preserves the authored silhouette. Nozzles, triggers,
gauges and hardware stay neutral outside the explicit wet/team regions.
The [tool implementation and review record](../../docs/polish-tools.md) describes
the motion, fixed grips, preserved gameplay and renderer evidence.

The PNG uses straight alpha. The DDS is RGBA8 with premultiplied alpha and one
mip level, as required by Labrador's sprite blend. Do not replace it with a
straight-alpha DDS. The packer omits colour-space metadata from the runtime PNG
so the bundled texconv preserves its channel values while premultiplying for
the engine's UNORM pipeline. Runtime packaging includes only the DDS and frame
JSON.

## Projectile and paint artwork

The [effects source and recipe](paint-fx/README.md) supply distinct liquid shot
and surface-impact strips for all five tools, including the roller's contact
swash. Rebuild the grayscale atlas and team-colour lineup with
`powershell -ExecutionPolicy Bypass -File scripts/build-paint-fx-art.ps1`.
Only `game/content/textures/paint_fx.{json,dds}` enters runtime content.
The [implementation and validation record](../../docs/polish-paint-effects.md)
describes exact surface ownership, paint keylines, wet settling and culling.

Fresh wall runoff is also code-drawn from the existing fill texel in
`game/objects/structure_paintable.cpp`, with authored measurements and timing in
`tuning.json.paint.visual.runoff`. The [runoff record](
../../docs/polish-paint-effects.md#wall-runoff) describes its clipping to current
same-team ownership and its bounded lifetime. Normal builds reproduce it directly.

Takeoff/landing puffs and step/skid flecks use the existing fill texel. Their
editable shapes are in [`movement_feedback.cpp`](../objects/movement_feedback.cpp),
with dimensions, timing and colours in `presentation.json.movement_feedback`.
The [production puff record](../../docs/polish-feedback.md#paintexe-production-puffs-2026-10-15)
describes the distinct silhouettes, clean fading and renderer checks. Normal
builds reproduce this code-drawn artwork without an atlas-generation step.

The physical monitor at stage entry and in the reveal is also code-drawn from
the existing fill texel. Its editable casing, keylines, airbrush shade and
decorations are in `game/objects/monitor.cpp`; authored values live in
`presentation.json.opening.monitor_art`. The [monitor scene record](
../../docs/polish-monitor.md) describes the foreground layering and evidence.
The anonymous rear person and fixed-tip cursor are code drawings in
`silhouette.cpp` and `screen_cursor.cpp`, with their colours, edge widths and
brief reveal-click response in `opening.silhouette_art` and `opening.cursor_art`.
The [foreground record](../../docs/polish-monitor.md#person-and-cursor-foreground)
retains their native-size review evidence.

The shared countdown and GO signal use editable coarse-cell strokes and stepped
paper/shade drawings in `game/objects/countdown_art.cpp`, with authored colours,
card pen widths and beat response in `presentation.json.match.countdown_art`.
They use the existing solid texel; no atlas recipe is needed. The
[start-signal record](../../docs/polish-countdown.md) retains native-size evidence
and describes the unchanged gameplay and audio boundary.

The [stage-entry header](../../docs/polish-countdown.md#stage-entry) reuses that
paper drawing with the loaded arena's name in the existing heading font.
`game/objects/level_opening.cpp` fits it above the countdown from
`match.stage_entry_size` and `stage_entry_top`; it clears when controls release.

Closing-clock paper, shade and detached pen ticks are code drawings in
`game/objects/interface_gameplay.cpp`; the reveal's team-labelled coverage cards
are in `game/objects/reveal_art.cpp`. Their authored values are
`presentation.json.hud.closing_clock` and `opening.reveal_art`. They reuse the
existing fill texel and fonts, so normal builds reproduce them directly. The
[match-accent record](../../docs/polish-match-accents.md) describes event gates,
Reduce effects and native-size renderer evidence.

The end menu, [pause/confirmation pages](../../docs/polish-pause-menu.md) and
[results panel and percentage cards](../../docs/polish-results-menu.md)
share the code-drawn stepped paper in
`game/states/menu_panel.cpp` (`MenuPaperArtwork`). Colours, pen width, padding
and shade are authored separately in `presentation.json.end_menu.art`,
`pause_menu.art` and `results_menu.art`; normal builds reproduce them from the
existing solid texel. End and pause pages also use its detached focus arrow.
The texture layer retains point sampling so the solid score bar cannot sample
neighbouring atlas art; the separate text layer uses linear filtering.

The [shared paper motion](../../docs/menu-presentation.md#shared-paper-focus-and-press-motion)
uses the same code drawing: a short inward arrow settle and a pressed card face
inside its existing shadow. `presentation.json.menu_style.motion` supplies its
captured durations and travel; applied Reduce effects keeps the settled artwork.
Labels and widget/navigation bounds keep their authored positions.

The [setup selection accents](../../docs/menu-presentation.md#setup-selection-motion)
also reuse the existing runtime art: Mode/Player Count use the shared paper
arrow and card response, Team contracts the chosen team icon inside its existing
canvas, and Weapon contracts only its tile and border within icon padding.
The required `menu_style.motion.selection_inset_fraction` captures the maximum
per-side inset when a page opens. Per-player response clocks share the existing
focus/press durations and reset when interrupted; Reduce effects draws the
settled selection. Tool layers, labels, badges and input rectangles stay fixed.

The [Join and arena-stage accents](../../docs/menu-presentation.md#join-and-arena-stage-motion)
reuse the same fill and clocks. An accepted Join claim/release briefly slides
and shrinks a thin stripe in the player row's existing left focus padding;
it is absent when settled or with Reduce effects. Arena browsing and lock/unlock
move only the existing briefing stripe within its upper padding. Existing accent
width and captured focus/press durations supply the response; no new JSON fields
or art assets are required. Text, preview, roster and input rectangles stay fixed.

The [title and home pages](../../docs/polish-main-menu.md) use the same paper
drawing in the texture layer, with linear-filtered lettering above it. Their
editable palette and pen widths are in `main_menu.art`; panel sizes and text
anchors live beside it. The neutral surround uses the existing solid texel.

The [Mode and Player Count pages](../../docs/polish-main-menu.md#mode-and-player-count)
reuse that paper and neutral surround with `main_menu.setup_box_size`, centered
above the controls footer. `main_menu.art` supplies the same inks, pen
and shade colour, with card padding and shadow offset overridden by
`main_menu.setup_focus_padding` (`28, 7`) and `main_menu.setup_shadow_offset`
(`2`) to retain pressed-face clearance within the existing row stride.
Transparent text shadows retain the existing offsets and logical
geometry while leaving plain dark lettering. Normal builds reproduce the
panel, focused card and detached arrow without new assets or an atlas recipe.

The [Join page](../../docs/polish-main-menu.md#join) reuses the setup paper and
captured art overrides without a shared focus card or arrow. Connected rows and
the existing claim/release gutter stripe use focus ink; missing/unclaimed rows
use disabled ink, with dark heading/status text. Transparent shadows retain the
original fonts, anchors and logical extents. The existing fill reproduces the
paper and gutter artwork directly; no new fields, assets or atlas recipe are
needed.

The [Team Select page](../../docs/polish-main-menu.md#team-select) uses
`main_menu.team_box_size` for its wider outer paper and the existing setup-art
overrides for static row cards. Each `focus_fill` card encloses its actual
label, badge and three unchanged choice icons. Dark lettering has transparent
retained shadows; `main_menu.team_select.warning_position` puts the existing
warning below the roster. The paper uses the existing solid texel, with no
new asset or atlas recipe. Choice frames, readiness colours and independent
icon motion retain their existing rules.

The [Weapon Select page](../../docs/polish-main-menu.md#weapon-select) uses
`main_menu.weapon_box_size` for its outer paper and draws stepped paper inside
the existing icon border envelope. The shared paper primitive retains its
layers and premultiplied colours; each icon still uses the captured catalog's
sheet/fill frame. Its pen uses the existing rule width, its shade uses the setup
shadow offset, and its live white/mint face indicates readiness. The existing
tile contraction leaves fitted tool, paint and supply layers fixed. Captured
dark lettering has transparent retained shadows. Normal builds reproduce the
drawing from existing assets without a new atlas recipe.
A fixed cream canvas using the existing heading colour preserves the preview
inside the animated readiness surround. Its bounded keyline bleed stays within
existing padding; tool/paint/supply and input geometry do not move.

The [Stage Select page](../../docs/polish-main-menu.md#stage-select) uses
`main_menu.stage_box_size` for its outer paper and a captured `focus_fill`
briefing card enclosing the authored panel and actual text with pen/shadow
clearance. The card follows actual refreshes without changing layout or fit
budgets. Dark name/status ink uses `stage_select.selected_ink` and `unselected`;
the preview edge retains its mint selection tint. Other paper text has
captured ink, with focus ink for the briefing label and goal, and transparent
retained shadows. Random's light question mark and visible
shadow stay on the dark preview. StagePreview rendering and arena art are
unchanged; the existing fill reproduces the new paper without assets or an
atlas recipe.

The [Options and Credits pages](../../docs/polish-main-menu.md#options-and-credits)
extend the same drawing, with authored panel sizes and text positions in
`main_menu`, readable disabled-row ink and a separate dark save-warning ink.
Options cards include each row's label and value. The complete Credits body,
palette names and save status fit their measured areas without changing copy.

## Terrain and backdrop artwork

The [terrain source](terrain/README.md) holds the Muted Day palette, the backdrop
drawings (hills, trees, a house, clouds, a sun, birds) and the four airbrush
speckle tiles, as editable cell shapes and grids. Rebuild it with
`powershell -ExecutionPolicy Bypass -File scripts/build-terrain-art.ps1`; only
`game/content/textures/terrain.{json,dds}` is staged and packaged. The walkable
materials themselves are drawn by code from the sheet's palette swatches; the
[terrain kit](../../docs/polish-terrain.md) owns their rules.

## Runtime content separation (GAP#8.6)

Implemented 2026-10-15. Tracked runtime content in `game/content/` is the reviewed set
shared by development staging and Release in
[`runtime_assets.cmake`](../../cmake/runtime_assets.cmake). Manifest changes are
checked against that list at configure time. Authoring and historical material
is retained here:

| Location | Retained material |
|---|---|
| `sources/textures/` | Sprite-sheet PNG/XCF, menu PNG, generated weapon PNG and legacy texture conversion command |
| `sources/levels/` | Publisher layouts and historical movement notes; playable JSONs remain runtime content |
| `sources/fonts/` | The [font recipe](#sprite-font-recipe-gap87) `fonts.json`, and the historical font command example `cmd.txt` |
| `tools/` | Existing texconv and MakeSpriteFont executables |
| `archive/textures/` | Old texture kit and unused player-frame experiment |
| `archive/levels/` | Unused historical level JSONs |

The 138 relocated files were byte-checked before recipe edits. The weapon packer
uses the relocated utility source and tool; its DDS, JSON and working PNG still
reproduce byte for byte. The legacy texture command and historical font example
now use repository-root paths; neither adds a new build dependency.

[`stage_runtime_assets.cmake`](../../cmake/stage_runtime_assets.cmake) runs on
every build, copies only the shared list, and prunes stale files within the
owned `fonts/`, `textures/`, `levels/` and `sounds/` directories. It validates
the source set and accepted bank before changing the mirror, rejects linked
content entries, and checks that the destination stays within the build's game
output. Other output-root files, including DLLs and developer captures, are
preserved. An absent optional bank also removes its stale staged copy; Release
packaging continues to require the accepted bank.

Validation on 2026-10-15:

- Debug and Release build and pass all 11 consumer acceptance cases each.
- All six runtime-bank tests pass, including the shared staging regression.
  Clean and stale mirrors, bank removal/restoration, invalid-bank and file-type
  rejection before cleanup, unrelated-file preservation and Windows junction
  rejection are covered. File symlinks require a privilege unavailable in this
  shell; the root-link check uses a directory junction here instead.
- Both existing build mirrors and the extracted 49-file Release ZIP contain
  exactly the same 22 runtime assets, byte-matching their sources. No authoring
  assets or tools enter the ZIP.
- Isolated Debug and extracted Release startup pass with the accepted bank and
  separately without it, using a system-only PATH and graceful shutdown. Each
  missing-bank run reports the expected single optional-audio diagnostic.

Evidence is in `out/validation/content-separation/` and `out/acceptance/`.
These checks close `GAP#8.6`; clean-PC, physical-pad and listening acceptance
remain in their existing TODO rows. To rerun the staging regression alone:

```powershell
cmake -DTEST_DIR=out/validation/content-separation -P cmake/check_runtime_staging.cmake
```

## Sprite font recipe (GAP#8.7)

Implemented 2026-10-17. The seven atlases in `game/content/fonts/` are tracked
output, on the wave bank's pattern: the recipe and tool are in this tree, the
build checks the output without running the tool, and regeneration is an
optional step.

[`sources/fonts/fonts.json`](sources/fonts/fonts.json) is the recipe: per atlas
the face, size, style, the face file it came from, and the texture size and
line spacing a regeneration must give; for all of them the character region
`0x20-0x7E` (exactly what [`drawable_text`](../drawable_text.h) accepts), no
default character, and MakeSpriteFont's `CompressedMono` (BC2) format.

| Atlas | Face | Size | Style |
|---|---|---|---|
| `courier_prime_bold_16` | Courier Prime (`CourierPrime-Bold.ttf`) | 16 | Bold |
| `lato_black_24`, `_36`, `_48`, `_72`, `_144` | Lato Black (`Lato-Black.ttf`) | 24, 36, 48, 72, 144 | Regular |
| `bowlby_one_144` | Bowlby One (`BowlbyOne-Regular.ttf`) | 144 | Regular |

All three faces are SIL Open Font License 1.1 and are tracked beside the
recipe, with their licences in [`sources/fonts/licenses/`](sources/fonts/licenses/),
which the release ZIP carries as `licenses/fonts/`. They replaced Gill Sans MT
Bold, Gill Sans Ultra Bold and Courier New Bold on 2026-10-25, before the
repository went public: those are Microsoft and Monotype faces, and an atlas of
them was not ColourWars' to redistribute. Lato Black was the closest weight and
width to Gill Sans MT Bold, Bowlby One to Gill Sans Ultra Bold, and Courier
Prime is a Courier.
Each entry's `line_spacing_adjust` is MakeSpriteFont's additive `/LineSpacing`,
rounding the face's own spacing to a value float sums keep exact: a whole pixel
for Lato and Courier Prime, whose 38.4, 57.6, 76.8, 115.2, 230.4 and 23.96
drift when added to a position and subtracted back, and 239.25 for Bowlby One,
whose own 300.75 is mostly vertical-metric headroom and whose two-line title
wordmark is laid out on the spacing of the face it replaced.

MakeSpriteFont reads only installed fonts, so the script installs any recipe
face that is missing for the current user (no elevation) before it runs.
Regeneration still stays out of builds and CI:

```powershell
scripts/build-fonts.ps1            # regenerate into a temp folder and compare
scripts/build-fonts.ps1 -Update    # also write any that differ over game/content/fonts/
```

It runs `tools/MakeSpriteFont.exe` with an explicit region and format and
compares each result with the tracked file: `identical`, `equivalent` (same
glyph table, region, texture size and format; line spacing within 1e-5 and at
most `-MaxTextureBlocks` BC2 blocks of texture apart, 8 by default) or
`DIFFERENT`, which fails the run. A face that is still
not installed after that stops it with the face's name rather than letting GDI+
substitute another. On 2026-10-25, on the machine that made them, all seven
atlases regenerate byte for byte.

Every build runs [`cmake/check_sprite_fonts.cmake`](../../cmake/check_sprite_fonts.cmake)
from runtime staging. With no tool and no faces, it reads each tracked atlas's
header against the recipe: the `DXTKfont` signature, every glyph exactly the
region in order, no default character, the recipe's texture size and DXGI
format, and a file size equal to what the header describes. The recipe and
`cmake/runtime_assets.cmake` must also name the same seven files. So a
regenerated font with a narrower or wider region, or a different size, fails
the build instead of reaching a menu.

Text wrapping, the other half of `GAP#8.7`, is a widget capability rather than
content: see [Weapon description wrapping](../../docs/menu-presentation.md#weapon-description-wrapping).
