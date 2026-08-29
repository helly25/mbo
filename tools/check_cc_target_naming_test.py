#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Unit tests for check_cc_target_naming (run via pre-commit / `python3` directly)."""

from __future__ import annotations

import io
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_cc_target_naming as cctn  # noqa: E402

_GOOD = """
cc_library(
    name = "glob_cc",
    srcs = ["glob.cc"],
)
"""

_BAD = """
cc_library(
    name = "regex_backend",
    srcs = ["backend.cc"],
)
"""


class ViolationsTest(unittest.TestCase):
    def test_a_conforming_library_is_accepted(self):
        self.assertEqual(cctn.violations(Path("BUILD.bazel"), _GOOD), [])

    def test_a_library_without_the_suffix_is_reported_at_the_rule_line(self):
        # The line reported is the rule's opening line, not the `name =` line: that is
        # where a reader edits, and it stays stable when attributes are reordered.
        self.assertEqual(cctn.violations(Path("BUILD.bazel"), _BAD), [(2, "regex_backend", "cc_library")])

    def test_a_cc_test_is_not_subject_to_the_cc_suffix(self):
        # Tests carry `_test`; checking them here would demand `foo_test_cc`.
        text = 'cc_test(\n    name = "regex_test",\n    srcs = ["regex_test.cc"],\n)\n'
        self.assertEqual(cctn.violations(Path("BUILD.bazel"), text), [])

    def test_an_unrelated_rule_kind_is_ignored(self):
        text = 'py_library(\n    name = "helper",\n)\n'
        self.assertEqual(cctn.violations(Path("BUILD.bazel"), text), [])

    def test_a_single_line_rule_is_checked_too(self):
        text = 'cc_library(name = "notice", hdrs = ["notice.h"])\n'
        self.assertEqual(cctn.violations(Path("BUILD.bazel"), text), [(1, "notice", "cc_library")])

    def test_only_the_first_name_attribute_binds_to_a_rule(self):
        # A `name` inside a nested attribute (a select key, a macro arg) must not be
        # mistaken for a second target, and the following rule must still be seen.
        text = _BAD + 'cc_library(\n    name = "second_cc",\n    deps = [":regex_backend"],\n)\n'
        self.assertEqual(cctn.violations(Path("BUILD.bazel"), text), [(2, "regex_backend", "cc_library")])

    def test_several_offenders_in_one_file_are_all_reported(self):
        text = _BAD + _BAD.replace("regex_backend", "license_notice")
        self.assertEqual(
            cctn.violations(Path("BUILD.bazel"), text),
            [(2, "regex_backend", "cc_library"), (7, "license_notice", "cc_library")],
        )


class CheckTest(unittest.TestCase):
    def test_the_message_names_the_file_line_and_the_suggested_rename(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            (root / "xff_extras_api").mkdir()
            build = root / "xff_extras_api" / "BUILD.bazel"
            build.write_text(_BAD)
            problems = cctn.check([build], root)
        self.assertEqual(
            problems,
            ["xff_extras_api/BUILD.bazel:2: cc_library 'regex_backend' should be named 'regex_backend_cc'"],
        )

    def test_an_allowlisted_target_is_permitted(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            (root / "pkg").mkdir()
            build = root / "pkg" / "BUILD.bazel"
            build.write_text(_BAD)
            original = cctn._ALLOWLIST
            cctn._ALLOWLIST = frozenset({"pkg:regex_backend"})
            try:
                self.assertEqual(cctn.check([build], root), [])
            finally:
                cctn._ALLOWLIST = original

    def test_a_file_outside_the_root_still_reports(self):
        # `main` derives the root from the cwd, so a caller naming a file elsewhere (or a
        # pre-commit run from a subdirectory) must not crash on relative_to; it reports
        # the absolute path instead.
        with tempfile.TemporaryDirectory() as raw:
            build = Path(raw) / "BUILD.bazel"
            build.write_text(_BAD)
            problems = cctn.check([build], Path("/nowhere"))
        self.assertEqual(problems, [f"{build}:2: cc_library 'regex_backend' should be named 'regex_backend_cc'"])

    def test_an_unreadable_file_is_reported_not_skipped(self):
        # Silently skipping would turn a broken checkout into a passing lint.
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            problems = cctn.check([root / "missing" / "BUILD.bazel"], root)
        self.assertEqual(len(problems), 1)
        self.assertIn("cannot read", problems[0])


class FindBuildFilesTest(unittest.TestCase):
    def test_generated_and_vendored_trees_are_skipped(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            for relative in (
                "BUILD.bazel",
                "xff/glob/BUILD.bazel",
                "bazel-out/k8/BUILD.bazel",
                "third_party/vendored/BUILD.bazel",
                "external/dep/BUILD.bazel",
            ):
                target = root / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(_GOOD)
            found = [str(p.relative_to(root)) for p in cctn.find_build_files(root)]
        self.assertEqual(found, ["BUILD.bazel", "xff/glob/BUILD.bazel"])


class MainTest(unittest.TestCase):
    def test_main_exits_zero_and_prints_nothing_when_clean(self):
        with tempfile.TemporaryDirectory() as raw:
            build = Path(raw) / "BUILD.bazel"
            build.write_text(_GOOD)
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                code = cctn.main(["check_cc_target_naming.py", str(build)])
        self.assertEqual(code, 0)
        self.assertEqual(stderr.getvalue(), "")

    def test_main_exits_nonzero_and_explains_the_convention(self):
        with tempfile.TemporaryDirectory() as raw:
            build = Path(raw) / "BUILD.bazel"
            build.write_text(_BAD)
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                code = cctn.main(["check_cc_target_naming.py", str(build)])
        self.assertEqual(code, 1)
        self.assertIn("must end in '_cc'", stderr.getvalue())
        self.assertIn("regex_backend_cc", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
