# Weapon artwork

The next art direction is recorded in the
[workshop style candidate](../../docs/polish-style-reference.md). Its concept
image and prompt live in `polish/`, outside runtime content; the existing atlases
below remain in use until their pixel-art replacements are authored and reviewed.
The [editable workshop kit](polish/kit/README.md) now provides a deterministic
native-grid study with a separate renderer/camera probe. It is not runtime content.

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
