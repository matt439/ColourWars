# Study A: run sheet for the experimenter

The design is section 8 of
[drawing-mistakes-research.md](../../../docs/drawing-mistakes-research.md).
This sheet is the procedure. Read it with the
[participant sheet](participant-sheet.md), the [consent sheet](consent.md) and
the [codebook](codebook.md).

## Before the first session

1. **Fix the rules first.** The candidate rule, the kappa target (0.6) and the
   thresholds at the top of `analyse.py` are set before any drawing is seen. If
   one has to change afterwards, say so and why in the write-up.
2. **Choose a study folder outside the repository**, for example
   `"$env:USERPROFILE\Documents\StudyA"` in PowerShell. Nothing from a
   session goes in the repository, not even under `out/`. Lay it out like
   this:

   ```
   StudyA\
     references\     make-references.py output
     consent\        signed sheets, and the name-to-ID key, kept apart
     drawings\       pid_task_condition_item.png
     recordings\     pid.mp4
     session-log.csv
     coding\         coding-A.csv, coding-B.csv, coding-resolved.csv
     analysis\       analyse.py output
   ```

3. **Make the references** into that folder, from the repository root in
   Windows PowerShell:
   `python scripts\studies\study-a\make-references.py --out "$env:USERPROFILE\Documents\StudyA\references"`.
4. Fill in the two dates and the contact line on the consent sheet.
5. **Pilot on yourself first** (half a day), as `p00`. Run the whole session,
   time it, then run `analyse.py` on your own drawings and code them. Fix the
   materials, not the rules. Pilot data is not counted.

## Participants

6-10 adults with mixed mouse skill. At least 2 have never used a pixel editor.
Nobody who will take part in Study B.

## Schedule

The person is always first. The second item and the canvas conditions follow
this cycle of four, repeated from `p01`:

| ID | Person | Second item |
|---|---|---|
| p01, p05, p09 | Z | Bricks, F |
| p02, p06, p10 | F | Brush pot, Z |
| p03, p07 | Z | Brush pot, F |
| p04, p08 | F | Bricks, Z |

Each item is done as D, then C, then T, 5 minutes each. If time allows,
finish with the optional run frames (below).

## Setting up jspaint

Use [jspaint.app](https://jspaint.app) in a fresh browser profile or private
window. Check each step in the pilot, because jspaint's menus can change.

- Theme: Extras > Themes > Classic.
- Canvas: File > Open the blank file for the item and condition,
  `<item>_blank_z.png` or `<item>_blank_f.png`. That sets the size.
- Z only: View > Zoom > Custom, 400%, and View > Zoom > Show Grid (Ctrl+G).
- F: zoom 100%, no grid.
- D: the canvas only; the participant reads the description from the
  participant sheet.
- C: open the reference in an image viewer beside the canvas, at 100%:
  `<item>_grid400.png` for Z (it looks like the zoomed canvas) and
  `<item>_f.png` for F.
- T: File > Open `<item>_trace_z.png` or `<item>_trace_f.png` instead of the
  blank file. Its pale greys read as paper in the analysis, so the participant
  need not clean them up.

## The session (about 35 minutes)

1. Consent. Give the participant ID. Ask which hand they mouse with, whether
   they have used a pixel-art or paint program, and their mouse skill from 1 to 5.
2. Start the screen recording: screen only, no camera, no microphone (for
   example Windows Game Bar, Win+Alt+R, or OBS).
3. One minute of free practice on a blank canvas, so the tools are not new.
4. For each item and task: set up the canvas, start a 5-minute timer, say when
   1 minute is left, and stop at 5.
5. Save at once with File > Save As, named `pid_task_condition_item.png`, for
   example `p03_C_Z_figure.png`. Items are `figure`, `bricks` and `brushpot`.
   Move the file from the browser's downloads into `drawings\`.
6. After each drawing, write a row in the session log.
7. Stop the recording and save it as `recordings\pid.mp4`.

**Optional run frames.** Open `run_frames_blank_z.png` (35 x 30, 400%, grid
on) and show `run_frames_grid400.png`: the first two run frames side by side.
Ask for a copy of both in 5 minutes. Note only whether the person copy-pasted
the shared top half or redrew it; if most copy-paste, identical frames are
authentic. Save it as `pid_C_Z_runframes.png`. `analyse.py` lists it as
skipped and does not score it.

## Session log

One row per drawing in `session-log.csv`:

| Column | What |
|---|---|
| pid | `p03` |
| hand | left or right |
| editor_before | yes or no: used a pixel or paint editor before |
| mouse_skill | 1-5, self-rated |
| consent | `signed` or `verbal` (the sheet read aloud and agreed) |
| consent_date | the date consent was given, for example 2026-11-01 |
| file | the drawing's file name |
| minutes | time used, if under 5 |
| undo_count | undos counted from the recording |
| undo_breach | yes if any undo went further back than Paint 98 could (see below) |
| tools | tools used, in order of first use |
| fill_leak | yes if a bucket fill escaped and was noticed |
| copy_paste | for run frames: copied, redrew, or mixed |
| notes | anything else |

**Counting undo breaches.** From the recording, keep a count of undoable
steps: each action (a stroke, a fill, a paste) adds one, up to 3, and each
undo uses one. An undo when the count is already 0 is a breach, because it
reached a step Paint 98 had lost. For example, undo 3 times, draw a stroke,
then undo 3 more: the second of those last undos is a breach.

## After the sessions

1. Run `analyse.py` once to make `coding-template.csv` and the cell images.
2. Each coder copies the template to `coding\coding-<name>.csv` and codes
   **every** drawing from the original PNG, `cells\` and `overlay\`, using the
   [codebook](codebook.md). Coders work apart and do not see each other's file.
3. Run `analyse.py` again with `--coding`. Check kappa against 0.6. Settle the
   listed disagreements together and write `coding-resolved.csv` (same columns).
   It needs only the rows you settle; every other row keeps the coders'
   agreement. The report lists each coder's blank or missing rows. They should
   be 0: fill any blank before this step.
4. Run it a last time. Take the candidates to section 4's exclusions.
5. On the date given in the consent sheet, delete everything that holds a
   participant's drawing or can link to them:
   - `drawings\`, every PNG;
   - `recordings\`;
   - `analysis\cells\`, `analysis\overlay\`, `analysis\drawings.json` (it
     holds stray coordinates) and `analysis\coding-template.csv`;
   - `coding\` and `session-log.csv`;
   - the name-to-ID key, and the participant ID on any signed consent sheet
     you must keep.

   Only `analysis\report.md`, which holds counts and sizes, is kept. Nothing
   but its counts leaves the study folder.
