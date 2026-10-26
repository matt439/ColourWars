# Level editor

`ColourWarsEditor.exe` is a separate Windows desktop application for editing
the game's level JSON files. It preserves fields that the editor does not know
about. The canvas
shows schematic geometry, type colours, paintable edges, ramps, spawn points and
bounds. It has no textures, audio or game simulation; test the saved level in
the game to judge how it plays.

## Build and open

Use an x64 Visual Studio Developer shell with `VCPKG_ROOT` set, as for the game:

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug
./out/build/x64-debug/editor/ColourWarsEditor.exe
```

You can also pass a level path as the first argument:

```powershell
./out/build/x64-debug/editor/ColourWarsEditor.exe ./game/content/levels/turbulence.json
```

The editor's file dialogs start in `game/content/levels/` in this checkout.
Edit source files there: the copies beside `ColourWars.exe` are replaced by
the next build. The editor does not read or write the player's save.

Open a level with **File > Open** or start with **File > New**. Save As lets
you keep an existing arena while experimenting with a copy. The title marks
unsaved changes, and opening another level, creating one or closing the editor
asks what to do with those changes.

New levels declare the top-level integer `"format": 1`. Files that omit
`format` load as legacy version 1; opening them preserves the original document,
and a successful save adds the version field. An explicit version must be the
integer `1`: unsupported versions and other types, including `1.0` or `1e0`,
are rejected before level fields are decoded. Version errors identify the file
and `format`; a rejected load or failed save preserves the open document,
its path and undo/redo history. Unknown fields survive the legacy upgrade.
See [the level format reference](level-format.md) for the file contract.

## Editing

Select an object in the list or click it on the canvas. Drag its body to move
it, or drag the handle at the bottom-right corner to resize it. Ramp editing
moves or scales all three vertices together. Spawn markers can be moved;
their boxes use `player.size` from source `game/content/tuning.json`.
Fixed scored slabs move without a resize handle; their footprint is fixed.

Use the palette to add paintable platforms, solid structures, jump-through
platforms, [horizontal moving paintables](#moving-structure-authoring-gapd5),
left/right ramps, [fixed scored slabs](#fixed-scored-slab-authoring-gapd5)
and decorative visuals. Team spawns and the four
bounds rectangles are also selectable and editable. The level entry exposes
the stage name, menu icon and sound settings. New levels use the replacement
`paint_exe` bank, `music.close_quarters` and its matching ending sting. Omit both
music name and volume for a music-free arena; `end_sting_name` and
`end_sting_volume` are independently optional as a pair. Movement Course omits
both pairs. Music and ending-sting gains must be 0..1. The editor warns about a bank,
music or sting name the bank does not define ([Resource names](#resource-names));
it does not audition them.

The property inspector lists individual fields for the current selection.
Choose a field, edit its value and click **Apply**. Numbers use a decimal
point; boolean fields use `true` or `false`. Text fields are plain text.
An invalid change displays an error and keeps the preceding valid document.
Enter also applies the current field. Apply or revert pending text before
choosing another property row. Selecting another object or saving applies
the pending value automatically; an invalid value blocks that action until
it is corrected or discarded with Escape or **Revert**.
Use the inspector for exact coordinates, paintable faces, sprite-sheet and
frame names, colours, and other object properties.

### Materials

Collision objects carry an optional `material`, the terrain-atlas look the game
draws them with ([Materials](level-format.md#materials)). The palette gives each
new collision object its usual one: a solid `Structure` is `ground`, a
jump-through `ruler`, a paintable platform `pencil` and either ramp
`set_square`. Decorative visuals get none. Duplicates keep their material.
Fixed scored slabs require their dedicated `eraser` material.

The inspector's `material` row lists, in its label, the values the object's
collision type allows, such as `material (ground, pencil, eraser, paint_tin)`
on a paintable platform. It is a text field checked on Apply: an unknown name,
or one the collision type does not allow, is refused like any other invalid
edit. An object without a material still shows the row, empty; applying a value
adds the field, as the Random setting does, and undo restores the omission.
Applying an empty value removes the field, returning the object to the legacy
stretched sheet/frame draw, except on a fixed slab whose material is required.
The inspector adds no other missing field, and
never a material on a decoration. Changing `collision_type` to one the current
material does not fit is refused too: change or clear the material first.
The canvas stays schematic and does not draw materials; play the stage to see
them.

| Action | Control |
|---|---|
| Select and move | Left click and drag |
| Resize selected geometry | Drag its bottom-right handle |
| Pan | Middle mouse button and drag |
| Zoom | Mouse wheel |
| Fit the arena in the viewport | Home in the canvas, or Fit arena button |
| Frame the current selection | F in the canvas or object list |
| Return to the match overview | Match view button |
| Move selection by the grid spacing | Arrow keys in the canvas |
| Grid spacing | Choose 1, 10, 25, 50 or 100 |
| Temporarily bypass grid snapping | Hold Shift while dragging |
| Duplicate selection | Ctrl+D |
| Delete selection | Delete |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| New / open / save | Ctrl+N / Ctrl+O / Ctrl+S |
| Save as | Ctrl+Shift+S |

The document keeps undo/redo history for edits. A successful save marks the
current revision as saved; undoing back to that revision clears the unsaved
indicator. Failed loads and saves keep the current document and its history.
Saves replace the destination only after writing the complete new JSON.
The initial view uses the level's zoom-out finish bounds.

## Blockout aids

**View > Blockout aids (reload tuning)** toggles the guides. They start enabled;
turn them off and on to reload source tuning after editing it outside the editor.
Document edits also refresh tuning and diagnostics. These reads do not edit the
level or the player's save. A missing or invalid aid file displays a diagnostic.

- **Cyan edges** show exposed scoring faces. Flat structures, including
  jump-through platforms, bury faces reached from their outward side. Ramps and
  decorations do not. Moving anchors never become permanent static blockers;
  their own scoring faces stay local to the body. The editor and game share the interval calculation in
  `game/paint_face_geometry.h`. With aids off, cyan shows authored faces instead.
  Fixed slabs clip their diagonal faces and neighbouring flat faces against
  the actual adjacency polygon, using the shared slab paint-geometry helper.
- **Scoring area** shows the whole level and the part inside the selected
  rectangle. Select an object or one of the bounds rectangles to choose a
  region; selecting the level shows only the total. Area clips the inward paint
  strips using current `paint.thickness`, in world units squared. Corners and
  overlapping strips contribute separately, as in runtime scoring. It measures
  available scoring surface, without assigning teams or predicting reachability.
  Movers appear at their initial phase, labelled in the summary. Their local
  strip dimensions and area are measured before translating the displayed
  positions, so changing phase does not change the available score.
  Slabs retain local face areas and display actual world quads; selection
  regions clip those quads, including their empty bounding-box corners.
  During a drag, measurements and scoring edges wait for the committed drop.
- **Purple/gold boxes** show the recorded tap/held jump envelope in both
  directions from the top centre of the selected geometry, or the feet of a
  selected spawn. They bound maximum rise and same-height horizontal travel;
  the top corners are not a trajectory. The data and matching movement-tuning
  snapshot live in `editor/movement-baseline.json`, from the actual Player/Scene
  [measurements](polish-movement-course.md#measured-baseline-and-current-evidence).
  These assume full running speed and a fixed timestep, exclude collisions and
  acceleration from rest, and do not guarantee a gap or landing. A changed
  movement snapshot hides the boxes and asks for new measurements; spawn sizes
  and scoring diagnostics still follow current tuning. After changing movement
  code, rerun the movement acceptance measurement before updating the baseline.

`ED#1` implemented on 2026-10-15. Debug and Release editor document checks cover
overlapping blockers, buried/touching faces, ramps versus flat collision classes,
disabled faces, clipped region area, undo and changed-tuning invalidation. Hidden
GUI checks select a spawn, toggle/reload the aids and frame the selection. Both
game consumer acceptance suites also pass with the shared geometry, reproducing
the baseline jump measurements. These checks do not close the manual canvas or
saved-stage play checks in `ED#5`.

