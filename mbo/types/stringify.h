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

// This header implements `Stringify`.
//
#ifndef MBO_TYPES_STRINGIFY_H_
#define MBO_TYPES_STRINGIFY_H_

// IWYU pragma private, include "mbo/types/extend.h"

#include <concepts>  // IWYU pragma: keep
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "mbo/types/internal/extender.h"      // IWYU pragma: keep
#include "mbo/types/internal/struct_names.h"  // IWYU pragma: keep
#include "mbo/types/traits.h"                 // IWYU pragma: keep
#include "mbo/types/tuple_extras.h"

namespace mbo::types {

struct StringifyOptions {
  enum class OutputMode {
    kDefault,
    kCpp,
    kJson,
  };

  friend std::ostream& operator<<(std::ostream& os, const OutputMode& value) {
    switch (value) {
      case OutputMode::kDefault: break;
      case OutputMode::kCpp: return os << "OutpuMode::kCpp";
      case OutputMode::kJson: return os << "OutpuMode::kJson";
    }
    return os << "OutpuMode::kDefault";
  }

  OutputMode output_mode = OutputMode::kDefault;

  // Field options:
  bool field_suppress = false;              // Allows complete suppression of the field.
  bool field_suppress_nullptr = false;      // Allows complete suppression of nullptr field values.
  bool field_suppress_nullopt = false;      // Allows complete suppression of nullopt field values.
  bool field_suppress_disabled = false;     // Allows complete suppression of disabled fields.
  std::string_view field_separator = ", ";  // Separator between two field (in front of field).

  // Key options:
  enum class KeyMode {
    kNone,  // No keys are shows. Not even `key_value_separator` will be used.
    kNormal,
    kNumericFallback,  // If not key is known use a numeric one from the field index.
  };

  friend std::ostream& operator<<(std::ostream& os, const KeyMode& value) {
    switch (value) {
      case KeyMode::kNone: break;
      case KeyMode::kNormal: return os << "KeyMode::kNormal";
      case KeyMode::kNumericFallback: return os << "KeyMode::kNumericFallback";
    }
    return os << "KeyMode::kNone";
  }

  KeyMode key_mode{KeyMode::kNormal};
  std::string_view key_prefix = ".";            // Prefix to key names.
  std::string_view key_suffix;                  // Suffix to key names.
  std::string_view key_value_separator = ": ";  // Seperator between key and value.
  std::string_view key_use_name;                // Force name for the key.

  // Value options:

  // If `true` (the default), then the value is printed as is.
  // Otherwise (`false`) all value format control is applied.
  bool value_other_types_direct = true;

  enum class EscapeMode {
    kNone,        // Values are printed as is.
    kCEscape,     // Values are C-escaped.
    kCHexEscape,  // Values are C-HEX-escaped.
  };

  friend std::ostream& operator<<(std::ostream& os, const EscapeMode& value) {
    switch (value) {
      case EscapeMode::kNone: break;
      case EscapeMode::kCEscape: return os << "EscapeMode::kCEscape";
      case EscapeMode::kCHexEscape: return os << "EscapeMode::kCHexEscape";
    }
    return os << "EscapeMode::kNone";
  }

  EscapeMode value_escape_mode{EscapeMode::kCEscape};

  std::string_view value_pointer_prefix = "*{";         // Prefix for pointer types.
  std::string_view value_pointer_suffix = "}";          // Suffix for pointer types.
  std::string_view value_smart_ptr_prefix = "{";        // Prefix for smart pointer types.
  std::string_view value_smart_ptr_suffix = "}";        // Suffix for smart pointer types.
  std::string_view value_nullptr_t = "std::nullptr_t";  // Value for `nullptr_t` types.
  std::string_view value_nullptr = "<nullptr>";         // Value for `nullptr` values.
  std::string_view value_optional_prefix = "{";         // Prefix for optional types.
  std::string_view value_optional_suffix = "}";         // Suffix for optional types.
  std::string_view value_nullopt = "std::nullopt";      // Value for `nullopt` values.
  std::string_view value_container_prefix = "{";        // Prefix for container values.
  std::string_view value_container_suffix = "}";        // Suffix for container values.

