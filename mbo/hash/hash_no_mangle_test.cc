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

// This target is built with -DMBO_HASH_MANGLE=0 (see BUILD.bazel) to cover the
// disabled-mangle path: `GetHash` must then equal `GetHash64` and `HashMangle`
// must be the identity.

#include "gtest/gtest.h"
#include "mbo/hash/hash.h"

namespace mbo::hash {
namespace {

static_assert(MBO_HASH_MANGLE == 0, "this target must be built with -DMBO_HASH_MANGLE=0");

// NOLINTBEGIN(*-magic-numbers)

TEST(HashNoMangleTest, MangleIsIdentity) {
  EXPECT_EQ(HashMangle(0), 0ULL);
  EXPECT_EQ(HashMangle(0x0123456789ABCDEFULL), 0x0123456789ABCDEFULL);
}

TEST(HashNoMangleTest, GetHashEqualsGetHash64) {
  EXPECT_EQ(GetHash("reproducible"), GetHash64("reproducible"));
  EXPECT_EQ(GetHash(""), GetHash64(""));
  EXPECT_EQ(GetHash("another value"), GetHash64("another value"));
}

// NOLINTEND(*-magic-numbers)

}  // namespace
}  // namespace mbo::hash
