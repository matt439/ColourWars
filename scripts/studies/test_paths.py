"""The common privacy guard, including caller-relative and other-drive paths."""

import os
from pathlib import Path
import tempfile
import subprocess
import unittest
from unittest import mock

from _paths import REPO_ROOT, inside_repo_but_not_out


class StudyPathChecks(unittest.TestCase):
    def test_output_alias_cannot_write_into_source(self):
        with tempfile.TemporaryDirectory() as scratch:
            alias = Path(scratch) / "checkout"
            junction = False
            try:
                alias.symlink_to(REPO_ROOT, target_is_directory=True)
            except OSError as error:
                if os.name != "nt":
                    self.skipTest(f"directory symlinks unavailable: {error}")
                # Directory junctions need no symlink privilege on Windows.
                result = subprocess.run(["cmd.exe", "/d", "/c", "mklink", "/J", str(alias), REPO_ROOT],
                                        capture_output=True)
                if result.returncode:
                    self.skipTest("directory symlinks and junctions unavailable")
                junction = True
            try:
                self.assertTrue(inside_repo_but_not_out(alias / "scripts/results.json"))
                self.assertFalse(inside_repo_but_not_out(alias / "out/study/results.json"))
            finally:
                if junction:
                    os.rmdir(alias)  # remove only the junction, never its target
                else:
                    alias.unlink()

    def test_source_out_and_neighbor(self):
        root = Path(REPO_ROOT)
        self.assertTrue(inside_repo_but_not_out(root))
        self.assertTrue(inside_repo_but_not_out(root / "scripts/studies/results.json"))
        self.assertTrue(inside_repo_but_not_out(root / "out/../scripts/results.json"))
        self.assertFalse(inside_repo_but_not_out(root / "out"))
        self.assertFalse(inside_repo_but_not_out(root / "out/study/results.json"))
        self.assertFalse(inside_repo_but_not_out(root.with_name(root.name + "-study") / "results.json"))

    @unittest.skipUnless(os.name == "nt", "Windows drive letters")
    def test_another_drive_and_case(self):
        other = "Q:" if Path(REPO_ROOT).drive.upper() != "Q:" else "R:"
        # Model the explicit path without touching an unrelated physical drive.
        with mock.patch("_paths.os.path.realpath", side_effect=lambda path: str(path)):
            self.assertFalse(inside_repo_but_not_out(other + "\\study\\results.json"))
        self.assertTrue(inside_repo_but_not_out(REPO_ROOT.upper() + "\\scripts\\results.json"))

    @unittest.skipUnless(os.name == "nt", "Windows drive letters")
    def test_other_drive_alias_is_checked_after_resolution(self):
        other = "Q:" if Path(REPO_ROOT).drive.upper() != "Q:" else "R:"
        alias = other + "\\checkout\\results.json"
        target = os.path.join(REPO_ROOT, "scripts", "results.json")
        with mock.patch("_paths.os.path.realpath", side_effect=lambda path: target if path == alias else str(path)):
            self.assertTrue(inside_repo_but_not_out(alias))

    def test_inaccessible_target_is_refused(self):
        with mock.patch("_paths.os.path.realpath", side_effect=OSError("inaccessible output target")):
            self.assertTrue(inside_repo_but_not_out(Path(REPO_ROOT).parent / "study/results.json"))


if __name__ == "__main__":
    unittest.main()
