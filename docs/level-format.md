# Level format 1

A level is one UTF-8 JSON object. Start from [editor/template.json](../editor/template.json)
or **File > New** in the [level editor](level-editor.md). The template uses shipped
resources and supplies a closed grey room, a paintable floor and four spawns per
team. It is an authoring example, outside the runtime asset list and manifest;
copy it before editing. Its geometry is not an accepted arena design.

[level.schema.json](level.schema.json) is a JSON Schema Draft-04 structural check,
compatible with the repository's bundled RapidJSON `SchemaValidator`. The runtime
still validates through [LevelLoadedInfo](../game/objects/level_loaded_info.cpp),
[LevelObjectBuilder](../game/objects/level_object_builder.cpp) and loaded assets.
Passing the schema alone does not establish that a stage loads or plays correctly.

## Version and compatibility

Write `"format": 1` on new files. An absent `format` is accepted as legacy format
1. An explicit version must be the JSON integer `1`; a string, boolean, null,
decimal representation such as `1.0`, or another integer is rejected before the
level is decoded. The four shipped levels and newly created editor documents write the
version. Opening a legacy file does not require rewriting unrelated fields.

Unknown keys are ignored by the runtime, including `_comment`, per-object `name`
and metadata belonging to an authoring tool. The editor preserves unknown keys
when loading, editing and saving. The schema consequently allows additional
properties at every object. An ignored field does not become an extension point:
for example, adding `rotation`, `velocity`, `layout` or `layer_depth` does not
rotate, move, position or reorder a level object. Unsupported `type` and
`collision_type` values are errors.

The old JSON examples in [game/art/archive/levels](../game/art/archive/levels)
are historical sources, retained unchanged. Some use `level_name` instead of the
current metadata or unsupported object tags; `base.json` there is not a template
for the current loader. The Publisher documents in
[game/art/sources/levels](../game/art/sources/levels) are layout source material,
not runtime assets.

## Root fields

| Key | Type | Meaning |
|---|---|---|
| `format` | Optional integer | `1`; omission selects legacy format 1. |
| `display_name` | String | Authored stage label. Startup checks it against the game's drawable text rules and fonts. |
| `icon_frame` | String | Existing frame in `sprite_sheet_1`. Required legacy icon metadata; stage selection now draws a live arena preview. |
| `include_in_random` | Optional boolean | Defaults to `true`; `false` keeps an experimental stage out of Random without removing its explicit menu entry. |
| `sound` | Object | Bank identifier and independent optional music/sting pairs, below. |
| `spawn_locations` | Object | `team_a` and `team_b` arrays of points, at least four entries each. |
| `level_bounds` | Object | All four named rectangles, below. |
| `stations` | Optional array | Named developer-console destinations; defaults to no authored stations. |
| `collision_objects` | Array | Structures, paintable structures and ramps; may be empty. |
| `non_collision_objects` | Array | Decorative `Visual` entries; may be empty. |

Keys and enum spellings are case sensitive. JSON numbers may be integral or
fractional except `format`, which uses the integer representation. Coordinates
become game `float` values. Use ordinary finite world coordinates: a JSON number
can overflow or underflow a game float even though a schema accepts it. Required
fields have no implicit default unless this document names one. Empty strings
are not useful asset identifiers; loaded-resource lookup determines whether a
sheet, frame, bank or effect actually exists.

## Coordinates, bounds and spawns

World coordinates increase rightward on X and downward on Y. They are game
units, independent of display resolution and split-screen layout. Level files
do not choose the match's player count, layout, weapons, palette, duration or
series length; setup and tuning supply those values.

Each `level_bounds` member uses the following encoding:

```json
{ "x": 0, "y": 0, "width": 3000, "height": 1400 }
```

| Bounds key | Purpose |
|---|---|
| `out_of_bounds` | Scene retirement boundary. Leaving it retires projectiles; players leaving it are an error. |
| `camera_bounds` | Limits the player-follow camera. |
| `zoom_out_start_bounds` | Authored start framing for the ending camera transition. |
| `zoom_out_finish_bounds` | Authored end/overview framing; also the editor's initial arena view. |

