#!/usr/bin/env python3
"""Score Study A drawings against the references and write a Markdown report.

Needs Python 3.9+ and Pillow, installed outside the repository (for example
`python -m pip install --user pillow`, or a venv outside the checkout). The
repository itself never depends on Pillow.

Usage:
  python analyse.py --references REFS --drawings DRAWINGS [--coding CODING] [--out OUT]

REFS is the folder make-references.py wrote (it reads items.json there).
DRAWINGS holds the participants' PNGs, named pid_task_condition_item.png:
  pid        letters and digits, e.g. p03
  task       D (from description), C (copy) or T (trace)
  condition  Z (cells at 400%) or F (100%, reduced to cells here)
  item       figure, bricks or brushpot
CODING holds coding-<coder>.csv files filled from coding-template.csv, and
optionally coding-resolved.csv. OUT defaults to a folder beside DRAWINGS.
Nothing is written inside the repository except under out/.

The method is section 8 of docs/drawing-mistakes-research.md; the codes are
defined in codebook.md beside this script.
"""

import argparse
import csv
import json
import math
import os
import re
import statistics
import sys

# The study tools are also runnable directly from their own folders.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from _paths import inside_repo_but_not_out

try:
    from PIL import Image
except ImportError:
    sys.exit("analyse.py needs Pillow, which this repository does not ship.\n"
             "Install it outside the repository, for example:\n"
             "  python -m pip install --user pillow\n"
             "or create a venv outside the checkout and run analyse.py with its python.")

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))

PAPER, FILL, INK = 0, 1, 2
AUTO_CODES = ["ASYM", "GAP", "OVER", "UNDER", "PROP", "PLACE", "STRAY"]
MANUAL_CODES = ["WOB", "WEIGHT", "FILL", "OMIT", "ADD", "GRID"]
ALL_CODES = ["ASYM", "GAP", "OVER", "UNDER", "WOB", "WEIGHT", "PROP", "PLACE", "STRAY",
             "FILL", "OMIT", "ADD", "GRID"]
NAME = re.compile(r"^([A-Za-z0-9]+)_([DCT])_([ZF])_([a-z]+)\.png$")

# Fixed before any drawing is scored (codebook.md).
MIN_DRAWN_IOU = 0.25      # below this a drawing is "unaligned" and gets no automatic codes
STRAY_MAX_CELLS = 2       # a component this small, away from the reference, is a stray
TAIL_MAX_CELLS = 4        # longer tails or shortfalls are left to OMIT/ADD
PLACE_MIN_CELLS = 1.0     # feature centroid offset that counts as misplaced
PROP_MIN_ERROR = 0.05     # relative aspect error, |(h/w) / (reference h/w) - 1|, that counts
ASYM_MIN_PAIRS = 2        # net new asymmetric pairs that count (items that opt in only)
CANDIDATE_SHARE = 0.30
CANDIDATE_PEOPLE = 3      # counted in the same task pool (C or T) that reaches the share
MARGIN_CELLS = 4          # paper added round the reference, so a drawing larger than it is not cropped
F_SCALE_RANGE = (0.5, 1.6)  # F block sizes searched, as multiples of the item's nominal px a cell
F_REFINE = 5              # best coarse F fits re-scored exactly
MANIFEST_VERSION = 2      # items.json version this script reads (make-references.py writes it)



# --------------------------------------------------------------------------
# Pixels and cells
# --------------------------------------------------------------------------

def classify(r, g, b, a):
    """Ink is near black, paper is white or near white (including the trace
    underlay), anything else is fill."""
    if a < 128:
        return PAPER
    if max(r, g, b) <= 64:
        return INK
    if min(r, g, b) >= 208:
        return PAPER
    return FILL


def load_classes(path):
    image = Image.open(path).convert("RGBA")
    width, height = image.size
    data = image.tobytes()
    grid = []
    for y in range(height):
        row = []
        base = y * width * 4
        for x in range(width):
            i = base + x * 4
            row.append(classify(data[i], data[i + 1], data[i + 2], data[i + 3]))
        grid.append(row)
    return grid


def reference_grid(item):
    """The reference's classes with MARGIN_CELLS of paper all round. Every
    drawing is aligned into this frame; the mirror axis stays central."""
    table = {"K": INK, "T": FILL, "L": FILL, "W": PAPER, ".": PAPER}
    m = MARGIN_CELLS
    width = len(item["rows"][0]) + 2 * m
    blank = [[PAPER] * width for _ in range(m)]
    body = [[PAPER] * m + [table[c] for c in row] + [PAPER] * m for row in item["rows"]]
    return blank + body + [row[:] for row in blank]


def features_of(item):
    m = MARGIN_CELLS
    return [{"name": f["name"], "window": [f["window"][0] + m, f["window"][1] + m,
                                           f["window"][2] + m, f["window"][3] + m]}
            for f in item["features"]]


def dims(grid):
    return len(grid[0]), len(grid)


def neighbours8(x, y, w, h):
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dx or dy:
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    yield nx, ny


def neighbours4(x, y, w, h):
    for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
        if 0 <= nx < w and 0 <= ny < h:
            yield nx, ny


