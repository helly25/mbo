# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

import unittest

from tools import clang_tidy_scope


class ClangTidyScopeTest(unittest.TestCase):
    def setUp(self) -> None:
        self.database: list[dict[str, object]] = [
            {"file": "mbo/a/a.cc"},
            {"file": "mbo/b/b_test.cc"},
            {"file": "mbo/a/a.h"},
            {"file": "external/dependency/source.cc"},
            {"file": "/usr/include/library.cc"},
            {"file": "mbo/a/a.cc"},
        ]

    def test_source_changes_stay_focused(self) -> None:
        self.assertEqual(
            clang_tidy_scope.select_sources(self.database, ["README.md", "mbo/a/a.cc"]),
            ["mbo/a/a.cc"],
        )

    def test_header_change_selects_all_first_party_sources(self) -> None:
        self.assertEqual(
            clang_tidy_scope.select_sources(self.database, ["mbo/a/a.h"]),
            ["mbo/a/a.cc", "mbo/b/b_test.cc"],
        )

    def test_generated_header_template_selects_all_sources(self) -> None:
        self.assertEqual(
            clang_tidy_scope.select_sources(self.database, ["mbo/config/config.h.in"]),
            ["mbo/a/a.cc", "mbo/b/b_test.cc"],
        )

    def test_bzl_change_selects_all_sources(self) -> None:
        self.assertEqual(
            clang_tidy_scope.select_sources(self.database, ["mbo/mope/mope.bzl"]),
            ["mbo/a/a.cc", "mbo/b/b_test.cc"],
        )


if __name__ == "__main__":
    unittest.main()
