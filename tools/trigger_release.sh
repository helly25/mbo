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

function die() {
  echo "ERROR: ${*}" 1>&2
  exit 1
}

[[ ${#} == 1 ]] || die "Must provide a version argument."

git fetch origin main # Make sure the below is relevant

# Must actually be on `main` at exactly origin/main. The tree-diff checks below
# pass for any branch whose tree matches main (e.g. a just-squash-merged feature
# branch), so on their own they would let a release be cut off the wrong commit.
BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ "${BRANCH}" != "main" ]]; then
  die "Must be run from the 'main' branch (currently on '${BRANCH}')."
fi
if [[ "$(git rev-parse HEAD)" != "$(git rev-parse origin/main)" ]]; then
  die "HEAD ($(git rev-parse --short HEAD)) is not at origin/main ($(git rev-parse --short origin/main)); pull first."
fi

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

# Releases are strictly numeric <major>.<minor>.<patch>.
if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  die "Version must be numeric <major>.<minor>.<patch> (got '${VERSION}')."
fi

BAZELMOD_VERSION="$(sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p' <MODULE.bazel | head -n1)"
CHANGELOG_VERSION="$(sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p' <CHANGELOG.md | head -n1)"
NEXT_VERSION="$(echo "${VERSION}" | awk -F. '/^(0|[1-9][0-9]*)([.](0|[1-9][0-9]*))([.](0|[1-9][0-9]*))+([-+]|$)/{print $1"."$2"."(($3)+1)}')"

if [[ "${BAZELMOD_VERSION}" != "${CHANGELOG_VERSION}" ]]; then
  die "MODULE.bazel (${BAZELMOD_VERSION}) != CHANGELOG.md (${CHANGELOG_VERSION})."
fi

if [[ "${VERSION}" != "${BAZELMOD_VERSION}" ]]; then
  die "Provided version argument (${VERSION}) different from merged version (${BAZELMOD_VERSION})."
fi

if [[ -z "${NEXT_VERSION}" ]]; then
  die "Could not determine next version from input (${VERSION})."
fi

grep "${VERSION}" < <(git tag -l) && die "Version tag is already in use."

# Pre-flight: the post-release publish-to-bcr step applies every .bcr/patches/*
# patch to the released MODULE.bazel. If one no longer applies (e.g. context
# drift after a dependency bump) it fails at "Create final entry" AFTER the
# release is already public. Catch it here, before we tag anything.
for bcr_patch in .bcr/patches/*.patch; do
  [[ -e "${bcr_patch}" ]] || continue
  git apply --check "${bcr_patch}" 2>/dev/null ||
    die "BCR patch '${bcr_patch}' no longer applies to MODULE.bazel; regenerate it before releasing."
done

git tag -s -a "${VERSION}" \
  -m "New release tag version: '${VERSION}'." \
  -m "$(awk '/^#/{if(NR>1)exit}/^[^#]/{print}' <CHANGELOG.md)"
git push origin --tags

echo "Next version: ${NEXT_VERSION}"

# Bump the module version (the first `version = "X"` line). Portable across BSD
# (macOS) and GNU sed: BSD `sed -i` needs a backup-suffix arg and `0,/re/` is a
# GNU-only address, so write to a temp file and use the portable `1,/re/` range.
sed "1,/version = \"${VERSION}\"/ s/version = \"${VERSION}\"/version = \"${NEXT_VERSION}\"/" MODULE.bazel >MODULE.bazel.tmp
mv MODULE.bazel.tmp MODULE.bazel

# Prepend a new top section for the next version (portable; no `sed -i`).
{ printf '# %s\n\n' "${NEXT_VERSION}"; cat CHANGELOG.md; } >CHANGELOG.md.tmp
mv CHANGELOG.md.tmp CHANGELOG.md

NEXT_BRANCH="chore/bump_version_to_${NEXT_VERSION}"

git checkout -b "${NEXT_BRANCH}"
git add MODULE.bazel
git add CHANGELOG.md
git commit -m "Bump version to ${NEXT_VERSION}"
git push -u origin "${NEXT_BRANCH}"
git push
if which gh; then
  PRNUM=""
  PRURL=""
  BUMP_TEXT="Bump version from ${VERSION} to ${NEXT_VERSION}"
  MERGE_TITLE="${BUMP_TEXT}"
  MERGE_SUBJECT="${BUMP_TEXT}"
  MERGE_BODY="Auto approved version bump from ${VERSION} to ${NEXT_VERSION} by trigger script."
  if gh pr create --title "${MERGE_TITLE}" -b "Created by ${0}." 2>&1 | tee pr_create_output.txt; then
    PRNUM="$(sed -rne 's,https?://github.com/[^/]+/[^/]+/pull/([0-9]+)$,\1,p' <pr_create_output.txt)"
    PRURL="$(sed -rne 's,https?://github.com/[^/]+/[^/]+/pull/([0-9]+)$,\0,p' <pr_create_output.txt)"
  else
    echo "ERROR: Cannot create PR:"
    cat pr_create_output.txt
  fi
  if [[ "${PRNUM}" -gt 1 ]]; then
    gh pr ready "${NEXT_BRANCH}"
    gh pr review "${NEXT_BRANCH}" -a -b "${MERGE_BODY}" || true
    if gh pr merge "${NEXT_BRANCH}" --admin -d -s -b "${MERGE_BODY}" -t "${MERGE_SUBJECT}"; then
      git checkout main
      git branch -d "${NEXT_BRANCH}"
      echo "PR ${PRNUM} was merged via admin override. See: ${PRURL}."
    else
      gh pr merge "${NEXT_BRANCH}" --auto -d -s -b "${MERGE_BODY}" -t "${MERGE_SUBJECT}"
      git checkout main
      git branch -d "${NEXT_BRANCH}" || true
      echo "PR ${PRNUM} cannot be merged via admin override."
      echo "Please approve it at ${PRURL}."
    fi
  fi
fi