def components(cells, w, h, eight=True):
    """Connected components of a set of (x, y) cells, in a stable order."""
    seen, out = set(), []
    for start in sorted(cells, key=lambda c: (c[1], c[0])):
        if start in seen:
            continue
        seen.add(start)
        stack, comp = [start], []
        while stack:
            c = stack.pop()
            comp.append(c)
            near = neighbours8(*c, w, h) if eight else neighbours4(*c, w, h)
            for n in near:
                if n in cells and n not in seen:
                    seen.add(n)
                    stack.append(n)
        out.append(sorted(comp, key=lambda c: (c[1], c[0])))
    return out


def cells_of(grid, classes):
    return {(x, y) for y, row in enumerate(grid) for x, v in enumerate(row) if v in classes}


def bbox(cells):
    xs = [c[0] for c in cells]
    ys = [c[1] for c in cells]
    return min(xs), min(ys), max(xs), max(ys)


def main_bbox(grid, cell_px=1):
    """Bounding box of everything drawn except components no larger than a
    stray (at most STRAY_MAX_CELLS cells across each way, at cell_px pixels a
    cell), so strays do not move it but a figure in several pieces is whole."""
    w, h = dims(grid)
    drawn = cells_of(grid, (INK, FILL))
    if not drawn:
        return None
    limit = STRAY_MAX_CELLS * cell_px
    kept = []
    for comp in components(drawn, w, h):
        x0, y0, x1, y1 = bbox(comp)
        if x1 - x0 + 1 > limit or y1 - y0 + 1 > limit:
            kept += comp
    return bbox(kept or drawn)


def ones(value):
    return bin(value).count("1")


def mask_bits(image):
    """A mode '1' image as one integer, for fast overlap counts."""
    return int.from_bytes(image.tobytes(), "big")


ANY = [0] + [255] * 255           # any ink in the cell
MAJORITY = [0] * 128 + [255] * 128  # more than half the cell


def mask_image(grid, classes, pad=0, mode="1"):
    """The cells of grid in classes as a 0/255 image, with pad cells of
    paper all round. Mode '1' for crops, 'L' for box averaging."""
    w, h = dims(grid)
    inner = Image.new("L", (w, h), 0)
    inner.putdata([255 if v in classes else 0 for row in grid for v in row])
    image = Image.new("L", (w + 2 * pad, h + 2 * pad), 0)
    image.paste(inner, (pad, pad))
    return image.point(ANY, "1") if mode == "1" else image


def score_bits(ink, drawn, ref_ink, ref_drawn):
    """score() on bit masks of the same frame: ink IoU + 0.5 drawn IoU."""
    def bit_iou(a, b):
        union = ones(a | b)
        return ones(a & b) / union if union else 1.0
    return bit_iou(ink, ref_ink) + 0.5 * bit_iou(drawn, ref_drawn)


def iou(a, b):
    union = len(a | b)
    return len(a & b) / union if union else 1.0


def score(aligned, ref_ink, ref_drawn):
    ink = cells_of(aligned, (INK,))
    drawn = ink | cells_of(aligned, (FILL,))
    return iou(ink, ref_ink) + 0.5 * iou(drawn, ref_drawn)


# --------------------------------------------------------------------------
# Alignment: Z by translation, F by block size and origin
# --------------------------------------------------------------------------

def align_z(grid, ref):
    """Every translation that leaves any of the drawing in the reference's
    frame is tried; ties go to the one nearest the centred guess."""
    rw, rh = dims(ref)
    pw, ph = dims(grid)
    pb, rb = main_bbox(grid), main_bbox(ref)
    if pb is None:
        return [[PAPER] * rw for _ in range(rh)], {"offset": None}
    bx = round((pb[0] + pb[2]) / 2 - (rb[0] + rb[2]) / 2)
    by = round((pb[1] + pb[3]) / 2 - (rb[1] + rb[3]) / 2)
    ref_ink = mask_bits(mask_image(ref, (INK,)))
    ref_drawn = mask_bits(mask_image(ref, (INK, FILL)))
    ink_image = mask_image(grid, (INK,), pad=max(rw, rh))
    drawn_image = mask_image(grid, (INK, FILL), pad=max(rw, rh))
    pad = max(rw, rh)
    best = None
    for oy in range(-(rh - 1), ph):
        for ox in range(-(rw - 1), pw):
            box = (ox + pad, oy + pad, ox + pad + rw, oy + pad + rh)
            key = (score_bits(mask_bits(ink_image.crop(box)), mask_bits(drawn_image.crop(box)),
                              ref_ink, ref_drawn),
                   -(abs(ox - bx) + abs(oy - by)))
            if best is None or key > best[0]:
                best = (key, (ox, oy))
    ox, oy = best[1]
    aligned = [[grid[y + oy][x + ox] if 0 <= y + oy < ph and 0 <= x + ox < pw else PAPER
                for x in range(rw)] for y in range(rh)]
    # Reported as where the reference's top-left cell sits in the drawing.
    return aligned, {"offset": [ox + MARGIN_CELLS, oy + MARGIN_CELLS]}


