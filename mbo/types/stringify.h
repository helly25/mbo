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
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "mbo/config/require.h"
#include "mbo/types/internal/extender.h"      // IWYU pragma: keep
#include "mbo/types/internal/struct_names.h"  // IWYU pragma: keep
#include "mbo/types/optional_data_or_ref.h"
#include "mbo/types/traits.h"  // IWYU pragma: keep
#include "mbo/types/tuple_extras.h"

namespace mbo::types {

struct StringifyFieldInfo;

using StringifyFieldInfoString = std::function<std::string_view(const StringifyFieldInfo&)>;

#undef MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
#if !defined(__clang__) || (__clang_major__ >= 19)
# define MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
constexpr bool kLiteralOptionalfunction = true;
#else   // !defined(__clang__) || (__clang_major__ >= 19)
constexpr bool kLiteralOptionalfunction = false;
#endif  // !defined(__clang__) || (__clang_major__ >= 19)
#ifdef MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
static_assert(__is_literal_type(std::optional<std::variant<int, const StringifyFieldInfoString>>));
#endif  // MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION

// Options that control all Stringify behavior.
//
// The objects are very large due to the very detailed ability to control behavior and override
// actual values.
//
// However, the objects support sparness, that is the data is grouped into severl aspects that can
// each be unset (nullopt), a const reference or actual materialized data. That enables fast
// creation and opying beyond the size. Nontheless, the data is large and will impact caches. So
// creating the options upfront and not specializing structs as much as neccessary is best.
//
// Construction example 1: Fully unset, which results in all defaults. Here we copy the whole object
// pretty much byte by byte.
//
// ```c++
// StringifyOptions opts;
// ```
//
// Construction example 2: Sparse with only ValueOverrides based on the default ValueOverrides by
// only initializing specific fields. We also leave all other groups unset so they refer to either
// core defaults or context dependent defaults.
//
// ```c++
// StringifyOptions opts{
//   .format{StringifyOptions::Format{
//     .message_prefix = "<",
//     .message_suffix = ">",
//   }},
// };
// ```
//
// Construction example 3: Sparse with only format overrides. Unlike before we first copy the format
// group's defaults from `AsJson()` then override specifics.
//
// ```c++
// StringifyOptions opts{[]() {
//   StringifyOptions opts = StringifyOptions::AsJson();
//   Format& format = opts.format.as_data();
//   format.message_prefix = "<",
//   format.message_suffix = ">\n";
//   return opts;
// }()};
// ```
struct StringifyOptions {
  enum class OutputMode {
    kDefault,
    kCpp,
    kJson,
    kJsonPretty,
  };

  friend std::ostream& operator<<(std::ostream& os, const OutputMode& value) {
    switch (value) {
      case OutputMode::kDefault: break;
      case OutputMode::kCpp: return os << "OutpuMode::kCpp";
      case OutputMode::kJson: return os << "OutpuMode::kJson";
      case OutputMode::kJsonPretty: return os << "OutpuMode::kJsonPretty";
    }
    return os << "OutpuMode::kDefault";
  }

  struct Format {
    // Message options:
    std::string_view message_prefix;
    std::string_view message_suffix;

    std::string_view field_indent;                // Indent for fields.
    std::string_view key_value_separator = ": ";  // Seperator between key and value.
    std::string_view field_separator = ", ";      // Separator between two field (in front of field).

    std::string_view pointer_prefix = "*{";   // Prefix for pointer types.
    std::string_view pointer_suffix = "}";    // Suffix for pointer types.
    std::string_view smart_ptr_prefix = "{";  // Prefix for smart pointer types.
    std::string_view smart_ptr_suffix = "}";  // Suffix for smart pointer types.
    std::string_view optional_prefix = "{";   // Prefix for optional types.
    std::string_view optional_suffix = "}";   // Suffix for optional types.

    std::string_view structure_prefix = "{";  // Prefix for object (struct/class) values.
    std::string_view structure_suffix = "}";  // Suffix for object (struct/class) values.
    std::string_view container_prefix = "{";  // Prefix for container values.
    std::string_view container_suffix = "}";  // Suffix for container values.

    std::string_view char_delim = "'";  // If empty, then use numeric value.
    std::string_view string_delim = "\"";
  };

  struct FieldControl {
    // Field options:
    bool suppress = false;           // Allows complete suppression of the field.
    bool suppress_nullptr = false;   // Allows complete suppression of nullptr field values.
    bool suppress_nullopt = false;   // Allows complete suppression of nullopt field values.
    bool suppress_disabled = false;  // Allows complete suppression of disabled fields.
    std::string_view field_disabled = "{/*MboTypesStringifyDisable*/}";  // Replacement for disabled complex field.
  };

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

  struct KeyControl {
    KeyMode key_mode{KeyMode::kNormal};
    std::string_view key_prefix = ".";  // Prefix to key names.
    std::string_view key_suffix;        // Suffix to key names.
  };

  struct KeyOverrides {
    // Force name for the key. This can be one of:
    // * A (literal/constexpr compatible) `std::string_view` which will be used as-is.
    // * A (literal/constexpr compatible) `const StringifyFieldInfoString*`.
    // * A (non literal) `std::optional<const StringifyFieldInfoString>`. This is wrapped in a `std::optional` because
    //   otherwise the whole struct cannot be a literal/constexpr.
#ifdef MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
    std::optional<std::variant<std::string_view, const StringifyFieldInfoString*, const StringifyFieldInfoString>>
#else   // MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
    std::optional<std::variant<std::string_view, const StringifyFieldInfoString*>>
#endif  // MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
        key_use_name;
  };

  // Value options:

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

