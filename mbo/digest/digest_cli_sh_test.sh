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

# Tests the `digest` binary's CLI surface: algorithm selection, the
# checksum-style output format, --reverse, stdin, multi-file processing and
# the error paths (directory, missing file, unknown algorithm).

# shellcheck disable=SC2317 # Functions are called by bashtest

set -euo pipefail

# shellcheck disable=SC1090,SC1091,SC2154
source "${helly25_bashtest}"

DIGEST="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/digest/digest"
declare -r DIGEST

[[ -x ${DIGEST} ]] || die "Program digest not found."

ABC="${BASHTEST_TMPDIR}/abc.txt"
declare -r ABC
printf 'abc' >"${ABC}"

# Known answers for the file containing exactly 'abc' (the values pinned in
# digest_test.cc / generated with independent references). No associative
# arrays: macOS ships bash 3.2.
SHA256_ABC="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
declare -r SHA256_ABC

function known_hash() {
  case "$1" in
    md5) echo "900150983cd24fb0d6963f7d28e17f72" ;;
    sha1) echo "a9993e364706816aba3e25717850c26c9cd0d89d" ;;
    sha256) echo "${SHA256_ABC}" ;;
    sha512-256) echo "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23" ;;
    sha3-256) echo "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532" ;;
    blake3) echo "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85" ;;
    *) die "No known hash for $1." ;;
  esac
}

function test::known_answers_per_algorithm() {
  local algorithm
  for algorithm in md5 sha1 sha256 sha512-256 sha3-256 blake3; do
    local out
    out="$("${DIGEST}" --algorithm="${algorithm}" "${ABC}")" || die "digest failed for ${algorithm}."
    [[ ${out} == "$(known_hash "${algorithm}")  ${ABC}" ]] \
      || die "Wrong ${algorithm} output: '${out}'"
  done
}

function test::short_algorithm_flag_alias() {
  local out
  out="$("${DIGEST}" -a sha3-256 "${ABC}")" || die "digest failed for -a."
  [[ ${out} == "$(known_hash sha3-256)  ${ABC}" ]] || die "Wrong -a output: '${out}'"
  out="$("${DIGEST}" -a md5 --algorithm=sha256 "${ABC}")" || die "digest failed for -a + --algorithm."
  [[ ${out} == "$(known_hash md5)  ${ABC}" ]] || die "-a must take precedence: '${out}'"
}

function test::default_algorithm_is_sha256() {
  local out
  out="$("${DIGEST}" "${ABC}")" || die "digest failed."
  [[ ${out} == "${SHA256_ABC}  ${ABC}" ]] || die "Default is not sha256: '${out}'"
}

function test::all_algorithms_produce_hex_lines() {
  local algorithm
  for algorithm in md5 sha1 sha224 sha256 sha384 sha512 sha512-224 sha512-256 \
    sha3-224 sha3-256 sha3-384 sha3-512 shake128 shake256 blake2b blake2b-256 blake3; do
    local out
    out="$("${DIGEST}" --algorithm="${algorithm}" "${ABC}")" || die "digest failed for ${algorithm}."
    [[ ${out} =~ ^[0-9a-f]+\ \ .+$ ]] || die "Malformed output for ${algorithm}: '${out}'"
  done
}

function test::reverse_swaps_columns() {
  local out
  out="$("${DIGEST}" --reverse "${ABC}")" || die "digest failed."
  [[ ${out} == "${ABC}  ${SHA256_ABC}" ]] || die "Wrong --reverse output: '${out}'"
}

function test::stdin_via_dash() {
  local out
  out="$(printf 'abc' | "${DIGEST}" --algorithm=sha3-256 -)" || die "digest failed on stdin."
  [[ ${out} == "$(known_hash sha3-256)  -" ]] || die "Wrong stdin output: '${out}'"
}

function test::multiple_files_one_line_each() {
  local out
  out="$("${DIGEST}" "${ABC}" "${ABC}" "${ABC}")" || die "digest failed."
  [[ $(wc -l <<<"${out}") -eq 3 ]] || die "Expected three lines, got: '${out}'"
}

function test::directory_is_an_error_but_processing_continues() {
  local out
  if out="$("${DIGEST}" "${BASHTEST_TMPDIR}" "${ABC}" 2>"${TEST_TMPDIR}/dir.err")"; then
    die "Expected exit code 1 for a directory argument."
  fi
  grep -q "Is a directory" "${TEST_TMPDIR}/dir.err" || die "Expected a directory error message."
  [[ ${out} == "${SHA256_ABC}  ${ABC}" ]] || die "Expected the regular file still to be hashed."
}

function test::ignore_directories_skips_silently() {
  local out
  out="$("${DIGEST}" --ignore_directories "${BASHTEST_TMPDIR}" "${ABC}" 2>"${TEST_TMPDIR}/skip.err")" \
    || die "Expected exit code 0 with --ignore_directories."
  [[ -s "${TEST_TMPDIR}/skip.err" ]] && die "Expected no error output with --ignore_directories."
  [[ ${out} == "${SHA256_ABC}  ${ABC}" ]] || die "Expected only the regular file to be hashed."
  out="$("${DIGEST}" -d "${BASHTEST_TMPDIR}" "${ABC}" 2>"${TEST_TMPDIR}/skip_short.err")" \
    || die "Expected exit code 0 with -d."
  [[ -s "${TEST_TMPDIR}/skip_short.err" ]] && die "Expected no error output with -d."
  [[ ${out} == "${SHA256_ABC}  ${ABC}" ]] || die "Expected only the regular file to be hashed with -d."
}

function test::missing_file_is_an_error() {
  if "${DIGEST}" "${BASHTEST_TMPDIR}/does-not-exist" 2>"${TEST_TMPDIR}/missing.err"; then
    die "Expected exit code 1 for a missing file."
  fi
  grep -q "Cannot read file" "${TEST_TMPDIR}/missing.err" || die "Expected a read error message."
}

function test::unknown_algorithm_is_an_error() {
  if "${DIGEST}" --algorithm=nope "${ABC}" 2>"${TEST_TMPDIR}/algo.err"; then
    die "Expected exit code 1 for an unknown algorithm."
  fi
  grep -q "Unknown --algorithm 'nope'" "${TEST_TMPDIR}/algo.err" || die "Expected an unknown-algorithm message."
}

function test::no_files_is_an_error() {
  if "${DIGEST}" 2>"${TEST_TMPDIR}/nofiles.err"; then
    die "Expected exit code 1 without file arguments."
  fi
  grep -q "At least one file" "${TEST_TMPDIR}/nofiles.err" || die "Expected a usage error message."
}

test_runner
