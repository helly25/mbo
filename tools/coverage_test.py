#!/usr/bin/env python3
"""Tests for tools/coverage.py."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage as coverage_tool  # noqa: E402


class CoverageTest(unittest.TestCase):
    def test_parse_filter_and_measure(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "coverage.lcov"
            report.write_text(
                "SF:/workspace/mbo/strings/parse.cc\n"
                "FNDA:1,Good\nFNDA:0,Bad\n"
                "DA:10,1\nDA:11,0\n"
                "BRDA:10,0,0,1\nBRDA:10,0,1,0\nend_of_record\n"
                "SF:/workspace/mbo/strings/parse_test.cc\nDA:1,1\nend_of_record\n",
                encoding="utf-8",
            )
            files = coverage_tool.select_files(
                coverage_tool.parse_lcov(report),
                {"include": ["mbo/**"], "exclude": ["mbo/**/*_test.cc"]},
            )
            self.assertEqual(["mbo/strings/parse.cc"], list(files))
            self.assertEqual(
                {"covered": 1, "total": 2, "percent": 50.0},
                coverage_tool.counts(files)["lines"],
            )
            self.assertEqual(
                {"covered": 1, "total": 2, "percent": 50.0},
                coverage_tool.counts(files)["branches"],
            )

    def test_changed_lines_only_count_coverable_lines(self):
        files = {
            "mbo/a.cc": coverage_tool.FileCoverage(
                lines={10: 1, 11: 0, 12: 0}, branches=[(10, True), (11, False)]
            )
        }
        result = coverage_tool.counts(files, {"mbo/a.cc": {10, 11, 99}})
        self.assertEqual({"covered": 1, "total": 2, "percent": 50.0}, result["lines"])
        self.assertEqual({"covered": 1, "total": 2, "percent": 50.0}, result["branches"])

    def test_threshold_failure(self):
        measured = {
            "overall": {
                "lines": {"percent": 79.9},
                "functions": {"percent": 90.0},
                "branches": {"percent": None},
            }
        }
        self.assertEqual(
            ["overall lines: 79.9% < 80.0%"],
            coverage_tool.failures(measured, {"overall": {"lines": 80.0}}),
        )

    def test_category_minimum_overrides_shared_target(self):
        policy = {
            "minimum": {"lines": 85.0, "functions": 90.0, "branches": 57.0},
            "categories": {
                "inherited": {"include": ["mbo/inherited/**"]},
                "overridden": {
                    "include": ["mbo/overridden/**"],
                    "minimum": {"lines": 40.0, "functions": 50.0, "branches": 30.0},
                },
            },
        }
        minimums, targets = coverage_tool.thresholds(policy)
        self.assertEqual(policy["minimum"], minimums["inherited"])
        self.assertEqual(
            {"lines": 40.0, "functions": 50.0, "branches": 30.0},
            minimums["overridden"],
        )
        self.assertEqual(policy["minimum"], targets["overridden"])

    def test_patch_without_coverable_code_is_not_reported(self):
        empty = {
            "lines": {"covered": 0, "total": 0, "percent": None},
            "functions": {"covered": 0, "total": 0, "percent": None},
            "branches": {"covered": 0, "total": 0, "percent": None},
        }
        self.assertFalse(coverage_tool.has_coverage(empty, ("lines", "branches")))

        with_lines = dict(empty)
        with_lines["lines"] = {"covered": 0, "total": 1, "percent": 0.0}
        self.assertTrue(coverage_tool.has_coverage(with_lines, ("lines", "branches")))

    def test_markdown_shows_compact_line_coverage(self):
        measured = {
            "overall": {
                "lines": {"covered": 9, "total": 10, "percent": 90.0},
                "functions": {"covered": 1, "total": 2, "percent": 50.0},
                "branches": {"covered": 3, "total": 4, "percent": 75.0},
            },
            "okay": {
                "lines": {"covered": 9, "total": 10, "percent": 90.0},
                "functions": {"covered": 2, "total": 2, "percent": 100.0},
                "branches": {"covered": 3, "total": 4, "percent": 75.0},
            },
            "low": {
                "lines": {"covered": 9, "total": 10, "percent": 90.0},
                "functions": {"covered": 1, "total": 2, "percent": 50.0},
                "branches": {"covered": 3, "total": 4, "percent": 75.0},
            },
            "empty": {
                "lines": {"covered": 0, "total": 0, "percent": None},
                "functions": {"covered": 0, "total": 0, "percent": None},
                "branches": {"covered": 0, "total": 0, "percent": None},
            },
            "unconfigured": {
                "lines": {"covered": 0, "total": 0, "percent": None},
                "functions": {"covered": 0, "total": 0, "percent": None},
                "branches": {"covered": 0, "total": 0, "percent": None},
            },
        }
        minimums = {
            "overall": {"lines": 80.0, "functions": 60.0, "branches": 70.0},
            "okay": {"lines": 80.0, "functions": 60.0, "branches": 70.0},
            "low": {"lines": 80.0, "functions": 40.0, "branches": 70.0},
            "empty": {"lines": 80.0, "branches": 70.0},
        }
        targets = {
            category: {"lines": 80.0, "functions": 60.0, "branches": 70.0}
            for category in minimums
        }
        self.assertEqual(
            "| Category     | Status           |  Lines | Covered | Total | Functions | Covered | Total | Branches | Covered | Total |\n"
            "| ------------ | ---------------- | -----: | ------: | ----: | --------: | ------: | ----: | -------: | ------: | ----: |\n"
            "| overall      | **FAIL: F**      | 90.00% |       9 |    10 |    50.00% |       1 |     2 |   75.00% |       3 |     4 |\n"
            "| okay         | OK               | 90.00% |       9 |    10 |   100.00% |       2 |     2 |   75.00% |       3 |     4 |\n"
            "| low          | **LOW: F**       | 90.00% |       9 |    10 |    50.00% |       1 |     2 |   75.00% |       3 |     4 |\n"
            "| empty        | **NO DATA: L/B** |    n/a |       0 |     0 |       n/a |       0 |     0 |      n/a |       0 |     0 |\n"
            "| unconfigured | N/A              |    n/a |       0 |     0 |       n/a |       0 |     0 |      n/a |       0 |     0 |\n",
            coverage_tool.markdown(measured, minimums, targets),
        )


if __name__ == "__main__":
    unittest.main()