  struct ValueControl {
    // If no suitable printing found then this option is controls the behavior:
    // - If `true` (the default), then the value is printed as is.
    // - Otherwise (`false`) all value format control is applied.
    bool other_types_direct = true;

    EscapeMode escape_mode{EscapeMode::kCEscape};

    std::string_view nullptr_t_str = "std::nullptr_t";  // Value for `nullptr_t` types.
    std::string_view nullptr_v_str = "<nullptr>";       // Value for `nullptr` values.
    std::string_view nullopt_str = "std::nullopt";      // Value for `nullopt` values.

    // Max num elements to show.
    std::size_t container_max_len = std::numeric_limits<std::size_t>::max();

    // Maximum length of value (prior to escaping).
    std::string_view::size_type str_max_length{std::string_view::npos};
    std::string_view str_cutoff_suffix = "...";  // Suffix if value gets shortened.
  };

  struct ValueOverrides {
    std::string_view replacement_str;    // Allows "redacted" for string values
    std::string_view replacement_other;  // Allows "redacted" for non string values
  };

  // Special types:

  struct Special {
    // Containers with Pairs whose `first` type is a string-like automatically
    // create objects where the keys are the field names. This is useful for JSON.
    // The types are checked with `IsPairFirstStr`.
    bool pair_first_is_name = false;

    // For all other cases of pairs, their names can be overwritten.
    std::optional<std::pair<std::string_view, std::string_view>> pair_keys{{"first", "second"}};
  };

  OptionalDataOrConstRef<Format> format;
  OptionalDataOrConstRef<FieldControl> field_control;
  OptionalDataOrConstRef<KeyControl> key_control;
  OptionalDataOrConstRef<KeyOverrides> key_overrides;
  OptionalDataOrConstRef<ValueControl> value_control;
  OptionalDataOrConstRef<ValueOverrides> value_overrides;
  OptionalDataOrConstRef<Special> special;

  template<IsSameAsAnyOf<Format, FieldControl, KeyControl, KeyOverrides, ValueControl, ValueOverrides, Special> T>
  constexpr const auto& Access() const {
    if constexpr (std::same_as<T, Format>) {
      return format;
    } else if constexpr (std::same_as<T, FieldControl>) {
      return field_control;
    } else if constexpr (std::same_as<T, KeyControl>) {
      return key_control;
    } else if constexpr (std::same_as<T, KeyOverrides>) {
      return key_overrides;
    } else if constexpr (std::same_as<T, ValueControl>) {
      return value_control;
    } else if constexpr (std::same_as<T, ValueOverrides>) {
      return value_overrides;
    } else if constexpr (std::same_as<T, Special>) {
      return special;
    }
  }

  template<IsSameAsAnyOf<Format, FieldControl, KeyControl, KeyOverrides, ValueControl, ValueOverrides, Special> T>
  static constexpr std::string_view TypeName() {
    if constexpr (std::same_as<T, Format>) {
      return "Format";
    } else if constexpr (std::same_as<T, FieldControl>) {
      return "FieldControl";
    } else if constexpr (std::same_as<T, KeyControl>) {
      return "KeyControl";
    } else if constexpr (std::same_as<T, KeyOverrides>) {
      return "KeyOverrides";
    } else if constexpr (std::same_as<T, ValueControl>) {
      return "ValueControl";
    } else if constexpr (std::same_as<T, ValueOverrides>) {
      return "ValueOverrides";
    } else if constexpr (std::same_as<T, Special>) {
      return "Special";
    }
  }

  std::string DebugStr() const;

  constexpr bool AllDataSet() const noexcept {
    return format && field_control && key_control && key_overrides && value_control && value_overrides && special;
  }

  constexpr explicit operator bool() const noexcept { return AllDataSet(); }

  template<typename Opts, typename Func, typename... Args>
  requires(std::same_as<std::remove_const_t<Opts>, StringifyOptions>)
  static constexpr bool ApplyAll(Opts& opts, const Func& func, const Args&... args) {
    return func(opts.format, args...) && func(opts.field_control, args...) && func(opts.key_control, args...)
           && func(opts.key_overrides, args...) && func(opts.value_control, args...)
           && func(opts.value_overrides, args...) && func(opts.special, args...);
  }

  static constexpr StringifyOptions WithAllData(StringifyOptions&& options, const StringifyOptions& defaults = {}) {
    ApplyAll(options, [&defaults]<typename T>(T& v) constexpr {
      v.as_data(defaults.Access<typename T::value_type>().get({}));
      return true;
    });
    MBO_CONFIG_REQUIRE_DEBUG(options, "Not all data set.");
    return std::move(options);
  }

  static constexpr StringifyOptions WithAllRefs(StringifyOptions&& options, const StringifyOptions& defaults = {}) {
    ApplyAll(options, [&defaults]<typename T>(T& v) constexpr {
      if (!v.has_value()) {
        v.set_ref(defaults.Access<typename T::value_type>().get({}));
      }
      return true;
    });
    MBO_CONFIG_REQUIRE_DEBUG(options, "Not all data set.");
    return std::move(options);
  }

  static constexpr StringifyOptions WithAllRefs(
      const StringifyOptions& options,
      const StringifyOptions& defaults = {}) {
    return WithAllRefs(StringifyOptions{options}, defaults);
  }

  // Arbirary default value.
  static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions& AsDefault() noexcept {
    static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions kDefaults = WithAllData({});
    MBO_CONFIG_REQUIRE(kDefaults.AllDataSet(), "Not all data set.");
    return kDefaults;
  }

