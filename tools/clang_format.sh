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

# 1) Assume we have the `external` convenience link.
BAZEL_OUTPUT="bazel-$(basename "${PWD}")"
[ -d "${BAZEL_OUTPUT}" ] || BAZEL_OUTPUT="."

declare -a CLANG_FORMAT_LOCS=(
  # 1) Find it via the `bazel-<repo>` link structure.
  # 1.1) Bazel workspaces
  "${BAZEL_OUTPUT}/external/llvm_toolchain_llvm/bin/clang-format"
  "${BAZEL_OUTPUT}/external/llvm_toolchain/bin/clang-format"
  # 1.2) Bazel modules < 8
  "${BAZEL_OUTPUT}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm/bin/clang-format"
  "${BAZEL_OUTPUT}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm_llvm/bin/clang-format"
  # 1.3) Bazel modules >= 8
  "${BAZEL_OUTPUT}/external/toolchains_llvm++llvm+llvm_toolchain_llvm/bin/clang-format"
  "${BAZEL_OUTPUT}/external/toolchains_llvm++llvm+llvm_toolchain_llvm_llvm/bin/clang-format"

  # 2) Try to find clang-format directly or by its name plus version suffix as is common.
  "$(which "clang-format-23")"
  "$(which "clang-format-22")"
  "$(which "clang-format-21")"
  "$(which "clang-format-20")"
  "$(which "clang-format-19")"
  "$(which "clang-format-18")"
  "$(which "clang-format-17")"
  "$(which "clang-format-16")"
  "$(which "clang-format-15")"

  # 3) Prefer clang-format from LLVM_PATH or PATH environment variables if any.
  "${LLVM_PATH:-/usr}/bin/clang-format"
  "$(which clang-format)"
)

for CLANG_FORMAT in "${CLANG_FORMAT_LOCS[@]}"; do
  [ -x "${CLANG_FORMAT}" ] && break
done

[[ -x ${CLANG_FORMAT} ]] || die "Cannot find clang-format"

"${CLANG_FORMAT}" "${@}"
