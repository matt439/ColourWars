"""Require scripts/README.md to name every tracked top-level entry under scripts/ (HYG#103)."""

from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]

# Inline links and reference definitions; the target is what names an entry.
LINK_TARGET = re.compile(r"\]\(\s*<?([^)\s>]+)>?[^)]*\)|^\s*\[[^\]]+\]:\s*<?(\S+?)>?\s*$", re.MULTILINE)


def tracked_entries(root: Path) -> set:
    """The top-level files and folders under scripts/ that Git tracks."""
    listing = subprocess.run(["git", "ls-files", "-z", "--", "scripts"], cwd=root, check=True,
                             capture_output=True).stdout.decode("utf-8")
    return {name.split("/")[1] for name in listing.split("\0") if name.count("/") >= 1}


def indexed_entries(readme: str) -> set:
    """The first path component of every relative link in the index."""
    named = set()
    for match in LINK_TARGET.finditer(readme):
        target = (match.group(1) or match.group(2)).split("#", 1)[0]
        if not target or "://" in target or target.startswith(("/", "../", "mailto:")):
            continue
        named.add(target.removeprefix("./").split("/", 1)[0])
    return named


def unindexed(entries: set, readme: str) -> list:
    """Entries the index does not link, sorted; the index itself names itself."""
    return sorted(entries - indexed_entries(readme) - {"README.md"})


class ScriptsIndex(unittest.TestCase):
    def test_every_tracked_entry_is_indexed(self) -> None:
        readme = (ROOT / "scripts/README.md").read_text(encoding="utf-8")
        missing = unindexed(tracked_entries(ROOT), readme)
        self.assertEqual(missing, [], "scripts/README.md does not link: " + ", ".join(missing))

    def test_a_planted_script_is_named(self) -> None:
        readme = "| [build.ps1](build.ps1) | x |\n| [ci/](ci/) | y |\n[studies/](studies/README.md#kits)\n"
        self.assertEqual(unindexed({"README.md", "build.ps1", "ci", "studies"}, readme), [])
        self.assertEqual(unindexed({"build.ps1", "ci", "planted.ps1"}, readme), ["planted.ps1"])

    def test_links_outside_scripts_and_prose_do_not_count(self) -> None:
        readme = ("[guide](../docs/planted.ps1) [web](https://example.invalid/planted.ps1)\n"
                  "`planted.ps1` is mentioned in prose only.\n")
        self.assertEqual(unindexed({"planted.ps1"}, readme), ["planted.ps1"])

    def test_reference_definitions_count(self) -> None:
        self.assertEqual(unindexed({"planted.ps1"}, "[ref]: planted.ps1\n"), [])


if __name__ == "__main__":
    unittest.main()
