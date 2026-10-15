# Paint.exe monitor scenes

Implemented 2026-10-15 as part of `POL#16`. This is the physical computer around
the picture at stage entry and during the existing winner reveal. The
[accepted art rules](polish-plan.md#1-establish-the-art-and-display-rules) govern
the drawing; the [backstory](backstory.md#building-it) still owns its beats.

## Drawing and composition

The monitor uses a coarse black keyline, stepped corners, a pale upper edge
and a shaded inner lip. Restrained airbrush dabs shade the lower/right casing
and the objects stuck to it. Vent slots, a recessed power button and a small
indicator make the beige rectangle read as a physical computer. Sticky notes,
stickers, coffee marks and cracks retain their existing per-match persona.

`presentation.json.opening.monitor_art` owns the new colours and dimensions.
The editable shapes are code drawings in `game/objects/monitor.cpp`, made from
the existing solid texel. No generated atlas or external art tool is required.
The glass, picture, palette, colour swatches and close-button geometry retain
their existing layout.

The monitor now has two drawing passes. `draw` supplies the casing and program
behind the arena. `draw_front_details` restores the physical lip, hardware and
decorations after the arena and its preview tint or closed-program fill. This
keeps notes attached to the bezel when they overlap the glass, including during
the countdown push and ending pullback. The cursor and person draw in front.

There is no new animation, simulation state or random draw. The same persona
appears at both ends of a match. Story timing, countdown, preview colours,
winner selection, coverage, skipping and quick rematches use their existing
paths. Solo practice keeps its plain countdown and overview.

## Validation

The focused renderer check is:

```powershell
scripts/check-painter.ps1 -Preset x64-debug -MonitorOnly
scripts/check-painter.ps1 -Preset x64-release -MonitorOnly
```

Debug and Release build under `/W4 /WX` and each pass all 21 consumer
acceptance cases. The focused renderer matrix passes in both configurations
on the NVIDIA GeForce RTX 5080 using hardware D3D11:

- Forty captures cover all six body shades, adult/mug/coffee/crack/notes and
  child/sticker compositions, native 720p/1080p/QHD and a bounded glass push.
- A deliberately overlapping note remains above the picture and its preview
  tint. Picture and palette interiors retain their values; saving fills the
  intended slot and closing replaces the picture with the desktop.
- Real two- and four-player matches cover opening, both previews, countdown,
  push and pullback. The two-player fixture paints 9216 units for team A and
  reaches the winning reveal; the unpainted four-player fixture closes the
  program. The solo fixture uses the shell's normal intro-off policy.
- Repeated frozen draws preserve pixels, persona, observed simulation state,
  the Debug replay fingerprint and the next random number.

Inspected 720p captures retain the [opening](images/polish-monitor/opening-720p.png),
[deliberate preview overlap](images/polish-monitor/preview-overlap-720p.png) and
[closed program](images/polish-monitor/closed-720p.png). The overlap image is a
controlled composition fixture; the other two are actual match scenes. These
stills establish the drawing and layering, not human motion/readability acceptance.

The 53-file Release ZIP contains the exact revised presentation data and no
probe/source files. Isolated Debug and extracted Release startup pass with
the accepted bank, system-only PATH and graceful close. Extracted Release also
passes separately with the optional bank absent and the expected single
silent-fallback diagnostic. These are startup checks, not listening sessions.

Evidence is under `out/acceptance/<preset>/` and
`out/validation/monitor-scenes/`. The focused visual runs are:

| Preset | Run under `visual/<preset>/` |
|---|---|
| Debug | `dc595cfe6cff428a99dd3114333d5d47` |
| Release | `c272fdac6b7340618f47e2bea2cbd122` |

Their `result.json` files record baseline `0480c18` plus working changes, the
pinned engine, device, scope and staged content hashes. Full-frame monitor
review sheets are separate from the painter's native-size crop sheets.

The [countdown pass](polish-countdown.md) follows this scene. Reference-arena
composition, closing/reveal accents, menus and HUD artwork remain under
`POL#16`. This increment does not close movement-feel,
physical-pad, listening or clean-PC acceptance.

## Person and cursor foreground

Implemented 2026-10-15 as another part of `POL#16`. The person keeps the existing
adult/child/mug silhouette and the same seated, sigh, stand and leave transforms.
A square-ended black contour joins the head, neck and shoulders into one dark
mass. A restrained neutral rim catches the monitor light on the left; layered
shade follows the opposite edge. The child's hair bunches and adult's mug keep
their existing persona role. No face or clothing detail is revealed.

The cursor is an editable 14-by-20 coarse-cell drawing with a black keyline,
pale fill, grey tail shade and a small offset shadow. Its top-left tip stays on
the same swatch or close button throughout the click. Three detached pen ticks
briefly respond to the existing winner/close click; their age is derived from
the reveal timeline, so pausing or drawing again cannot advance them. Reduce
effects suppresses the ticks while preserving the selected colour, saved slot,
closed desktop and coverage reveal.

The source drawings are `game/objects/silhouette.cpp` and `screen_cursor.cpp`.
`presentation.json.opening.silhouette_art` owns the dark colours and pen/shade
widths (fractions of person height). `opening.cursor_art` owns cursor colours,
shadow offset, click reach (fractions of cursor height) and click duration in
seconds. The loader bounds these values and names malformed content at startup.
All new translucent drawing colours use the renderer's premultiplied blend.
No atlas, simulation timer, persona draw or random-number draw was added.

The focused check remains `scripts/check-painter.ps1 -MonitorOnly` in each
preset. It extends the existing monitor fixtures and real two/four-player story
paths; the same solo fixture continues to bypass the person and monitor.

Debug and Release build under `/W4 /WX` and each pass all 21 consumer cases,
including ten malformed foreground-content variants and accepted numeric limits.
Both focused D3D11 renderer runs pass on the NVIDIA GeForce RTX 5080, retaining
75 captures each:

- Adult, child and mug personas in seated, sigh, standing and partial-leave
  poses at 720p/1080p/QHD; full departure leaves no pixels. An offset window
  translates the drawing exactly. Native crops preserve playing-size detail.
- Resting, initial and mid-click cursor rasters keep the same hotspot and arrow
  pixels; changes remain local. Negative/expired ages and reduced effects restore
  the resting cursor.
- Real painted two-player and unpainted four-player reveals bracket the save
  and close clicks. Repeated draws preserve pixels, persona, pose, simulation
  state, draw RNG and the Debug replay fingerprint.

Inspected 720p evidence retains the [opening](images/polish-monitor/foreground-opening-720p.png),
[winning click](images/polish-monitor/foreground-click-720p.png) and
[native cursor crop](images/polish-monitor/foreground-cursor-720p.png).
These establish drawing and event placement; human motion/readability review
remains open.

The 53-file Release ZIP byte-matches the revised presentation data and unchanged
accepted sound bank. Isolated Debug and extracted Release startup pass with the
bank and system-only PATH; extracted Release also passes the missing-bank
silent fallback separately. All runs close gracefully and use isolated saves.

Evidence is under `out/validation/foreground-scenes/`: visual Debug run
`d397d94b4eba4e79bc8233f7f71a74e0`, Release run
`1c181c8900344623b3368044deb44450`, startup records and `release/result.json`.
The visual result files identify baseline `4bb35b3` plus working changes and
the exact staged content. This increment keeps the remaining `POL#16` work open.
