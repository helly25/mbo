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

#include <cstdarg>  // IWYU pragma: keep
#include <string_view>
#include <type_traits>  // IWYU pragma: keep

#include "absl/types/span.h"
#include "mbo/types/internal/decompose_count.h"  // IWYU pragma: keep

// IWYU pragma: private, include "mbo/types/internal/struct_names.h"

namespace mbo::types::types_internal::clang {

template<typename T>
class StructMeta {
 public:
  static absl::Span<const std::string_view> GetNames() {
    return absl::MakeConstSpan(kFieldNames);
  }

 private:
  using FieldData = std::array<std::string_view, DecomposeCountImpl<T>::value>;

  static constexpr void Init(FieldData& fields, std::size_t& field_index) {
    union Uninitialized {
      constexpr Uninitialized() noexcept {}
      constexpr ~Uninitialized() noexcept {};
      Uninitialized(const Uninitialized&) = delete;
      Uninitialized& operator=(const Uninitialized&) = delete;
      Uninitialized(Uninitialized&&) = delete;
      Uninitialized& operator=(Uninitialized&&) = delete;
      alignas(T) const char buf[sizeof(T)]{};  // NOLINT(*-avoid-c-arrays)
      T invalid;
    };
    constexpr Uninitialized kTemp;
    __builtin_dump_struct(&kTemp.invalid, &StructMeta<T>::DumpStructVisitor, fields, field_index);  // NOLINT(*-vararg)
  }

  // NOLINTNEXTLINE(cert-dcl50-cpp)
  static constexpr int DumpStructVisitor(FieldData& fields, std::size_t& field_index, std::string_view format, ...) {
    if (field_index >= DecomposeCountImpl<T>::value) {
      return 0;
    }
    if (format.starts_with("%s%s %s =")) {
      // NOLINTBEGIN(*-array-to-pointer-decay,*-avoid-c-arrays,*-no-array-decay,*-pointer-arithmetic,*-vararg)
      va_list vap{};
      va_start(vap, format);
      const char* indent = va_arg(vap, const char*);
      if (indent[0] == ' ' && indent[1] == ' ' && indent[2] == '\0') {
        [[maybe_unused]] const char* type = va_arg(vap, const char*);
        const char* name = va_arg(vap, const char*);
        // Since an **uninitialized** value is passed the 'value' should not be accessed.
        // [[maybe_unused]] const char* type = va_arg(vap, const char*);
        fields[field_index++] = std::string_view(name);
      }
      va_end(vap);
      // NOLINTEND(*-array-to-pointer-decay,*-avoid-c-arrays,*-no-array-decay,*-pointer-arithmetic,*-vararg)
    }
    return 0;
  }

  inline static const FieldData kFieldNames = []{  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::size_t field_index = 0;
    FieldData fields;
    Init(fields, field_index);
    return fields;
  }();
};

}  // namespace mbo::types::types_internal::clang

#endif  // __clang__
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
