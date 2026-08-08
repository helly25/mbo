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

# Generate `compile_commands.json` describing the HERMETIC clang, so that clangd
# and `tools/clang_tidy.sh` parse with the same compiler the `--config=clang`
# builds use. Note that `--config=clang` cannot do this: it only configures the
# build of the extractor tool itself, and never reaches the `aquery` the tool
# runs internally, so the recorded commands still named the autodetected local
# (Apple) clang. The fork's runtime flags below are the supported override; they
# must follow `--`, or `bazel run` hands them to bazel rather than to the tool.

set -euo pipefail

function die() {
  echo "ERROR: ${*}" 1>&2
  exit 1
}

OUTPUT_BASE="$(bazel info output_base 2>/dev/null || true)"
[ -n "${OUTPUT_BASE}" ] || die "'bazel info output_base' failed; is bazel on PATH?"

# The hermetic LLVM install (headers + libs) lives in the `_llvm_llvm` repo; the
# plain `_llvm` repo only re-exports clang-format/clang-tidy/clangd.
declare -a CLANG_LOCS=(
  "${OUTPUT_BASE}/external/toolchains_llvm++llvm+llvm_toolchain_llvm_llvm/bin/clang++"
  "${OUTPUT_BASE}/external/toolchains_llvm~~llvm~llvm_toolchain_llvm_llvm/bin/clang++"
  "${OUTPUT_BASE}/external/llvm_toolchain_llvm/bin/clang++"
)

CLANG=""
function resolve_clang() {
  for LOC in "${CLANG_LOCS[@]}"; do
    if [ -x "${LOC}" ]; then
      CLANG="${LOC}"
      return
    fi
  done
}

resolve_clang
if [ -z "${CLANG}" ]; then
  # A fresh checkout (or CI runner) has not materialized the toolchain yet: it is
  # only fetched once something is actually built with `--config=clang`. Build the
  # smallest cc target there is to trigger that, then look again.
  echo "Hermetic clang++ not present; fetching the toolchain via a probe build ..." 1>&2
  bazel build --config=clang //tools:show_compiler >/dev/null \
    || die "probe build '//tools:show_compiler --config=clang' failed; cannot fetch the LLVM toolchain"
  resolve_clang
fi

[ -n "${CLANG}" ] || die "Cannot find the hermetic clang++ even after a '--config=clang' build"

# Sources reachable both normally and through a build-machine tool are compiled
# twice (target + exec configuration), and both commands would be emitted. Keep
# only the target-configuration one: clang-tidy works per entry, so the exec copy
# is duplicate linting for a near-identical result. Files compiled ONLY in the
# exec configuration keep their command, so nothing leaves the compile DB.
declare -a BCCE_ARGS=("--bcce-compiler=${CLANG}" "--bcce-prefer-target-config")

# The hermetic clang carries its own libc++ but no system C headers: without the
# SDK sysroot its <locale> support headers fail on `'time.h' file not found`.
# The bazel `--config=clang` toolchain supplies this itself; the extracted
# commands come from the autodetected toolchain, which relies on Apple clang's
# built-in default, so it has to be made explicit here. Linux needs no such flag.
if [ "$(uname -s)" = "Darwin" ]; then
  SDKROOT_PATH="$(xcrun --show-sdk-path 2>/dev/null || true)"
  [ -n "${SDKROOT_PATH}" ] || die "'xcrun --show-sdk-path' failed; install the Xcode command line tools"
  BCCE_ARGS+=("--bcce-copt=-isysroot${SDKROOT_PATH}")
fi

bazel run @bazel_compile_commands_extractor//:refresh_all -- "${BCCE_ARGS[@]}"
echo "OK"