## Resource names

The game refuses a stage at startup when a name it resolves is missing
(`LevelLoadedInfo::validate_content` and `LevelObjectBuilder::validate_objects`).
The editor checks the same names against the same source files, so a typo shows
while you edit rather than at the next game launch:

| Field | Must name |
|---|---|
| `icon_frame` | a frame in `sprite_sheet_1` |
| `sheet_name`, when the object uses generic sprite fields | a `sprite_sheet` listed in `game/content/manifest.json` |
| that object's `frame_name` | an entry in that sheet's `sprite_frames`; an animation strip is not a frame |
| fixed slab's dedicated material | sprite sheet `terrain` and its `swatch_eraser_fill` frame |
| `sound.sound_bank_name` | a `sound_bank` listed in the manifest |
| `sound.music_name`, `sound.end_sting_name` | an effect of that bank: every wave when `create_effect_instance_for_each_wave` is true, plus each `sound_effect_instances` name |

`colour` is not in the table because it is already a structural check: only the
game's grey names and `RRGGBB` greys are accepted, and anything else is refused
like any other invalid edit.

These checks only warn, because a stage is often saved halfway through adding its
art. The status line counts the unresolved names and shows the first one. Save
and Save As list them and ask before writing; No is the default. The editor
rereads the manifest, its sheets and its banks from source `game/content/` on
each refresh, as it does tuning. If the manifest or a listed file cannot be read,
the status line says *Resource names unchecked* and only the readable names are
checked. Audio names are checked against the tracked `paint_exe.json`
definition, not the `.xwb`. They are still checked when the bank is absent, even
though the game then substitutes its silent fallback, which accepts any name.

