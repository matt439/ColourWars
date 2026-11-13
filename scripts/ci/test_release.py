"""Exercise the publish job's checks and notes against real Git histories."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from release import (check_tag, is_prerelease, main, package_name, project_version,
                     release_notes, SYMBOLS, SYMBOLS_LABEL)


ROOT = Path(__file__).resolve().parents[2]


def cmake(version: str) -> str:
    return f"cmake_minimum_required(VERSION 3.26)\n\nproject(ColourWars VERSION {version} LANGUAGES CXX RC)\n"


class ProjectVersion(unittest.TestCase):
    def test_reads_the_repository_version(self) -> None:
        text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(project_version(text) or "", r"^[0-9]+\.[0-9]+\.[0-9]+$")

    def test_needs_three_numbers_and_the_project_name(self) -> None:
        self.assertEqual(project_version(cmake("1.2.3")), "1.2.3")
        self.assertIsNone(project_version(cmake("1.2")))
        self.assertIsNone(project_version("project(Other VERSION 1.2.3 LANGUAGES CXX)\n"))
        self.assertIsNone(project_version("# project(ColourWars VERSION 1.2.3 LANGUAGES CXX)\n"))

    def test_package_name_matches_cpack(self) -> None:
        rule = (ROOT / "cmake/release_package.cmake").read_text(encoding="utf-8")
        self.assertIn("ColourWars-${PROJECT_VERSION}-windows-x64", rule)
        self.assertEqual(package_name("0.1.0"), "ColourWars-0.1.0-windows-x64.zip")


class Prerelease(unittest.TestCase):
    def test_zero_major_versions_are_prereleases(self) -> None:
        self.assertTrue(is_prerelease("0.1.0"))
        self.assertTrue(is_prerelease("0.12.3"))
        self.assertFalse(is_prerelease("1.0.0"))
        self.assertFalse(is_prerelease("10.0.0"))


class Notes(unittest.TestCase):
    def test_notes_keep_the_player_instructions(self) -> None:
        notes = release_notes("0.1.0")
        for text in ("`ColourWars-0.1.0-windows-x64.zip`", "**Extract All**", "`ColourWars.exe`",
                     "Windows 10/11", "Direct3D 11", "Windows protected your PC",
                     "**More info**", "**Run anyway**", "`HOW-TO-PLAY.txt`"):
            self.assertIn(text, notes)
        self.assertIn(f"`{SYMBOLS}`", notes)
        self.assertNotIn("<version>", notes)
        self.assertNotIn("{", notes)

    def test_notes_command_writes_the_file(self) -> None:
        with tempfile.TemporaryDirectory() as scratch:
            output = Path(scratch) / "notes.md"
            self.assertEqual(main(["notes", "--version", "2.3.4", "--output", str(output)]), 0)
            self.assertEqual(output.read_text(encoding="utf-8"), release_notes("2.3.4"))


class TagChecks(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.repo = Path(self.scratch.name)
        self.git("init", "-b", "master")
        for key, value in {"user.name": "CI regression tests", "user.email": "ci-tests@example.invalid",
                           "core.autocrlf": "false", "commit.gpgsign": "false"}.items():
            self.git("config", key, value)
        self.write(cmake("0.1.0"))
        self.tagged = self.commit()

    def git(self, *args: str) -> str:
        return subprocess.run(["git", *args], cwd=self.repo, check=True,
                              text=True, capture_output=True).stdout.strip()

    def write(self, text: str) -> None:
        (self.repo / "CMakeLists.txt").write_text(text, encoding="utf-8", newline="\n")

    def commit(self) -> str:
        self.git("add", "--all")
        self.git("commit", "-m", "Regression fixture")
        return self.git("rev-parse", "HEAD")

    def check(self, tag: str, commit: str):
        return check_tag(tag, commit, "master", cwd=self.repo)

    def test_matching_tag_on_master_passes(self) -> None:
        self.assertEqual(self.check("v0.1.0", self.tagged), ("0.1.0", []))

    def test_version_is_read_from_the_tagged_commit(self) -> None:
        self.write(cmake("0.2.0"))
        later = self.commit()
        self.assertEqual(self.check("v0.1.0", self.tagged), ("0.1.0", []))
        self.assertEqual(self.check("v0.2.0", later), ("0.2.0", []))
        _, problems = self.check("v0.2.0", self.tagged)
        self.assertEqual(problems, ["Tag v0.2.0 does not match project(VERSION 0.1.0)"])

    def test_mismatched_tag_is_refused(self) -> None:
        _, problems = self.check("v0.1.1", self.tagged)
        self.assertEqual(problems, ["Tag v0.1.1 does not match project(VERSION 0.1.0)"])

    def test_tag_off_master_is_refused(self) -> None:
        self.git("switch", "-c", "feature")
        self.write(cmake("0.1.0") + "# a branch-only change\n")
        branch = self.commit()
        self.git("switch", "master")
        _, problems = self.check("v0.1.0", branch)
        self.assertEqual(problems, ["v0.1.0 is not on master"])

    def test_malformed_tag_is_refused(self) -> None:
        for tag in ("v0.1", "0.1.0", "v0.1.0-rc1"):
            with self.subTest(tag=tag):
                version, problems = self.check(tag, self.tagged)
                self.assertIsNone(version)
                self.assertIn(f"Tag {tag} is not v<major>.<minor>.<patch>", problems)

    def test_unreadable_version_is_refused(self) -> None:
        self.write("project(ColourWars LANGUAGES CXX)\n")
        commit = self.commit()
        _, problems = self.check("v0.1.0", commit)
        self.assertEqual(problems, ["Cannot read project(ColourWars VERSION) from CMakeLists.txt"])

    def test_unknown_commit_is_refused(self) -> None:
        _, problems = self.check("v0.1.0", "0" * 40)
        self.assertTrue(problems)
        self.assertTrue(problems[0].startswith("Cannot compare v0.1.0 with master"), problems)

    def test_check_command_writes_outputs_or_errors(self) -> None:
        output = self.repo.parent / f"{self.repo.name}-github-output"
        self.addCleanup(lambda: output.unlink(missing_ok=True))
        cwd = os.getcwd()
        os.chdir(self.repo)
        self.addCleanup(os.chdir, cwd)
        with mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(output)}), \
                mock.patch("builtins.print") as printed:
            self.assertEqual(main(["check", "--tag", "v0.1.0", "--commit", self.tagged,
                                   "--master", "master"]), 0)
            self.assertEqual(output.read_text(encoding="utf-8").splitlines(), [
                "version=0.1.0", "package=ColourWars-0.1.0-windows-x64.zip",
                f"symbols_asset={SYMBOLS}#{SYMBOLS_LABEL}", "prerelease=true"])
            output.unlink()
            self.assertEqual(main(["check", "--tag", "v9.9.9", "--commit", self.tagged,
                                   "--master", "master"]), 1)
            self.assertFalse(output.exists())
            printed.assert_any_call("::error::Tag v9.9.9 does not match project(VERSION 0.1.0)")


if __name__ == "__main__":
    unittest.main()
