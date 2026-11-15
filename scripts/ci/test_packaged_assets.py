"""Release ZIP content must match reviewed source, not merely exist."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class PackagedAssets(unittest.TestCase):
    def test_reviewed_bytes_and_corruptions(self):
        with tempfile.TemporaryDirectory() as scratch:
            package = Path(scratch)
            # Extract the simple authoritative CMake list, not another asset list.
            assets = (ROOT / "cmake/runtime_assets.cmake").read_text().split(
                "set(COLOURWARS_RUNTIME_ASSETS", 1)[1].split(")", 1)[0].split()
            for asset in assets:
                destination = package / asset
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / "game/content" / asset, destination)
            shutil.copyfile(ROOT / "docs/HOW-TO-PLAY.txt", package / "HOW-TO-PLAY.txt")

            def check(expected=None):
                result = subprocess.run([
                    "cmake", f"-DPACKAGE_DIR={package}", "-P",
                    str(ROOT / "cmake/check_packaged_assets.cmake")],
                    capture_output=True, text=True)
                if expected is None:
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(expected, result.stdout + result.stderr)

            check()
            for asset in ("HOW-TO-PLAY.txt", "presentation.json", "textures/terrain.png",
                          "levels/movement_course.json"):
                with self.subTest(asset=asset):
                    path = package / asset
                    original = path.read_bytes()
                    path.write_bytes(original + b"stale")
                    check("Packaged asset differs from reviewed source")
                    path.write_bytes(original)
            missing = package / "tuning.json"
            original = missing.read_bytes()
            missing.unlink()
            check("Packaged asset missing or not a regular file")
            missing.write_bytes(original)
            extra = package / "textures/unreviewed.txt"
            extra.write_text("not shipped")
            check("Unreviewed packaged runtime asset")
            extra.unlink()
            check()
