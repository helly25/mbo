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

# Run clang-tidy over the given C++ source files (a report-only pass: no --fix,
# so it never rewrites the tree). clang-tidy is a LOCAL-ONLY developer aid, not a
# CI gate: it needs a compile_commands.json (a local artifact, generated with
# `./compile_commands-update.sh`) and a clang-tidy new enough for this C++23
# codebase. When either is missing - a fresh checkout, or a CI runner without the
# compile DB - this SKIPS cleanly (exit 0) rather than failing, so it never blocks
# a commit or forces the hermetic toolchain download in CI. CI's hard gate stays
# the compiler -Werror in the bazel matrix.
#
# clang-tidy resolution prefers the hermetic toolchains_llvm binary (so it matches
# the compile DB's clang flags and understands C++23), then a versioned system
# clang-tidy on PATH. A resolved clang-tidy older than the minimum below is treated
# as "not installed" and skipped - clang-tidy 16/17 mis-parse this codebase's C++23
# and emit false positives whose fixes break the build. That is not hypothetical:
# trunk pinned clang-tidy 16 and its `--export-fixes` runs rewrote the working tree
# with build-breaking "fixes", which is why clang-tidy moved here out of trunk.
# Note that a plain `clang-tidy` on PATH may well BE trunk's 16 (.trunk/tools), so
# the version gate below is what keeps it from being picked up. Mirrors the binary
# resolution ladder in tools/clang_format.sh.

set -euo pipefail

# The minimum clang-tidy major version. Below this, C++23 parsing is unreliable.
readonly MIN_MAJOR=18

function skip() {
  echo "clang-tidy: skipped (${*}). It is a local-only aid; CI relies on the bazel -Werror gate." 1>&2
  exit 0
}

# A compile DB is mandatory; without it clang-tidy cannot resolve include paths.
[ -f "compile_commands.json" ] \
  || skip "no compile_commands.json; generate it with './compile_commands-update.sh'"

# The `bazel-<repo>` convenience link points at the execroot; fall back to cwd.
BAZEL_OUTPUT="bazel-$(basename "${PWD}")"
[ -d "${BAZEL_OUTPUT}" ] || BAZEL_OUTPUT="."

# The output base holds every fetched repo, including the `--config=clang`
# dev-dependency LLVM toolchain that the execroot symlink forest may omit.
OUTPUT_BASE="$(bazel info output_base 2>/dev/null || true)"

declare -a CLANG_TIDY_LOCS=(
  # 1) Hermetic toolchain via the `bazel-<repo>` execroot link.
  # 1.1) Bazel workspaces
  "${BAZEL_OUTPUT}/external/llvm_toolchain_llvm/bin/clang-tidy"
  "${BAZEL_OUTPUT}/external/llvm_toolchain/bin/clang-tidy"
  # 1.2) Bazel modules < 8
  "${BAZEL_OUTPUT}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm/bin/clang-tidy"
  "${BAZEL_OUTPUT}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm_llvm/bin/clang-tidy"
  # 1.3) Bazel modules >= 8
  "${BAZEL_OUTPUT}/external/toolchains_llvm++llvm+llvm_toolchain_llvm/bin/clang-tidy"
  "${BAZEL_OUTPUT}/external/toolchains_llvm++llvm+llvm_toolchain_llvm_llvm/bin/clang-tidy"

  # 2) Same hermetic toolchain via the output base (covers the dev-dependency
  #    case where it is absent from the execroot symlink forest). Still hermetic,
  #    so this is tried BEFORE any system clang-tidy below.
  "${OUTPUT_BASE:+${OUTPUT_BASE}/external/toolchains_llvm++llvm+llvm_toolchain_llvm/bin/clang-tidy}"
  "${OUTPUT_BASE:+${OUTPUT_BASE}/external/toolchains_llvm++llvm+llvm_toolchain_llvm_llvm/bin/clang-tidy}"

  # 3) System clang-tidy by versioned name (its version may differ from the
  #    hermetic toolchain, so it is a last resort). Nothing below MIN_MAJOR is
  #    listed - such a binary would be rejected by the version gate anyway.
  "$(which "clang-tidy-23" 2>/dev/null || true)"
  "$(which "clang-tidy-22" 2>/dev/null || true)"
  "$(which "clang-tidy-21" 2>/dev/null || true)"
  "$(which "clang-tidy-20" 2>/dev/null || true)"
  "$(which "clang-tidy-19" 2>/dev/null || true)"
  "$(which "clang-tidy-18" 2>/dev/null || true)"

  # 4) LLVM_PATH or a plain clang-tidy on PATH.
  "${LLVM_PATH:-/usr}/bin/clang-tidy"
  "$(which clang-tidy 2>/dev/null || true)"
)

