#!/usr/bin/env python3
"""Make the Study A reference images from the repository's own art sources.

Standard library only. Every image is generated from the files the game is
built from, so the references cannot drift from the art:

  figure    game/art/painter/painter.json: torso + idle legs, rest_arms on top
  bricks    the halved bond of GroundMaterial
            (game/objects/terrain_material_drawings.cpp), at quarter scale
  brushpot  the brush_pot frame cropped from game/art/sources/textures/terrain.png
            at the atlas position recorded in game/content/textures/terrain.json

Writes to --out (default out/study/study-a/references under the repository,
which git ignores). See scripts/studies/README.md.
"""

import argparse
import hashlib
import json
import os
import struct
import sys

# The study tools are also runnable directly from their own folders.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from _paths import inside_repo_but_not_out
import zlib

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))

PAPER = (255, 255, 255)
INK = (0, 0, 0)
# Paint 98's default palette has both greys, so a participant can pick the
# exact colour the reference uses.
GREY = (128, 128, 128)
LIGHT_GREY = (192, 192, 192)
# Trace underlays are lighter than every Paint palette colour except white, so
# analyse.py reads them as paper (its paper threshold is min channel >= 208).
UNDER_INK = (212, 212, 212)
UNDER_FILL = (236, 236, 236)
GRID_LINE = (150, 170, 210)  # not a Paint palette colour, so it cannot be confused with the art

ITEM_ORDER = ("figure", "bricks", "brushpot")


# --------------------------------------------------------------------------
# PNG writing and reading (8-bit, non-interlaced)
# --------------------------------------------------------------------------

