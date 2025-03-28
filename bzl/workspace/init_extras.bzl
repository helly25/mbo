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

"""Initializes extra modules required by the workspace, requies helly25_mbo_init_extras_llvm."""

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
load("@toolchains_llvm//toolchain:deps.bzl", "bazel_toolchain_dependencies")
load("@toolchains_llvm//toolchain:rules.bzl", "llvm_toolchain")

def helly25_mbo_init_extras():
    """Initializes all standard modules required by the workspace."""

    hedron_compile_commands_setup()

    bazel_toolchain_dependencies()

    llvm_toolchain(
        name = "llvm_toolchain",
        llvm_version = "19.1.6",
    )