  // Formatting control that disables a field.
  static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions& AsDisabled() noexcept {
    static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions kOptionsDisabled = StringifyOptions::WithAllData({
        .field_control{StringifyOptions::FieldControl{
            .suppress = true,
        }},
    });
    MBO_CONFIG_REQUIRE(kOptionsDisabled.AllDataSet(), "Not all data set.");
    return kOptionsDisabled;
  }

  // Formatting control that mostly produces C++ code.
  static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions& AsCpp() noexcept {
    // NOLINTNEXTLINE(readability-identifier-naming)
    static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions kOptionsCpp = StringifyOptions::WithAllData({
        .format{StringifyOptions::Format{
            .key_value_separator = " = ",
            .pointer_prefix = "",
            .pointer_suffix = "",
        }},
        .field_control{StringifyOptions::FieldControl{
            .field_disabled = "{/*MboTypesStringifyDisable*/}",
        }},
        .key_control{StringifyOptions::KeyControl{
            .key_prefix = ".",
        }},
        .value_control{StringifyOptions::ValueControl{
            .nullptr_t_str = "nullptr",
            .nullptr_v_str = "nullptr",
        }},
    });
    MBO_CONFIG_REQUIRE(kOptionsCpp.AllDataSet(), "Not all data set.");
    return kOptionsCpp;
  }

  // Formatting control that mostly produces JSON data.
  //
  // NOTE: JSON data requires field names. So unless Clang is used only with
  // types that support field names, they must be provided by an extension API
  // point: `MboTypesStringifyFieldNames` or `MboTypesStringifyOptions`, the
  // latter possibly with the `StringifyWithFieldNames` adapter. Alternatively,
  // numeric field names will be generated as a last resort.
  static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions& AsJson() noexcept {
    // NOLINTNEXTLINE(readability-identifier-naming)
    static MBO_CONFIG_CONSTEXPR_23 StringifyOptions kOptionsJson = StringifyOptions::WithAllData({
        .format{StringifyOptions::Format{
            .key_value_separator = ": ",
            .pointer_prefix = "",
            .pointer_suffix = "",
            .smart_ptr_prefix = "",
            .smart_ptr_suffix = "",
            .optional_prefix = "",
            .optional_suffix = "",
            .container_prefix = "[",
            .container_suffix = "]",
            .char_delim = "\"",
        }},
        .field_control{StringifyOptions::FieldControl{
            .suppress_nullptr = true,
            .suppress_nullopt = true,
            .suppress_disabled = true,
        }},
        .key_control{StringifyOptions::KeyControl{
            .key_mode = StringifyOptions::KeyMode::kNumericFallback,
            .key_prefix = "\"",
            .key_suffix = "\"",
        }},
        .value_control{StringifyOptions::ValueControl{
            .nullptr_t_str = "0",
            .nullptr_v_str = "0",
            .nullopt_str = "0",
        }},
        .special{StringifyOptions::Special{
            .pair_first_is_name = true,
        }},
    });
    MBO_CONFIG_REQUIRE(kOptionsJson.AllDataSet(), "Not all data set.");
    return kOptionsJson;
  }

  static MBO_CONFIG_CONSTEXPR_23 const StringifyOptions& AsJsonPretty() noexcept {
    // NOLINTNEXTLINE(readability-identifier-naming)
    static MBO_CONFIG_CONSTEXPR_23 StringifyOptions kOptions = []() MBO_CONFIG_CONSTEXPR_23 {
      StringifyOptions opts = AsJson();
      Format& format = opts.format.as_data();
      format.message_suffix = "\n";
      format.field_indent = "  ";
      format.field_separator = ",";
      return opts;
    }();
    MBO_CONFIG_REQUIRE(kOptions.AllDataSet(), "Not all data set.");
    return kOptions;
  }

  static constexpr const StringifyOptions& As(OutputMode mode) noexcept {
    switch (mode) {
      case OutputMode::kDefault: break;
      case OutputMode::kCpp: return AsCpp();
      case OutputMode::kJson: return AsJson();
      case OutputMode::kJsonPretty: return AsJsonPretty();
    }
    return AsDefault();
  }

  static_assert(__is_literal_type(Format));
  static_assert(__is_literal_type(FieldControl));
  static_assert(__is_literal_type(KeyControl));
  static_assert(__is_literal_type(KeyOverrides));
  static_assert(__is_literal_type(ValueControl));
  static_assert(__is_literal_type(ValueOverrides));
  static_assert(__is_literal_type(Special));
};

#if defined(__clang__) || defined(__GNUC__)
static_assert(sizeof(StringifyOptions) <= 700);  // NOLINT(*-magic-numbers)
static_assert(__is_literal_type(StringifyOptions));
#endif  // defined(__clang__) || defined(__GNUC__)

struct StringifyFieldOptions {
  constexpr ~StringifyFieldOptions() noexcept = default;

  template<typename U>
  StringifyFieldOptions(U&&) = delete;  // NOLINT(*-missing-std-forward,*-forwarding-reference-overload)
  template<typename U, typename V>
  StringifyFieldOptions(U&&, V&&) = delete;  // NOLINT(*-missing-std-forward,*-forwarding-reference-overload)

  // NOLINTNEXTLINE(*-explicit-*): Implicit conversion in this direction is pretty intended.
  constexpr StringifyFieldOptions(const StringifyOptions& both) : outer(both), inner(both) {}

  // NOLINTNEXTLINE(*-swappable-parameters)
  constexpr StringifyFieldOptions(const StringifyOptions& outer, const StringifyOptions& inner) noexcept
      : outer(outer), inner(inner) {}

