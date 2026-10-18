# Tool Props terrain, shadows and backdrop (`POL#9`, first increment)

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

## Not done (what `POL#9` still holds)

- Human review in play, clean and heavily painted, with all three team pairs, at 1P
  and 4P (part of locking the style with `POL#7` and `POL#14`).
- Landmarks, signage and room life; small ambient animation; captioned Paint-artefact
  jokes in the backdrop; modest parallax after camera framing.
- Close Quarters' own backdrop and fitting the kit to its blockout (`POL#17`); a
  palette and composition per arena (`POL#16`).
- The opening's monitor palette strip still shows 15 greys rather than the level's
  palette.
- The overview and the monitor picture draw the arena small enough that one-cell
  lines and ruler ticks drop pixels under point sampling.
