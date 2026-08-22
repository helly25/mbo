#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

"""Group LCOV sources by the repository coverage-policy categories."""

import argparse
import fnmatch
import json
import re
from pathlib import Path


def _matches(path: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def grouped(report: str, policy: dict, source_root: Path, workspace: Path = Path.cwd()) -> str:
    """Returns selected LCOV records rooted below their one policy category."""
    includes = policy.get("include", ["mbo/**"])
    excludes = policy.get("exclude", [])
    categories = [(name, value["include"]) for name, value in policy["categories"].items()]
    result = []
    for record in report.split("end_of_record\n"):
        match = re.search(r"(?m)^SF:(.+)$", record)
        if not match:
            continue
        if not re.search(r"(?m)^(LF|FNF|BRF):[1-9][0-9]*$", record):
            continue
        physical = match.group(1)
        logical = physical.removeprefix(str(workspace.resolve()) + "/")
        if not _matches(logical, includes) or _matches(logical, excludes):
            continue
        matches = [name for name, patterns in categories if _matches(logical, patterns)]
        if len(matches) != 1:
            raise ValueError(f"coverage source {logical!r} belongs to {len(matches)} policy categories")
        linked = source_root.resolve() / matches[0] / logical
        linked.parent.mkdir(parents=True, exist_ok=True)
        if not linked.exists() and not linked.is_symlink():
            linked.symlink_to((workspace / logical).resolve())
        result.append(record.replace(f"SF:{physical}", f"SF:{linked}", 1) + "end_of_record\n")
    return "".join(result)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--policy", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    args = parser.parse_args()
    report = grouped(
        args.input.read_text(encoding="utf-8"),
        json.loads(args.policy.read_text(encoding="utf-8")),
        args.source_root,
    )
    args.output.write_text(report, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
