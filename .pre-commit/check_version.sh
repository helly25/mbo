#!/usr/bin/env bash

BAZELMOD_VERSION="$(cat MODULE.bazel | sed -rne 's,.*version = "([0-9]+([.][0-9]+)+.*)".*,\1,p'|head -n1)"
CHANGELOG_VERSION="$(cat CHANGELOG.md | sed -rne 's,^# ([0-9]+([.][0-9]+)+.*)$,\1,p'|head -n1)"

if [ "${BAZELMOD_VERSION}" != "${CHANGELOG_VERSION}" ]; then
  echo "ERROR: MODULE.bazel (${BAZELMOD_VERSION}) != CHANGELOG.md (${CHANGELOG_VERSION})."
  exit 1
fi