class Integral:
    def __init__(self, grid, value):
        w, h = dims(grid)
        self.w, self.h = w, h
        s = [[0] * (w + 1) for _ in range(h + 1)]
        for y in range(h):
            run = 0
            row, above, cur = grid[y], s[y], s[y + 1]
            for x in range(w):
                run += row[x] == value
                cur[x + 1] = above[x + 1] + run
        self.s = s

    def count(self, x0, y0, x1, y1):
        x0, y0 = max(0, min(self.w, x0)), max(0, min(self.h, y0))
        x1, y1 = max(0, min(self.w, x1)), max(0, min(self.h, y1))
        if x1 <= x0 or y1 <= y0:
            return 0
        s = self.s
        return s[y1][x1] - s[y0][x1] - s[y1][x0] + s[y0][x0]


def reduce_f(ink_sum, fill_sum, rw, rh, block, ox, oy):
    """Any-ink for black, majority for fills (section 8)."""
    xs = [round(ox + i * block) for i in range(rw + 1)]
    ys = [round(oy + j * block) for j in range(rh + 1)]
    out = []
    for j in range(rh):
        y0, y1 = ys[j], ys[j + 1]
        row = []
        for i in range(rw):
            x0, x1 = xs[i], xs[i + 1]
            if ink_sum.count(x0, y0, x1, y1):
                row.append(INK)
            elif 2 * fill_sum.count(x0, y0, x1, y1) > (x1 - x0) * (y1 - y0):
                row.append(FILL)
            else:
                row.append(PAPER)
        out.append(row)
    return out


def f_blocks(nominal):
    """Every block size searched for F: quarter pixels across F_SCALE_RANGE of
    the nominal size, whatever the drawing's own bounding box suggests."""
    low, high = nominal * F_SCALE_RANGE[0], nominal * F_SCALE_RANGE[1]
    blocks, b = [], math.ceil(low * 4) / 4
    while b <= high + 1e-9:
        if b >= 2:
            blocks.append(b)
        b += 0.25
    return blocks or [float(nominal)]


def align_f(grid, ref, nominal):
    """One uniform block size (so a proportion error stays visible) and an
    origin. A coarse pass box-averages every block size in f_blocks() at every
    pixel origin within a cell and a pixel of the centred guess; the best
    F_REFINE are then reduced exactly by reduce_f() at their origin and the
    eight pixels round it."""
    rw, rh = dims(ref)
    ref_ink_cells, ref_drawn_cells = cells_of(ref, (INK,)), cells_of(ref, (INK, FILL))
    pb, rb = main_bbox(grid, nominal), main_bbox(ref)
    if pb is None:
        return [[PAPER] * rw for _ in range(rh)], {"block": None, "origin": None}
    blocks = f_blocks(nominal)
    ref_ink = mask_bits(mask_image(ref, (INK,)))
    ref_drawn = mask_bits(mask_image(ref, (INK, FILL)))
    pad = math.ceil(max(rw, rh) * blocks[-1]) + 2
    ink_image = mask_image(grid, (INK,), pad=pad, mode="L")
    fill_image = mask_image(grid, (FILL,), pad=pad, mode="L")
    # Centres, in pixels and in cells, of everything drawn bar the strays.
    pcx, pcy = (pb[0] + pb[2] + 1) / 2, (pb[1] + pb[3] + 1) / 2
    rcx, rcy = (rb[0] + rb[2] + 1) / 2, (rb[1] + rb[3] + 1) / 2

    coarse = []
    for block in blocks:
        gx, gy = pcx - rcx * block, pcy - rcy * block
        reach = int(block) + 1
        for oy in range(round(gy) - reach, round(gy) + reach + 1):
            for ox in range(round(gx) - reach, round(gx) + reach + 1):
                box = (ox + pad, oy + pad, ox + pad + rw * block, oy + pad + rh * block)
                ink = mask_bits(ink_image.resize((rw, rh), Image.BOX, box=box).point(ANY, "1"))
                fill = mask_bits(fill_image.resize((rw, rh), Image.BOX, box=box).point(MAJORITY, "1"))
                key = (score_bits(ink, ink | fill, ref_ink, ref_drawn), -abs(block - nominal),
                       -(abs(ox - gx) + abs(oy - gy)))
                coarse.append((key, block, ox, oy))
    coarse.sort(key=lambda c: c[0], reverse=True)

    ink_sum, fill_sum = Integral(grid, INK), Integral(grid, FILL)
    best, tried = None, set()
    for _, block, cx, cy in coarse[:F_REFINE]:
        gx, gy = pcx - rcx * block, pcy - rcy * block
        for oy in (cy - 1, cy, cy + 1):
            for ox in (cx - 1, cx, cx + 1):
                if (block, ox, oy) in tried:
                    continue
                tried.add((block, ox, oy))
                reduced = reduce_f(ink_sum, fill_sum, rw, rh, block, ox, oy)
                key = (score(reduced, ref_ink_cells, ref_drawn_cells), -abs(block - nominal),
                       -(abs(ox - gx) + abs(oy - gy)))
                if best is None or key > best[0]:
                    best = (key, reduced, (ox, oy), block)
    # Reported as the pixel where the reference's top-left cell starts.
    return best[1], {"block": best[3], "origin": [round(best[2][0] + MARGIN_CELLS * best[3]),
                                                  round(best[2][1] + MARGIN_CELLS * best[3])]}


