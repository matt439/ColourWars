#!/usr/bin/env python3
"""Build the executable icon from its editable drawing.

Reads game/art/icon/icon.json and writes game/colourwars.ico, which
game/resource.rc embeds, and the review sheet game/art/icon/preview.png.
Standard library only. --check compares decoded image data so different zlib
builds can encode the same drawing without making the committed files stale.
scripts/ci/test_build_icon.py uses the same comparison on every CI event.
"""

import argparse
import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "game" / "art" / "icon" / "icon.json"
ICON = ROOT / "game" / "colourwars.ico"
PREVIEW = ROOT / "game" / "art" / "icon" / "preview.png"
PRESENTATION = ROOT / "game" / "content" / "presentation.json"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

# Sizes at or below this are stored as 32-bit BMP, the form every icon reader
# understands; larger ones as PNG, which Windows reads for 256 x 256.
LARGEST_BMP = 64

# Review sheet backgrounds: the Muted Day paper, and a dark taskbar grey.
PREVIEW_BACKGROUNDS = ((0xC9, 0xD3, 0xD9), (0x20, 0x20, 0x20))
PREVIEW_GAP = 8


def load(path=SOURCE, presentation=PRESENTATION):
    """Return [(size, rows of RGBA tuples)] for every image the drawing names."""
    document = json.loads(path.read_text(encoding="utf-8"))
    teams = json.loads(presentation.read_text(encoding="utf-8"))["team_palette"]
    colours = {entry["name"]: entry["colour"] for entry in teams["colours"]}
    pair = next(entry for entry in teams["pairs"] if entry["key"] == document["team_pair"])
    palette = {}
    for key, value in document["palette"].items():
        if len(key) != 1:
            raise ValueError(f"palette key {key!r} must be one character")
        if isinstance(value, dict):
            value = colours[pair[value["team"]]]
        palette[key] = None if value is None else (
            int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), 255)

    grids = {}
    for name, rows in document["grids"].items():
        side = len(rows)
        for number, row in enumerate(rows):
            if len(row) != side:
                raise ValueError(f"grid {name} row {number} is {len(row)} wide, not {side}")
            unknown = set(row) - palette.keys()
            if unknown:
                raise ValueError(f"grid {name} row {number} uses {sorted(unknown)}, not in the palette")
        grids[name] = rows

    images = []
    for entry in document["images"]:
        size, rows = entry["size"], grids[entry["grid"]]
        scale, remainder = divmod(size, len(rows))
        if remainder or scale < 1:
            raise ValueError(f"{size} is not a whole multiple of grid {entry['grid']} ({len(rows)})")
        pixels = []
        for row in rows:
            line = []
            for cell in row:
                colour = palette[cell] or (0, 0, 0, 0)
                line.extend([colour] * scale)
            pixels.extend([line] * scale)
        images.append((size, pixels))
    sizes = [size for size, _ in images]
    if len(set(sizes)) != len(sizes) or any(size > 256 for size in sizes):
        raise ValueError(f"image sizes {sizes} must be distinct and at most 256")
    return images


def png(pixels):
    """Encode RGBA rows as a PNG with no ancillary chunks."""
    height, width = len(pixels), len(pixels[0])

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    raw = b"".join(b"\0" + bytes(channel for pixel in row for channel in pixel) for row in pixels)
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b""))


