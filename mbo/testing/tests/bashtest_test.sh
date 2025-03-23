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

# shellcheck disable=SC2317 # Functions are called bashtest

set -euo pipefail

# shellcheck disable=SC1091
source "mbo/testing/bashtest.sh"

bad_bashtest() {
    echo >&2 "FATAL: Test functionality broken: ${*}"
    exit 1
}

_DONE_CALLED=0
_INIT_CALLED=0

test::test_init() {
    echo "Hello World of tests."
    _INIT_CALLED=1
}

test::test_done() {
    echo "The world is ending."
    _DONE_CALLED=1
}

test::expect_eq() {
    [[ "${_BASHTEST_HAS_ERROR}" == "0" ]] || bad_bashtest "Test starts with _BASHTEST_HAS_ERROR != 0, got '${_BASHTEST_HAS_ERROR}'."

    NUM_PASS="${_BASHTEST_NUM_PASS}"
    NUM_FAIL="${_BASHTEST_NUM_FAIL}"
    NUM_SKIP="${_BASHTEST_NUM_SKIP}"

    expect_eq "Hello" "Hello"
    test_has_error && bad_bashtest "expect_eq with same values."
    _BASHTEST_HAS_ERROR=0

    expect_eq "Hello" "World"
    test_has_error || bad_bashtest "expect_eq with different values."
    _BASHTEST_HAS_ERROR=0

    expect_eq "Hello" "City"
    test_has_error || bad_bashtest "expect_eq with different values."
    _BASHTEST_HAS_ERROR=0

    expect_eq "Meow" "Meow"
    test_has_error && bad_bashtest "expect_eq with different values."
    _BASHTEST_HAS_ERROR=0

    [[ "${_BASHTEST_NUM_PASS}" == "${NUM_PASS}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_PASS."
    [[ "${_BASHTEST_NUM_FAIL}" == "${NUM_FAIL}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_FAIL."
    [[ "${_BASHTEST_NUM_SKIP}" == "${NUM_SKIP}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_SKIP."
    _BASHTEST_NUM_PASS="${NUM_PASS}"
    _BASHTEST_NUM_FAIL="${NUM_FAIL}"
    _BASHTEST_NUM_SKIP="${NUM_SKIP}"
}

test::expect_ne() {
    [[ "${_BASHTEST_HAS_ERROR}" == "0" ]] || bad_bashtest "Test starts with _BASHTEST_HAS_ERROR != 0, got '${_BASHTEST_HAS_ERROR}'."

    NUM_PASS="${_BASHTEST_NUM_PASS}"
    NUM_FAIL="${_BASHTEST_NUM_FAIL}"
    NUM_SKIP="${_BASHTEST_NUM_SKIP}"

    expect_ne "Hello" "Hello"
    test_has_error || bad_bashtest "expect_ne with same values."
    _BASHTEST_HAS_ERROR=0

    expect_ne "Hello" "World"
    test_has_error && bad_bashtest "expect_ne with different values."
    _BASHTEST_HAS_ERROR=0

    expect_ne "Hello" "City"
    test_has_error && bad_bashtest "expect_ne with different values."
    _BASHTEST_HAS_ERROR=0

    expect_ne "Meow" "Meow"
    test_has_error || bad_bashtest "expect_ne with different values."
    _BASHTEST_HAS_ERROR=0

    [[ "${_BASHTEST_NUM_PASS}" == "${NUM_PASS}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_PASS."
    [[ "${_BASHTEST_NUM_FAIL}" == "${NUM_FAIL}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_FAIL."
    [[ "${_BASHTEST_NUM_SKIP}" == "${NUM_SKIP}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_SKIP."
    _BASHTEST_NUM_PASS="${NUM_PASS}"
    _BASHTEST_NUM_FAIL="${NUM_FAIL}"
    _BASHTEST_NUM_SKIP="${NUM_SKIP}"
}

test::expect_ne() {
    [[ "${_BASHTEST_HAS_ERROR}" == "0" ]] || bad_bashtest "Test starts with _BASHTEST_HAS_ERROR != 0, got '${_BASHTEST_HAS_ERROR}'."

    NUM_PASS="${_BASHTEST_NUM_PASS}"
    NUM_FAIL="${_BASHTEST_NUM_FAIL}"
    NUM_SKIP="${_BASHTEST_NUM_SKIP}"

    EMPTY=()
    expect_contains "foo" "${EMPTY[@]}" && bad_bashtest "Element is not present, yet test passed."
    _BASHTEST_HAS_ERROR=0

    expect_contains "" "${EMPTY[@]}" && bad_bashtest "Element is not present, yet test passed."
    _BASHTEST_HAS_ERROR=0

    FOO_BAR=("foo" "bar")
    expect_contains "foo" "${FOO_BAR[@]}" || bad_bashtest "Element is present, yet test failed."
    _BASHTEST_HAS_ERROR=0

    expect_contains "bar" "${FOO_BAR[@]}" || bad_bashtest "Element is present, yet test failed."
    _BASHTEST_HAS_ERROR=0

    expect_contains "baz" "${FOO_BAR[@]}" && bad_bashtest "Element is not present, yet test passed."
    _BASHTEST_HAS_ERROR=0

    expect_contains "" "${FOO_BAR[@]}" && bad_bashtest "Element is not present, yet test passed."
    _BASHTEST_HAS_ERROR=0

    FOO_BAR+=("")

    expect_contains "" "${FOO_BAR[@]}" || bad_bashtest "Element is present, yet test failed."
    _BASHTEST_HAS_ERROR=0

    [[ "${_BASHTEST_NUM_PASS}" == "${NUM_PASS}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_PASS."
    [[ "${_BASHTEST_NUM_FAIL}" == "${NUM_FAIL}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_FAIL."
    [[ "${_BASHTEST_NUM_SKIP}" == "${NUM_SKIP}" ]] || bad_bashtest "Test modified _BASHTEST_NUM_SKIP."
    _BASHTEST_NUM_PASS="${NUM_PASS}"
    _BASHTEST_NUM_FAIL="${NUM_FAIL}"
    _BASHTEST_NUM_SKIP="${NUM_SKIP}"
}

test_runner || bad_bashtest "Test was meant to pass but did not."

[[ "${_INIT_CALLED}" == "1" ]] || bad_bashtest "Test init (test::test_init) was not called."
[[ "${_DONE_CALLED}" == "1" ]] || bad_bashtest "Test done (test::test_done) was not called."
