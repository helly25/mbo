// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
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

#ifndef MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
#define MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
#if defined(__clang__) && __has_builtin(__builtin_dump_struct)

# include <string_view>
# include <type_traits>

# include "absl/types/span.h"
# include "mbo/types/internal/decompose_count.h"  // IWYU pragma: keep
# include "mbo/types/tuple.h"                     // IWYU pragma: keep

// IWYU pragma: private, include "mbo/types/internal/struct_names.h"

namespace mbo::types::types_internal::clang {

template<typename T>
concept SupportsFieldNames = std::is_default_constructible_v<T> && !std::is_array_v<T>  // Minimum requirement
                             && !::mbo::types::HasUnionMember<T>;

template<typename T, bool = SupportsFieldNames<T>>
class StructMeta {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() { return absl::MakeSpan(kFieldNames); }

 private:
  static constexpr std::size_t kFieldCount = DecomposeCountImpl<T>::value;

  struct FieldInfo {
    std::string_view name;
    std::string_view type;
  };

  using FieldData = std::array<FieldInfo, kFieldCount>;

  class Storage final {
   private:
    union Uninitialized {
      constexpr Uninitialized() noexcept {}

      constexpr ~Uninitialized() noexcept {}

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

  // NOLINTBEGIN(*-swappable-parameters)
  static constexpr int DumpStructVisitor(  // NOLINT(cert-dcl50-cpp)
      FieldData& fields,
      std::size_t& field_index,
      std::string_view format,
      std::string_view indent = {},
      std::string_view type = {},
      std::string_view name = {},
      ...) {
    if (field_index < fields.size() && format.starts_with("%s%s %s =") && indent == "  ") {
      fields[field_index++] = {
          // NOLINT(*-array-index)
          .name = name,
          .type = type,
      };
    }
    return 0;
  }

  // NOLINTEND(*-swappable-parameters)

  static constexpr void Init(const T* ptr, FieldData& fields, std::size_t& field_index) {
    __builtin_dump_struct(ptr, &DumpStructVisitor, fields, field_index);  // NOLINT(*-vararg)
  }

  // Older compilers may not allow this to be a `constexpr`.
  inline static constexpr FieldData kFieldData = []() constexpr {
    Storage storage{};
    std::size_t field_index = 0;
    FieldData fields;
    Init(&storage.Get(), fields, field_index);
    return fields;
  }();

  inline static constexpr std::array<std::string_view, kFieldCount> kFieldNames = []() constexpr {
    std::array<std::string_view, kFieldCount> data;
    for (std::size_t pos = 0; pos < kFieldCount; ++pos) {
      data[pos] = kFieldData[pos].name;
    }
    return data;
  }();
};

template<typename T>
class StructMeta<T, false> {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() { return {}; }

  static constexpr absl::Span<const std::string_view> GetFieldTypes() { return {}; }
};

}  // namespace mbo::types::types_internal::clang

#endif  // __clang__
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
