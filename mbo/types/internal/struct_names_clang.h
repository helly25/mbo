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

# include "absl/types/span.h"                     // IWYU pragma: keep
# include "mbo/types/internal/decompose_count.h"  // IWYU pragma: keep
# include "mbo/types/tuple_extras.h"              // IWYU pragma: keep

// IWYU pragma: private, include "mbo/types/internal/struct_names.h"

namespace mbo::types::types_internal::clang {

inline constexpr std::size_t kMaxFieldCount = 50;

template<typename T>
concept SupportsFieldNames =
    std::is_class_v<T> && std::is_default_constructible_v<T> && !std::is_array_v<T>  // Minimum requirement
    && std::is_destructible_v<T>  // Type T must have a constexpr destructor. But `std::is_literal_type` is deprecated,
    && !::mbo::types::HasUnionMember<T>;

template<typename T>
concept SupportsFieldNamesConstexpr =  // Constexpr capability is more restrictive.
    SupportsFieldNames<T>              // All general requirements, plus:
    && __is_literal_type(T);           // so the internal version has to be used.

template<typename T, bool, bool>
class StructMeta;

template<typename T>
class StructMetaBase {
 private:
  template<typename U, bool, bool>
  friend class StructMeta;

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
    constexpr Storage() noexcept { std::construct_at(&storage_.value); }

    constexpr ~Storage() noexcept { std::destroy_at(&storage_.value); }

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(Storage&&) = delete;

    constexpr const T& Get() const noexcept { return storage_.value; }

   private:
    const Uninitialized storage_;
  };

  // NOLINTBEGIN(*-swappable-parameters)

  static constexpr int DumpStructFieldCounter(  // NOLINT(cert-dcl50-cpp)
      std::size_t& field_index,
      std::string_view format,
      std::string_view indent = {},
      std::string_view /*type*/ = {},
      std::string_view /*name*/ = {},
      ...) {
    if (field_index < kMaxFieldCount && format.starts_with("%s%s %s =") && indent == "  ") {
      ++field_index;
    }
    return 0;
  }

  // NOLINTEND(*-swappable-parameters)

  static constexpr int FieldCount(const T* ptr, std::size_t& field_index) {
    __builtin_dump_struct(ptr, &DumpStructFieldCounter, field_index);  // NOLINT(*-vararg)
    return 0;
  }

  static constexpr std::size_t ComputeFieldCount() {
    std::size_t field_index{0};
    if constexpr (!std::is_empty_v<T>) {
      Storage storage{};
      FieldCount(&storage.Get(), field_index);
    }
    return field_index;
  }
};

template<typename T, bool = SupportsFieldNames<T> && !std::is_empty_v<T>, bool = SupportsFieldNamesConstexpr<T>>
class StructMeta final {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() {
    if constexpr (std::is_empty_v<T>) {
      return {};
    } else {
      return absl::MakeSpan(kFieldNames);
    }
  }

 private:
  static constexpr std::size_t kDecomposeCount = DecomposeCountImpl<T>::value;

  struct FieldInfo {
    std::string_view name;
    std::string_view type;
  };

  static constexpr std::size_t kFieldCount =
      DecomposeCondition<T> ? kDecomposeCount : StructMetaBase<T>::ComputeFieldCount();

  using FieldData = std::array<FieldInfo, kFieldCount>;

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
  inline static constexpr FieldData kFieldData = []() consteval {
    FieldData fields;
    if constexpr (!std::is_empty_v<T>) {
      typename StructMetaBase<T>::Storage storage{};
      std::size_t field_index = 0;
      Init(&storage.Get(), fields, field_index);
    }
    return fields;
  }();

  inline static constexpr std::array<std::string_view, kFieldCount> kFieldNames = []() consteval {
    std::array<std::string_view, kFieldCount> data;
    if constexpr (!std::is_empty_v<T>) {
      for (std::size_t pos = 0; pos < kFieldCount; ++pos) {
        data[pos] = kFieldData[pos].name;
      }
    }
    return data;
  }();
};

// Support for non literal types requires runtime operation.
template<typename T>
class StructMeta<T, true, false> final {
 public:
  static absl::Span<const std::string_view> GetFieldNames() {
    if constexpr (std::is_empty_v<T>) {
      return {};
    } else {
      return absl::MakeSpan(kFieldNames);
    }
  }

 private:
  static constexpr std::size_t kDecomposeCount = DecomposeCountImpl<T>::value;

  struct FieldInfo {
    std::string_view name;
    std::string_view type;
  };

  static constexpr std::size_t kFieldCount =
      DecomposeCondition<T> ? kDecomposeCount : StructMetaBase<T>::ComputeFieldCount();

  using FieldData = std::array<FieldInfo, kFieldCount>;

  // NOLINTBEGIN(*-swappable-parameters)

  static int DumpStructVisitor(  // NOLINT(cert-dcl50-cpp)
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

  static void Init(const T* ptr, FieldData& fields, std::size_t& field_index) {
    __builtin_dump_struct(ptr, &DumpStructVisitor, fields, field_index);  // NOLINT(*-vararg)
  }

  // Older compilers may not allow this to be a `constexpr`.
  inline static FieldData kFieldData = []() {
    FieldData fields;
    if constexpr (!std::is_empty_v<T>) {
      typename StructMetaBase<T>::Storage storage{};
      std::size_t field_index = 0;
      Init(&storage.Get(), fields, field_index);
    }
    return fields;
  }();

  inline static std::array<std::string_view, kFieldCount> kFieldNames = []() {
    std::array<std::string_view, kFieldCount> data;
    if constexpr (!std::is_empty_v<T>) {
      for (std::size_t pos = 0; pos < kFieldCount; ++pos) {
        data[pos] = kFieldData[pos].name;
      }
    }
    return data;
  }();
};

// No support possible.
template<typename T>
class StructMeta<T, false, false> final {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() { return {}; }

  static constexpr absl::Span<const std::string_view> GetFieldTypes() { return {}; }
};

}  // namespace mbo::types::types_internal::clang

#endif  // __clang__
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
