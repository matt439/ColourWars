"""The publish job's checks and release notes (REL#4).

A v* tag push runs this twice from the `publish` job in ci.yml:

    python3 scripts/ci/release.py check --tag v0.1.0 --commit <sha> --master origin/master
    python3 scripts/ci/release.py notes --version 0.1.0 --output notes.md

`check` refuses a tag whose commit is not on master or whose name is not the
`project(VERSION)` that commit's CMakeLists.txt declares, and names the assets
the release attaches, and whether it is a pre-release (REL#6: every 0.x
version is). It reads the version from the tagged commit, not the
working tree, with the same pattern as scripts/release-package-path.ps1.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys


PROJECT_VERSION = re.compile(r"^project\(ColourWars VERSION ([0-9]+\.[0-9]+\.[0-9]+)\s", re.MULTILINE)
SYMBOLS = "ColourWars.pdb"
SYMBOLS_LABEL = "ColourWars.pdb (debug symbols for crash reports; not needed to play)"

NOTES = """\
**To play:** download the `{package}` below, right-click it and choose
**Extract All**, then run `ColourWars.exe` from the extracted folder. Windows 10/11
x64 with a Direct3D 11 graphics card; nothing else needs installing. Windows may
say "Windows protected your PC" because the game is unsigned: choose **More info**,
then **Run anyway**. `HOW-TO-PLAY.txt` in the ZIP has the rest.

`{symbols}` is the executable's debug symbols, kept so a crash report from this
version can be read. Players do not need it.
"""


def project_version(cmake_text: str) -> str | None:
    """The version project(ColourWars VERSION x.y.z ...) declares, or None."""
    match = PROJECT_VERSION.search(cmake_text)
    return match.group(1) if match else None


def package_name(version: str) -> str:
    """The Release ZIP's name, as CPack writes it (cmake/release_package.cmake)."""
    return f"ColourWars-{version}-windows-x64.zip"


def is_prerelease(version: str) -> bool:
    """REL#6: every 0.x release is published as a GitHub pre-release."""
    return version.split(".", 1)[0] == "0"


def release_notes(version: str) -> str:
    return NOTES.format(package=package_name(version), symbols=SYMBOLS)


def _git(args: list[str], cwd: Path | None) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], cwd=cwd, text=True, capture_output=True)


def check_tag(tag: str, commit: str, master: str, cwd: Path | None = None) -> tuple[str | None, list[str]]:
    """Return (version, problems) for publishing `tag` at `commit`.

    The version is the tag's own number when the tag is well formed, even if a
    problem is found, so a caller can still name what it refused.
    """
    problems = []
    ancestry = _git(["merge-base", "--is-ancestor", commit, master], cwd)
    if ancestry.returncode == 1:
        problems.append(f"{tag} is not on master")
    elif ancestry.returncode != 0:
        problems.append(f"Cannot compare {tag} with {master}: {ancestry.stderr.strip()}")

    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag):
        problems.append(f"Tag {tag} is not v<major>.<minor>.<patch>")
        return None, problems
    version = tag[1:]

    project = _git(["show", f"{commit}:CMakeLists.txt"], cwd)
    if project.returncode != 0:
        problems.append(f"Cannot read CMakeLists.txt at {commit}: {project.stderr.strip()}")
        return version, problems
    declared = project_version(project.stdout)
    if declared is None:
        problems.append("Cannot read project(ColourWars VERSION) from CMakeLists.txt")
    elif declared != version:
        problems.append(f"Tag {tag} does not match project(VERSION {declared})")
    return version, problems


def _write_outputs(values: dict[str, str]) -> None:
    path = os.environ.get("GITHUB_OUTPUT")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as output:
        for key, value in values.items():
            output.write(f"{key}={value}\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    check = commands.add_parser("check", help="refuse a tag off master or naming another version")
    check.add_argument("--tag", required=True)
    check.add_argument("--commit", required=True)
    check.add_argument("--master", default="origin/master")
    notes = commands.add_parser("notes", help="write the release notes")
    notes.add_argument("--version", required=True)
    notes.add_argument("--output", required=True, type=Path)
    arguments = parser.parse_args(argv)

    if arguments.command == "check":
        version, problems = check_tag(arguments.tag, arguments.commit, arguments.master)
        for problem in problems:
            print(f"::error::{problem}")
        if problems:
            return 1
        _write_outputs({"version": version, "package": package_name(version),
                        "symbols_asset": f"{SYMBOLS}#{SYMBOLS_LABEL}",
                        "prerelease": "true" if is_prerelease(version) else "false"})
        print(f"{arguments.tag} is on master and names project(VERSION {version}).")
        return 0

    arguments.output.write_text(release_notes(arguments.version), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
