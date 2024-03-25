# Copyright M. Boerger (helly25.com)
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

###########################################################################

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

###########################################################################

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

###########################################################################

load("@toolchains_llvm//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@toolchains_llvm//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_versions = {
        "linux-aarch64": "17.0.6",
        "linux-x86_64": "17.0.6",
        "darwin-aarch64": "17.0.6",
        "darwin-x86_64": "15.0.7",  # For Intel based macs.
    },
    sha256 = {
        "linux-aarch64": "6dd62762285326f223f40b8e4f2864b5c372de3f7de0731cb7cd55ca5287b75a",
        "limux-x86_64": "884ee67d647d77e58740c1e645649e29ae9e8a6fe87c1376be0f3a30f3cc9ab3",
        "darwin-aarch64": "1264eb3c2a4a6d5e9354c3e5dc5cb6c6481e678f6456f36d2e0e566e9400fcad",
        "darwin-x86_64": "d16b6d536364c5bec6583d12dd7e6cf841b9f508c4430d9ee886726bd9983f1c",
    },
    strip_prefix = {
        "linux-aarch64": "clang+llvm-17.0.6-aarch64-linux-gnu",
        "linux-x86_64": "clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04",
        "darwin-aarch64": "clang+llvm-17.0.6-arm64-apple-darwin22.0",
        "darwin-x86_64": "clang+llvm-15.0.7-x86_64-apple-darwin21.0",
    },
    urls = {
        "linux-aarch64": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-aarch64-linux-gnu.tar.xz"],
        "linux-x86_64": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz"],
        "darwin-aarch64": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-arm64-apple-darwin22.0.tar.xz"],
        "darwin-x86_64": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/clang+llvm-15.0.7-x86_64-apple-darwin21.0.tar.xz"],
    },
)

load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

llvm_register_toolchains()

###########################################################################

# Run: ./compile_commands-update.sh
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()

###########################################################################