All four require finite coordinates and positive width/height. They are separate
rectangles: the loader does not require them to nest or enforce an aspect ratio.
Only object rectangles, not these bounds, accept `rectangle_tlbr`.

Spawn entries are `{ "x": 100, "y": 1075 }`, the character's **top-left**, not its
feet or centre. Character size comes from `tuning.player.size`; the template
places the current 52-by-120 character five units above the floor. The loader
checks each point is inside `out_of_bounds`, including its edges. This point
test does not prove the whole character fits, has a supporting floor or avoids
overlap with terrain or other players. Test those conditions in play.

Team A spawn X coordinates must be strictly left of
`out_of_bounds.x + out_of_bounds.width / 2`; Team B coordinates must be at or
right of that midpoint. Each array needs at least four points even if the
intended match has fewer players. Players consume their own team's points in
roster order; the first player on Team B uses `team_b[0]`, regardless of their
device slot or view index. The same position is retained for respawn.

## Objects and geometry

Every object needs `type`, `sheet_name`, `frame_name` and `colour`. Collision
objects also need an allowed `collision_type` and may name a `material`
([Materials](#materials)). A `name` is optional authoring
metadata: the editor uses a string name as its list label, and the runtime
does not use it for lookup, collision or uniqueness.

| Array | `type` | Allowed `collision_type` | Geometry and extra fields |
|---|---|---|---|
| `collision_objects` | `Structure` | `STRUCTURE`, `STRUCTURE_JUMP_THROUGH` | Rectangle; optional `material`. Fixed solid or jump-through terrain; no scoring paint. |
| `collision_objects` | `StructurePaintable` | `STRUCTURE_PAINTABLE`, `STRUCTURE_JUMP_THROUGH` | Rectangle plus all four `paintable_faces` booleans; optional `material` and horizontal `motion`. Paint-scoring solid or jump-through terrain. |
| `collision_objects` | `StructureRamp` | `STRUCTURE_RAMP_RIGHT`, `STRUCTURE_RAMP_LEFT` | `triangle` with six coordinates; optional `material`. Fixed ramp collision; no scoring paint. |
| `non_collision_objects` | `Visual` | None is read | Rectangle; optional `drift`. Decorative art with no collision or score; backdrop props use sheet `terrain`. |

Flat terrain and visuals use either of these object members:

```json
"rectangle": { "x": 100, "y": 600, "width": 400, "height": 40 }
```

```json
"rectangle_tlbr": { "left": 100, "top": 600, "right": 500, "bottom": 640 }
```

Width and height must be finite and positive; for TLBR, `right > left` and
`bottom > top`. If both encodings are present, `rectangle` takes precedence and
the runtime ignores `rectangle_tlbr`, even if that ignored field is malformed.
The schema preserves that precedence. Neither encoding accepts anchor names,
relative positions or a layout expression.

### Horizontal paintable motion

A flat `StructurePaintable` may add:

```json
"motion": { "type": "horizontal", "travel_x": 120, "period": 6.4, "phase": 0.25 }
```

The rectangle is the **leftmost anchor**, rather than the initial world pose.
The body travels right by `travel_x`, then returns along the same horizontal
path. `period` is the complete round trip in seconds. `phase` is a fraction of
that trip: 0 is the left endpoint departing right, 0.5 is the right endpoint
departing left, and 0.25 starts halfway to the right endpoint. The initial pose
is used by gameplay, culling and the stage preview; paint and credit stay on
the object's local faces. Omission retains the original static object path.

All four fields are required. Travel must be finite and positive and reach a
distinct representable endpoint. The full period must span at least two fixed
simulation steps (currently 1/30 second); phase must remain in [0,1) after float
conversion. The anchor and endpoint must have finite, positive represented
edges. Both solid and jump-through paintables are supported. Motion on plain
structures, ramps or visuals, another motion `type`, and `axis`, `travel_y` or
`rotation` in the record are errors, as are a `drive` other than the two above
and the paint-drive rules below. Other unknown keys remain ignored metadata,
as elsewhere in format 1; they do not add motion capabilities.

A mover may instead be a **paint ferry**:

```json
"motion": { "type": "horizontal", "travel_x": 320, "period": 4, "phase": 0,
            "drive": "paint", "team_a_heads": "right" }
```

`drive` is `"cycle"` (the default, the triangular round trip above) or
`"paint"`. A paint drive moves along the outgoing leg only while one team holds
more of the mover's own paint, by area over all its faces: toward
`team_a_heads` (`"right"` or `"left"`) for team A, and the other way for team
B. It holds on a tie, including no paint, and stops at either end. Its speed is
a cycle's leg speed, `2 * travel_x / period`. `phase` must be in [0, 0.5],
where 0 is the left end and 0.5 the right. `team_a_heads` is required with a
paint drive and refused with a cycle. The game's loader and the
[editor](level-editor.md#moving-structure-authoring-gapd5) read both fields
through one rule in `game/horizontal_motion_contract.h`, so the editor refuses
a bad record before save and the game refuses it at load.

This is the existing horizontal triangular mover, with its existing safety
limits. On actual Level admission the entire platform envelope and expanded
standing corridor must lie inside Scene bounds and clear every other structure
over its complete path. The actual player dimensions/rates must leave safe
relative displacement at the fixed step. Two individually valid records can
therefore still fail when combined into a stage. The schema and startup object
validation alone cannot establish this scene-level contract.

Moving anchors do not permanently bury static paint or terrain edges and do
not leave a static airbrush shadow. A material-bearing moving platform casts
the existing translated shadow tied to the actual Scene-owned mover.

The [editor](level-editor.md#moving-structure-authoring-gapd5) now exposes solid
and one-way horizontal paintables, anchor/motion fields and full travel/standing
guides. Its document model requires explicit source `tuning.json` context for
player size and validates envelope edits transactionally. Both-preset document
and editor-save/public-loader checks pass. No shipped arena uses this field yet;
public-loader record/play reconstruction is CPU-qualified in the
`moving_structure` acceptance case. Useful arena placement and native/human
acceptance remain separate.

A ramp reads `triangle` instead of either rectangle:

```json
"triangle": { "x1": 100, "y1": 600, "x2": 500, "y2": 600, "x3": 500, "y3": 400 }
```

This example is an upright right ramp: a horizontal bottom edge, with the high
corner on the right, paired with `STRUCTURE_RAMP_RIGHT`. For a left ramp, put
the high corner at the left and use `STRUCTURE_RAMP_LEFT`. The editor accepts
the three distinct corners in any order and checks that they match the tag.
The runtime constructs `TriangleRightAxisAligned` from the six numbers and
uses its bounding box for the sprite; it does not perform the editor's full
orientation/degeneracy check at construction. The schema checks the coordinate
fields, not that geometric relationship. Author upright nondegenerate ramps.
The loader mirrors the sprite horizontally for a left ramp, so its source
`frame_name` should contain the right-facing artwork. Paint projectiles pass
through ramps under the current collision masks.

Paintable rectangles require every face explicitly:

```json
"paintable_faces": { "left": false, "top": true, "right": false, "bottom": false }
```

Paintability and player collision are independent: `StructurePaintable` with
`STRUCTURE_JUMP_THROUGH` keeps upward passage and Down-drop behavior while its
enabled faces paint and score. Projectiles still collide with that support.
Use `ruler` for this collision tag. Thin rulers can score just their top face;
this avoids overlapping top and bottom strips when paint thickness exceeds the
support's depth. Paint strips do not change the physical support rectangle.

All-false is allowed. An authored true face creates scoring paint only where
it remains exposed: other flat `Structure`/`StructurePaintable` rectangles,
including jump-through terrain, clip buried face intervals during construction.
Ramps and decorative visuals do not clip paintable faces. Tile granularity and
paint-strip thickness come from tuning, not per-level fields. The editor's
blockout aids show the same exposed-face calculation.

Objects are created in array order, with non-collision objects before collision
objects and players. Put backgrounds in `non_collision_objects`. A frame is
stretched into the authored rectangle or ramp bounding box; its source texel
size does not change collision dimensions.

### Decorative drift

A `Visual` may carry a bounded horizontal drift around its authored rectangle:

```json
"drift": { "distance": 32, "period": 60, "phase": 0.25 }
```

All three fields are required when `drift` is present. `distance` is the maximum
horizontal excursion in world units, greater than zero and at most 256; `period`
is a full cycle in seconds, from 1 through 3600; `phase` is the initial fraction
of a cycle, from zero inclusive to one exclusive. Values must be finite numbers
and stay in range when converted to the game's float numbers; the editor checks
that conversion too.
The offset is `distance * sin(2 * pi * (time / period + phase))`, snapped to the
four-world-unit art grid. The authored rectangle is the stationary anchor, not
the left edge of the motion. A distance between grid steps can round outward by
up to two units; culling rounds the distance up to the next whole grid step so
it includes the entire possible excursion.

The initial pose follows `phase`; the stage preview retains that pose. Reduce
effects holds the drawing at its anchor. Drift never changes collision, scoring
or gameplay randomness, and cannot be authored on a collision object. Without
`drift`, a `Visual` keeps its ordinary stationary drawing. The editor preserves
the fields through loading and saving and exposes their numeric values in the
property inspector.

## Materials

`material` is an optional string on a collision object. It names the look the
object is drawn with from the `terrain` atlas, instead of stretching its
sheet/frame into the rectangle. An absent `material` keeps that legacy stretched
draw unchanged. Each material may sit only on these collision types:

| `material` | Allowed `collision_type` (object `type`) |
|---|---|
| `ground` | `STRUCTURE` (`Structure`) or `STRUCTURE_PAINTABLE` (`StructurePaintable`) |
| `pencil`, `eraser`, `paint_tin` | `STRUCTURE_PAINTABLE` (`StructurePaintable`) |
| `ruler` | `STRUCTURE_JUMP_THROUGH` (`Structure` or `StructurePaintable`) |
| `set_square` | `STRUCTURE_RAMP_LEFT` or `STRUCTURE_RAMP_RIGHT` (`StructureRamp`) |

Any other value, a non-string, or a value on a type the table does not allow is
an error in the runtime builder, the editor and the schema alike. A material
does not replace the sprite fields: `sheet_name` and `frame_name` stay required
and validated, so keep them `sprite_sheet_1` / `square_white_4`, or the ramp
frame for a ramp. Collision, scoring faces and geometry do not depend on it.
Decorative backdrop objects take no material; they are ordinary `Visual`s whose
`sheet_name` is `terrain` and whose `frame_name` is one of its props.

## Grey tints and resource identifiers

Authored tints are grey; scenery takes its muted hues only from the `terrain`
sheet's texels, and no team hue appears until players add paint. `colour` accepts six hex digits
`RRGGBB` without `#`, case insensitive for hex digits, with equal red, green and
blue values, such as `404040`, `808080` and `DCDCDC`. Named colours use the
engine's exact uppercase spelling:

`BLACK`, `DARK_GRAY`, `DARK_GREY`, `DIM_GRAY`, `DIM_GREY`, `GAINSBORO`, `GRAY`,
`GREY`, `LIGHT_GRAY`, `LIGHT_GREY`, `SILVER`, `WHITE`, `WHITE_SMOKE`.

The schema recognizes these names and six-digit hex syntax. Equality of the
three hex channels is checked by the game/editor, so a coloured hex value can
pass the schema and still fail content validation. Tinting art grey does not
validate that every source texel follows the art direction.

Hues come only from atlas texels: the `terrain` atlas carries the stage
palette, and every JSON `colour` tint stays grey, with `WHITE` on `terrain`
backdrop visuals so their texels show unchanged. A collision object with a
`material` still needs a grey `colour`; neither a material nor a backdrop prop
makes a coloured tint valid.

`sheet_name` is a sprite-sheet manifest identifier; `frame_name` names a frame
inside that sheet, not an animation strip or a file path. The template uses
`sprite_sheet_1` / `square_white_4`. `icon_frame` resolves in `sprite_sheet_1`
independently. Runtime startup constructs the objects and resolves these names
after the manifest has loaded. The level editor warns about names that do not
resolve in the source manifest's sheets and banks
([Resource names](level-editor.md#resource-names)); it does not load the
resources, and startup stays the authority. Registration still requires both the manifest and runtime
asset list; follow [Play the level](level-editor.md#play-the-level).

## Sound

`sound_bank_name` is required; the current shipped bank is `paint_exe`. The
optional pairs are independent:

| Pair | Meaning |
|---|---|
| `music_name` and `music_volume` | Looping arena music. Omit both for an arena with no looping music. |
| `end_sting_name` and `end_sting_volume` | One owned nonlooping sting at time-up. Omit both for no sting. |

A half-present pair is rejected, including when its provided value is null.
The gain is authored gain multiplied by the saved music preference; the shared
presentation mix controls fades and ducking. Both gains must be finite and in
`0..1`, in the runtime, the editor and the schema alike, so a gain above one is
refused at load rather than clamped by playback.

Names resolve as **effect instance** identifiers in the bank's JSON definition,
not raw wave indices or audio paths. For example, the template uses
`music.close_quarters` and `music.end_sting.close_quarters`; the other accepted
arena suffixes are `king_of_the_hill` and `turbulence`. Movement Course omits
both pairs. The optional missing-bank substitution remains supported: an absent
bank becomes silent and cannot establish whether named cues exist in the real
bank. See the [audio inventory](audio-inventory.md).

## Fixed scored slabs

`FixedSlabPaintable` is a bounded static solid: a 160-unit horizontal span,
80-unit rise and **24-unit normal thickness**. It accepts the two mirrored
orientations below, rather than arbitrary angles, sizes or moving rotation.

```json
{
    "type": "FixedSlabPaintable",
    "name": "shoulder_a",
    "collision_type": "STRUCTURE_FIXED_SLAB",
    "material": "eraser",
    "slab": { "x": 1240, "y": 880, "rise": "right" },
    "paintable_faces": { "left": true, "top": true, "right": true, "bottom": true }
}
```

`x,y` names the **leftmost endpoint of the top walking edge**. `right` ends
at `(x+160,y-80)`; `left` ends at `(x+160,y+80)`. Both endpoints must remain
finite game floats within -65,536 to 65,536 and retain the fixed footprint
after conversion. The dedicated eraser treatment resolves its own resources;
this type does not require the ordinary sprite sheet, frame or colour fields.
Unknown metadata survives editor round trips. Motion, drift, arbitrary rotation,
alternate rectangle/triangle geometry and slab size/thickness requests are
refused. Intersecting slabs need a separate shared-boundary contract and are
refused; ordinary flat terrain can meet or overlap their ends.

The top is supported by its actual sloping segment. End caps and underside
remain solid; the existing paint-pass ramps keep their original behavior.
Flying paint uses four-axis continuous contact selection, including growing
rectangular projectile volumes and mixed static/moving cover. Paint tiles live
in the slab's local frame. Score uses their authoritative local areas, while
public surface samples publish transformed corners. Friendly repaint retains
credit and enemy capture transfers it. Exposed faces are clipped against actual
flat terrain; reciprocal flat faces use the slab polygon, never its bounding
box. Adjacency uses the authored top endpoints and normal extrusion, correcting
only OBB reconstruction drift within four coordinate float ULPs. Obstacle
coordinates and real authored gaps are preserved; larger corrections fail.
The collider and published tile corners retain their represented OBB pose.
The slab's material, paint, splash, culling and static underside shadow
follow that fixed frame. Sloped gravity runoff has no separate policy yet.

The editor offers both fixed orientations, actual outline/top-edge guides,
translation and rise edits. Resize is refused. These contracts do not qualify
a placement's routes, competitive value, native composition or attended play.

## Stations and validation limits

Optional stations are `{ "name": "centre", "x": 1474, "y": 1075 }` entries for
Debug `goto`. Names must contain one or more lowercase ASCII letters, digits
or underscores; `spawn_a` and `spawn_b` are reserved, and authored names must be
unique. Points use the same top-left convention and inclusive out-of-bounds
check as spawns. After authored stations, the game appends `spawn_a` and
`spawn_b` for each team's first spawn. Stations do not create geometry or
affect normal gameplay.

The schema covers required keys, scalar/array types, positive rectangle sizes,
spawn counts, type/collision combinations, material values and the collision
types each allows, paint-face booleans, sound-pair
presence, sting gain and station-name syntax/reserved names. It leaves these
checks to their owners:

- Runtime: finite bounds/object rectangles after float conversion; TLBR ordering;
  spawn/station containment and spawn halves; duplicate station names; grey
  hex channel equality; assets and drawable text; and the engine's
  anti-tunnelling bound ([tunnelling_limits.h](../game/tunnelling_limits.h)):
  a stage is rejected when a player at its farthest step would travel at
  least its own extent plus an object's width or height, so it could pass
  through that object. That step is `tuning.player.max_velocity`, widened
  on x to `x_initial_velocity` if that is larger (the first step of input is
  unclamped) and on y by `drop_down_velocity` (a drop-through adds it after
  the clamp). The `tunnelling`
  acceptance case prints each shipped stage's margin. Flying paint is exempt
  because each shot's whole path is tested against the geometry every step.
- Editor: its additional nonempty-string, numeric and upright-ramp checks,
  and warnings, not refusals, for resource names that do not resolve.
  Unknown metadata and stations remain preserved even when not exposed as
  dedicated editor controls.
- Play/review: supported spawns, routes, weapon roles, fair side swaps, paint
  budgets, readability, timing and frame pacing. Format validity supplies none
  of those acceptance results.

Schema ranges apply to JSON numbers before conversion to game floats; rounding
near a numeric boundary can therefore differ from a runtime comparison. Keep
runtime validation in the authoring workflow even after a schema pass.

## Validation and authoring workflow

Point a JSON editor at [level.schema.json](level.schema.json), then open the file
in ColourWarsEditor and save a copy. Register an intended runtime stage using
the manifest and asset-list steps in [Play the level](level-editor.md#play-the-level).
Rebuild and run it in the game: the schema does not resolve its assets, the
editor only warns about names, and neither establishes playability.

From an x64 Visual Studio Developer shell at the repository root:

```powershell
./scripts/run-acceptance.ps1 -Preset x64-debug
./scripts/check-level-editor.ps1 -Preset x64-debug
```

Repeat with `x64-release`. The `level_format` consumer case uses bundled
RapidJSON Draft-04 validation and the actual `LevelLoadedInfo`/`LevelObjectBuilder`
paths with CPU resource metadata; it checks source and staged copies of all
four shipped levels plus the template, with an untagged legacy clone of each.
Its fixtures also exercise invalid
versions, structural failures, unknown and mismatched materials, unknown metadata, rectangle precedence and
runtime checks beyond the schema. It also holds the editor's resource-name
catalog to the engine's sprite-sheet and sound-bank loaders. Editor acceptance checks version compatibility,
new-document versioning and template/unknown-field round trips.

On 2026-10-15, Debug and Release production builds passed under `/W4 /WX`, all
25 consumer cases passed on each preset, and both editor document suites and
hidden editor launch/selection/toggle/close checks passed. Final focused checks
also reject unsupported, string and decimal schema version tags. Real-bank game
startup and graceful shutdown pass in both builds. The retained
[validation evidence](../out/validation/level-format) accompanies the ordinary
logs under `out/acceptance/x64-debug` and `out/acceptance/x64-release`. The
[content comparison](../out/validation/level-format/content-compatibility.json)
records that adding `format: 1` was the only data change in the four shipped
levels.

The schema is an authoring and acceptance artifact. Normal builds and runtime
loading add no Python or external schema-package dependency. Manual editor and
saved-stage play checks (`ED#5`), physical multiplayer sessions (`ROAD#2`) and
visible uncontended frame-pacing/hardware qualification (`POL#25`) remain open.
