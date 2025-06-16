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

"""Loads all standard modules required by the workspace."""

load("//bzl:archive.bzl", "github_archive", "http_archive")

# buildifier: disable=unnamed-macro
def helly25_mbo_load_modules():
    """Loads all standard modules required by the workspace."""

    if not native.existing_rule("platforms"):
        http_archive(
            name = "platforms",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
                "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
            ],
            sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
        )

    if not native.existing_rule("rules_license"):
        http_archive(
            name = "rules_license",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_license/releases/download/0.0.7/rules_license-0.0.7.tar.gz",
                "https://github.com/bazelbuild/rules_license/releases/download/0.0.7/rules_license-0.0.7.tar.gz",
            ],
            sha256 = "4531deccb913639c30e5c7512a054d5d875698daeb75d8cf90f284375fe7c360",
        )

    if not native.existing_rule("bazel_skylib"):
        http_archive(
            name = "bazel_skylib",
            sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
            urls = [
                "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
                "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
            ],
        )

    if not native.existing_rule("bazel_features"):
        http_archive(
            name = "bazel_features",
            sha256 = "4fd9922d464686820ffd8fcefa28ccffa147f7cdc6b6ac0d8b07fde565c65d66",
            strip_prefix = "bazel_features-1.25.0",
            url = "https://github.com/bazel-contrib/bazel_features/releases/download/v1.25.0/bazel_features-v1.25.0.tar.gz",
        )

    if not native.existing_rule("rules_foreign_cc"):
        http_archive(
            name = "rules_foreign_cc",
            sha256 = "476303bd0f1b04cc311fc258f1708a5f6ef82d3091e53fd1977fa20383425a6a",
            strip_prefix = "rules_foreign_cc-0.10.1",
            url = "https://github.com/bazelbuild/rules_foreign_cc/releases/download/0.10.1/rules_foreign_cc-0.10.1.tar.gz",
        )

    # Used for absl/GoogleTest
    # Note GoogleTest uses "com_googlesource_code_re2" rather than "com_google_re2"
    if not native.existing_rule("com_googlesource_code_re2"):
        http_archive(
            name = "com_googlesource_code_re2",
            strip_prefix = "re2-2023-06-02",
            urls = ["https://github.com/google/re2/archive/refs/tags/2023-06-02.tar.gz"],
            sha256 = "4ccdd5aafaa1bcc24181e6dd3581c3eee0354734bb9f3cb4306273ffa434b94f",
        )

    # Abseil, LTS
    # Used for GoogleTest through .bazelrc "build --define absl=1"
    if not native.existing_rule("com_google_absl"):
        github_archive(
            name = "com_google_absl",
            repo = "https://github.com/abseil/abseil-cpp",
            tag = "20240116.1",
            sha256 = "3c743204df78366ad2eaf236d6631d83f6bc928d1705dd0000b872e53b73dc6a",
        )

    # GoogleTest
    if not native.existing_rule("com_google_googletest"):
        github_archive(
            name = "com_google_googletest",
            repo = "https://github.com/google/googletest",
            strip_prefix = "googletest-1.14.0",
            sha256 = "8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7",
            tag = "v1.14.0",
        )

    if not native.existing_rule("com_github_google_benchmark"):
        github_archive(
            name = "com_github_google_benchmark",
            repo = "https://github.com/google/benchmark",
            sha256 = "2aab2980d0376137f969d92848fbb68216abb07633034534fc8c65cc4e7a0e93",
            strip_prefix = "benchmark-1.8.2",
            tag = "v1.8.2",
        )

    if not native.existing_rule("rules_python"):
        http_archive(
            name = "rules_python",
            sha256 = "9c6e26911a79fbf510a8f06d8eedb40f412023cf7fa6d1461def27116bff022c",
            strip_prefix = "rules_python-1.1.0",
            url = "https://github.com/bazelbuild/rules_python/releases/download/1.1.0/rules_python-1.1.0.tar.gz",
        )

    if not native.existing_rule("com_google_protobuf"):
        http_archive(
            name = "com_google_protobuf",
            sha256 = "008a11cc56f9b96679b4c285fd05f46d317d685be3ab524b2a310be0fbad987e",
            strip_prefix = "protobuf-29.3",
            url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.3/protobuf-29.3.tar.gz",
        )

    if not native.existing_rule("com_helly25_bashtest"):
        http_archive(
            name = "com_helly25_bashtest",
            sha256 = "3f0314c2dbc6f780c02543c6355d07f98cd9935e54a07ec4477e3301434517d6",
            strip_prefix = "bashtest-0.1.0",
            url = "https://github.com/helly25/bashtest/releases/download/0.1.0/bashtest-0.1.0.tar.gz",
        )