  // Max num elements to show.
  std::size_t value_container_max_len = std::numeric_limits<std::size_t>::max();

  std::string_view value_replacement_str;    // Allows "redacted" for string values
  std::string_view value_replacement_other;  // Allows "redacted" for non string values

  // Maximum length of value (prior to escaping).
  std::string_view::size_type value_max_length{std::string_view::npos};
  std::string_view value_cutoff_suffix = "...";  // Suffix if value gets shortened.

  // Special types:

  // Containers with Pairs whose `first` type is a string-like automatically
  // create objects where the keys are the field names. This is useful for JSON.
  // The types are checked with `IsPairFirstStr`.
  bool special_pair_first_is_name = false;

  // For all other cases of pairs, their names can be overwritten.
  std::string_view special_pair_first = "first";
  std::string_view special_pair_second = "second";

  // Arbirary default value.
  static const StringifyOptions& AsDefault() noexcept;

  // Formatting control that mostly produces C++ code.
  static const StringifyOptions& AsCpp() noexcept;

  // Formatting control that mostly produces JSON data.
  //
  // NOTE: JSON data requires field names. So unless Clang is used only with
  // types that support field names, they must be provided by an extension API
  // point: `MboTypesStringifyFieldNames` or `MboTypesStringifyOptions`, the
  // latter possibly with the `StringifyWithFieldNames` adapter. Alternatively,
  // numeric field names will be generated as a last resort.
  static const StringifyOptions& AsJson() noexcept;

