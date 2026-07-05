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

#ifndef MBO_HASH_HASH_EXTRA_H_
#define MBO_HASH_HASH_EXTRA_H_

#include "mbo/hash/hash.h"            // IWYU pragma: export
#include "mbo/hash/hash_rapidhash.h"  // IWYU pragma: export
#include "mbo/hash/hash_xxh3.h"       // IWYU pragma: export
#include "mbo/hash/hash_xxh64.h"      // IWYU pragma: export

// The NOTICE-bearing algorithm transcriptions, deliberately OUTSIDE the
// default `//mbo/hash:hash_cc` target: depending on `//mbo/hash:hash_extra_cc`
// (this header) pulls in code whose upstream licenses require reproducing
// their notices - ship the repository-root NOTICE file with any distribution
// that links this target. `hash_cc` itself contains only notice-free code.
//
// - `rapidhash::Algorithm` - rapidhash V3 (MIT; wyhash family). Canonical
//   values, best-in-class quality (SMHasher3 188/188).
// - `xxh64::Algorithm` - XXH64 (BSD-2-Clause). Canonical values, streaming.
// - `xxh3::Algorithm` - XXH3 64/128 (BSD-2-Clause). Canonical values incl.
//   the 128-bit file-checksum format.
//
// All satisfy `mbo::hash::IsHashAlgorithm` and plug into `GetHash64<Algo>`,
// `Hasher<Algo>`, and (xxh64) `Streamer<Algo>` exactly like the built-in
// algorithms. Quality and performance comparisons: see README.md.

#endif  // MBO_HASH_HASH_EXTRA_H_
