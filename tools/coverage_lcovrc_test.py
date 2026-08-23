#!/usr/bin/env python3
"""Tests for tools/coverage_lcovrc.py."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage_lcovrc


class CoverageLcovrcTest(unittest.TestCase):
    def test_maps_the_global_enforcement_boundaries_to_lcov_colors(self):
        self.assertEqual(
            coverage_lcovrc.render(
                {
                    "minimum": {"lines": 90, "functions": 95, "branches": 80},
                    "target": {"lines": 92, "functions": 95, "branches": 82},
                    "enforce": "medium",
                }
            ),
            """genhtml_line_hi_limit = 92
genhtml_line_med_limit = 90
genhtml_function_hi_limit = 95
genhtml_function_med_limit = 95
genhtml_branch_hi_limit = 82
genhtml_branch_med_limit = 80
""",
        )

    def test_rejects_an_invalid_minimum(self):
        with self.assertRaisesRegex(ValueError, "branch coverage"):
            coverage_lcovrc.render(
                {
                    "minimum": {"lines": 90, "functions": 90, "branches": 101},
                }
            )

    def test_rejects_a_target_below_the_minimum(self):
        with self.assertRaisesRegex(ValueError, "branch coverage"):
            coverage_lcovrc.render(
                {
                    "minimum": {"lines": 90, "functions": 95, "branches": 80},
                    "target": {"branches": 79},
                }
            )

if __name__ == "__main__":
    unittest.main()
