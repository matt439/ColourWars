# Editable workshop pixel study

`source.json` is the authoring source: one character in a row is one native
texel, with an explicit small material palette. Edit the rows and palette, then
run `scripts/build-workshop-kit.ps1` from the repository root. The packer emits
the PNG, premultiplied RGBA8 DDS, Labrador frame JSON and attachment/layout JSON.
No image service, resampling or private art file is needed to rebuild this kit.

These are newly authored pixel shapes informed by the retained workshop concept,
not crops or a reduction of that image. The source and generated atlas stay here,
outside the game manifest and release content. This is an inspectable art study,
not the accepted replacement character, animation set or arena.

The candidate density is **four world units per texel**. The 13 by 30 painter
therefore matches the current 52 by 120 collision rectangle. One-texel outlines,
small material clusters, a cap, overalls and work boots establish the silhouette.
The cap and overalls share the team layer; hardware, face and gloves stay neutral.
Sprayer paint and roller sleeve use the same separate grayscale team layer.
The base and team pixels are disjoint, so no hue extraction is involved.

All coordinates in `source.json` are measured from the sprite's top-left texel
edge. Pivots and semantic anchors may be fractional even though the artwork is
grid-exact. The sprayer is 22 by 12 texels, with grip at (2, 6) and muzzle at
(21.375, 6). This preserves the current 77.5-world-unit muzzle distance. The
roller is 28 by 22 texels with grip at (2, 7) and drum centre at (20.75, 7),
matching its existing 75-unit contact-centre distance. Its sleeve is authored
diagonally to be horizontal at the game's fixed 45-degree right-facing pose.
These anchors preserve geometric relationships; they do not prove that every
rotated pixel or the whole sleeve matches the gameplay contact volume.

Draw tool artwork at its own native size times four. Do not stretch it into the
old weapon sizes: those sizes also determine the gameplay muzzle and are not
integer multiples of this grid. Left-facing UV flips need the mirrored sprite
pivot to keep the grip attached. The study measures those relationships using
the actual weapon calculations without changing them.

The platform is a repeatable 128 by 48-world-unit timber/concrete/steel module.
Wall, shelving and tank use a darker, quieter palette. Platform upper edges
remain straight; decorative depth does not author collisions or scoring faces.
The puff is a single static shape study, not a replacement for the movement
feedback prototype. `pixel` is the probe's solid-colour utility frame.

Run `scripts/check-workshop-kit.ps1` to check deterministic rebuilding, source
validation, disjoint team masks, gutters and the emitted texture contract.
From an x64 VS Developer shell, `scripts/check-workshop-style.ps1` renders the
kit through the real camera and renderer. See the evidence and remaining
decisions in [the style study](../../../../docs/polish-style-kit.md).
