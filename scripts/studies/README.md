# Drawing-mistake studies

The kits for the two human studies in
[drawing-mistakes-research.md](../../docs/drawing-mistakes-research.md#8-human-tests)
(`POL#46`). Study A asks what mistakes people make when they draw the game's art
with a mouse at 400%. Study B asks whether stage 1 still reads as well as the
art before it, and whether people prefer it. Running them is the author's job;
these scripts only prepare, present and score.

No participant data, drawing, recording or result enters the repository. Every
script that writes refuses or warns when its output would land in the tree
outside `out/`, and the reports are written where you point them.

- [Study A kit](#study-a-kit-what-mistakes-do-people-make)
- [Study B stimuli](#study-b-stimuli)
- [Study B: test page and analysis](#study-b-test-page-and-analysis)

## Study A kit: what mistakes do people make?

[study-a/](study-a/) holds everything Study A needs: the reference images, the
sheets for participants and the experimenter, the codebook, and the analysis.
The design is section 8 of
[drawing-mistakes-research.md](../../docs/drawing-mistakes-research.md); stage 2
(the figure) waits on its result.

| File | What it is |
|---|---|
| [make-references.py](study-a/make-references.py) | Makes the reference images from the repository's art sources. Standard library only. |
| [participant-sheet.md](study-a/participant-sheet.md) | What the participant reads: the rules, the canvases, the three tasks and the exact descriptions. |
| [consent.md](study-a/consent.md) | The consent sheet. |
| [run-sheet.md](study-a/run-sheet.md) | The experimenter's procedure: schedule, jspaint setup, timing, what to save and where, coding. |
| [codebook.md](study-a/codebook.md) | The 13 codes, the fixed thresholds and the candidate rule. |
| [analyse.py](study-a/analyse.py) | Scores the drawings, merges the coders' codes, computes kappa and writes `report.md`. Needs Pillow. |
| [selftest.py](study-a/selftest.py) | Checks the kit end to end on synthetic drawings. Needs Pillow. |

### Data stays out of the repository

No participant pixels, recordings or notes enter the repository. Real sessions
use a study folder outside the checkout ([run-sheet.md](study-a/run-sheet.md)
gives the layout). Both scripts refuse to write inside the repository except
under the git-ignored `out/`. Only the mistake patterns, as counts and sizes,
come back into the game's docs.

### Requirements

- Python 3.9 or later. Run the scripts with `python` from Windows PowerShell 5.1,
  Git Bash or any shell; none of them uses PowerShell itself.
- `make-references.py` uses the standard library only. It reads and writes PNGs
  itself (8-bit, non-interlaced RGB or RGBA, which is what System.Drawing writes).
- `analyse.py` and `selftest.py` need **Pillow, installed outside the
  repository**: `python -m pip install --user pillow`, or a venv outside the
  checkout. The repository does not depend on it. Without it both scripts stop
  with that instruction.

### Making the references

```
python scripts/studies/study-a/make-references.py --out <study folder>/references
```

Without `--out` it writes to `out/study/study-a/references`. The output is the
same bytes every run; it changes only when the sources do, and `items.json`
records each source's SHA-256.

| Item | Source | Cells | F canvas |
|---|---|---|---|
| `figure` | [painter.json](../../game/art/painter/painter.json): torso, idle legs, rest arms drawn on top | 17 x 30 | 136 x 240 (8 px a cell) |
| `bricks` | the halved bond in [terrain_material_drawings.cpp](../../game/objects/terrain_material_drawings.cpp), 6 bricks by 3 courses | 49 x 13 | 392 x 104 (8 px a cell) |
| `brushpot` | the `brush_pot` frame cropped from `game/art/sources/textures/terrain.png` at its position in `game/content/textures/terrain.json` | 64 x 80 | 256 x 320 (4 px a cell) |

Choices made so the task is drawable, and recorded here:

- **Colours are Paint 98 palette colours**, so participants can match them:
  keyline black, team tint `T` and the brick fill mid grey `808080`, white
  stays white. The brush pot's `bg_line` becomes black, `bg_mid` `808080` and
  `bg_far` `C0C0C0`. The study is about shape, not colour.
- **The bricks are at quarter scale.** In the game a course is 16 cells and a
  brick 32, which makes a 6 x 3 wall 193 cells wide. Here a course is 4 cells
  and a brick 8, with one-cell keylines and joints and the same halved bond.
  The script checks the game's constants and stops if they change.
- **The brush pot's F canvas is 4 px a cell**, its atlas size, because 8 px
  would not fit a laptop screen at 100%.

For each item it writes `<item>_cells.png` (one pixel a cell, the answer the
analysis uses), `<item>_grid400.png` (as jspaint shows it at 400% with the
grid), `<item>_f.png` (the F reference), `<item>_trace_z.png` and
`<item>_trace_f.png` (pale underlays for task T, which the analysis reads as
paper), `<item>_blank_z.png` and `<item>_blank_f.png` (opening one sets the
canvas size), and `items.json`. It also writes `run_frames_grid400.png` and
`run_frames_blank_z.png` for the optional run-frame question, which is not
scored.

### Analysing

```
python scripts/studies/study-a/analyse.py --references <study>/references --drawings <study>/drawings --coding <study>/coding --out <study>/analysis
```

Drawings are named `pid_task_condition_item.png`, for example
`p03_C_Z_figure.png`. Other files are listed as skipped. `--out` defaults to a
folder beside the drawings, and `--coding` to the drawings folder.

What it does, in order:

1. Classifies each pixel as ink (no channel above 64), paper (white, or every
   channel 208 or more, which includes the trace underlay) or fill.
2. Aligns each drawing to the reference by the best overlap, ink IoU plus
   half the drawn-cell IoU. Z drawings are already cells and every
   translation is tried. F drawings are reduced to cells with **any-ink for
   black and majority vote for fills**: every block size (one uniform scale)
   from 0.5 to 1.6 times the item's nominal size, in quarter pixels, is tried
   at every origin within a cell of the centred guess, by a fast box average
   in Pillow; the five best are re-scored with the exact reduction. The
   centred guess uses everything drawn except stray-sized pieces, so a figure
   drawn in several pieces is aligned as a whole. About 1.5 s an F drawing.
3. Scores the automatic codes (ASYM, GAP, OVER, UNDER, PROP, PLACE, STRAY) as
   [codebook.md](study-a/codebook.md) defines them. A drawing whose drawn-cell
   IoU is below 0.25, usually a D drawing, is marked unaligned and gets none.
   A code with nothing to measure on an item is not computable there and stays
   out of that code's shares: ASYM and PLACE on the brush pot, UNDER on the
   bricks.
4. Reads `coding-<coder>.csv` files for the manual codes (WOB, WEIGHT, FILL,
   OMIT, ADD, GRID), computes Cohen's kappa between the first two coders per
   code and pooled, and lists their disagreements and each coder's blank or
   missing rows. A manual code counts where every coder marked it, except
   where `coding-resolved.csv` settles that row; the resolved file needs only
   the rows it settles.
5. Writes `report.md`: per code, the drawings with it, its share of C and of T
   drawings, the people in each, the median size in cells, whether it survives
   reduction, and the **candidate** flag (at least 30% of the C drawings from
   at least 3 people among them, or the same within the T drawings, and
   survives reduction). It also writes
   `drawings.json`, `coding-template.csv`, and `cells/` and `overlay/` images
   for the coders.

All of it is deterministic: no randomness, files in sorted order, ties broken
by a fixed rule.

### Checking the kit

```
python scripts/studies/study-a/selftest.py
```

It makes the references and sixteen synthetic drawings in a temporary folder,
runs `analyse.py` and checks the result, then deletes the folder. `--keep
<folder>` works in a folder and keeps it; the folder must be new, empty, or
kept by an earlier self-test (it carries a `.study-a-selftest` marker), so a
real study folder is refused rather than overwritten. The drawings are the
references perturbed in known ways:

| Drawing | Change | Expected |
|---|---|---|
| p01 C Z figure | none | no codes |
| p02 C Z figure | left eye one cell up | PLACE 1, ASYM 2 pairs |
| p03 T Z figure | one cell missing from the top of the cap | GAP, size 1 |
| p04 C Z figure | two stray cells | STRAY 2, size 1 each |
| p05 C Z figure | brim one cell longer | OVER, size 1 |
| p06 T Z figure | brim one cell shorter | UNDER, size 1 |
| p07 C F figure | drawn at 7 px a cell, offset (9, 3) | no codes; fit found as 7 px at (9, 3) |
| p08 T Z bricks | one joint a cell to the right | PLACE 1, ASYM 6 pairs |
| p09 C F brushpot | none, at 4 px a cell | no codes |
| p10 D Z figure | an unrelated box | unaligned |
| p11 C F figure | left eye up, one stray pixel | PLACE 1, ASYM 2, STRAY 1 |
| p12 C Z figure | two rows taller, on a larger canvas | PROP, size 1 |
| p13 C Z figure | no neck, so head and body are two pieces | GAP 2, size 1 each; fit at offset (0, 0) |
| p14 C F figure | the same at 8 px a cell | GAP 2, size 1 each; fit 8 px at (0, 0) |
| p15 C F figure | 1-px pencil lines wandering 1 px, fills under the lines | no codes; fit 8 px |
| p16 C F bricks | 1-px pencil lines wandering 2 px, fills not touching them | no codes; fit 8 px |

It also checks that ASYM and PLACE are not computable on the brush pot and
UNDER on the bricks. Then it fills two coders' files: WOB with known codes
(kappa 0.667; not a candidate, because its 33% of T drawings is one person),
WEIGHT on four people's C drawings (a candidate), and one row left blank by
coder B (reported). A `coding-resolved.csv` with one row must change only that
row. A misnamed file must be skipped. Last run: 2026-11-01, Python 3.14.2 and
Pillow 12.1.1, PASS in about 25 s.

### Limits

- The automatic codes are cell rules, not judgement. OVER and UNDER find only
  stroke ends with one ink neighbour, so an overshoot that merges into a thick
  corner is missed and left to the coders. PLACE is defined only for the
  person's eyes and mouth and the brick joints.
- ASYM compares counts of asymmetric pairs, so a drawing that fixes one of the
  reference's asymmetries and adds another nets to zero. It is scored only on
  the figure and the bricks; `items.json` records each item's choice.
- F alignment searches 0.5 to 1.6 times the nominal block size and origins
  within a cell of the centred guess. A drawing far outside that, or one
  whose bounding box is badly wrong (a large part left out at one edge), may
  fit poorly; the overlay shows it.
- jspaint's menu paths in the run sheet should be checked in the pilot.
- The kit has been run only on synthetic drawings. The pilot is its first real
  test.

## Study B stimuli

[capture-study-b.ps1](capture-study-b.ps1) makes the images for Study B in
[the drawing-mistakes research](../../docs/drawing-mistakes-research.md#study-b-is-it-charming-and-is-it-readable).
It builds each variant, runs it on a seeded match with paint bots, and copies
the screenshots into one folder with a manifest. It holds no participant data.
It writes everything under `-OutputDirectory` except git's records for the
worktrees and their submodule clones, which git keeps under
`.git/worktrees/`. It changes no tracked file and leaves the repository's
`.git/config` as it found it.

### Variants

| Variant | Build |
|---|---|
| `V0` | The merge base of `-Revision` and `master`: the art before stage 1. |
| `V1` | `-Revision` (default `HEAD`): stage 1, with one-cell brick slips. |
| `V1b` | `-Revision` with `BRICK_SLIP_CELLS = 2.0f` in `game/objects/terrain_material_drawings.cpp`. |

Each variant is a detached git worktree under
`<OutputDirectory>/worktrees/<variant>`, built with the `x64-debug` preset.
It must be Debug, because Release compiles out the console, scripts, seed and
screenshot commands. The `V1b` edit is made in its worktree only, never in the
main tree. The engine submodule is cloned from the main checkout's
`.git/modules/external/labrador`, so no network is needed. That URL is given
to the one `git submodule update` command with `git -c`, never written with
`git config`: a linked worktree shares the main repository's `.git/config`,
so a write there would change the main checkout's submodule URL. The script
compares that config's `submodule.*` entries before and after the update and
puts them back, with a warning, if git changed them. Each build goes
through a generated `build-<variant>.bat` that calls `vcvars64.bat` and sets
`VCPKG_ROOT` to Visual Studio's bundled vcpkg. Configure runs only when the
worktree has no `CMakeCache.txt`. Revisions are resolved once, at the start.

### Run

From Windows PowerShell 5.1, in the repository:

```powershell
# Everything: 3 variants x 2 sizes x 6 palettes, Crossfeed and Press Tower, 4/2/1 players.
powershell -ExecutionPolicy Bypass -File scripts\studies\capture-study-b.ps1

# A small set from builds that already exist.
powershell -ExecutionPolicy Bypass -File scripts\studies\capture-study-b.ps1 `
    -SkipBuild -Resolutions 1280x720 -Palettes blue_orange,purple_gold -Players 4
```

| Flag | Default | Does |
|---|---|---|
| `-Variants` | `V0,V1,V1b` | which builds to make and capture |
| `-Resolutions` | `1280x720,1920x1080` | window sizes, one game run each |
| `-Palettes` | all six pairs | team pair, one game run each |
| `-Players` | `4,2,1` | player counts, one pass each inside a run |
| `-Stages` | `crossfeed,press_tower` | the stages, one pass each per player count |
| `-IdentityCues` | `on,off` | with or without the P1-P4 badges and teammate markers |
| `-Seed` | `7` | the round seed |
| `-Revision`, `-BaseBranch` | `HEAD`, `master` | V1's commit; V0 is their merge base |
| `-SkipBuild` | off | use the existing worktree builds |
| `-SkipCapture` | off | build only |
| `-OutputDirectory` | `out/study` | everything the script writes; a relative path is resolved against the caller's PowerShell location |
| `-RunTimeoutSeconds` | `900` | the limit for one game run, after which it is killed |

The full matrix is 36 game runs of 24 images each, 864 images. On MattPC a
run of one stage, three player counts and both cue settings took 38 seconds
(2026-11-01). Each Debug build takes about ten minutes the first time.

### What one game run does

Each run is one variant at one size and palette. It sets `LOCALAPPDATA` for the
game to `<OutputDirectory>/appdata/<variant>-<size>-<palette>`. That folder
holds a fresh `ColourWars/save_data.json` with only the display section
(`save_version` 5, the resolution, `fullscreen` false and `team_palette`), and a
copy of the `study_b_*.cfg` scripts in `ColourWars/debug/`, which is where
`exec` finds them. The player's own save is never read or written.

The launch arguments are
`+script.background 1 +intro off +exec study_b_begin <seed>`, then for each
stage, player count and cue setting `+exec study_b_frames <stage> <n> cues-<on|off>`,
then `+quit`. `script.background 1` lets the run go unattended in a window that
is not in front, but the window must not be minimised.

- [study_b_begin.cfg](study_b_begin.cfg) puts shots in `screens\studyb\`
  without the status strip, fixes the seed, turns the overlays off and turns
  the cursor marker off. The marker follows the real mouse pointer, so leaving
  it on would put the pointer's position into the stimuli.
- [study_b_frames.cfg](study_b_frames.cfg) has the same shape as
  `review_arena_pass`. Every seat is a paint bot, at timescale 4. It takes a
  paused `clean` frame at GO and a paused `heavy` frame 20 match seconds later.
  Each shot follows a `study-b: shot <name>` marker and a `checksum` line.
- [study_b_cues_off.cfg](study_b_cues_off.cfg) and
  [study_b_cues_on.cfg](study_b_cues_on.cfg) turn
  `presentation.player_identity.enabled` and
  `presentation.hud.teammate_marker.enabled` off and on again around the
  cue-off pass. The badge setting applies when the match screen is next
  opened, and each `stage` in `study_b_frames` opens a new one, so it takes
  effect for the pass. The teammate markers change at once. The HUD label in each pane's corner
  (`P1 / TEAM A`, and `Bot: paint` because a bot drives the seat) stays in
  both settings: it names the pane, not the figure. Crop it out for a find
  test that must not show the team.

### Output

- `stimuli/<variant>/<size>/<n>p/<palette>/<stage>-<clean|heavy>-cues-<on|off>.png`.
- `stimuli/manifest.json`: each variant's revision, `identical_images` (see
  below), then one entry per image with `variant`, `revision`, `resolution`,
  `players`, `palette`, `stage`, `shot`, `frame`, `identity_cues`, `seed`,
  `path` (relative to `stimuli/`), `sha256`, the frame's `checksum`, `update`
  and `paint`, and `palette_pixels` (the palette check's counts, heavy frames
  only).
- `stimuli/simulation.json`: for each size, palette, stage, player count, frame
  and cue setting, the checksum line per variant and whether they are equal.
- `logs/`: the build logs and each run's transcript (`+logfile`).

Some images are byte-identical, and `identical_images` lists each group of
paths with one `sha256`. The full run of 2026-11-01 had 72 groups, each of two
images:

- 48 groups of 1P frames. A 1P frame shows only team A's colour, and
  `blue_orange` and `blue_yellow` share team A's blue, so their 1P frames are
  the same image. 1P frames add nothing to a check across the six palette
  pairs.
- 24 groups where V1b equals V1: every `press_tower` 2P heavy frame, at both
  sizes, in all six palettes and both cue settings. No slipped brick is in
  view there, so a paired V1 and V1b trial on them would show one image
  twice.

A test page must drop or dedupe trials whose images share a `sha256`.

A capture deletes `stimuli/` first, so the folder always matches its manifest.
A smaller run replaces a full one. The worktrees live in the same
`-OutputDirectory`, so `-SkipBuild` needs the folder the builds were made in.

### The art-only check

What is compared is the Debug `checksum` line logged just before each shot:
the running match's state hash, the update number and the paint per team. The
script fails if, for any frame, these differ between the variants it captured,
or between the cue-on and cue-off passes of one variant. Equal lines mean
the variants ran the same simulation, so the pictures differ only in how
it was drawn. Pixels are not compared between variants. The script also
checks that every PNG has the requested size.

The game keeps its default pair when the save's `team_palette` names no pair,
so a wrong or renamed key would still capture, with the wrong colours. The
script therefore checks each heavy frame for its palette. It reads the pair's
colours from the variant's own `game/content/presentation.json`, samples every
second pixel in each direction, and counts the pixels within 12 per channel of
each palette colour. Each team colour the frame should show (team A only in a
1P frame) must be counted at least 50 times and more often than any colour of
another pair. On the 2026-11-01 run all 432 heavy frames passed, and frames
deliberately checked against the wrong pair were refused. The check cannot
tell `blue_orange` from `blue_yellow` in a 1P frame, which shows only blue.

On 2026-11-01 a pixel diff of the V0 and V1 heavy frames (1280x720, 4P,
`blue_orange`) found 2,934 changed pixels, all in stage 1's art: the clouds,
hills, sun ray, tape roll, brick joints and the simplified tools. The figures,
paint and HUD were identical. V1 against V1b differed only at brick joints, and some frames show no slipped
brick at all, so they are identical (see Output).

### Not covered yet

- The intro's reveal and overview frames, which the design asks for. These
  runs use `+intro off`.
- Clips (idle, run, turning around, respawn redraw, camera pan).
- Colour-blind simulations, team-ID crops and both team positions. These are
  made from these PNGs in later analysis, not captured here.
- V2 and V3, which do not exist yet.

## Study B: test page and analysis

`study-b/` runs the tests of Study B in
[drawing-mistakes-research.md](../../docs/drawing-mistakes-research.md#study-b-is-it-charming-and-is-it-readable)
and scores them against the pass rules fixed there before anyone takes part.

| File | What it is |
|---|---|
| [study-b/index.html](study-b/index.html) | The test page. One file, no network, no build step. |
| [study-b/analyse.py](study-b/analyse.py) | Applies the pass rules to exported results. Standard library only. |
| [study-b/selftest.py](study-b/selftest.py) | Builds a synthetic fixture in a temporary folder and checks both of the above. Needs Node.js. |

### Participant data stays out of the repository

- The page never sends anything anywhere. Results leave it only through its
  **Export results** download, which lands in the browser's download folder.
- Move each results file to a folder outside the repository, or under `out/`,
  which Git ignores.
- `analyse.py` refuses to write its report inside the repository unless the
  path is under `out/`. Any other drive, such as `D:` or a USB stick, is fine.
- Nothing in this folder is participant data. The self-test makes its fixture
  in a temporary folder and deletes it.

### Before anyone is run

1. Make the stimuli. The capture step writes them to `out/study/stimuli/` with
   a `manifest.json` (format below).
2. **Write `regions.json` beside `manifest.json`.** It holds the seats and the
   team-ID crops as well as the mistakes and non-standable objects. Without
   seats, find-the-player clicks cannot be scored; without crops, the team
   test is skipped. Either way the minimum study (paired choice, find the bug
   and team ID) cannot complete. The capture script rewrites `manifest.json`
   on every recapture but never touches `regions.json`.
3. Open the page, pick the folder and read the setup screen's warnings. A
   region whose `where` matches no image is listed there.
4. Fix the pass rules, the glitch word list and the primary comparison in
   section 8 and in `analyse.py`. None of them changes once the first person
   has been run.

### Running a session

1. Open `scripts/studies/study-b/index.html` from disk in Chrome or Edge, by
   double-clicking it. No server is needed.
2. On the setup screen, pick the stimulus folder, the one that holds
   `manifest.json`. The page reads `regions.json` from the same folder. A
   regions file kept elsewhere can be picked separately. The page lists how
   many images, seats and crops it loaded, and warns about any listed file
   that is missing; those are skipped.
3. Enter the **participant number**: 1 for the first person, 2 for the next,
   and so on, with no gaps. The number sets the test order, row
   `(number - 1) mod 4` of the Latin square, and the variant rotation, which
   is `(number - 1) mod` the number of variants. The setup screen shows both.
   After **Next participant** the page fills in the next number.
4. Keep or change the anonymous participant ID. The ID and number together
   seed the remaining random choices, so the same pair always gets the same
   trials.
5. Check the **Display** section. Frames are shown only at 1:1 or a
   whole-number multiple, in device pixels, and are never shrunk. **Start
   stays disabled while an included resolution does not fit the window.**
   Use full screen (F11), a larger display, or untick that resolution.
   1280x720 needs a window of about 1630x800 CSS pixels at 100% scaling. 1920x1080 does
   not fit a 1920x1080 display, because the controls need a column beside
   the picture.
6. Press **Hand over to the participant**. The participant reads the consent
   screen and agrees or declines. Declining records nothing.
7. At the end, press **Export results (JSON)**. **Save results so far**, in the
   header, exports a partial file at any time. The page asks before closing
   with unexported results.

Everything is done with the mouse; no keyboard is needed except for the
optional typed comments.

**Why a folder picker.** Under `file://`, Chrome refuses `fetch()` of local
files, and a pasted path cannot be read at all. The page uses
`<input type="file" webkitdirectory>` and shows each image through an object
URL, which works from disk.

### What a participant sees

Every trial shows the picture on the left and the prompt and controls in a
column on the right.

1. **Consent.**
2. **Practice:** click a red circle on a generated picture. Recorded, but not
   scored.
3. **First look:** one picture per variant, in random order, all in the same
   context where possible. The participant writes what they notice. This comes
   before any test is explained, so these comments are the free text the
   glitch rule counts.
4. The four tests, in the order given by the participant number's row of a
   balanced 4x4 Latin square.
   - **Paired choice:** V0 and one variant of the same frame, shown one at a
     time as picture A and picture B. Which is A is random. The participant
     switches between them with two buttons, then picks the one that "looks
     drawn by a person". The pick buttons stay disabled until both pictures
     have been seen. There is an optional "Why?" box. Two full frames side by side
     would not fit at 1:1.
   - **Find the bug:** told the frame may contain a rendering bug, the
     participant clicks it or presses **No bug**. Clicks are normalised to the
     image, 0 to 1 from the top left.
   - **Where could you stand?** Click every thing you could stand on, then
     **Done**. Every click is recorded.
   - **Find the player:** a 4-player frame with identity cues off and a prompt
     such as "Click on Player 3 (gold team)". The first click and its time
     count; the timer starts when the image has loaded.
   - **Which team?** follows find-the-player when the regions file has crops.
     The crop is scaled up by whole device pixels, and the two team names are
     buttons in random order.
5. A progress bar shows trials done out of the total.

By default each participant sees, per variant, up to 8 paired trials, 2
find-the-bug, 2 standable, 3 find-the-player and 6 team-ID crops. The setup
screen can change these limits.

### Matched frames

Find the bug, standable, find the player and team ID compare a variant with
V0, so both must be judged on the same frames.

- The page uses only contexts that exist in every variant. A context is
  resolution, players, palette, shot and `identity_cues` together.
- The contexts for each test are put in one fixed order per stimulus set, the
  same for every participant. Each find context is given one fixed seat, and
  each team unit is one crop in one context.
- Participant *n* sees each chosen context once, in variant
  `(i + n - 1) mod K`, where *i* is the context's place in the fixed order.
  So nobody sees the same frame twice in one test, which would let them
  remember where the player or the bug was.
- Over every K consecutive participant numbers, every context is shown once in
  every variant. With three variants, run participants in multiples of three
  where possible.
- `analyse.py` compares find times and team accuracy only over contexts and
  crops shown in both the variant and V0.

### Manifest format

`capture-study-b.ps1` writes it. The page reads only `images`:

```json
{
  "study": "B", "seed": 7,
  "images": [
    {"variant": "V1", "resolution": "1280x720", "players": 4,
     "palette": "purple_gold", "stage": "crossfeed", "shot": "crossfeed-heavy",
     "frame": "heavy", "identity_cues": false,
     "path": "V1/1280x720/4p/purple_gold/crossfeed-heavy-cues-off.png"}
  ]
}
```

- `path` is relative to the folder that holds `manifest.json`, and has the
  form `<variant>/<res>/<n>p/<palette>/<stage>-<frame>-cues-<on|off>.png`.
  `shot` is `<stage>-<frame>`, such as `crossfeed-clean` or `crossfeed-heavy`.
- `variant` and `path` are required. `V0` is the baseline; any other variant
  name is compared with it.
- Find-the-player uses images with `players: 4` and `identity_cues: false`.
- Seats and crops in the manifest are ignored, with a warning. They live in
  `regions.json`, so a recapture cannot erase them.

### Results format

One file per participant, `study-b-<ID>-<date>.json`, with `format`
`colourwars-study-b-results` and `version` 2. It holds:

- the participant ID and `participant_number`;
- the test `order`, `order_row`, `variants` and `rotation`;
- the trial limits, consent, start and finish times and `complete`;
- the browser and the viewport, with its device pixel ratio;
- `trials`.

Every trial has `test`, `index`, `rt_ms`, `display` and the image's manifest
fields plus `context`. `display` records how the picture was shown: `scale`
(device pixels per image pixel), `css_w`, `css_h`, `dpr`, `natural`,
`window`, and `fits`, which is false if the frame was larger than the window.
Then, by test:

| `test` | Fields |
|---|---|
| `practice` | `click`, `hit` |
| `impressions` | `text` |
| `paired` | `baseline_path`, `first` (the variant shown as picture A), `chosen`, `chose_variant`, `switches`, `changes`, `why` |
| `bug` | `no_bug`, `click` (`null` when `no_bug`) |
| `stand` | `clicks`, each with `x`, `y` and `t_ms` |
| `find` | `seat`, `seat_team`, `seat_rect`, `prompt`, `click` |
| `team` | `crop_id`, `crop`, `truth`, `options`, `answer`, `correct` |

### Regions file

The experimenter writes this once per stimulus set, after capture and before
any participant is run, as `regions.json` beside `manifest.json`. It says
where things are, so clicks can be scored and seats and crops can be shown.
The same file is passed to `analyse.py`.

```json
{
  "regions": [
    {"id": "brick_slip_3", "kind": "mistake", "layer": "gameplay",
     "where": {"variant": "V1", "resolution": "1280x720", "players": 4, "shot": "crossfeed-clean"},
     "rect": [0.50, 0.80, 0.56, 0.84]},
    {"id": "cloud_big_bump", "kind": "mistake", "layer": "backdrop",
     "where": {"variants": ["V1", "V1b"], "path": "V1/1280x720/4p/purple_gold/crossfeed-clean-cues-on.png"},
     "rect": [0.10, 0.08, 0.18, 0.15]},
    {"id": "tree_left", "kind": "not_standable",
     "where": {"resolution": "1280x720", "players": 4},
     "rect": [0.70, 0.40, 0.76, 0.62]},
    {"kind": "seat", "seat": 2, "side": 0,
     "where": {"resolution": "1280x720", "players": 4, "shot": "crossfeed-heavy", "identity_cues": false},
     "rect": [0.62, 0.30, 0.66, 0.40]},
    {"id": "seat3_in_own_paint", "kind": "crop", "side": 1,
     "where": {"resolution": "1280x720", "players": 4, "shot": "crossfeed-heavy", "identity_cues": true},
     "rect": [0.41, 0.52, 0.47, 0.66]}
  ]
}
```

- `kind` is one of:
  - `mistake`, which needs `layer`: `gameplay` or `backdrop`;
  - `not_standable`;
  - `seat`, which needs an integer `seat`;
  - `crop`, which needs an `id`.
- A seat or crop names its team with `team`, as in `"team": "gold"`, or with
  `side`. `side` is 0 or 1, the first or second half of the palette name.
  For `purple_gold`, side 0 is `purple`. One seat entry with `side` then
  serves all six palettes, because every palette is the same simulation.
- A crop may give its own `options` list. Otherwise the answer options are
  the two halves of the palette name.
- A seat or crop must hold in every variant, so its `where` may not use
  `variant`, `variants` or `path`. The page ignores such an entry, with a
  warning, and `analyse.py` refuses it.
- `rect` is `[x0, y0, x1, y1]`, normalised to the image, origin top left. A
  click on the edge counts as inside. Draw rectangles generously; a near miss
  on a mistake is still a click on it.
- `where` limits a region to the images that match every key it gives:
  - `variant`, or `variants` as a list;
  - `path`, `resolution`, `players` or `palette`;
  - `stage`, `shot`, `frame` or `identity_cues`.

  With no `where`, a region applies to every image. Mistakes move with the
  camera, so most mistake regions need `path` or a full context.
- Both the page and `analyse.py` warn about any region whose `where` matches
  nothing. A mistyped `shot` or path would otherwise score nothing in silence.
- Give a `not_standable` object the **same `id` in every variant, V0 included**.
  The standable rule compares variants by id.

### Analysis

```
python scripts/studies/study-b/analyse.py D:\study-b\results\ --regions D:\study-b\regions.json --report D:\study-b\report.md
```

- Results can be files, globs or folders of `*.json`. Files without consent are
  excluded. A file that is not valid JSON, such as a truncated download, is
  skipped with a warning. A participant who appears twice keeps the most
  complete file.
- `--primary V1` names the primary comparison, chosen in advance. V1 is the
  default.
- `--json PATH` also writes the verdicts, the warnings and the glitch pattern
  as JSON.
- The glitch word list is the constant `GLITCH_PATTERN` in `analyse.py`. It is
  not an option, because it is a pass rule. The report and the JSON print it.

The script prints a table of PASS, FAIL or NO DATA per rule and variant, and
the reason for every rule that did not pass. The report adds per-region and
per-seat detail, and lists every flagged comment so it can be traced to a
cause. A variant is PASS only if every rule passes; any NO DATA makes it
INCOMPLETE, which is not a pass.

It also warns about the run itself:

- participant numbers that are missing or used twice;
- order rows or variant rotations whose counts differ by more than one;
- regions that matched no trial;
- trials where the frame did not fit the window.

How each rule from section 8 is applied:

| Rule | As implemented |
|---|---|
| Drawn by a person | Variant chosen in at least 65% of paired trials, and the Wilson 95% lower bound above 50%. The report also gives how many participants chose the variant in most of their trials, chose V0 in most, or tied. |
| Find the bug | Any click inside a `gameplay` mistake fails. A `backdrop` mistake fails if the share of participants who clicked it, out of those who saw it, is above 10%. |
| Standable confusions | Only objects V0 also showed are compared. One fails if any participant clicked it in the variant while no participant clicked it in V0. An object V0 already confuses is not new. An object V0 never showed is listed as not compared. If no object can be compared, the rule is NO DATA. |
| Team ID | Accuracy per palette and per palette-and-team cell, over crops shown in both the variant and V0. Any cell below V0's fails. A cell with no V0 trials is NO DATA. The trial count is reported. |
| Find time | Per seat, over contexts shown in both the variant and V0. Accuracy is the share of clicks inside the seat rectangle. Median and nearest-rank 90th percentile are of correct clicks only. Lower accuracy than V0, or either time above V0's, fails. A seat with no rectangle is NO DATA, because a wrong click cannot be told from a right one. |
| Glitch comments | Participants whose first-look comment matches the glitch words. Fails if the variant's share is above V0's share plus one participant. |

**Strict by design, so expect noise to fail them.** "No drop" in team ID and
"no worse" find times are applied literally, as section 8 states them. With
8-12 people, chance alone will often put a variant a few milliseconds or one
answer behind V0. A FAIL on those two rules is a cue to look at the per-seat
and per-cell tables, not a measurement. Any change to that reading belongs in
section 8, before the study runs.

### Verification

`python scripts/studies/study-b/selftest.py` builds a fixture in a temporary
folder: 109 manifest entries under the capture script's real paths (108 tiny
PNGs and one deliberately missing file), shaped like the capture manifest,
with a `regions.json` beside it. It extracts the page's script and runs
`node --check` on it. It then plans trials for twelve scripted participants,
numbered 1 to 12, with the page's own code. Their answers become results
through the page's own `recordFor`. It checks that:

- plans are deterministic per ID and number;
- the order row is `(number - 1) mod 4`, and twelve numbers fill each row
  three times;
- in find the bug, standable, find the player and team ID, V0 and each variant
  are shown at the same contexts, seats and crops. Each participant sees each
  once, and each cycle of three participants shows each one in every variant;
- which picture comes first in paired choice is near half and half;
- a missing file is skipped, and a region matching no image is reported;
- without a regions file, the team test is skipped with a note and find
  clicks are flagged as unscorable;
- the display scale is a whole number of device pixels, and a frame that is
  too big is not shrunk. No CSS rule squeezes a frame;
- the practice screen does not claim to be unrecorded;
- the Wilson interval matches a published value.

Then it runs `analyse.py` and checks that:

- V1 passes all six rules and V1b fails all six;
- without a regions file, V1 is INCOMPLETE;
- a report path inside the repository is refused, and one on another drive
  raises no error;
- two unreadable results files are skipped with warnings;
- uneven order rows and a region matching no trial are reported;
- the glitch pattern is in the report and the JSON, and `--glitch-pattern` is
  refused.

Hand-made trials check the standable rule when V0 never showed an object, and
the find rule with fast wrong clicks, unmatched contexts and no seat
rectangles. `--keep DIR` copies the fixture, results and reports to `DIR` for
inspection.

On 2026-11-01 the self-test passed. The page was also driven end to end in
headless Chrome from `file://` at a 1920x1200 window, through the DevTools
protocol, as participant 3:

- the folder was picked, and 108 images, 4 seats and 2 crops were loaded;
- the missing file and the stale region were reported;
- the order row and rotation were shown;
- all seven blocks were completed and the results exported;
- every recorded `display` had a whole-number scale and `fits: true`;
- the export was analysed, and no page errors were reported.

That driver is not kept in the repository. The page has not yet been run on
real stimuli, a high-DPI display or a window too small for the frames.