  constexpr StringifyFieldOptions(const StringifyFieldOptions& other) noexcept = default;
  constexpr StringifyFieldOptions& operator=(const StringifyFieldOptions& other) = delete;
  constexpr StringifyFieldOptions(StringifyFieldOptions&& other) noexcept = default;
  constexpr StringifyFieldOptions& operator=(StringifyFieldOptions&& other) = delete;

  std::string DebugStr() const;

  constexpr bool AllDataSet() const noexcept { return outer.AllDataSet() && inner.AllDataSet(); }

  constexpr explicit operator bool() const noexcept { return AllDataSet(); }

  constexpr StringifyFieldOptions ToInner() const noexcept { return StringifyFieldOptions{inner, inner}; }

  template<typename T = StringifyOptions>
  constexpr const auto& Outer() const noexcept {
    if constexpr (std::same_as<T, StringifyOptions>) {
      return outer;
    } else {
      return outer.Access<T>().value();
    }
  }

  const StringifyOptions& outer;
  const StringifyOptions& inner;
};

static_assert(sizeof(StringifyFieldOptions) == 2 * sizeof(void*));

// Control for overall use a Stringify. This controls root options for messages.
// Note that `root_prefix` and `root_suffix` are streamed without `root_indent`.
// This means in particular, that is the first line of the output should already be indented, then
// `root_prefix` should end in `root_indent`.
struct StringifyRootOptions {
  std::string_view root_prefix;  // First line prefix - not using root_indent - before message_prefix.
  std::string_view root_suffix;  // Last line suffix - not using root_indent - after message_suffix.
  std::string_view root_indent;

  static MBO_CONFIG_CONSTEXPR_23 const StringifyRootOptions& Defaults() {
    static MBO_CONFIG_CONSTEXPR_23 const StringifyRootOptions kDefaults{};
    return kDefaults;
  }
};

struct StringifyFieldInfo {
  const StringifyFieldOptions options;  // BY VALUE
  std::size_t idx;
  std::string_view name;
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

// Identify presence for a `Stringify` extender API extension point `MboTypesStringifyOptions`.
//
// That extension point can be used to perform fine grained control of field printing/streaming.
//
// NOTE: Unlike `MboTypesStringifyFieldNames` this extension point cannot deal with
// `std::string_view` members referencing temp objects such as `std::string` since life-time
// extension CANNOT be applied as we always go via the actual `std::strig_view` fields.
//
// The return value can be one of:
// * `FieldOptions`
// * `const FieldOptions&`
// * `StringifyOptions`
// * `const StringifyOptions&`
//
// Internally we always use `FieldOptions` by reference. If the function returns by value, then the
// object will be cached locally. If `StringifyOptions` are provided then they will be extended to
// `FieldOptions` as both `outer` and `inner`.
//
// The `inner` component is used for complex fields. This allows to format say a container itself
// different from its values.
//
// The function receives the following parameters:
// * `const T&`: The object itself - so this is an ADL solution.
// * `const StringifyFieldInfo&`: Information about options to use and the field they apply to:
//    * The `const FieldOptions& options`: the current (possibly root) options.
//    * The `std::size_t`:                 the index of the field.
//    * The `std::string_view`:            the name of the field.
//
// The implementation must therefore match one of the following signatures:
// ```
// friend const FieldOptions& MboTypesStringifyOptions(const T&, const const StringifyFieldInfo&)
// friend FieldOptions MboTypesStringifyOptions(const T&, const StringifyFieldInfo&)
// friend const StringifyOptions& MboTypesStringifyOptions(const T&, const const StringifyFieldInfo&)
// friend StringifyOptions MboTypesStringifyOptions(const T&, const StringifyFieldInfo&)
// ```
template<typename T>
concept HasMboTypesStringifyOptions = requires(const T& v, const StringifyFieldInfo& opts) {
  { MboTypesStringifyOptions(v, opts) } -> IsSameAsAnyOfRaw<StringifyOptions, StringifyFieldOptions>;
};

namespace types_internal {
// NOLINTBEGIN(*-identifier-length)
template<typename T>
concept BadMboTypesStringifyOptions_1 = requires(const T& a1) {
  { MboTypesStringifyOptions(a1) };
};

template<typename T>
concept BadMboTypesStringifyOptions_2 = requires(const T& a1, const AnyOtherType<StringifyFieldInfo>& a2) {
  { MboTypesStringifyOptions(a1, a2) };
};

template<typename T>
concept BadMboTypesStringifyOptions_3 = requires(const T& a1, const AnyType& a2, const AnyType& a3) {
  { MboTypesStringifyOptions(a1, a2, a3) };
};

template<typename T>
concept BadMboTypesStringifyOptions_4 = requires(const T& a1, const AnyType& a2, const AnyType& a3, const AnyType& a4) {
  { MboTypesStringifyOptions(a1, a2, a3, a4) };
};

template<typename T>
concept BadMboTypesStringifyOptions_5 =
    requires(const T& a1, const AnyType& a2, const AnyType& a3, const AnyType& a4, const AnyType& a5) {
      { MboTypesStringifyOptions(a1, a2, a3, a4, a5) };
    };

template<typename T>
concept BadMboTypesStringifyOptions_6 = requires(
    const T& a1,
    const AnyType& a2,
    const AnyType& a3,
    const AnyType& a4,
    const AnyType& a5,
    const AnyType& a6) {
  { MboTypesStringifyOptions(a1, a2, a3, a4, a5, a6) };
};
// NOLINTEND(*-identifier-length)
}  // namespace types_internal

// Concept that identifies bad `MboTypesStringifyOptions` signatures.
template<typename T>
concept BadMboTypesStringifyOptions = (  //
    types_internal::BadMboTypesStringifyOptions_1<T> || types_internal::BadMboTypesStringifyOptions_2<T>
    || types_internal::BadMboTypesStringifyOptions_3<T> || types_internal::BadMboTypesStringifyOptions_4<T>
    || types_internal::BadMboTypesStringifyOptions_5<T>);

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
  // Prevent passing in temporaries to prevent dangling issues.
  static Stringify AsCpp(const StringifyRootOptions&&) = delete;
  static Stringify AsJson(const StringifyRootOptions&&) = delete;
  static Stringify AsJsonPretty(const StringifyRootOptions&&) = delete;

