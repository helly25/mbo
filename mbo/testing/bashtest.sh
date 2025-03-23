#!/usr/bin/env bash

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

set -euo pipefail

function die() { echo >&2 "ERROR: ${*}"; exit 1; }

_BASHTEST_USAGE=$(cat <<'EOF'
bashtest.sh - A Bazel shell test runner.

- All declared functions that start with "test::" are considered as tests.
- Each test should `return 0` to indicate success or `return 1` for failure.
- Call `test_runner` at the end of the test program.
- Provides '${BASHTEST_TMPDIR}' which is a test scratch directory.
- Tests can be filtered (skipping non matching) using flag '--test_filter <pattern>'.
- Tests that use diff functionality 'expect_files_eq' can use '-u="${PWD}"' to
  update their golden files.

Usage:
  bazel test <test_target> [ --test_arg=-f=<pattern> ] [ -v ] [ -u="${PWD}" ]
  bazel run <test_target -- [ -f=<pattern> ] [ -v ][ -u="${PWD}" ]

-f --test-filter <pattern>  A glob pattern for tests to run (skip others). The
                            test is considered failing of no test was run.
-u --update <workspace>     Update golden files, assuming the <workspace>.
-v --verbose                Show additional output while tests succeed or fail.
                            Prints all test calls and file diffs.

Assertions:
  expect_eq "${LHS}" "${RHS}"
                            Asserts that two strings are the same.

  expect_ne "${LHS}" "${RHS}"
                            Asserts that two strings are different.

  expect_files_eq "${LHS}" "${RHS}"
                            Asserts that two file are the same (supports golden
                            updates).

  expect_contains "${EXPECTED}" "${ARRAY[@]}"
                            Assert that one string is present in an array.

  expect_not_contains "${EXPECTED}" "${ARRAY[@]}"
                            Assert that one string is NOT present in an array.


Status:
  test_has_error            Returns whether a test function has had an error.
  test_has_failed_tests     Returns whether a test program had previous
                            failing test functions.

Setup/Shutdown:
  test::test_init           If present, then this function runs first!
                            Test will only be executed if it succeeds.
  test::test_done           If present, then this function runs last!

Example:

```sh
source mbo/testing/bashtest.sh

function test::hello() {
    expect_ne "Hello" "World"
}

test_runner
```

In your BUILD file:

```bzl
sh_test(
    name = "my_test",
    srcs = ["my_test.sh"],
    deps = ["@com_helly25_mbo//mbo/testing:bashtest_sh"],
)
```
EOF
)

_BASHTEST_FILTER=""
_BASHTEST_UPDATE_GOLDEN=""
_BASHTEST_VERBOSE=

################################################################################
# Flag parsing:
# - short flags with values support optional value separation with '=' or ' '.
# - long form flags must have at least two characters in order to distinguish.

while getopts -- '-:f:hu:v' OPTION; do
    if [ "${OPTION}" = "-" ]; then
        OPTION="${OPTARG%%[= ]*}"
        OPTARG="${OPTARG#"${OPTION}"}"
        OPTARG="${OPTARG#[= ]}"
        OPTERR="--${OPTION}"
        if [[ "${#OPTION}" -le 1 ]]; then
            die "Long options have at least two characters. Did you mean '-${OPTION}'?"
        fi
    else
        OPTARG="${OPTARG:-}"
        OPTARG="${OPTARG//[= ]}"
        OPTERR="-${OPTION}"
    fi
    case "${OPTION}" in
        f|test[-_]filter) _BASHTEST_FILTER="${OPTARG}" ;;
        h|help) echo "${_BASHTEST_USAGE}"; return 2 ;;
        u|update[-_]golden) _BASHTEST_UPDATE_GOLDEN="${OPTARG}" ;;
        v|verbose) _BASHTEST_VERBOSE=1 ;;
        *) die "Unknown flag '${OPTERR}'." ;;
    esac
done

################################################################################
# Beyond this point we must be sourced!
[[ -z "$(caller 0)" ]] && die "The ${0} script must be sourced."

_BASHTEST_NUM_PASS=0
_BASHTEST_NUM_FAIL=0
_BASHTEST_NUM_SKIP=0

_BASHTEST_HAS_ERROR=0
_BASHTEST_SUGGEST_UPDATE=0
_BASHTEST_INIT_FAILED=0
_BASHTEST_DONE_FAILED=0

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
    _BASHTEST_HAS_ERROR=0
    if ${FUNC_NAME} && [[ "${_BASHTEST_HAS_ERROR}" == "0" ]]; then
        echo "[  PASS  ] ${TEST_NAME}"
        ((_BASHTEST_NUM_PASS+=1))
    else
        echo >&2 "[  FAIL  ] ${TEST_NAME}"
        ((_BASHTEST_NUM_FAIL+=1))
    fi
}

