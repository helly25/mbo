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

set -euo pipefail

function die() {
  echo "ERROR: ${*}" 1>&2
  exit 1
}

# Resolve clang-format, preferring the hermetic toolchains_llvm binary so CI and
# local runs agree on the version, ahead of any system clang-format on PATH.

# The `bazel-<repo>` convenience link points at the execroot; fall back to cwd.
BAZEL_OUTPUT="bazel-$(basename "${PWD}")"
[ -d "${BAZEL_OUTPUT}" ] || BAZEL_OUTPUT="."

# The output base holds every fetched repo, including the `--config=clang`
# dev-dependency LLVM toolchain that the execroot symlink forest may omit.
OUTPUT_BASE="$(bazel info output_base 2>/dev/null || true)"

declare -a CLANG_FORMAT_LOCS=(
  # 1) Hermetic toolchain via the `bazel-<repo>` execroot link.
  # 1.1) Bazel workspaces
  "${BAZEL_OUTPUT}/external/llvm_toolchain_llvm/bin/clang-format"
  "${BAZEL_OUTPUT}/external/llvm_toolchain/bin/clang-format"
  # 1.2) Bazel modules < 8
  "${BAZEL_OUTPUT}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm/bin/clang-format"
  "${BAZEL_OUTPUT}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm_llvm/bin/clang-format"
  # 1.3) Bazel modules >= 8
  "${BAZEL_OUTPUT}/external/toolchains_llvm++llvm+llvm_toolchain_llvm/bin/clang-format"
  "${BAZEL_OUTPUT}/external/toolchains_llvm++llvm+llvm_toolchain_llvm_llvm/bin/clang-format"

  # 2) Same hermetic toolchain via the output base (covers the dev-dependency
  #    case where it is absent from the execroot symlink forest). Still hermetic,
  #    so this is tried BEFORE any system clang-format below.
  "${OUTPUT_BASE:+${OUTPUT_BASE}/external/toolchains_llvm++llvm+llvm_toolchain_llvm/bin/clang-format}"
  "${OUTPUT_BASE:+${OUTPUT_BASE}/external/toolchains_llvm++llvm+llvm_toolchain_llvm_llvm/bin/clang-format}"

  # 3) System clang-format by versioned name (its version may differ from the
  #    hermetic toolchain, so it is a last resort).
  "$(which "clang-format-23" 2>/dev/null || true)"
  "$(which "clang-format-22" 2>/dev/null || true)"
  "$(which "clang-format-21" 2>/dev/null || true)"
  "$(which "clang-format-20" 2>/dev/null || true)"
  "$(which "clang-format-19" 2>/dev/null || true)"
  "$(which "clang-format-18" 2>/dev/null || true)"
  "$(which "clang-format-17" 2>/dev/null || true)"
  "$(which "clang-format-16" 2>/dev/null || true)"
  "$(which "clang-format-15" 2>/dev/null || true)"

  # 4) LLVM_PATH or a plain clang-format on PATH.
  "${LLVM_PATH:-/usr}/bin/clang-format"
  "$(which clang-format 2>/dev/null || true)"
)

CLANG_FORMAT=""
for LOC in "${CLANG_FORMAT_LOCS[@]}"; do
  if [ -n "${LOC}" ] && [ -x "${LOC}" ]; then
    CLANG_FORMAT="${LOC}"
    break
  fi
done

[ -n "${CLANG_FORMAT}" ] || die "Cannot find clang-format (build once with --config=clang so the hermetic toolchain is fetched, or install clang-format)"

exec "${CLANG_FORMAT}" "${@}"
