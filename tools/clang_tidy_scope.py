#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

"""Select translation units affected by files passed to clang-tidy."""

import json
import pathlib
import sys

SOURCE_SUFFIXES = (".cc", ".cpp", ".cxx")
WIDE_SUFFIXES = (".bzl", ".h", ".h.in", ".hh", ".hpp", ".hxx", ".inc", ".ipp")


def is_source(path: str) -> bool:
    return path.endswith(SOURCE_SUFFIXES)


def requires_full_sweep(path: str) -> bool:
    return path.endswith(WIDE_SUFFIXES)


def select_sources(database: list[dict[str, object]], changed: list[str]) -> list[str]:
    if not any(requires_full_sweep(path) for path in changed):
        return sorted({path for path in changed if is_source(path)})

    sources: set[str] = set()
    for entry in database:
        path = str(entry.get("file", ""))
        candidate = pathlib.PurePath(path)
        if candidate.is_absolute() or not is_source(path):
            continue
        if candidate.parts and candidate.parts[0] in {"bazel-out", "external"}:
            continue
        sources.add(path)
    return sorted(sources)


def main() -> int:
    if len(sys.argv) < 2:
        raise SystemExit("usage: clang_tidy_scope.py COMPILE_COMMANDS [CHANGED_FILE ...]")
    with open(sys.argv[1], encoding="utf-8") as stream:
        database = json.load(stream)
    for source in select_sources(database, sys.argv[2:]):
        print(source)
    return 0


if __name__ == "__main__":
    sys.exit(main())
