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

function die() { echo "ERROR: ${*}" 1>&2 ; exit 1; }

if [[ ! -r "MODULE.bazel" ]] || [[ "${0}" != "tools/dwyu.sh" ]]; then
    die "Must be run from workspace directory."
fi

bazel build --aspects=//tools:dwyu.bzl%dwyu_aspect --output_groups=dwyu //...  || \
    bazel run @depend_on_what_you_use//:apply_fixes -- --fix-all

# -//mbo/strings:indent_cc
# -//mbo/types/internal:struct_names_cc