# --------------------------------------------------------------------------
# Automatic codes
# --------------------------------------------------------------------------

def find_strays(aligned, ref):
    w, h = dims(ref)
    ref_drawn = cells_of(ref, (INK, FILL))
    near_ref = {n for c in ref_drawn for n in list(neighbours8(*c, w, h)) + [c]}
    strays = []
    for comp in components(cells_of(aligned, (INK, FILL)), w, h):
        if len(comp) <= STRAY_MAX_CELLS and not any(c in near_ref for c in comp):
            strays.append(comp)
    return strays


def exterior(grid):
    """Non-ink cells reachable from the border, 4-connected like Paint's bucket."""
    w, h = dims(grid)
    seen, stack = set(), []
    for x in range(w):
        for y in (0, h - 1):
            if grid[y][x] != INK and (x, y) not in seen:
                seen.add((x, y)); stack.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            if grid[y][x] != INK and (x, y) not in seen:
                seen.add((x, y)); stack.append((x, y))
    while stack:
        c = stack.pop()
        for n in neighbours4(*c, w, h):
            if n not in seen and grid[n[1]][n[0]] != INK:
                seen.add(n); stack.append(n)
    return seen


def find_gaps(aligned, ref):
    w, h = dims(ref)
    ref_ext, p_ext = exterior(ref), exterior(aligned)
    enclosed = {(x, y) for y in range(h) for x in range(w)
                if ref[y][x] != INK and (x, y) not in ref_ext}
    gaps = []
    for region in components(enclosed, w, h, eight=False):
        open_cells = [c for c in region if aligned[c[1]][c[0]] != INK]
        leaked = [c for c in open_cells if c in p_ext]
        if not open_cells or 2 * len(leaked) <= len(open_cells):
            continue
        leaked = set(leaked)
        region_set = set(region)
        openings = {n for c in leaked for n in neighbours4(*c, w, h)
                    if n not in region_set and n in p_ext and aligned[n[1]][n[0]] != INK}
        sizes = [len(o) for o in components(openings, w, h)] or [1]
        gaps.append({"region_cells": len(region), "openings": sizes})
    return gaps


def ink_neighbours(ink, c, w, h):
    return [n for n in neighbours8(*c, w, h) if n in ink]


def find_tails(aligned, ref):
    """OVER: a stroke end off the reference ink, traced back to it.
    UNDER: a stroke end on the reference ink, short of a reference end."""
    w, h = dims(ref)
    p_ink, r_ink = cells_of(aligned, (INK,)), cells_of(ref, (INK,))
    r_ends = {c for c in r_ink if len(ink_neighbours(r_ink, c, w, h)) == 1}
    overs, unders = [], []
    for e in sorted((c for c in p_ink if len(ink_neighbours(p_ink, c, w, h)) == 1),
                    key=lambda c: (c[1], c[0])):
        if e in r_ends:
            continue
        if e not in r_ink:
            path, cur = [e], e
            while cur not in r_ink and len(path) <= TAIL_MAX_CELLS:
                onward = [n for n in ink_neighbours(p_ink, cur, w, h) if n not in path]
                if len(onward) != 1:
                    break
                cur = onward[0]
                path.append(cur)
            if cur in r_ink and len(path) - 1 <= TAIL_MAX_CELLS:
                overs.append(path[:-1])
            continue
        own = set(ink_neighbours(p_ink, e, w, h)) | {e}
        missing = r_ink - p_ink
        run, frontier = set(), [n for n in neighbours8(*e, w, h) if n in missing]
        run.update(frontier)
        while frontier and len(run) <= TAIL_MAX_CELLS:
            c = frontier.pop()
            for n in neighbours8(*c, w, h):
                if n in missing and n not in run:
                    run.add(n); frontier.append(n)
        if not run or len(run) > TAIL_MAX_CELLS:
            continue
        reconnects = any(n in p_ink and n not in own for c in run for n in neighbours8(*c, w, h))
        if reconnects or not (run & r_ends):
            continue
        unders.append(sorted(run, key=lambda c: (c[1], c[0])))
    return overs, unders


def find_places(aligned, ref, features):
    out = []
    for feature in features:
        x0, y0, x1, y1 = feature["window"]
        window = [(x, y) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1)]
        r = [c for c in window if ref[c[1]][c[0]] == INK]
        p = [c for c in window if aligned[c[1]][c[0]] == INK]
        if not r:
            continue
        if not p:
            out.append({"feature": feature["name"], "missing": True})
            continue
        dx = sum(c[0] for c in p) / len(p) - sum(c[0] for c in r) / len(r)
        dy = sum(c[1] for c in p) / len(p) - sum(c[1] for c in r) / len(r)
        out.append({"feature": feature["name"], "dx": round(dx, 2), "dy": round(dy, 2), "missing": False})
    return out


