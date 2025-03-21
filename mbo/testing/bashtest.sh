#!/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

# BashTest:  EXPERIMENTAL - see `bashtest.sh -h`
#

set -euo pipefail

function die() { echo >&2 "ERROR: ${*}"; exit 1; }

_BASHTEST_USAGE=$(cat <<'EOF'
bashtest.sh - A Bazel shell test runner.

- All declared functions that start with "test::" are considered as tests.
- Each test should `return 0` to indicate success or `return 1` for failure.
- Call `test_runner` at the end of the test program.
- Provides '${BASHTEST_TMPDIR}' which is a test scratch directory.
- Tests can be filtered (skipping non matching) using flag '--filter <pattern>'.

Example:

```sh
source mbo/testing/bashtest.sh

function test::hello() {
    echo "Hello"
    return 0
}

test_runner
```
EOF
)

while getopts -- '-:f:h' OPTION; do
    OPTERR="-${OPTION}"
    if [ "${OPTION}" = "-" ]; then
        OPTION="${OPTARG%%[= ]*}"
        OPTARG="${OPTARG#"${OPTION}"}"
        OPTARG="${OPTARG#=}"
        OPTERR="--${OPTION}"
    fi
    case "${OPTION}" in
        f|filter) _BASHTEST_FILTER="${OPTARG//[= ]}" ;;
        h|help) echo "${_BASHTEST_USAGE}"; exit 2 ;;
        *) die "Unknown flag '${OPTERR}'." ;;
    esac
done

################################################################################
# Beyond this point we must be sourced!
[[ -z "$(caller 0)" ]] && die "The ${0} script must be sourced."

_BASHTEST_NUM_PASS=0
_BASHTEST_NUM_FAIL=0
_BASHTEST_NUM_SKIP=0
_BASHTEST_FILTER=""

function _bashtest_cleanup () {
    # Bazel sandboxing will delete anyway unless `--sandbox_debug` is used.
    if [[ "${_BASHTEST_NUM_FAIL}" == "0" ]]; then
        rm -rf "${BASHTEST_TMPDIR}"
    fi
}
trap _bashtest_cleanup EXIT HUP INT QUIT TERM

mkdir -p "${TEST_TMPDIR:=/tmp/$(date "+%Y%m%dT%H%M%S")}"
BASHTEST_TMPDIR="$(mktemp -d -p "${TEST_TMPDIR}")"
declare -r BASHTEST_TMPDIR

################################################################################
function _bashtest_handler() {
    FUNC_NAME="${1}"
    TEST_NAME="${2}"
    if [[ -n "${_BASHTEST_FILTER}" ]] && ! [[ ${TEST_NAME} =~ ${_BASHTEST_FILTER} ]]; then
        echo "[  SKIP  ] ${TEST_NAME}"
        ((_BASHTEST_NUM_SKIP+=1))
        return
    fi
    echo "[  TEST  ] ${TEST_NAME}"
    if ${FUNC_NAME}; then
        echo "[  PASS  ] ${TEST_NAME}"
        ((_BASHTEST_NUM_PASS+=1))
    else
        echo >&2 "[  FAIL  ] ${TEST_NAME}"
        ((_BASHTEST_NUM_FAIL+=1))
    fi
}

function test_runner() {
    if [[ -n "${_BASHTEST_FILTER}" ]]; then
        echo "Test filter: '${_BASHTEST_FILTER}'."
    fi
    echo "----------"
    TEST_FUNCS=()
    while IFS='' read -r line; do TEST_FUNCS+=("${line}"); done < <(declare -f|sed -rne 's,^test::([^ ]*).*$,\1,p')
    for TEST in "${TEST_FUNCS[@]}"; do
        _bashtest_handler "test::${TEST}" "${TEST}"
        echo "----------"
    done
    echo "PASS: ${_BASHTEST_NUM_PASS}"
    echo "SKIP: ${_BASHTEST_NUM_SKIP}"
    echo >&2 "FAIL: ${_BASHTEST_NUM_FAIL}"
    echo "----------"
    if [[ "${_BASHTEST_NUM_FAIL}" -gt 0 ]]; then
        echo >&2 "ERROR: Some tests failed."
        exit 1
    elif [[ "${_BASHTEST_NUM_PASS}" == 0 ]]; then
        echo >&2 "ERROR: No tests were run."
        exit 2
    elif [[ "${_BASHTEST_NUM_SKIP}" -gt 0 ]]; then
        echo "All selected tests passed but some tests were skipped."
        exit 0
    else
        echo "All tests passed."
        exit 0
    fi
}
