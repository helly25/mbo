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
"""Assert every tracked header is claimed by its package's BUILD file.

A header no target lists in ``hdrs``/``srcs``/``textual_hdrs`` is invisible to the
build - and therefore to the clang-tidy gate, which lints ``.cc`` files and reaches
headers only through a translation unit that includes them. An unclaimed header is
unlinted, unbuilt dead weight that still LOOKS like source.

"Claimed" is textual and package-scoped, mirroring bazel's own rule that ``hdrs``
and ``srcs`` labels are package-relative: header ``mbo/x/sub/y.h`` must appear as
``"sub/y.h"`` in the nearest ancestor ``BUILD.bazel`` (``mbo/x/BUILD.bazel`` when
``mbo/x/sub`` has no BUILD file of its own).

Deliberate exceptions live in ``_ALLOWLIST`` with the reason recorded there.

Usage:
    check_headers_claimed.py [FILE...]

With no arguments it checks every ``git ls-files`` header under ``mbo/``;
pre-commit passes the changed files instead. Exits non-zero listing every orphan as
``header -> expected in BUILD-file``.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

_ALLOWLIST: dict[str, str] = {
    "mbo/hash/measurements/smhasher3/mbohash.h": (
        "an SMHasher3 plugin: copied into that project by build_smhasher3.sh and compiled by ITS "
        "cmake; there is no BUILD file here on purpose"
    ),
}


def repo_root() -> Path:
    out = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True, check=True
    )
    return Path(out.stdout.strip())


def tracked_headers(root: Path) -> list[str]:
    out = subprocess.run(
        ["git", "ls-files", "mbo/**/*.h", "mbo/*.h"], capture_output=True, text=True, check=True, cwd=root
    )
    return [line for line in out.stdout.splitlines() if line]


def owning_build_file(root: Path, header: str) -> Path | None:
    """The nearest ancestor BUILD.bazel, walking up from the header's directory."""
    directory = (root / header).parent
    while directory >= root:
        build = directory / "BUILD.bazel"
        if build.is_file():
            return build
        if directory == root:
            return None
        directory = directory.parent
    return None


def check(headers: list[str], root: Path) -> list[str]:
    errors: list[str] = []
    for header in sorted(headers):
        if header in _ALLOWLIST:
            continue
        build = owning_build_file(root, header)
        if build is None:
            errors.append(f"  {header} -> no BUILD.bazel found in any ancestor directory")
            continue
        package_dir = build.parent
        relative = (root / header).relative_to(package_dir).as_posix()
        if f'"{relative}"' not in build.read_text():
            errors.append(f"  {header} -> not listed in {build.relative_to(root)}")
    return errors


def main(argv: list[str]) -> int:
    root = repo_root()
    if argv:
        headers = [arg for arg in argv if arg.endswith(".h") and arg.startswith("mbo/")]
    else:
        headers = tracked_headers(root)
    errors = check(headers, root)
    if errors:
        print("every tracked header must be claimed by its package's BUILD file (hdrs/srcs);")
        print("an unclaimed header is invisible to the build and the clang-tidy gate:")
        print("\n".join(errors))
        print("List it in a target, or add an _ALLOWLIST entry in this tool stating why.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
