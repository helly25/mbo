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

#ifndef MBO_DIGEST_CHECKSUM_H_
#define MBO_DIGEST_CHECKSUM_H_

#include <cstddef>
#include <optional>
#include <string_view>

namespace mbo::digest::internal {

struct ChecksumLine {
  std::string_view hex;
  std::string_view file_name;

  friend bool operator==(const ChecksumLine&, const ChecksumLine&) = default;
};

// Parses the checksum format emitted by coreutils: either
// "<hex>  <filename>" or "<hex> *<filename>".
std::optional<ChecksumLine> ParseChecksumLine(std::string_view line, std::size_t hex_length);

}  // namespace mbo::digest::internal

#endif  // MBO_DIGEST_CHECKSUM_H_
