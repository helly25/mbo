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

# Tests the `diff` binary's CLI surface: flag combinations that the unit
# tests cannot reach (flag parsing, the deprecated alias enforcement and the
# usage/help text).

# shellcheck disable=SC2317 # Functions are called by bashtest

set -euo pipefail

# shellcheck disable=SC1090,SC1091,SC2154
source "${helly25_bashtest}"

DIFF="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/diff/diff"
declare -r DIFF
TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/diff/testdata/diff_cli_sh_test"
declare -r TESTDATA

# The full engine and format matrix. Every combination must have an expected
# output in testdata; a missing file fails the matrix test, so an engine or
# format added without coverage (or an unsupported combination) is spotted
# immediately.
declare -ra ALGORITHMS=(myers naive direct)
declare -ra FORMATS=(unified context normal side-by-side)

[[ -x ${DIFF} ]] || die "Program diff not found."

LHS="${BASHTEST_TMPDIR}/lhs.txt"
RHS="${BASHTEST_TMPDIR}/rhs.txt"
declare -r LHS RHS
printf 'a\nb\nc\n' >"${LHS}"
printf 'a\nx\nc\n' >"${RHS}"

function test::equal_files_exit_zero() {
  "${DIFF}" "${LHS}" "${LHS}" >"${TEST_TMPDIR}/same.out" || die "Expected exit code 0 for equal files."
  [[ -s "${TEST_TMPDIR}/same.out" ]] && die "Expected empty output for equal files."
  return 0
}

function test::exit_codes_follow_posix_diff() {
  # 0 = equal (covered above), 1 = different, 2 = trouble.
  local rc=0
  "${DIFF}" "${LHS}" "${RHS}" >/dev/null 2>&1 || rc=$?
  [[ ${rc} == 1 ]] || die "Expected exit code 1 for differing files, got ${rc}."
  rc=0
  "${DIFF}" "${LHS}" "${BASHTEST_TMPDIR}/does-not-exist.txt" >/dev/null 2>&1 || rc=$?
  [[ ${rc} == 2 ]] || die "Expected exit code 2 for an unreadable input, got ${rc}."
  rc=0
  "${DIFF}" "${LHS}" >/dev/null 2>&1 || rc=$?
  [[ ${rc} == 2 ]] || die "Expected exit code 2 for bad usage, got ${rc}."
}

function test::unified_alias_selects_unified_format() {
  if "${DIFF}" --algorithm=unified --file_header_use=none "${LHS}" "${RHS}" >"${TEST_TMPDIR}/alias.out" 2>&1; then
    die "Expected exit code 1 for differing files."
  fi
  grep -q '^@@ -1,3 +1,3 @@$' "${TEST_TMPDIR}/alias.out" || die "Expected a unified chunk header."
}

function test::unified_alias_rejects_other_formats() {
  if "${DIFF}" --algorithm=unified --format=normal "${LHS}" "${RHS}" >/dev/null 2>"${TEST_TMPDIR}/alias_err.out"; then
    die "Expected failure for the unified alias with a non-unified format."
  fi
  grep -q "implies '--format=unified'" "${TEST_TMPDIR}/alias_err.out" || die "Expected the alias error message."
}

function test::all_algorithms_and_formats_accepted() {
  for algorithm in "${ALGORITHMS[@]}"; do
    for format in "${FORMATS[@]}"; do
      "${DIFF}" --algorithm="${algorithm}" --format="${format}" "${LHS}" "${LHS}" >/dev/null 2>&1 \
        || die "Rejected: --algorithm=${algorithm} --format=${format}"
    done
  done
}

function test::engine_format_matrix() {
  # Per (engine, format) expected outputs on the Myers paper example
  # (ABCABBA vs CBABAC), where every engine legitimately differs: myers is
  # minimal (5 edits), naive resyncs greedily (7), direct pairs by position.
  for algorithm in "${ALGORITHMS[@]}"; do
    for format in "${FORMATS[@]}"; do
      local expected="${TESTDATA}/engine_${algorithm}_${format}.txt"
      [[ -f "${expected}" ]] || die "Missing expected output '${expected}': untested combination."
      "${DIFF}" --algorithm="${algorithm}" --format="${format}" --context=0 --file_header_use=none --width=30 \
        "${TESTDATA}/engine.lhs.txt" "${TESTDATA}/engine.rhs.txt" >"${TEST_TMPDIR}/matrix.out" || true
      expect_files_eq "${expected}" "${TEST_TMPDIR}/matrix.out"
    done
  done
}

function test::minimal_flag_changes_myers() {
  "${DIFF}" --algorithm=myers --minimal --context=0 --file_header_use=none \
    "${TESTDATA}/engine.lhs.txt" "${TESTDATA}/engine.rhs.txt" >"${TEST_TMPDIR}/minimal.out" || true
  expect_files_eq "${TESTDATA}/engine_myers_minimal_unified.txt" "${TEST_TMPDIR}/minimal.out"
}

function test::width_controls_side_by_side() {
  "${DIFF}" --format=side-by-side --width=20 "${LHS}" "${RHS}" >"${TEST_TMPDIR}/w20.out" || true
  grep -q '^b        | x$' "${TEST_TMPDIR}/w20.out" || die "Expected a 20 column side-by-side row."
}

function test::usage_names_all_formats_and_algorithms() {
  "${DIFF}" --help >"${TEST_TMPDIR}/help.out" 2>&1 || true
  for keyword in context normal side-by-side unified myers naive direct '--format' '--algorithm'; do
    grep -q -- "${keyword}" "${TEST_TMPDIR}/help.out" || die "Usage/help is missing '${keyword}'."
  done
}

test_runner
