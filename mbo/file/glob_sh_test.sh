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

declare -a EXPECTED=(
    "dir"
    "sub"
    "sub/dir"
    "sub/dir:file1"
    "sub/dir:file2"
    "sub/two"
    "sub/two/dir"
    "sub/two/dir:file1"
    "sub/two/dir:file2"
    "sub/two/dir:file3"
)

function setup() {
    for entry in "${EXPECTED[@]}"; do
        IFS=":" read -r -a dir_file <<< "${entry}"
        if [[ "${#dir_file[@]}" -ge 1 ]]; then
            mkdir -p "${BASHTEST_TMPDIR}/${dir_file[0]}"
        fi
        if [[ "${#dir_file[@]}" -ge 2 ]]; then
            touch "${BASHTEST_TMPDIR}/${dir_file[0]}/${dir_file[1]}"
        fi
        if [[ "${#dir_file[@]}" = 1 ]] && [[ "${#dir_file[@]}" = 2 ]]; then
            die "Bad expected entry."
        fi
    done
}

setup

GLOB="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/file/glob"
declare -r GLOB
TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/mbo/file/testdata/glob_sh_test"
declare -r TESTDATA

[[ -x "${GLOB}" ]] || die "Program glob not found."

function _test_glob_and_diff() {
    NAME="${1}"
    shift
    "${GLOB}" "${@}" > "${TEST_TMPDIR}/${NAME}.out" 2>&1
    expect_files_eq "${TESTDATA}/${NAME}.txt" "${TEST_TMPDIR}/${NAME}.out"
}

function test::default() {
    _test_glob_and_diff default "${BASHTEST_TMPDIR}"
}

function test::flag_type() {
    _test_glob_and_diff flag_type "${BASHTEST_TMPDIR}" -type
}

function test::simple() {
    _test_glob_and_diff simple "${BASHTEST_TMPDIR}/*"
}

function test::select() {
    _test_glob_and_diff select "${BASHTEST_TMPDIR}/**/file[23]"
}

test_runner
