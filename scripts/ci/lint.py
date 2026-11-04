"""Lint changed lines and decide whether a GitHub event needs the Windows jobs."""

import json
import os
from pathlib import PurePosixPath
import re
import subprocess
import urllib.parse
import urllib.request


ENGINE_PATH = "external/labrador"
ENGINE_REPOSITORY = "matt439/Labrador"
ENGINE_WORKFLOW = ".github/workflows/ci.yml"
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


def github(path, repository=None, **query):
    repository = repository or os.environ["GITHUB_REPOSITORY"]
    request = urllib.request.Request(
        f"{os.environ['GITHUB_API_URL']}/repos/{repository}"
        f"/{path}?{urllib.parse.urlencode(query)}",
        headers={"Accept": "application/vnd.github+json",
                 "Authorization": f"Bearer {os.environ['GITHUB_TOKEN']}"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def tree_passed(tree, query=github):
    """Whether a pull_request run of this workflow built tree and passed.

    The record job uploads passed-tree-<tree> only after every Windows job
    passed on the commit it checked out, so its name is the evidence. A fork's
    run could upload that name from its own ci.yml, so it must be this
    repository's run.
    """
    # GITHUB_WORKFLOW_REF is owner/repo/.github/workflows/ci.yml@ref.
    workflow = os.environ["GITHUB_WORKFLOW_REF"].split("@")[0].split("/", 2)[2]
    name = f"passed-tree-{tree}"
    for artifact in query("actions/artifacts", name=name)["artifacts"]:
        if artifact["name"] != name or artifact["expired"]:
            continue
        run = query(f"actions/runs/{artifact['workflow_run']['id']}")
        if (run["event"] == "pull_request" and run["path"] == workflow
                and run["conclusion"] == "success"
                and run["head_repository"]["full_name"] == os.environ["GITHUB_REPOSITORY"]):
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


def engine_pin(commit, cwd=None):
    entry = git("ls-tree", commit, "--", ENGINE_PATH, cwd=cwd).decode("ascii").split()
    return entry[2] if entry else None


def moved_engine_pin(event_name, event, cwd=None):
    """The engine revision a pull request moves the pin to, if it moves it."""
    if event_name != "pull_request":
        return None
    # Full CI is the safe answer to unusable history, but it runs no engine
    # tests, so here the event's own commits stand in, and a failed lookup stops.
    request = event["pull_request"]
    base, head = (comparison(event_name, event, cwd)
                  or (request["base"]["sha"], request["head"]["sha"]))
    pin = engine_pin(head, cwd)
    return pin if pin and pin != engine_pin(base, cwd) else None


def engine_pin_problem(pin, query=github):
    """Why this repository should not merge pin, or None.

    This repository runs no engine tests, so what it pins must already be on
    the engine's master and have passed the engine's own CI there. The run's
    conclusion is the evidence, not its jobs: the engine skips its Windows jobs
    for a tree it has already built.
    """
    try:
        compared = query(f"compare/master...{pin}", repository=ENGINE_REPOSITORY)
        if compared["status"] not in ("behind", "identical"):
            return f"{pin} is not on Labrador's master ({compared['status']})."
        runs = query("actions/runs", repository=ENGINE_REPOSITORY,
                     head_sha=pin, event="push")["workflow_runs"]
    except Exception as error:
        return f"could not check {pin} against Labrador ({error})."
    if not any(run["path"] == ENGINE_WORKFLOW and run["conclusion"] == "success"
               for run in runs):
        return (f"Labrador's CI has no passed push run for {pin}. Pin the merge "
                "commit or tag: a commit that was never master's tip never gets one.")
    return None


def main():
    with open(os.environ["GITHUB_EVENT_PATH"], encoding="utf-8") as source:
        event = json.load(source)
    event_name = os.environ["GITHUB_EVENT_NAME"]
    docs_only = check_changes(event_name, event)
    pin = moved_engine_pin(event_name, event)
    if pin:
        problem = engine_pin_problem(pin)
        if problem:
            raise SystemExit(f"::error::Engine pin: {problem}")
        print(f"Engine pin {pin} is on Labrador's master and passed its CI.")
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