  static Stringify AsCpp(const StringifyRootOptions& root_options = StringifyRootOptions::Defaults()) noexcept {
    return Stringify(StringifyOptions::AsCpp(), root_options);
  }

  static Stringify AsJson(const StringifyRootOptions& root_options = StringifyRootOptions::Defaults()) noexcept {
    return Stringify(StringifyOptions::AsJson(), root_options);
  }

  static Stringify AsJsonPretty(const StringifyRootOptions& root_options = StringifyRootOptions::Defaults()) noexcept {
    return Stringify(StringifyOptions::AsJsonPretty(), root_options);
  }

  explicit Stringify(const StringifyOptions&&) = delete;
  template<typename SO>
  explicit Stringify(SO&&, StringifyRootOptions&&) = delete;  // NOLINT(*)

  explicit Stringify(
      const StringifyOptions& default_options = StringifyOptions::AsDefault(),
      const StringifyRootOptions& root_options = StringifyRootOptions::Defaults()) noexcept
      : root_options_(root_options), default_field_options_(default_options), indent_(*this) {
    MBO_CONFIG_REQUIRE(default_field_options_.AllDataSet(), "Not all data set: ") << default_field_options_.DebugStr();
  }

  explicit Stringify(
      StringifyOptions::OutputMode output_mode,
      const StringifyRootOptions& root_options = StringifyRootOptions::Defaults()) noexcept
      : Stringify(StringifyOptions::As(output_mode), root_options) {}

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
    os << root_options_.root_prefix;
    os << default_field_options_.outer.format.get({}).message_prefix;
    StreamImpl(os, default_field_options_, value);
    os << default_field_options_.outer.format.get({}).message_suffix;
    os << root_options_.root_suffix;
  }

 private:
  using SO = StringifyOptions;
  using SFO = StringifyFieldOptions;

  class Indent {
   public:
    Indent() = delete;

    explicit Indent(const Stringify& stringify)
        : enable_(!stringify.default_field_options_.outer.format.get({}).field_indent.empty()) {
      if (!stringify.root_options_.root_indent.empty()) {
        level_.push_back(stringify.root_options_.root_indent);
      }
    }

    void IncContainer(std::ostream& os, const StringifyOptions::Format& format) {
      os << format.container_prefix;
      level_.push_back(format.field_indent);
      enable_ = !format.field_indent.empty();
    }

    void DecContainer(std::ostream& os, const StringifyOptions::Format& format) {
      level_.pop_back();
      StreamIndent(os);
      os << format.container_suffix;
      enable_ = level_.empty() || !level_.back().empty();
    }

    void IncStruct(std::ostream& os, const StringifyOptions::Format& format) {
      os << format.structure_prefix;
      level_.push_back(format.field_indent);
      enable_ = !format.field_indent.empty();
    }

    void DecStruct(std::ostream& os, const StringifyOptions::Format& format) {
      level_.pop_back();
      StreamIndent(os);
      os << format.structure_suffix;
      enable_ = level_.empty() || !level_.back().empty();
    }

    void StreamIndent(std::ostream& os) {
      if (!enable_) {
        return;
      }
      os << "\n";
      for (std::string_view level : level_) {
        os << level;
      }
    }

   private:
    bool enable_ = true;
    std::vector<std::string_view> level_;
  };

  template<typename T>
  requires(std::is_aggregate_v<T>)
  void StreamImpl(std::ostream& os, const StringifyFieldOptions& options, const T& value) const {
    // It is not allowed to deny field name printing but provide filed names.
    static_assert(!(HasMboTypesStringifyDoNotPrintFieldNames<T> && HasMboTypesStringifyFieldNames<T>));
    StreamFieldsImpl<T>(os, options, value);
  }

