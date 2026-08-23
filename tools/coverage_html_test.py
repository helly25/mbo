#!/usr/bin/env python3
"""Tests for tools/coverage_html.py."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage_html  # noqa: E402


_HEADER = """<html><head></head><body>
          <table width="100%" border=0 cellspacing=0 cellpadding=0>
            <tr><td class="title">LCOV - code coverage report</td></tr>
            <tr><td class="ruler"><img src="{prefix}glass.png" width=3 height=3 alt=""></td></tr>
            <tr><td><table><tr><td>Coverage table</td></tr></table></td></tr>
            <tr><td class="ruler"><img src="{prefix}glass.png" width=3 height=3 alt=""></td></tr>
          </table>
          <p id="content">Report content</p>
</body></html>
"""


class CoverageHtmlTest(unittest.TestCase):
    def setUp(self):
        self.policy = {
            "minimum": {"lines": 88, "functions": 80, "branches": 60},
            "target": {"lines": 90, "functions": 95, "branches": 82},
        }

    def test_moves_policy_below_header_table_and_adds_root_navigation(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory)
            page = report / "index.html"
            page.write_text(_HEADER.format(prefix=""), encoding="utf-8")
            coverage_html.apply(report, self.policy, "pr/360")
            rendered = page.read_text(encoding="utf-8")
            self.assertIn('<a href="../index.html">Report overview</a>', rendered)
            self.assertIn('<a href="../../../index.html">All coverage reports</a>', rendered)
            self.assertLess(rendered.index("mboNavigation"), rendered.index("Coverage table"))
            self.assertLess(rendered.index("Coverage table"), rendered.index("Coverage policy:"))
            self.assertLess(rendered.index("Coverage policy:"), rendered.index('id="content"'))
            self.assertIn("medium: &gt;= 88 % and &lt; 90 %", rendered)
            self.assertIn("medium: &gt;= 80 % and &lt; 95 %", rendered)
            self.assertIn("medium: &gt;= 60 % and &lt; 82 %", rendered)

    def test_equal_floor_and_target_omits_medium_band(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory)
            page = report / "index.html"
            page.write_text(_HEADER.format(prefix=""), encoding="utf-8")
            policy = {
                "minimum": {"lines": 90, "functions": 95, "branches": 82},
                "target": {"lines": 90, "functions": 95, "branches": 82},
            }
            coverage_html.apply(report, policy, "main")
            rendered = page.read_text(encoding="utf-8")
            self.assertNotIn("medium:", rendered)

    def test_nested_page_links_reach_the_same_overview_and_index(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory)
            page = report / "container" / "mbo" / "source.gcov.html"
            page.parent.mkdir(parents=True)
            page.write_text(_HEADER.format(prefix="../../"), encoding="utf-8")
            coverage_html.apply(report, self.policy, "main")
            rendered = page.read_text(encoding="utf-8")
            self.assertIn('<a href="../../../index.html">Report overview</a>', rendered)
            self.assertIn('<a href="../../../../index.html">All coverage reports</a>', rendered)

    def test_reprocessing_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory)
            page = report / "index.html"
            page.write_text(_HEADER.format(prefix=""), encoding="utf-8")
            coverage_html.apply(report, self.policy, "main")
            coverage_html.apply(report, self.policy, "main")
            rendered = page.read_text(encoding="utf-8")
            self.assertEqual(1, rendered.count('class="mboNavigation"'))
            self.assertEqual(1, rendered.count('class="mboPolicy"'))
            self.assertEqual(1, rendered.count("Coverage policy:"))

    def test_rejects_an_unrecognized_genhtml_page(self):
        with tempfile.TemporaryDirectory() as directory:
            page = Path(directory) / "index.html"
            page.write_text("not a genhtml page", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "header anchors not found"):
                coverage_html.apply(Path(directory), self.policy, "main")

    def test_rejects_an_unsafe_target(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "invalid coverage target"):
                coverage_html.apply(Path(directory), self.policy, "../main")


if __name__ == "__main__":
    unittest.main()
