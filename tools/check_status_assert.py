#!/usr/bin/env python3
"""Reject ASSERT_THAT(StatusOr, IsOk()) followed by dereferencing that value."""

import re
import sys

_ASSERT = re.compile(r"\bASSERT_THAT\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*IsOk\(\)\s*\)")
_WINDOW = 8


def check(path):
    with open(path, encoding="utf-8") as source:
        lines = source.read().splitlines()
    problems = []
    for index, line in enumerate(lines):
        match = _ASSERT.search(line)
        if not match:
            continue
        name = re.escape(match.group(1))
        following = "\n".join(lines[index + 1 : index + 1 + _WINDOW])
        if re.search(rf"\*{name}\b|\b{name}->", following):
            problems.append(
                f"{path}:{index + 1}: bind the StatusOr value with MBO_ASSERT_OK_AND_ASSIGN "
                "or match it with IsOkAndHolds instead of asserting IsOk and dereferencing it"
            )
    return problems


def main(paths):
    problems = [problem for path in paths if path.endswith("_test.cc") for problem in check(path)]
    for problem in problems:
        print(problem, file=sys.stderr)
    return int(bool(problems))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
