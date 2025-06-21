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
