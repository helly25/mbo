#!/usr/bin/env bash

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

# Verifies every `cc_library` under //mbo/... is reachable from at least one test
# target, so no library can be added - or left behind by a deleted test - without
# anything exercising it.
#
# Why reachability rather than a `<xxx>_cc` / `<xxx>_test` naming rule: the naming
# rule reports 32 of 75 libraries as unpaired, nearly all of them false alarms.
# `//mbo/types/internal:extend_cc` is thoroughly tested by `//mbo/types:extend_test`,
# it simply does not share its name. A rule that needs a 32-entry allowlist on the
# day it lands enforces nothing; reachability describes what actually matters and
# needs one exception.
#
# The test kind is matched as `.*_test`, NOT `cc_test`. mope is covered entirely by
# `*_diff_test` golden targets, so a `cc_test`-only query calls `//mbo/mope:mope_cc`
# untested when it is not. That narrower query reported 4 uncovered libraries, 3 of
# them wrong.
#
# Skips when bazel is unavailable rather than failing, so a checkout without a
# toolchain can still commit.

set -euo pipefail

if ! command -v bazel >/dev/null 2>&1; then
  echo "library-test-coverage: skipped (no bazel on PATH)." 1>&2
  exit 0
fi

# Libraries exempt from needing test coverage, each with the reason it is exempt.
# Keep this list SHORT - an entry here is a statement that the code needs no test,
# not a convenient way to silence the check.
#
#   //mbo/container:limited_set_benchmark_cc
#     A benchmark harness. It is built (and linted) via the `clang-tidy` tag, but
#     measuring performance is not a correctness assertion, so there is nothing for
#     a test to check.
readonly EXEMPT=(
  "//mbo/container:limited_set_benchmark_cc"
)

LIBS="$(bazel query 'kind("cc_library", //mbo/...)' 2>/dev/null | sed -E 's|^@+||; s|^//|//|' | sort -u || true)"
if [ -z "${LIBS}" ]; then
  echo "library-test-coverage: skipped (bazel query returned no libraries)." 1>&2
  exit 0
fi

# Every library any test target depends on, transitively.
COVERED="$(bazel query 'kind("cc_library", deps(kind(".*_test", //mbo/...)))' 2>/dev/null \
  | sed -E 's|^@+||; s|^//|//|' | sort -u || true)"
if [ -z "${COVERED}" ]; then
  echo "::error::library-test-coverage: no libraries reachable from any test - the query is wrong, not the tree."
  exit 1
fi

UNCOVERED=()
while IFS= read -r lib; do
  [ -n "${lib}" ] || continue
  if grep -qxF "${lib}" <<<"${COVERED}"; then
    continue
  fi
  skip=0
  for exempt in ${EXEMPT[@]+"${EXEMPT[@]}"}; do # empty-array safe under `set -u`
    if [ "${lib}" = "${exempt}" ]; then
      skip=1
      break
    fi
  done
  [ "${skip}" -eq 1 ] || UNCOVERED+=("${lib}")
done <<<"${LIBS}"

if [ "${#UNCOVERED[@]}" -gt 0 ]; then
  {
    echo "ERROR: library-test-coverage: no test target reaches these libraries:"
    printf '  %s\n' "${UNCOVERED[@]}"
    echo ""
    echo "       Add a test that depends on the library, or - if it genuinely cannot"
    echo "       be tested - add it to EXEMPT in $0 with the reason why."
  } 1>&2
  exit 1
fi

exit 0
