#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

# shellcheck disable=SC2317 # Functions are called by the bashtest runner.

set -euo pipefail

# shellcheck disable=SC1090,SC1091,SC2154
source "${helly25_bashtest}"

RELEASE_GIT="${TEST_SRCDIR}/${TEST_WORKSPACE}/tools/release_git.sh"
declare -r RELEASE_GIT
# shellcheck source=tools/release_git.sh
source "${RELEASE_GIT}"

function make_repository() {
  local root
  root="${BASHTEST_TMPDIR}/${1}"
  git init --bare "${root}/remote.git" >/dev/null
  git init "${root}/local" >/dev/null
  git -C "${root}/local" config user.email "release-test@example.com"
  git -C "${root}/local" config user.name "Release Test"
  git -C "${root}/local" commit --allow-empty -m initial >/dev/null
  git -C "${root}/local" remote add origin "${root}/remote.git"
  echo "${root}"
}

function test::tag_lookup_is_exact() {
  local root
  root="$(make_repository tag_lookup)"
  git -C "${root}/local" tag 1.2.30

  if (cd "${root}/local" && release_tag_exists 1.2.3); then
    die "A prefix of an existing tag was reported as present."
  fi
  (cd "${root}/local" && release_tag_exists 1.2.30) || die "The exact tag was not found."
}

function test::push_publishes_only_requested_tag() {
  local root
  root="$(make_repository selective_push)"
  git -C "${root}/local" tag 1.2.3
  git -C "${root}/local" tag 9.9.9

  (cd "${root}/local" && push_release_tag 1.2.3)

  git --git-dir="${root}/remote.git" show-ref --verify --quiet refs/tags/1.2.3 \
    || die "Requested release tag was not published."
  if git --git-dir="${root}/remote.git" show-ref --verify --quiet refs/tags/9.9.9; then
    die "Unrelated local tag was published."
  fi
}

test_runner