CLANG_TIDY=""
for LOC in "${CLANG_TIDY_LOCS[@]}"; do
  if [ -n "${LOC}" ] && [ -x "${LOC}" ]; then
    # Gate on the major version: "LLVM version 22.1.8" -> 22. Skip anything older
    # than MIN_MAJOR (an unparseable version is treated as too old / unusable).
    major="$("${LOC}" --version 2>/dev/null | grep -oE 'version [0-9]+' | grep -oE '[0-9]+' | head -1 || true)"
    if [ -n "${major}" ] && [ "${major}" -ge "${MIN_MAJOR}" ]; then
      CLANG_TIDY="${LOC}"
      break
    fi
  fi
done

[ -n "${CLANG_TIDY}" ] \
  || skip "no clang-tidy >= ${MIN_MAJOR} found; build once with --config=clang to fetch the hermetic toolchain, or install a recent clang-tidy"

# Nothing to check (pre-commit may invoke with no matching files).
[ "${#}" -gt 0 ] || exit 0

# Checks that carry no meaning in test code, disabled for `*_test.cc` only. Doing
# it here rather than with a NOLINT per test keeps one statement of the rule
# instead of a comment repeated on every test, and new tests are covered without
# anyone remembering to annotate them.
#   * readability-function-cognitive-complexity: a gtest TestBody's score comes
#     from ASSERT_*/EXPECT_* macros expanding to branches, not from logic that
#     could be refactored. Test bodies reached 122 against a threshold of 30.
# `--checks` APPENDS to the `Checks` in .clang-tidy (it does not replace it), so
# every other check still applies to tests.
readonly TEST_DISABLED_CHECKS='-readability-function-cognitive-complexity'

declare -a SOURCES=()
declare -a TESTS=()
for FILE in "${@}"; do
  case "${FILE}" in
    # Not built by bazel at all: an SMHasher3 plugin, copied into that project by
    # mbo/hash/measurements/build_smhasher3.sh and compiled by ITS cmake. It has
    # no compile command here, so clang-tidy would lint it with flags guessed from
    # unrelated files and report its SMHasher3 includes as missing.
    mbo/hash/measurements/smhasher3/*) continue ;;
    *_test.cc | *_test.cpp | *_test.cxx) TESTS+=("${FILE}") ;;
    *) SOURCES+=("${FILE}") ;;
  esac
done

# A source file with no entry in the compile DB is NOT linted with the right
# flags - clang-tidy falls back to guessed defaults, fails to find even <gtest>,
# and then reports a cascade of nonsense (invalid case style for `TEST_F`, and
# so on) from the wreckage of a failed parse. That looks like real findings and
# is entirely an artefact of a stale DB, so fail loudly with the one command
# that fixes it. Only sources are checked: every .cc bazel builds has a compile
# command, whereas headers legitimately may not.
declare -a MISSING=()
for FILE in ${SOURCES[@]+"${SOURCES[@]}"} ${TESTS[@]+"${TESTS[@]}"}; do
  case "${FILE}" in
    *.cc | *.cpp | *.cxx) ;;
    *) continue ;;
  esac
  python3 -c '
import json, os, sys
want = os.path.realpath(sys.argv[1])
db = json.load(open("compile_commands.json"))
for e in db:
    f = e["file"]
    p = f if os.path.isabs(f) else os.path.join(e.get("directory", "."), f)
    if os.path.realpath(p) == want:
        sys.exit(0)
sys.exit(1)
' "${FILE}" || MISSING+=("${FILE}")
done
if [ "${#MISSING[@]}" -gt 0 ]; then
  echo "ERROR: clang-tidy: not in compile_commands.json: ${MISSING[*]}" 1>&2
  echo "       The compile DB is stale. Regenerate it with './compile_commands-update.sh'." 1>&2
  exit 1
fi

# Report only: --header-filter restricts diagnostics to this repo's own headers
# (not the toolchain's force-included / system headers), -p points at the compile
# DB. WarningsAsErrors in .clang-tidy makes any finding a non-zero exit.
# Both groups must run, and a finding in either has to fail, so no `exec` here.
STATUS=0
if [ "${#SOURCES[@]}" -gt 0 ]; then
  "${CLANG_TIDY}" --header-filter='(^|/)mbo/' -p . "${SOURCES[@]}" || STATUS=1
fi
if [ "${#TESTS[@]}" -gt 0 ]; then
  "${CLANG_TIDY}" --header-filter='(^|/)mbo/' --checks="${TEST_DISABLED_CHECKS}" -p . "${TESTS[@]}" || STATUS=1
fi
exit "${STATUS}"
