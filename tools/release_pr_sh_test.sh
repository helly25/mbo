#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

# shellcheck disable=SC2317 # Functions are called by the bashtest runner.

set -euo pipefail

# shellcheck disable=SC1090,SC1091,SC2154
source "${helly25_bashtest}"

RELEASE_PR="${TEST_SRCDIR}/${TEST_WORKSPACE}/tools/release_pr.sh"
FAKE_GH="${TEST_SRCDIR}/${TEST_WORKSPACE}/tools/testdata/fake_gh.sh"
declare -r RELEASE_PR FAKE_GH
# shellcheck source=tools/release_pr.sh
source "${RELEASE_PR}"

function test::auto_merge_preserves_review_and_branch_protection() {
  local actual="${BASHTEST_TMPDIR}/actual.txt"
  local expected="${BASHTEST_TMPDIR}/expected.txt"
  local fake_bin="${BASHTEST_TMPDIR}/bin"
  mkdir -p "${fake_bin}"
  ln -s "${FAKE_GH}" "${fake_bin}/gh"

  GH_LOG="${actual}" PATH="${fake_bin}:${PATH}" \
    enable_version_bump_auto_merge "chore/bump_version_to_1.2.4" \
    "Automated version bump from 1.2.3 to 1.2.4 by trigger script." \
    "Bump version from 1.2.3 to 1.2.4"

  {
    echo "pr ready chore/bump_version_to_1.2.4"
    echo "pr merge chore/bump_version_to_1.2.4 --auto --squash --body Automated version bump from 1.2.3 to 1.2.4 by trigger script. --subject Bump version from 1.2.3 to 1.2.4"
  } >"${expected}"
  expect_files_eq "${expected}" "${actual}"
}

test_runner
