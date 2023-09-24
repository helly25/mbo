#!/bin/bash

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

set -euxo pipefail

bazel build -c opt //mbo/diff:unified_diff

UNIFIED_DIFF="bazel-bin/mbo/diff/unified_diff"

MAX_LINES=$(wc -l tools/header_cpp.txt | cut -w -f 2)

find -E . -regex '.*[.](c|cc|cpp|h|hh|hpp)$' \
    -exec "${UNIFIED_DIFF}" \
        --file_header_use left \
        --ignore_matching_lines="^($|([^/]($|[^/])))" \
        --max_lines "${MAX_LINES}" \
        --skip_left_deletions \
        {} \
        tools/header_cpp.txt > /tmp/diff-{} \
        \; \
        > /tmp/header-diffs.txt

find -E . -regex '.*/(.*[.](bzl|bazel)|BAZEL|WORKSPACE)$' \
    -exec "${UNIFIED_DIFF}" \
        --file_header_use left \
        --ignore_matching_lines="^($|[^#])" \
        --max_lines "${MAX_LINES}" \
        --skip_left_deletions \
        {} \
        tools/header.txt > /tmp/diff-{} \
        \; \
        >> /tmp/header-diffs.txt

find -E . -regex '.*[.](sh)$' \
    -exec "${UNIFIED_DIFF}" \
        --file_header_use left \
        --ignore_matching_lines="^($|[^#]|(#!/bin/bash$))" \
        --max_lines "${MAX_LINES}" \
        --skip_left_deletions \
        {} \
        tools/header_sh.txt > /tmp/diff-{} \
        \; \
        >> /tmp/header-diffs.txt

patch < /tmp/header-diffs.txt
