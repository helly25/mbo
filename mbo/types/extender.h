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

// This header contains extenders to be used with `mbo::types::extend`.
//
// The following extenders are available:
//
// * Printable (default)
//
//   * Provides method `std::string ToString()` which generates a string
//     represenation of the fields of the extended class as `"{value, value}"`.
//   * This is a default extender.
//   * Requires `AbslStringify`
//
// * Streamable
//
//   * Provides an `std:::ostream& operator<<` that uses the same format as
//     `Printable`.
//   * This is a default extender.
//   * Requires `AbslStringify`
//
// * Comparable
//
//   * Implements all comparison operators by means of `<=>` referring to the
//     `<=>` operator for the temporary `std::tuple` copies.
//   * Since the comparison is implemented with templated left/right parameters,
//     it might be necessary to provide concrete implementations for operators
//     `==` and/or `<` for types that have no known conversion or comparison.
//
// * AbslStringify (default)
//
//   * Provides: friend void AbslStringify(Sink&, const Type& t)
//   * Provides: private void OStreamFields(std::ostream& os) const
//   * Enables use of the struct with `absl::Format` library.
//   * Required by: Printable, Streamable
//
// * AbslHashable
//
//   * Make the extended type hashable.
//
// * Default:
//
//    * Printable, Streamable, Comparable, AbslHashable, AbslStringify.
//
//  * NoPrint:
//
//    * Comparable, AbslHashable, AbslStringify.
//
#ifndef MBO_TYPES_EXTENDER_H_
#define MBO_TYPES_EXTENDER_H_

// IWYU pragma private, include "mbo/types/extend.h"

#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include "absl/hash/hash.h"  // IWYU pragma: keep
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "mbo/types/internal/extender.h"      // IWYU pragma: export
#include "mbo/types/internal/struct_names.h"  // IWYU pragma: keep
#include "mbo/types/traits.h"                 // IWYU pragma: keep
#include "mbo/types/tstring.h"
#include "mbo/types/tuple.h"

namespace mbo::types {

// Simplify generation of an 'Extender'. Instead of writing the whole template
// struct of struct with all necessary provisions, it is enough to provide the
// struct for the actual implementation. E.g.:
//
// ```c++
//   template <typename ExtenderBase>
//   struct MyExtenderImpl : ExtenderBase {
//     using Type = typename ExtenderBase::Type;  // Important!
//     // Implementation
//   };
//
//   using MyExtender =
//       mbo::types::MakeExtender<"MyExtender"_ts, MyExtenderImpl>;
// ```
//
// NOTE: The implementation must be a `struct`, it cannot be a `class`.
//
// NOTE: If the implementation requires another Extender to be a base, then
// the `RequiredExtender` template parameter can be set to that Extender (not
// its implementation). As a result, the required Extender must be listed prior
// to this Extender in the `Extend` template parameter list.
// The final `MyExtender` can be a class or struct and it can be constructed
// with a 'using' directive. However, generating a class or struct is preferable
// as it provides better compiler error message in the event of requirement
// violations.
//
// The following example demonstrates construction of extender `MyExtender` that
// depends on `Printable`.
//
// ```c++
//   template <typename ExtenderBase>
//   struct MyExtenderImpl : ExtenderBase {
//     // Implementation
//   };
//
//   struct MyExtender final : mbo::types::MakeExtender<
//       "MyExtender"_ts,
//       MyExtenderImpl,
//       mbo::types::Printable>;
// ```
template<::mbo::types::tstring ExtenderNameT, template<typename> typename ImplT, typename RequiredExtenderT = void>
struct MakeExtender {
  using RequiredExtender = RequiredExtenderT;

  static constexpr std::string_view GetExtenderName() { return decltype(ExtenderNameT)::str(); }

 private:
  // This friend is necessary to keep symbol visibility in check. But it has to
  // be either 'struct' or 'class' and thus results in `ImplT` being a struct.
  template<typename T, typename Extender>
  friend struct ::mbo::types::types_internal::UseExtender;