# Returns whether a test function has had an error.
test_has_error() {
    [[ "${_BASHTEST_HAS_ERROR}" != "0" ]]
}

# Returns whether a test program had previous failing test functions.
test_has_failed_tests() {
    [[ "${_BASHTEST_NUM_FAIL}" != "0" ]]
}

_bashtest_contains_element () {
    local expected="$1"
    shift
    for element; do [[ "${element}" == "${expected}" ]] && return 0; done
    return 1
}

################################################################################
# The test's main function that finds and runs all tests.

function test_runner() {
    if [[ -n "${_BASHTEST_FILTER}" ]]; then
        echo "Test filter: '${_BASHTEST_FILTER}'."
    fi
    echo "----------"
    TEST_FUNCS=()
    while IFS='' read -r line; do TEST_FUNCS+=("${line}"); done < <(declare -f|sed -rne 's,^test::([^ ]*).*$,\1,p')
    if _bashtest_contains_element "test_init" "${TEST_FUNCS[@]}"; then
        if ! "test::test_init"; then
            _BASHTEST_INIT_FAILED=1
            _BASHTEST_FILTER=("${TEST_FUNCS[*]}")
            echo "Test init (test::test_init) failed! Skipping all tests."
        fi
        echo "----------"
    fi
    for TEST in "${TEST_FUNCS[@]}"; do
        if [[ "${TEST}" != "test_init" ]] && [[ "${TEST}" != "test_done" ]]; then
            _bashtest_handler "test::${TEST}" "${TEST}"
            echo "----------"
        fi
    done
    if _bashtest_contains_element "test_done" "${TEST_FUNCS[@]}"; then
        if ! "test::test_done"; then
            _BASHTEST_DONE_FAILED=1
        fi
        echo "----------"
    fi
    echo "PASS: ${_BASHTEST_NUM_PASS}"
    echo "SKIP: ${_BASHTEST_NUM_SKIP}"
    echo >&2 "FAIL: ${_BASHTEST_NUM_FAIL}"
    echo "----------"
    if [[ "${_BASHTEST_INIT_FAILED}" -ne "0" ]]; then
        echo >&2 "ERROR: Test initialization failed."
        return 1
    fi
    if [[ "${_BASHTEST_NUM_FAIL}" != "0" ]]; then
        echo >&2 "ERROR: Some tests failed."
        if [[ "${_BASHTEST_SUGGEST_UPDATE}" -gt 0 ]]; then
            echo ""
            echo "Some test failures from golden file comparisons can be updated using either of:"
            echo "  > bazel test --spawn_strategy=local --test_arg=-u=\"\${PWD}\" ${TEST_TARGET:-<test_target>}"
            echo "  > bazel run ${TEST_TARGET:-<test_target>} -- -u=\"\${PWD}\""
        fi
        return 1
    elif [[ "${_BASHTEST_NUM_PASS}" == 0 ]]; then
        echo >&2 "ERROR: No tests were run."
        return 2
    fi
    if [[ "${_BASHTEST_DONE_FAILED}" != "0" ]]; then
        echo >&2 "ERROR: Test shutdown failed."
        return 1
    fi
    if [[ "${_BASHTEST_NUM_SKIP}" -gt 0 ]]; then
        echo "All selected tests passed but some tests were skipped."
        return 0
    else
        echo "All tests passed."
        return 0
    fi
}

################################################################################
# Assert that two files are the same.
#
# ```sh
# expect_files_eq <golden_file> <result_file>
# ```
#
# If the test fails it will explain how to update the golden file.

