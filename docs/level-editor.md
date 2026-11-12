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
the next build. **File > Save to My Stages** instead saves into the
player's own stage folder, which the game reads without a rebuild ([Play the
level](#play-the-level)). The editor does not read or write the player's save.

Open a level with **File > Open** or start with **File > New**, which opens a
copy of [editor/template.json](../editor/template.json) (compiled in, less its
`_comment`): a walled floor, eight spawns, one station and `include_in_random`
`false`. Save As lets
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

How each `ED` slice was built and checked, from the 2026-10-28 shortlist to
`ED#27`, is in [the editor's record](level-editor-record.md); this page
describes the editor as it is.

## Editing

Select an object in the list or click it on the canvas. Drag its body to move
it, or drag the handle at the bottom-right corner to resize it. Ramp editing
moves or scales all three vertices together. Spawn markers can be moved;
their boxes use `player.size` from source `game/content/tuning.json`.
Fixed scored slabs move without a resize handle; their footprint is fixed.
Arrow keys cancel an in-progress mouse gesture and nudge the committed position.
Ramp selection follows the triangular body, so its empty corner lets you
select an object beneath it.
Ctrl+click cycles through overlapping objects without starting a drag. Repeated
clicks wrap through the same hit order; empty space retains the selection.

Use the palette to add paintable platforms, solid structures, jump-through
platforms, [horizontal moving paintables](#moving-structure-authoring-gapd5),
left/right ramps, [fixed scored slabs](#fixed-scored-slab-authoring-gapd5)
and decorative visuals. Team spawns and the four
bounds rectangles are also selectable and editable, and so are the level's
[stations](level-editor-record.md#station-editing-ed26) (goto targets), which the palette also adds. The level entry exposes
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

Imported metadata containing an embedded NUL character is shown as escaped,
read-only text. The inspector keeps those values and keys intact when you
save or change selection. You can select and copy the preview; edit the JSON
file externally to change it. String previews use quotes and escapes such as
`\u0000` and `\\`, while numbers and booleans keep their usual form.

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
| Select overlapping objects without moving them | Ctrl+click repeatedly |
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
| Mirror copy across the arena centre ([rules](level-editor-record.md#mirror-copy-across-the-arena-centre-ed24)) | Ctrl+Shift+D |
| Copy / paste one object, including into another level ([rules](level-editor-record.md#copy-and-paste-between-documents-ed27)) | Ctrl+C / Ctrl+V |
| Delete selection | Delete |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| New / open / save | Ctrl+N / Ctrl+O / Ctrl+S |
| Save as | Ctrl+Shift+S |
| Save to My Stages ([play without a rebuild](#play-the-level)) | Ctrl+M |

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
  `game/contracts/paint_face_geometry.h`. With aids off, cyan shows authored faces instead.
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
- **Symmetry** gives the scoring area either side of the arena centre and lists
  spawns and objects without an `ED#24` mirror partner. See
  [Symmetry report](level-editor-record.md#symmetry-report-in-the-blockout-aids-ed25).
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

To play a level without rebuilding, choose **File > Save to My Stages...**
(Ctrl+M). The save dialog opens in `%LOCALAPPDATA%\ColourWars\stages\`, beside
the save, which is the folder the game reads player-made stages from. It
offers the open file's name. A copy of a shipped stage is offered `my_<name>`,
and a new level `my_stage`. The editor then applies the game's own rules
(`game/contracts/user_stage_contract.h`, shared with the game's scan). The name must be
1 to 64 ASCII letters, digits, `_` or `-`, not starting with `-`. It may not
be a shipped stage's name in any case; the names come from the source
manifest. The file must go directly in that folder: use Save As for any other
folder. If one of your stages already has that name (in any case), the editor
asks whether to replace it; No is the default. The replacement keeps the
existing file's spelling. The folder is created only by this command. If you
cancel, or the name is refused, a folder the command created is removed again
while it is still empty. After the save, the open document edits the saved
file, so Ctrl+S saves there too. Without `LOCALAPPDATA` the command refuses:
the game would then read `save/stages` beside its own executable, which the
editor cannot know. Like the other saves, it never touches the player's save.

The game reads that folder at startup and again each time Stage Select opens
from the main menu. A stage saved while the game runs joins the list, and an
edited one plays its new version. You can also copy a file there by hand. A
user stage appears in Stage Select after the shipped stages, with "Built by
you." in the briefing, and `+stage <name>` starts it from the Debug command
line. It joins Random only when its file says `"include_in_random": true`
(the author's 2026-11-05 call for `ROAD#16`); without the key, or with `false`,
it stays out. The game checks it as it checks a
shipped stage, including the format's size [limits](level-format.md#limits),
which the editor applies at open, on each edit and at save. A file that fails prints one `user stage refused: <file>:
<reason>` line on stderr and is left out; the game goes on without it. Main-menu
Stage Select also shows an issue count: Down while browsing opens the retained
filename/reason details, with issue/page controls and a return to the same
arena choice. See
[User stages](feature-roadmap.md#user-stages-road16).

To ship a stage with the game, register it instead. A new file in
`game/content/levels/` is not automatically a menu stage. To register it:

1. Save it as `game/content/levels/<name>.json`.
2. Add `<name>` without the extension to the `level` asset group's `names`
   in `game/content/manifest.json`.
3. Add `levels/<name>.json` to `COLOURWARS_RUNTIME_ASSETS` in
   `cmake/runtime_assets.cmake`, shared by development staging and Release.
   Configure checks that these two lists agree.
4. Add `"<name>": "<its display_name>"` to `menu_text.stage_names` in
   `game/content/presentation.json`, the copy a locale translates
   ([Stage names](localisation.md#stage-names)). Startup refuses a manifest
   stage without an entry, an entry without a stage, and an entry whose
   English is not the level's `display_name` byte for byte.
5. Configure/build again, then start the game and choose the new stage.

[Registration assistance](#registration-assistance-ed4) makes steps 2-4 in
one previewed operation. Locales are not touched: the new stage shows its
English in every locale until one names it, and the `qps-ploc` test locale
brackets it only once `"<name>": "[<display_name>]"` is added to
`game/content/locales/qps-ploc.json` by hand
([Stage names](localisation.md#stage-names)). No check requires that.

The file's `display_name` is the stage's label on the stage-select page, which
also draws a live preview of the arena itself. For a registered stage it is
also the English of its `stage_names` entry: change both together, or the game
will not start. The status line says so when a source level the manifest names
no longer matches its entry. A Save to My Stages copy keeps its
`display_name` and is never in `stage_names`, so it shows that name,
untranslated, in every locale.
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
The game reads a missing field as `true` only for a shipped stage: a stage
saved to My Stages joins Random only when the field is present and `true`.

Each team must retain at least four spawns inside `out_of_bounds`. The other
bounds control camera limits and the match's zoom-out presentation. Resizing
`out_of_bounds` must keep every spawn inside it. Objects outside those bounds
can be intentional boundary geometry and are allowed.

Neither the editor nor the game checks that the arena is sealed, and a player
who leaves `out_of_bounds` currently closes the game
([Seal the arena](level-format.md#coordinates-bounds-and-spawns), `REV#1`).
Play every reachable edge before sharing a stage.

## Acceptance checks

`ED#5` was accepted by the author on 2026-11-05: the editor is accepted as
built. The attended GUI and saved-stage gameplay session was not held, so there
is no recorded manual result. The automated checks below establish document
behaviour and hidden-window operation, not that manual acceptance; the manual
checklist below stays as what such a pass would cover.

```powershell
./scripts/check-level-editor.ps1 -Preset x64-debug
./scripts/check-level-editor.ps1 -Preset x64-release
```

These build and run the native document model, a hidden in-process DPI/canvas
probe and hidden GUI launch/open/respond/close and save-guidance checks. None
creates a game graphics/audio device.
They cover the shipped levels, the canonical template, unknown-field preservation, both rectangle
encodings, ramps, the palette, undo/redo and save
state, Unicode paths and failure recovery. Evidence and generated test levels
remain in `out/acceptance/<preset>/level-editor/`. They also check the optional
Random setting's default, insertion, validation, undo/redo and save/reopen,
and each palette kind's default material, refusal of unknown and mismatched
materials (in edits and loaded files), and inserting and clearing a material.
They check the resource-name warnings against source content, and a third
hidden launch reads the status line of a level the game would refuse.
The Save to My Stages model checks find the folder
under a scratch `LOCALAPPDATA` without creating it, and refuse a case-only
shipped-name collision, a space, a leading hyphen, a 65-character name, a
non-ASCII name, a non-`.json` file, and files outside the folder or in a
sub-folder. They accept the 64-character limit and the offered names. They
check that the first save creates the folder and file and reloads unchanged,
that a second save under another spelling is reported as a replacement of
the same file and leaves one file, and that no save file appears.
The script points `LOCALAPPDATA` at its run folder for every launch. It also
checks that opening the editor creates neither a save nor the stage folder.
The hidden save-guidance helper also drives its own file dialogs, checking
Save to My Stages, ordinary Save/refresh, cancellation, failed-save recovery
and Save As elsewhere. It waits for a responsive owner and the operation's
observable results, including folder cleanup and saved file/status changes,
before issuing the next action. Attended dialog usability and the replace
question remain part of the manual pass.
The document checks also cover the shared current version, legacy save upgrades,
unknown-field preservation, strict format type/version rejection before other
field decoding, filename diagnostics, failed-upgrade recovery and retained
undo/redo branches across a successful upgrade.
Use `-SkipBuild` only after
building `colourwars_editor_acceptance`, `colourwars_editor_dpi_acceptance`
and `ColourWarsEditor` for the
current source.

On 2026-10-15, both presets passed the document checks (including the optional
Random setting) and hidden GUI launch/open/respond/close checks. Evidence:
`out/acceptance/x64-debug/level-editor/088ef35b371047ee97c715f163c61008/`
and `out/acceptance/x64-release/level-editor/2ca17df03ee24d7683cb8131b44fb245/`.
The manual GUI and saved-stage play checks below were not run; `ED#5` was
accepted without them on 2026-11-05.

On 2026-10-28, both presets also pass the expanded DPI/canvas and native
save-guidance checks: Debug `8c23b3aacabc4e26a2b4e25bb1986fcd`, Release
`9f85cf04ebba419c94034365360a57bf`, under the same evidence root. These are
software checks; the attended GUI and saved-stage play pass was not held.

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

A mover is made a [paint ferry](feature-roadmap.md#paint-ferry-road15-first-slice)
(`ROAD#15`) in the same inspector. `motion/drive (cycle or paint)` is always
listed, as `cycle` when the record omits it; `motion/team_a_heads (right or
left)` is listed when present, and a paint drive's phase is labelled
`0 left end to 0.5 right end`. Setting the drive to `paint` adds team A heading
`right` unless the mover already has a heading. Setting `cycle`, or clearing the
drive, removes the heading, and a cleared drive is omitted rather than written
as `cycle`. Typing `cycle` over an omitted drive changes nothing. Any other
value is written as typed and then refused.

The drive is checked by the game's own rule, not a copy:
`horizontal_motion_contract::decode_drive` reads the two text fields and
`validate_drive` the paint-drive phase, and the game's `LevelObjectBuilder`
calls the same two. An unknown drive, a paint drive without a heading or with
one other than right/left, a heading on a cycle (explicit or by default), a
paint drive whose phase is past 0.5, and a non-text drive or heading are each
refused. As an edit, the refusal leaves the document and history unchanged; as
a file, the load is refused, and every drive rule above is in the same words
as the game's load error (only the non-text type error is each side's own).
So a bad drive never reaches a save.

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
The `level_editor` document checks open, save and reopen the shipped Ferry Trial
(`turbulence_ferry_trial`) unchanged, flip and switch its drives with undo, turn
a palette mover into a ferry, and refuse each bad drive shape both as an edit
and as a file.

The application supplies source `game/content/tuning.json` for player size;
the registration helper supplies the same repository context. Tools using the
document API pass that path to `LevelDocument(tuning_path)`. The default
constructor still works for static documents, but cannot load or add a mover
without this explicit context. Mover validation rereads current `player.size`
and shares the runtime's float field, travel and standing-envelope arithmetic
through the engine-free `game/contracts/horizontal_motion_contract.h`. Changes to source
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

### Vertical lifts (`ROAD#28`)

**Added 2026-11-03.** The palette's **Vertical lift** entry writes a solid
`StructurePaintable` with a [vertical motion record](level-format.md#vertical-lift-motion):
`travel_y` 320, period 4 and phase 0, the slice's provisional one storey at
160 units/s. The rectangle is the lift's **topmost** anchor; the inspector
labels `motion/type (horizontal or vertical)` and
`motion/travel_y (world units down from the top anchor)`. The editor decodes a
lift by the same rule the loader uses, so it refuses `travel_x` on a lift, a
heading on a cycle, and jump-through collision, each before the edit lands.

The guides add a second gold dotted box around a lift: the column it keeps
clear, a body width (and rounding pad) beside its whole shaft, from a body
above its highest pose to a body below its lowest. An edit, a duplicate or a
bounds change that puts geometry in it, or takes it out of the Scene, is
refused, as the game's admission refuses it. Duplicate places the copy's column
below the source's rather than below its path. Mirror copy reflects a lift's anchor and keeps its phase. Blockout
aids translate a lift's scoring faces on y by its initial phase, keeping their
local size. The `level_editor` document checks cover all of this
(`check_lift_motion`).

**Paint lifts (2026-11-03).** Setting a lift's `motion/drive` to `paint` adds
`"team_a_heads": "up"`; the inspector labels the heading
`motion/team_a_heads (up or down)` and the phase
`motion/phase (paint: 0 top to 0.5 bottom)`. `right`, `left` and a return-leg
phase are refused in the game's words, at load and on every edit, by the
shared `decode_drive`/`validate_drive`. The canvas draws a **PAINT LIFT**
panel above the body: the ferry panel's A and B rows with vertical arrows,
down for a `down` heading. Mirror copy refuses a paint lift as it refuses a
paint ferry. `drive_checks` and `mechanism_guide_checks` cover the fields,
refusals, save/reopen and guide shapes.

## Team gate authoring (`ROAD#15`)

The palette's **Team gate** entry adds a
[team gate](feature-roadmap.md#team-gate-road15-second-slice): a solid
`StructurePaintable` with the `eraser` material, `"gate": "paint"`, and every
face paintable except the bottom, which stands on the floor. The inspector
lists `gate (paint: open to the team holding more of it)`. Clearing it removes
the field and leaves an ordinary paintable block; any other value is written as
typed and then refused. A gate cannot be added to an existing block in the
inspector, which only edits fields a record already has; use the palette entry.

The gate is checked by the game's own rule, `game/contracts/team_gate_contract.h`: an
unknown or non-text value, a gate on anything but a `StructurePaintable`, a
one-way (`STRUCTURE_JUMP_THROUGH`) gate, a moving gate and a gate with no
paintable face are refused in the game's words, as an edit (document and
history unchanged) or as a file. So is a gate overlapping a spawn body, which
the editor sizes from `tuning.json` as it does for movers, with the game's
startup words. The canvas draws a gate like any eraser block; it does not show
which team it is open to, which is runtime state. The `level_editor` document
checks cover the field, palette entry, every refusal and save/reopen of the
Gate Trial.

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
the three additions before choosing to apply them:

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
asset lists already agree and that `presentation.json`'s `menu_text.stage_names`
names exactly the manifest's levels, then shows the exact added name, runtime
path and stage-name entry, whose English is the level's `display_name`.
The basename follows the user-stage rule (`user_stage_contract::usable_name`):
1 to 64 ASCII letters, digits, `_` or `-`, with no leading hyphen and no
Windows device name. It also may not start with `_`, which `stage_names` would
read as a comment. Every name in the manifest is held to the same rule. The
file must have a `.json` extension and be directly in this source tree's levels
directory; linked/reparse paths are refused. Its `display_name` must be one
nonblank line, since a stage title is. Duplicate or case-alias names, a stage in
only some of the three files, an existing entry that differs from the
`display_name`, and unsupported/nonliteral CMake list syntax are diagnosed
rather than guessed at. An entry already present in all three is a validated
no-op.

Apply revalidates the proposal and unchanged stage/source bytes. It prepares and
flushes the three candidates and original backups before replacing any file,
then replaces the manifest, the runtime list and `presentation.json` in that
order, rechecking every file's bytes before each. Only the three entries are
inserted; surrounding bytes, line endings and unrelated metadata remain. If a
later replacement fails, the files already replaced are restored, newest first,
while each is still this operation's candidate. A failed rollback retains every
original not used by a restore at the recovery paths printed in the error;
inspect all three files before retrying. Each replacement is atomic; the set is
not crash-atomic. The saved stage itself is never rewritten.

Configure/build the game after registration to copy the stage beside the
executable. The game's startup/content validation remains authoritative;
registration does not establish route, spawn or gameplay acceptance (`ED#5`).
GUI integration can follow actual use of the command-line helper.

The explicit-only [Turbulence slab trial](../game/content/levels/turbulence_slab_trial.json)
was saved/reopened by `LevelDocument` and registered through this API on
2026-10-26. It preserves the qualified 24-object geometry and all eight earlier
level files. The [arena record](polish-turbulence.md) owns its public-loader,
route and composition evidence; this automated use does not close `ED#5`.

The document acceptance case also checks exact LF/CRLF and BOM-preserving
appends, malformed/partial/aliased lists, resource validation, stale inputs and
proposals, first/second/third replacement failures, locked/read-only files and
named original backups after a failed or partial rollback, a stage named in any
one or two of the three files, and a stage-name entry that disagrees with its
level. The separate integration check uses a fresh scratch repository and only
the reviewed runtime assets, and since 2026-11-05 checks that apply changed only
the manifest, the runtime list and `presentation.json`, each by its one entry:

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
play review was `ED#5`, accepted without that session on 2026-11-05.

Since 2026-10-29 CI runs `-SkipBuild` in both jobs, after building
`ColourWarsRegisterStage`, and uploads the receipts and logs. Timing and focus
evidence are in
[code-hygiene.md](code-hygiene.md#registration-and-failure-reporting-checks-in-ci-ci3).

## Movement Course icon (`ED#6`, retired)

Retired 2026-10-18 during the backlog audit: `icon_frame` is required legacy
metadata, and no current screen displays it. Stage Select draws the actual
arena preview. Keep the existing valid frame rather than commission invisible
artwork. A distinct icon becomes work only if an accepted screen needs one.
