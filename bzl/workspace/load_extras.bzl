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

"""Loads all extra modules used by the workspace."""

load("//bzl:archive.bzl", "github_archive")

def helly25_mbo_load_extras():
    """Loads all extra modules required by the workspace."""

    if not native.existing_rule("hedron_compile_commands"):
        github_archive(
            name = "hedron_compile_commands",
            commit = "6dd21b47db481a70c61698742438230e2399b639",
            repo = "https://github.com/helly25/bazel-compile-commands-extractor",
            sha256 = "348a643defa9ab34ed9cb2ed1dc54b1c4ffef1282240aa24c457ebd8385ff2d5",
        )

    # Minimum requirements:
    # * 2023.10.06: https://github.com/bazel-contrib/toolchains_llvm/pull/229: Move minimum supported version to Bazel
    # * 2024.03.12: https://github.com/bazel-contrib/toolchains_llvm/pull/286: Support LLD linker for Darwin
    # * 2025.01.22: https://github.com/bazel-contrib/toolchains_llvm/pull/446: Verion 1.3.0:
    #     b3c96d2dbc698eab752366bbe747e2a7df7fa504 / sha256-ZDefpvZja99JcCg37NO4dkdH11yN2zMrx2D77sWlWug=
    # * 2025.02.15: https://github.com/bazel-contrib/toolchains_llvm/pull/461: Add LLVM 19.1.4...19.1.7:
    #     0bd3bff40ab51a8e744ccddfd24f311a9df81c2d / sha256-YpBdoaSAXSatJwLcB2yzuZw5Ls/h5+dcWip+h+pVdUo=
    # In order to go past version 1.0.0 we also add the actual fix:
    # * 2024.06.06: https://github.com/bazel-contrib/toolchains_llvm/pull/337: Force Clang modules with LLVM >= 14
    if not native.existing_rule("toolchains_llvm"):
        github_archive(
            name = "toolchains_llvm",
            commit = "0bd3bff40ab51a8e744ccddfd24f311a9df81c2d",
            repo = "https://github.com/bazel-contrib/toolchains_llvm",
            integrity = "sha256-YpBdoaSAXSatJwLcB2yzuZw5Ls/h5+dcWip+h+pVdUo=",
        )
        #http_archive(
        #    name = "toolchains_llvm",
        #    sha256 = "",
        #    strip_prefix = "toolchains_llvm-1.0.0",
        #    canonical_id = "1.0.0",
        #    url = "https://github.com/bazel-contrib/toolchains_llvm/releases/download/1.0.0/toolchains_llvm-1.0.0.tar.gz",
        #)