def _chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def write_png(path, pixels):
    """pixels: list of rows of (r, g, b) tuples. Writes 8-bit RGB, filter 0."""
    height = len(pixels)
    width = len(pixels[0])
    raw = bytearray()
    for row in pixels:
        if len(row) != width:
            raise ValueError("ragged image rows")
        raw.append(0)
        for r, g, b in row:
            raw += bytes((r, g, b))
    data = (b"\x89PNG\r\n\x1a\n"
            + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
            + _chunk(b"IDAT", zlib.compress(bytes(raw), 9))
            + _chunk(b"IEND", b""))
    with open(path, "wb") as handle:
        handle.write(data)


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def read_png(path):
    """Minimal reader for 8-bit, non-interlaced RGB or RGBA PNGs (what
    System.Drawing writes). Returns (width, height, rows of RGBA tuples)."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    pos, ihdr, idat = 8, None, bytearray()
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if zlib.crc32(kind + body) & 0xFFFFFFFF != struct.unpack(">I", data[pos + 8 + length:pos + 12 + length])[0]:
            raise ValueError(f"{path}: bad CRC in {kind!r}")
        if kind == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
        pos += 12 + length
    width, height, depth, colour, _, _, interlace = ihdr
    if depth != 8 or colour not in (2, 6) or interlace != 0:
        raise ValueError(f"{path}: only 8-bit non-interlaced RGB/RGBA is supported "
                         f"(depth {depth}, colour type {colour}, interlace {interlace})")
    bpp = 4 if colour == 6 else 3
    stride = width * bpp
    raw = zlib.decompress(bytes(idat))
    if len(raw) != height * (stride + 1):
        raise ValueError(f"{path}: unexpected image data length")
    rows, previous = [], bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        kind = raw[start]
        line = bytearray(raw[start + 1:start + 1 + stride])
        for i in range(stride):
            left = line[i - bpp] if i >= bpp else 0
            up = previous[i]
            corner = previous[i - bpp] if i >= bpp else 0
            if kind == 1:
                line[i] = (line[i] + left) & 0xFF
            elif kind == 2:
                line[i] = (line[i] + up) & 0xFF
            elif kind == 3:
                line[i] = (line[i] + ((left + up) >> 1)) & 0xFF
            elif kind == 4:
                line[i] = (line[i] + _paeth(left, up, corner)) & 0xFF
            elif kind != 0:
                raise ValueError(f"{path}: bad filter type {kind}")
        if bpp == 4:
            rows.append([tuple(line[x:x + 4]) for x in range(0, stride, 4)])
        else:
            rows.append([tuple(line[x:x + 3]) + (255,) for x in range(0, stride, 3)])
        previous = line
    return width, height, rows


# --------------------------------------------------------------------------
# The three items, as grids of class characters: '.' paper, 'K' ink,
# 'T' grey fill, 'L' light-grey fill, 'W' white inside the outline.
# --------------------------------------------------------------------------

COLOURS = {".": PAPER, "W": PAPER, "K": INK, "T": GREY, "L": LIGHT_GREY}


def figure_grid(legs="idle"):
    source = json.load(open(os.path.join(REPO, "game/art/painter/painter.json"), encoding="utf-8"))
    rows = list(source["torso"]) + list(source["legs"][legs])
    if len(rows) != source["height"] or any(len(r) != source["width"] for r in rows):
        raise ValueError(f"painter.json: {legs} figure is not width x height")
    grid = [list(r) for r in rows]
    # The rest arms are drawn over the body (build-painter-art.ps1).
    for y, row in enumerate(source["rest_arms"]):
        for x, cell in enumerate(row):
            if cell != ".":
                grid[y][x] = cell
    for row in grid:
        for cell in row:
            if cell not in ".KWT":
                raise ValueError(f"painter.json: unknown legend character {cell!r}")
    features = [
        # Windows are inclusive cell rectangles x0, y0, x1, y1 inside the head.
        {"name": "left_eye", "window": [5, 5, 7, 8]},
        {"name": "right_eye", "window": [9, 5, 11, 8]},
        {"name": "mouth", "window": [5, 9, 11, 10]},
    ]
    check_figure_features(grid, features)
    return grid, features


def check_figure_features(grid, features):
    """The windows are written by hand, so check them against the art: each
    eye window holds one unbroken column of ink, the mouth window holds ink,
    and no window reaches ink on its left or right edge column (which would
    mean it has caught the head's outline)."""
    for feature in features:
        x0, y0, x1, y1 = feature["window"]
        ink = [(x, y) for y in range(y0, y1 + 1) for x in range(x0, x1 + 1) if grid[y][x] == "K"]
        problem = None
        if not ink:
            problem = "holds no ink"
        elif any(x in (x0, x1) for x, _ in ink):
            problem = "has ink on its edge column"
        elif feature["name"].endswith("_eye"):
            xs, ys = {x for x, _ in ink}, sorted(y for _, y in ink)
            if len(xs) != 1 or ys != list(range(ys[0], ys[-1] + 1)):
                problem = "does not hold exactly one unbroken column of ink"
        if problem:
            raise ValueError(f"painter.json: the {feature['name']} window {feature['window']} {problem}; "
                             "update the feature windows in make-references.py to match the face")


BRICK_SCALE = 4         # cells in the game per cell here
COURSE_CELLS = 16       # terrain_material_drawings.cpp: course = 16 cells
BRICK_CELLS = 32        # terrain_material_drawings.cpp: brick = 32 cells


def check_brick_constants():
    path = os.path.join(REPO, "game/objects/terrain_material_drawings.cpp")
    text = open(path, encoding="utf-8").read()
    for needle in ("constexpr float course = %d.0f * CELL;" % COURSE_CELLS,
                   "constexpr float brick = %d.0f * CELL;" % BRICK_CELLS,
                   "(index % 2 == 0) ? 0.0f : brick / 2.0f"):
        if needle not in text:
            raise ValueError(f"{path}: expected `{needle}`; update make-references.py to match the bricks")


def bricks_grid():
    """Six bricks by three courses, halved bond, one-cell outline and joints,
    course and brick sizes divided by BRICK_SCALE (a 4x8-cell brick)."""
    check_brick_constants()
    course = COURSE_CELLS // BRICK_SCALE
    brick = BRICK_CELLS // BRICK_SCALE
    width, height = 6 * brick + 1, 3 * course + 1
    grid = [["T"] * width for _ in range(height)]
    features = []
    for y in range(height):
        for x in range(width):
            if x in (0, width - 1) or y in (0, height - 1):
                grid[y][x] = "K"
    for index in range(3):
        top = index * course
        if index > 0:
            for x in range(width):
                grid[top][x] = "K"
        offset = 0 if index % 2 == 0 else brick // 2
        for x in range(offset or brick, width - 1, brick):
            for y in range(top + 1, top + course):
                grid[y][x] = "K"
            features.append({"name": f"joint_c{index}_x{x}",
                             "window": [max(1, x - 3), top + 1, min(width - 2, x + 3), top + course - 1]})
    return grid, features


def brushpot_grid():
    atlas_json = json.load(open(os.path.join(REPO, "game/content/textures/terrain.json"), encoding="utf-8"))
    frame = next(f for f in atlas_json["sprite_frames"] if f["name"] == "brush_pot")
    art = json.load(open(os.path.join(REPO, "game/art/terrain/terrain.json"), encoding="utf-8"))
    cell = art["cell_size"]
    prop = next(p for p in art["props"] if p["name"] == "brush_pot")
    px, py = frame["position"]["x"], frame["position"]["y"]
    pw, ph = frame["size"]["w"], frame["size"]["h"]
    if [pw // cell, ph // cell] != prop["cells"]:
        raise ValueError("brush_pot atlas frame does not match its cells in terrain.json")
    palette = {tuple(int(art["palette"][role][i:i + 2], 16) for i in (0, 2, 4)): role
               for role in art["palette"]}
    # Recoloured to Paint 98's palette so a participant can match it, and so the
    # outline reads as ink in the analysis. The study is about shape.
    role_class = {"bg_line": "K", "bg_mid": "T", "bg_far": "L"}
    width, height, rows = read_png(os.path.join(REPO, "game/art/sources/textures/terrain.png"))
    grid = []
    for cy in range(ph // cell):
        line = []
        for cx in range(pw // cell):
            block = {rows[py + cy * cell + dy][px + cx * cell + dx]
                     for dy in range(cell) for dx in range(cell)}
            if len(block) != 1:
                raise ValueError(f"brush_pot cell {cx},{cy} is not one colour: {block}")
            r, g, b, a = block.pop()
            if a == 0:
                line.append(".")
                continue
            role = palette.get((r, g, b))
            if a != 255 or role not in role_class:
                raise ValueError(f"brush_pot cell {cx},{cy}: unexpected colour {(r, g, b, a)} ({role})")
            line.append(role_class[role])
        grid.append(line)
    return grid, []


# --------------------------------------------------------------------------
# Rendering
# --------------------------------------------------------------------------

def render(grid, scale, colours):
    out = []
    for row in grid:
        line = []
        for cell in row:
            line += [colours[cell]] * scale
        for _ in range(scale):
            out.append(list(line))
    return out


def render_grid(grid, pitch):
    """What jspaint shows at 400% with the grid on: pitch-1 pixels of colour
    per cell and a one-pixel grid line."""
    height, width = len(grid), len(grid[0])
    out = [[GRID_LINE] * (width * pitch + 1) for _ in range(height * pitch + 1)]
    for y, row in enumerate(grid):
        for x, cell in enumerate(row):
            for dy in range(1, pitch):
                for dx in range(1, pitch):
                    out[y * pitch + dy][x * pitch + dx] = COLOURS[cell]
    return out


UNDERLAY = {".": PAPER, "W": PAPER, "K": UNDER_INK, "T": UNDER_FILL, "L": UNDER_FILL}

# F canvases: the figure is 136x240 at 100% (8 px a cell), as the design says.
# The bricks use the same 8 px. The brush pot uses its own atlas size, 4 px a
# cell (256x320), because at 8 px it would not fit a laptop screen at 100%.
F_SCALE = {"figure": 8, "bricks": 8, "brushpot": 4}
# ASYM is scored only on items whose reference is nearly mirror-symmetric.
# The brush pot is not (hundreds of asymmetric pairs), so a net count against
# it is noise and analyse.py reports ASYM as not computable there.
ASYM_ITEMS = {"figure": True, "bricks": True, "brushpot": False}
MANIFEST_VERSION = 2


def asymmetric_pairs(grid):
    """Cells mirrored about the centre column whose class (paper, fill, ink) differs."""
    kind = {".": 0, "W": 0, "T": 1, "L": 1, "K": 2}
    width = len(grid[0])
    return sum(1 for row in grid for x in range(width // 2) if kind[row[x]] != kind[row[width - 1 - x]])
SOURCES = {
    "figure": ["game/art/painter/painter.json"],
    "bricks": ["game/objects/terrain_material_drawings.cpp"],
    "brushpot": ["game/art/terrain/terrain.json", "game/content/textures/terrain.json",
                 "game/art/sources/textures/terrain.png"],
}


def sha256(path):
    with open(os.path.join(REPO, path), "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()



def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=os.path.join(REPO, "out", "study", "study-a", "references"),
                        help="output folder (default: out/study/study-a/references in the repository)")
    args = parser.parse_args(argv)
    out = os.path.abspath(args.out)
    if inside_repo_but_not_out(out):
        sys.exit(f"refusing to write inside the repository outside out/: {out}")
    os.makedirs(out, exist_ok=True)

    builders = {"figure": figure_grid, "bricks": bricks_grid, "brushpot": brushpot_grid}
    manifest = {"version": MANIFEST_VERSION, "classes": {"K": "ink", "T": "fill", "L": "fill", "W": "paper", ".": "paper"},
                "items": []}
    for name in ITEM_ORDER:
        grid, features = builders[name]()
        height, width = len(grid), len(grid[0])
        f = F_SCALE[name]
        files = {
            "cells": f"{name}_cells.png",
            "grid": f"{name}_grid400.png",
            "f": f"{name}_f.png",
            "trace_z": f"{name}_trace_z.png",
            "trace_f": f"{name}_trace_f.png",
            "blank_z": f"{name}_blank_z.png",
            "blank_f": f"{name}_blank_f.png",
        }
        write_png(os.path.join(out, files["cells"]), render(grid, 1, COLOURS))
        write_png(os.path.join(out, files["grid"]), render_grid(grid, 4))
        write_png(os.path.join(out, files["f"]), render(grid, f, COLOURS))
        write_png(os.path.join(out, files["trace_z"]), render(grid, 1, UNDERLAY))
        write_png(os.path.join(out, files["trace_f"]), render(grid, f, UNDERLAY))
        # Opening a blank file in jspaint sets the canvas size in one step.
        write_png(os.path.join(out, files["blank_z"]), [[PAPER] * width for _ in range(height)])
        write_png(os.path.join(out, files["blank_f"]), [[PAPER] * (width * f) for _ in range(height * f)])
        manifest["items"].append({
            "name": name,
            "cells": [width, height],
            "z_canvas": [width, height],
            "f_scale": f,
            "f_canvas": [width * f, height * f],
            "files": files,
            "features": features,
            "asym": ASYM_ITEMS[name],
            "asym_pairs": asymmetric_pairs(grid),
            "rows": ["".join(r) for r in grid],
            "sources": {p: sha256(p) for p in SOURCES[name]},
        })
        print(f"{name}: {width}x{height} cells, F canvas {width * f}x{height * f}")
    # The optional run frames: run_0 and run_1 side by side, one empty column
    # between. Shown for the copy-paste-or-redraw question only; not scored.
    left, _ = figure_grid("run_0")
    right, _ = figure_grid("run_1")
    pair = [a + ["."] + b for a, b in zip(left, right)]
    write_png(os.path.join(out, "run_frames_grid400.png"), render_grid(pair, 4))
    write_png(os.path.join(out, "run_frames_blank_z.png"), [[PAPER] * len(pair[0]) for _ in pair])
    print(f"run frames: {len(pair[0])}x{len(pair)} cells (not scored)")
    with open(os.path.join(out, "items.json"), "w", encoding="utf-8", newline="\n") as handle:
        json.dump(manifest, handle, indent=2)
        handle.write("\n")
    print(f"wrote {out}")


if __name__ == "__main__":
    main(sys.argv[1:])
