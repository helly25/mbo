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

#ifndef MBO_STRINGS_NUMBERS_H_
#define MBO_STRINGS_NUMBERS_H_

#include <cassert>
#include <cmath>
#include <concepts>
#include <limits>
#include <string>

#include "mbo/container/limited_map.h"

namespace mbo::strings {

// Determine the string length needed to convert a number into a representation where thousands are
// separated by `'`.
template<std::integral T>
requires(sizeof(T) <= 8)
unsigned BigNumberLen(T v) {
  // We could compute this for smller numbers as:
  //   len = 1 + static_cast<unsigned>(std::log10(std::abs(v)));
  //   return (v < 0 ? 1 : 0) + len + std::floor((len - 1) / 3);
  // But:
  //   a) The input would need to be converted to `long double` and even then can only handle up to
  //      17 digits, but int64_t needs 19.
  //   b) That computation is slow.
  //   c) Binary searching in a limit/length map is much faster.
  if constexpr (std::signed_integral<T>) {
    if (v < 0) {
      if (v == std::numeric_limits<T>::min()) {
        return 1 + BigNumberLen(-(v + 1));
      }
      return 1 + BigNumberLen(-v);
    }
  }
  // Perform binary search for length to limit the number of tests.
  // We first check whether we can do even better by limiting us to 4 byte types.
  // We use a macro to let the compiler compute the actual length values.
  if constexpr (sizeof(v) <= 4) {
#define CHECK_CAP(cap) std::make_pair<uint32_t, unsigned>(cap, std::string_view(#cap).size() - 3)
    constexpr auto kData = mbo::container::ToLimitedMap<std::pair<uint32_t, unsigned>>({
        CHECK_CAP(4'294'967'295ULL),
        CHECK_CAP(999'999'999ULL),
        CHECK_CAP(99'999'999ULL),
        CHECK_CAP(9'999'999ULL),
        CHECK_CAP(999'999ULL),
        CHECK_CAP(99'999ULL),
        CHECK_CAP(9'999ULL),
        CHECK_CAP(999ULL),
        CHECK_CAP(99ULL),
        CHECK_CAP(9ULL),
        CHECK_CAP(0ULL),
    });
#undef CHECK_CAP
    return kData.lower_bound(v)->second;
  } else {
#define CHECK_CAP(cap) std::make_pair<uint64_t, unsigned>(cap, std::string_view(#cap).size() - 3)
    constexpr auto kData = mbo::container::ToLimitedMap<std::pair<uint64_t, unsigned>>({
        CHECK_CAP(18'446'744'073'709'551'615ULL),
        CHECK_CAP(9'999'999'999'999'999'999ULL),
        CHECK_CAP(999'999'999'999'999'999ULL),
        CHECK_CAP(99'999'999'999'999'999ULL),
        CHECK_CAP(9'999'999'999'999'999ULL),
        CHECK_CAP(999'999'999'999'999ULL),
        CHECK_CAP(99'999'999'999'999ULL),
        CHECK_CAP(9'999'999'999'999ULL),
        CHECK_CAP(999'999'999'999ULL),
        CHECK_CAP(99'999'999'999ULL),
        CHECK_CAP(9'999'999'999ULL),
        CHECK_CAP(999'999'999ULL),
        CHECK_CAP(99'999'999ULL),
        CHECK_CAP(9'999'999ULL),
        CHECK_CAP(999'999ULL),
        CHECK_CAP(99'999ULL),
        CHECK_CAP(9'999ULL),
        CHECK_CAP(999ULL),
        CHECK_CAP(99ULL),
        CHECK_CAP(9ULL),
        CHECK_CAP(0ULL),
    });
#undef CHECK_CAP
    return kData.lower_bound(v)->second;
  }
}

// Convert the number to a string representation with thousands separators.
template<std::integral T>
std::string BigNumber(T v) {
  const std::string tmp = std::to_string(v);
  const std::size_t neg = tmp.starts_with('-') ? 1 : 0;
  const std::size_t ofs = tmp.size() % 3;
  std::string res;
  {
    const std::size_t cap = tmp.size() + std::floor((tmp.size() - neg - 1) / 3);
    if (cap > res.capacity()) {
      res.reserve(cap);
    }
  }
  std::size_t pos{0};
  for (; pos < tmp.size(); ++pos) {
    if (pos > neg && pos % 3 == ofs) {
      break;
    }
    res.push_back(tmp[pos]);
  }
  while (pos < tmp.size()) {
    res.push_back('\'');
    res.push_back(tmp[pos]);
    res.push_back(tmp[++pos]);
    res.push_back(tmp[++pos]);
    ++pos;
  }
  return res;
}

}  // namespace mbo::strings

#endif  // MBO_STRINGS_NUMBERS_H_
