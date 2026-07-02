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

// The signature every pluggable 64-bit hash implementation provides:
// `(data, seed) -> hash`, constexpr-safe and non-throwing.
using Hash64Fn = uint64_t (*)(std::string_view, uint64_t) noexcept;

// A fast, constexpr-safe, non-cryptographic hash with a per-build seed.
//
// `GetHash` folds a 64-bit hash implementation through a `HashMangle` step that
// mixes in one of a small, fixed set of build-derived seeds (bucketed from
// `__DATE__`/`__TIME__`, see `kMangleSeedCount`). Its value therefore **may**
// change from build to build -- bounded to that seed set so it stays
// cache-friendly, and pinned to a single seed in a hermetic build (e.g. Bazel).
// Use `GetHash` when you want values that are deliberately not comparable across
// independent builds.
//
// The implementation defaults to the current default (`mbo::hash::mh`), but any
// hash matching `Hash64Fn` can be plugged in as a template argument, and the
// seed likewise, e.g. `GetHash<&mbo::hash::mh::GetHash64, 0x1234>(data)`.
//
// For a fully deterministic, reproducible value (identical at compile time and
// run time, and across builds) call `GetHash64` / `GetHash128` directly.
//
// Stability: none of these values are guaranteed stable across library versions,
// none are portable as a persisted/on-the-wire format, and none are suitable for
// cryptographic use.
template<Hash64Fn GetHashFn = &::mbo::hash::mh::GetHash64, uint64_t Seed = mh::kDefaultSeed>
inline constexpr uint64_t GetHash(std::string_view data) {
  return HashMangle(GetHashFn(data, Seed));
}

}  // namespace mbo::hash

#endif  // MBO_HASH_HASH_H_