  template<typename ExtenderOrActualType>
  struct Impl : ImplT<ExtenderOrActualType> {
    using Type = typename ExtenderOrActualType::Type;
  };
};

struct AbslStringifyFieldOptions {
  // Field options:
  bool field_suppress = false;              // Allows to completely suppress the field.
  std::string_view field_separator = ", ";  // Separator between two field.

  // Key options:
  enum class KeyMode {
    kNone,  // No keys are shows. Not even `key_value_separator` will be used.
    kNormal,
  };
  KeyMode key_mode{KeyMode::kNormal};
  std::string_view key_prefix = ".";            // Prefix to key names.
  std::string_view key_suffix;                  // Suffix to key names.
  std::string_view key_value_separator = ": ";  // Seperator between key and value.
  std::string_view key_use_name{};              // Force name for the key.

  // Value options:

  // If `true` (the default), then the value is printed as is.
  // Otherwise (`false`) all value format control is applied.
  bool value_other_types_direct = true;

  enum class EscapeMode {
    kNone,        // Values are printed as is.
    kCEscape,     // Values are C-escaped.
    kCHexEscape,  // Values are C-HEX escaped.
  };
  EscapeMode value_escape_mode{EscapeMode::kCEscape};

  std::string_view value_pointer_prefix = "*{";    // Prefix for pointer types.
  std::string_view value_pointer_suffix = "}";     // Suffix for pointer types.
  std::string_view value_nullptr_t = "nullptr_t";  // Value for `nullptr_t` types.
  std::string_view value_nullptr = "<nullptr>";    // Value for `nullptr` values.
  std::string_view value_container_prefix = "{";   // Prefix for container values.
  std::string_view value_container_suffix = "}";   // Suffix for container values.

  // Max num elements to show.
  std::size_t value_container_max_len = std::numeric_limits<std::size_t>::max();

  std::string_view value_replacement_str;    // Allows "redacted" for string values
  std::string_view value_replacement_other;  // Allows "redacted" for non string values

  // Maximum length of value (prior to escaping).
  std::string_view::size_type value_max_length{std::string_view::npos};
  std::string_view value_cutoff_suffix = "...";  // Suffix if value gets shortened.

  // Arbirary default value.
  static constexpr AbslStringifyFieldOptions Default() noexcept { return {}; }

  // Formatting control that mostly produces C++ code.
  static constexpr AbslStringifyFieldOptions Cpp() noexcept {
    return {
        .key_prefix = ".",
        .key_value_separator = " = ",
        .value_pointer_prefix = "",
        .value_pointer_suffix = "",
        .value_nullptr_t = "nullptr",
        .value_nullptr = "nullptr",
    };
  }

  // Formatting control that mostly produces JSON data.
  static constexpr AbslStringifyFieldOptions Json() noexcept {
    return {
        .key_prefix = "\"",
        .key_suffix = "\"",
        .key_value_separator = ": ",
        .value_pointer_prefix = "",
        .value_pointer_suffix = "",
        .value_nullptr_t = "0",
        .value_nullptr = "0",
        .value_container_prefix = "[",
        .value_container_suffix = "]",
    };
  }
};

template<typename T>
concept HasAbslStringifyFieldOptions = requires(const T& v, const T::Type& val) {
  { v.AbslStringifyFieldOptions(val, std::size_t{0}, std::string_view()) } -> std::same_as<AbslStringifyFieldOptions>;
};

template<typename ExtenderBase>
struct AbslStringify_ : ExtenderBase {  // NOLINT(readability-identifier-naming)
  using Type = typename ExtenderBase::Type;

  void OStreamFields(std::ostream& os) const {
    OStreamFieldsImpl(os, static_cast<const Type&>(*this), this->ToTuple());
  }

