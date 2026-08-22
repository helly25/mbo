#!/usr/bin/env python3
"""Tests for tools/coverage.py."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage as coverage_tool  # noqa: E402


class CoverageTest(unittest.TestCase):
    def test_json_summary_contains_policy_and_coverable_patch(self):
        metric = {
            "lines": {"covered": 9, "total": 10, "percent": 90.0},
            "functions": {"covered": 1, "total": 1, "percent": 100.0},
            "branches": {"covered": 3, "total": 4, "percent": 75.0},
        }
        summary = coverage_tool.json_summary(
            {"overall": metric},
            {"overall": {"lines": 88}},
            {"overall": {"lines": 90}},
            metric,
        )
        self.assertEqual(summary["schema"], 1)
        self.assertEqual(summary["measurements"]["overall"], metric)
        self.assertEqual(summary["patch"], metric)

    def test_parse_filter_and_measure(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "coverage.lcov"
            report.write_text(
                "SF:/workspace/mbo/strings/parse.cc\n"
                "FN:9,Good\nFN:12,Bad\nFNDA:1,Good\nFNDA:0,Bad\n"
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

    def test_parse_excludes_marked_compiler_generated_branches(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "mbo/a.cc"
            source.parent.mkdir(parents=True)
            source.write_text(
                "if (runtime) {}  // LCOV_EXCL_BR_LINE\n"
                "quick_exit(1);  // LCOV_EXCL_LINE\n"
                "if (normal) {}\n",
                encoding="utf-8",
            )
            report = root / "coverage.lcov"
            report.write_text(
                "SF:mbo/a.cc\n"
                "BRDA:1,0,0,0\nBRDA:1,0,1,1\n"
                "DA:1,1\nDA:2,0\nDA:3,1\n"
                "BRDA:2,0,0,0\nBRDA:2,0,1,1\n"
                "BRDA:3,0,0,0\nBRDA:3,0,1,1\nend_of_record\n",
                encoding="utf-8",
            )
            files = coverage_tool.parse_lcov(report, root)
            self.assertEqual({1: 1, 3: 1}, files["mbo/a.cc"].lines)
            self.assertEqual([(3, False), (3, True)], files["mbo/a.cc"].branches)

    def test_parse_excludes_functions_at_marked_source_lines(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "mbo/a.cc"
            source.parent.mkdir(parents=True)
            source.write_text(
                "auto dispatcher = []<typename Value>(  // LCOV_EXCL_FUNC_LINE: generated dispatchers\n"
                "    const Value& value) { return value; };\n"
                "ordinary();\n",
                encoding="utf-8",
            )
            report = root / "coverage.lcov"
            report.write_text(
                "SF:mbo/a.cc\n"
                "FN:1,GeneratedOne\nFN:2,GeneratedTwo\nFN:3,Ordinary\n"
                "FNDA:1,GeneratedOne\nFNDA:0,GeneratedTwo\nFNDA:1,Ordinary\n"
                "end_of_record\n",
                encoding="utf-8",
            )
            files = coverage_tool.parse_lcov(report, root)
            self.assertEqual([(3, 1)], files["mbo/a.cc"].functions)

    def test_parse_merges_specializations_at_marked_source_lines(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "mbo/a.cc"
            source.parent.mkdir(parents=True)
            source.write_text(
                "template<typename T>  // LCOV_MERGE_FUNC_LINE: template specializations\n"
                "void merged(T value) { use(value); }\n"
                "void ordinary();\n",
                encoding="utf-8",
            )
            report = root / "coverage.lcov"
            report.write_text(
                "SF:mbo/a.cc\n"
                "FN:1,MergedOne\nFN:2,MergedTwo\nFN:3,Ordinary\n"
                "FNDA:0,MergedOne\nFNDA:2,MergedTwo\nFNDA:0,Ordinary\n"
                "end_of_record\n",
                encoding="utf-8",
            )
            files = coverage_tool.parse_lcov(report, root)
            self.assertEqual([(3, 0), (1, 2)], files["mbo/a.cc"].functions)
            self.assertEqual(
                {"covered": 1, "total": 2, "percent": 50.0},
                coverage_tool.counts(files)["functions"],
            )

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

    def test_category_minimum_only_overrides_named_metrics(self):
        policy = {
            "minimum": {"lines": 88.0, "functions": 80.0, "branches": 60.0},
            "categories": {
                "compile_time": {
                    "include": ["mbo/types/**"],
                    "minimum": {"branches": 47.0},
                },
            },
        }
        minimums, targets = coverage_tool.thresholds(policy)
        self.assertEqual(
            {"lines": 88.0, "functions": 80.0, "branches": 47.0},
            minimums["compile_time"],
        )
        self.assertEqual(policy["minimum"], targets["compile_time"])

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

    def test_uncovered_patch_locations_are_sorted_and_deduplicated(self):
        files = {
            "mbo/b.cc": coverage_tool.FileCoverage(
                lines={4: 0, 5: 1}, branches=[(4, False), (4, False), (5, True)]
            ),
            "mbo/a.cc": coverage_tool.FileCoverage(lines={2: 0}),
        }
        self.assertEqual(
            (["mbo/a.cc:2", "mbo/b.cc:4"], ["mbo/b.cc:4"]),
            coverage_tool.uncovered_patch_locations(
                files, {"mbo/a.cc": {2}, "mbo/b.cc": {4, 5}}
            ),
        )

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
