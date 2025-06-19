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

#include <ios>
#include <sstream>

#include "mbo/config/require.h"

namespace mbo::types {

constexpr StringifyOptions kOptionsCpp = StringifyOptions::WithAllData({
    .format{StringifyOptions::Format{
        .key_value_separator = " = ",
        .pointer_prefix = "",
        .pointer_suffix = "",
    }},
    .field_control{StringifyOptions::FieldControl{
        .field_disabled = "{/*MboTypesStringifyDisable*/}",
    }},
    .key_control{StringifyOptions::KeyControl{
        .key_prefix = ".",
    }},
    .value_control{StringifyOptions::ValueControl{
        .nullptr_t_str = "nullptr",
        .nullptr_v_str = "nullptr",
    }},
});

const StringifyOptions& StringifyOptions::AsCpp() noexcept {
  MBO_CONFIG_REQUIRE(kOptionsCpp.AllDataSet(), "Not all data set.");
  return kOptionsCpp;
}

constexpr StringifyOptions kOptionsJson = StringifyOptions::WithAllData({
    .format{StringifyOptions::Format{
        .key_value_separator = ": ",
        .pointer_prefix = "",
        .pointer_suffix = "",
        .smart_ptr_prefix = "",
        .smart_ptr_suffix = "",
        .optional_prefix = "",
        .optional_suffix = "",
        .container_prefix = "[",
        .container_suffix = "]",
        .char_delim = "\"",
    }},
    .field_control{StringifyOptions::FieldControl{
        .suppress_nullptr = true,
        .suppress_nullopt = true,
        .suppress_disabled = true,
    }},
    .key_control{StringifyOptions::KeyControl{
        .key_mode = StringifyOptions::KeyMode::kNumericFallback,
        .key_prefix = "\"",
        .key_suffix = "\"",
    }},
    .value_control{StringifyOptions::ValueControl{
        .nullptr_t_str = "0",
        .nullptr_v_str = "0",
        .nullopt_str = "0",
    }},
    .special{StringifyOptions::Special{
        .pair_first_is_name = true,
    }},
});

const StringifyOptions& StringifyOptions::AsJson() noexcept {
  MBO_CONFIG_REQUIRE(kOptionsJson.AllDataSet(), "Not all data set.");
  return kOptionsJson;
}

const StringifyOptions& StringifyOptions::AsJsonPretty() noexcept {
  static constexpr StringifyOptions kOptions = []() constexpr {
    StringifyOptions opts = kOptionsJson;
    Format& format = opts.format.as_data();
    format.message_suffix = "\n";
    format.field_indent = "  ";
    format.field_separator = ",";
    return opts;
  }();
  MBO_CONFIG_REQUIRE(kOptions.AllDataSet(), "Not all data set.");
  return kOptions;
}

std::string StringifyOptions::DebugStr() const {
  std::ostringstream out;
  out << "{\n";
  ApplyAll(*this, [&out]<typename T>(const T& v) {
    out << "  " << TypeName<typename T::value_type>() << ": " << std::boolalpha << v.has_value() << "\n";
    return true;
  });
  out << "}\n";
  return out.str();
}

std::string StringifyFieldOptions::DebugStr() const {
  return absl::StrCat("Outer: ", outer.DebugStr(), "Inner: ", inner.DebugStr());
}

}  // namespace mbo::types