  template<typename Sink>
  friend void AbslStringify(Sink& sink, const Type& value) {
    std::ostringstream os;
    value.OStreamFields(os);
    sink.Append(os.str());
  }

 private:
  // Override possibly "inherited" `AbslStringifyFieldOptions` which receives the `field_index` and
  // the `field_name` if present (otherwise it will be an empty string).
  static constexpr struct AbslStringifyFieldOptions GetAbslStringifyFieldOptions(
      const ExtenderBase::Type& value,
      std::size_t field_index,
      std::string_view field_name) noexcept {
    if constexpr (HasAbslStringifyFieldOptions<typename ExtenderBase::Type>) {
      return ExtenderBase::Type::AbslStringifyFieldOptions(value, field_index, field_name);
    } else {
      return {};
    }
  }

  template<typename... Ts>
  void OStreamFieldsImpl(std::ostream& os, const Type& value, const std::tuple<Ts...>& v) const {
    bool use_seperator = false;
    std::apply(
        [&os, &value, &use_seperator](const Ts&... fields) {
          os << '{';
          std::size_t idx{0};  // NOLINT(misc-const-correctness)
          (OStreamField(os, value, use_seperator, idx++, fields), ...);
          os << '}';
        },
        v);
  }

  template<typename V>
  static void OStreamField(std::ostream& os, const Type& value, bool& use_seperator, std::size_t idx, const V& v) {
    std::string_view field_name;
    if constexpr (!requires { typename Type::MboTypesExtendDoNotPrintFieldNames; }) {
      static constexpr auto kNames = ::mbo::types::types_internal::GetFieldNames<Type>();
      if (idx < kNames.length() && !kNames[idx].empty()) {
        field_name = kNames[idx];
      }
    }
    const auto options = GetAbslStringifyFieldOptions(value, idx, field_name);
    if (options.field_suppress) {
      return;
    }
    if (use_seperator) {
      os << options.field_separator;
    }
    use_seperator = true;
    OStreamKey(os, field_name, options);
    OStreamValue(os, v, options);
  }

  static void OStreamKey(std::ostream& os, std::string_view key, const struct AbslStringifyFieldOptions& options) {
    switch (options.key_mode) {
      case AbslStringifyFieldOptions::KeyMode::kNone: return;
      case AbslStringifyFieldOptions::KeyMode::kNormal:
        if (!options.key_use_name.empty()) {
          key = options.key_use_name;
        }
        if (key.empty()) {
          return;
        }
        os << options.key_prefix << key << options.key_suffix << options.key_value_separator;
        return;
    }
  }

  template<typename C>
  requires(::mbo::types::ContainerIsForwardIteratable<C> && !std::convertible_to<C, std::string_view>)
  static void OStreamValue(std::ostream& os, const C& vs, const struct AbslStringifyFieldOptions& options) {
    os << options.value_container_prefix;
    std::string_view sep;
    std::size_t index = 0;
    for (const auto& v : vs) {
      if (++index > options.value_container_max_len) {
        break;
      }
      os << sep;
      sep = ", ";
      OStreamValue(os, v, options);
    }
    os << options.value_container_suffix;
  }

