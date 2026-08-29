#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

"""Create a fail-closed, one-command-per-source clang-tidy database."""

import argparse
import json
import re
import shlex
import sys
from collections import defaultdict
from typing import Optional, Sequence


FUZZ_DEFINE = "-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION"
BAZEL_OUTPUT = re.compile(r"bazel-out/[^/]+/")


def arguments(entry: dict[str, object]) -> list[str]:
    raw_arguments = entry.get("arguments")
    if isinstance(raw_arguments, list):
        return [str(argument) for argument in raw_arguments]
    command = entry.get("command")
    if isinstance(command, str):
        return shlex.split(command)
    raise ValueError(f"compile command has neither arguments nor command: {entry!r}")


def normalized_arguments(entry: dict[str, object]) -> list[str]:
    """Remove only known fuzz-transition and generated-output differences."""
    normalized: list[str] = []
    skip_value = False
    for argument in arguments(entry):
        if skip_value:
            skip_value = False
            continue
        if argument in {"-o", "-MF"}:
            skip_value = True
            continue
        if argument == FUZZ_DEFINE or argument.startswith("-frandom-seed="):
            continue
        normalized.append(BAZEL_OUTPUT.sub("bazel-out/<config>/", argument))
    if skip_value:
        raise ValueError("compile command ends with an output flag lacking its value")
    return normalized


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

        expected = normalized_arguments(entries[0])
        if any(normalized_arguments(entry) != expected for entry in entries[1:]):
            raise ValueError(
                f"duplicate compile commands for {path} differ beyond the allowed "
                "fuzz-transition/output arguments"
            )
        ordinary = [entry for entry in entries if not is_fuzz_entry(entry)]
        if len(ordinary) != 1:
            raise ValueError(
                f"expected exactly one ordinary compile command for {path}, found {len(ordinary)}"
            )
        retained.append(ordinary[0])
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
        f"({removed} redundant fuzz-transition entries removed)",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
