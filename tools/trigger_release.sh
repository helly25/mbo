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

set -euo pipefail

function die() { echo "ERROR: ${*}" 1>&2 ; exit 1; }

[ ${#} == 1 ] || die "Must provide a version argument."

VERSION="${1}"

BAZELMOD_VERSION="$(sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p' < MODULE.bazel|head -n1)"
CHANGELOG_VERSION="$(sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p' < CHANGELOG.md|head -n1)"

if [ "${BAZELMOD_VERSION}" != "${CHANGELOG_VERSION}" ]; then
    die "MODULE.bazel (${BAZELMOD_VERSION}) != CHANGELOG.md (${CHANGELOG_VERSION})."
fi

if [ "${VERSION}" != "${BAZELMOD_VERSION}" ]; then
    die "Provided version argument (${VERSION}) different from merged version (${BAZELMOD_VERSION})."
fi

grep "${VERSION}" < <(git tag -l) && die "Version tag is already in use."

git tag -s -a "${VERSION}"
git push origin --tags
