// SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
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

#ifndef MBO_TYPES_INTERNAL_STRUCT_NAMES_GCC_H_
#define MBO_TYPES_INTERNAL_STRUCT_NAMES_GCC_H_

#if defined(__GNUC__) && !defined(__clang__)

# include <array>
# include <cstddef>
# include <memory>
# include <string_view>
# include <tuple>
# include <type_traits>
# include <utility>

# include "absl/types/span.h"                     // IWYU pragma: keep
# include "mbo/types/internal/decompose_count.h"  // IWYU pragma: keep
# include "mbo/types/tuple_extras.h"              // IWYU pragma: keep

// IWYU pragma: private, include "mbo/types/internal/struct_names.h"

namespace mbo::types::types_internal::gcc {

namespace struct_names_gcc_internal {

// This object is intentionally declared but never defined. It provides a
// compile-time address expression without constructing T. An accidental
// runtime use consequently becomes a link error.
template<typename T>
struct FakeObjectStorage {
  const T value;
};

template<typename T>
extern const FakeObjectStorage<T> kFakeObject;

template<typename T>
constexpr const T& FakeObject() noexcept {
  return kFakeObject<T>.value;
}

template<typename T, std::size_t kIndex>
constexpr auto FieldAddress() noexcept {
  return std::addressof(std::get<kIndex>(::mbo::types::StructToTuple(FakeObject<T>())));
}

template<auto kAddress>
using AddressAsTemplateArgument = void;

template<typename T, std::size_t kIndex>
concept FieldHasAddress = requires { typename AddressAsTemplateArgument<FieldAddress<T, kIndex>()>; };

template<typename T, std::size_t... kIndex>
consteval bool AllFieldsHaveAddresses(std::index_sequence<kIndex...> /*unused*/) noexcept {
  return (FieldHasAddress<T, kIndex> && ...);
}

template<typename T>
consteval bool AllFieldsHaveAddresses() noexcept {
  if constexpr (!::mbo::types::CanCreateTuple<T>) {
    return false;
  } else {
    return AllFieldsHaveAddresses<T>(
        std::make_index_sequence<::mbo::types::types_internal::DecomposeCountImpl<T>::value>{});
  }
}

template<typename T, auto kAddress>
consteval std::string_view FieldSignature() noexcept {
  return __PRETTY_FUNCTION__;
}

constexpr bool IsIdentifierByte(char ch) noexcept {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_'
         || static_cast<unsigned char>(ch) >= 0x80;
}

consteval std::string_view ExtractFieldName(std::string_view signature) noexcept {
  constexpr std::string_view kAddressMarker = "kAddress = ";
  const std::size_t marker = signature.rfind(kAddressMarker);
  if (marker == std::string_view::npos) {
    return {};
  }

  const std::size_t value_begin = marker + kAddressMarker.size();
  std::size_t value_end = signature.find(';', value_begin);
  if (value_end == std::string_view::npos) {
    value_end = signature.rfind(']');
  }
  if (value_end == std::string_view::npos || value_end <= value_begin) {
    return {};
  }

  while (value_end > value_begin && !IsIdentifierByte(signature[value_end - 1])) {
    --value_end;
  }
  std::size_t name_begin = value_end;
  while (name_begin > value_begin && IsIdentifierByte(signature[name_begin - 1])) {
    --name_begin;
  }
  return signature.substr(name_begin, value_end - name_begin);
}

template<typename T, std::size_t kIndex>
consteval std::string_view FieldName() noexcept {
  return ExtractFieldName(FieldSignature<T, FieldAddress<T, kIndex>()>());
}

struct ParserSentinel {
  int expected_field_name;
};

static_assert(FieldName<ParserSentinel, 0>() == "expected_field_name");

template<typename T, std::size_t kIndex>
inline constexpr auto kStoredFieldName = []() consteval {
  constexpr std::string_view kName = FieldName<T, kIndex>();
  std::array<char, kName.size() + 1> result{};
  for (std::size_t pos = 0; pos < kName.size(); ++pos) {
    result[pos] = kName[pos];
  }
  return result;
}();

template<typename T, std::size_t... kIndex>
consteval auto MakeFieldNames(std::index_sequence<kIndex...> /*unused*/) noexcept {
  return std::array<std::string_view, sizeof...(kIndex)>{
      std::string_view{kStoredFieldName<T, kIndex>.data(), kStoredFieldName<T, kIndex>.size() - 1}...,
  };
}

}  // namespace struct_names_gcc_internal

template<typename T>
concept SupportsFieldNames =
    std::is_class_v<T> && !std::is_array_v<T> && !std::is_union_v<T>
    && (IsEmptyType<T> || (::mbo::types::CanCreateTuple<T> && struct_names_gcc_internal::AllFieldsHaveAddresses<T>()));

template<typename T>
concept SupportsFieldNamesConstexpr = SupportsFieldNames<T>;

template<typename T, bool = SupportsFieldNames<T>>
class StructMeta final {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() noexcept {
    return absl::MakeConstSpan(kFieldNames);
  }

 private:
  static constexpr std::size_t kFieldCount =
      IsEmptyType<T> ? 0 : ::mbo::types::types_internal::DecomposeCountImpl<T>::value;
  inline static constexpr auto kFieldNames =
      struct_names_gcc_internal::MakeFieldNames<T>(std::make_index_sequence<kFieldCount>{});
};

template<typename T>
class StructMeta<T, false> final {
 public:
  static constexpr absl::Span<const std::string_view> GetFieldNames() noexcept { return {}; }
};

}  // namespace mbo::types::types_internal::gcc

#endif  // defined(__GNUC__) && !defined(__clang__)
#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_GCC_H_
