// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MBO_HASH_HASH_H_
#define MBO_HASH_HASH_H_

#include <cstdint>
#include <string_view>

#include "mbo/hash/hash_internal_util.h"  // IWYU pragma: export
#include "mbo/hash/hash_mangle.h"         // IWYU pragma: export
#include "mbo/hash/hash_mh.h"             // IWYU pragma: export
#include "mbo/hash/hash_simple.h"         // IWYU pragma: export
#include "mbo/hash/hash_types.h"          // IWYU pragma: export

namespace mbo::hash {

// Selected current default implementations (see `hash_mh.h`).
using ::mbo::hash::mh::GetHash128;
using ::mbo::hash::mh::GetHash64;

// A fast, constexpr-safe, non-cryptographic hash with a per-build seed.
//
// `GetHash` folds `GetHash64` through a `HashMangle` step that mixes in one of a
// small, fixed set of build-derived seeds (bucketed from `__DATE__`/`__TIME__`,
// see `kMangleSeedCount`). Its value therefore **may** change from build to
// build -- bounded to that seed set so it stays cache-friendly, and pinned to a
// single seed in a hermetic build (e.g. Bazel). Use `GetHash` when you want
// values that are deliberately not comparable across independent builds.
//
// For a fully deterministic, reproducible value (identical at compile time and
// run time, and across builds) call `GetHash64` / `GetHash128` directly.
//
// Stability: none of these values are guaranteed stable across library versions,
// none are portable as a persisted/on-the-wire format, and none are suitable for
// cryptographic use.
inline constexpr uint64_t GetHash(std::string_view data) {
  return HashMangle(GetHash64(data));
}

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_H_
