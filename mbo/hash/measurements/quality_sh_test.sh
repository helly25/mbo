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

# Guards the generated SMHasher3 "Results" table in mbo/hash/README.md: it must
# stay in sync with the curated source of truth in hash_benchmark_report.py
# (`quality --check`), the drift guard must actually fail on a changed table (so
# it is not a no-op), and regenerating must be idempotent (the committed table is
# already in canonical form).

# shellcheck disable=SC2317 # Functions are called by the bashtest runner.

set -euo pipefail

# shellcheck disable=SC1090,SC1091,SC2154
source "${helly25_bashtest}"

REPORT="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/hash/measurements/hash_benchmark_report.py"
README="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/hash/README.md"
declare -r REPORT README

[[ -f ${REPORT} ]] || die "hash_benchmark_report.py not found."
[[ -f ${README} ]] || die "README.md not found."

PYTHON="${PYTHON:-python3}"
command -v "${PYTHON}" >/dev/null || die "python3 not found on PATH."
declare -r PYTHON

function quality() {
  "${PYTHON}" "${REPORT}" quality "$@"
}

# The committed table must match the curated data - this is the drift guard.
function test::committed_table_is_in_sync() {
  quality --check --readme "${README}" \
    || die "README SMHasher3 Results table is stale; run: hash_benchmark_report.py quality"
}

# The guard must actually catch drift: a table with a flipped score must fail.
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
  quality --readme "${copy}" >/dev/null || die "quality failed to write the table."
  cmp -s "${README}" "${copy}" || die "regenerating changed the README - it was not in canonical form."
}

test_runner
