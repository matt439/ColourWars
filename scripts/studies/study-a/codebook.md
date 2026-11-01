# Study A: codebook

Each code names one kind of drawing mistake. A drawing can carry several codes,
and one mistake can carry two (an eye drawn a cell higher is both PLACE and
ASYM). Codes are scored on the drawing **reduced to cells**: Z drawings are
already one pixel a cell, and `analyse.py` reduces F drawings with any-ink for
black and majority vote for fills. Coders look at the original PNG, the
reduced `cells\` image and the `overlay\` image, where black is ink in both,
blue is ink the reference lacks, red is reference ink the drawing lacks, and
orange is a stray.

Automatic codes are measured by `analyse.py` after the drawing is aligned to
the reference (best overlap over every translation for Z, and over block size
and origin for F). A code with nothing to measure on an item is "not
computable" for that item's drawings, not 0, and those drawings stay out of its
shares: ASYM on the brush pot, PLACE on the brush pot (it has no feature
windows), and UNDER on the bricks (the reference has no stroke ends).

Manual codes are entered by two coders in `coding-<name>.csv`: `present` 0 or
1, `count`, `size_cells`, and `survives` (0 if the mistake shows in the
original F drawing but not in its reduced cells). **Every row needs `present`
0 or 1.** A blank row drops out of kappa and of the counts; `report.md` lists
each coder's blank or missing rows, and they should be 0.
`coding-resolved.csv` settles disagreements row by row: it needs only the rows
it settles, and every other row keeps the coders' agreement.

"Size" is always in cells. The report gives the median size of each code.

## Fixed thresholds

These are set before any drawing is seen (constants at the top of
`analyse.py`):

| Rule | Value |
|---|---|
| A drawing is unaligned and gets no automatic codes | drawn-cell IoU below 0.25 |
| Stray | a component of at most 2 cells |
| Asym | at least 2 new asymmetric pairs, on the figure and the bricks only |
| Over or under | a tail or shortfall of at most 4 cells |
| Place | a centroid offset of at least 1 cell |
| Prop | a relative aspect error of at least 0.05 |
| Candidate | in at least 30% of the C drawings, from at least 3 people among those C drawings; or the same within the T drawings; and survives reduction |
| Coder agreement | Cohen's kappa of at least 0.6 |

## Automatic codes

**ASYM: asymmetry.**
- Definition: the drawing is less mirror-symmetric than the reference.
- How counted: pairs of cells mirrored about the reference's centre column
  whose classes (paper, fill, ink) differ. The score is the drawing's pairs
  minus the reference's own (the brim, smile and hands are already
  asymmetric). Strays and over/under tails are left out. At least 2 new pairs
  count; one pair is a single cell and is left to WOB and WEIGHT. One per
  drawing; size is the net number of new pairs.
- Example: the left eye drawn one cell higher gives 2 new pairs.
- Only items whose reference is nearly mirror-symmetric are scored: the figure
  (11 pairs of its own) and the bricks (none). The brush pot has 576, so a net
  count against it is noise, and ASYM is not computable there. `items.json`
  records each item's choice and its own pair count.

**GAP: open outline.**
- Definition: an outline that should be closed is not, so Paint's bucket fill
  would leak out.
- How counted: a flood from the canvas edge through non-ink cells, 4-connected
  like the bucket, reaches more than half of a region the reference encloses.
  One per opening; size is the cells in the opening.
- Example: the top of the cap missing one cell gives GAP 1, size 1.

**OVER: overshoot.**
- Definition: a stroke runs past where it should end.
- How counted: a stroke end (an ink cell with one ink neighbour) off the
  reference ink, traced back along the stroke to the reference ink in at most
  4 cells. Size is the tail's length.
- Example: the brim drawn one cell longer gives OVER 1, size 1.

**UNDER: undershoot.**
- Definition: a stroke stops short of where it should end.
- How counted: a stroke end on the reference ink, next to at most 4 missing
  reference cells that run to a reference stroke end and do not join other
  ink. Size is the missing run. A break inside a closed outline is GAP, not
  UNDER. Not computable on an item whose reference has no stroke ends (the
  bricks).
- Example: the brim drawn one cell shorter gives UNDER 1, size 1.

**PROP: proportion.**
- Definition: the drawing is taller or wider for its size than the reference.
- How counted: the bounding box of the drawing (strays and overshoots left
  out) against the reference's. It counts when the relative aspect error,
  |(height / width) / (reference height / width) - 1|, is at least 0.05, the
  same share on every item whatever its size. Size is how many cells of width
  the error is worth, |width - height / reference aspect|, rounded, at least 1.
  One per drawing.
- Example: the person two rows taller (31 rows to 29) is an error of 0.07 and
  gives PROP, size 1.

**PLACE: placement.**
- Definition: a feature sits in the wrong place inside its shape.
- How counted: the centre of the ink inside a fixed window for each feature
  (person: left eye, right eye, mouth; bricks: each joint, within its course)
  against the reference's. One per feature moved; size is the larger offset,
  rounded. A feature with no ink at all is not PLACE: code it OMIT. Not
  computable on the brush pot, which has no feature windows.
  `make-references.py` checks the person's windows against the face and stops
  if an eye window no longer holds one column of ink or the mouth window holds
  none.
- Example: one brick joint a cell to the right gives PLACE 1, size 1.

**STRAY: stray mark.**
- Definition: a dot or tiny mark that belongs to nothing.
- How counted: a connected group of at most 2 drawn cells with no cell within
  one cell of the reference drawing. One per group; size is its cells. Strays
  are removed before the other automatic codes.
- Example: a single pixel clicked by accident beside the cap gives STRAY 1,
  size 1.

## Manual codes

**WOB: wobble.**
- Definition: a line that should be straight wanders off it by at least one
  cell and comes back: a kink, step or wave. A whole line drawn one cell over
  is PLACE or PROP, not WOB.
- How counted: one per line that wobbles; size is the largest sideways
  departure.
- Example: the left side of the head steps out one cell for two rows.

**WEIGHT: line weight.**
- Definition: a keyline thicker or thinner than the reference's; most keylines
  are one cell and the ears are two.
- How counted: one per stretch; size is the stretch's length.
- Example: the chin drawn two cells thick for its whole width, size 9.

**FILL: fill error.**
- Definition: a coloured area wrong in its filling: a bucket fill that escaped,
  an area left unfilled, specks the fill missed, or the wrong colour.
- How counted: one per area; size is the cells affected.
- Example: the cap left white, size 14.

**OMIT and ADD: missing or extra parts.** Two codes, entered separately.
- Definition: OMIT is a reference part left out (an ear, an eye, a strap, the
  neck, a joint). ADD is a part the reference does not have (a nose, fingers,
  an extra joint).
- How counted: one per part; size is the part's cells, or the cells it should
  have had.
- Example: no ears is OMIT 2, size 2 each.

**GRID: off the grid.**
- Definition: F only. A mark that falls across the cell grid, so reduction
  changes it: a one-pixel line that becomes two cells thick, a dot split over
  two cells, or an edge moved by one. Always 0 on Z drawings.
- How counted: one per mark; size is the cells it gained or lost.
- Example: the right side of the body drawn on a cell boundary, reduced to a
  two-cell line.

## Survives reduction

A mistake survives if it can still be seen in the reduced cells. The game draws
in whole cells, so a mistake that only exists between cells cannot be used.
Automatic codes are measured on cells and always survive. For a manual code,
mark `survives` 0 when the mistake shows in the original F PNG but not in
`cells\`.
