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

#ifndef MBO_LOG_DEMANGLE_H_
#define MBO_LOG_DEMANGLE_H_

#include <string>
#include <typeinfo>

namespace mbo::log {

std::string Demangle(const char* mangled_name);

template<typename T>
std::string DemangleV(T&& v) {  // NOLINT(*-missing-std-forward)
  return Demangle(typeid(v).name());
}

}  // namespace mbo::log

#endif  // MBO_LOG_DEMANGLE_H_