def bmp(pixels):
    """Encode RGBA rows as an icon's DIB: header, bottom-up BGRA, then AND mask."""
    size = len(pixels)
    colour = b"".join(bytes((b, g, r, a)) for row in reversed(pixels) for (r, g, b, a) in row)
    stride = (size + 31) // 32 * 4
    mask = bytearray()
    for row in reversed(pixels):
        line = bytearray(stride)
        for x, (_, _, _, alpha) in enumerate(row):
            if alpha == 0:
                line[x // 8] |= 0x80 >> (x % 8)
        mask += line
    header = struct.pack("<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0,
                         len(colour) + len(mask), 0, 0, 0, 0)
    return header + colour + bytes(mask)


def ico(images):
    """Pack the images into one .ico, smallest first."""
    blobs = [(size, bmp(pixels) if size <= LARGEST_BMP else png(pixels))
             for size, pixels in sorted(images, key=lambda image: image[0])]
    offset = 6 + 16 * len(blobs)
    directory, data = [struct.pack("<HHH", 0, 1, len(blobs))], []
    for size, blob in blobs:
        side = 0 if size == 256 else size
        directory.append(struct.pack("<BBBBHHII", side, side, 0, 0, 1, 32, len(blob), offset))
        data.append(blob)
        offset += len(blob)
    return b"".join(directory + data)


def preview(images):
    """Every size at 1:1, left to right, on a light row above a dark row."""
    width = PREVIEW_GAP + sum(size + PREVIEW_GAP for size, _ in images)
    band = max(size for size, _ in images) + 2 * PREVIEW_GAP
    rows = []
    for background in PREVIEW_BACKGROUNDS:
        canvas = [[background + (255,)] * width for _ in range(band)]
        x = PREVIEW_GAP
        for size, pixels in images:
            top = band - PREVIEW_GAP - size
            for y, row in enumerate(pixels):
                line = canvas[top + y] = list(canvas[top + y])
                for column, pixel in enumerate(row):
                    if pixel[3]:
                        line[x + column] = pixel
            x += size + PREVIEW_GAP
        rows.extend(canvas)
    return png(rows)


def outputs(source=SOURCE):
    images = load(source)
    return {ICON: ico(images), PREVIEW: preview(images)}


def decoded_png(data):
    """Compare PNG chunks and inflated scanlines, independent of compression."""
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("not a PNG")
    chunks, image, at = [], bytearray(), len(PNG_SIGNATURE)
    while at < len(data):
        (length,) = struct.unpack_from(">I", data, at)
        kind, body = data[at + 4:at + 8], data[at + 8:at + 8 + length]
        if len(body) != length:
            raise ValueError("truncated PNG chunk")
        (crc,) = struct.unpack_from(">I", data, at + 8 + length)
        if crc != zlib.crc32(kind + body) & 0xFFFFFFFF:
            raise ValueError("invalid PNG chunk checksum")
        if kind == b"IDAT":
            image.extend(body)
        else:
            chunks.append((kind, body))
        at += 12 + length
    return chunks, zlib.decompress(image)


def decoded(data):
    """The generated PNG or ICO without its zlib-specific compressed bytes."""
    if data.startswith(PNG_SIGNATURE):
        return decoded_png(data)
    reserved, kind, count = struct.unpack_from("<HHH", data, 0)
    if (reserved, kind) != (0, 1):
        raise ValueError("not an icon")
    images = []
    for index in range(count):
        width, height, _, _, planes, bits, length, offset = struct.unpack_from(
            "<BBBBHHII", data, 6 + 16 * index)
        blob = data[offset:offset + length]
        if len(blob) != length:
            raise ValueError("truncated icon image")
        images.append((width, height, planes, bits,
                       decoded_png(blob) if blob.startswith(PNG_SIGNATURE) else blob))
    return images


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="fail if the committed icon or preview is not what the drawing builds")
    arguments = parser.parse_args(argv)
    stale = []
    for path, data in outputs().items():
        if arguments.check:
            try:
                current = path.is_file() and decoded(path.read_bytes()) == decoded(data)
            except (ValueError, struct.error, zlib.error):
                current = False
            if not current:
                stale.append(path.relative_to(ROOT).as_posix())
        else:
            path.write_bytes(data)
            print(f"wrote {path.relative_to(ROOT).as_posix()} ({len(data)} bytes)")
    if stale:
        print("out of date with game/art/icon/icon.json: " + ", ".join(stale)
              + "\nrun: python3 scripts/build_icon.py", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