The game's startup validation is still the authority. The editor's catalog
(`editor/content_catalog.cpp`) is a second reader of the sheet and bank formats,
so the game-side `level_format` consumer case holds it to the engine's loaders on
the staged content: every frame it lists resolves in the engine's sprite sheet,
every animation strip it leaves out is refused as a frame, and its effect names
equal the engine's sound-bank definition.

`ED#3` implemented on 2026-10-17. The editor document checks cover every
shipped level, the template and new-level defaults without warnings. They
mutate each field in the table, including a strip name used as a frame and a
frame borrowed from another sheet, and expect exactly one line naming the
field and value. They also check undo, sheet scoping, the wave rule and a
partial or missing content folder. A hidden GUI launch opens a level with an
unknown `icon_frame` and reads the warning from the status line. The save
prompt is a modal box, so it belongs to the manual pass in `ED#5`.

## Play the level

For an existing stage, save its source JSON, rebuild the game and choose the
stage from the game menu. The build copies the revised file beside the game.

A new file is not automatically a menu stage. To register it:

1. Save it as `game/content/levels/<name>.json`.
2. Add `<name>` without the extension to the `level` asset group's `names`
   in `game/content/manifest.json`.
3. Add `levels/<name>.json` to `COLOURWARS_RUNTIME_ASSETS` in
   `cmake/runtime_assets.cmake`, shared by development staging and Release.
   Configure checks that these two lists agree.
4. Configure/build again, then start the game and choose the new stage.

