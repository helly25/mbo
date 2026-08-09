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

# Generate `compile_commands.json` so that clangd and `tools/clang_tidy.sh` parse
# in the configuration development actually targets.
#
# The extraction runs through `//bazelmod:refresh_compile_commands`, whose target
# list carries `--config=clang-tidy` (`.bazelrc`: `--config=clang` plus an
# explicit libc++). That is what puts the flags right at the source, so no
# compiler override is needed: passing `--config=clang` on THIS command line
# would not work, as it only configures the build of the extractor tool itself
# and never reaches the `aquery` the tool runs internally.
#
# The runtime `--bcce-*` flags below must follow `--`, or `bazel run` hands them
# to bazel rather than to the tool.

set -euo pipefail

function die() {
  echo "ERROR: ${*}" 1>&2
  exit 1
}

# Sources reachable both normally and through a build-machine tool are compiled
# twice (target + exec configuration), and both commands would be emitted. Keep
# only the target-configuration one: clang-tidy works per entry, so the exec copy
# is duplicate linting for a near-identical result. Files compiled ONLY in the
# exec configuration keep their command, so nothing leaves the compile DB.
declare -a BCCE_ARGS=("--bcce-prefer-target-config")

# macOS only: the hermetic clang carries its own libc++ but no system C headers,
# so without the SDK sysroot its <locale> support headers fail on
# `'time.h' file not found`. Linux needs no such flag.
if [ "$(uname -s)" = "Darwin" ]; then
  SDKROOT_PATH="$(xcrun --show-sdk-path 2>/dev/null || true)"
  [ -n "${SDKROOT_PATH}" ] || die "'xcrun --show-sdk-path' failed; install the Xcode command line tools"
  BCCE_ARGS+=("--bcce-copt=-isysroot${SDKROOT_PATH}")
fi

bazel run //bazelmod:refresh_compile_commands -- "${BCCE_ARGS[@]}"
echo "OK"
