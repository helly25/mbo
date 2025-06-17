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

# invoked by release workflow
# (via https://github.com/bazel-contrib/.github/blob/master/.github/workflows/release_ruleset.yaml)

set -euo pipefail

# Custom args to update as needed.
PACKAGE_NAME="mbo"
BAZELMOD_NAME="helly25_mbo"
WORKSPACE_NAME="com_helly25_mbo"
PATCHES=(
  ".github/workflows/bazelmod.patch"
)

# Automatic vars from workflow integration.
TAG="${GITHUB_REF_NAME}"

function die() {
  echo "ERROR: ${*}" 1>&2
  exit 1
}

# Computed vars.
PREFIX="${PACKAGE_NAME}-${TAG}"
ARCHIVE="${PACKAGE_NAME}-${TAG}.tar.gz"
BAZELMOD_VERSION="$(sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p' <MODULE.bazel | head -n1)"
CHANGELOG_VERSION="$(sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p' <CHANGELOG.md | head -n1)"

if [ "${BAZELMOD_VERSION}" != "${TAG}" ]; then
  die "Tag = '${TAG}' does not match version = '${BAZELMOD_VERSION}' in MODULE.bazel."
fi
if [ "${CHANGELOG_VERSION}" != "${TAG}" ]; then
  die "Tag = '${TAG}' does not match version = '${CHANGELOG_VERSION}' in CHANGELOG.md."
fi

# Instead of embed the version in MODULE.bazel, we expect it to be correct already.
# perl -pi -e "s/version = \"\d+\.\d+\.\d+\",/version = \"${TAG}\",/g" MODULE.bazel

# Empty `BUILD.bazel`
{
  cat tools/header.txt
  echo ""
  echo "\"\"\"Empty root BUILD for @${BAZELMOD_NAME}.\"\"\""
} >BUILD.bazel

# Apply patches
for patch in "${PATCHES[@]}"; do
  patch -s -p 1 <"${patch}"
done

# Exclude some dev stuff from the archive.
EXCLUDES=(
  ".bcr"
  ".github"
  ".pre-commit"
  ".pre-commit-config.yaml"
  "tools"
)
{
  for exclude in "${EXCLUDES[@]}"; do
    echo "${exclude} export-ignore"
    if [[ -d ${exclude} ]]; then
      echo "${exclude}/** export-ignore"
    fi
  done
} >>.gitattributes

# Build the archive
git archive --format=tar.gz --prefix="${PREFIX}/" "${TAG}" -o "${ARCHIVE}" --add-virtual-file="${PREFIX}/VERSION:${TAG}" --worktree-attributes

SHA256="$(shasum -a 256 "${ARCHIVE}" | awk '{print $1}')"

# Print header
echo "# Version ${TAG}"
echo "## [Changelog](https://github.com/helly25/${PACKAGE_NAME}/blob/${TAG}/CHANGELOG.md)"

# Print Changelog
awk '/^#/{if(NR>1)exit}/^[^#]/{print}' <CHANGELOG.md

cat <<EOF
## For Bazel MODULES.bazel

\`\`\`bzl
bazel_dep(name = "${BAZELMOD_NAME}", version = "${TAG}")
\`\`\`

### Using the provided LLVM

Copy [llvm.MODULE.bazel](https://github.com/helly25/${PACKAGE_NAME}/blob/main/bazelmod/llvm.MODULE.bazel) to your repository's root directory and add the following line to your MODULES.bazel file or paste the whole contents into it.

\`\`\`bzl
include("//:llvm.MODULE.bazel")
\`\`\`

## For Bazel WORKSPACE

\`\`\`bzl
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "${WORKSPACE_NAME}",
  url = "https://github.com/helly25/${PACKAGE_NAME}/releases/download/${TAG}/${ARCHIVE}",
  sha256 = "${SHA256}",
  strip_prefix = "${PREFIX}",
)
\`\`\`

### Initializing the required modules

The project depends on some additional external repositories that can be added
manually of by calling the support functions in the user' WORKSPACE file:

\`\`\`bzl
load("@com_helly25_mbo//bzl/workspace:load_modules.bzl", "helly25_mbo_load_modules")

helly25_mbo_load_modules()

load("@com_helly25_mbo//bzl/workspace:init_modules.bzl", "helly25_mbo_init_modules")

helly25_mbo_init_modules()
\`\`\`

### Initializing optional extra modules

The project further has some re-usable external components:

\`\`\`bzl
load("@com_helly25_mbo//bzl/workspace:load_extras.bzl", "helly25_mbo_load_extras")

helly25_mbo_load_extras()  # Adds Hedron + LLVM

load("@com_helly25_mbo//bzl/workspace:init_extras.bzl", "helly25_mbo_init_extras")

helly25_mbo_init_extras()  # Init Hedron + LLVM

load("@com_helly25_mbo//bzl/workspace:init_extras_llvm.bzl", "helly25_mbo_init_extras_llvm")

helly25_mbo_init_extras_llvm()  # Init LLVM/Part 2
\`\`\`
EOF
