# Art sources and runtime content

The accepted Paint.exe art rules are in the
[polish plan](../../docs/polish-plan.md#1-establish-the-art-and-display-rules).
The author's [accepted round-2 study](references/style-board/index.html)
([original artifact](https://claude.ai/artifact/9J5kQgRMvVtXPe6r6cJ7vP))
is the reference for Zoom 400% values, Tool Props terrain, airbrush shadows,
black paint keylines and figure E with teammate head accents.
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

The [E painter source](painter/README.md) begins the accepted gameplay character
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
| `sources/fonts/` | Historical font command example; regeneration recipes for the shipped fonts remain `GAP#8.7` |
| `tools/` | Existing texconv and MakeSpriteFont executables |
| `archive/textures/` | Old texture kit and unused player-frame experiment |
| `archive/fonts/` | Unused compiled fonts |
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
