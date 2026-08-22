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
"""Align GitHub-flavored Markdown table columns so the pipes line up.

A "table" is a header row, a delimiter row (cells of ``:?-+:?``), then zero or more
body rows -- all pipe-delimited and equally indented. Each column is padded to its
widest cell, justified per the delimiter's alignment markers (``:--`` left, ``--:``
right, ``:-:`` center, ``---`` default/left). Content inside fenced code blocks
(``` ``` ``` / ``~~~``) is left untouched, as is anything that does not parse as a
table.

Usage:
    align_markdown_tables.py [--check] FILE...

Without ``--check`` each FILE is rewritten in place if it changed; with ``--check``
nothing is written. Either way the exit status is non-zero if any FILE needed
reformatting (so it gates like clang-format under pre-commit), and 2 on error.

Known limitation: column width is measured in code points (str length), so East
Asian wide / combining characters are not visually accounted for.
"""

import argparse
import re
import sys

_DELIM_CELL = re.compile(r"^:?-+:?$")
_FENCE = re.compile(r"^(\s*)(`{3,}|~{3,})")


def _split_cells(row):
    """Splits a table row into trimmed cell texts, honoring backslash-escaped pipes
    and an optional leading/trailing pipe."""
    s = row.strip()
    if s.startswith("|"):
        s = s[1:]
    # A trailing pipe is a delimiter unless it is escaped (``\|``).
    if s.endswith("|") and not s.endswith("\\|"):
        s = s[:-1]
    cells = []
    cur = []
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == "\\" and i + 1 < len(s):
            cur.append(ch)
            cur.append(s[i + 1])
            i += 2
            continue
        if ch == "|":
            cells.append("".join(cur).strip())
            cur = []
            i += 1
            continue
        cur.append(ch)
        i += 1
    cells.append("".join(cur).strip())
    return cells


def _is_delimiter(cells):
    return bool(cells) and all(_DELIM_CELL.match(c) for c in cells)


def _alignment(cell):
    """left / right / center / none, read from a delimiter cell's colons."""
    left = cell.startswith(":")
    right = cell.endswith(":")
    if left and right:
        return "center"
    if right:
        return "right"
    if left:
        return "left"
    return "none"


def _looks_like_row(line):
    """A non-fence, non-blank line that carries a pipe -- a table-row candidate."""
    stripped = line.strip()
    return "|" in stripped and stripped != ""


def _format_table(rows):
    """rows: list of raw row strings (header, delimiter, body...). Returns the
    aligned replacement lines, preserving the header row's indentation."""
    indent = rows[0][: len(rows[0]) - len(rows[0].lstrip())]
    parsed = [_split_cells(r) for r in rows]
    aligns = [_alignment(c) for c in parsed[1]]
    ncols = max(len(r) for r in parsed)
    aligns += ["none"] * (ncols - len(aligns))
    for cells in parsed:
        cells += [""] * (ncols - len(cells))

    widths = [0] * ncols
    for idx, cells in enumerate(parsed):
        if idx == 1:
            continue  # the delimiter row does not constrain width
        for col in range(ncols):
            widths[col] = max(widths[col], len(cells[col]))
    widths = [max(w, 3) for w in widths]  # room for the delimiter markers

    def render_delim(col):
        width = widths[col]
        align = aligns[col]
        if align == "center":
            return ":" + ("-" * (width - 2)) + ":"
        if align == "left":
            return ":" + ("-" * (width - 1))
        if align == "right":
            return ("-" * (width - 1)) + ":"
        return "-" * width

    def render_cell(text, col):
        width = widths[col]
        align = aligns[col]
        if align == "right":
            return text.rjust(width)
        if align == "center":
            return text.center(width)
        return text.ljust(width)

    out = []
    for idx, cells in enumerate(parsed):
        if idx == 1:
            body = " | ".join(render_delim(c) for c in range(ncols))
        else:
            body = " | ".join(render_cell(cells[c], c) for c in range(ncols))
        out.append(f"{indent}| {body} |".rstrip())
    return out


def align_text(text):
    """Returns `text` with every Markdown table's columns aligned."""
    lines = text.split("\n")
    out = []
    fence = None  # the closing fence marker while inside a code block, else None
    i = 0
    while i < len(lines):
        line = lines[i]
        fence_match = _FENCE.match(line)
        if fence is not None:
            out.append(line)
            if fence_match and fence_match.group(2)[0] == fence[0] and len(fence_match.group(2)) >= len(fence):
                fence = None
            i += 1
            continue
        if fence_match:
            fence = fence_match.group(2)
            out.append(line)
            i += 1
            continue
        # A table needs a row, then a delimiter row, both pipe-bearing.
        if _looks_like_row(line) and i + 1 < len(lines) and _looks_like_row(lines[i + 1]) and _is_delimiter(
            _split_cells(lines[i + 1])
        ):
            block = [line, lines[i + 1]]
            j = i + 2
            while j < len(lines) and _looks_like_row(lines[j]) and not _FENCE.match(lines[j]):
                block.append(lines[j])
                j += 1
            out.extend(_format_table(block))
            i = j
            continue
        out.append(line)
        i += 1
    return "\n".join(out)


def main(argv):
    parser = argparse.ArgumentParser(description="Align Markdown table columns.")
    parser.add_argument("--check", action="store_true", help="do not write; only report files that would change")
    parser.add_argument("files", nargs="*", help="Markdown files to align")
    args = parser.parse_args(argv)

    changed = []
    for path in args.files:
        try:
            with open(path, "r", encoding="utf-8") as handle:
                original = handle.read()
        except OSError as err:
            print(f"align_markdown_tables: {path}: {err}", file=sys.stderr)
            return 2
        aligned = align_text(original)
        if aligned != original:
            changed.append(path)
            if not args.check:
                with open(path, "w", encoding="utf-8") as handle:
                    handle.write(aligned)
    for path in changed:
        verb = "would reformat" if args.check else "reformatted"
        print(f"align_markdown_tables: {verb} {path}", file=sys.stderr)
    return 1 if changed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
