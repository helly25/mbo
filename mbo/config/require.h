// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

#ifndef MBO_CONFIG_REQUIRE_H_
#define MBO_CONFIG_REQUIRE_H_

#include <stdexcept>  // IWYU pragma: keep

#include "absl/log/absl_log.h"  // IWYU pragma: keep

// If `mbo/config/config.h` is available, then include that. In order to
// make indexers work, we also include the generator "...in" as a fallback.
#if __has_include("mbo/config/config.h")
# include "mbo/config/config.h"  // IWYU pragma: keep
#else
# include "mbo/config/internal/config.h.in"  // IWYU pragma: keep
#endif

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define MBO_PRIVATE_CONFIG_CAT_CAT_(line) #line
#define MBO_PRIVATE_CONFIG_NUM2STR_(line) MBO_PRIVATE_CONFIG_CAT_CAT_(line)

#ifdef MBO_CONFIG_REQUIRE
# undef MBO_CONFIG_REQUIRE
#endif

#define MBO_CONFIG_REQUIRE(condition, message)                 \
  if constexpr (!::mbo::config::kRequireThrows) {              \
    /* NOLINTNEXTLINE(bugprone-switch-missing-default-case) */ \
    ABSL_LOG_IF(FATAL, !(condition)) << message;               \
  } else if ((condition)) { /* GOOD */                         \
  } else                                                       \
    throw std::runtime_error(__FILE__ ":" MBO_PRIVATE_CONFIG_NUM2STR_(__LINE__) " : " #condition " : " message)

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif  // MBO_CONFIG_REQUIRE_H_
