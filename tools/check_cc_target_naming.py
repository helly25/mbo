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
"""Assert every ``cc_library`` target name ends in ``_cc``.

Project convention (``STYLE_CPP.md``): a C++ library target is named ``<thing>_cc``
(``glob_cc``, ``license_cc``, ``vfs_cc``), so a label reads as "the C++ library for
<thing>" and never collides with the directory / file / binary of the same name. Tests
keep their ``_test`` suffix and are not checked here.

The convention was previously unwritten and only enforced by review, which let four
targets drift (``regex_backend``, ``license_notice``, ``archive_reader``,
``pcre2_backend`` - all in the removable extras, exactly where review attention is
thinnest). This check is the enforcement, so a rename cannot silently regress.

Usage:
    check_cc_target_naming.py [FILE...]

With no arguments it walks the repository for ``BUILD.bazel`` / ``BUILD`` files;
pre-commit passes the changed files instead. ``bazel-*`` symlink trees, ``external/``
checkouts and vendored ``third_party/`` modules are skipped (their target names are not
ours to choose). Exits non-zero listing every offender as
``path:line: target -> suggested name``.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_rules import display_path, find_build_files, parse_rules  # noqa: E402

# The rule kinds this convention covers. cc_test / cc_binary carry their own suffix
# conventions (`_test`, none) and are deliberately not checked here.
_CHECKED_KINDS = ("cc_library",)

_SUFFIX = "_cc"

# Target names allowed to break the rule, as "<package-relative path>:<name>". Empty by
# design: an exception should be rare enough to argue for in review. Add an entry only
# with a comment saying why the name cannot carry the suffix.
_ALLOWLIST: frozenset[str] = frozenset()

__all__ = ["check", "find_build_files", "main", "violations"]


def violations(path: Path, text: str) -> list[tuple[int, str, str]]:
    """Return (line number, target name, rule kind) for each misnamed library."""
    return [
        (rule.line, rule.name, rule.kind)
        for rule in parse_rules(text)
        if rule.kind in _CHECKED_KINDS and not rule.name.endswith(_SUFFIX)
    ]


def check(paths: list[Path], root: Path) -> list[str]:
    """Return one human-readable complaint per offending target."""
    problems: list[str] = []
    for path in paths:
        try:
            text = path.read_text()
        except OSError as error:  # unreadable file: report rather than skip silently
            problems.append(f"{path}: cannot read ({error})")
            continue
        relative = display_path(path, root)
        for line, name, kind in violations(path, text):
            if f"{relative.parent}:{name}" in _ALLOWLIST:
                continue
            problems.append(f"{relative}:{line}: {kind} '{name}' should be named '{name}{_SUFFIX}'")
    return problems


def main(argv: list[str]) -> int:
    root = Path.cwd()
    paths = [Path(arg) for arg in argv[1:]] or find_build_files(root)
    problems = check(paths, root)
    if not problems:
        return 0
    print(
        f"cc_library target names must end in '{_SUFFIX}' (STYLE_CPP.md); rename these and their deps:",
        file=sys.stderr,
    )
    for problem in problems:
        print(f"  {problem}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