  static const StringifyOptions& As(OutputMode mode) noexcept {
    switch (mode) {
      case OutputMode::kDefault: break;
      case OutputMode::kCpp: return AsCpp();
      case OutputMode::kJson: return AsJson();
    }
    return AsDefault();
  }
};

template<typename T>
concept IsFieldNameContainer = requires(const T& field_names) {
  { std::size(field_names) } -> std::equality_comparable_with<std::size_t>;
  { std::data(field_names)[1] } -> std::convertible_to<std::string_view>;
};

// Whether Stringify should suppress field names.
template<typename T>
concept HasMboTypesStringifyDoNotPrintFieldNames =
    requires { typename std::remove_cvref_t<T>::MboTypesStringifyDoNotPrintFieldNames; };

// This breaks `MboTypesStringifyOptions` lookup within this namespace.
void MboTypesStringifyOptions();  // Has no implementation!

// Identify presence for `AbslStringify` extender API extension point
// `MboTypesStringifyOptions`.
//
// That extension point can be used to perform fine grained control of field
// printing.
//
// NOTE: Unlike `MboTypesStringifyFieldNames` this extenion point cannot deal
// with `std::string_view` members referencing temp objects such as `std::string`
// since life-time extension CANNOT be applied as we always go via the actual
// `std::strig_view` fields.
//
// The return value can be `StringifyOptions` or `const StringifyOptions&` where
// the latter is preferred. However, when used with `StringifyWithFieldNames`
// the return value must be `StringifyOptions` (by copy).
//
// The implementation must therefore match one of the following:
// ```
// const StringifyOptions& Func(const T&, std::size_t, std::string_view, const StringifyOptions& default_options);
// StringifyOptions Alternative(const T&, std::size_t, std::string_view, const StringifyOptions& default_options);
// ```
template<typename T>
concept HasMboTypesStringifyOptions = requires(const T& v) {
  {
    MboTypesStringifyOptions(v, std::size_t{0}, std::string_view(), StringifyOptions())
  } -> IsSameAsRaw<StringifyOptions>;
};

// This breaks `MboTypesStringifyFieldNames` lookup within this namespace.
void MboTypesStringifyFieldNames();  // Has no implementation!

// Extension API point that disables printing for tagged types.
//
// Use this sparingly because it actually completely prevents any output from
// such types. If a field of a type tagged this way receives `StringifyOptions`
// with `field_suppress_disabled` set true, then there will be no output for the
// field at all. Otherwise the field name will be shown with a special marker
// `/* MboTypesStringifyDisable */`.
//
// If `Stringify` is used on a type tagged this way and the default options have
// `field_suppress_disabled` then there for consistency there will be no output
// at all.
//
// NOTE: The presence of a type named `MboTypesStringifyDisable` does not by
// itself enable `Stringify` for that type. However it can break any automatic
// call-chain and be used in parallel to `MboTypesStringifySupport` or CRTP
// types using `mbo::types::Extend`.
template<typename T>
concept HasMboTypesStringifyDisable = requires { typename T::MboTypesStringifyDisable; };

// Identify presence for `AbslStringify` extender API extension point
// `MboTypesStringifyFieldNames`.
//
// That extension point can be used to provide field names in cases where the
// compiler does not provide or where they should be different when printed.
//
// The function must comply with the following requirements:
// * The return value must be usable in:
//   * `std::size(result)`, and
//   * `std::data(result)`.
// * The values must be convertible to a `std::string_view`.
// * Temp containers of temp `std::string` values are admissible since life-time
//   extension is being applied.
template<typename T>
concept HasMboTypesStringifyFieldNames =
    requires(const T& v) { requires IsFieldNameContainer<decltype(MboTypesStringifyFieldNames(v))>; };

// Whether Stringify should automatically take affect if `T` is used as a sub field in a Stringify invocation.
// Otherwise the subfield needs its own support for printing (e.g. Abseil stringify support).
//
// Note that this takes precendence and thus disables Abseil stringify support in `Stringify`.
//
// This gets triggered by:
// * Presence of a type named `MboTypesStringifySupport`,
// * `T` qualifies for `HasMboTypesStringifyDoNotPrintFieldNames`,
// * `T` qualifies for `HasMboTypesStringifyFieldNames`, or
// * `T` qualified for `HasMboTypesStringifyOptions`,
template<typename T>
concept HasMboTypesStringifySupport =               //
    HasMboTypesStringifyDoNotPrintFieldNames<T> ||  // type `MboTypesStringifyDoNotPrintFieldNames`
    HasMboTypesStringifyFieldNames<T> ||            // function `MboTypesStringifyFieldNames`
    HasMboTypesStringifyOptions<T> ||               // function `MboTypesStringifyOptions`
    requires { typename std::remove_cvref_t<T>::MboTypesStringifySupport; };

enum class StringifyNameHandling {
  kOverwrite = 0,  // Use the provided names to override automatically determined names.
  kVerify = 1,     // Verify that the provided name matches the detrmined if possible.
};

template<typename T, typename ObjectType = mbo::types::types_internal::AnyType>
concept CanProduceStringifyOptions = std::is_assignable_v<
    StringifyOptions,
    std::invoke_result_t<T, ObjectType, std::size_t, std::string_view, const StringifyOptions&>>;

// Concept that verifies whether `T` can be used to construct (or assign to) an
// `StringifyOptions`.
//
// The actual `ObjectType` might not be evaluated immediately. This happens when
// an adapter does not know the type yet if it applies type-erasure techniques
// like `WithFieldNames` does. However, all concrete calls will verify their
// factual object type.
//
// Read more about producing `StringifyOptions` in the documnetation for
// `MboTypesStringifyOptions` and `HasMboTypesStringifyOptions`.
template<typename T, typename ObjectType = mbo::types::types_internal::AnyType>
concept IsOrCanProduceStringifyOptions =
    std::is_convertible_v<T, StringifyOptions> || CanProduceStringifyOptions<T, ObjectType>;

// Concept that verifies whether `T` has a static `MboTypesStringifyConvert`
// member function callable as:
//     `MboTypesStringifyConvert(const T& obj, std::size_t idx, const V& val)`
// Where `obj` is the owning object, `idx` is the field index and `val` is the
// actual field value.
template<typename T, typename V>
concept HasMboTypesStringifyConvert = requires {
  { T::MboTypesStringifyConvert(std::size_t{}, std::declval<const T&>(), std::declval<const V&>()) };
};

// Class `Stringify` implements the conversion of any aggregate into a string.
class Stringify {
 public:
  static Stringify AsCpp() noexcept { return Stringify(StringifyOptions::AsCpp()); }

  static Stringify AsJson() noexcept { return Stringify(StringifyOptions::AsJson()); }

  explicit Stringify(const StringifyOptions& default_options = StringifyOptions::AsDefault())
      : default_options_(default_options) {}

  explicit Stringify(const StringifyOptions::OutputMode output_mode)
      : default_options_(StringifyOptions::As(output_mode)) {}