  template<typename T>
  requires(HasMboTypesStringifyDisable<T>)
  void StreamFieldsImpl(
      std::ostream& os,
      const StringifyFieldOptions& options,
      const T& /*value*/,
      bool /*allow_field_names*/ = !HasMboTypesStringifyDoNotPrintFieldNames<T>) const {
    const SO::FieldControl& field_control = *options.outer.field_control;
    if (!field_control.suppress_disabled) {
      os << field_control.field_disabled;
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
      const StringifyFieldOptions& options,
      const T& value,
      bool use_field_names = !HasMboTypesStringifyDoNotPrintFieldNames<T>) const {
    use_field_names |= options.outer.key_control->key_mode == StringifyOptions::KeyMode::kNumericFallback;
    bool use_sep = false;
    const auto& field_names = GetFieldNames(value);
    std::apply(
        [&](const auto&... fields) {
          const SO::Format& format = *options.outer.format;
          indent_.IncStruct(os, format);
          std::size_t idx{0};
          ((StreamField(os, options, use_sep, value, TsValue<T>(idx, value, fields), idx, field_names, use_field_names),
            ++idx),
           ...);
          indent_.DecStruct(os, format);
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

  template<typename Field>
  static SpecialFieldValue IsSpecial(const Field& field) {
    using RawField = std::remove_cvref_t<Field>;
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
  }

  template<typename T, typename Field, typename FieldNames>
  void StreamField(
      std::ostream& os,
      const StringifyFieldOptions& outer_options,
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
    static_assert(!BadMboTypesStringifyOptions<T>, "Provided `BadMboTypesStringifyOptions` has a bad signature.");
    // Custom can be very expensive on the stack, so only include that call-path if necessary.
    if constexpr (HasMboTypesStringifyOptions<T>) {
      StreamFieldCustom(
          os, outer_options, use_seperator, field, idx, field_name, allow_field_names,
          MboTypesStringifyOptions(value, {.options = outer_options, .idx = idx, .name = field_name}));
    } else {
      // No customization
      const StringifyFieldInfo field_info{.options{default_field_options_}, .idx = idx, .name = field_name};
      StreamFieldDo(os, outer_options, field_info, use_seperator, field, allow_field_names);
    }
  }

  template<typename Field, typename CustomT>
  void StreamFieldCustom(
      std::ostream& os,
      const StringifyFieldOptions& outer_options,
      bool& use_seperator,
      const Field& field,
      std::size_t idx,
      std::string_view field_name,
      bool allow_field_names,
      CustomT&& custom) const {
    // The custom handling is expensive only if we have to cache the data. So hand that over to specific functions.
    if constexpr (std::same_as<std::remove_cvref_t<CustomT>, StringifyFieldOptions>) {
      if constexpr (std::is_reference_v<CustomT>) {
        if (custom.AllDataSet()) {
          // No caching needed for `custom` since we use the reference as is.
          const StringifyFieldInfo field_info{.options{std::forward<CustomT>(custom)}, .idx = idx, .name = field_name};
          StreamFieldDo(os, outer_options, field_info, use_seperator, field, allow_field_names);
          return;
        }
      }
      if (&custom.outer == &custom.inner) {
        StreamFieldCustomCache(  // Using StringifyOptions
            os, outer_options, custom.outer, use_seperator, field, idx, field_name, allow_field_names);
      } else {
        StreamFieldCustomCache(  // Using StringifyFieldOptions
            os, outer_options, std::forward<CustomT>(custom), use_seperator, field, idx, field_name, allow_field_names);
      }
    } else {
      if constexpr (std::is_reference_v<CustomT>) {
        if (custom.AllDataSet()) {
          // No caching needed for `custom` since we use the reference as is.
          const StringifyFieldInfo field_info{.options{custom}, .idx = idx, .name = field_name};
          StreamFieldDo(os, outer_options, field_info, use_seperator, field, allow_field_names);
          return;
        }
      }
      StreamFieldCustomCache(
          os, outer_options, std::forward<CustomT>(custom), use_seperator, field, idx, field_name, allow_field_names);
    }
  }

  template<typename Field>
  void StreamFieldCustomCache(
      std::ostream& os,
      const StringifyFieldOptions& outer_options,
      const StringifyFieldOptions& custom,
      bool& use_seperator,
      const Field& field,
      std::size_t idx,
      std::string_view field_name,
      bool allow_field_names) const {
    // VERY EXPENSIVE: close to 2k on the stack
    const StringifyOptions inner_outer = StringifyOptions::WithAllRefs(custom.outer, outer_options.inner);
    const StringifyOptions inner_inner = StringifyOptions::WithAllRefs(custom.inner, outer_options.inner);
    const StringifyFieldInfo field_info{.options{inner_outer, inner_inner}, .idx = idx, .name = field_name};
    StreamFieldDo(os, outer_options, field_info, use_seperator, field, allow_field_names);
  }

  template<typename Field, std::same_as<StringifyOptions> CustomT>
  void StreamFieldCustomCache(
      std::ostream& os,
      const StringifyFieldOptions& outer_options,
      CustomT&& custom,
      bool& use_seperator,
      const Field& field,
      std::size_t idx,
      std::string_view field_name,
      bool allow_field_names) const {
    // VERY EXPENSIVE: close to 1k on the stack
    const StringifyOptions cached{SO::WithAllRefs(std::forward<CustomT>(custom), outer_options.inner)};
    const StringifyFieldInfo field_info{.options{cached}, .idx = idx, .name = field_name};
    StreamFieldDo(os, outer_options, field_info, use_seperator, field, allow_field_names);
  }

  template<typename Field>
  void StreamFieldDo(
      std::ostream& os,
      const StringifyFieldOptions& outer_options,
      const StringifyFieldInfo& field_info,
      bool& use_seperator,
      const Field& field,
      bool allow_field_names) const {
    const SpecialFieldValue is_special = IsSpecial(field);
    if (!StreamFieldKeyEnabled(field_info.options.outer, is_special)) {
      return;
    }
    if (use_seperator) {
      os << outer_options.outer.format->field_separator;
    }
    use_seperator = true;
    indent_.StreamIndent(os);
    if (allow_field_names) {
      StreamFieldName(os, field_info);
    }
    StreamValue(os, field_info.options, field, allow_field_names);
  }

  static bool StreamFieldKeyEnabled(const StringifyOptions& options, SpecialFieldValue is_special) {
    switch (is_special) {
      case SpecialFieldValue::kNoSuppress: break;
      case SpecialFieldValue::kNormal:
        if (options.field_control->suppress) {
          return false;
        }
        break;
      case SpecialFieldValue::kIsNullptr:
        if (options.field_control->suppress_nullptr) {
          return false;
        }
        break;
      case SpecialFieldValue::kIsNullopt:
        if (options.field_control->suppress_nullopt) {
          return false;
        }
        break;
      case SpecialFieldValue::kStringifyDisabled:
        if (options.field_control->suppress_disabled) {
          return false;
        }
        break;
    }
    return true;
  }

  static void StreamFieldName(std::ostream& os, const StringifyFieldInfo& field, bool allow_key_override = true) {
    const SO::Format& format = *field.options.outer.format;
    const SO::KeyControl& key_control = *field.options.outer.key_control;
    switch (key_control.key_mode) {
      case StringifyOptions::KeyMode::kNone: return;
      case StringifyOptions::KeyMode::kNormal:
      case StringifyOptions::KeyMode::kNumericFallback: break;
    }
    std::string_view field_name;
    if (allow_key_override && field.options.outer.key_overrides->key_use_name.has_value()) {
      const auto& key_use_name = *field.options.outer.key_overrides->key_use_name;
      if (std::holds_alternative<std::string_view>(key_use_name)) {
        field_name = std::get<std::string_view>(key_use_name);
      } else if (std::holds_alternative<const StringifyFieldInfoString*>(key_use_name)) {
        const auto* func = std::get<const StringifyFieldInfoString*>(key_use_name);
        if (func != nullptr && *func) {
          field_name = (*func)(field);
        }
#ifdef MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
      } else if (std::holds_alternative<const StringifyFieldInfoString>(key_use_name)) {
        const auto& func = std::get<const StringifyFieldInfoString>(key_use_name);
        if (func) {
          field_name = func(field);
        }
#endif  // MBO_TYPES_STRINGIFY_LITERAL_OPTIONAL_FUNCTION
      } else {
        ABSL_LOG(FATAL) << "Bad field name override: variant type not handled.";
      }
    }
    if (field_name.empty()) {
      field_name = field.name;
    }
    if (!field_name.empty()) {
      os << key_control.key_prefix << field_name << key_control.key_suffix << format.key_value_separator;
    } else if (key_control.key_mode == StringifyOptions::KeyMode::kNumericFallback) {
      os << key_control.key_prefix << field.idx << key_control.key_suffix << format.key_value_separator;
    }
  }

  template<typename C>
  requires(::mbo::types::ContainerIsForwardIteratable<C> && !std::convertible_to<C, std::string_view>)
  void StreamValue(std::ostream& os, const StringifyFieldOptions& options, const C& vs, bool allow_field_names) const {
    const SO::Format& format = *options.outer.format;
    if constexpr (mbo::types::IsPairFirstStr<std::remove_cvref_t<typename C::value_type>>) {
      if (options.outer.special->pair_first_is_name) {
        // Each pair element of the container `vs` is an element whose key is the `first` member and
        // whose value is the `second` member.
        indent_.IncStruct(os, format);
        std::string_view sep;
        std::size_t index = 0;
        for (const auto& v : vs) {
          if (index >= options.outer.value_control->container_max_len) {
            break;
          }
          os << sep;
          indent_.StreamIndent(os);
          sep = options.outer.format->field_separator;
          if (allow_field_names) {
            StreamFieldName(
                os, StringifyFieldInfo{.options = options, .idx = index, .name = v.first},
                /*allow_key_override=*/false);
          }
          StreamValue(os, options.ToInner(), v.second, allow_field_names);
          ++index;
        }
        indent_.DecStruct(os, format);
        return;
      }
    }
    indent_.IncContainer(os, format);
    std::string_view sep;
    std::size_t index = 0;
    for (const auto& v : vs) {
      if (++index > options.outer.value_control->container_max_len) {
        break;
      }
      os << sep;
      indent_.StreamIndent(os);
      sep = options.outer.format->field_separator;
      StreamValue(os, options.ToInner(), v, allow_field_names);
    }
    indent_.DecContainer(os, format);
  }

  template<typename T>
  static constexpr bool kUseStringify = types_internal::IsExtended<T> || HasMboTypesStringifySupport<T>
                                        || (std::is_aggregate_v<T> && !absl::HasAbslStringify<T>::value);

  template<typename V>  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void StreamValue(std::ostream& os, const StringifyFieldOptions& options, const V& v, bool allow_field_names) const {
    using RawV = std::remove_cvref_t<V>;
    // IMPORTANT: ALL if-clauses must be `if constexpr`.
    if constexpr (HasMboTypesStringifyDisable<RawV>) {
      os << options.outer.field_control->field_disabled;
    } else if constexpr (kUseStringify<RawV>) {
      StreamImpl(os, options, v);
    } else if constexpr (std::is_same_v<RawV, std::nullptr_t>) {
      os << options.outer.value_control->nullptr_t_str;
    } else if constexpr (IsSmartPtr<RawV>) {
      StreamValueSmartPtr(os, options, v, allow_field_names);
    } else if constexpr (std::is_pointer_v<RawV>) {
      StreamValuePointer(os, options, v, allow_field_names);
    } else if constexpr (mbo::types::IsOptional<RawV>) {
      StreamValueOptional(os, options, v, allow_field_names);
    } else if constexpr (mbo::types::IsPair<RawV>) {
      StreamValuePair(os, options, v, allow_field_names);
    } else if constexpr (std::is_same_v<RawV, char> || std::is_same_v<RawV, unsigned char>) {
      if (options.outer.format->char_delim.empty()) {
        os << absl::StreamFormat("%v", int(v));
      } else {
        os << options.outer.format->char_delim;
        if (v == '\'') {
          StreamValueStr(os, options.outer, "\\'");
        } else {
          std::string_view vv{reinterpret_cast<const char*>(&v), 1};  // NOLINT(*-type-reinterpret-cast)
          StreamValueStr(os, options.outer, vv);
        }
        os << options.outer.format->char_delim;
      }
    } else if constexpr (std::is_arithmetic_v<RawV>) {
      if (options.outer.value_overrides->replacement_other.empty()) {
        os << absl::StreamFormat("%v", v);
      } else {
        os << options.outer.value_overrides->replacement_other;
      }
    } else if constexpr (
        std::same_as<RawV, std::string_view> || std::same_as<V, std::string>
        || (std::is_convertible_v<V, std::string_view> && !absl::HasAbslStringify<V>::value)) {
      // Do not attempt to invoke string conversion for AbslStringify supported types as that breaks
      // this very implementation.
      os << options.outer.format->string_delim;
      StreamValueStr(os, options.outer, v);
      os << options.outer.format->string_delim;
    } else {  // NOTE WHEN EXTENDING: Must always use `else if constexpr`
      StreamValueFallback(os, options.outer, v);
    }
  }

  template<typename V>
  void StreamValuePair(std::ostream& os, const StringifyFieldOptions& options, const V& v, bool allow_field_names)
      const {
    using RawV = std::remove_cvref_t<V>;
    if constexpr (!HasMboTypesStringifyFieldNames<RawV> && !HasMboTypesStringifyOptions<RawV>) {
      if (options.outer.special->pair_keys.has_value()) {
        const std::array<std::string_view, 2> field_names{
            options.outer.special->pair_keys->first,
            options.outer.special->pair_keys->second,
        };
        bool use_seperator = false;
        indent_.IncStruct(os, *options.outer.format);
        StreamField(os, options.ToInner(), use_seperator, v, v.first, 0, field_names, allow_field_names);
        StreamField(os, options.ToInner(), use_seperator, v, v.second, 1, field_names, allow_field_names);
        indent_.DecStruct(os, *options.outer.format);
        return;
      }
    }
    StreamFieldsImpl(os, options, v, allow_field_names);
  }

  template<typename V>
  void StreamValueSmartPtr(std::ostream& os, const StringifyFieldOptions& options, const V& v, bool allow_field_names)
      const {
    if (v) {
      os << options.outer.format->smart_ptr_prefix;
      StreamValue(os, options.ToInner(), *v, allow_field_names);
      os << options.outer.format->smart_ptr_suffix;
    } else {
      os << options.outer.value_control->nullptr_v_str;
    }
  }

  template<typename V>
  void StreamValuePointer(std::ostream& os, const StringifyFieldOptions& options, const V& v, bool allow_field_names)
      const {
    if (v) {
      os << options.outer.format->pointer_prefix;
      StreamValue(os, options.ToInner(), *v, allow_field_names);
      os << options.outer.format->pointer_suffix;
    } else {
      os << options.outer.value_control->nullptr_v_str;
    }
  }

  template<typename V>
  void StreamValueOptional(std::ostream& os, const StringifyFieldOptions& options, const V& v, bool allow_field_names)
      const {
    if (v.has_value()) {
      os << options.outer.format->optional_prefix;
      StreamValue(os, options.ToInner(), *v, allow_field_names);
      os << options.outer.format->optional_suffix;
    } else {
      os << options.outer.value_control->nullopt_str;
    }
  }

  template<typename V>
  static void StreamValueFallback(std::ostream& os, const StringifyOptions& options, const V& v) {
    if (options.value_control->other_types_direct) {
      if (options.value_overrides->replacement_other.empty()) {
        os << absl::StreamFormat("%v", v);
      } else {
        os << options.value_overrides->replacement_other;
      }
    } else {
      const std::string vv = absl::StrFormat("%v", v);
      StreamValueStr(os, options, vv);
    }
  }

  static void StreamValueStr(std::ostream& os, const StringifyOptions& options, std::string_view v) {
    if (!options.value_overrides->replacement_str.empty()) {
      os << options.value_overrides->replacement_str;
      return;
    }
    std::string_view vvv{v};
    if (options.value_control->str_max_length != std::string_view::npos) {
      vvv = vvv.substr(0, options.value_control->str_max_length);
    }
    switch (options.value_control->escape_mode) {
      case StringifyOptions::EscapeMode::kNone: os << vvv; break;
      case StringifyOptions::EscapeMode::kCEscape: os << absl::CEscape(vvv); break;
      case StringifyOptions::EscapeMode::kCHexEscape: os << absl::CHexEscape(vvv); break;
    }
    if (vvv.length() < v.length()) {
      os << options.value_control->str_cutoff_suffix;
    }
  }

  const StringifyRootOptions& root_options_;
  const StringifyFieldOptions default_field_options_;
  mutable Indent indent_;
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
//       const MyType& v, const mbo::types::StringifyFieldInfo& field) {
//     return StringifyWithFieldNames({"one", "two"})(v, field);
//   }
// };
// ```
template<IsFieldNameContainer FieldNames = std::initializer_list<std::string_view>>
inline constexpr auto StringifyWithFieldNames(
    const FieldNames& field_names,
    StringifyNameHandling name_handling = StringifyNameHandling::kVerify) {
  // In order to provide overrides, the actual target `options` have to be created.
  // Once those are available, the overrides can be applied.
  return [&field_names, name_handling]<typename T>(const T&, const StringifyFieldInfo& field) -> StringifyOptions {
    if (field.idx >= std::size(field_names)) {
      return field.options.outer;
    }
    StringifyOptions options = field.options.outer;
    std::string_view field_name = std::data(field_names)[field.idx];  // NOLINT(*-pointer-arithmetic)
    options.key_overrides.as_data().key_use_name = field_name;
    if (types_internal::SupportsFieldNames<T> && name_handling == StringifyNameHandling::kVerify) {
      ABSL_CHECK_EQ(field.name, field_name) << "Bad field_name injection for field #" << field.idx;
    }
    return options;
  };
}

}  // namespace mbo::types

#endif  // MBO_TYPES_STRINGIFY_H_
