#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0
"""Require direct Bazel test rules to declare their scheduling size."""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_rules import display_path, find_build_files, parse_rules  # noqa: E402

_DIRECT_TEST_RULES = frozenset({"bashtest", "cc_fuzz_test", "cc_test", "py_test", "sh_test"})
_SIZE_ATTR = re.compile(r"^\s+size\s*=")


def violations(text: str) -> list[tuple[int, str, str]]:
    """Return ``(line, name, kind)`` for direct test rules without ``size``."""
    lines = text.splitlines()
    problems: list[tuple[int, str, str]] = []
    for rule in parse_rules(text):
        if rule.kind not in _DIRECT_TEST_RULES:
            continue
        block = []
        for line in lines[rule.line :]:
            if line.startswith(")"):
                break
            block.append(line)
        if not any(_SIZE_ATTR.match(line) for line in block):
            problems.append((rule.line, rule.name, rule.kind))
    return problems


def check(paths: list[Path], root: Path) -> list[str]:
    """Return one human-readable complaint per unsized direct test rule."""
    problems: list[str] = []
    for path in paths:
        try:
            text = path.read_text()
        except OSError as error:
            problems.append(f"{path}: cannot read ({error})")
            continue
        relative = display_path(path, root)
        for line, name, kind in violations(text):
            problems.append(f"{relative}:{line}: {kind} '{name}' has no explicit size")
    return problems


def main(argv: list[str]) -> int:
    root = Path.cwd()
    paths = [Path(arg) for arg in argv[1:]] or find_build_files(root)
    problems = check(paths, root)
    if not problems:
        return 0
    print("direct Bazel test rules must declare size explicitly (STYLE_CPP.md):", file=sys.stderr)
    for problem in problems:
        print(f"  {problem}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
