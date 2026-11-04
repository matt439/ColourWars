"""The committed executable icon matches its editable drawing."""

import importlib.util
import struct
import unittest
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location("build_icon", ROOT / "scripts" / "build_icon.py")
build_icon = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(build_icon)

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def decoded_png(data):
    """A PNG's chunks with its image data inflated."""
    chunks, image, at = [], b"", len(PNG_SIGNATURE)
    while at < len(data):
        (length,) = struct.unpack_from(">I", data, at)
        kind, body = data[at + 4:at + 8], data[at + 8:at + 8 + length]
        if kind == b"IDAT":
            image += body
        else:
            chunks.append((kind, body))
        at += 12 + length
    return chunks, zlib.decompress(image)


def decoded(data):
    """What an output shows, without its compressed bytes.

    Those differ by zlib build: python.org's Windows 3.14 links zlib-ng, so the
    same pixels compress differently there and CI's byte comparison would fail.
    """
    if data.startswith(PNG_SIGNATURE):
        return decoded_png(data)
    _, _, count = struct.unpack_from("<HHH", data, 0)
    images = []
    for index in range(count):
        width, height, _, _, planes, bits, length, offset = struct.unpack_from(
            "<BBBBHHII", data, 6 + 16 * index)
        blob = data[offset:offset + length]
        images.append((width, height, planes, bits,
                       decoded_png(blob) if blob.startswith(PNG_SIGNATURE) else blob))
    return images


class BuildIconTest(unittest.TestCase):
    def test_committed_files_are_current(self):
        for path, data in build_icon.outputs().items():
            with self.subTest(path=path.name):
                self.assertTrue(path.is_file(), f"{path} is missing; run python3 scripts/build_icon.py")
                self.assertEqual(decoded(path.read_bytes()), decoded(data),
                                 f"{path.name} is stale; run python3 scripts/build_icon.py")

    def test_one_changed_pixel_is_seen(self):
        images = build_icon.load()
        current = decoded(build_icon.ico(images))
        for index in (0, len(images) - 1):  # the smallest is a DIB, the largest a PNG
            with self.subTest(size=images[index][0]):
                size, pixels = images[index]
                rows = [list(row) for row in pixels]
                rows[0][0] = tuple(255 - channel for channel in rows[0][0])
                changed = images[:index] + [(size, rows)] + images[index + 1:]
                self.assertNotEqual(decoded(build_icon.ico(changed)), current)

    def test_icon_holds_the_usual_windows_sizes(self):
        data = build_icon.ICON.read_bytes()
        reserved, kind, count = struct.unpack_from("<HHH", data, 0)
        self.assertEqual((reserved, kind), (0, 1))
        sizes = []
        for index in range(count):
            width, height, _, _, planes, bits, length, offset = struct.unpack_from(
                "<BBBBHHII", data, 6 + 16 * index)
            self.assertEqual((width, planes, bits), (height, 1, 32))
            self.assertLessEqual(offset + length, len(data))
            sizes.append(width or 256)
        self.assertEqual(sizes, [16, 24, 32, 48, 64, 256])

    def test_resource_script_embeds_it(self):
        script = (ROOT / "game" / "resource.rc").read_text(encoding="utf-8")
        self.assertIn('"colourwars.ico"', script)


if __name__ == "__main__":
    unittest.main()
