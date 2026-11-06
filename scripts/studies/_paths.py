"""Keep study output outside tracked source, or under the ignored out folder."""

import os


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def inside_repo_but_not_out(path):
    """Refuse source paths and paths whose physical location cannot be checked."""
    # Resolve junctions/symlinks too: out/source-alias must not authorize a write
    # into source, and a checkout alias must not bypass the privacy guard.
    try:
        path = os.path.normcase(os.path.realpath(os.path.abspath(path)))
        repo = os.path.normcase(os.path.realpath(REPO_ROOT))
    except OSError:
        return True  # a locked/inaccessible target is not a verified output path
    try:
        if os.path.commonpath([path, repo]) != repo:
            return False
        return os.path.commonpath([path, os.path.join(repo, "out")]) != os.path.join(repo, "out")
    except ValueError:
        return False  # another drive is outside this repository
