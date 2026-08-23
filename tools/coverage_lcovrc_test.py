#!/usr/bin/env python3
"""Tests for tools/coverage_lcovrc.py."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage_lcovrc  # noqa: E402


class CoverageLcovrcTest(unittest.TestCase):
    def test_maps_policy_minimums_and_targets_to_metric_specific_color_bands(self):
        self.assertEqual(
            coverage_lcovrc.render(
                {
                    "minimum": {"lines": 88, "functions": 80, "branches": 60},
                    "target": {"lines": 90, "functions": 95, "branches": 82},
                }
            ),
            """genhtml_line_hi_limit = 90
genhtml_line_med_limit = 88
genhtml_function_hi_limit = 95
genhtml_function_med_limit = 80
genhtml_branch_hi_limit = 82
genhtml_branch_med_limit = 60
""",
        )

    def test_equal_minimum_and_target_form_one_boundary(self):
        rendered = coverage_lcovrc.render(
            {
                "minimum": {"lines": 90, "functions": 95, "branches": 82},
                "target": {"lines": 90, "functions": 95, "branches": 82},
            }
        )
        self.assertIn("genhtml_function_hi_limit = 95", rendered)
        self.assertIn("genhtml_function_med_limit = 95", rendered)

    def test_rejects_a_target_below_the_minimum(self):
        with self.assertRaisesRegex(ValueError, "branch coverage"):
            coverage_lcovrc.render(
                {
                    "minimum": {"lines": 88, "functions": 80, "branches": 60},
                    "target": {"branches": 59},
                }
            )


if __name__ == "__main__":
    unittest.main()
