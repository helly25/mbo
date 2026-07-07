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

# Reproducible SMHasher3 build for mbo/hash quality verification.
#
# Clones SMHasher3 at the pinned commit, applies the two documented build fixes,
# and builds it with gcc inside a linux container, matching the methodology in
# mbo/hash/README.md ("Methodology (reproduction)"). Prints the path to the
# built SMHasher3 binary; drive it with
#   hash_benchmark_report.py smhasher --smhasher3 <path> --algos all
#
# The third-party algorithms (rapidhash, xxh3, xxh64, murmur3, siphash, fnv1a)
# are SMHasher3 built-ins and work immediately. The in-house mumbo/jumbo/dumbo
# need a registration source dropped into SMHasher3's hashes/ tree BEFORE the
# build (a transcription registering "mumbo-64" / "jumbo-128" / "dumbo-64",
# matching mbo/hash/hash_mumbo.h and hash_dumbo.h) - that plugin is the
# outstanding piece; see mbo/hash/measurements/README.md.
#
# Usage: build_smhasher3.sh [work_dir]   (default: ./.smhasher3)

set -euo pipefail

# Pinned upstream (gitlab.com/fwojcik/smhasher3), same as the README table.
readonly SMHASHER3_REPO="https://gitlab.com/fwojcik/smhasher3.git"
readonly SMHASHER3_COMMIT="6ab4343396fbe0f7a1c7ac4f01d0eb9acffe4202"
# gcc 13 on linux; -march=armv8-a+crc for arm64 (drop CRC on x86_64 - see below).
readonly BUILD_IMAGE="gcc:13"

WORK_DIR="${1:-$(pwd)/.smhasher3}"
SRC_DIR="${WORK_DIR}/smhasher3"

mkdir -p "${WORK_DIR}"

if [[ ! -d "${SRC_DIR}/.git" ]]; then
  git clone "${SMHASHER3_REPO}" "${SRC_DIR}"
fi
git -C "${SRC_DIR}" fetch --depth=1 origin "${SMHASHER3_COMMIT}"
git -C "${SRC_DIR}" checkout --quiet "${SMHASHER3_COMMIT}"

# Fix 1: lib/AEStest.cpp is missing <cstdlib> (used for abort()/exit()).
if ! grep -q '#include <cstdlib>' "${SRC_DIR}/lib/AEStest.cpp"; then
  sed -i.bak '1i\
#include <cstdlib>
' "${SRC_DIR}/lib/AEStest.cpp"
fi

# Fix 2: CMakeLists.txt uses -march=native, which emits SHA3 (eor3) instructions
# the container toolchain rejects. Pin an explicit, portable arch instead.
#   arm64 -> armv8-a+crc ; x86_64 -> x86-64-v2 (has the CRC/SSE SMHasher3 wants).
arch="$(uname -m)"
if [[ "${arch}" == "x86_64" || "${arch}" == "amd64" ]]; then
  march="x86-64-v2"
else
  march="armv8-a+crc"
fi
sed -i.bak "s/-march=native/-march=${march}/g" "${SRC_DIR}/CMakeLists.txt"

# Build with gcc in a container (native on arm64 hosts; matches the README rig).
docker run --rm -v "${SRC_DIR}:/src" -w /src "${BUILD_IMAGE}" bash -c '
  set -euo pipefail
  apt-get update -qq && apt-get install -y -qq cmake >/dev/null
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j"$(nproc)"
'

echo "SMHasher3 built: ${SRC_DIR}/build/SMHasher3"
echo "Confirm registrations:  ${SRC_DIR}/build/SMHasher3 --list | grep -Ei 'mumbo|jumbo|dumbo|rapid|xxh|murmur|sip|fnv'"
