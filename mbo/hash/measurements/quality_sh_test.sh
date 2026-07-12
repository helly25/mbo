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

# Guards the generated SMHasher3 quality tables in mbo/hash/README.md (the
# Algorithm-overview and the Results table), rendered by
# `hash_benchmark_report.py quality` from hash_algorithms.json (manual data) x a
# measured data bundle. Asserts: the committed tables are in sync (`quality
# --check`); the drift guard actually fails on a changed table; regenerating is
# idempotent; and all committed bundles agree on their measurements
# (`consistency`, since SMHasher3 verdicts are machine-independent).

# shellcheck disable=SC2317 # Functions are called by the bashtest runner.

set -euo pipefail

# shellcheck disable=SC1090,SC1091,SC2154
source "${helly25_bashtest}"

MEAS="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/hash/measurements"
REPORT="${MEAS}/hash_benchmark_report.py"
README="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/hash/README.md"
declare -r MEAS REPORT README

[[ -f ${REPORT} ]] || die "hash_benchmark_report.py not found."
[[ -f ${README} ]] || die "README.md not found."

# The data bundles are Git-LFS: CI materializes them (see test.yml) and a local
# run needs `git lfs pull`. An unmaterialized pointer is tiny, so guard on size.
BUNDLES=("${MEAS}"/data/*.tgz)
[[ -f ${BUNDLES[0]} ]] || die "no data bundles found (run: git lfs pull)."
[[ $(wc -c <"${BUNDLES[0]}") -gt 1024 ]] || die "bundle is a Git-LFS pointer; run: git lfs pull."
declare -r BUNDLE="${BUNDLES[0]}"

PYTHON="${PYTHON:-python3}"
command -v "${PYTHON}" >/dev/null || die "python3 not found on PATH."
declare -r PYTHON

function quality() {
  "${PYTHON}" "${REPORT}" quality --bundle "${BUNDLE}" "$@"
}

# The committed tables must match hash_algorithms.json x the bundle measurements.
function test::committed_tables_are_in_sync() {
  quality --check --readme "${README}" \
    || die "README quality tables are stale; run: hash_benchmark_report.py quality --bundle <bundle>"
}

# The guard must actually catch drift: a flipped score must fail --check.
function test::check_detects_drift() {
  local drift="${TEST_TMPDIR}/drift_README.md"
  sed 's#| 7/186 #| 9/186 #' "${README}" >"${drift}"
  cmp -s "${README}" "${drift}" && die "drift fixture is identical - the score edit did not match."
  if quality --check --readme "${drift}" 2>/dev/null; then
    die "quality --check passed on a changed table; the drift guard is a no-op."
  fi
}

# Regenerating into a copy of the committed README must change nothing.
function test::regenerate_is_idempotent() {
  local copy="${TEST_TMPDIR}/regen_README.md"
  cp "${README}" "${copy}"
  quality --readme "${copy}" >/dev/null 2>&1 || die "quality failed to write the tables."
  cmp -s "${README}" "${copy}" || die "regenerating changed the README - it was not in canonical form."
}

# All committed bundles at the same source SHA must report identical measurements.
function test::bundles_agree_per_source_sha() {
  "${PYTHON}" "${REPORT}" consistency --bundles "${BUNDLES[@]}" \
    || die "committed bundles disagree on SMHasher3 measurements."
}

test_runner
