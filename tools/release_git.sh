#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

# Shared, testable Git operations used by trigger_release.sh.

function release_tag_exists() {
  local version="${1}"
  git show-ref --verify --quiet "refs/tags/${version}"
}

function push_release_tag() {
  local version="${1}"
  git push origin "refs/tags/${version}:refs/tags/${version}"
}
