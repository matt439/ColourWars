"""Lint changed lines and decide whether a GitHub event needs the Windows jobs."""

import json
import os
from pathlib import PurePosixPath
import re
import subprocess
import urllib.parse
import urllib.request


PACKAGED_DOCS = {"README.md", "game/audio/paint_exe/PROVENANCE.md"}
DOC_MEDIA = {".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp", ".mp4"}


def git(*args, cwd=None):
    return subprocess.check_output(["git", *args], cwd=cwd)


def is_tag(event):
    """A tag push publishes a release, so it always builds in full."""
    return event.get("ref", "").startswith("refs/tags/")


def comparison(event_name, event, cwd):
    if event_name == "push" and is_tag(event):
        return None
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


def github(path, **query):
    request = urllib.request.Request(
        f"{os.environ['GITHUB_API_URL']}/repos/{os.environ['GITHUB_REPOSITORY']}"
        f"/{path}?{urllib.parse.urlencode(query)}",
        headers={"Accept": "application/vnd.github+json",
                 "Authorization": f"Bearer {os.environ['GITHUB_TOKEN']}"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def tree_passed(tree):
    """Whether a pull_request run of this workflow built tree and passed.

    The record job uploads passed-tree-<tree> only after both Windows jobs
    passed on the commit they checked out, so its name is the evidence.
    """
    # GITHUB_WORKFLOW_REF is owner/repo/.github/workflows/ci.yml@ref.
    workflow = os.environ["GITHUB_WORKFLOW_REF"].split("@")[0].split("/", 2)[2]
    name = f"passed-tree-{tree}"
    for artifact in github("actions/artifacts", name=name)["artifacts"]:
        if artifact["name"] != name or artifact["expired"]:
            continue
        run = github(f"actions/runs/{artifact['workflow_run']['id']}")
        if (run["event"] == "pull_request" and run["path"] == workflow
                and run["conclusion"] == "success"):
            return True
    return False


def head_tree(cwd=None):
    return git("rev-parse", "HEAD^{tree}", cwd=cwd).decode("ascii").strip()


def already_tested(event_name, event, cwd=None, passed=tree_passed):
    """Whether a push carries a tree that a pull request's CI already passed.

    A merge commit of a branch that contains the old tip is the branch's own
    tree, which its pull request built. Anything else is a new tree.
    """
    if event_name != "push" or is_tag(event):
        return False
    after = event.get("after", "")
    if not re.fullmatch(r"[0-9a-f]{40}", after) or set(after) == {"0"}:
        return False
    try:
        tree = git("rev-parse", f"{after}^{{tree}}", cwd=cwd).decode("ascii").strip()
    except subprocess.CalledProcessError:
        return False
    # An unanswered query must never suppress the build.
    try:
        return passed(tree)
    except Exception as error:
        print(f"Pull request records unavailable ({error}); running full CI.")
        return False


def main():
    with open(os.environ["GITHUB_EVENT_PATH"], encoding="utf-8") as source:
        event = json.load(source)
    event_name = os.environ["GITHUB_EVENT_NAME"]
    docs_only = check_changes(event_name, event)
    tested = not docs_only and already_tested(event_name, event)
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        output.write(f"skip_windows={str(docs_only or tested).lower()}\n")
        # The tree the Windows jobs will check out, for the record job.
        output.write(f"tree={head_tree()}\n")
    if docs_only:
        print("Documentation-only changes: lint passed; Windows jobs skipped.")
    elif tested:
        print("A pull request's CI already passed this exact tree: "
              "lint passed; Windows jobs skipped.")
    else:
        print("Lint passed; running full Windows CI.")


if __name__ == "__main__":
    main()