expect_files_eq() {
    FILE_GOLDEN="${1}"
    FILE_RESULT="${2}"
    shift
    shift
    FILE_OUTPUT="${FILE_RESULT}.diff"
    # Short filenames for output.
    FILE_REL_GOLDEN="${FILE_GOLDEN}"
    FILE_REL_RESULT="${FILE_RESULT}"
    FILE_REL_OUTPUT="${FILE_OUTPUT}"
    if [[ -n "${RUNFILES_DIR}${TEST_WORKSPACE}" ]]; then
        FILE_REL_GOLDEN="${FILE_REL_GOLDEN##"${RUNFILES_DIR}/${TEST_WORKSPACE}/"}"
        FILE_REL_RESULT="${FILE_REL_RESULT##"${RUNFILES_DIR}/${TEST_WORKSPACE}/"}"
        FILE_REL_OUTPUT="${FILE_REL_OUTPUT##"${RUNFILES_DIR}/${TEST_WORKSPACE}/"}"
    fi
    if [[ -n "${TEST_SRCDIR}" ]]; then
        FILE_REL_GOLDEN="${FILE_REL_GOLDEN##"${TEST_SRCDIR}/"}"
        FILE_REL_RESULT="${FILE_REL_RESULT##"${TEST_SRCDIR}/"}"
        FILE_REL_OUTPUT="${FILE_REL_OUTPUT##"${TEST_SRCDIR}/"}"
    fi
    if [[ -n "${TEST_TMPDIR}" ]]; then
        FILE_REL_GOLDEN="${FILE_REL_GOLDEN##"${TEST_TMPDIR}/"}"
        FILE_REL_RESULT="${FILE_REL_RESULT##"${TEST_TMPDIR}/"}"
        FILE_REL_OUTPUT="${FILE_REL_OUTPUT##"${TEST_TMPDIR}/"}"
    fi

    if [[ ! -r "${FILE_GOLDEN}" ]]; then
        _BASHTEST_HAS_ERROR=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: golden file readable."
        echo >&2 "  Golden:  '${FILE_REL_GOLDEN}'"
        return 1
    fi
    if [[ ! -r "${FILE_RESULT}" ]]; then
        _BASHTEST_HAS_ERROR=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: result file readable."
        echo >&2 "  Result:  '${FILE_REL_RESULT}'"
        return 1
    fi
    FILE_DIFF_GOLDEN="${FILE_GOLDEN##"${PWD}/"}"
    FILE_DIFF_RESULT="${FILE_RESULT##"${PWD}/"}"
    if ! diff -u "${FILE_DIFF_GOLDEN}" "${FILE_DIFF_RESULT}" > "${FILE_OUTPUT}" 2>&1; then
        cp "${FILE_OUTPUT}" "${FILE_OUTPUT}.tmp"
        # shellcheck disable=SC2002
        cat "${FILE_OUTPUT}.tmp" \
            | sed -re "s,^[-][-][-] ${FILE_DIFF_GOLDEN}(.*)\$,--- a/${FILE_REL_GOLDEN}\\1,g" \
            | sed -re "s,^[+][+][+] ${FILE_DIFF_RESULT}(.*)\$,+++ b/${FILE_REL_GOLDEN}\\1,g" \
            > "${FILE_OUTPUT}"
        if [[ -n "${_BASHTEST_UPDATE_GOLDEN}" ]]; then
            UPDATE_FILE="${_BASHTEST_UPDATE_GOLDEN}/${FILE_REL_GOLDEN}"
            if [[ ! -w "${UPDATE_FILE}" ]]; then
                _BASHTEST_HAS_ERROR=1
                echo >&2 ""
                echo >&2 "Test failure:"
                echo >&2 "  Expected: golden file is writable."
                echo >&2 "  Did you mean to use either:"
                echo >&2 "    > bazel test --spawn_strategy=local --test_arg=-u=\"\${PWD}\" ${TEST_TARGET:-<test_target>}"
                echo >&2 "    > bazel run ${TEST_TARGET:-<test_target>} -- -u=\"\${PWD}\""
                echo >&2 ""
                die "Cannot write golden file: '${UPDATE_FILE}'."
                # shellcheck disable=SC2317
                return 1 # Technically we die, but that could be an option.
            fi
            if cp "${FILE_RESULT}" "${UPDATE_FILE}"; then
                echo "Updated golden file '${FILE_REL_GOLDEN}'."
                return 0
            else
                _BASHTEST_HAS_ERROR=1
                echo >&2 ""
                echo >&2 "Test failure:"
                echo >&2 "  Expected: golden file gets updated."
                die "Cannot update golden file: '${UPDATE_FILE}'."
                # shellcheck disable=SC2317
                return 1  # Technically we die, but that could be an option.
            fi
        fi
        _BASHTEST_HAS_ERROR=1
        _BASHTEST_SUGGEST_UPDATE=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: files are equal:"
        echo >&2 "  Golden:  '${FILE_REL_GOLDEN}'"
        echo >&2 "  Result:  '${FILE_REL_RESULT}'"
        echo >&2 "  Diff:    '${FILE_REL_OUTPUT}'"
        if [[ -n "${_BASHTEST_VERBOSE}" ]]; then
            echo >&2 "Diff output:"
            cat "${FILE_OUTPUT}" 2>&1
            echo >&2 "----------------------------------------"
        fi
        return 1
    fi
    if [[ -n "${_BASHTEST_VERBOSE}" ]]; then
        echo ""
        echo "Test success:"
        echo "  Result equals golden: '${FILE_REL_GOLDEN}'."
    fi
    return 0
}

