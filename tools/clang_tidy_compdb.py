#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

"""Create a one-command-per-source clang-tidy database."""

import argparse
import json
import shlex
import sys
from collections import defaultdict
from typing import Optional, Sequence


FUZZ_DEFINE = "-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION"


def arguments(entry: dict[str, object]) -> list[str]:
    raw_arguments = entry.get("arguments")
    if isinstance(raw_arguments, list):
        return [str(argument) for argument in raw_arguments]
    command = entry.get("command")
    if isinstance(command, str):
        return shlex.split(command)
    raise ValueError(f"compile command has neither arguments nor command: {entry!r}")


def is_fuzz_entry(entry: dict[str, object]) -> bool:
    return FUZZ_DEFINE in arguments(entry)


def deduplicate(database: list[dict[str, object]]) -> tuple[list[dict[str, object]], int]:
    grouped: dict[str, list[dict[str, object]]] = defaultdict(list)
    for entry in database:
        path = entry.get("file")
        if not isinstance(path, str) or not path:
            raise ValueError(f"compile command has no file: {entry!r}")
        grouped[path].append(entry)

    retained: list[dict[str, object]] = []
    removed = 0
    for path, entries in grouped.items():
        if len(entries) == 1:
            retained.append(entries[0])
            continue

        ordinary = [entry for entry in entries if not is_fuzz_entry(entry)]
        retained.append(ordinary[0] if ordinary else entries[0])
        removed += len(entries) - 1
    return retained, removed


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    with open(args.input, encoding="utf-8") as stream:
        database = json.load(stream)
    filtered, removed = deduplicate(database)
    with open(args.output, "w", encoding="utf-8") as stream:
        json.dump(filtered, stream)
    print(
        f"clang-tidy compile database: {len(database)} -> {len(filtered)} entries "
        f"({removed} duplicate entries removed)",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