  template<typename T>
  requires(std::is_aggregate_v<T>)
  std::string ToString(const T& value) const {
    std::ostringstream os;
    Stream<T>(os, value);
    return os.str();
  }

  template<typename T>
  requires(std::is_aggregate_v<T>)
  void Stream(std::ostream& os, const T& value) const {
    // It is not allowed to deny field name printing but provide filed names.
    static_assert(!(HasMboTypesStringifyDoNotPrintFieldNames<T> && HasMboTypesStringifyFieldNames<T>));

    StreamFieldsImpl<T>(os, value);
  }

 private:
  template<typename T>
  requires(HasMboTypesStringifyDisable<T>)
  void StreamFieldsImpl(
      std::ostream& os,
      const T& /*value*/,
      bool /*allow_field_names*/ = !HasMboTypesStringifyDoNotPrintFieldNames<T>) const {
    if (!default_options_.field_suppress_disabled) {
      os << "{/*MboTypesStringifyDisable*/}";
    }
    // Otherwise no output! Streaming `"{}"` would also be possible, but not streaming anything is
    // is inline with what field streaming does.
  }

  template<typename T, typename V>
  requires(HasMboTypesStringifyConvert<T, V>)
  static constexpr auto TsValue(std::size_t field_index, const T& object, const V& value) {
    return T::MboTypesStringifyConvert(field_index, object, value);
  }

  template<typename T, typename V>
  requires(!HasMboTypesStringifyConvert<T, V>)
  static constexpr const V& TsValue(std::size_t /*field_index*/, const T& /*object*/, const V& value) {
    return value;  // NOLINT(bugprone-return-const-ref-from-parameter)
  }

  template<typename T>
  requires(!HasMboTypesStringifyDisable<T>)
  void StreamFieldsImpl(
      std::ostream& os,
      const T& value,
      bool allow_field_names = !HasMboTypesStringifyDoNotPrintFieldNames<T>) const {
    allow_field_names |= default_options_.key_mode == StringifyOptions::KeyMode::kNumericFallback;
    bool use_seperator = false;
    const auto& field_names = GetFieldNames(value);
    std::apply(
        [&](const auto&... fields) {
          os << '{';
          std::size_t idx{0};
          ((StreamField(os, use_seperator, value, TsValue<T>(idx, value, fields), idx, field_names, allow_field_names),
            ++idx),
           ...);
          os << '}';
        },
        [&value]() {
          if constexpr (types_internal::IsExtended<T>) {
            return value.ToTuple();
          } else if constexpr (IsPair<T> || IsTuple<T>) {
            return value;  // works for pair and tuple.
          } else {
            return StructToTuple(value);
          }
        }());
  }

  template<typename T>
  static auto GetFieldNames(const T& value) {
    if constexpr (HasMboTypesStringifyDoNotPrintFieldNames<T>) {
      return std::array<std::string_view, 0>{};
    } else if constexpr (HasMboTypesStringifyFieldNames<T>) {
      return MboTypesStringifyFieldNames(value);
    } else {
      // Works for both constexpr and static/const aka compile-time and run-time cases.
      return ::mbo::types::types_internal::GetFieldNames<T>();
    }
  }

  enum class SpecialFieldValue {
    kNoSuppress = 0,
    kNormal,
    kIsNullptr,
    kIsNullopt,
    kStringifyDisabled,
  };

