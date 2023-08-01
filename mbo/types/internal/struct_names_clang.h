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

#ifndef MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
#define MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
#if defined(__clang__) && __has_builtin(__builtin_dump_struct)

#include <string_view>
#include <type_traits>  // IWYU pragma: keep

#include "absl/types/span.h"
#include "mbo/types/internal/decompose_count.h"  // IWYU pragma: keep

// IWYU pragma: private, include "mbo/types/internal/struct_names.h"

namespace mbo::types::types_internal::clang {

template<typename T>
class StructMeta {
 public:
  static absl::Span<const std::string_view> GetNames(const T& v) {
    if (!initialized) {
      __builtin_dump_struct(&v, &StructMeta<T>::DumpStructVisitor);  // NOLINT(*-vararg)
      initialized = true;
    }
    return absl::MakeConstSpan(field_names);
  }

  static int DumpStructVisitor(std::string_view format, ...) {  // NOLINT(cert-dcl50-cpp)
    static std::size_t field_index = 0;
    if (field_index >= kNumFields) {
      return 0;
    }
    if (format.starts_with("%s%s %s =")) {
      // NOLINTBEGIN(*-array-to-pointer-decay,*-avoid-c-arrays,*-no-array-decay,*-pointer-arithmetic,*-vararg)
      va_list vap = nullptr;
      va_start(vap, format);
      const char* indent = va_arg(vap, const char*);
      if (indent[0] == ' ' && indent[1] == ' ' && indent[2] == '\0') {
        (void)va_arg(vap, const char*); // type
        const char* name = va_arg(vap, const char*);
        // index 3 is the init value (const char* or other type)
        field_names[field_index++] = std::string_view(name);
      }
      va_end(vap);
      // NOLINTEND(*-array-to-pointer-decay,*-avoid-c-arrays,*-no-array-decay,*-pointer-arithmetic,*-vararg)
    }
    return 0;
  }

  // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
  inline static constexpr std::size_t kNumFields = DecomposeCountImpl<T>::value;
  inline static std::array<std::string_view, kNumFields> field_names;
  inline static bool initialized = false;
  // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
};

}  // namespace mbo::types::types_internal::clang

#endif  // __clang__
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
