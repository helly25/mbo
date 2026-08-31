#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Unit tests for ``check_test_sizes``."""

from __future__ import annotations

import io
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_test_sizes as cts  # noqa: E402


class ViolationsTest(unittest.TestCase):
    def test_explicit_small_and_medium_sizes_are_accepted(self):
        text = """
cc_test(
    name = "unit_test",
    size = "small",
)
sh_test(
    name = "integration_test",
    size = "medium",
)
"""
        self.assertEqual(cts.violations(text), [])

    def test_unsized_direct_test_is_reported_at_rule_line(self):
        text = 'cc_test(\n    name = "unit_test",\n    srcs = ["unit_test.cc"],\n)\n'
        self.assertEqual(cts.violations(text), [(1, "unit_test", "cc_test")])

    def test_non_test_rule_and_project_macro_are_ignored(self):
        text = """
cc_library(
    name = "library_cc",
)
diff_test(
    name = "golden_test",
)
"""
        self.assertEqual(cts.violations(text), [])


class CheckTest(unittest.TestCase):
    def test_message_names_file_line_kind_and_target(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            build = root / "BUILD.bazel"
            build.write_text('bashtest(\n    name = "cli_test",\n)\n')
            problems = cts.check([build], root)
        self.assertEqual(problems, ["BUILD.bazel:1: bashtest 'cli_test' has no explicit size"])

    def test_main_explains_the_rule(self):
        with tempfile.TemporaryDirectory() as raw:
            build = Path(raw) / "BUILD.bazel"
            build.write_text('cc_test(\n    name = "unit_test",\n)\n')
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                code = cts.main(["check_test_sizes.py", str(build)])
        self.assertEqual(code, 1)
        self.assertIn("must declare size explicitly", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
