#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
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
"""Unit tests for check_cc_library_tested (run via pre-commit / `python3` directly)."""

from __future__ import annotations

import io
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_cc_library_tested as cclt  # noqa: E402

_LIB = 'cc_library(\n    name = "glob_cc",\n    srcs = ["glob.cc"],\n)\n'


def _test_rule(kind: str = "cc_test", dep: str = ":glob_cc", name: str = "glob_test") -> str:
    return f'{kind}(\n    name = "{name}",\n    srcs = ["{name}.cc"],\n    deps = [\n        "{dep}",\n    ],\n)\n'


class UntestedLibrariesTest(unittest.TestCase):
    def test_a_library_with_a_local_test_is_covered(self):
        self.assertEqual(cclt.untested_libraries(_LIB + _test_rule(), "xff/glob"), [])

    def test_a_fully_qualified_label_to_the_same_package_counts(self):
        # Both spellings appear in the tree; they mean the same target.
        text = _LIB + _test_rule(dep="//xff/glob:glob_cc")
        self.assertEqual(cclt.untested_libraries(text, "xff/glob"), [])

    def test_a_library_with_no_test_at_all_is_reported(self):
        self.assertEqual(cclt.untested_libraries(_LIB, "xff/glob"), [(1, "glob_cc")])

    def test_a_test_in_another_package_does_not_count(self):
        # The same label text, but this file is a different package: the test lives
        # elsewhere, so from here the library is uncovered.
        text = _LIB + _test_rule(dep="//xff/glob:glob_cc")
        self.assertEqual(cclt.untested_libraries(text, "xff/other"), [(1, "glob_cc")])

    def test_transitive_coverage_does_not_count(self):
        # `inner_cc` is only reached through `outer_cc`; a test of the outer library does
        # not test the inner one, which is the entire point of the rule.
        text = (
            'cc_library(\n    name = "inner_cc",\n)\n\n'
            'cc_library(\n    name = "outer_cc",\n    deps = [":inner_cc"],\n)\n\n' + _test_rule(dep=":outer_cc")
        )
        self.assertEqual(cclt.untested_libraries(text, "xff/pkg"), [(1, "inner_cc")])

    def test_a_non_test_rule_depending_on_it_does_not_count(self):
        # A binary (or a bashtest, whose kind does not end in `_test`) linking the library
        # is not a unit test of it. `//xff/cli:main_cc` is exactly this case and is
        # allowlisted with that reason spelled out.
        text = _LIB + 'cc_binary(\n    name = "glob",\n    deps = [":glob_cc"],\n)\n'
        self.assertEqual(cclt.untested_libraries(text, "xff/glob"), [(1, "glob_cc")])

    def test_any_rule_kind_ending_in_test_counts(self):
        # diff_test / xff_golden-style wrappers are tests too when they dep the library.
        self.assertEqual(cclt.untested_libraries(_LIB + _test_rule(kind="diff_test"), "xff/glob"), [])

    def test_several_libraries_are_each_checked(self):
        text = _LIB + 'cc_library(\n    name = "other_cc",\n)\n' + _test_rule()
        self.assertEqual(cclt.untested_libraries(text, "xff/glob"), [(5, "other_cc")])


class CheckTest(unittest.TestCase):
    def _write(self, root: Path, relative: str, text: str) -> Path:
        target = root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        return target

    def test_the_message_names_the_file_line_and_library(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            build = self._write(root, "xff/glob/BUILD.bazel", _LIB)
            problems = cclt.check([build], root)
        self.assertEqual(
            problems,
            ["xff/glob/BUILD.bazel:1: cc_library 'glob_cc' has no test in its package depending on it"],
        )

    def test_an_allowlisted_library_is_permitted(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            build = self._write(root, "mbo/container/BUILD.bazel", 'cc_library(\n    name = "limited_set_benchmark_cc",\n)\n')
            self.assertEqual(cclt.check([build], root), [])

    def test_the_allowlist_entry_records_a_reason(self):
        # An allowlist without reasons decays into a list of things nobody dares remove.
        self.assertIn("mbo/container:limited_set_benchmark_cc", cclt._ALLOWLIST)
        self.assertIn("benchmark", cclt._ALLOWLIST["mbo/container:limited_set_benchmark_cc"])

    def test_a_library_in_the_root_package_is_handled(self):
        # The root BUILD file's package is "", so the `//pkg:` prefix logic must not
        # produce "//:"-style false negatives.
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            build = self._write(root, "BUILD.bazel", _LIB)
            problems = cclt.check([build], root)
        self.assertEqual(problems, ["BUILD.bazel:1: cc_library 'glob_cc' has no test in its package depending on it"])

    def test_an_unreadable_file_is_reported_not_skipped(self):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            problems = cclt.check([root / "missing" / "BUILD.bazel"], root)
        self.assertEqual(len(problems), 1)
        self.assertIn("cannot read", problems[0])


class MainTest(unittest.TestCase):
    def test_main_exits_zero_and_prints_nothing_when_covered(self):
        with tempfile.TemporaryDirectory() as raw:
            build = Path(raw) / "BUILD.bazel"
            build.write_text(_LIB + _test_rule())
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                code = cclt.main(["check_cc_library_tested.py", str(build)])
        self.assertEqual(code, 0)
        self.assertEqual(stderr.getvalue(), "")

    def test_main_exits_nonzero_and_offers_both_remedies(self):
        with tempfile.TemporaryDirectory() as raw:
            build = Path(raw) / "BUILD.bazel"
            build.write_text(_LIB)
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                code = cclt.main(["check_cc_library_tested.py", str(build)])
        self.assertEqual(code, 1)
        self.assertIn("needs a test in its own package", stderr.getvalue())
        self.assertIn("_ALLOWLIST", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
