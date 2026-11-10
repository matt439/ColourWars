"""Exercise CI selection against real Git histories, without a game build."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from lint import (already_tested, check_changes, engine_pin_problem,
                  moved_engine_pin, packaged_markdown, tree_passed)


class ChangeChecks(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.repo = Path(self.scratch.name)
        self.git("init", "-b", "main")
        for key, value in {
            "user.name": "CI regression tests",
            "user.email": "ci-tests@example.invalid",
            "core.autocrlf": "false",
            "core.whitespace": "blank-at-eol,blank-at-eof,space-before-tab",
            "commit.gpgsign": "false",
        }.items():
            self.git("config", key, value)
        self.write("seed", "Initial repository.\n")
        self.write("cmake/release_package.cmake",
                   (Path(__file__).resolve().parents[2] / "cmake/release_package.cmake").read_text(encoding="utf-8"))
        self.initial = self.commit()

    def git(self, *args: str) -> str:
        return subprocess.run(["git", *args], cwd=self.repo, check=True,
            text=True, capture_output=True).stdout.strip()

    def write(self, name: str, content: str = "Document text.\n") -> None:
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")

    def commit(self) -> str:
        self.git("add", "--all")
        self.git("commit", "-m", "Regression fixture")
        return self.git("rev-parse", "HEAD")

    def push(self, before: str, after: str) -> bool:
        return check_changes("push", {"before": before, "after": after}, cwd=self.repo)

    def pull_request(self, base: str, head: str) -> bool:
        return check_changes("pull_request", {"pull_request": {
            "base": {"sha": base}, "head": {"sha": head}}}, cwd=self.repo)

    def test_markdown_and_document_media_are_docs_only(self) -> None:
        for path in ("README.md", "AGENTS.md", "TODO.md", "docs/guide.md",
                "game/audio/paint_exe/PROVENANCE.md", "scripts/README.md",
                "docs/images/example.png", "docs/images/nested/example.jpg",
                "docs/images/example.jpeg", "docs/images/example.gif",
                "docs/images/example.svg", "docs/images/example.webp",
                "docs/images/example.mp4"):
            self.write(path)
        self.assertTrue(self.push(self.initial, self.commit()))

    def test_push_includes_every_commit_in_range(self) -> None:
        self.write("docs/first.md")
        self.commit()
        self.write("docs/second.md")
        self.assertTrue(self.push(self.initial, self.commit()))

    def test_earlier_code_commit_keeps_later_doc_push_on_full_ci(self) -> None:
        self.write("game/example.cpp", "int example = 1;\n")
        self.commit()
        self.write("docs/latest.md")
        self.assertFalse(self.push(self.initial, self.commit()))

    def test_non_docs_paths_keep_full_ci(self) -> None:
        for path in ("CMakeLists.txt", "docs/schema.json", "docs/example.cpp",
                "scripts/example.py", ".github/workflows/ci.yml",
                "game/content/tuning.json", "game/content/texture.png",
                "docs/images/example.py", "LICENSE", "NOTICE", "docs/upper.MD"):
            with self.subTest(path=path):
                before = self.git("rev-parse", "HEAD")
                self.write(path)
                self.assertFalse(self.push(before, self.commit()))

    def test_submodule_revision_keeps_full_ci(self) -> None:
        self.git("update-index", "--add", "--cacheinfo",
            f"160000,{self.initial},external/labrador")
        self.git("commit", "-m", "Add a gitlink")
        self.assertFalse(self.push(self.initial, self.git("rev-parse", "HEAD")))

    def test_pull_request_excludes_diverged_base_changes(self) -> None:
        self.git("checkout", "-b", "feature")
        self.write("docs/first.md")
        self.commit()
        self.write("docs/second.md")
        head = self.commit()
        self.git("checkout", "main")
        self.write("game/base_only.cpp", "int base_only = 1;\n")
        base = self.commit()
        self.assertTrue(self.pull_request(base, head))

    def test_pull_request_includes_earlier_source_commits(self) -> None:
        self.git("checkout", "-b", "feature")
        self.write("game/example.cpp", "int example = 1;\n")
        self.commit()
        self.write("docs/latest.md")
        head = self.commit()
        self.git("checkout", "main")
        self.write("docs/base_only.md")
        self.assertFalse(self.pull_request(self.commit(), head))

    def test_renames_consider_both_original_and_destination(self) -> None:
        for old, new, docs_only in (
                ("game/renamed.cpp", "docs/renamed.md", False),
                ("docs/moved.md", "game/moved.cpp", False),
                ("docs/before.md", "docs/after.md", True)):
            with self.subTest(old=old, new=new):
                self.write(old)
                before = self.commit()
                (self.repo / new).parent.mkdir(parents=True, exist_ok=True)
                self.git("mv", old, new)
                self.assertEqual(self.push(before, self.commit()), docs_only)

    def test_deleting_packaged_markdown_requires_full_ci(self) -> None:
        for path in ("game/audio/paint_exe/PROVENANCE.md",):
            with self.subTest(path=path):
                self.write(path)
                before = self.commit()
                self.git("rm", path)
                self.assertFalse(self.push(before, self.commit()))

    def test_new_packaged_markdown_is_derived_from_install_rule(self) -> None:
        rule = self.repo / "cmake/release_package.cmake"
        rule.write_text(rule.read_text(encoding="utf-8") +
                        '\ninstall(FILES "${PROJECT_SOURCE_DIR}/docs/new-license.md" DESTINATION licenses)\n',
                        encoding="utf-8")
        self.assertIn("docs/new-license.md", packaged_markdown(self.repo))
        self.write("docs/new-license.md")
        before = self.commit()
        self.git("rm", "docs/new-license.md")
        self.assertFalse(self.push(before, self.commit()))

    def test_deleting_ordinary_markdown_remains_docs_only(self) -> None:
        # The player guide replaces the developer README in the Release ZIP.
        for path in ("docs/obsolete.md", "README.md"):
            with self.subTest(path=path):
                self.write(path)
                before = self.commit()
                self.git("rm", path)
                self.assertTrue(self.push(before, self.commit()))

    def test_empty_diff_does_not_skip_builds(self) -> None:
        self.assertFalse(self.push(self.initial, self.initial))

    def test_unavailable_push_bases_and_other_events_use_full_ci(self) -> None:
        self.write("docs/latest.md")
        after = self.commit()
        for event_name, event in (
                ("push", {"before": "0" * 40, "after": after}),
                ("push", {"before": "f" * 40, "after": after}),
                ("push", {"ref": "refs/tags/v0.1.0", "before": self.initial,
                          "after": after}),
                ("workflow_dispatch", {}),
                ("schedule", {})):
            with self.subTest(event_name=event_name, event=event):
                self.assertFalse(check_changes(event_name, event, cwd=self.repo))

    def test_whitespace_errors_in_any_push_commit_fail_lint(self) -> None:
        self.write("docs/first.md", "Trailing whitespace. \n")
        self.commit()
        self.write("docs/latest.md")
        with self.assertRaises(subprocess.CalledProcessError):
            self.push(self.initial, self.commit())

    def test_conflict_markers_fail_lint(self) -> None:
        self.write("docs/conflict.md", "<<<<<<< ours\nOne.\n=======\nTwo.\n>>>>>>> theirs\n")
        with self.assertRaises(subprocess.CalledProcessError):
            self.push(self.initial, self.commit())

    def test_pull_request_lints_the_cumulative_source_diff(self) -> None:
        self.git("checkout", "-b", "feature")
        self.write("docs/first.md", "Trailing whitespace. \n")
        self.commit()
        self.write("docs/latest.md")
        head = self.commit()
        with self.assertRaises(subprocess.CalledProcessError):
            self.pull_request(self.initial, head)

    def tree(self, commit: str) -> str:
        return self.git("rev-parse", f"{commit}^{{tree}}")

    def reuses(self, after: str, passed) -> bool:
        return already_tested("push", {"before": self.initial, "after": after},
                              cwd=self.repo, passed=passed)

    def test_only_a_tree_a_pull_request_passed_skips_the_build(self) -> None:
        self.git("checkout", "-b", "feature")
        self.write("game/example.cpp", "int example = 1;\n")
        head = self.commit()
        recorded = {self.tree(head)}
        self.git("checkout", "main")
        self.git("merge", "--no-ff", "--no-edit", "feature")
        merged = self.git("rev-parse", "HEAD")
        asked = []
        self.assertTrue(self.reuses(merged, lambda tree: asked.append(tree) or True))
        self.assertEqual(asked, [self.tree(head)])
        self.assertTrue(self.reuses(merged, recorded.__contains__))
        self.assertFalse(self.reuses(merged, set().__contains__))
        # Changes made in or after the merge are a tree no run built.
        self.write("game/amended.cpp", "int amended = 1;\n")
        self.git("add", "--all")
        self.git("commit", "--amend", "--no-edit")
        self.assertFalse(self.reuses(self.git("rev-parse", "HEAD"), recorded.__contains__))
        self.git("reset", "--hard", self.initial)
        self.write("game/other.cpp", "int other = 1;\n")
        self.commit()
        self.git("merge", "--no-ff", "--no-edit", "feature")
        self.assertFalse(self.reuses(self.git("rev-parse", "HEAD"), recorded.__contains__))

    def test_other_events_and_unknown_commits_keep_full_ci(self) -> None:
        for event_name, event in (
                ("pull_request", {"after": self.initial}),
                ("workflow_dispatch", {}),
                ("push", {"after": "0" * 40}),
                ("push", {"after": "f" * 40}),
                ("push", {"after": "not a commit"}),
                ("push", {"ref": "refs/tags/v0.1.0", "after": self.initial})):
            with self.subTest(event_name=event_name, event=event):
                self.assertFalse(already_tested(event_name, event, cwd=self.repo,
                                                passed=lambda tree: True))

    def test_unanswered_record_query_keeps_full_ci(self) -> None:
        def unavailable(tree: str) -> bool:
            raise OSError("API unreachable")
        self.assertFalse(self.reuses(self.initial, unavailable))

    def test_full_ci_fallbacks_still_lint_head(self) -> None:
        self.write("docs/latest.md", "Trailing whitespace. \n")
        after = self.commit()
        for event_name, event in (
                ("push", {"before": "0" * 40, "after": after}),
                ("push", {"before": "f" * 40, "after": after}),
                ("workflow_dispatch", {}),
                ("schedule", {})):
            with self.subTest(event_name=event_name, event=event):
                with self.assertRaises(subprocess.CalledProcessError):
                    check_changes(event_name, event, cwd=self.repo)

    def test_a_pull_request_moving_the_pin_is_reported(self) -> None:
        def pull_request(head: str) -> dict:
            return {"pull_request": {"base": {"sha": self.initial}, "head": {"sha": head}}}

        self.git("update-index", "--add", "--cacheinfo",
                 f"160000,{'a' * 40},external/labrador")
        self.git("commit", "-m", "pin")
        pinned = self.git("rev-parse", "HEAD")
        self.assertEqual(moved_engine_pin("pull_request", pull_request(pinned), cwd=self.repo),
                         "a" * 40)
        self.assertIsNone(moved_engine_pin("push", {"before": self.initial, "after": pinned},
                                           cwd=self.repo))
        self.write("game/example.cpp", "int example = 1;\n")
        later = self.commit()
        self.initial = pinned
        self.assertIsNone(moved_engine_pin("pull_request", pull_request(later), cwd=self.repo))

    def test_unusable_history_stops_the_pin_check_rather_than_passing_it(self) -> None:
        event = {"pull_request": {"base": {"sha": self.initial}, "head": {"sha": "f" * 40}}}
        with self.assertRaises(subprocess.CalledProcessError):
            moved_engine_pin("pull_request", event, cwd=self.repo)


class TreeRecords(unittest.TestCase):
    NAME = "passed-tree-abc"

    def asks(self, artifact: dict | None = None, **run) -> bool:
        record = {"name": self.NAME, "expired": False, "workflow_run": {"id": 7}}
        record.update(artifact or {})
        answer = {"event": "pull_request", "path": ".github/workflows/ci.yml",
                  "conclusion": "success",
                  "head_repository": {"full_name": "owner/repo"}}
        answer.update(run)

        def query(path: str, **parameters) -> dict:
            if path == "actions/artifacts":
                self.assertEqual(parameters, {"name": self.NAME})
                return {"artifacts": [record]}
            self.assertEqual(path, "actions/runs/7")
            return answer

        environment = {"GITHUB_REPOSITORY": "owner/repo",
                       "GITHUB_WORKFLOW_REF": "owner/repo/.github/workflows/ci.yml@refs/heads/main"}
        with mock.patch.dict(os.environ, environment):
            return tree_passed("abc", query=query)

    def test_this_repository_s_passed_pull_request_run_counts(self) -> None:
        self.assertTrue(self.asks())

    def test_anything_less_does_not(self) -> None:
        for artifact, run in (
                ({"expired": True}, {}),
                ({"name": "passed-tree-abcd"}, {}),
                ({}, {"event": "push"}),
                ({}, {"path": ".github/workflows/other.yml"}),
                ({}, {"conclusion": "failure"}),
                ({}, {"head_repository": {"full_name": "fork/repo"}})):
            with self.subTest(artifact=artifact, run=run):
                self.assertFalse(self.asks(artifact, **run))

    def test_a_run_with_no_head_repository_raises_for_the_full_ci_fallback(self) -> None:
        with self.assertRaises(TypeError):
            self.asks(head_repository=None)


class EnginePins(unittest.TestCase):
    PIN = "b" * 40

    def problem(self, status: str = "behind", runs: list | None = None) -> str | None:
        passed = {"path": ".github/workflows/ci.yml", "conclusion": "success"}

        def query(path: str, repository: str, **parameters) -> dict:
            self.assertEqual(repository, "matt439/Labrador")
            if path == f"compare/master...{self.PIN}":
                return {"status": status}
            self.assertEqual((path, parameters), (
                "actions/runs", {"head_sha": self.PIN, "event": "push"}))
            return {"workflow_runs": [passed] if runs is None else runs}

        return engine_pin_problem(self.PIN, query=query)

    def test_a_passed_pin_on_master_is_accepted(self) -> None:
        self.assertIsNone(self.problem("behind"))
        self.assertIsNone(self.problem("identical"))

    def test_a_pin_off_master_is_refused(self) -> None:
        for status in ("ahead", "diverged"):
            with self.subTest(status=status):
                self.assertIn("not on Labrador's master", self.problem(status))

    def test_a_pin_without_a_passed_engine_run_is_refused(self) -> None:
        for runs in ([],
                     [{"path": ".github/workflows/ci.yml", "conclusion": "failure"}],
                     [{"path": ".github/workflows/ci.yml", "conclusion": None}],
                     [{"path": ".github/workflows/website.yml", "conclusion": "success"}]):
            with self.subTest(runs=runs):
                self.assertIn("no passed push run", self.problem(runs=runs))

    def test_an_unanswered_query_refuses_rather_than_passes(self) -> None:
        def unavailable(path: str, **parameters) -> dict:
            raise OSError("API unreachable")
        self.assertIn("could not check", engine_pin_problem(self.PIN, query=unavailable))


if __name__ == "__main__":
    unittest.main()
