# SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

"""Initializes all standard modules required by the workspace."""

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")
load("@rules_python//python:repositories.bzl", "py_repositories")
load("@rules_shell//shell:repositories.bzl", "rules_shell_dependencies", "rules_shell_toolchains")

def helly25_mbo_init_modules():
    """Initializes all standard modules required by the workspace."""
    bazel_skylib_workspace()
    rules_foreign_cc_dependencies()
    rules_shell_dependencies()
    rules_shell_toolchains()
    py_repositories()
    protobuf_deps()
