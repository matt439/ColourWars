"""Keep study output outside tracked source, or under the ignored out folder."""

import os


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def inside_repo_but_not_out(path):
    path = os.path.abspath(path)
    if os.path.normcase(os.path.splitdrive(path)[0]) != os.path.normcase(os.path.splitdrive(REPO_ROOT)[0]):
        return False  # do not query another (possibly locked) volume
    # Resolve junctions/symlinks too: out/source-alias must not authorize a write
    # into source, and a checkout alias must not bypass the privacy guard.
    path = os.path.normcase(os.path.realpath(path))
    repo = os.path.normcase(os.path.realpath(REPO_ROOT))
    try:
        if os.path.commonpath([path, repo]) != repo:
            return False
        return os.path.commonpath([path, os.path.join(repo, "out")]) != os.path.join(repo, "out")
    except ValueError:
        return False  # another drive is outside this repository