  template<typename T, typename Field, typename FieldNames>
  void StreamField(
      std::ostream& os,
      bool& use_seperator,
      const T& value,
      const Field& field,
      std::size_t idx,
      const FieldNames& field_names,
      bool allow_field_names) const {
    std::string_view field_name;
    if (allow_field_names && idx < std::size(field_names)) {
      field_name = std::data(field_names)[idx];
    }
    const auto& field_options = [&]() {
      if constexpr (HasMboTypesStringifyOptions<T>) {
        return MboTypesStringifyOptions(value, idx, field_name, default_options_);
      } else {
        return StringifyOptions::As(default_options_.output_mode);
      }
    }();
    using RawField = std::remove_cvref_t<Field>;
    const SpecialFieldValue is_special = [&field] {
      (void)field;
      if constexpr (std::is_pointer_v<RawField> || IsSmartPtr<RawField>) {
        return field == nullptr ? SpecialFieldValue::kIsNullptr : SpecialFieldValue::kNoSuppress;
      } else if constexpr (std::is_same_v<RawField, std::nullptr_t>) {
        return SpecialFieldValue::kIsNullptr;
      } else if constexpr (IsOptional<RawField>) {
        return field == std::nullopt ? SpecialFieldValue::kIsNullopt : SpecialFieldValue::kNoSuppress;
      } else if constexpr (HasMboTypesStringifyDisable<RawField>) {
        return SpecialFieldValue::kStringifyDisabled;
      }
      return SpecialFieldValue::kNormal;
    }();
    if (StreamFieldKey(os, use_seperator, idx, field_name, field_options, allow_field_names, is_special)) {
      StreamValue(os, field, field_options, allow_field_names);
    }
  }

  static bool StreamFieldKey(
      std::ostream& os,
      bool& use_seperator,
      std::size_t idx,
      std::string_view field_name,
      const StringifyOptions& field_options,
      bool allow_field_names,
      SpecialFieldValue is_special) {
    switch (is_special) {
      case SpecialFieldValue::kNoSuppress: break;
      case SpecialFieldValue::kNormal:
        if (field_options.field_suppress) {
          return false;
        }
        break;
      case SpecialFieldValue::kIsNullptr:
        if (field_options.field_suppress_nullptr) {
          return false;
        }
        break;
      case SpecialFieldValue::kIsNullopt:
        if (field_options.field_suppress_nullopt) {
          return false;
        }
        break;
      case SpecialFieldValue::kStringifyDisabled:
        if (field_options.field_suppress_disabled) {
          return false;
        }
        break;
    }
    if (use_seperator) {
      os << field_options.field_separator;
    }
    use_seperator = true;
    if (allow_field_names) {
      StreamFieldName(os, idx, field_name, field_options);
    }
    return true;
  }

  static void StreamFieldName(
      std::ostream& os,
      std::size_t idx,
      std::string_view field_name,
      const StringifyOptions& field_options,
      bool allow_key_override = true) {
    switch (field_options.key_mode) {
      case StringifyOptions::KeyMode::kNone: return;
      case StringifyOptions::KeyMode::kNormal:
      case StringifyOptions::KeyMode::kNumericFallback: {
        if (allow_key_override && !field_options.key_use_name.empty()) {
          field_name = field_options.key_use_name;
        }
        if (!field_name.empty()) {
          os << field_options.key_prefix << field_name << field_options.key_suffix << field_options.key_value_separator;
        } else if (field_options.key_mode == StringifyOptions::KeyMode::kNumericFallback) {
          os << field_options.key_prefix << idx << field_options.key_suffix << field_options.key_value_separator;
        }
      }
    }
  }

  template<typename C>
  requires(::mbo::types::ContainerIsForwardIteratable<C> && !std::convertible_to<C, std::string_view>)
  void StreamValue(std::ostream& os, const C& vs, const StringifyOptions& field_options, bool allow_field_names) const {
    if constexpr (mbo::types::IsPairFirstStr<std::remove_cvref_t<typename C::value_type>>) {
      if (field_options.special_pair_first_is_name) {
        // Each pair element of the container `vs` is an element whose key is the `first` member and
        // whose value is the `second` member.
        os << "{";
        std::string_view sep;
        std::size_t index = 0;
        for (const auto& v : vs) {
          if (index >= field_options.value_container_max_len) {
            break;
          }
          os << sep;
          sep = field_options.field_separator;
          if (allow_field_names) {
            StreamFieldName(os, index, v.first, field_options, /*allow_key_override=*/false);
          }
          StreamValue(os, v.second, field_options, allow_field_names);
          ++index;
        }
        os << "}";
        return;
      }
    }
    os << field_options.value_container_prefix;
    std::string_view sep;
    std::size_t index = 0;
    for (const auto& v : vs) {
      if (++index > field_options.value_container_max_len) {
        break;
      }
      os << sep;
      sep = field_options.field_separator;
      StreamValue(os, v, field_options, allow_field_names);
    }
    os << field_options.value_container_suffix;
  }

