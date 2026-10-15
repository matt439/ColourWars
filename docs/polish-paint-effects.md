# Paint.exe projectiles and fresh paint

Implemented 2026-10-15, completing the `POL#11` production pass. It joins the
[painter](polish-painter.md) and
[tools](polish-tools.md) in the accepted
[Paint.exe drawing language](polish-plan.md#1-establish-the-art-and-display-rules).
The reference arena and human composition review remain `POL#17`, `POL#7`
and `POL#24`.

## Drawing and contact

The editable [effects source and recipe](../game/art/paint-fx/README.md) supply
team-tinted grayscale shapes with black keylines. Spray has a compact liquid
bead, the precision nozzle a narrow pressure streak, the mister a fine cluster,
and the bucket a large irregular blob. The precision streak follows its launch
direction around its centre; the collider remains axis aligned. The roller
keeps its invisible, one-step contact volume and shows its swash at the surface.
Each tool has its own short impact strip with separating droplets and a clear
end, rather than leaving an animated decal behind.
Surface splashes rotate outward from the contacted top, bottom, left or right
face. Airborne splashes draw before all owned surface fills on that structure,
so a neighbouring tile's droplets cannot overwrite its persistent team tint.

The existing exposed-face tiles still decide every capture. Enemy repaint
replaces the tint immediately and transfers credit; friendly repaint refreshes
the wet effect while retaining the original capturing player's credit.
An inward black edge follows the face without bordering individual four-unit
tiles. A lighter wet band moves inward briefly and settles back to the team
colour. These persistent marks stay inside each exact scored rectangle,
including fractional tiles at occluded edges.

`tuning.json.paint.visual` owns edge width, wet duration, highlight amount,
settling depth and impact size. This keeps the existing paint artwork choices
with their owning record. Projectile artwork changes only asset references;
launch positions, collision sizes, diffusion, gravity, lifetime, ammunition,
damage and scoring retain their existing simulation paths.

## Lifetime and rendering

Each tile retains at most one impact animation. A new contact restarts that
slot; storage does not grow with repeated shots or player views. Construction
does not emit an impact. Positive simulation updates age the wet band and
impact; pause and nonpositive deltas freeze them, a long update expires them,
and level teardown destroys their owning tiles. No gameplay random draw is
used. Drawing is const and does not advance effects.

The owning structure includes airborne impact overhang in `cull_bounds`,
while tile contact bounds remain exact. The jet includes its rotated visual
extent in `cull_bounds` too. Geometry bounds used by layout and collision stay
unchanged. Only the reviewed DDS and frame JSON enter
staging and Release; the editable source, working PNG, preview and recipe stay
outside runtime content.

## Reproduction and validation

Rebuild the reviewed atlas with
`powershell -ExecutionPolicy Bypass -File scripts/build-paint-fx-art.ps1`.
Run the CPU consumer cases with `scripts/run-acceptance.ps1` and the hidden
D3D11 matrix with `scripts/check-paint-fx.ps1`, each from an x64 VS Developer
shell and with `-Preset x64-debug` or `x64-release`.

Validation on 2026-10-15:

- Debug and Release build and pass all 12 consumer cases. Added paint checks
  cover all five impact selections, initial inactivity, nonpositive updates,
  long-step retirement and unchanged tile geometry/area, friendly credit and
  immediate enemy capture. Movement Course flight/cover trials, weapon
  ammunition/cadence and Debug replay determinism also pass.
- Both builds pass the D3D11 hardware matrix on the NVIDIA GeForce RTX 5080.
  It renders real four-player matches in all three palettes at 720p, 1080p and
  QHD, plus controlled four-viewport captures at the real gameplay zoom.
  Four flying types are checked in both team colours at eight launch angles;
  all five tool impacts contact each of the four surface orientations.
  Friendly repaint restarts the same raster without taking credit; enemy
  repaint immediately matches a fresh contact of the new team. Repeated const
  draws are byte-identical and preserve geometry, scores and attribution.
  Projected raster extents fit their culling bounds. These are bounds and
  before/after checks, not an independent per-pixel scoring oracle.
- Two art regenerations produce identical source PNG, JSON, DDS and lineup.
  Pixel checks cover grayscale liquid materials, transparent frame borders
  and impact terminal frames, all 40 frames and the DDS's premultiplied RGBA8
  bytes/one mip. Gameplay tuning matches the previous revision apart from
  projectile asset names and new cosmetic paint settings.
- All six runtime-bank/staging tests pass. The 53-file Release ZIP contains
  all 26 reviewed runtime assets byte for byte and excludes authoring files.
  Isolated Debug and extracted Release startup pass with the accepted bank,
  a system-only PATH and graceful shutdown. A separate extracted Release copy
  also starts without the bank and reports the expected single silent-bank
  diagnostic.

Evidence is under `out/validation/paint-exe-effects/` and `out/acceptance/`.
Successful visual run IDs are Debug `082b604309d24a8a8c8002a8f1545311` and
Release `a941abb6bd6d4e8f8a10f0c384bbb6e8`.
Retained captures show [native shot crops](images/polish-paint-effects/shots-720p.png),
[surface impacts](images/polish-paint-effects/impacts-720p.png) and a
[four-player match](images/polish-paint-effects/four-player-720p.png).

These checks and image inspection do not establish human composition,
movement-feel, controller, listening or frame-pacing acceptance. The reference
arena and its review remain in their existing TODO rows.
[Idle nozzle drips](polish-plan.md#tool-personality-and-quiet-detail) and
[player hit/elimination feedback](polish-combat-feedback.md) are implemented.

## Wall runoff

Implemented 2026-10-15 as a gameplay-style increment of `POL#16`: small cosmetic
runs on fresh wall paint. A projectile contact starts at most one run on each
exposed vertical strip. After a short delay, a stepped, black-outlined bead and thin wet trail
accelerate down the existing team paint. Further friendly contacts let the
current phrase finish instead of holding it at its first frame.

The run stops at the first neutral or opposing tile, the exposed strip's end,
its travel limit or its lifetime. Repainting its source for the other team
cancels it. The bead and trail are clipped inside the current same-team colour
band, preserving the inward black keyline, fractional tile edges and gaps where
another structure covers the wall. They never add painted area or transfer
credit. Horizontal faces retain their existing wet settling treatment.

One slot is stored per vertical strip, independent of shot count and player
views. Positive simulation updates advance it; pause and nonpositive updates
freeze it. Reduce effects hides the runoff while its lifetime continues, so
restoring the preference cannot replay an expired mark. Drawing stays const
and consumes no random numbers. Collision, coverage, attribution, audio and
culling extents keep their existing paths.

The editable measurements, delay, lifetime and shading live in
`tuning.json.paint.visual.runoff`. This uses the existing fill texel and needs
no new atlas, authoring tool or runtime asset.

The defaults wait 0.12 seconds, then accelerate over a 0.65-second phrase with
at most 28 world units of travel. The bead is 3 by 5 units, with a 9-unit trail,
a 0.75-unit keyline and restrained team-coloured shade/highlight. The available
owned corridor can end the phrase sooner. Once a boundary shortens a run,
reclaiming paint does not extend that old phrase through it.

### Runoff validation

Debug and Release build under `/W4 /WX` and each pass all 21 consumer cases.
The expanded `paint` case checks delay, accelerated travel, expiry, nonpositive
ticks, sustained contacts, source/enemy repaint, sticky ownership boundaries,
different friendly capturing players, fractional occlusion and tiny exposed
fragments. Coverage and player credit remain exact. Content variants reject
missing/wrong-type fields, out-of-range values and incompatible
width, inset, trail or keyline measurements with an owning-field diagnostic.

Both D3D11 hardware probes pass on the NVIDIA GeForce RTX 5080. The existing
projectile/impact matrix is retained; added one/four-view runoff fixtures use
both team colours in all three palettes at native 720p, 1080p and QHD scales.
They compare early/late motion with same-state Reduce effects suppression,
check every changed pixel against independently authored owned-wall rectangles
with the renderer's edge quantization, and preserve the inward keyline, enemy
paint and fractional occlusion. Frozen redraws preserve pixels and observed
effect/ownership state. Hidden runs continue to expire. Native-size inspection
confirms the restrained moving bead/trail in the retained
[blue/orange](images/polish-paint-effects/runoff-blue-orange-720p.png) and
[pink/green](images/polish-paint-effects/runoff-pink-green-720p.png) contact sheets.
These controlled fixtures do not establish reference-arena composition or human
readability acceptance; those remain in `POL#7` and `POL#24`.

The 53-file Release ZIP byte-matches all 26 reviewed runtime assets and the
accepted sound bank, and excludes authoring/probe files. Isolated Debug and
extracted Release startup pass with the bank, a system-only PATH and graceful
close. A separate extracted copy passes the absent-bank fallback with its
single expected diagnostic. The player's real save is never opened.

Evidence is under `out/validation/runoff/`, `out/acceptance/<preset>/` and
`out/validation/runoff-{build,acceptance,visual}-x64-{debug,release}.log`.
Final renderer run IDs are Debug `59bf1b10d53e402fb11cf3296d0efd0a` and
Release `13666037379046308b1bef2bdd8ffc4d`; receipts record baseline `3262636`
plus the working changes and the unchanged engine pin. `POL#16` remains open
for the remaining arena/interface treatment in the polish plan.
