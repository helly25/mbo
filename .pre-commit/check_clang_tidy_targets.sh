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

# Verifies that `CLANG_TIDY_MANUAL_TARGETS` in bazelmod/clang_tidy_targets.bzl is
# exactly the set of targets tagged `clang-tidy`.
#
# The list has to exist because aquery expands `//...` with `manual` targets
# already removed, so `//bazelmod:refresh_compile_commands` must name them
# explicitly to get them into `compile_commands.json`. The tag stays the
# statement of intent; this check stops the two from drifting apart when a target
# is added, removed or retagged.
#
# Skips when bazel is unavailable rather than failing, so a checkout without a
# toolchain can still commit.

set -euo pipefail

if ! command -v bazel >/dev/null 2>&1; then
  echo "clang-tidy-targets: skipped (no bazel on PATH)." 1>&2
  exit 0
fi

readonly TARGETS_BZL="bazelmod/clang_tidy_targets.bzl"

# Tagged targets, as bazel sees them. `bazel query` (unlike aquery) does report
# `manual` targets, which is what makes this check possible at all. Strip the
# `@@//` canonical-repo prefix so both sides compare as `//pkg:target`.
if ! TAGGED="$(bazel query 'attr(tags, "clang-tidy", //...)' 2>/dev/null | sed 's|^@@||' | sort)" \
  || [ -z "${TAGGED}" ]; then
  # A failed or empty query means bazel could not analyse the workspace (no
  # fetched dependencies on a lint-only CI runner, for instance). That is not the
  # same as "the list is wrong", and treating it as a mismatch would fail every
  # such job, so skip instead.
  echo "clang-tidy-targets: skipped (bazel query returned nothing; cannot verify)." 1>&2
  exit 0
fi

# The list the build actually uses. Expand `_fuzz_test_targets` from its source
# rather than duplicating rules_fuzzing's generated suffixes in this check.
LISTED="$({
  sed -n 's|^ *"\(//[^"]*\)",$|\1|p' "${TARGETS_BZL}"
  while IFS= read -r base; do
    sed -n '/^def _fuzz_test_targets/,/^CLANG_TIDY_MANUAL_TARGETS/ {
      s|^ *"\([^"]*\)",$|\1|p
    }' "${TARGETS_BZL}" | while IFS= read -r suffix; do
      printf '%s%s\n' "${base}" "${suffix}"
    done
  done < <(
    grep -o '_fuzz_test_targets("//[^"]*")' "${TARGETS_BZL}" \
      | sed 's|_fuzz_test_targets("\(//[^"]*\)")|\1|'
  )
} | sort)"

if [ "${TAGGED}" = "${LISTED}" ]; then
  exit 0
fi

echo "ERROR: ${TARGETS_BZL} and the 'clang-tidy' tag disagree." 1>&2
echo "  Tagged but not listed (these would be MISSING from compile_commands.json):" 1>&2
comm -23 <(printf '%s\n' "${TAGGED}") <(printf '%s\n' "${LISTED}") | sed 's/^/    /' 1>&2
echo "  Listed but not tagged (stale entries):" 1>&2
comm -13 <(printf '%s\n' "${TAGGED}") <(printf '%s\n' "${LISTED}") | sed 's/^/    /' 1>&2
echo "  Fix by editing ${TARGETS_BZL}, or by adding/removing the 'clang-tidy' tag." 1>&2
exit 1