  template<typename T>
  static constexpr bool kUseStringify = types_internal::IsExtended<T> || HasMboTypesStringifySupport<T>
                                        || (std::is_aggregate_v<T> && !absl::HasAbslStringify<T>::value);

  template<typename V>  // xxxOLINTNEXTLINE(readability-function-cognitive-complexity)
  void StreamValue(std::ostream& os, const V& v, const StringifyOptions& field_options, bool allow_field_names) const {
    using RawV = std::remove_cvref_t<V>;
    // IMPORTANT: ALL if-clauses must be `if constexpr`.
    if constexpr (HasMboTypesStringifyDisable<RawV>) {
      os << "{/*MboTypesStringifyDisable*/}";  // TODO(helly25@github): Verify this always works.
    } else if constexpr (kUseStringify<RawV>) {
      Stream(os, v);
    } else if constexpr (std::is_same_v<RawV, std::nullptr_t>) {
      os << field_options.value_nullptr_t;
    } else if constexpr (IsSmartPtr<RawV>) {
      StreamValueSmartPtr(os, v, field_options, allow_field_names);
    } else if constexpr (std::is_pointer_v<RawV>) {
      StreamValuePointer(os, v, field_options, allow_field_names);
    } else if constexpr (mbo::types::IsOptional<RawV>) {
      StreamValueOptional(os, v, field_options, allow_field_names);
    } else if constexpr (mbo::types::IsPair<RawV>) {
      if constexpr (!HasMboTypesStringifyFieldNames<RawV> && !HasMboTypesStringifyOptions<RawV>) {
        if (!field_options.special_pair_first.empty() || !field_options.special_pair_second.empty()) {
          const std::array<std::string_view, 2> field_names{
              field_options.special_pair_first,
              field_options.special_pair_second,
          };
          bool use_seperator = false;
          os << "{";
          StreamField(os, use_seperator, v, v.first, 0, field_names, allow_field_names);
          StreamField(os, use_seperator, v, v.second, 1, field_names, allow_field_names);
          os << "}";
          return;
        }
      }
      StreamFieldsImpl(os, v, allow_field_names);
    } else if constexpr (std::is_same_v<RawV, char> || std::is_same_v<RawV, unsigned char>) {
      os << "'";
      std::string_view vv{reinterpret_cast<const char*>(&v), 1};  // NOLINT(*-type-reinterpret-cast)
      StreamValueStr(os, vv, field_options);
      os << "'";
    } else if constexpr (std::is_arithmetic_v<RawV>) {
      if (field_options.value_replacement_other.empty()) {
        os << absl::StreamFormat("%v", v);
      } else {
        os << field_options.value_replacement_other;
      }
    } else if constexpr (
        std::same_as<RawV, std::string_view> || std::same_as<V, std::string>
        || (std::is_convertible_v<V, std::string_view> && !absl::HasAbslStringify<V>::value)) {
      // Do not attempt to invoke string conversion for AbslStringify supported types as that breaks
      // this very implementation.
      os << '"';
      StreamValueStr(os, v, field_options);
      os << '"';
    } else {  // NOTE WHEN EXTENDING: Must always use `else if constexpr`
      StreamValueFallback(os, v, field_options);
    }
  }

  template<typename V>
  void StreamValueSmartPtr(std::ostream& os, const V& v, const StringifyOptions& field_options, bool allow_field_names)
      const {
    if (v) {
      os << field_options.value_smart_ptr_prefix;
      StreamValue(os, *v, field_options, allow_field_names);
      os << field_options.value_smart_ptr_suffix;
    } else {
      os << field_options.value_nullptr;
    }
  }

  template<typename V>
  void StreamValuePointer(std::ostream& os, const V& v, const StringifyOptions& field_options, bool allow_field_names)
      const {
    if (v) {
      os << field_options.value_pointer_prefix;
      StreamValue(os, *v, field_options, allow_field_names);
      os << field_options.value_pointer_suffix;
    } else {
      os << field_options.value_nullptr;
    }
  }

