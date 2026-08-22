#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

readonly COMPARISON='\b(ASSERT|EXPECT)_(EQ|NE|LT|LE|GT|GE|STREQ|STRNE|STRCASEEQ|STRCASENE|FLOAT_EQ|DOUBLE_EQ|NEAR)[[:space:]]*\('
readonly QUALIFIED='(::mbo::)?testing::[A-Z][A-Za-z0-9_]*\('
readonly QUALIFIED_UTILITY='testing::(ExplainMatchResult|MakePolymorphicMatcher|PrintToString|Return|RunfilesDirOrDie|TempDir|Test|TestWithParam|Values)\('

status=0
for file in "$@"; do
  source="$(sed 's|//.*||' "${file}")"
  comparisons="$(grep -nE "${COMPARISON}" <<<"${source}" || true)"
  qualified=""
  if [[ ${file} != mbo/testing/*.h ]]; then
    qualified="$(grep -nE "${QUALIFIED}" <<<"${source}" | grep -vE "${QUALIFIED_UTILITY}" || true)"
  fi
  if [[ -n ${comparisons} ]]; then
    echo "${file}: use EXPECT_THAT / ASSERT_THAT and a matcher:" >&2
    echo "${comparisons}" >&2
    status=1
  fi
  if [[ -n ${qualified} ]]; then
    echo "${file}: import matchers with a using declaration instead of qualifying them inline:" >&2
    echo "${qualified}" >&2
    status=1
  fi
done
exit "${status}"
