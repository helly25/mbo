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
"""Checks header guards against STYLE_CPP.md: `{PATH}_{FILE}_`.

That is the path plus filename, uppercased, every non-alphanumeric replaced by
`_`, with a trailing `_`.

clang-tidy cannot do this for us. `readability-identifier-naming`'s UPPER_CASE
notion splits inside words at digit boundaries (it wants MBO_DIGEST_DIGEST_M_D5_H
for digest_md5.h) and drops the trailing underscore, so it can never accept the
documented convention - which is exactly why `MacroDefinitionIgnoredRegexp`
exempts guard-shaped names from it. This check enforces the real rule instead of
leaving it to review.

`llvm-header-guard` is likewise disabled: it derives the guard from the absolute
checkout path.
"""

import re
import sys


def expected_guard(path: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "_", path).upper() + "_"


def check(path: str) -> list[str]:
    errors: list[str] = []
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError as err:
        return [f"{path}: cannot read: {err}"]

    want = expected_guard(path)
    match = re.search(r"^#ifndef\s+(\S+)", text, re.M)
    if not match:
        return [f"{path}: no `#ifndef` header guard found; expected `{want}`"]
    got = match.group(1)
    if got != want:
        errors.append(f"{path}: guard is `{got}`, expected `{want}`")

    if not re.search(rf"^#define\s+{re.escape(got)}\s*$", text, re.M):
        errors.append(f"{path}: `#ifndef {got}` has no matching `#define {got}`")

    # Only the LAST `#endif` closes the guard. Inner ones legitimately comment
    # their own condition (`#endif  // NDEBUG`), so they must not be touched.
    lines = text.split("\n")
    for line in reversed(lines):
        if line.startswith("#endif"):
            comment = re.match(r"#endif\s*//\s*(\S+)\s*$", line)
            if comment and comment.group(1) != got:
                errors.append(
                    f"{path}: closing `#endif` says `{comment.group(1)}`, expected `{got}`"
                )
            break
    return errors


def main(argv: list[str]) -> int:
    errors: list[str] = []
    for path in argv[1:]:
        if path.endswith(".h"):
            errors.extend(check(path))
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        print(
            "\nHeader guards follow STYLE_CPP.md: {PATH}_{FILE}_ (uppercased, "
            "non-alphanumerics -> `_`, trailing `_`).",
            file=sys.stderr,
        )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
