# Executable icon

`icon.json` is the editable drawing of `ColourWars.exe`'s icon, which
[game/resource.rc](../../resource.rc) embeds from `game/colourwars.ico`. It
replaced the stock DirectX template icon (`GAP#7.6`).

It is the painter's head from the [production figure](../painter/README.md),
backwards blue cap and all, in front of an orange paint splat: the two colours
are the default `blue_orange` team pair in `presentation.json`. It follows the
[accepted art rules](../../../docs/polish-plan.md#1-establish-the-art-and-display-rules):
black keylines, flat fills, no dithering, and five colours (black, white, the
two team colours and transparency), so each size is a few strokes in Windows 98 Paint.

Each grid character is one pixel. Three sizes are drawn by hand, because a
16-pixel head is not a shrunken 32-pixel one; the larger sizes are
whole-number nearest-neighbour enlargements, so nothing is ever filtered:

| Size | Drawn from |
|---:|---|
| 16 | the 16 grid, one pixel per painter cell |
| 24 | the 24 grid |
| 32 | the 32 grid, two pixels per painter cell |
| 48 | the 24 grid at 2x |
| 64 | the 32 grid at 2x |
| 256 | the 32 grid at 8x, the Zoom 800% look |

Sizes up to 64 are stored as 32-bit BMP with an AND mask; 256 is PNG. After
editing a grid, run from the repository root:

```
python3 scripts/build_icon.py
```

It needs only the Python standard library, writes `game/colourwars.ico` and
[preview.png](preview.png) (every size at 1:1 on the Muted Day paper and on a
dark taskbar grey), and always produces the same bytes. CI runs
`scripts/ci/test_build_icon.py`, which fails if either committed file is not
what the drawing builds.

On a dark taskbar the black keylines, including the cap's brim, sink into the
background; the white face, blue cap and orange splat still carry it.
