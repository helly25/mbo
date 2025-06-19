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

#include "mbo/log/demangle.h"

#ifdef MBO_LOG_HAS_CXA_DEMANGLE
# error MBO_LOG_HAS_CXA_DEMANGLE cannot be set directly
#elif defined(OS_ANDROID) && (defined(__i386__) || defined(__x86_64__))
# define MBO_LOG_HAS_CXA_DEMANGLE 0
#elif defined(__GNUC__)
# define MBO_LOG_HAS_CXA_DEMANGLE 1
#elif defined(__clang__) && !defined(_MSC_VER)
# define MBO_LOG_HAS_CXA_DEMANGLE 1
#endif

#if MBO_LOG_HAS_CXA_DEMANGLE
# include <cxxabi.h>
#endif  // MBO_LOG_HAS_CXA_DEMANGLE

namespace mbo::log {

std::string Demangle(const char* mangled_name) {
  std::string out;
  int status = 0;
  char* demangled = nullptr;
#if MBO_LOG_HAS_CXA_DEMANGLE
  demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
#endif
  if (status == 0 && demangled != nullptr) {
    out.append(demangled);
    free(demangled);  // NOLINT(*-no-malloc,*-owning-memory)
  } else {
    out.append(mangled_name);
  }
  return out;
}

}  // namespace mbo::log
