"""Keep study output outside tracked source, or under the ignored out folder."""

import os
from pathlib import Path


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def _realpath_allow_missing(path):
    allow_missing = getattr(os.path, "ALLOW_MISSING", None)
    if allow_missing is not None:
        return os.path.realpath(path, strict=allow_missing)

    # Python before 3.14 has no ALLOW_MISSING. Validate the deepest existing
    # ancestor strictly first: ordinary realpath otherwise swallows permission
    # and sharing errors as well as missing paths. The final non-strict pass
    # still follows a dangling link into its missing target, so its suffix is
    # classified at the physical destination rather than the link's spelling.
    ancestor = Path(path)
    while True:
        try:
            ancestor.resolve(strict=True)
            break
        except FileNotFoundError:
            parent = ancestor.parent
            if parent == ancestor:
                raise
            ancestor = parent
    return os.path.realpath(path)


def inside_repo_but_not_out(path):
    """Refuse source paths and paths whose physical location cannot be checked."""
    # Resolve junctions/symlinks too: out/source-alias must not authorize a write
    # into source, and a checkout alias must not bypass the privacy guard.
    try:
        path = os.path.normcase(_realpath_allow_missing(path))
        repo = os.path.normcase(str(Path(REPO_ROOT).resolve(strict=True)))
    except OSError:
        return True  # a locked/inaccessible target is not a verified output path
    try:
        if os.path.commonpath([path, repo]) != repo:
            return False
        return os.path.commonpath([path, os.path.join(repo, "out")]) != os.path.join(repo, "out")
    except ValueError:
        return False  # another drive is outside this repository
