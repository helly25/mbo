#!/usr/bin/env bash

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

set -euxo pipefail

bazel build -c opt //mbo/diff:unified_diff

UNIFIED_DIFF="bazel-bin/mbo/diff/unified_diff"

if find -P -E . >/dev/null 2>&1; then
    FLAGS=("-P" "-E")
    EXTRA=()
elif find -P . -regextype posix-extended >/dev/null 2>&1; then
    FLAGS=("-P")
    EXTRA=("-regextype" "posix-extended")
else
    FLAGS=("-P")
    EXTRA=()
fi

rm -f /tmp/header-diffs.txt
touch /tmp/header-diffs.txt

MAX_LINES=$(wc -l tools/header_cpp.txt | sed -e 's,^ *,,g' | cut -d ' ' -f 1)
find "${FLAGS[@]}" . "${EXTRA[@]+"${EXTRA[@]}"}" -regex '.*[.](c|cc|cpp|h|hh|hpp)([.]mope)?$' \
    -exec "${UNIFIED_DIFF}" \
    --file_header_use left \
    --ignore_matching_lines='^($|([^/]($|[^/])))' \
    --max_lines "${MAX_LINES}" \
    {} \
    tools/header_cpp.txt \
    \; \
    >> /tmp/header-diffs.txt

MAX_LINES=$(wc -l tools/header.txt | sed -e 's,^  *,,g' | cut -d ' ' -f 1)
find "${FLAGS[@]}" . "${EXTRA[@]+"${EXTRA[@]}"}" -regex '.*/(.*[.](bzl|bazel)|BAZEL|WORKSPACE)$' \
    -exec "${UNIFIED_DIFF}" \
    --file_header_use left \
    --ignore_matching_lines='^($|[^#])' \
    --max_lines "${MAX_LINES}" \
    {} \
    tools/header.txt \
    \; \
    >> /tmp/header-diffs.txt

MAX_LINES=$(wc -l tools/header_sh.txt | sed -e 's,^  *,,g' | cut -d ' ' -f 1)
find "${FLAGS[@]}" . "${EXTRA[@]+"${EXTRA[@]}"}" -regex '.*[.](sh)$' \
    -exec "${UNIFIED_DIFF}" \
    --file_header_use left \
    --ignore_matching_lines='^($|[^#]|(#!/bin/bash$))' \
    --max_lines "${MAX_LINES}" \
    {} \
    tools/header_sh.txt \
    \; \
    >> /tmp/header-diffs.txt

patch -p 1 < /tmp/header-diffs.txt
