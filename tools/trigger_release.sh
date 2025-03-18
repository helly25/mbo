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

[[ ${#} == 1 ]] || die "Must provide a version argument."

git fetch origin main  # Make sure the below is relevant

if [[ -n "$(git status --porcelain)" ]]; then
    # Non empty output means non clean branch.
    die "Must be run from clean 'main' branch."
fi
if [[ -n "$(git diff origin/main --numstat)" ]]; then
    die "Must be run from clean 'main' branch."
fi
if [[ -n "$(git diff origin/main --cached --numstat)" ]]; then
    die "Must be run from clean 'main' branch."
fi

VERSION="${1}"

BAZELMOD_VERSION="$(sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p' < MODULE.bazel|head -n1)"
CHANGELOG_VERSION="$(sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p' < CHANGELOG.md|head -n1)"
NEXT_VERSION="$(echo "${VERSION}"|awk -F. '/^(0|[1-9][0-9]*)([.](0|[1-9][0-9]*)){2,}([-+]|$)/{print $1"."$2"."(($3)+1)}')"

if [[ "${BAZELMOD_VERSION}" != "${CHANGELOG_VERSION}" ]]; then
    die "MODULE.bazel (${BAZELMOD_VERSION}) != CHANGELOG.md (${CHANGELOG_VERSION})."
fi

if [[ "${VERSION}" != "${BAZELMOD_VERSION}" ]]; then
    die "Provided version argument (${VERSION}) different from merged version (${BAZELMOD_VERSION})."
fi

if [[ -z "${NEXT_VERSION}" ]]; then
    die "Could not determine next version from input (${VERSION)})."
fi

grep "${VERSION}" < <(git tag -l) && die "Version tag is already in use."

git tag -s -a "${VERSION}"
git push origin --tags

echo "Next version: ${NEXT_VERSION}"

sed -i "0,/version = \"${VERSION}\"/s/version = \"${VERSION}\"/version = \"${NEXT_VERSION}\"/" MODULE.bazel

sed -i "1i\
    # ${NEXT_VERSION}\n" CHANGELOG.md

NEXT_BRANCH="chore/bump_version_to_${NEXT_VERSION}"

git checkout -b "${NEXT_BRANCH}"
git add MODULE.bazel
git add CHANGELOG.md
git commit -m "Bump version to ${NEXT_VERSION}"
git push -u origin "${NEXT_BRANCH}"
git push
if which gh; then
    gh pr create --title "Bump version to ${NEXT_VERSION}" -b "Created by ${0}."
fi