  template<typename V>
  static void OStreamValue(std::ostream& os, const V& v, const struct AbslStringifyFieldOptions& options) {
    using RawV = std::remove_cvref_t<V>;
    if constexpr (types_internal::IsExtended<RawV>) {
      os << absl::StreamFormat("%v", v);
    } else if constexpr (std::is_same_v<RawV, std::nullptr_t>) {
      os << options.value_nullptr_t;
    } else if constexpr (std::is_pointer_v<RawV>) {
      if (v) {
        os << options.value_pointer_prefix;
        OStreamValue(os, *v, options);
        os << options.value_pointer_suffix;
      } else {
        os << options.value_nullptr;
      }
    } else if constexpr (std::same_as<RawV, std::string_view> || std::same_as<V, std::string>) {
      os << '"';
      OStreamValueStr(os, v, options);
      os << '"';
      // Do not attempt to invoke string conversion as that breaks this very implementation.
      //} else if constexpr (std::is_convertible_v<V, std::string_view>) {
    } else if constexpr (std::is_same_v<RawV, char>) {
      char vv[2] = {v, '\0'};       // NOLINT(*-avoid-c-arrays)
      std::string_view vvv(vv, 1);  // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
      os << "'";
      OStreamValueStr(os, vvv, options);
      os << "'";
    } else if constexpr (std::is_arithmetic_v<RawV>) {
      if (options.value_replacement_other.empty()) {
        os << absl::StreamFormat("%v", v);
      } else {
        os << options.value_replacement_other;
      }
    } else {
      if (options.value_other_types_direct) {
        if (options.value_replacement_other.empty()) {
          os << absl::StreamFormat("%v", v);
        } else {
          os << options.value_replacement_other;
        }
      } else {
        const std::string vv = absl::StrFormat("%v", v);
        OStreamValueStr(os, vv, options);
      }
    }
  }

  static void OStreamValueStr(std::ostream& os, std::string_view v, const struct AbslStringifyFieldOptions& options) {
    if (!options.value_replacement_str.empty()) {
      os << options.value_replacement_str;
      return;
    }
    std::string_view vvv{v};
    if (options.value_max_length != std::string_view::npos) {
      vvv = vvv.substr(0, options.value_max_length);
    }
    switch (options.value_escape_mode) {
      case AbslStringifyFieldOptions::EscapeMode::kNone: os << vvv; break;
      case AbslStringifyFieldOptions::EscapeMode::kCEscape: os << absl::CEscape(vvv); break;
      case AbslStringifyFieldOptions::EscapeMode::kCHexEscape: os << absl::CHexEscape(vvv); break;
    }
    if (vvv.length() < v.length()) {
      os << options.value_cutoff_suffix;
    }
  }
};

template<typename ExtenderBase>
struct AbslHashable_ : ExtenderBase {  // NOLINT(readability-identifier-naming)
 private:
  using T = typename ExtenderBase::Type;

 public:
  template<typename H>
  friend H AbslHashValue(H hash, const T& obj) noexcept {
    return H::combine(std::move(hash), obj.ToTuple());
  }
};

template<typename ExtenderBase>
struct Comparable_ : ExtenderBase {  // NOLINT(readability-identifier-naming)
 private:
  using T = typename ExtenderBase::Type;

 public:
  // Define operator `<=>` on `const T&` since that is what defines the
  // comparisons for the intended type `T`. So we cannot simply default the
  // operator.
  // Also provide backing `==` and `<` operators.
  // Further this has to be consistend so all provided operators are on type 'T'
  // and implemented as friends. Otherwise this would trigger warning: ISO C++20
  // considers use of overloaded operator '==' (with operand types 'const
  // TestComparable' and 'const TestComparable') to be ambiguous despite there
  // being a unique best viable function [-Wambiguous-reversed-operator].
  // The core problem stems from synthesized operators interfering with finding
  // the best candidate. Also see:
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2468r2.html
  friend auto operator<=>(const T& lhs, const T& rhs) { return lhs.ToTuple() <=> rhs.ToTuple(); }

  friend bool operator==(const T& lhs, const T& rhs) { return lhs.ToTuple() == rhs.ToTuple(); }

  friend bool operator<(const T& lhs, const T& rhs) { return lhs.ToTuple() < rhs.ToTuple(); }

  // Define operator `<=>` on `const T&` and another type.
  template<typename Other, typename = decltype(StructToTuple(std::declval<const Other&>()))>
  auto operator<=>(const Other& other) const {
    return this->ToTuple() <=> StructToTuple(other);
  }

  template<IsTuple Other>
  auto operator<=>(const Other& other) const {
    return this->ToTuple() <=> other;
  }

