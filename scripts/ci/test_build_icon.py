"""The committed executable icon matches its editable drawing."""

import importlib.util
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location("build_icon", ROOT / "scripts" / "build_icon.py")
build_icon = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(build_icon)


class BuildIconTest(unittest.TestCase):
    def test_committed_files_are_current(self):
        for path, data in build_icon.outputs().items():
            with self.subTest(path=path.name):
                self.assertTrue(path.is_file(), f"{path} is missing; run python3 scripts/build_icon.py")
                self.assertEqual(path.read_bytes(), data,
                                 f"{path.name} is stale; run python3 scripts/build_icon.py")

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
