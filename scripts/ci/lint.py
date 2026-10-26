"""Lint changed lines and decide whether a GitHub event needs the Windows jobs."""

import json
import os
from pathlib import PurePosixPath
import re
import subprocess


PACKAGED_DOCS = {"README.md", "game/audio/paint_exe/PROVENANCE.md"}
DOC_MEDIA = {".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp", ".mp4"}


def git(*args, cwd=None):
    return subprocess.check_output(["git", *args], cwd=cwd)


def comparison(event_name, event, cwd):
    if event_name == "push":
        base, head = event.get("before", ""), event.get("after", "")
    elif event_name == "pull_request":
        request = event["pull_request"]
        base, head = request["base"]["sha"], request["head"]["sha"]
    else:
        return None

    # New branches and unavailable history must never suppress the build.
    if not all(re.fullmatch(r"[0-9a-f]{40}", sha) and set(sha) != {"0"}
               for sha in (base, head)):
        return None
    try:
        for sha in (base, head):
            git("cat-file", "-e", f"{sha}^{{commit}}", cwd=cwd)
        if event_name == "pull_request":
            base = git("merge-base", base, head, cwd=cwd).decode("ascii").strip()
    except subprocess.CalledProcessError:
        print("Comparison history unavailable; running full CI.")
        return None
    return base, head


def check_changes(event_name, event, cwd=None):
    commits = comparison(event_name, event, cwd)
    if commits is None:
        # Manual runs still lint the latest commit and always run full CI.
        subprocess.run(["git", "show", "--format=", "--check", "HEAD", "--"],
                       cwd=cwd, check=True)
        return False

    base, head = commits
    # Disabling rename detection exposes both sides of code/doc moves.
    diff = ("diff", "--no-ext-diff", "--no-renames", "--ignore-submodules=none")
    subprocess.run(["git", *diff, "--check", base, head, "--"], cwd=cwd, check=True)
    paths = git(*diff, "--name-only", "-z", base, head, "--", cwd=cwd).split(b"\0")
    paths = [path.decode("utf-8", errors="surrogateescape") for path in paths if path]
    deleted = git(*diff, "--name-only", "--diff-filter=D", "-z", base, head,
                  "--", cwd=cwd).split(b"\0")
    if any(path.encode("utf-8") in deleted for path in PACKAGED_DOCS):
        return False

    def is_doc(path):
        suffix = PurePosixPath(path).suffix
        return suffix == ".md" or (path.startswith("docs/images/") and suffix in DOC_MEDIA)

    # Keep schema/exhibit code under docs/, runtime assets, scripts and pins on
    # full CI. An empty comparison is not evidence of a documentation change.
    return bool(paths) and all(is_doc(path) for path in paths)


def main():
    with open(os.environ["GITHUB_EVENT_PATH"], encoding="utf-8") as source:
        event = json.load(source)
    docs_only = check_changes(os.environ["GITHUB_EVENT_NAME"], event)
    value = str(docs_only).lower()
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        output.write(f"docs_only={value}\n")
    print("Documentation-only changes: lint passed; Windows jobs skipped."
          if docs_only else "Lint passed; running full Windows CI.")


if __name__ == "__main__":
    main()
