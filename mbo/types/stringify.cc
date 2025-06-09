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

#include "mbo/types/stringify.h"

namespace mbo::types {

const StringifyOptions& StringifyOptions::AsDefault() noexcept {
  static constexpr StringifyOptions kDefault{};
  return kDefault;
}

const StringifyOptions& StringifyOptions::AsCpp() noexcept {
  static constexpr StringifyOptions kCpp{
      .key_prefix = ".",
      .key_value_separator = " = ",
      .value_pointer_prefix = "",
      .value_pointer_suffix = "",
      .value_nullptr_t = "nullptr",
      .value_nullptr = "nullptr",
  };
  return kCpp;
}

const StringifyOptions& StringifyOptions::AsJson() noexcept {
  static constexpr StringifyOptions kJson{
      .field_suppress_nullptr = true,
      .field_suppress_nullopt = true,
      .field_suppress_disabled = true,
      .key_mode = KeyMode::kNumericFallback,
      .key_prefix = "\"",
      .key_suffix = "\"",
      .key_value_separator = ": ",
      .value_pointer_prefix = "",
      .value_pointer_suffix = "",
      .value_smart_ptr_prefix = "",
      .value_smart_ptr_suffix = "",
      .value_nullptr_t = "0",
      .value_nullptr = "0",
      .value_optional_prefix = "",
      .value_optional_suffix = "",
      .value_nullopt = "0",
      .value_container_prefix = "[",
      .value_container_suffix = "]",
      .value_char_delim = "\"",
      .special_pair_first_is_name = true,
  };
  return kJson;
}

const StringifyOptions& StringifyOptions::AsJsonPretty() noexcept {
  static const StringifyOptions kJson = [] {
    StringifyOptions json = AsJson();
    json.message_end = "\n";
    json.field_indent = "  ";
    json.field_separator = ",";
    return json;
  }();
  return kJson;
}

}  // namespace mbo::types
