"""Exercise the local Markdown link check against fixture documents."""

from pathlib import Path
import subprocess
import tempfile
import unittest

from check_doc_links import Checker, slug, tracked_markdown


class LinkChecks(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.write("docs/target.md", "# Target\n\n## Routes and trials (`POL#41`)\n\n"
                   "## Notes\n\n## Notes\n\n<a id=\"explicit\"></a>\n")

    def write(self, name: str, content: str) -> None:
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")

    def problems(self, name: str, content: str) -> list:
        self.write(name, content)
        return Checker(self.root).check([name])

    def test_slug_follows_github(self) -> None:
        self.assertEqual(slug("Shipping-bot routes and team trials (`POL#41`)"),
                         "shipping-bot-routes-and-team-trials-pol41")
        self.assertEqual(slug("Route trials (2026-10-20 provisional profile)"),
                         "route-trials-2026-10-20-provisional-profile")
        self.assertEqual(slug("[Linked](x.md) heading"), "linked-heading")

    def test_resolving_links_pass(self) -> None:
        self.write("docs/with space.md", "# Spaced\n")
        self.write("docs/notes#1.md", "Hash in the name.\n")
        self.assertEqual(self.problems("docs/source.md", "\n".join([
            "[file](target.md) [heading](target.md#routes-and-trials-pol41)",
            "[second](target.md#notes-1) [html](target.md#explicit) [self](#local)",
            "[root](/docs/target.md) ![image](target.md) [encoded](with%20space.md#spaced)",
            "[hash](notes#1.md) [web](https://example.invalid/missing.md) [mail](mailto:a@b.invalid)",
            "[evidence](../out/run/result.json) [folder](../docs)",
            "[ref]: target.md#notes",
            "## Local",
        ]) + "\n"), [])

    def test_broken_anchor_is_reported(self) -> None:
        problems = self.problems("docs/source.md", "[stale](target.md#retained-route-trials)\n")
        self.assertEqual(problems, ["docs/source.md:1: target.md#retained-route-trials: "
                                    "no heading #retained-route-trials in docs/target.md"])

    def test_missing_file_and_local_anchor_are_reported(self) -> None:
        problems = self.problems("docs/source.md", "[gone](retired.md)\n\n[here](#nowhere)\n")
        self.assertEqual(len(problems), 2)
        self.assertIn("source.md:1: retired.md: no file docs/retired.md", problems[0])
        self.assertIn("source.md:3: #nowhere: no heading #nowhere", problems[1])

    def test_code_is_not_a_link(self) -> None:
        self.assertEqual(self.problems("docs/source.md", "\n".join([
            "`[inline](missing.md)`",
            "```",
            "[fenced](missing.md)",
            "```",
        ]) + "\n"), [])

    def test_historical_record_keeps_retired_files_but_not_stale_anchors(self) -> None:
        problems = self.problems("docs/record.md", "\n".join([
            "# Record",
            "",
            "Historical workshop study, kept as written.",
            "",
            "[retired](style-kit.md) [stale](target.md#retained-route-trials)",
        ]) + "\n")
        self.assertEqual(len(problems), 1)
        self.assertIn("no heading #retained-route-trials", problems[0])

    def test_a_mention_of_history_is_not_a_declaration(self) -> None:
        problems = self.problems("docs/plan.md", "The historical audit is elsewhere.\n\n[gone](retired.md)\n")
        self.assertEqual(len(problems), 1)

    def test_tracked_markdown_skips_the_engine(self) -> None:
        def git(*args: str) -> None:
            subprocess.run(["git", *args], cwd=self.root, check=True, capture_output=True)
        git("init")
        self.write("external/labrador/README.md", "[gone](missing.md)\n")
        self.write("README.md", "Top.\n")
        git("add", "--all")
        self.assertEqual(sorted(tracked_markdown(self.root)), ["README.md", "docs/target.md"])


if __name__ == "__main__":
    unittest.main()
