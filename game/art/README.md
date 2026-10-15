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
review evidence. The existing weapon atlases below remain in use while their
Paint.exe replacements are authored and reviewed.

## Painter artwork

The [E painter source](painter/README.md) begins the accepted gameplay character
pass. Its editable coarse-cell drawing, deterministic System.Drawing/texconv
recipe and all-palette pose preview live outside runtime content. Rebuild it with
`powershell -ExecutionPolicy Bypass -File scripts/build-painter-art.ps1`.
Only `game/content/textures/painter.{json,dds}` is staged and packaged.
The [implementation and review record](../../docs/polish-painter.md) states
what this increment covers and which character work remains open.

## Weapon artwork

`weapon_paint_source.png` is the original transparent sprite sheet generated
with the built-in imagegen tool. `weapon_paint_prompt.txt` records the prompt.
Keep this authoring source outside runtime content.

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-weapon-art.ps1
```

This crops and packs the five weapons, copies the legacy
utility frames from `sources/textures/sprite_sheet_1.png`, and regenerates
`sources/textures/weapon_paint.png` here and
`game/content/textures/weapon_paint.{json,dds}`. This uses Windows System.Drawing
and the retained `tools/texconv.exe`.
No image service or Python dependency is needed to reproduce the runtime atlas.

Gameplay frames have four texels per world unit. Their aim pivots are at the
left centre in source texels; the crop canvases align the barrels to that line
and the roller drum with its existing paint contact. Gameplay dimensions,
offsets, ammunition and projectile behaviour remain in `tuning.json`.

The packer separates the authored coloured paint from the neutral hardware.
Every weapon has a base frame and a grayscale `<weapon>_paint` overlay with
identical dimensions and origin. The overlay is tinted with the player's actual
team colour at draw time. The roller only colours its sleeve while firing;
its arm and handle stay neutral. Fixed-palette menu previews use the same team
colours, while Random stays neutral until the match chooses its palette.

Mask extraction is part of the deterministic packing process, using the source
accent hues within the reservoir and canister regions, the bucket's paint, and
the roller sleeve region. Nozzles, triggers, gauges and other hardware retain
their authored colours. It does not require another image generation.
Complementary layer alpha preserves the original silhouette.

The PNG uses straight alpha. The DDS is RGBA8 with premultiplied alpha and one
mip level, as required by Labrador's sprite blend. Do not replace it with a
straight-alpha DDS. The packer omits colour-space metadata from the runtime PNG
so the bundled texconv preserves its channel values while premultiplying for
the engine's UNORM pipeline. Runtime packaging includes only the DDS and frame
JSON.

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
