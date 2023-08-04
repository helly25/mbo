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
#include <type_traits>

#include "absl/types/span.h"
#include "mbo/types/internal/decompose_count.h"  // IWYU pragma: keep

// IWYU pragma: private, include "mbo/types/internal/struct_names.h"

namespace mbo::types::types_internal::clang {

template<typename T, bool = std::is_default_constructible_v<T> && !std::is_array_v<T>>
class StructMeta {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() { return absl::MakeSpan(kFieldNames); }

 private:
  using FieldData = std::array<std::string_view, DecomposeCountImpl<T>::value>;

  class Storage final {
   private:
    union Uninitialized {
      constexpr Uninitialized() noexcept {}
      constexpr ~Uninitialized() noexcept {};
      Uninitialized(const Uninitialized&) = delete;
      Uninitialized& operator=(const Uninitialized&) = delete;
      Uninitialized(Uninitialized&&) = delete;
      Uninitialized& operator=(Uninitialized&&) = delete;
      T value;
    };

   public:
    constexpr Storage() noexcept { std::construct_at(&storage_[0].value); }
    constexpr ~Storage() noexcept { std::destroy_at(&storage_[0].value); }

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    constexpr const T& Get() const noexcept { return storage_[0].value; }

   private:
    const std::array<Uninitialized, 1> storage_;
  };

  static constexpr void Init(const T* ptr, FieldData& fields, std::size_t& field_index) {
    __builtin_dump_struct(ptr, &DumpStructVisitor, fields, field_index);  // NOLINT(*-vararg)
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

  // Older compilers may not allow this to be a `constexpr`.
  inline static constexpr FieldData kFieldNames = []() constexpr {
    Storage storage{};
    std::size_t field_index = 0;
    FieldData fields;
    Init(&storage.Get(), fields, field_index);
    return fields;
  }();
};

template<typename T>
class StructMeta<T, false> {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() { return {}; }
};

}  // namespace mbo::types::types_internal::clang

#endif  // __clang__
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
