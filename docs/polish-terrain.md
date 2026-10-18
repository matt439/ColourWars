# Tool Props terrain, shadows and backdrop (`POL#9`)

Implemented 2026-10-18. This page owns the terrain kit's rules, its runtime shape and
what of `POL#9` remains. The art rules it follows are the accepted
[Paint.exe rules](polish-plan.md#1-establish-the-art-and-display-rules) (Tool Props
terrain, airbrush shadows, Zoom 400%) and the author's Muted Day palette. The picture
it was built to is the world the author chose that palette in, in the
[box-head builder](https://claude.ai/artifact/47ANdGfpipyeFa1W527cJq): drawn
stationery on tiled ground, airbrush speckle under floating platforms, and a crude
drawing of hills, trees, a house, clouds, a sun and birds behind.

## What it looks like

Every collision object in the four shipped arenas wears a **material**, a drawing of
a piece of stationery that fills its collision shape exactly:

| Material | Worn by | Drawing |
|---|---|---|
| `ground` | solid structures; the big paintable floors | Halved-bond tile courses with black joints in the top four courses, `ground_shade` below. Paintable ground has a light `ground_top` lip under its top keyline; solid ground does not. |
| `pencil` | long paintable platforms and walls | `pencil_body` with a shaded lower facet; an eraser and ferrule at the end away from the stage's mirror line, a cut wood end with its graphite core toward it. A pencil across the mirror line is cut at both ends; one standing up has its eraser at the top; one shorter than four times its thickness (or 60 units) has no end caps. |
| `eraser` | tall paintable blocks (King of the Hill's tiers) | `eraser_fill`, a keylined `eraser_shade` lower third and one shine dash below the paint band. |
| `paint_tin` | King of the Hill's summit | Metal tin side on: rim, `prop_accent` label band, shaded side. |
| `ruler` | jump-through platforms and fences | Light `ruler_fill` with tick marks rising from the lower edge (short every 16 units, long every 80). |
| `set_square` | ramps | A `metal_light` triangle stepped in whole cells inside the collision triangle, its engraved inner triangle drawn as a line, not a hole. |

Behind the terrain, each arena's decorations are a `paper` sky over the whole
out-of-bounds rectangle, rolling `bg_far` hills on the floor line, two kinds of tree,
a house, clouds, a sun with a flat little face and two flights of birds, all from the
`terrain` sheet. Under every exposed underside of a flat platform an airbrush shadow
hangs: baked speckle tiles, dense `shadow_deep` near the platform thinning to
`shadow`.

## Rules

- **One cell is four world units**, as the painter's. Keylines are one cell. Interior
  detail (ticks, tile joints) sits on the world grid so neighbours line up; outlines
  sit on the exact edges, so sizes that are not whole cells lose part of an interior
  cell, never an outline.
- **The drawing never leaves the collision shape.** No overhang, no hole in a
  walkable, no tip beyond the rectangle (the pencil's cut end is the honest version of
  its sharpened one). If it is black-lined you can stand on it; backdrop props are
  lined in `bg_line`, never black, and never look solid.
- **Outlines, lips and end caps go only on exposed faces.** The same exposed-interval
  test that decides where paint can land (`paint_face_geometry.h`) decides where
  terrain is outlined, so abutting and stacked pieces (King of the Hill's castle, the
  Turbulence divider's ledge) read as one surface rather than a seam.
- **Thin rulers keep one keyline.** Under five cells thick the lower (or right) edge
  stays open, so a 10-unit spawn ceiling is a black line over a light board, not a
  black bar.
- **Material fits type.** `ground` on solid or paintable structures, `pencil`,
  `eraser` and `paint_tin` on paintable ones, `ruler` on jump-throughs, `set_square`
  on ramps only. The game, the editor and the schema refuse anything else
  ([level format](level-format.md)). An object without a material keeps the legacy
  stretched frame.
- **Hues live in texels; tints stay grey.** Every JSON `colour` is still a grey (the
  terrain's and backdrop's are `WHITE`); Muted Day's muted hues reach the screen only
  through the `terrain` sheet's swatch frames and backdrop drawings, which the art
  build refuses to contain any colour outside the palette. No team hue appears in the
  scenery ([backstory](backstory.md)).
- **Shadows land on the backdrop only.** They are plain scene objects drawn before
  every structure, so terrain and paint always cover them. One object per exposed
  underside, each culled by its own extent.

## Runtime

- [`TerrainMaterial`](../game/objects/terrain_material.h) is a small hierarchy, in
  the game's OOP grain: the base resolves the swatch frames and replays a quad list;
  `OrientedTerrainMaterial` lays its drawing along the long axis; `GroundMaterial`,
  `PencilMaterial`, `RulerMaterial`, `EraserMaterial`, `PaintTinMaterial` and
  `SetSquareMaterial` each work their drawing out once, at construction, into an
  immutable list of swatch quads. `draw` only replays it: const, deterministic and
  identical in every view. A `Structure` owns its material; `Structure::draw` paints
  it instead of the stretched frame.
- [`AirbrushShadow`](../game/objects/airbrush_shadow.h) lays the four baked speckle
  tiles along a span on the world grid, choosing a variant by grid index and cropping
  the last tile rather than stretching it.
- `LevelObjectBuilder` reads `material`, computes each object's exposed faces and the
  stage's mirror line (pencils point their erasers away from it, so mirrored halves
  stay mirrored), and builds the shadows. Level and the stage-select preview both
  take the shadows from it, so the preview shows the same arena.
- The [`terrain` sheet](../game/art/terrain/README.md) is built by
  `scripts/build-terrain-art.ps1` from `game/art/terrain/terrain.json`.
- The opening and reveal's monitor palette uses those same swatch frames with a
  WHITE tint; its colours have no second RGB list in presentation data.

## Checks

`scripts/acceptance/terrain.cpp` builds every shipped stage and requires that every
collision object has a material fitting its type, that every quad lies inside the
collision rectangle (and, for ramps, inside the triangle), that two builds draw the
same quads, that each object stays within a quad budget, that every shadow hangs
under a real underside inside its cull bounds, that every backdrop is untinted on
paper, and that the material table refuses what it does not draw. Level-format and
editor acceptance cover the field ([level editor](level-editor.md)).

## Decisions made while building it

These are calls the author has not yet seen; each is easy to reverse.

- **Tool Props over the warehouse brief.** The plan's warehouse direction predates the
  2026-10-09 switch to Paint.exe. The kit follows the accepted Tool Props rule and the
  mockup the author chose Muted Day in. The paint workshop and supply warehouse remain
  Close Quarters' *subject*: its own backdrop and landmarks are fitted with its
  blockout (`POL#17`).
- **Which material each existing piece wears**: solids are ground, jump-throughs are
  rulers, long paintables pencils, tall ones erasers, the King of the Hill summit a
  paint tin.
- **The one-cell keyline**, matching the figure, with thin rulers keeping only the top
  one.
- **Paint covers the outline.** The paint band (12 units, keyline at its inner edge)
  sits over the top of the material, so a painted top's outer edge is the team colour
  against the sky. In the 2026-10-18 shots that reads clearly for every pair checked;
  if yellow or green on paper proves weak in play, the alternative is to redraw the
  outer keyline over the paint.

## Monitor palette coherence (2026-10-18)

The monitor's old row of 15 greys is replaced by the kit's 26 role swatches,
followed by the one empty slot the story is about. `opening.palette_sheet` and
`opening.palette_frames` in `presentation.json` name the atlas and the ordered
frames. The face and highlight roles deliberately remain separate white cells,
as they are in the kit. A later palette edit changes the shared atlas texels.

The row keeps square cells and visible gutters, shrinking together to fit the
program's width. The picker pair, including its panel and outline, stays inside
the picture above the row. The cursor follows these fitted targets through the
opening and reveal; saving fills only the final slot. The existing picture,
glass, opening/reveal timing and gameplay cameras retain their layout.

Startup checks the sheet and each indexed frame before a match can use it,
requiring positive square swatches with zero authored origins. Missing, empty or
wrongly typed fields and missing resources report their presentation keys.

Validation on 2026-10-18:

- Debug and Release build under `/W4 /WX` and pass all 36 consumer cases. The
  story/audio case also checks the shipped role order, malformed fields, missing
  resources, displaced/nonsquare frames and valid custom palettes.
- Both focused D3D11 monitor matrices pass on the NVIDIA GeForce RTX 5080 at
  640x480, 1024x768, 720p, 1080p and QHD, plus pushed framing. Actual pixels match
  all 26 Muted Day cells in order, with square cells and visible gutters. Preview
  and saving retain every existing swatch; the picker and panel remain contained.
  Existing 2P/4P opening/reveal/winner/draw, solo bypass, cursor targeting,
  reduced-effects and frozen raster/state/RNG checks pass; Debug also preserves
  replay fingerprints.
- Isolated Debug and Release startup pass with the accepted bank, a system-only
  PATH and graceful shutdown. `git diff --check` passes.

The retained [story preview at 720p](images/polish-terrain/monitor-palette-story-720p.png)
shows the row with the arena; native-size palette/picker crops are retained for
[1280x720](images/polish-terrain/monitor-palette-1280.png) and
[640x480](images/polish-terrain/monitor-palette-640.png).
Local evidence is under `out/validation/monitor-palette/` and `out/acceptance/`.
Renderer run IDs are Debug `cab6344d19ea4df6a077b7f879b996e6` and Release
`40276642ba4c4c4e865b8bbce23d5c95`. Human gameplay/style review remains below.

## Quiet cloud drift (2026-10-18)

The existing 23 clouds now drift slowly behind the four arenas. Each follows a
bounded horizontal cycle, with a 32- or 48-unit excursion and a 48–72-second
period. Different starting phases and periods keep neighbouring clouds from
moving in lockstep. Position changes in four-world-unit cells, preserving the
drawn grid. The atlas, palette and collision silhouettes are unchanged.

`Visual` entries opt in through the level's optional `drift` object
([format](level-format.md#decorative-drift)). The authored rectangle is the
stationary anchor. [`DriftingVisual`](../game/objects/drifting_visual.h) owns a
bounded cosmetic clock; Scene advances it once per world update, independently
of how many views draw it. Draws only read the chosen position. Pause, count-in,
the opening and the ending freeze it with the world. Rebuilding the stage starts
from its authored phase, and stage previews retain that initial pose.

Reduce effects immediately draws each cloud at its stationary anchor, including
when changed while paused. Its clock stays frozen while reduced and resumes
from the held phase when detail returns. Culling includes both drift extremes,
the anchor, authored frame origins and view-pixel quantization. Drift has no
collision, paint, audio or gameplay-randomness role.

The runtime, editor and schema check the optional fields. The editor's existing
numeric inspector can edit them; undo/redo and save/reopen retain them and
unknown metadata. A plain `Visual` without `drift` remains stationary.

Validation on 2026-10-18:

- Debug and Release build under `/W4 /WX` and pass all 36 consumer cases. The
  terrain case covers every shipped cloud, deterministic restart and phases,
  full-cycle bounds, four-unit snapping, large/nonpositive/nonfinite ticks,
  RNG independence, live reduction and named invalid-content diagnostics.
- Both editor document suites and hidden launch/selection/close checks pass.
  Drift checks cover schema agreement, numeric/type limits, float conversion,
  transactional inspector edits, history and metadata-preserving save/reopen.
- `scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -DriftOnly` and its
  Release counterpart pass on the NVIDIA GeForce RTX 5080. Real D3D11 cloud
  rasters match the old static art at zero phase, move to distinct extremes
  and return exactly after a cycle at 1P/4P 720p, 1080p and QHD. Direct draws
  agree with Scene when only a drifting edge enters the view. Projected
  bounds, immediate reduction, held phases, frozen redraw/RNG and resize
  round trips pass. Actual arena captures preserve simulation observations
  and Debug replay fingerprints during drawing.
- Isolated Debug and Release startup pass with the accepted bank and a
  system-only PATH. The 55-file Release ZIP builds; all four packaged levels
  byte-match their sources, required runtimes and accepted audio are present,
  and no development files are packaged. Its extracted copy also passes
  isolated startup and graceful shutdown. `git diff --check` passes.

Retained native-size evidence includes the
[cloud/culling crops at 720p](images/polish-terrain/cloud-drift-culling-720p.png),
Close Quarters [early](images/polish-terrain/cloud-drift-cq-early-720p.png) and
[12 seconds later](images/polish-terrain/cloud-drift-cq-later-720p.png), and a
[4P 1080p sky composition](images/polish-terrain/cloud-drift-cq-4p-sky-1080p.png). The last
fixture explicitly places the real players above the platforms so the normal
cameras show clouds; it establishes no route or jump reachability. These are
art/renderer checks; human motion/readability review remains open.

Local evidence is under `out/validation/cloud-drift/` and `out/acceptance/`.
Final renderer run IDs are Debug `d47fffce9a4342f3ade57904128a1741` and Release
`3b239ecb30a345f3bce5c803adfe3f1d`.

## Not done (what `POL#9` still holds)

- Human review in play, clean and heavily painted, with all three team pairs, at 1P
  and 4P (part of locking the style with `POL#7` and `POL#14`).
- Landmarks, signage and further room life beyond cloud drift; captioned
  Paint-artefact jokes in the backdrop; modest parallax after camera framing.
- Close Quarters' own backdrop and fitting the kit to its blockout (`POL#17`); a
  palette and composition per arena (`POL#16`).
- The overview and the monitor picture draw the arena small enough that one-cell
  lines and ruler ticks drop pixels under point sampling.

## Ready implementation slices (2026-10-18)

`POL#9` has three implementation slices in the current arenas. Retain checks and
evidence for each slice. Final human style acceptance remains separate; no slice
unlocks the phase-2 blockouts by itself.

| Slice | Scope | Acceptance | Size |
|---|---|---|---|
| Small-view terrain readability | Address disappearing keylines and ruler ticks in the miniature overview and monitor picture. Choose an art/client-rendering treatment after comparing native-size captures. | Retain useful silhouettes at 720p, 1080p and QHD through overview/monitor transitions, resize and 1P/4P framing; keep gameplay cameras, collision and point-sampled gameplay art correct. | M |
| Reusable landmarks | Add a small set of non-text Tool Props landmarks/background details to the current arenas under the Muted Day rules. Keep them reusable when layouts change. | Compare clean/heavily painted scenes for every team pair at 1P/4P. Landing edges and routes stay truthful; decoration changes no collision, paint budget or gameplay RNG. Animated details must respect pause, culling and Reduce effects. | M |
| Modest parallax prototype | Compare distant/middle backdrop layers with the static version in the current arenas. Establish the per-view draw approach before broadening the art. | Independent split-screen views, conservative culling, resize and overview/monitor transitions remain correct; define static preview/editor behaviour. Keep drawing free of simulation/RNG mutation and retain a static Reduce effects presentation. | L |

Start with the small-view defect. Landmarks and the parallax prototype are further
current-phase choices, not a request to finish all scenery before reviewing it.
Retain parallax only if the comparison improves the composition and readability;
the static backdrop remains a valid outcome of the prototype.
Captioned jokes need the author's wording and do not gate non-text landmarks.
Close Quarters' own composition and fitting the kit to revised geometry stay
with `POL#17`; final per-arena production stays with `POL#16`/`POL#21`.
