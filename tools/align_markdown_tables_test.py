#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
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
"""Unit tests for align_markdown_tables (run via pre-commit / `python3` directly)."""

import os
from pathlib import Path
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import align_markdown_tables as mdt  # noqa: E402


class AlignMarkdownTablesTest(unittest.TestCase):
    def test_pads_columns_to_widest_cell(self):
        src = "| a | bb |\n| --- | --- |\n| ccc | d |\n"
        want = "| a   | bb  |\n| --- | --- |\n| ccc | d   |\n"
        self.assertEqual(mdt.align_text(src), want)

    def test_is_idempotent(self):
        aligned = "| a   | bb  |\n| --- | --- |\n| ccc | d   |\n"
        self.assertEqual(mdt.align_text(aligned), aligned)

    def test_honors_alignment_markers(self):
        src = "| L | R | C |\n|:--|--:|:-:|\n| a | b | c |\n"
        out = mdt.align_text(src)
        lines = out.split("\n")
        self.assertEqual(lines[0], "| L   |   R |  C  |")
        self.assertEqual(lines[1], "| :-- | --: | :-: |")
        self.assertEqual(lines[2], "| a   |   b |  c  |")

    def test_leaves_code_fences_untouched(self):
        src = "```\n| not | a |\n|-|-|\n| real | table |\n```\n"
        self.assertEqual(mdt.align_text(src), src)

    def test_does_not_split_escaped_pipe(self):
        src = "| a \\| b | c |\n| --- | --- |\n| d | e |\n"
        out = mdt.align_text(src)
        # The escaped pipe stays inside one cell ("a \| b"); the neighbor pads to the
        # 3-wide minimum.
        self.assertEqual(out.split("\n")[0], "| a \\| b | c   |")

    def test_pads_ragged_rows(self):
        src = "| a | b |\n| --- | --- |\n| c |\n"
        out = mdt.align_text(src)
        self.assertEqual(out.split("\n")[2], "| c   |     |")

    def test_ignores_pipe_lines_without_a_delimiter_row(self):
        src = "a | b | c\nnot a table here\n"
        self.assertEqual(mdt.align_text(src), src)

    def test_accepts_tables_without_outer_pipes(self):
        src = "a | bb\n--- | ---\nccc | d\n"
        want = "| a   | bb  |\n| --- | --- |\n| ccc | d   |\n"
        self.assertEqual(mdt.align_text(want), want)  # already-aligned form is stable
        self.assertEqual(mdt.align_text(src), want)  # bare form normalizes to it

    def test_preserves_indentation(self):
        src = "  | a | b |\n  | --- | --- |\n  | cc | d |\n"
        out = mdt.align_text(src)
        # Indentation preserved; columns pad to the 3-wide minimum.
        self.assertEqual(out.split("\n")[0], "  | a   | b   |")

    def test_check_mode_reports_without_writing(self):
        # main(--check) on a temp file returns 1 and does not modify it.
        import tempfile

        with tempfile.NamedTemporaryFile("w", suffix=".md", delete=False) as handle:
            handle.write("| a | bb |\n| - | - |\n| ccc | d |\n")
            path = handle.name
        try:
            before = Path(path).read_text(encoding="utf-8")
            rc = mdt.main(["--check", path])
            after = Path(path).read_text(encoding="utf-8")
            self.assertEqual(rc, 1)
            self.assertEqual(before, after)  # --check never writes
        finally:
            os.unlink(path)


if __name__ == "__main__":
    unittest.main()
