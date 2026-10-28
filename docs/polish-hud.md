# Paint.exe live HUD artwork

Implemented 2026-10-15 as a gameplay-presentation increment of `POL#16`.
Each pane's player label, health/paint panel and ordinary clock now use pale,
stepped paper with a dark keyline, offset shade and restrained lower/right
airbrush shading. Dark text retains the existing fonts and positions. The
numbered badge, team label, device or bot name and team-coloured edge keep
their existing roster mapping.

Both meters have an outside ink border and neutral empty track. Their interior
keeps its original size: the actual clamped health or ammunition fraction
determines the fill width, including completely empty and full values. Health
retains its existing green and paint retains the exact team colour. The clock
keeps its ceiling-based `m:ss` value; the final ten positive live seconds retain
the [closing card and brief pen ticks](polish-match-accents.md).

This drawing is static. Reduce effects retains all information, and rendering
introduces no clock, animation state or randomness. Match timing, controls,
health, ammunition, scoring and the withholding of live coverage stay unchanged.
The same HUD follows pause, death/redraw and resizing through its existing call
sites; it adds no story elements to solo practice.

`game/objects/interface_gameplay.cpp` draws the paper with the existing solid
texel. `presentation.json.hud.paper` owns opaque, distinct ink/paper colours,
visible shade, keyline width and shadow offset in HUD reference pixels. Startup
validates required fields, types, colour syntax and bounded measurements. The
existing `hud.player_identity.background_colour` remains the stage-preview
backing colour; gameplay uses the new paper settings. No new atlas or authoring
asset is required.

## Validation

From an x64 VS Developer shell:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -SkipBuild -HudOnly
scripts/check-painter.ps1 -Preset x64-release -SkipBuild -HudOnly
```

Validation on 2026-10-15:

- Debug and Release build under `/W4 /WX` and each pass all 21 consumer cases.
  New content checks cover required fields and types, numeric limits and
  overflow, malformed colours, opacity, distinct ink/paper and accepted bounds.
- Both focused hardware D3D11 matrices pass on the NVIDIA GeForce RTX 5080.
  Native full/half/empty and clamped health/paint rasters preserve the exact
  meter tracks; independently full health/empty paint and the reverse leave
  the other track and all static artwork unchanged. Exterior keylines and
  dark-text/pale-paper raster contrast remain visible.
- One/two/three/four-player 720p layouts, every one/four-player palette,
  representative four-player 1080p/QHD, 4:3, portrait and ultrawide cases fit
  within their owning panes. Static Reduce effects, repeated frozen draws,
  draw RNG and the existing final-ten clock boundary matrix pass. A real
  four-player match preserves observed state, Debug replay fingerprint and
  exact pixels across a 720p/1080p resize round-trip.
- Isolated Debug, staged Release and extracted Release startup pass with the
  accepted bank and a system-only PATH. A separate extracted missing-bank
  copy starts with its single expected silent-fallback diagnostic. Each run
  closes gracefully. The 53-file Release ZIP contains all 26 reviewed runtime
  assets byte for byte, the unchanged bank and provenance, and no authoring
  assets or developer-console RTTI.

### One-pixel keyline floor (2026-10-28)

`-HudOnly` had started failing in both presets with "HUD meter lost its
exterior dark keyline" on the four-player 720x1280 portrait case. The probe was
right. Each pane there is 360x640, so the HUD scale is
`min(360 / 1920, 640 / 1080) = 0.1875` and the 2-reference-pixel paper keyline
came out at 0.375 px. Whether a rectangle that thin covers any pixel centre
depends on its sub-pixel phase. In the current raster the meter's ink frame
landed on a pixel centre only along its bottom edge, and that row falls inside
the track the probe models. The meter therefore lost its keyline on all four
sides. The 2026-10-15 run above passed only because the phase was luckier then.
The arithmetic has been sub-pixel since this artwork landed (`dd6d0fa8`).

`InterfaceGameplay::paper_pen` now returns `hud.paper.keyline_width * scale`
with a floor of one device pixel. The meters, paper panels and identity stripe
inset all use it. The floor is a raster rule, like the cursor marker's
`std::max(1.0f, ...)` in `level_draw.cpp`, and not a new art value. It changes
only panes whose scaled pen was below one pixel: two/three/four-player 720p,
`2p-1024x768` and four-player portrait. The closing clock keeps its own
`closing_clock.keyline_width`, which this change does not touch. The probe is
unchanged. With the fix, `-HudOnly`, `-IdentityOnly`, `-SupplyOnly`,
`-MonitorOnly` and `-ClosingOnly` pass. Three switches fail with the same
message on the unmodified base, so they are older, separate problems:

- `-CountdownOnly`: "Stage entry retained stale fitting after an aspect-ratio
  round-trip".
- `-HudClosingMaterialsOnly`: "Closing timer font source line spacing changed".
- `-HudPaperTuneColoursOnly`: "HUD paper ordinal glyph source metadata changed".

`-HudTrackTuneColourOnly` passes its probe and then fails in the script, which
calls `SHA256.HashData`. That method is missing in Windows PowerShell 5.1.

The [native meter detail](images/polish-hud/meters-720p.png) and
[four-player match](images/polish-hud/four-player-720p.png) were inspected at
720p without enlargement. These establish this drawing and placement;
reference-arena composition, human readability, movement, physical-pad and
listening acceptance remain in their existing TODO rows. Menus, cards, stage
preview, pause, results and accepted arenas remain open under `POL#16`.

Evidence is under `out/validation/hud/` and `out/acceptance/`, with build and
consumer logs at `out/validation/hud-{build,acceptance}-x64-{debug,release}.log`.
Successful renderer runs are Debug `f1009b98361646baa92950ae06c2eb3f` and
Release `6a09be1247d948f4889652a83e2b2916`. Package and startup evidence is in
`out/validation/hud/package-startup-501d44cd77e94f2fb036662a3fab339b/`.
