#!/usr/bin/env python3
"""Check the Study A kit end to end on synthetic drawings.

Generates the references into a temporary folder, makes 'participant'
drawings by perturbing them in known ways, runs analyse.py and checks the
automatic codes, the alignment and reduction of F drawings, Cohen's kappa,
blank coding rows, coding-resolved.csv and the candidate rule. No real
participant data is involved and nothing is written inside the repository.

Needs Pillow, like analyse.py. Usage: python selftest.py [--keep FOLDER]

--keep FOLDER must be a new or empty folder, or one an earlier selftest.py
run kept (it carries a .study-a-selftest marker). Any other folder is
refused, so a real study folder is never touched.
"""

import argparse
import csv
import json
import os
import shutil
import subprocess
import sys

# The study tools are also runnable directly from their own folders.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from _paths import inside_repo_but_not_out
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
MARKER = ".study-a-selftest"

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("selftest.py needs Pillow; see the README (install it outside the repository).")

COLOUR = {".": (255, 255, 255), "W": (255, 255, 255), "K": (0, 0, 0),
          "T": (128, 128, 128), "L": (192, 192, 192)}


def save(path, rows, scale=1, canvas=None, origin=(0, 0)):
    """rows: list of strings. Each cell becomes a scale x scale block, drawn
    at origin on a white canvas (default: exactly the drawing's size)."""
    w, h = len(rows[0]), len(rows)
    size = canvas or (w * scale, h * scale)
    image = Image.new("RGB", size, (255, 255, 255))
    px = image.load()
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            for dy in range(scale):
                for dx in range(scale):
                    X, Y = origin[0] + x * scale + dx, origin[1] + y * scale + dy
                    if 0 <= X < size[0] and 0 <= Y < size[1]:
                        px[X, Y] = COLOUR[c]
    image.save(path)


