#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
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
"""Assert every ``cc_library`` has a test in its own package depending on it.

Project rule (``STYLE_CPP.md``): all exported code is tested, at every level. Nothing
enforced it, so gaps accumulated quietly - an audit on 2026-08-10 found 4 of 50
libraries with no test at all, 3 of them in the shared extras API, the module other
modules implement. This check is the enforcement; the gaps themselves were fixed first.

"Has a test" means: some rule whose kind ends in ``_test`` in the SAME package lists the
library in its ``deps``. Same-package is the whole rule - a library tested only from
another package reads as untested here, and that is deliberate: the test belongs next to
the code it covers, so the exception has to be argued for in ``_ALLOWLIST`` rather than
discovered later by someone reorganizing packages.

Transitive coverage does not count either. A library pulled in by another library that
happens to have a test is not itself tested; requiring a direct dependency is what makes
the check mean "this unit has its own tests".

Usage:
    check_cc_library_tested.py [FILE...]

With no arguments it walks the repository for ``BUILD.bazel`` / ``BUILD`` files;
pre-commit passes the changed files instead. Because the rule is package-local, checking
one file in isolation is sound. Exits non-zero listing every uncovered library.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_rules import display_path, find_build_files, parse_rules  # noqa: E402

# Libraries allowed to have no test of their own, as "<package path>:<name>" -> why.
# Every entry is a judgement call that a reviewer should be able to check, so each one
# carries its reason here rather than in a commit message.
_ALLOWLIST: dict[str, str] = {
    "mbo/container:limited_set_benchmark_cc": (
        "benchmark harness: it is built and linted via the clang-tidy tag, but measuring performance "
        "makes no correctness claim a test could check"
    ),
}


def _package_local_deps(rules: list, package: str) -> set[str]:
    """Names of same-package targets that some `*_test` rule in this file depends on.

    Both spellings resolve to the bare name: `":foo_cc"` and the fully qualified
    `"//pkg:foo_cc"` for this same `pkg`. A label into another package or module is not
    package-local and is ignored.
    """
    tested: set[str] = set()
    for rule in rules:
        if not rule.kind.endswith("_test"):
            continue
        for label in rule.deps:
            if label.startswith(":"):
                tested.add(label[1:])
            elif label.startswith(f"//{package}:"):
                tested.add(label.split(":", 1)[1])
    return tested


def untested_libraries(text: str, package: str) -> list[tuple[int, str]]:
    """Return (line, name) for each `cc_library` in `text` no local test depends on."""
    rules = parse_rules(text)
    tested = _package_local_deps(rules, package)
    return [(r.line, r.name) for r in rules if r.kind == "cc_library" and r.name not in tested]


def check(paths: list[Path], root: Path) -> list[str]:
    """Return one human-readable complaint per uncovered library."""
    problems: list[str] = []
    for path in paths:
        try:
            text = path.read_text()
        except OSError as error:  # unreadable file: report rather than skip silently
            problems.append(f"{path}: cannot read ({error})")
            continue
        relative = display_path(path, root)
        package = str(relative.parent).replace(os.sep, "/")
        if package == ".":
            package = ""
        for line, name in untested_libraries(text, package):
            if f"{package}:{name}" in _ALLOWLIST:
                continue
            problems.append(f"{relative}:{line}: cc_library '{name}' has no test in its package depending on it")
    return problems


def main(argv: list[str]) -> int:
    root = Path.cwd()
    paths = [Path(arg) for arg in argv[1:]] or find_build_files(root)
    problems = check(paths, root)
    if not problems:
        return 0
    print(
        "every cc_library needs a test in its own package depending on it (STYLE_CPP.md).",
        file=sys.stderr,
    )
    print(
        "Add a cc_test next to it, or - if it genuinely has no testable surface - an "
        "_ALLOWLIST entry in tools/check_cc_library_tested.py stating why:",
        file=sys.stderr,
    )
    for problem in problems:
        print(f"  {problem}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
