# Classic Microsoft Paint programs: Windows 3.x Paintbrush, Paint 95 and Paint 98, for the ColourWars art rules

Scope: what the old Microsoft paint programs looked like and which tools they had, so that ColourWars' "Paint.exe Maximalism" rules (the accepted §1 table in [polish-plan.md](polish-plan.md)) can be limited to marks a real copy of the program could make. Each claim carries a confidence tag: **[H]** high, **[M]** medium, **[L]** low. Where a source was refuted, this document uses the correction. Where nothing could be verified it says "unknown".

---

## 1. Timeline

| Version | What changed |
|---|---|
| **Paint (PAINT.EXE), Windows 1.x/2.x** | Monochrome only; reads and writes only .MSP. 24 tools, including Pencil, Scroll, Selection Net, Spray Paint (sprays a pattern) and a 3‑D isometric axis tool. A Palette menu supplies patterns, line widths and brush shapes; the Options menu has Fine/Medium/Coarse grids and Edit Pattern. **[H]** ([Win 2.x manuals](https://archive.org/details/microsoft-windows-2.x-manuals), [Wikipedia](https://en.wikipedia.org/wiki/Microsoft_Paint), [bc-programming 1.04 shot](https://bc-programming.com/blogs/wp-content/uploads/2017/08/Windows_104.png)) |
| **Paintbrush (PBRUSH.EXE), Windows 3.0 (1990)** | Colour, based on a licensed newer version of ZSoft PC Paintbrush **[M]**. 18 tools, with separate filled and hollow shape tools. Menus: File, Edit, View, Font, Style, Size, Pick, Options, Help. A cutout is swept with Ctrl+drag and copied with Shift+drag. **[H]** ([3.0 User's Guide](https://ftp.mirrorservice.org/sites/www.bitsavers.org/pdf/microsoft/windows_3.0/Windows_3.0_Users_Guide_198909.pdf)) |
| **Paintbrush, Windows 3.1 (1992)** | Essentially the same program. Font, Style and Size merge into one Text menu, and the cutout keys swap: Shift+drag sweeps, Ctrl+drag copies. Ctrl+Z becomes Undo. **[H]** ([3.1 User's Guide](https://ftp.mirrorservice.org/sites/www.bitsavers.org/pdf/microsoft/windows_3.1/Microsoft_Windows_3.1/PC21669-0492_Windows_3.1_Users_Guide_Apr92.pdf), [PBRUSH.HLP](https://discmaster.textfiles.com/file/4696/Tandy%20Sensation%20II%20System.iso/mwin/pbrush.hlp/pbrush.pdf)) |
| **Paint (MSPAINT.EXE 4.00.950), Windows 95 / NT 4.0** | Rewritten from scratch as an MFC 3.0 app, with no further ZSoft licensing **[M]**. 16 tools plus a per‑tool options well. New: Pencil, Pick Color (eyedropper), Magnifier, 3‑level undo with Repeat, 90° rotate, Stretch/Skew, the fixed 28‑colour default palette, and View > Zoom with 100/200/400/600/800% and Show Grid. Menus: File, Edit, View, Image, **Options**, Help. A 5 KB PBRUSH.EXE stub redirects to the new program, and Paintbrush's print scale‑to‑fit is gone. **[H]** ([MSPaintWin95 binary](https://archive.org/details/MSPaintWin95), [Kaiser 1996](https://kaisernet.org/library/1996/0796/07h01001.htm)) |
| **Paint, Windows 98 / 98 SE / ME / 2000** | Same tools and layout. The Options menu becomes **Colors**, and Draw Opaque moves to Image **[M]**. JPEG/GIF only through Office or PhotoDraw graphics filters (KB Q298580) **[H]**. PCX dropped. HTML Help replaces WinHelp **[L]**. Wikipedia says ME stops prompting and enlarges the canvas automatically on a larger paste; this is **unverified [L]**. |
| **Paint, Windows XP** | Same classic UI and tools. GDI+ adds native BMP/JPEG/GIF/TIFF/PNG saving. Get/Save Colors (.pal) removed. Still 3 undos. **[M]** ([Wikipedia](https://en.wikipedia.org/wiki/Microsoft_Paint)) |
| *(after)* | Vista: new icons and palette, 10 undos. Windows 7: ribbon, antialiased shapes, semi‑transparent "artistic" brushes, 50 undos. Windows 11 (Sept 2023): layers and transparent PNG. None of these belong to the classic look. **[H]** |

---

## 2. What each version looked like

### Windows 3.1 Paintbrush
The window has Windows 3.x chrome and a blue title bar reading "Paintbrush - (Untitled)". The menus are File, Edit, View, Text, Pick, Options, Help, and Pick is greyed out until a cutout exists. Down the left edge is a toolbox of 2 columns × 9 rows (18 small coloured icons). The filled shapes have teal fills and black keylines. The selected tool is shown inverted, and the Brush is selected at startup. Under the toolbox, in the lower‑left corner, is the **Linesize box**: sample lines whose top line is 1 pel, with an arrow marking the current width. Eight widths, counted from screenshots **[M]**. The palette runs along the bottom: on the left a "Selected Colors" box shows the foreground inside the background (black on white at startup), and to its right are 28 swatches in 14 columns × 2 rows. The **white and light‑grey row is on top**, the reverse of Paint 95 **[H]**. On a 16‑colour display the last 6 columns show as dither checkerboards **[M]**: the driver and colour depth of the screenshot are not stated. An optional floating Cursor Position window shows x,y. ([toastytech 3.0](http://toastytech.com/guis/win30paint.png), [bc-programming 3.1](https://bc-programming.com/blogs/wp-content/uploads/2017/08/Windows_31.png), [archive.org 3.1 shot](https://archive.org/details/windows-3.1-paint))

```
+--------------------------------------------------------------+
|[-]          Paintbrush - (Untitled)                   [v][^] |
+--------------------------------------------------------------+
| File  Edit  View  Text  Pick  Options  Help                  |
+-------+------------------------------------------------------+
|Sci|Pck|                                                    |^|
|Air|abc|                                                    | |
|CEr|Era|                                                    | |
|Rol|Brs|               drawing area (white)                 | |
|Crv|Lin|                                                    | |
|Box|FBx|                                                    | |
|RBx|FRB|                                                    | |
|Elp|FEl|                                                    | |
|Ply|FPl|                                                    |v|
+-------+                                                    +-+
|>____  |  <-------------- horizontal scroll ------------->   |
| ====  +------------------------------------------------------+
| ##### | [fg] |W |lG|R |Y |G |C |B |M |d1|d2|d3|d4|d5|d6|    |
| Linesz| [bg] |K |dG|dR|dY|dG|dC|dB|dM|d7|..|..|..|..|..|    |
+-------+------------------------------------------------------+
   Linesize box     palette: 14 x 2 (d* = dithered on 16-col VGA)
```

### Windows 95 Paint
The window has Win95 chrome: navy title bar ("untitled - Paint"), grey 3‑D bevels at RGB 192,192,192 and MS Sans Serif. The menus are File, Edit, View, Image, **Options**, Help. A 2 × 8 toolbox of 16 buttons is docked on the left, and under it is a sunken **options well** whose contents change with the tool: brush tips, eraser sizes, line widths, fill styles, zoom factors, opaque/transparent. The canvas is white in a scrolled client area. Along the bottom is the **colour box**: a two‑square foreground/background indicator, then 28 swatches in 2 × 14 with **black, grey and the dark colours on top**. A three‑pane status bar shows help text, cursor x,y, and selection or shape size. The Text toolbar floats. **[H]** ([winhistory 95 shot](https://www.winhistory.de/vpc/vwin95/paint95.gif), [archive.org paint95prev.PNG](https://archive.org/download/MSPaintWin95/paint95prev.PNG))

### Windows 98 Paint
Visually the same as 95: same toolbox, options well, colour box and status bar ([PC Plus 1998](https://discmaster.textfiles.com/file/35119/PC%20Plus%20SuperCD%2050b%20Issue%20142%20(CD142b)%20(August%201998).iso/full/W98Comp/Access/paint.htm?html=true): "surprisingly familiar form"). The visible difference is the menu bar, File, Edit, View, Image, **Colors**, Help, with Draw Opaque under Image **[M]**: taken from jspaint and a 98 tutorial, not from a 98 binary. jspaint's "Classic" theme at [98.js.org](https://98.js.org/) / [jspaint.app](https://jspaint.app) reproduces this layout. Pixel‑exact chrome (icon bitmaps, metrics) is **unknown** here.

```
+--------------------------------------------------------------+
|[ico] untitled - Paint                             [_][#][X]  |
+--------------------------------------------------------------+
| File  Edit  View  Image  Colors(98)/Options(95)  Help        |
+-----+--------------------------------------------------------+
|FFS|Sel|                                                    |^|
|Era|Fil|                                                    | |
|Pck|Mag|                                                    | |
|Pen|Brs|               canvas (white bitmap)                | |
|Air| A |                                                    | |
|Lin|Crv|                                                    | |
|Rec|Ply|                                                    | |
|Elp|RRc|                                                    |v|
+-------+                                                    +-+
|options|  <---------------- scroll -------------------->      |
| well  |                                                      |
+-------+------------------------------------------------------+
|[F]  |K |dG|Mr|Ol|Gr|Tl|Nv|Pu|Kh|DT|Az|SB|Vi|Br|               |
|  [B]|W |lG|R |Y |Li|Aq|Bl|Fu|PY|SG|PC|Lv|Ro|Or|               |
+--------------------------------------------------------------+
| For Help, click Help Topics...      | + 51,76  | [] 12x8     |
+--------------------------------------------------------------+
```

---

## 3. Tool‑by‑tool comparison

"—" means the tool is absent. The 95 and 98 columns differ only where noted; the preset pixel sizes come from jspaint's Windows 98 model and agree with ReactOS, but were **not measured on a real 95 or 98 [M]**.

| Tool | 3.1 Paintbrush | 95 Paint | 98 Paint | Canvas behaviour |
|---|---|---|---|---|
| Free‑form select | **Scissors** | Free‑Form Select | same | Lasso cutout. 3.1: left button drags transparent, right opaque. 95/98: an opaque/transparent option in the well, where transparent keys out exactly the current background colour. **[H]** |
| Rect select | **Pick** | Select | same | Ctrl+drag leaves a copy (3.1 and 95/98). Shift+drag **sweeps** (smear trail): documented in 3.1 (3.0 used the opposite keys); in 95/98 it is undocumented but known from tutorials and jspaint **[M]**. Shift+click stamps (tutorial only **[L‑M]**). |
| Selection transforms | Pick menu: Flip H/V, Inverse, Shrink+Grow (Shift keeps aspect), Tilt (horizontal shear), Clear | Image: Flip/Rotate (90/180/270), Stretch/Skew (%, °), Invert Colors | same | No arbitrary rotation in any of them. Enlarging duplicates pixels; whether shrinking averages is **disputed [L]**. A 500% per‑step stretch cap is reported for real Paint **[L]**. |
| Airbrush | Yes; diameter from Linesize | Yes; 3 sizes | same | Random hard 1‑px dots of solid colour inside a disc, with no falloff and no alpha. Density depends on drag speed. jspaint uses 9/16/24 px **[M]** and keeps spraying while the mouse is still. The real dot pattern and timing are **unknown**. |
| Text | Yes; Regular/Bold/Italic/Underline/**Outline/Shadow** (Outline and Shadow draw in the background colour) | Yes; drag a frame, Text toolbar (font, size, B/I/U); opaque/transparent | same | Rasterised system fonts, permanent once committed. 95/98 accept text only at 100% zoom **[H]**. Whether 98 font smoothing antialiased text into the bitmap is **unknown**. |
| Colour replace | **Color Eraser** (separate tool); double‑click replaces every occurrence in the *visible* area | Eraser right‑drag | same | Changes only foreground‑colour pixels to the background colour. **[H]** |
| Eraser | Yes; size from Linesize; double‑click = New | Square, 4 sizes (jspaint 4/6/8/10) | same | Paints the background colour, not transparency. Hard square bites. |
| Flood fill | **Paint Roller** | Fill With Color | same | Exact colour, no tolerance. Leaks through gaps in the border. A dithered or pattern fill can't be refilled (3.1) **[H]**. 4‑connectivity is true of jspaint; for the real program it is **inference [M]**. |
| Eyedropper | — | Pick Color | same | Left click sets the foreground, right click the background. |
| Magnifier | — (View > Zoom In, one undocumented factor) | Yes; 1/2/6/8× (jspaint also has a hidden 10×) **[M]** | same | 95/98 View > Zoom: Normal 100%, **Large 400%**, Custom 100/200/400/600/800%, Show Grid. **[H]** |
| Pencil | — (narrowest Brush, or edit pels in Zoom In) | 1 px, no options | same | Aliased Bresenham‑style line; Shift constrains to 0/45/90°. |
| Brush | 6 shapes: square (default), circle, ─, │, /, \\; size from Linesize | 12 presets: circle 7/4/1, square, / and \\ at 8/5/2 **[M]** | same | Hard stamped tip. The slash tips give thick/thin calligraphy that depends on stroke direction. |
| Line | Yes; Linesize | 5 widths, 1–5 px **[M]** | same | Aliased; Shift constrains to 0/45/90°. Right‑drag uses the background colour (95/98). |
| Curve | Yes; 1 or 2 bends | Yes; 1 or 2 arcs | same | Smooth but aliased Bézier‑like curve. |
| Rectangle / Rounded / Ellipse / Polygon | Separate **hollow** and **filled** tools (8 in all). Filled = **foreground interior, background border** | One tool each, with 3 fill styles: outline / outline+fill / fill only. Outline+fill = **foreground outline, background fill** **[H]** | same | Border width = line width **[M]**. Rounded corner radius fixed and **unknown**. Shift gives a square or circle. |
| Undo | 1 level (everything since the tool was selected), plus Backspace selective erase | 3 levels + Repeat **[H]** (95 .hlp) | 3 **[M]** | — |

---

## 4. Pixel facts an artist needs

**Default palette, Paint 95/98 (and through XP).** These values were read from the COLORREF table in MSPAINT.EXE 4.00.950 at file offset 0x37578 and match jspaint and ReactOS exactly **[H]** ([color-data.js](https://github.com/1j01/jspaint/blob/master/src/color-data.js), [archive.org binary](https://archive.org/details/MSPaintWin95)). Row 1 is the top row of the colour box.

| Col | Top row | Bottom row |
|---|---|---|
| 1 | 0,0,0 black | 255,255,255 white |
| 2 | 128,128,128 grey | 192,192,192 silver |
| 3 | 128,0,0 maroon | 255,0,0 red |
| 4 | 128,128,0 olive | 255,255,0 yellow |
| 5 | 0,128,0 green | 0,255,0 lime |
| 6 | 0,128,128 teal | 0,255,255 aqua |
| 7 | 0,0,128 navy | 0,0,255 blue |
| 8 | 128,0,128 purple | 255,0,255 fuchsia |
| 9 | 128,128,64 khaki | 255,255,128 pale yellow |
| 10 | 0,64,64 dark teal | 0,255,128 spring green |
| 11 | 0,128,255 azure | 128,255,255 pale cyan |
| 12 | 0,64,128 slate blue | 128,128,255 lavender |
| 13 | 64,0,255 violet | 255,0,128 rose |
| 14 | 128,64,0 brown | 255,128,64 orange |

- Columns 1–8 are exactly the 16 Windows VGA colours. Columns 9–14 are outside that set and were dithered on 16‑colour displays.
- Paintbrush 3.1 has 28 swatches with the rows swapped, but its **RGB values are unknown**: the screenshot samples look colour‑converted. Its last six columns look like the same pale/bright extras.
- In 256‑colour mode Windows reserves 20 static colours, the VGA 16 plus C0DCC0, A6CAF0, FFFBF0 and A0A0A4 **[H]**.
- Any swatch can be redefined with Edit Colors: 3.1 has RGB 0–255 sliders; 95/98 use the standard dialog with 48 basic and 16 custom colours. Palettes save as .PAL in 3.x–2000; from 95 on, .pal handling works correctly only at 16 bpp or higher.
- Default colours are foreground black and background white in every version. GDI snaps pen colours to the nearest palette entry and dithers brush colours on palette devices **[H]**, but which Paint tool wrote dithered pixels is **unknown**.

**Brush shapes.**
- 3.1: six shapes, at any of the 8 Linesize widths.
- 95/98: 12 presets, round 7/4/1 and square, `/` and `\` at 8/5/2 **[M]**. The diagonal tips are 1‑px slanted lines, so a stroke along the slant is hairline and a stroke across it is fat.
- Exact round‑tip bitmaps are **unknown**: jspaint's ellipses are admittedly not raster‑exact ([issue #158](https://github.com/1j01/jspaint/issues/158)).

**Airbrush.**
- 95/98: 3 sizes (jspaint 9/16/24 px diameter **[M]**). 3.1: diameter = Linesize.
- Every dot is a solid 1×1 pixel in the foreground colour (95/98: right button = background colour). Holding still builds the disc toward solid.
- Claims about real‑Paint fill times and "nubs" at the cardinal points are **unverified**, so they are dropped here.

**Line widths.**
- 95/98: 1–5 px, shared by Line, Curve and shape outlines **[M]**.
- 3.1: 8 Linesize presets, top = 1 pel; the others are **unknown** (they display at roughly 1, 2, 2, 2, 3, 4, 5 and 7 screen px).

**Eraser.** 95/98: square, 4/6/8/10 px **[M]**. 3.1: Linesize.

**Zoom.**
- 95/98: 100/200/400/600/800% from the menu ("Large Size" = 400%). Magnifier presets 1/2/6/8×. Show Grid at high zoom; jspaint gates it at ≥4× **[M]**. Show Thumbnail gives a 1:1 preview.
- 3.1: a single Zoom In factor, **unknown**; no grid.

**Fill.** Exact‑colour contiguous flood. No tolerance, no gradient, no pattern fill in 95/98. Pattern fills existed in 3.x on 16‑colour/B&W displays.

**No antialiasing.** No version before Windows 7 antialiases any tool. There are no alpha, layers or opacity; the only "transparency" is a single exact background colour dropped from a moving or pasted selection. **[H]**

**Text.** Installed Windows fonts rasterised to the bitmap. 3.1 adds Outline/Shadow in the background colour. 95/98 offer B/I/U and an opaque or transparent frame. Smoothing is **unknown**.

**Canvas size.** 3.1 default = display size. The 95/98 default is **unknown**: jspaint's 683×384 has no source.

**Formats.**
- 3.x: PCX (one option) or BMP at 1/4/8/24 bit; MSP import only.
- 95/98: BMP at 1/4/8/24 bit. PCX status in 95 is disputed **[L]**.

---

## 5. The recognisable "MS Paint" vocabulary and its techniques

Synthesis **[M]**, with sources in the reports.

| Mark | How it is made | 3.1 | 95/98 |
|---|---|---|---|
| Hard aliased keylines with stair‑steps | Pencil, Line at width 1, Curve | ✓ (Brush/Line at the smallest Linesize) | ✓ |
| Faceted polyline outlines (Pat Hines) | Line tool segments | ✓ | ✓ |
| Smooth C/S curves joined at kinks (Jim'll Paint It, reportedly **[L]**) | Curve, 1–2 arcs | ✓ | ✓ |
| Perfectly flat colour inside closed outlines; whole‑canvas leaks | Outline, then bucket | ✓ Paint Roller | ✓ Fill With Color |
| Keyline + fill in one drag | Filled shape | ✓ (border = background colour) | ✓ outline+fill (outline = foreground) |
| Speckled single‑pixel shading / cast shadows | Airbrush; knock back with a background‑colour pass | ✓ | ✓ |
| Thick/thin calligraphic strokes | `/` and `\` brushes | ✓ | ✓ |
| Pixel‑identical repeats | Ctrl+drag a selection | ✓ | ✓ |
| Smear trails / extrusions / "custom brush" | Shift+drag a selection | ✓ documented "sweep" | ✓ undocumented **[M]** |
| Hard‑edged composites with an occasional colour‑key fringe | Transparent paste keyed on the background colour (Jim'll paints backgrounds in a separate file and pastes the characters in) | ✓ left button | ✓ Draw Opaque off |
| One‑colour swap that leaves the keylines alone | Color Eraser / Eraser right‑drag | ✓ (and global double‑click) | ✓ |
| Sheared "perspective" | Tilt / Skew | ✓ Tilt | ✓ Skew |
| Blocky enlargements | Shrink+Grow / Stretch | ✓ | ✓ |
| Pixel‑by‑pixel cleanup at high zoom (Hal Lasko on 95 **[M]**) | Zoom + grid | ✓ (no grid) | ✓ |
| Checkerboard dither fills | 16‑colour display with an extra swatch | ✓ | unknown |
| Default‑palette hues (0/128/255 plus odd extras) | The colour box | ✓ (values unknown) | ✓ |
| Committed, imperfect strokes | 1 undo (3.1) or 3 undos (95/98) | ✓ | ✓ |

---

## 6. Implications for ColourWars' art rules

The accepted §1 rules ([polish-plan.md](polish-plan.md), [game/art/README.md](../game/art/README.md)) are Zoom 400% values, Tool Props terrain, airbrush cast shadows, black keyline under paint, and figure E with a teammate head accent. Gameplay frames are four texels per world unit ([game/art/README.md](../game/art/README.md)).

| Treatment | Real tool / version that produces it | Constraint "achievable in real Paint" imposes |
|---|---|---|
| **Zoom 400% values** | 95/98 View > Zoom > **Large Size** is exactly 400%, with Show Grid for cell work. 3.1 has a zoom of unknown factor, so it can't name the value. | Author at 1 source pixel = 1 Paint pixel. Display at **integer** 4× **nearest‑neighbour** with no filtering. Edges are hard cells only; no sub‑cell detail and no smooth rotation of the source art. A rotated weapon is a real‑Paint impossibility beyond 90° steps, so rotation is a runtime concession and should be named as one. |
| **Tool Props terrain** | Rectangle/Polygon/Rounded Rectangle with the outline+fill style (95/98), or Filled Box/Polygon (3.1). Line at 1–5 px for edges, Ctrl+drag stamped repeats for kit pieces, Shift+drag sweeps for extruded edges or rails, Skew/Tilt for fake depth. | Outline widths from a fixed set (1–5 px). Corner radius fixed. Shapes axis‑aligned or 45°‑constrained, with free angles from Polygon only. Repeats are pixel‑identical copies, not variants. Perspective is shear only. |
| **Airbrush cast shadows** | Airbrush, 3 sizes (95/98) or a Linesize diameter (3.1), all versions. | Shadow = opaque single‑pixel speckle of **one palette colour**, denser where dwelt. **No alpha, no gradient, no blur.** A darker shadow needs a second pass in a second colour, not opacity. Speckle extent fits a 9/16/24‑px disc (jspaint values, **[M]**). At runtime this means a speckle texture of solid texels, not a translucent multiply. |
| **Black keyline under paint** | Pencil or Line at width 1 in black, then Fill. Or 95/98 outline+fill with black as foreground. In 3.1, filled shapes with **black as the background** colour. Paint recolouring with the eraser's right‑drag (95/98) or the Color Eraser (3.1) keeps the keyline intact. | Keyline in whole pixels (1 px at source, so 4 screen px at 400%). Fills butt hard against the keyline with no halo. The keyline must be **closed**, or the fill leaks. "Under paint" means paint pixels overwrite flat colour but stop at, or sit on, the black line: there is no blended edge. |
| **Box‑head figures, E with head accent** | Rectangle (outline+fill) for head and body, Pencil for features, Curve for limbs or tools. The head accent is a Fill or eraser‑right‑drag swap in the team colour. | Figure colours drawn from a fixed palette. A team swap is an exact‑colour replace, which needs **dedicated key colours** in the source (the repo already separates exact paint‑material IDs). Mouse wobble is allowed, but every edge stays a hard cell. |

### Decided by the author (2026-10-17)

- **Version: Paint 98.** Assets should be achievable with Windows 98 Paint's tools,
  presets and limits. jspaint's Windows 98 model is the working reference.
- **Colour: 24-bit true colour, 28 swatches per level.** Any RGB value is allowed, but a
  level uses at most 28 colours, the size of the colour box, as if it were drawn from
  one saved .pal. Each level therefore gets its own restricted look. 16-bit was rejected
  because it changes nothing visible in flat, unantialiased art.
- **No dithering** by default: true colour wouldn't produce it. If it comes back, it
  comes back as a deliberate texture when the background kit (`POL#9`) is built.

- **Everything counts within the 28:** the level, the characters, the team paint
  colours and the HUD. Every colour on screen comes from that level's one colour box.
  So the team colours each level supports take slots from its 28.

### Open decisions for the author

Items 1, 2 and 5 below are answered above; they stay as the reasoning that was offered.

1. **Which version to be faithful to.** *Recommendation: Paint 95/98 (MSPAINT 4.x), using jspaint ([jspaint.app](https://jspaint.app)) as the practical working tool.* Reasons:
   - Its 28‑colour palette is verified from the actual binary.
   - "Zoom 400%" is literally its Large Size.
   - It has the 1‑px Pencil and the Pick Color eyedropper (3.1 has neither), and outline+fill shapes match the "keyline + fill" rule directly.
   - It has documented preset sizes to cite, and a browser clone you can draw in today without an emulator.
   - 95 and 98 differ only in menu naming, so "95/98" can be one target.

   3.1 is the more exotic choice: Color Eraser global swap, documented sweeps, dithers. But its palette RGB and zoom factor are unknown, and its filled shapes reverse the colour roles.
2. **Palette: default 28 only, or Edit Colors allowed?** Both are "real". A strict default palette is maximally recognisable but has few greys and few dark values for a "quiet background". Edit Colors allows any RGB but only **28 slots at a time**, which fits the "one palette slot" theme. Suggested middle option: a fixed, saved .pal of 28 entries, starting from the default and with documented replacements.
3. **Must rendering stay aliased at integer scale?** Real Paint implies nearest‑neighbour, integer magnification, no rotation beyond 90°, and no alpha. Decide which runtime concessions are allowed and name them: continuous weapon rotation, non‑integer split‑screen scaling, camera sub‑pixel motion, alpha on HUD or shadow. Otherwise each one becomes an implicit exception.
4. **Is the "real program" test literal or stylistic?** Literal means every source asset must be reproducible in jspaint, using its preset sizes. Stylistic means the asset only has to look like it could be. This decides whether the current System.Drawing recipes in `scripts/build-*-art.ps1` are acceptable as they stand.
5. **Are 16‑colour dithers allowed?** They are authentic for 3.x but unverified for 95/98. Allow them as a texture, or exclude them because the chosen version is 95/98.

---

## 7. Sources

**Primary documents and binaries**
- Windows 3.0 User's Guide (1989): https://ftp.mirrorservice.org/sites/www.bitsavers.org/pdf/microsoft/windows_3.0/Windows_3.0_Users_Guide_198909.pdf
- Windows 3.1 User's Guide (1992): https://ftp.mirrorservice.org/sites/www.bitsavers.org/pdf/microsoft/windows_3.1/Microsoft_Windows_3.1/PC21669-0492_Windows_3.1_Users_Guide_Apr92.pdf · https://archive.org/details/bitsavers_microsoftwindows3.1PC216690492Windows3.1UsersGuide_27129713
- PBRUSH.HLP as PDF: https://discmaster.textfiles.com/file/4696/Tandy%20Sensation%20II%20System.iso/mwin/pbrush.hlp/pbrush.pdf
- Windows 2.x manuals: https://archive.org/details/microsoft-windows-2.x-manuals
- Win95 MSPAINT.EXE + help + screenshot: https://archive.org/details/MSPaintWin95
- Windows 95 Road Map (Dec 1995): https://discmaster.textfiles.com/file/28542/The%20Best%20Shareware%20of%20the%20Year%201997%20Winter.iso/library/1995/1295/12featc.htm?html=true
- PC Plus, Win98 Paint (Aug 1998): https://discmaster.textfiles.com/file/35119/PC%20Plus%20SuperCD%2050b%20Issue%20142%20(CD142b)%20(August%201998).iso/full/W98Comp/Access/paint.htm?html=true
- Kaiser 1996 column: https://kaisernet.org/library/1996/0796/07h01001.htm
- KB Q298580 (forum mirror): https://www.helpmij.nl/forum/threads/paint.105701/
- GDI palette and dithering: https://learn.microsoft.com/en-us/windows/win32/gdi/default-palette · https://learn.microsoft.com/en-us/windows/win32/gdi/system-palette-and-static-colors
- VS6 image editor (Shift‑drag trail): https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-6.0/aa233584(v=vs.60)

**Screenshot galleries**
- Windows 3.0 Paintbrush: http://toastytech.com/guis/win30paint.png
- bc-programming series (1.04, 2.1, 3.0, 3.1, fill fail): https://bc-programming.com/blogs/2017/08/the-deprecation-of-microsoft-paint/
- Windows 3.1 Paint (VirtualBox): https://archive.org/details/windows-3.1-paint
- Windows 95 Paint: https://www.winhistory.de/vpc/vwin95/paint95.gif · https://archive.org/download/MSPaintWin95/paint95prev.PNG
- Palette chart: https://commons.wikimedia.org/wiki/File:Microsoft_Paint_color_palettes.png
- Live Windows 98 reproduction: https://98.js.org/ · https://jspaint.app

**Reimplementations (good models, not ground truth)**
- jspaint source and help: https://github.com/1j01/jspaint (`src/tools.js`, `src/tool-options.js`, `src/color-data.js`, `src/image-manipulation.js`, `help/`); ellipse caveat: https://github.com/1j01/jspaint/issues/158
- ReactOS mspaint: https://github.com/reactos/reactos/tree/master/base/applications/mspaint

**Secondary and culture**
- https://en.wikipedia.org/wiki/Microsoft_Paint · https://en.wikipedia.org/wiki/List_of_software_palettes
- Tutorials: https://www.albinoblacksheep.com/tutorial/mspaint · https://nerdparadise.com/mspaint/randomtips
- Artists: https://en.wikipedia.org/wiki/Jim%27ll_Paint_It · https://thequietus.com/culture/art/jim-ll-paint-it-interview/ · https://www.printmag.com/comics-animation-design/pat-hines-master-of-microsoft-paint/ · https://www.foxnews.com/tech/97-year-old-pixel-painter-creates-digital-masterpiece-on-windows-95

### Remaining uncertainty
These are not settled by any source above:
- Paintbrush 3.x palette RGB values, zoom factor and Linesize pixel widths.
- The real 95/98 airbrush algorithm, round‑brush and ellipse bitmaps, rounded‑rectangle radius and default canvas size.
- Whether 95/98 shrink averages pixels.
- Whether 98 text was antialiased.
- 4‑ vs 8‑connected fill in the real program.
- 95 vs 98 brush, eraser and airbrush preset sizes (taken from jspaint's 98 model).
- PCX in 95.
- ME automatic canvas expansion.
