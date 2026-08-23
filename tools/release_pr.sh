#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

# Shared, testable GitHub operations used by trigger_release.sh.

function enable_version_bump_auto_merge() {
  local branch="${1}"
  local body="${2}"
  local subject="${3}"

  gh pr ready "${branch}"
  gh pr merge "${branch}" --auto --squash --body "${body}" --subject "${subject}"
}