The file's `display_name` is the stage's label on the stage-select page, which
also draws a live preview of the arena itself.
Stage names use the glyphs supported by the game's compiled fonts; unsupported
characters such as emoji are rejected before saving.
`icon_frame` is required legacy icon metadata that no screen draws today; it
must still name an existing frame in `sprite_sheet_1`. Object sprite-sheet
and frame names and music names must also exist in the game's assets. The
editor warns about any that do not ([Resource names](#resource-names)); the
game's startup validation resolves the actual resources.

An optional top-level `include_in_random` boolean defaults to `true`. Set it to
`false` in the level entry's property inspector for an experimental course that
should remain explicitly selectable without appearing in Random matches. When
the JSON omits the field, the inspector shows its default `true` without changing
the document. Applying a changed value adds the field; undo restores its omission.

Each team must retain at least four spawns inside `out_of_bounds`. The other
bounds control camera limits and the match's zoom-out presentation. Resizing
`out_of_bounds` must keep every spawn inside it. Objects outside those bounds
can be intentional boundary geometry and are allowed.

## Acceptance checks

`ED#5` is `DECISION`: the remaining GUI and saved-stage gameplay checks need an
attended session and a recorded result. The automated checks below establish
document behaviour and hidden-window operation, not that manual acceptance.

```powershell
./scripts/check-level-editor.ps1 -Preset x64-debug
./scripts/check-level-editor.ps1 -Preset x64-release
```

These build and run a native document-model check followed by a hidden GUI
launch/open/respond/close check. Neither creates a game graphics/audio device.
They cover the shipped levels, the canonical template, unknown-field preservation, both rectangle
encodings, ramps, the palette, undo/redo and save
state, Unicode paths and failure recovery. Evidence and generated test levels
remain in `out/acceptance/<preset>/level-editor/`. They also check the optional
Random setting's default, insertion, validation, undo/redo and save/reopen,
and each palette kind's default material, refusal of unknown and mismatched
materials (in edits and loaded files), and inserting and clearing a material.
They check the resource-name warnings against source content, and a third
hidden launch reads the status line of a level the game would refuse.
The document checks also cover the shared current version, legacy save upgrades,
unknown-field preservation, strict format type/version rejection before other
field decoding, filename diagnostics, failed-upgrade recovery and retained
undo/redo branches across a successful upgrade.
Use `-SkipBuild` only after
building both `colourwars_editor_acceptance` and `ColourWarsEditor` for the
current source.

On 2026-10-15, both presets passed the document checks (including the optional
Random setting) and hidden GUI launch/open/respond/close checks. Evidence:
`out/acceptance/x64-debug/level-editor/088ef35b371047ee97c715f163c61008/`
and `out/acceptance/x64-release/level-editor/2ca17df03ee24d7683cb8131b44fb245/`.
The manual GUI and saved-stage play checks below remain open (`ED#5`).

For a manual GUI check, open each shipped arena, select a platform, move and
resize it, edit a field, duplicate/delete, undo/redo, and save a copy. Reopen
the copy and check its coordinates. Also check wheel zoom, middle-button pan,
grid snapping and the unsaved-changes prompt on New, Open and Close. Give a
copy more than eight unknown resource names and check that Save and Save As
list the first eight and "and N more", that No is the default and writes
nothing, and that answering Yes to the unsaved-changes prompt on Close and then
No here keeps the editor open. Rebuild
and play a saved stage to check collisions, paintable faces, spawns, camera
bounds and ramps. Document acceptance does not establish those play qualities.

## Moving-structure authoring (`GAP#D5`)

**Implemented and qualified 2026-10-26.** The palette's
**Moving paintable block** and **Moving paintable platform** create the existing
solid and jump-through horizontal paintables. They write `StructurePaintable`
with the optional [motion record](level-format.md#horizontal-paintable-motion),
using `pencil` and `ruler` materials respectively. No new motion kind is added.

The rectangle is the **leftmost anchor**. Dragging, nudging, resizing and exact
coordinate edits change that anchor; horizontal travel remains an absolute
distance and the period and phase remain unchanged. The inspector exposes
`motion/type` (`horizontal`), `motion/travel_x` in world units,
`motion/period` in round-trip seconds and `motion/phase` in cycles [0,1).
New movers start at travel 120, period 6.4 and phase 0 as editable authoring
defaults, not an accepted arena placement or balance choice.

The filled body shows its initial phase. A white dashed outline marks the
anchor, blue dashes the complete travel envelope, and gold dots the expanded
standing corridor. Selection and resize handles edit the anchor; clicking the
initial body also selects that object. **F** frames the complete travel and
standing guides. Drag guides are provisional until the drop commits. These
are geometry guides, without simulated riding or a route-feasibility claim.

**Ctrl+D** retains motion, material and unknown metadata and places the copy
below the original's complete body/standing corridor. It does not use the
ordinary object's overlapping 50-unit offset. If the new envelope leaves Scene
bounds or intersects another structure, duplication is refused. Anchor, motion,
static-geometry and bounds edits validate the complete candidate before changing
the document or undo/redo branch. Save/reopen retains unknown root, object and
motion metadata, rectangle/TLBR encoding and ignored alternative geometry.

The application supplies source `game/content/tuning.json` for player size;
the registration helper supplies the same repository context. Tools using the
document API pass that path to `LevelDocument(tuning_path)`. The default
constructor still works for static documents, but cannot load or add a mover
without this explicit context. Mover validation rereads current `player.size`
and shares the runtime's float field, travel and standing-envelope arithmetic
through the engine-free `game/horizontal_motion_contract.h`. Changes to source
size can therefore invalidate a formerly usable corridor, including an undo
candidate. No guessed player size is substituted.

The complete platform and standing envelopes must lie inside Scene bounds and
clear the other flat/ramp structures and mover paths. Full actual-player
rate/displacement admission remains authoritative in production `LevelBuilder`;
passing editor geometry validation does not qualify that budget or gameplay.
Scoring diagnostics keep mover dimensions and area in anchor space and translate
only guide locations, including at fractional float boundaries. Gameplay's
per-tile float accumulation can round differently from continuous editor strips;
neither total changes merely because initial phase changes.

The document checks cover both palette kinds, properties, phase/standing guides,
transactional rejection, anchor move/resize, duplication, undo/redo, changed
source dimensions, metadata/TLBR preservation and save/reopen. The CPU
`moving_structure` case compiles the actual editor model and passes its saved
stage through production `LevelBuilder`, with material/scoring checks and Debug
Recorder/Player reconstruction. Debug/Release editor builds, document checks and
`moving_structure` passed after the fractional-area fix. Debug includes 180
exact editor-save replay updates; Release explicitly excludes developer replay.
Receipts: `out/validation/overnight-20261026/phase4/` (`editor-*` and `mover/`).
No new GUI, native-pixel or human acceptance is claimed here.

Human GUI handling and saved-stage play remain `ED#5`; Turbulence
route/readability acceptance remains `POL#21`.

## Fixed scored-slab authoring (`GAP#D5`)

The **Scored slab rising right** and **Scored slab rising left** palette entries
write `FixedSlabPaintable` with `STRUCTURE_FIXED_SLAB` collision and the required
`eraser` material. The fixed footprint spans 160 horizontal units, rises 80
units and has 24 units of thickness measured along the slab normal. It is a
solid scored slope, distinct from the existing unscored paint-pass ramp.

`slab/x` and `slab/y` locate the top endpoint with the smaller world X;
`slab/rise` selects `left` or `right`. A right-rising slab at `(1240,880)` ends
at `(1400,800)`; its mirrored left-rising counterpart starts at `(2600,800)`.
Endpoints must remain finite and within 65,536 world units. Dragging, nudging
and rectangle-coordinate edits translate the body; resizing is refused. The
canvas draws the actual sloping polygon and top support segment, and selects
its body without treating an empty bounding-box corner as solid. Duplicates
move below the complete body. Intersecting slabs are refused using the same
strict positive-overlap test as runtime; a separated pair with overlapping
bounding boxes remains valid.

The four paint-face switches retain local face identity. Areas are measured
before transforming their strips into world quads. Shared canonical paint
adjacency corrects only checked OBB reconstruction drift at an authored join;
the collider, visible tile pose and guide stay at their represented coordinates.
It does not round an obstacle or erase a real small authored gap. Arbitrary
rotation, resizing, motion and combined shape records are refused. Unknown
metadata survives edits, duplication, undo/redo and save/reopen. The new type
uses its dedicated material rather than generic sheet/frame/colour fields.

Debug and Release game/editor builds, the full editor document suite and three
hidden editor launch/status/close checks per preset pass. New document checks
include mirrored
normal extrusion, translation and immutable size, both-direction duplication,
transactional pair/field refusals, saved metadata/schema round trips, actual
polygon scoring, reciprocal face clipping, exact buried-cap zero and retained
`0.00004` authored gaps. The CPU `fixed_slab` case separately loads an actual
editor-saved trial through fresh 1/4-player public builders. Receipts are under
`out/acceptance/x64-debug/level-editor/30f6ffeacf6a466a904168cdb7192c11/`
and the matching Release receipt `78fd5457eb954c6da74d8b73355ac283`, alongside
`out/validation/overnight-20261026/phase11/` (`slab-v4/x64-debug`, `regression/x64-release`).
These checks do not establish attended GUI handling, saved-stage play, native
composition or competitive value.

## Registration assistance (`ED#4`)

`ColourWarsRegisterStage` previews and explicitly registers a level already
saved directly under `game/content/levels/`. Build the opt-in tool, then preview
the two additions before choosing to apply them:

```powershell
cmake --build --preset x64-debug --target ColourWarsRegisterStage --parallel 2
out/build/x64-debug/editor/ColourWarsRegisterStage.exe --repo . --stage game/content/levels/my_stage.json
out/build/x64-debug/editor/ColourWarsRegisterStage.exe --repo . --stage game/content/levels/my_stage.json --apply
```

The target is excluded from normal builds and release packaging. Relative stage
paths are relative to `--repo`; wide Windows paths are retained. `--help` explains
the arguments. Exit codes are 0 for success/no-op, 2 for usage errors and 1 for a
validation or apply failure. Ordinary Save/Save As remains independent, and the
editor Help names `cmake/runtime_assets.cmake` and this separate helper.

Preview writes nothing. It checks the existing level's structure and references
against a complete content catalog, verifies that the complete manifest/runtime
asset lists already agree, then shows the exact added name and runtime path.
The basename uses ASCII letters, digits, `_` or `-`, with no leading hyphen or
Windows device name. The file must have a `.json` extension and be directly in
this source tree's levels directory; linked/reparse paths are refused. Duplicate
or case-alias names, one-sided registration and unsupported/nonliteral CMake list
syntax are diagnosed rather than guessed at. An entry already present in both
lists is a validated no-op.

Apply revalidates the proposal and unchanged stage/source bytes. It prepares and
flushes both candidates and both original backups before replacing either list.
Only the two entries are inserted; surrounding bytes, line endings and unrelated
metadata remain. If the second replacement fails, the first is restored while
it is still this operation's candidate. A failed rollback retains both originals
at the recovery paths printed in the error; inspect both lists before retrying.
Each replacement is atomic; the pair is not crash-atomic. The saved stage itself
is never rewritten.

Configure/build the game after registration to copy the stage beside the
executable. The game's startup/content validation remains authoritative;
registration does not establish route, spawn or gameplay acceptance (`ED#5`).
GUI integration can follow actual use of the command-line helper.

The document acceptance case also checks exact LF/CRLF and BOM-preserving
appends, malformed/partial/aliased lists, resource validation, stale inputs and
proposals, first/second replacement failures, locked/read-only files and named
original backups after a failed rollback. The separate integration check uses
a fresh scratch repository and only the reviewed runtime assets:

```powershell
./scripts/check-stage-registration.ps1 -Preset x64-debug
./scripts/check-stage-registration.ps1 -Preset x64-release
```

On 2026-10-18, both presets passed the editor document and three hidden-window
checks, then preview/apply/no-op and rejection checks, including a directory
junction. Each scratch registration changed only the manifest and runtime list,
passed the existing CMake agreement check, staged all 29 registered assets
byte-for-byte, and passed isolated game startup with the published audio bank
and per-monitor v2 DPI awareness. No hardware skip was used. Evidence is under
`out/validation/stage-registration/`; the integration runs are
`integration/x64-debug/6821efb469a946b89b2b943419b58153/` and
`integration/x64-release/3850fed0b130438a814d23c6ee7817aa/`.
These automated checks complete `ED#4`; the attended editor and saved-stage
play review remains `ED#5`.

## Movement Course icon (`ED#6`, retired)

Retired 2026-10-18 during the backlog audit: `icon_frame` is required legacy
metadata, and no current screen displays it. Stage Select draws the actual
arena preview. Keep the existing valid frame rather than commission invisible
artwork. A distinct icon becomes work only if an accepted screen needs one.
