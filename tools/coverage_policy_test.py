#!/usr/bin/env python3
"""Tests for tools/coverage_policy.py."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage_policy  # noqa: E402


class CoveragePolicyTest(unittest.TestCase):
    def setUp(self):
        self.policy = {
            "minimum": {"lines": 90, "functions": 95, "branches": 80},
            "target": {"lines": 92, "functions": 97, "branches": 82},
            "enforce": "medium",
        }

    def test_rates_both_boundaries_and_enforces_the_selected_rating(self):
        medium = coverage_policy.resolve(self.policy)["lines"]
        high = coverage_policy.resolve({**self.policy, "enforce": {"lines": "high"}})["lines"]
        self.assertEqual("low", coverage_policy.rating(89.99, medium))
        self.assertEqual("medium", coverage_policy.rating(90.0, medium))
        self.assertEqual("high", coverage_policy.rating(92.0, medium))
        self.assertTrue(coverage_policy.passes(90.0, medium))
        self.assertFalse(coverage_policy.passes(90.0, high))
        self.assertTrue(coverage_policy.passes(92.0, high))

    def test_partial_override_inherits_and_can_strengthen_enforcement(self):
        parent = coverage_policy.resolve(self.policy)
        child = coverage_policy.validate_override(
            {"minimum": {"lines": 91}, "enforce": {"lines": "high"}}, parent, "program / matching"
        )
        self.assertEqual(91, child["lines"].minimum)
        self.assertEqual(92, child["lines"].target)
        self.assertEqual("high", child["lines"].enforce)
        self.assertEqual(parent["branches"], child["branches"])

    def test_raising_minimum_also_raises_inherited_target(self):
        parent = coverage_policy.resolve(self.policy)
        child = coverage_policy.validate_override({"minimum": {"lines": 98}}, parent, "extension")
        self.assertEqual(98, child["lines"].minimum)
        self.assertEqual(98, child["lines"].target)

    def test_weaker_override_requires_a_reason(self):
        parent = coverage_policy.resolve(self.policy)
        with self.assertRaisesRegex(ValueError, "requires a reason"):
            coverage_policy.validate_override({"minimum": {"branches": 75}}, parent, "new module")
        child = coverage_policy.validate_override(
            {"minimum": {"branches": 75}, "reason": "New module onboarding."}, parent, "new module"
        )
        self.assertEqual(75, child["branches"].minimum)

    def test_rejects_invalid_boundaries_and_enforcement(self):
        with self.assertRaisesRegex(ValueError, "minimum <= target"):
            coverage_policy.resolve({**self.policy, "target": {"branches": 79}})
        with self.assertRaisesRegex(ValueError, "medium or high"):
            coverage_policy.resolve({**self.policy, "enforce": {"branches": "low"}})

    def test_rejects_independent_presentation_bands(self):
        with self.assertRaisesRegex(ValueError, "not separate"):
            coverage_policy.policies({**self.policy, "bands": {}})

    def test_validates_baseline_tolerances(self):
        policy = {
            **self.policy,
            "baseline": {"maximum_drop": {"lines": 0.1, "functions": 0.2, "branches": 0}},
        }
        self.assertEqual(
            {"lines": 0.1, "functions": 0.2, "branches": 0.0},
            coverage_policy.baseline_tolerances(policy),
        )
        with self.assertRaisesRegex(ValueError, "missing metrics: branches"):
            coverage_policy.baseline_tolerances(
                {**self.policy, "baseline": {"maximum_drop": {"lines": 0.1, "functions": 0.1}}}
            )
        with self.assertRaisesRegex(ValueError, "between 0 and 100"):
            coverage_policy.baseline_tolerances(
                {
                    **self.policy,
                    "baseline": {
                        "maximum_drop": {"lines": -0.1, "functions": 0.1, "branches": 0.1}
                    },
                }
            )


if __name__ == "__main__":
    unittest.main()