def sketch(path, rows, scale, wobble, fill_overlap):
    """A freehand-like F drawing: each ink cell becomes a point near the cell
    centre, moved by up to `wobble` px (a fixed pattern, not random), joined
    to its right and lower ink neighbours by 1-px pencil lines. Fills are
    bucket-like blocks; with fill_overlap False they stop short of the lines,
    so they do not touch them."""
    w, h = len(rows[0]), len(rows)
    image = Image.new("RGB", (w * scale, h * scale), (255, 255, 255))
    draw = ImageDraw.Draw(image)
    grow = scale // 2 if fill_overlap else 0
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c in "TL":
                draw.rectangle([x * scale - grow, y * scale - grow,
                                x * scale + scale - 1 + grow, y * scale + scale - 1 + grow], fill=COLOUR[c])
    span = 2 * wobble + 1

    def centre(x, y):
        return (x * scale + scale // 2 + (x * 7 + y * 13) % span - wobble,
                y * scale + scale // 2 + (x * 11 + y * 5) % span - wobble)
    ink = {(x, y) for y, row in enumerate(rows) for x, c in enumerate(row) if c == "K"}
    for x, y in sorted(ink, key=lambda c: (c[1], c[0])):
        draw.point(centre(x, y), fill=(0, 0, 0))
        for dx, dy in ((1, 0), (0, 1)):
            if (x + dx, y + dy) in ink:
                draw.line([centre(x, y), centre(x + dx, y + dy)], fill=(0, 0, 0), width=1)
    image.save(path)


def put(rows, x, y, c):
    rows = list(rows)
    rows[y] = rows[y][:x] + c + rows[y][x + 1:]
    return rows



def prepare(keep):
    """The work folder. A --keep folder must be new, empty or marked as ours."""
    if not keep:
        return tempfile.mkdtemp(prefix="study-a-selftest-")
    work = os.path.abspath(keep)
    if inside_repo_but_not_out(work):
        sys.exit(f"refusing --keep inside repository source or at an unresolved path: {work}")
    if os.path.exists(work) and not os.path.isdir(work):
        sys.exit(f"refusing --keep: {work} is a file")
    if os.path.isdir(work) and os.listdir(work) and not os.path.isfile(os.path.join(work, MARKER)):
        sys.exit(f"refusing --keep: {work} is not empty and was not made by selftest.py "
                 f"(it has no {MARKER} file). Give a new or empty folder.")
    os.makedirs(work, exist_ok=True)
    with open(os.path.join(work, MARKER), "w", encoding="utf-8", newline="\n") as handle:
        handle.write("Made by scripts/studies/study-a/selftest.py, which may replace this folder's contents.\n")
    for name in ("references", "drawings", "analysis"):   # only a marked folder has any
        shutil.rmtree(os.path.join(work, name), ignore_errors=True)
    return work


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--keep", help="work in this new or empty folder and keep it "
                                       "(default: a temporary folder, deleted)")
    args = parser.parse_args(argv)
    work = prepare(args.keep)
    refs, drawings, out = (os.path.join(work, n) for n in ("references", "drawings", "analysis"))
    os.makedirs(drawings, exist_ok=True)

    def analyse():
        subprocess.run([sys.executable, os.path.join(HERE, "analyse.py"), "--references", refs,
                        "--drawings", drawings, "--out", out], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        subprocess.run([sys.executable, os.path.join(HERE, "make-references.py"), "--out", refs],
                       check=True, stdout=subprocess.DEVNULL)
        items = {i["name"]: i for i in json.load(open(os.path.join(refs, "items.json")))["items"]}
        fig, bricks, pot = (items[n]["rows"] for n in ("figure", "bricks", "brushpot"))

        expect = {}

        def case(name, rows, codes, **kw):
            save(os.path.join(drawings, name + ".png"), rows, **kw)
            expect[name] = codes

        case("p01_C_Z_figure", fig, {})
        eye_up = put(put(fig, 6, 7, "W"), 6, 5, "K")
        case("p02_C_Z_figure", eye_up, {"PLACE": [1], "ASYM": [2]})
        case("p03_T_Z_figure", put(fig, 8, 1, "."), {"GAP": [1]})
        case("p04_C_Z_figure", put(put(fig, 1, 27, "K"), 15, 2, "T"), {"STRAY": [1, 1]})
        case("p05_C_Z_figure", put(fig, 0, 4, "K"), {"OVER": [1]})
        case("p06_T_Z_figure", put(fig, 1, 4, "."), {"UNDER": [1]})
        # F at 7 px a cell instead of 8, off the canvas origin: the reduction
        # must find the block size and phase and lose nothing.
        case("p07_C_F_figure", fig, {}, scale=7, canvas=(136, 240), origin=(9, 3))
        joint = [r for r in bricks]
        for y in (1, 2, 3):
            joint = put(put(joint, 16, y, "T"), 17, y, "K")
        case("p08_T_Z_bricks", joint, {"PLACE": [1], "ASYM": [6]})
        case("p09_C_F_brushpot", pot, {}, scale=4)
        case("p10_D_Z_figure", ["." * 17] * 12 + ["...KKKKK" + "." * 9] * 3 + ["." * 17] * 15, None)
        # An F drawing with the eye moved and one stray pixel, at 8 px a cell.
        save(os.path.join(drawings, "p11_C_F_figure.png"), eye_up, scale=8)
        image = Image.open(os.path.join(drawings, "p11_C_F_figure.png"))
        image.putpixel((133, 100), (0, 0, 0))
        image.save(os.path.join(drawings, "p11_C_F_figure.png"))
        expect["p11_C_F_figure"] = {"PLACE": [1], "ASYM": [2], "STRAY": [1]}
        # Two extra rows of body and leg, on a canvas taller than the set size:
        # taller than the reference's proportion, and not cropped.
        tall = fig[:20] + [fig[19]] + fig[20:26] + [fig[25]] + fig[26:]
        case("p12_C_Z_figure", tall, {"PROP": [1]})
        # The figure without its neck: head and body are two pieces, and the
        # neck cell opens to the outside at both sides. Alignment must still
        # use the whole figure, at Z and at F.
        no_neck = put(put(fig, 7, 12, "."), 9, 12, ".")
        case("p13_C_Z_figure", no_neck, {"GAP": [1, 1]})
        case("p14_C_F_figure", no_neck, {"GAP": [1, 1]}, scale=8)
        # Freehand-like 1-px pencil lines that wander, F at 8 px a cell: bucket
        # fills reaching under the lines on the figure, and fills that stop
        # short of them on the bricks.
        sketch(os.path.join(drawings, "p15_C_F_figure.png"), fig, 8, 1, True)
        expect["p15_C_F_figure"] = {}
        sketch(os.path.join(drawings, "p16_C_F_bricks.png"), bricks, 8, 2, False)
        expect["p16_C_F_bricks"] = {}
        with open(os.path.join(drawings, "notes.png"), "wb") as handle:
            handle.write(b"")   # misnamed: must be skipped, not read

        analyse()
        results = {d["stem"]: d for d in json.load(open(os.path.join(out, "drawings.json")))}

        failures = []
        auto = ["ASYM", "GAP", "OVER", "UNDER", "PROP", "PLACE", "STRAY"]
        for stem, want in expect.items():
            d = results[stem]
            if want is None:
                if d["aligned"]:
                    failures.append(f"{stem}: expected unaligned, drawn IoU {d['drawn_iou']}")
                continue
            got = {c: d["codes"][c]["sizes"] for c in auto if d["codes"][c] and d["codes"][c]["count"]}
            if got != want:
                failures.append(f"{stem}: expected {want}, got {got}")
            print(f"{stem}: {got or 'no codes'} (drawn IoU {d['drawn_iou']}, fit {d['fit']})")
        fits = {"p07_C_F_figure": {"block": 7, "origin": [9, 3]},
                "p13_C_Z_figure": {"offset": [0, 0]},
                "p14_C_F_figure": {"block": 8, "origin": [0, 0]}}
        for stem, fit in fits.items():
            if results[stem]["fit"] != fit:
                failures.append(f"{stem}: expected fit {fit}, got {results[stem]['fit']}")
        for stem in ("p15_C_F_figure", "p16_C_F_bricks"):
            if results[stem]["fit"]["block"] != 8:
                failures.append(f"{stem}: expected 8 px a cell, got {results[stem]['fit']}")
        # Codes with nothing to measure are None, not 0: no ASYM or PLACE on
        # the brush pot (asymmetric, no feature windows), no UNDER on the
        # bricks (no stroke ends).
        for stem, code in (("p09_C_F_brushpot", "ASYM"), ("p09_C_F_brushpot", "PLACE"),
                           ("p08_T_Z_bricks", "UNDER"), ("p16_C_F_bricks", "UNDER")):
            if results[stem]["codes"][code] is not None:
                failures.append(f"{stem}: {code} should be not computable, got {results[stem]['codes'][code]}")

        # Two coders over the sixteen drawings, in sorted order. Indices 0-4
        # are p01 C, p02 C, p03 T, p04 C, p05 C; there are 12 C and 3 T drawings.
        # WOB: A marks 0-3, B marks 0-2 and 4. Agreement 14/16, pA = pB = 1/4,
        # expected agreement 1/16 + 9/16 = 5/8, so kappa = (7/8 - 5/8) / (3/8) = 0.667.
        # Both mark 0-2: 2 of 12 C drawings, and 1 of 3 T drawings (33%) but
        # from one person, so WOB is not a candidate: the share and the people
        # must come from the same task pool.
        # WEIGHT: both mark 0, 1, 3 and 4: 4 of 12 C drawings (33%) from 4
        # people, so it is a candidate.
        # B leaves GRID blank on the last drawing: one blank row reported.
        stems = sorted(expect)
        marks = {"A": {"WOB": {0, 1, 2, 3}, "WEIGHT": {0, 1, 3, 4}},
                 "B": {"WOB": {0, 1, 2, 4}, "WEIGHT": {0, 1, 3, 4}}}
        for coder, marked in marks.items():
            with open(os.path.join(out, "coding-template.csv"), encoding="utf-8") as src:
                rows = list(csv.DictReader(src))
            for row in rows:
                index = stems.index(row["drawing"])
                if row["code"] in marked:
                    row["present"] = "1" if index in marked[row["code"]] else "0"
                    row["size_cells"] = "2" if row["present"] == "1" else ""
                elif coder == "B" and row["code"] == "GRID" and index == len(stems) - 1:
                    row["present"] = ""
                else:
                    row["present"] = "0"
            with open(os.path.join(drawings, f"coding-{coder}.csv"), "w", encoding="utf-8", newline="") as dst:
                writer = csv.DictWriter(dst, fieldnames=list(rows[0]))
                writer.writeheader()
                writer.writerows(rows)
        analyse()
        report = open(os.path.join(out, "report.md"), encoding="utf-8").read()

        def code_row(code):
            return next(line for line in report.splitlines() if line.startswith(f"| {code} | manual |"))
        for want in ("| WOB | 16 | 0.667 | 0.875 |", "| A | 96 | 0 |", "| B | 96 | 1 |", "- notes.png"):
            if want not in report:
                failures.append(f"report lacks '{want}'")
        if "**yes**" in code_row("WOB"):
            failures.append(f"WOB should not be a candidate: {code_row('WOB')}")
        if "**yes**" not in code_row("WEIGHT"):
            failures.append(f"WEIGHT should be a candidate: {code_row('WEIGHT')}")

        # coding-resolved.csv settles one row (WEIGHT on p05 is 0). Every other
        # row keeps the coders' agreement: WEIGHT drops to 3 of 12 C drawings
        # (25%) and is no longer a candidate, and WOB is unchanged.
        with open(os.path.join(drawings, "coding-resolved.csv"), "w", encoding="utf-8", newline="") as dst:
            writer = csv.writer(dst)
            writer.writerow(["drawing", "code", "present"])
            writer.writerow([stems[4], "WEIGHT", "0"])
        analyse()
        report = open(os.path.join(out, "report.md"), encoding="utf-8").read()
        weight = code_row("WEIGHT")
        if not weight.startswith("| WEIGHT | manual | 3/16 | 3/12 (25%) | 0/3 (0%) | 3/0 |") or "**yes**" in weight:
            failures.append(f"coding-resolved.csv should override one row only: {weight}")
        if not code_row("WOB").startswith("| WOB | manual | 3/16 |"):
            failures.append(f"WOB should keep the coders' agreement: {code_row('WOB')}")

        if failures:
            print("FAIL\n" + "\n".join(failures))
            return 1
        print(f"PASS: {len(expect)} synthetic drawings; fits, not-computable codes, kappa, blank rows, "
              "coding-resolved.csv and the candidate rule as expected")
        return 0
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