  template<typename Other, typename = decltype(StructToTuple(std::declval<const Other&>()))>
  bool operator==(const Other& other) const {
    return this->ToTuple() == StructToTuple(other);
  }

  template<IsTuple Other>
  bool operator==(const Other& other) const {
    return this->ToTuple() == other;
  }

  template<typename Other, typename = decltype(StructToTuple(std::declval<const Other&>()))>
  bool operator<(const Other& other) const {
    return this->ToTuple() < StructToTuple(other);
  }

  template<IsTuple Other>
  bool operator<(const Other& other) const {
    return this->ToTuple() < other;
  }
};

template<typename ExtenderBase>
struct Printable_ : ExtenderBase {  // NOLINT(readability-identifier-naming)
 public:
  std::string ToString() const {
    std::ostringstream os;
    this->OStreamFields(os);
    return os.str();
  }
};

template<typename ExtenderBase>
struct Streamable_ : ExtenderBase {  // NOLINT(readability-identifier-naming)
 public:
  friend std::ostream& operator<<(std::ostream& os, const ExtenderBase::Type& v) {
    v.OStreamFields(os);
    return os;
  }
};

namespace extender {

// Extender that injects functionality to make an `Extend`ed type work with
// abseil format/print functions (see
// [AbslStringify](https://abseil.io/docs/cpp/guides/format#abslstringify)).
//
// This default Extender is automatically available through `mb::types::Extend`
// and `mbo::types::NoPrint`.
//
// If the compiler and the structure support `__buildtin_dump_struct` (e.g. if
// compiled with Clang), then this automatically supports field names. However,
// this does not work with `union`s. Further, providing field names can be
// suppressed by providing typename `MboTypesExtendDoNotPrintFieldNames`, e.g.:
//   `using MboTypesExtendDoNotPrintFieldNames = void;`
// Once presnet, the compiler will no longer generate code to fetch field names.
//
// Note that even if field names are disabled using the above type injection,
// supporting field names is still manually possible, and thus indepedent of
// compiler support if necessary using `AbslStringifyFieldOptions`, see below.
//
// The implementation allows for complex formatting control by implementing the
// static member func `AbslStringifyFieldOptions(self, field_index, field_name)`
// which must return `struct mbo::types::AbslStringifyFieldOptions`. That struct
// contains the full documenation. While the function must technically be static
// its first parameter is the object itself. Not that the correct type for
// the first parameter `self` in the absence of C++23 is `Type` as provided by
// `Extend`. Example:
// ```
// struct Test : mbo::types::Extend<Test> {
//   int number;
//   static struct mbo::types::AbslStringifyFieldOptions AbslStringifyFieldOptions(
//       const Type& /* unused */,
//       std::size_t field_index,
//       std::string_view field_name) {
//     return {
//       .value_max_length = 42,
//     };
//   }
// };
// ```
struct AbslStringify final : MakeExtender<"AbslStringify"_ts, AbslStringify_> {};

// Extender that injects functionality to make an `Extend`ed type work with
// abseil hashing (and also `std::hash`).
//
// Example:
// ```c++
// struct Name : mbo::types::Extend<Name> {
//   std::string first;
//   std::string last;
// };
//
// void demo() {
//   const Name name{.first = "first", .last = "last"};
//   const std::size_t hash = absl::HashOf(name);
//   const std::size_t std_hash = std::hash<Name>{}(name);
//   static_assert(hash == std_hash, "Must be the same");
// }
// ```
//
// This default Extender is automatically available through `mb::types::Extend`
// and `mbo::types::NoPrint`.
struct AbslHashable final : MakeExtender<"AbslHashable"_ts, AbslHashable_> {};

// Extender that injects functionality to make an `Extend`ed type comparable.
// All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
//
// This default Extender is automatically available through `mb::types::Extend`
// and `mbo::types::NoPrint`.
struct Comparable final : MakeExtender<"Comparable"_ts, Comparable_> {};

// Extender that injects functionality to make an `Extend`ed type get a
// `ToString` function which can be used to convert a type into a `std::string`.
//
// This default Extender is automatically available through `mb::types::Extend`.
struct Printable final : MakeExtender<"Printable"_ts, Printable_, AbslStringify> {};

// Extender that injects functionality to make an `Extend`ed type streamable.
// This allows the type to be used directly with `std::ostream`s.
//
// This default Extender is automatically available through `mb::types::Extend`.
struct Streamable final : MakeExtender<"Streamable"_ts, Streamable_, AbslStringify> {};

namespace extender_internal {
// Temporarily unused: This would allow to simplify the default wrapper combos (e.g. `Default`).
// However it also massively increases their typename lengths. Use:
//  template<typename ExtenderOrActualType>
//  using Impl : Expand<ExtenderOrActualType, ExtenderTuple>;
#if MBO_TYPES_EXTENDER_USE_EXPAND
template<typename... T>
struct Expand;

template<typename Base>
struct Expand<Base, std::tuple<>> : Base {
  using Type = Base::Type;
};

template<typename Base, typename T>
struct Expand<Base, std::tuple<T>> : ::mbo::types::types_internal::UseExtender<Base, T>::type {
  using Type = Base::Type;
};

template<typename Base, typename T, typename... U>
struct Expand<Base, std::tuple<T, U...>>
    : ::mbo::types::types_internal::UseExtender<Expand<Base, std::tuple<U...>>, T>::type {};
#endif
}  // namespace extender_internal

// The default extender is a wrapper around:
// * Streamable
// * Printable
// * Comparable
// * AbslHashable
// * AbslStringify
struct Default final {
  using RequiredExtender = void;
  using ExtenderTuple = std::tuple<Streamable, Printable, Comparable, AbslStringify, AbslHashable>;