def asymmetry(grid, skip):
    w, h = dims(grid)
    pairs = set()
    for y in range(h):
        for x in range(w // 2):
            m = w - 1 - x
            if grid[y][x] != grid[y][m] and (x, y) not in skip and (m, y) not in skip:
                pairs.add((x, y))
    return pairs


def auto_codes(aligned, ref, item):
    w, h = dims(ref)
    strays = find_strays(aligned, ref)
    clean = [row[:] for row in aligned]
    for comp in strays:
        for x, y in comp:
            clean[y][x] = PAPER
    gaps = find_gaps(clean, ref)
    overs, unders = find_tails(clean, ref)
    places = find_places(clean, ref, features_of(item))
    tail_cells = {c for t in overs for c in t} | {c for t in unders for c in t}

    # Net new asymmetric pairs: a count, not a set difference, so a drawing
    # whose lower half sits a row lower is not charged twice for the
    # reference's own asymmetric rows. Only items that opt in through
    # items.json (near-symmetric references) are scored; the rest get None.
    new_asym = max(0, len(asymmetry(clean, tail_cells)) - len(asymmetry(ref, set())))
    asym = None
    if item["asym"]:
        hit = new_asym >= ASYM_MIN_PAIRS
        asym = {"count": 1 if hit else 0, "sizes": [new_asym] if hit else []}

    shape = cells_of(clean, (INK, FILL)) - {c for t in overs for c in t}
    shape |= {c for t in unders for c in t}
    rx0, ry0, rx1, ry1 = bbox(cells_of(ref, (INK, FILL)))
    ref_aspect = (ry1 - ry0 + 1) / (rx1 - rx0 + 1)
    prop = None
    prop_hit = False
    if shape:
        px0, py0, px1, py1 = bbox(shape)
        pw, ph = px1 - px0 + 1, py1 - py0 + 1
        error = (ph / pw) / ref_aspect - 1
        prop_hit = abs(error) >= PROP_MIN_ERROR
        prop = {"size": [pw, ph], "reference": [rx1 - rx0 + 1, ry1 - ry0 + 1],
                "aspect_error": round(error, 3),
                "width_cells": round(abs(pw - ph / ref_aspect), 2)}

    moved = [p for p in places if not p["missing"]
             and max(abs(p["dx"]), abs(p["dy"])) >= PLACE_MIN_CELLS]
    # A code with nothing to measure on this item is None (not computable),
    # so it stays out of the denominators: PLACE without feature windows,
    # UNDER without reference stroke ends.
    r_ink = cells_of(ref, (INK,))
    ref_has_ends = any(len(ink_neighbours(r_ink, c, w, h)) == 1 for c in r_ink)
    codes = {
        "ASYM": asym,
        "GAP": {"count": sum(len(g["openings"]) for g in gaps),
                "sizes": [s for g in gaps for s in g["openings"]]},
        "OVER": {"count": len(overs), "sizes": [len(t) for t in overs]},
        "UNDER": {"count": len(unders), "sizes": [len(t) for t in unders]} if ref_has_ends else None,
        "PROP": {"count": 1 if prop_hit else 0,
                 "sizes": [max(1, round(prop["width_cells"]))] if prop_hit else []},
        "PLACE": {"count": len(moved),
                  "sizes": [round(max(abs(p["dx"]), abs(p["dy"]))) for p in moved]} if item["features"] else None,
        "STRAY": {"count": len(strays), "sizes": [len(c) for c in strays]},
    }
    detail = {"gaps": gaps, "overs": overs, "unders": unders, "places": places, "prop": prop,
              "asym_pairs": new_asym,
              "strays": strays}
    return codes, detail, strays


# --------------------------------------------------------------------------
# Images for the coders
# --------------------------------------------------------------------------

def save_cells(path, grid, scale=8):
    w, h = dims(grid)
    colour = {PAPER: (255, 255, 255), FILL: (128, 128, 128), INK: (0, 0, 0)}
    image = Image.new("RGB", (w * scale + 1, h * scale + 1), (150, 170, 210))
    px = image.load()
    for y in range(h):
        for x in range(w):
            c = colour[grid[y][x]]
            for dy in range(1, scale):
                for dx in range(1, scale):
                    px[x * scale + dx, y * scale + dy] = c
    image.save(path)


def save_overlay(path, grid, ref, strays, scale=8):
    w, h = dims(ref)
    stray_cells = {c for comp in strays for c in comp}
    image = Image.new("RGB", (w * scale, h * scale), (255, 255, 255))
    px = image.load()
    for y in range(h):
        for x in range(w):
            p, r = grid[y][x], ref[y][x]
            if (x, y) in stray_cells:
                c = (255, 140, 0)
            elif p == INK and r == INK:
                c = (0, 0, 0)
            elif p == INK:
                c = (40, 80, 230)          # ink the reference does not have
            elif r == INK:
                c = (235, 90, 90)          # reference ink the drawing lacks
            elif p == FILL:
                c = (190, 190, 190)
            elif r == FILL:
                c = (255, 220, 220)        # reference fill left as paper
            else:
                c = (255, 255, 255)
            for dy in range(scale):
                for dx in range(scale):
                    px[x * scale + dx, y * scale + dy] = c
    image.save(path)


# --------------------------------------------------------------------------
# Manual coding and agreement
# --------------------------------------------------------------------------

TEMPLATE_FIELDS = ["drawing", "pid", "task", "condition", "item", "code",
                   "present", "count", "size_cells", "survives", "note"]


def write_template(path, drawings):
    with open(path, "w", encoding="utf-8", newline="") as handle:
        out = csv.writer(handle)
        out.writerow(TEMPLATE_FIELDS)
        for d in drawings:
            for code in MANUAL_CODES:
                out.writerow([d["stem"], d["pid"], d["task"], d["condition"], d["item"], code,
                              "", "", "", "", ""])


def read_coding(path):
    rows = {}
    with open(path, encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            present = (row.get("present") or "").strip()
            if present == "":
                continue
            if present not in ("0", "1"):
                sys.exit(f"{path}: present must be 0 or 1 (drawing {row['drawing']}, code {row['code']})")
            code = row["code"].strip().upper()
            if code not in MANUAL_CODES:
                sys.exit(f"{path}: unknown manual code {code!r}")

            def number(field, default):
                text = (row.get(field) or "").strip()
                return float(text) if text else default
            rows[(row["drawing"].strip(), code)] = {
                "present": present == "1",
                "count": int(number("count", 1 if present == "1" else 0)),
                "size": number("size_cells", None),
                "survives": (row.get("survives") or "1").strip() != "0",
                "note": (row.get("note") or "").strip(),
            }
    return rows


def kappa(pairs):
    """Cohen's kappa for two raters' binary judgements."""
    n = len(pairs)
    if n == 0:
        return None
    agree = sum(a == b for a, b in pairs) / n
    pa = sum(a for a, _ in pairs) / n
    pb = sum(b for _, b in pairs) / n
    expected = pa * pb + (1 - pa) * (1 - pb)
    if expected >= 1:
        return None
    return (agree - expected) / (1 - expected)


# --------------------------------------------------------------------------
# Report
# --------------------------------------------------------------------------

def pct(n, d):
    return f"{100 * n / d:.0f}%" if d else "-"


def fmt(v, digits=2):
    return "-" if v is None else f"{v:.{digits}f}"


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--references", required=True)
    parser.add_argument("--drawings", required=True)
    parser.add_argument("--coding", help="folder of coding-<coder>.csv files (default: DRAWINGS)")
    parser.add_argument("--out", help="output folder (default: DRAWINGS-analysis beside DRAWINGS)")
    args = parser.parse_args(argv)

    drawings_dir = os.path.abspath(args.drawings)
    coding_dir = os.path.abspath(args.coding or drawings_dir)
    out = os.path.abspath(args.out or drawings_dir.rstrip("\\/") + "-analysis")
    if inside_repo_but_not_out(out):
        sys.exit(f"refusing output inside repository source or at an unresolved path: {out}; use out/ or an accessible path outside the repository")
    manifest = json.load(open(os.path.join(args.references, "items.json"), encoding="utf-8"))
    if manifest.get("version") != MANIFEST_VERSION:
        sys.exit(f"{args.references}: items.json is version {manifest.get('version')}, this script reads "
                 f"version {MANIFEST_VERSION}. Run make-references.py again.")
    items = {item["name"]: item for item in manifest["items"]}
    os.makedirs(os.path.join(out, "cells"), exist_ok=True)
    os.makedirs(os.path.join(out, "overlay"), exist_ok=True)

    drawings, skipped = [], []
    for name in sorted(os.listdir(drawings_dir)):
        if not name.lower().endswith(".png"):
            continue
        match = NAME.match(name)
        if not match or match.group(4) not in items:
            skipped.append(name)
            continue
        pid, task, condition, item_name = match.groups()
        item = items[item_name]
        ref = reference_grid(item)
        grid = load_classes(os.path.join(drawings_dir, name))
        size = list(dims(grid))
        if condition == "Z":
            aligned, fit = align_z(grid, ref)
        else:
            aligned, fit = align_f(grid, ref, item["f_scale"])
        ref_ink, ref_drawn = cells_of(ref, (INK,)), cells_of(ref, (INK, FILL))
        ink_iou = iou(cells_of(aligned, (INK,)), ref_ink)
        drawn_iou = iou(cells_of(aligned, (INK, FILL)), ref_drawn)
        stem = name[:-4]
        record = {"stem": stem, "pid": pid, "task": task, "condition": condition, "item": item_name,
                  "image_size": size, "fit": fit, "ink_iou": round(ink_iou, 3),
                  "drawn_iou": round(drawn_iou, 3)}
        expected = item["z_canvas"] if condition == "Z" else item["f_canvas"]
        record["canvas_ok"] = size == expected
        strays = []
        if drawn_iou < MIN_DRAWN_IOU:
            record["aligned"] = False
            record["codes"] = {code: None for code in AUTO_CODES}
        else:
            record["aligned"] = True
            record["codes"], record["detail"], strays = auto_codes(aligned, ref, item)
        save_cells(os.path.join(out, "cells", stem + ".png"), aligned)
        save_overlay(os.path.join(out, "overlay", stem + ".png"), aligned, ref, strays)
        drawings.append(record)
        print(f"{stem}: ink IoU {ink_iou:.2f}, drawn IoU {drawn_iou:.2f}"
              + ("" if record["aligned"] else " (unaligned)"))

    write_template(os.path.join(out, "coding-template.csv"), drawings)
    with open(os.path.join(out, "drawings.json"), "w", encoding="utf-8", newline="\n") as handle:
        json.dump(drawings, handle, indent=1, default=list)
        handle.write("\n")

    # Manual codes.
    coders = {}
    if os.path.isdir(coding_dir):
        for name in sorted(os.listdir(coding_dir)):
            m = re.match(r"^coding-(.+)\.csv$", name)
            if m and m.group(1) not in ("template", "resolved"):
                coders[m.group(1)] = read_coding(os.path.join(coding_dir, name))
    resolved_path = os.path.join(coding_dir, "coding-resolved.csv")
    resolved = read_coding(resolved_path) if os.path.isfile(resolved_path) else None
    stems = [d["stem"] for d in drawings]
    names = sorted(coders)
    agreement, disagreements = [], []
    if len(names) >= 2:
        a, b = coders[names[0]], coders[names[1]]
        pooled = []
        for code in MANUAL_CODES:
            pairs = [(a[(s, code)]["present"], b[(s, code)]["present"])
                     for s in stems if (s, code) in a and (s, code) in b]
            pooled += pairs
            agreement.append((code, len(pairs), kappa(pairs),
                              sum(x == y for x, y in pairs) / len(pairs) if pairs else None))
            for s in stems:
                if (s, code) in a and (s, code) in b and a[(s, code)]["present"] != b[(s, code)]["present"]:
                    disagreements.append((s, code, a[(s, code)]["present"], b[(s, code)]["present"]))
        agreement.append(("all manual", len(pooled), kappa(pooled),
                          sum(x == y for x, y in pooled) / len(pooled) if pooled else None))

    # Rows each coder left blank or missed: they drop out of kappa and of the
    # counts, so the report lists them rather than losing them quietly.
    uncoded = {n: sum(1 for s in stems for code in MANUAL_CODES if (s, code) not in coders[n])
               for n in names}
    for n in names:
        if uncoded[n]:
            print(f"warning: coding-{n}.csv leaves {uncoded[n]} drawing-code rows blank or missing",
                  file=sys.stderr)

    def manual(stem, code):
        # coding-resolved.csv overrides row by row; any row it lacks falls
        # back to the coders' agreement.
        if resolved is not None and (stem, code) in resolved:
            return resolved[(stem, code)]
        votes = [coders[n].get((stem, code)) for n in names]
        votes = [v for v in votes if v is not None]
        if not votes:
            return None
        present = all(v["present"] for v in votes)
        sizes = [v["size"] for v in votes if v["present"] and v["size"] is not None]
        return {"present": present,
                "count": min(v["count"] for v in votes) if present else 0,
                "size": statistics.median(sizes) if present and sizes else None,
                "survives": sum(v["survives"] for v in votes) * 2 >= len(votes)}

    for d in drawings:
        for code in MANUAL_CODES:
            v = manual(d["stem"], code)
            d["codes"][code] = None if v is None else {
                "count": v["count"] if v["present"] else 0,
                "sizes": [v["size"]] if v["present"] and v["size"] is not None else [],
                "survives": v["survives"]}

    # Per-code summary.
    rows = []
    for code in ALL_CODES:
        coded = [d for d in drawings if d["codes"].get(code) is not None]
        hits = [d for d in coded if d["codes"][code]["count"] > 0]
        share, people = {}, {}
        for task in "CT":
            pool = [d for d in coded if d["task"] == task]
            share[task] = (sum(1 for d in hits if d["task"] == task), len(pool))
            people[task] = len({d["pid"] for d in hits if d["task"] == task})
        sizes = [s for d in hits for s in d["codes"][code]["sizes"]]
        if not hits:
            survives = None
        elif code in AUTO_CODES:
            survives = True
        else:
            flags = [d["codes"][code]["survives"] for d in hits]
            survives = 2 * sum(flags) >= len(flags)
        # The share and the people are counted in the same task pool: C, or T.
        qualifies = any(share[t][1] and share[t][0] / share[t][1] >= CANDIDATE_SHARE
                        and people[t] >= CANDIDATE_PEOPLE for t in "CT")
        candidate = bool(hits) and qualifies and bool(survives)
        rows.append({"code": code, "kind": "auto" if code in AUTO_CODES else "manual",
                     "coded": len(coded), "hits": len(hits), "share": share,
                     "people": people, "median": statistics.median(sizes) if sizes else None,
                     "survives": survives, "candidate": candidate})

    lines = ["# Study A analysis", "",
             f"Drawings: {len(drawings)} from {len({d['pid'] for d in drawings})} people. "
             f"Coders: {', '.join(names) if names else 'none yet'}"
             + "; a manual code counts where every coder marked it"
             + (", except where coding-resolved.csv settles the row." if resolved is not None else "."),
             "",
             "At about 8 people this is a ranking, not a set of rates. A candidate still has to "
             "pass the exclusions in section 4 of docs/drawing-mistakes-research.md.",
             "",
             "## Codes", "",
             "| Code | Kind | Drawings with it | C drawings | T drawings | People (C/T) | Median size (cells) | Survives cells | Candidate |",
             "|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        (cn, ct), (tn, tt) = r["share"]["C"], r["share"]["T"]
        lines.append(f"| {r['code']} | {r['kind']} | {r['hits']}/{r['coded']} | {cn}/{ct} ({pct(cn, ct)}) "
                     f"| {tn}/{tt} ({pct(tn, tt)}) | {r['people']['C']}/{r['people']['T']} | {fmt(r['median'], 1)} "
                     f"| {'-' if r['survives'] is None else 'yes' if r['survives'] else 'no'} | {'**yes**' if r['candidate'] else 'no'} |")
    lines += ["", f"A candidate appears in at least {CANDIDATE_SHARE:.0%} of the C drawings or of the T drawings, "
              f"from at least {CANDIDATE_PEOPLE} people in that same task, and survives reduction to cells. "
              "Automatic codes are measured on cells, so they survive by construction.", ""]

    lines += ["## By item and condition", "", "| Item | Condition | Drawings | " + " | ".join(ALL_CODES) + " |",
              "|---|---|---|" + "---|" * len(ALL_CODES)]
    for item_name in items:
        for condition in "ZF":
            group = [d for d in drawings if d["item"] == item_name and d["condition"] == condition]
            if not group:
                continue
            cells = []
            for code in ALL_CODES:
                coded = [d for d in group if d["codes"].get(code) is not None]
                cells.append(f"{sum(1 for d in coded if d['codes'][code]['count'] > 0)}/{len(coded)}")
            lines.append(f"| {item_name} | {condition} | {len(group)} | " + " | ".join(cells) + " |")

    lines += ["", "## Drawings", "",
              "| Drawing | Canvas | Fit | Ink IoU | Drawn IoU | Codes (count: sizes) |", "|---|---|---|---|---|---|"]
    for d in drawings:
        fit = (f"offset {d['fit']['offset']}" if d["condition"] == "Z"
               else f"{d['fit']['block']} px/cell at {d['fit']['origin']}")
        if not d["aligned"]:
            found = "unaligned: automatic codes not scored"
            extra = [f"{c} {d['codes'][c]['count']}" for c in MANUAL_CODES
                     if d["codes"].get(c) and d["codes"][c]["count"]]
            if extra:
                found += "; " + ", ".join(extra)
        else:
            found = ", ".join(f"{c} {d['codes'][c]['count']}: {d['codes'][c]['sizes']}"
                              for c in ALL_CODES if d["codes"].get(c) and d["codes"][c]["count"]) or "none"
        canvas = "x".join(map(str, d["image_size"])) + ("" if d["canvas_ok"] else " (not the set size)")
        lines.append(f"| {d['stem']} | {canvas} | {fit} | {d['ink_iou']:.2f} | {d['drawn_iou']:.2f} | {found} |")

    lines += ["", "## Coder agreement", ""]
    if names:
        lines += ["Every row needs `present` 0 or 1. Rows left blank or missing drop out of kappa and "
                  "of the counts.", "", "| Coder | Rows expected | Blank or missing |", "|---|---|---|"]
        lines += [f"| {n} | {len(stems) * len(MANUAL_CODES)} | {uncoded[n]} |" for n in names]
        lines.append("")
    if agreement:
        lines += [f"Cohen's kappa between {names[0]} and {names[1]} on presence. Target 0.6 or more. "
                  "A dash means kappa is undefined because neither coder varied on that code.", "",
                  "| Code | Pairs | Kappa | Raw agreement |", "|---|---|---|---|"]
        for code, n, k, raw in agreement:
            lines.append(f"| {code} | {n} | {fmt(k, 3)} | {fmt(raw, 3)} |")
        if len(names) > 2:
            lines += ["", f"Only the first two coders by name are compared; {', '.join(names[2:])} ignored here."]
        if disagreements:
            lines += ["", "Disagreements to resolve (write coding-resolved.csv to settle them):", ""]
            lines += [f"- {s} {code}: {names[0]} {int(x)}, {names[1]} {int(y)}" for s, code, x, y in disagreements]
    else:
        lines.append("Fewer than two coding files, so no kappa. Copy coding-template.csv to "
                     "coding-<coder>.csv in the coding folder for each coder.")
    if skipped:
        lines += ["", "## Skipped files", "", "Not named pid_task_condition_item.png with a known item:", ""]
        lines += [f"- {n}" for n in skipped]
    with open(os.path.join(out, "report.md"), "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"wrote {os.path.join(out, 'report.md')}")


if __name__ == "__main__":
    main(sys.argv[1:])
