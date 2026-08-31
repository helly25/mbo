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
"""A minimal BUILD-file reader shared by the repository's BUILD lints.

Deliberately NOT a Starlark parser: it recognizes the shape buildifier produces (one
top-level rule call per block, its attributes one per line) and pulls out each rule's
kind, name, line number, and `deps` labels. That is all the BUILD hygiene checks need,
and it keeps them dependency-free so they can run as plain pre-commit scripts.

Both lints share this reader so they cannot disagree about what a rule is:

* ``check_cc_target_naming.py``   - every ``cc_library`` is named ``<thing>_cc``;
* ``check_cc_library_tested.py``  - every ``cc_library`` has a test depending on it.

Known limits, by design: a rule assembled by a macro or a comprehension is invisible,
and ``deps`` reached through a variable is not resolved. Both lints therefore treat a
finding as "prove it or allowlist it", never as ground truth about the build graph -
``bazel query`` is the authority when the two disagree.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

# A rule call opens with `kind(` at the start of a line (buildifier formats top-level
# rules at column 0); attributes follow indented, one per line.
_RULE_OPEN = re.compile(r"^(\w+)\(")
_NAME_ATTR = re.compile(r'^\s*name\s*=\s*"([^"]*)"')
_INLINE_NAME = re.compile(r'^\w+\(\s*name\s*=\s*"([^"]*)"')
# `deps = [` / `deps = select({` / `deps = [...] + select({`, and the next attribute
# (4-space indented `word =`) that ends it.
_DEPS_ATTR = re.compile(r"^\s+deps\s*=")
_NEXT_ATTR = re.compile(r"^\s{4}\w+\s*=")
_LABEL = re.compile(r'"((?::|//|@)[^"]*)"')

_SKIP_DIRS = ("bazel-", "external", "third_party", ".git")


@dataclass
class Rule:
    """One top-level rule call: its kind, name, opening line, and `deps` labels."""

    kind: str
    name: str
    line: int
    deps: list[str] = field(default_factory=list)


def parse_rules(text: str) -> list[Rule]:
    """Every top-level rule in `text`, in file order.

    The reported line is the rule's OPENING line, not its `name =` line: that is where a
    reader edits, and it stays stable when attributes are reordered.
    """
    rules: list[Rule] = []
    current: Rule | None = None
    in_deps = False
    for number, line in enumerate(text.splitlines(), start=1):
        opened = _RULE_OPEN.match(line)
        if opened:
            inline = _INLINE_NAME.match(line)
            current = Rule(kind=opened.group(1), name=inline.group(1) if inline else "", line=number)
            rules.append(current)
            in_deps = False
            # A single-line rule closes immediately; anything else keeps collecting.
            if line.rstrip().endswith(")") and inline:
                current = None
            continue
        if current is None:
            continue
        if line.startswith(")"):
            current = None
            in_deps = False
            continue
        if not current.name:
            name = _NAME_ATTR.match(line)
            if name:
                current.name = name.group(1)
                continue
        if _DEPS_ATTR.match(line):
            in_deps = True
            current.deps.extend(_LABEL.findall(line))
            continue
        if in_deps:
            if _NEXT_ATTR.match(line):
                in_deps = False
                continue
            current.deps.extend(_LABEL.findall(line))
    return [rule for rule in rules if rule.name]


def is_skipped(relative: Path) -> bool:
    """True for generated (`bazel-*`), external, vendored, or VCS trees."""
    return any(part == skip or part.startswith("bazel-") for part in relative.parts for skip in _SKIP_DIRS)


def find_build_files(root: Path) -> list[Path]:
    """Every BUILD.bazel / BUILD file under `root`, minus generated and vendored trees."""
    found = [p for p in root.rglob("BUILD.bazel") if not is_skipped(p.relative_to(root))]
    found += [p for p in root.rglob("BUILD") if not is_skipped(p.relative_to(root))]
    return sorted(found)


def display_path(path: Path, root: Path) -> Path:
    """`path` relative to `root` when inside it, else unchanged (never raises)."""
    try:
        return path.relative_to(root)
    except ValueError:
        return path
