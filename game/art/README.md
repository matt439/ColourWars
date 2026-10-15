# Weapon artwork

The accepted Paint.exe art rules are in the
[polish plan](../../docs/polish-plan.md#1-establish-the-art-and-display-rules).
The workshop kit, comparison modes, atlas and dedicated probes were retired on
2026-10-15. Its historical concept image and prompt remain in `polish/`; the
[study records](../../docs/polish-playable-kit.md) and Git history retain the
review evidence. The existing weapon atlases below remain in use while their
Paint.exe replacements are authored and reviewed.

`weapon_paint_source.png` is the original transparent sprite sheet generated
with the built-in imagegen tool. `weapon_paint_prompt.txt` records the prompt.
Keep this authoring source outside runtime content.

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-weapon-art.ps1
```

This crops and packs the five weapons, copies the legacy
utility frames, and regenerates `game/content/textures/weapon_paint.{png,json,dds}`.
This uses Windows System.Drawing and the repository's existing texconv tool.
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

Decision accepted 2026-10-15; the moves and development staging changes remain
unimplemented. Move unused and historical assets, including old textures,
unused fonts and levels, into `game/art/`, with the archive separate from active
working sources. Move editable sources here too, preserving authoring inputs
and updating their recipes together. In particular,
[`build-weapon-art.ps1`](../../scripts/build-weapon-art.ps1) still reads
`game/content/textures/sprite_sheet_1.png`; that input must remain available
after its move. Preserve the required authoring tools and update references to
any that move.

Release packaging already uses the reviewed allow-list in
[`release_assets.cmake`](../../cmake/release_assets.cmake). Share that runtime
list with development staging, replacing its broad directory copies. Remove
stale staged extras only within the build's owned content directories. Preserve
the optional sound-bank fallback and its removal of a stale staged bank when
the source bank is absent.

Implementation validation must cover weapon-art reproduction, Debug and Release
builds, clean and existing staging trees containing only the allowed runtime
assets, Release package inspection, and startup with the accepted bank and with
the optional bank absent. Recording this decision does not complete those
checks or close the implementation work.