  static constexpr std::string_view GetExtenderName() { return decltype("Default"_ts)::str(); }

 private:
  // This friend is necessary to keep symbol visibility in check. But it has to
  // be either 'struct' or 'class' and thus results in `ImplT` being a struct.
  template<typename T, typename Extender>
  friend struct ::mbo::types::types_internal::UseExtender;

  // NOTE: The list of wrapped extenders needs to be in sync with `ExtenderTuple`.

  template<typename ExtenderOrActualType>
  struct Impl : Streamable_<Printable_<Comparable_<AbslHashable_<AbslStringify_<ExtenderOrActualType>>>>> {
    using Type = ExtenderOrActualType::Type;
  };
};

// This default extender is a wrapper around:
// * NOT Streamable
// * NOT Printable
// * Comparable
// * AbslHashable
// * AbslStringify
struct NoPrint final {
  using RequiredExtender = void;
  using ExtenderTuple = std::tuple<AbslHashable, AbslStringify, Comparable>;

  static constexpr std::string_view GetExtenderName() { return decltype("NoPrint"_ts)::str(); }

 private:
  // This friend is necessary to keep symbol visibility in check. But it has to
  // be either 'struct' or 'class' and thus results in `ImplT` being a struct.
  template<typename T, typename Extender>
  friend struct ::mbo::types::types_internal::UseExtender;

  // NOTE: The list of wrapped extenders needs to be in sync with `ExtenderTuple`.

  template<typename ExtenderOrActualType>
  struct Impl : Comparable_<AbslHashable_<AbslStringify_<ExtenderOrActualType>>> {
    using Type = ExtenderOrActualType::Type;
  };
};

}  // namespace extender

// using namespace extender;

}  // namespace mbo::types

// Add a convenience namespace to clearly isolate just the extenders.
namespace mbo::extender {
using namespace ::mbo::types::extender;  // NOLINT(google-build-using-namespace)
}  // namespace mbo::extender

#endif  // MBO_TYPES_EXTENDER_H_
