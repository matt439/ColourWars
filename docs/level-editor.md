# Level editor

`ColourWarsEditor.exe` is a separate Windows desktop application for editing
the game's level JSON files. It opens existing levels without changing their
format and preserves fields that the editor does not know about. The canvas
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

## Editing

Select an object in the list or click it on the canvas. Drag its body to move
it, or drag the handle at the bottom-right corner to resize it. Ramp editing
moves or scales all three vertices together. Spawn markers can be moved;
their boxes use `player.size` from source `game/content/tuning.json`.

Use the palette to add paintable platforms, solid structures, jump-through
platforms, left/right ramps and decorative visuals. Team spawns and the four
bounds rectangles are also selectable and editable. The level entry exposes
the stage name, menu icon and sound settings.

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

## Play the level

For an existing stage, save its source JSON, rebuild the game and choose the
stage from the game menu. The build copies the revised file beside the game.

A new file is not automatically a menu stage. To register it:

1. Save it as `game/content/levels/<name>.json`.
2. Add `<name>` without the extension to the `level` asset group's `names`
   in `game/content/manifest.json`.
3. Add `levels/<name>.json` to `COLOURWARS_RELEASE_ASSETS` in
   `cmake/release_assets.cmake`. Configure checks that these two lists agree.
4. Configure/build again, then start the game and choose the new stage.

The file's `display_name` and `icon_frame` control its menu presentation.
Stage names use the glyphs supported by the game's compiled fonts; unsupported
characters such as emoji are rejected before saving.
The icon must name an existing frame in `sprite_sheet_1`. Object sprite-sheet
and frame names, CSS colour names and music names must also exist in the
game's assets. The editor checks document structure and geometry; the game's
startup validation resolves the actual resources.

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
They cover the shipped levels, unknown-field preservation, both rectangle
encodings, ramps, the palette, undo/redo and save
state, Unicode paths and failure recovery. Evidence and generated test levels
remain in `out/acceptance/<preset>/level-editor/`. They also check the optional
Random setting's default, insertion, validation, undo/redo and save/reopen.
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
grid snapping and the unsaved-changes prompt on New, Open and Close. Rebuild
and play a saved stage to check collisions, paintable faces, spawns, camera
bounds and ramps. Document acceptance does not establish those play qualities.
