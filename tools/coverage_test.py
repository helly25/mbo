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
            coverage_tool.failures(measured, {"minimum": {"lines": 80.0}}),
        )


if __name__ == "__main__":
    unittest.main()