################################################################################
# Assert that two string values are the same.
#
# ```sh
# expect_eq <golden_text> <result_text>
# ```

expect_eq() {
    TEXT_GOLDEN="${1}"
    TEXT_RESULT="${2}"
    shift
    shift
    TEXT_GOLDEN_OUT="${TEXT_GOLDEN}"
    TEXT_RESULT_OUT="${TEXT_RESULT}"
    if [[ "${#TEXT_GOLDEN_OUT}" -ge 50 ]]; then
        TEXT_GOLDEN_OUT="${TEXT_GOLDEN_OUT:0:47}..."
    fi
    if [[ "${#TEXT_RESULT_OUT}" -ge 50 ]]; then
        TEXT_RESULT_OUT="${TEXT_RESULT_OUT:0:47}..."
    fi
    if [[ "${TEXT_GOLDEN}" != "${TEXT_RESULT}" ]]; then
        _BASHTEST_HAS_ERROR=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: strings are equal:"
        echo >&2 "  Golden:  '${TEXT_GOLDEN_OUT}'"
        echo >&2 "  Result:  '${TEXT_RESULT_OUT}'"
        return 1
    else
        if [[ -n "${_BASHTEST_VERBOSE}" ]]; then
            echo ""
            echo "Test success:"
            echo "  Result equals golden: '${TEXT_GOLDEN_OUT}'."
        fi
        return 0
    fi
}

################################################################################
# Assert that two string values are different.
#
# ```sh
# expect_ne <golden_text> <result_text>
# ```

expect_ne() {
    TEXT_GOLDEN="${1}"
    TEXT_RESULT="${2}"
    shift
    shift
    TEXT_GOLDEN_OUT="${TEXT_GOLDEN}"
    TEXT_RESULT_OUT="${TEXT_RESULT}"
    if [[ "${#TEXT_GOLDEN_OUT}" -ge 50 ]]; then
        TEXT_GOLDEN_OUT="${TEXT_GOLDEN_OUT:0:47}..."
    fi
    if [[ "${#TEXT_RESULT_OUT}" -ge 50 ]]; then
        TEXT_RESULT_OUT="${TEXT_RESULT_OUT:0:47}..."
    fi
    if [[ "${TEXT_GOLDEN}" == "${TEXT_RESULT}" ]]; then
        _BASHTEST_HAS_ERROR=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: strings are different:"
        echo >&2 "  Golden:  '${TEXT_GOLDEN_OUT}'"
        echo >&2 "  Result:  '${TEXT_RESULT_OUT}'"
        return 1
    else
        if [[ -n "${_BASHTEST_VERBOSE}" ]]; then
            echo ""
            echo "Test success:"
            echo "  Golden: '${TEXT_GOLDEN_OUT}'."
            echo "  Result: '${TEXT_RESULT_OUT}'."
        fi
        return 0
    fi
}

################################################################################
# Assert that one string is present in an array.
#
# ```sh
# expect_contains ${ELEMENT}" "${ARRAY[@]}"
# ```

expect_contains() {
    ELEMENT="${1}"
    shift
    if _bashtest_contains_element "${ELEMENT}" "${@}"; then
        if [[ -n "${_BASHTEST_VERBOSE}" ]]; then
            echo ""
            echo "Test success:"
            echo "  Expected: element is present in array: '${ELEMENT}'."
        fi
        return 0
    else
        _BASHTEST_HAS_ERROR=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: element is present in array:"
        echo >&2 "  Element:  '${ELEMENT}'"
        echo >&2 "  Array:    '${*}'"
        return 1
    fi
}

################################################################################
# Assert that one string is NOT present in an array.
#
# ```sh
# expect_not_contains ${ELEMENT}" "${ARRAY[@]}"
# ```

expect_not_contains() {
    ELEMENT="${1}"
    shift
    if ! _bashtest_contains_element "${ELEMENT}" "${@}"; then
        if [[ -n "${_BASHTEST_VERBOSE}" ]]; then
            echo ""
            echo "Test success:"
            echo "  Expected: element is NOT present in array: '${ELEMENT}'."
        fi
        return 0
    else
        _BASHTEST_HAS_ERROR=1
        echo >&2 ""
        echo >&2 "Test failure:"
        echo >&2 "  Expected: element is NOT present in array:"
        echo >&2 "  Element:  '${ELEMENT}'"
        echo >&2 "  Array:    '${*}'"
        return 1
    fi
}
