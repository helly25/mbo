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
  static absl::Span<const std::string_view> GetNames() { return absl::MakeConstSpan(kFieldNames); }

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

  static constexpr int DumpStructVisitor(  // NOLINT(cert-dcl50-cpp)
      FieldData& fields,
      std::size_t& field_index,
      std::string_view format,
      std::string_view indent = {},
      std::string_view /*type*/ = {},
      std::string_view name = {},
      ...) {
    if (field_index < fields.size() && format.starts_with("%s%s %s =") && indent.length() == 2
        && indent.starts_with("  ")) {
      fields[field_index++] = std::string_view(name);
    }
    return 0;
  }

  inline static const FieldData kFieldNames = [] {  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::size_t field_index = 0;
    FieldData fields;
    Init(fields, field_index);
    return fields;
  }();
};

}  // namespace mbo::types::types_internal::clang

#endif  // __clang__
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