  template<typename V>
  void StreamValueOptional(std::ostream& os, const V& v, const StringifyOptions& field_options, bool allow_field_names)
      const {
    if (v.has_value()) {
      os << field_options.value_optional_prefix;
      StreamValue(os, *v, field_options, allow_field_names);
      os << field_options.value_optional_suffix;
    } else {
      os << field_options.value_nullopt;
    }
  }

  template<typename V>
  static void StreamValueFallback(std::ostream& os, const V& v, const StringifyOptions& field_options) {
    if (field_options.value_other_types_direct) {
      if (field_options.value_replacement_other.empty()) {
        os << absl::StreamFormat("%v", v);
      } else {
        os << field_options.value_replacement_other;
      }
    } else {
      const std::string vv = absl::StrFormat("%v", v);
      StreamValueStr(os, vv, field_options);
    }
  }

  static void StreamValueStr(std::ostream& os, std::string_view v, const StringifyOptions& field_options) {
    if (!field_options.value_replacement_str.empty()) {
      os << field_options.value_replacement_str;
      return;
    }
    std::string_view vvv{v};
    if (field_options.value_max_length != std::string_view::npos) {
      vvv = vvv.substr(0, field_options.value_max_length);
    }
    switch (field_options.value_escape_mode) {
      case StringifyOptions::EscapeMode::kNone: os << vvv; break;
      case StringifyOptions::EscapeMode::kCEscape: os << absl::CEscape(vvv); break;
      case StringifyOptions::EscapeMode::kCHexEscape: os << absl::CHexEscape(vvv); break;
    }
    if (vvv.length() < v.length()) {
      os << field_options.value_cutoff_suffix;
    }
  }

  const StringifyOptions& default_options_;
};

// Adapter (not Extender) that injects field names into field control.
//
// Parameter `field_options` must be an instance of `StringifyOptions` or
// something that can produce those. It defaults to `MboTypesStringifyOptions`
// if available and otherwise the default `StringifyOptions` will be used.
//
// If `field_names` is true, then the injected field names must match the actual
// field names. Otherwise the provided `field_names` take precedence over
// compile time determined names.
//
// NOTE: If the field names are constants and all that needs to be done is to
// provide field names, then `MboTypesStringifyFieldNames` is a much better API
// extension point.
//
//
// Example:
//
// ```c++
// struct MyType : mbo::types::Extend<MyType> {
//   int one = 25;
//   int two = 42;
//
//   friend StringifyOptions MboTypesStringifyOptions(
//       const MyType& v, std::size_t idx, std::string_view name,
//       const mbo::types::StringifyOptions& default_options) {
//     return StringifyWithFieldNames(
//         StringifyOptions::AsDefault(),
//         {"one", "two"})(v, idx, name, default_options);
//   }
// };
// ```
template<
    IsOrCanProduceStringifyOptions FieldOptions,
    IsFieldNameContainer FieldNames = std::initializer_list<std::string_view>>
inline constexpr auto StringifyWithFieldNames(
    const FieldOptions& field_options,
    const FieldNames& field_names,
    StringifyNameHandling name_handling = StringifyNameHandling::kVerify) {
  // In order to provide overrides, the actual target `options` have to be created.
  // Once those are available, the overrides can be applied.
  return [&field_options, &field_names, name_handling]<typename T>(
             [[maybe_unused]] const T& v, std::size_t field_index, std::string_view field_name,
             [[maybe_unused]] const StringifyOptions& default_options) {
    StringifyOptions options = [&] {
      if constexpr (
          std::is_assignable_v<StringifyOptions, FieldOptions>
          || std::is_convertible_v<FieldOptions, StringifyOptions>) {
        return StringifyOptions(field_options);
      } else {
        // Here `field_options` must be `CanProduceStringifyOptions`
        return field_options(v, field_index, field_name, default_options);
      }
    }();
    if (field_index >= std::size(field_names)) {
      return options;
    }
    options.key_use_name = std::data(field_names)[field_index];  // NOLINT(*-pointer-arithmetic)
    if (types_internal::SupportsFieldNames<T> && name_handling == StringifyNameHandling::kVerify) {
      ABSL_CHECK_EQ(field_name, options.key_use_name) << "Bad field_name injection for field #" << field_index;
    }
    return options;
  };
}

}  // namespace mbo::types

#endif  // MBO_TYPES_STRINGIFY_H_
