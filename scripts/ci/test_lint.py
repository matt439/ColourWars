"""Exercise CI selection against real Git histories, without a game build."""

from pathlib import Path
import subprocess
import tempfile
import unittest

from lint import already_tested, check_changes


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
        for path in ("README.md", "game/audio/paint_exe/PROVENANCE.md"):
            with self.subTest(path=path):
                self.write(path)
                before = self.commit()
                self.git("rm", path)
                self.assertFalse(self.push(before, self.commit()))

    def test_deleting_ordinary_markdown_remains_docs_only(self) -> None:
        self.write("docs/obsolete.md")
        before = self.commit()
        self.git("rm", "docs/obsolete.md")
        self.assertTrue(self.push(before, self.commit()))

    def test_empty_diff_does_not_skip_builds(self) -> None:
        self.assertFalse(self.push(self.initial, self.initial))

    def test_unavailable_push_bases_and_other_events_use_full_ci(self) -> None:
        self.write("docs/latest.md")
        after = self.commit()
        for event_name, event in (
                ("push", {"before": "0" * 40, "after": after}),
                ("push", {"before": "f" * 40, "after": after}),
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
                ("push", {"after": "not a commit"})):
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


if __name__ == "__main__":
    unittest.main()
