# Copyright 2023 M. Boerger (helly25.com)
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

workspace(name = "com_helly25_mbo")

load(":workspace.bzl", "mbo_workspace_load_modules")

mbo_workspace_load_modules()

#load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

#bazel_toolchain_dependencies()

#load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

#llvm_toolchain(
#    name = "llvm_toolchain",
#    llvm_versions = {
#        "": "15.0.6",
#        "darwin-aarch64": "15.0.7",
#        "darwin-x86_64": "15.0.7",
#    },
#)

#load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

#llvm_register_toolchains()

#load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

#bazel_skylib_workspace()

# Run: ./compile_commands-update.sh
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
