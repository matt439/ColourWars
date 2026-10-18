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

Use the palette to add paintable platforms, solid structures, jump-through
platforms, left/right ramps and decorative visuals. Team spawns and the four
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

The inspector's `material` row lists, in its label, the values the object's
collision type allows, such as `material (ground, pencil, eraser, paint_tin)`
on a paintable platform. It is a text field checked on Apply: an unknown name,
or one the collision type does not allow, is refused like any other invalid
edit. An object without a material still shows the row, empty; applying a value
adds the field, as the Random setting does, and undo restores the omission.
Applying an empty value removes the field, returning the object to the legacy
stretched sheet/frame draw. The inspector adds no other missing field, and
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
  decorations do not. The editor and game share the interval calculation in
  `game/paint_face_geometry.h`. With aids off, cyan shows authored faces instead.
- **Scoring area** shows the whole level and the part inside the selected
  rectangle. Select an object or one of the bounds rectangles to choose a
  region; selecting the level shows only the total. Area clips the inward paint
  strips using current `paint.thickness`, in world units squared. Corners and
  overlapping strips contribute separately, as in runtime scoring. It measures
  available scoring surface, without assigning teams or predicting reachability.
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
| each object's `sheet_name` | a `sprite_sheet` listed in `game/content/manifest.json` |
| each object's `frame_name` | an entry in that sheet's `sprite_frames`; an animation strip is not a frame |
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
