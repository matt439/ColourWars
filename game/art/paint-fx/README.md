# Paint.exe liquid FX source

[`paint-fx.json`](paint-fx.json) is the editable production recipe for the
five projectile kinds and their surface impacts. It follows the painter and
tool drawing: a black keyline, simple grayscale liquid body, white wet glint,
uneven joined lobes and detached droplets. Runtime draw colour supplies every
team hue. No hue or neutral grey background is baked into the art.

Run `powershell -ExecutionPolicy Bypass -File scripts/build-paint-fx-art.ps1`
from the repository root. Windows System.Drawing draws the integer-cell recipe
without antialiasing. Each of the 32x32 cells expands to exactly 4x4 source
texels, producing a 1024x1536 working atlas at
`../sources/textures/paint_fx.png`, runtime
`../../content/textures/paint_fx.json` and `.dds`, and the review
[`lineup.png`](lineup.png). Ordinary builds need neither System.Drawing nor
Python; they stage only the DDS and JSON.

Each frame lists `lobes`, which are merged before outlining, and `tones`,
which are clipped to that union. Shapes are `["ellipse", x, y, w, h]` and
`["poly", x0, y0, x1, y1, ...]`. Tone shapes put an integer grayscale value
after the operation: `["ellipse", 255, x, y, w, h]`. The body defaults to
232, the wet glint to 255, and its underside to 192 or 208. An inset black
keyline surrounds the merged silhouette rather than drawing seams around each
ellipse. Every frame keeps a transparent edge cell, including its disconnected
droplets, so contiguous strip frames cannot bleed into one another.

| Kind | Looping shot | Surface impact |
|---|---|---|
| Spray | Three uneven compact drops with a changing trailing bead | A small rounded fork opens into detached drops |
| Jet | Three narrow pressure needles, inside the canvas's inscribed circle | A directional strike opens into a narrow two-lobed burst |
| Rolling | Three transparent frames; the instantaneous contact volume stays invisible | A low compressed wash with short upward beads |
| Mist | Three broad small beads with orbiting fine drops | Disconnected fine droplets scatter without a solid central patch |
| Ball | Three heavy wobbling blobs with a broad wet glint | A wide joined crown separates into three large drops |

Strip names are `shot_<kind>` and `impact_<kind>`. Shots have three looping
frames. Impacts have five nonlooping frames: contact, opening lobes, detached
droplets, settling residue, then a fully transparent terminal frame. Durations
remain authored per strip. The fixed runtime splash owns its playback; these
drawings allocate no particles or gameplay objects.

All projectile world sizes, collision sizes, timing, damage, diffusion and
coverage remain the existing tuning. The jet's drawn silhouette stays inside
its inscribed circle so aiming does not move its visible material outside the
original square canvas. The roller drawing does not expose its short-lived
50x50 contact volume.

`lineup.png` shows every frame enlarged at three pixels per authored cell,
then every frame in all five team hues at the actual 720p gameplay scale,
`720 / presentation.camera.reference_height`. It reads shot sizes from
`tuning.json`, including the mist's initial 3-pixel canvas and the bucket's
67-pixel canvas. Impacts use the runtime
`paint.thickness * paint.visual.splash_scale` canvas (currently 16 pixels at
720p). Grey review panels are preview backgrounds only.

The DDS is premultiplied RGBA8 UNORM with one mip. Metadata-free source PNGs
preserve authored UNORM bytes through texconv. Strip order and coordinates are
fixed, and rebuilding the same source produces byte-identical PNG, JSON, DDS
and lineup. This authoring review is separate from runtime composition,
split-screen performance and human readability acceptance.
