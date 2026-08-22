#!/usr/bin/env python3
"""Tests for tools/coverage_index.py."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import coverage_index  # noqa: E402


def _summary(percent: float) -> dict:
    metric = {"covered": 9, "total": 10, "percent": percent}
    return {
        "measurements": {"overall": {name: metric for name in ("lines", "functions", "branches")}},
        "minimums": {"overall": {name: 80 for name in ("lines", "functions", "branches")}},
        "targets": {"overall": {name: 90 for name in ("lines", "functions", "branches")}},
    }


class CoverageIndexTest(unittest.TestCase):
    def test_report_contains_policy_table_and_lcov_link(self):
        rendered = coverage_index.render_report(_summary(95.0), "pr/42")
        self.assertIn("mbo coverage: pr/42", rendered)
        self.assertIn("95.00%", rendered)
        self.assertIn('href="lcov/"', rendered)
        self.assertIn('href="../../"', rendered)

    def test_site_shows_all_metrics_with_main_first_and_numeric_sorting(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for report in ("main", "pr/9", "pr/42", "tag/0.9.0", "tag/0.10.0"):
                target = root / report
                target.mkdir(parents=True)
                (target / "index.html").touch()
                (target / "coverage-summary.json").write_text(json.dumps(_summary(95.0)))
                metadata = coverage_index.report_metadata(
                    _summary(95.0),
                    report,
                    "2026-08-22T09:59:00Z",
                    "2026-08-22T10:00:00Z",
                    "2026-08-22T10:01:00Z",
                    1,
                    1,
                    "abc",
                )
                (target / "coverage-meta.json").write_text(json.dumps(metadata))

            rendered = coverage_index.render_site(root)

            self.assertIn("PR 42", rendered)
            self.assertIn("PR 9", rendered)
            self.assertIn("release 0.10.0", rendered)
            self.assertIn("release 0.9.0", rendered)
            self.assertIn(
                'href="https://github.com/helly25/mbo/releases/tag/v0.10.0">release v0.10.0</a>',
                rendered,
            )
            self.assertIn('href="https://github.com/helly25/mbo/pull/42">PR #42</a>', rendered)
            self.assertIn("2026-08-22 10:01:00 UTC", rendered)
            self.assertIn('href="https://github.com/helly25/mbo/commit/abc"><code>abc</code></a>', rendered)
            self.assertIn('href="https://github.com/helly25/mbo/actions/runs/1">run 1</a>', rendered)
            self.assertIn("font-variant-numeric: tabular-nums", rendered)
            self.assertLess(rendered.index('href="main/"'), rendered.index('href="tag/0.10.0/"'))
            self.assertLess(rendered.index('href="tag/0.10.0/"'), rendered.index('href="tag/0.9.0/"'))
            self.assertLess(rendered.index('href="tag/0.9.0/"'), rendered.index('href="pr/42/"'))
            self.assertLess(rendered.index('href="pr/42/"'), rendered.index('href="pr/9/"'))
            self.assertNotIn("<ul>", rendered)

    def test_empty_site_says_no_reports_are_available(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertIn("No coverage reports are available", coverage_index.render_site(Path(directory)))

    def test_legacy_report_does_not_link_placeholder_metadata(self):
        metadata = coverage_index.report_metadata(
            _summary(95.0),
            "pr/9",
            "1970-01-01T00:00:00Z",
            "1970-01-01T00:00:00Z",
            "1970-01-01T00:00:00Z",
            0,
            0,
            "legacy",
        )
        row = coverage_index._short_row(metadata)
        self.assertIn('href="https://github.com/helly25/mbo/pull/9">PR #9</a>', row)
        self.assertNotIn("actions/runs/0", row)
        self.assertNotIn("commit/legacy", row)

    def test_newer_report_wins_even_when_an_older_run_finishes_later(self):
        summary = _summary(95.0)
        old = coverage_index.report_metadata(
            summary,
            "pr/42",
            "2026-08-22T10:00:00Z",
            "2026-08-22T10:01:00Z",
            "2026-08-22T10:10:00Z",
            100,
            1,
            "old",
        )
        new = coverage_index.report_metadata(
            summary,
            "pr/42",
            "2026-08-22T10:01:00Z",
            "2026-08-22T10:02:00Z",
            "2026-08-22T10:03:00Z",
            101,
            1,
            "new",
        )
        rerun = coverage_index.report_metadata(
            summary,
            "pr/42",
            "2026-08-22T10:01:00Z",
            "2026-08-22T10:11:00Z",
            "2026-08-22T10:12:00Z",
            101,
            2,
            "new",
        )
        self.assertTrue(coverage_index.is_newer(new, old))
        self.assertFalse(coverage_index.is_newer(old, new))
        self.assertTrue(coverage_index.is_newer(rerun, new))
        self.assertEqual(
            {"pr/42": rerun}, coverage_index.latest_metadata([new, old, rerun, new])
        )
        same_tick_new_run = {
            **old,
            "source": {**old["source"], "run_id": 102, "run_attempt": 1},
        }
        same_tick_old_rerun = {
            **old,
            "source": {**old["source"], "run_id": 100, "run_attempt": 9},
        }
        self.assertTrue(
            coverage_index.is_newer(same_tick_new_run, same_tick_old_rerun)
        )


if __name__ == "__main__":
    unittest.main()
