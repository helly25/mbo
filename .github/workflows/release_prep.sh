#!/usr/bin/env bash

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

# Computed vars.
PREFIX="${PACKAGE_NAME}-${TAG}"
ARCHIVE="${PACKAGE_NAME}-${TAG}.tar.gz"
BAZELMOD_VERSION="$(cat MODULE.bazel | sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p'|head -n1)"
CHANGELOG_VERSION="$(cat CHANGELOG.md | sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p'|head -n1)"

if [ "${BAZELMOD_VERSION}" != "${TAG}" ]; then
  echo "ERROR: Tag = '${TAG}' does not match version = '${BAZELMOD_VERSION}' in MODULE.bazel."
  exit 1
fi
if [ "${CHANGELOG_VERSION}" != "${TAG}" ]; then
  echo "ERROR: Tag = '${TAG}' does not match version = '${CHANGELOG_VERSION}' in WORKSPACE."
  exit 1
fi

# Instead of embed the version in MODULE.bazel, we expect it to be correct already.
# perl -pi -e "s/version = \"\d+\.\d+\.\d+\",/version = \"${TAG}\",/g" MODULE.bazel

# Apply patches
for patch in "${PATCHES[@]}"; do
  echo "Patching ${patch}."
  patch -p 1 <"${patch}"
done

# Build the archive
git archive --format=tar.gz --prefix="${PREFIX}/" "${TAG}" -o "${ARCHIVE}"

SHA256="$(shasum -a 256 "${ARCHIVE}" | awk '{print $1}')"

# Print Changelog
cat CHANGELOG.md | awk '/#/{f+=1;if(f>1)exit} {print}'

cat << EOF
# For Bazel WORKSPACE

\`\`\`
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "${WORKSPACE_NAME}",
  url = "https://github.com/helly25/mbo/releases/download/${TAG}/${ARCHIVE}",
  sha256 = "${SHA256}",
)
\`\`\`

# For Bazel MODULES.bazel

\`\`\`
bazel_dep(name = "${BAZELMOD_NAME}", version = "${TAG}")
\`\`\`

## Using the provided LLVM

Copy [llvm.MODULE.bazel](https://github.com/helly25/mbo/blob/main/bazelmod/llvm.MODULE.bazel) to your repository's root directory and add the following line to your MODULES.bazel file or paste the whole contents into it.

\`\`\`
include("//:llvm.MODULE.bazel")
\`\`\`
EOF
