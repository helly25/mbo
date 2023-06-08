# Copyright 2023 M.Boerger
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

"""Loads modules required by the workspace."""

load("//bzl:archive.bzl", "github_archive", "http_archive")

# buildifier: disable=unnamed-macro
def mbo_workspace_load_modules():
    """Loads all modules requred by the workspace."""

    # 2022-05-27
    github_archive(
        name = "platforms",
        commit = "da5541f26b7de1dc8e04c075c99df5351742a4a2",
        repo = "https://github.com/bazelbuild/platforms",
        sha256 = "a879ea428c6d56ab0ec18224f976515948822451473a80d06c2e50af0bbe5121",
        ext = ".zip",
    )

    # Load and configure a LLVM based C/C++ toolchain.
    #github_archive(
    #    name = "com_grail_bazel_toolchain",
    #    repo = "https://github.com/grailbio/bazel-toolchain",
    #    commit = "41ff2a05c0beff439bad7acfd564e6827e34b13b",
    #    sha256 = "52b09a61b9a03f4e0994402243a03018a858da6a5774de898f99e344520e9a25",
    #)

    http_archive(
        name = "bazel_skylib",
        sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
        urls = [
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
        ],
    )

    # GoogleTest
    github_archive(
        name = "com_google_googletest",
        repo = "https://github.com/google/googletest",
        strip_prefix = "googletest-1.13.0",
        tag = "v1.13.0",
        sha256 = "ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363",
    )

    # Abseil, LTS 20230125
    github_archive(
        name = "com_google_absl",
        repo = "https://github.com/abseil/abseil-cpp",
        tag = "20230125.3",
        sha256 = "5366d7e7fa7ba0d915014d387b66d0d002c03236448e1ba9ef98122c13b35c36",
    )

    # hedron_compile_commands
    github_archive(
        name = "hedron_compile_commands",
        commit = "6f63be6e2ccfdb6a1f248abbb3614107106de4a9",
        repo = "https://github.com/helly25/bazel-compile-commands-extractor",
        sha256 = "22aa86db4c1d7c9b417f19b9a4477017d505df58eaed024e68c3452bd1a26b74",
    )
    #github_archive(
    #    name = "hedron_compile_commands",
    #    repo = "https://github.com/hedronvision/bazel-compile-commands-extractor",
    #    commit = "ed994039a951b736091776d677f324b3903ef939",
    #    sha256 = "085bde6c5212c8c1603595341ffe7133108034808d8c819f8978b2b303afc9e7",
    #)
    #native.local_repository(
    #    name = "hedron_compile_commands",
    #    path = "/Users/marcus/Documents/hedronvision_bazel-compile-commands-extractor/helly25-bcce",
    #)
