"""Release ZIP content must match reviewed source, not merely exist."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def stage_documents(scratch, package):
    fixture = scratch / "stage-documents.cmake"
    fixture.write_text("""
cmake_minimum_required(VERSION 3.26)
function(colourwars_packaged_document source installed)
    configure_file("${SOURCE_DIR}/${source}" "${PACKAGE_DIR}/${installed}" COPYONLY)
endfunction()
function(colourwars_packaged_vcpkg_notice package installed)
    file(WRITE "${PACKAGE_DIR}/${installed}" "Fixture notice for ${package}\n")
endfunction()
include("${SOURCE_DIR}/cmake/packaged_documents.cmake")
""", encoding="utf-8")
    subprocess.run(["cmake", f"-DSOURCE_DIR={ROOT}", f"-DPACKAGE_DIR={package}",
                    "-P", str(fixture)], check=True)


class PackagedAssets(unittest.TestCase):
    def test_reviewed_bytes_and_corruptions(self):
        with tempfile.TemporaryDirectory() as scratch:
            package = Path(scratch) / "package"
            # Extract the simple authoritative CMake list, not another asset list.
            assets = (ROOT / "cmake/runtime_assets.cmake").read_text().split(
                "set(COLOURWARS_RUNTIME_ASSETS", 1)[1].split(")", 1)[0].split()
            for asset in assets:
                destination = package / asset
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / "game/content" / asset, destination)
            stage_documents(Path(scratch), package)

            def check(expected=None, installed=None):
                result = subprocess.run([
                    "cmake", f"-DPACKAGE_DIR={package}", "-P",
                    str(ROOT / "cmake/check_packaged_assets.cmake")],
                    capture_output=True, text=True)
                if expected is None:
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                else:
                    self.assertNotEqual(result.returncode, 0)
                    diagnostic = result.stdout + result.stderr
                    self.assertIn(expected, diagnostic)
                    self.assertIn(installed, diagnostic)

            check()
            for asset in ("HOW-TO-PLAY.txt", "presentation.json", "textures/terrain.png",
                          "levels/movement_course.json", "licenses/fonts/Lato-OFL.txt"):
                with self.subTest(asset=asset):
                    path = package / asset
                    original = path.read_bytes()
                    path.write_bytes(original + b"stale")
                    check("Packaged asset differs from reviewed source", asset)
                    path.write_bytes(original)
            for asset in ("tuning.json", "NOTICE", "licenses/paint_exe/sources.json",
                          "licenses/DirectXTK-LICENSE.txt", "licenses/DirectXMath-LICENSE.txt",
                          "licenses/XAudio2Redist-LICENSE.txt"):
                with self.subTest(asset=asset):
                    path = package / asset
                    original = path.read_bytes()
                    path.unlink()
                    check("Packaged asset missing or not a regular file", asset)
                    path.write_bytes(original)
            extra = package / "textures/unreviewed.txt"
            extra.write_text("not shipped")
            check("Unreviewed packaged runtime asset", "textures/unreviewed.txt")
            extra.unlink()
            check()
