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

#ifndef MBO_TYPES_STRING_OR_VIEW_H_
#define MBO_TYPES_STRING_OR_VIEW_H_

#include <compare>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "absl/strings/str_format.h"

namespace mbo {

// Read-only text that either owns a string or borrows existing storage.
//
// A borrowed string_view or character array must outlive this object and every
// copy of it. Runtime C strings deliberately have no unambiguous constructor:
// callers must select std::string for ownership or std::string_view to borrow.
class StringOrView {
 public:
  ~StringOrView() = default;

  constexpr StringOrView() noexcept = default;

  StringOrView(std::string value)  // NOLINT(google-explicit-constructor)
      : value_(std::in_place_type<std::string>, std::move(value)) {}

  constexpr StringOrView(std::string_view value) noexcept  // NOLINT(*-explicit-*)
      : value_(std::in_place_type<std::string_view>, value) {}

  template<std::size_t N>
  // A literal-array reference is the lifetime-safe alternative to an
  // ambiguous runtime `const char*` conversion.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  constexpr StringOrView(const char (&literal)[N]) noexcept  // NOLINT(*-explicit-*)
      : value_(std::in_place_type<std::string_view>, literal, N - 1) {}

  StringOrView(const StringOrView&) = default;
  StringOrView(StringOrView&&) noexcept = default;
  StringOrView& operator=(const StringOrView&) = default;
  StringOrView& operator=(StringOrView&&) noexcept = default;

  [[nodiscard]] constexpr std::string_view view() const noexcept {  // NOLINT(*-identifier-naming)
    if (const auto* const borrowed = std::get_if<std::string_view>(&value_); borrowed != nullptr) {
      return *borrowed;
    }
    return std::get<std::string>(value_);
  }

  [[nodiscard]] constexpr bool owns_string() const noexcept {  // NOLINT(*-identifier-naming)
    return std::holds_alternative<std::string>(value_);
  }

  constexpr bool operator==(const StringOrView& other) const noexcept { return view() == other.view(); }

  constexpr auto operator<=>(const StringOrView& other) const noexcept { return view() <=> other.view(); }

  template<typename Other>
  requires(
      !std::same_as<std::remove_cvref_t<Other>, StringOrView>
      && requires(const Other& other) { std::string_view{other}; })
  constexpr bool operator==(const Other& other) const noexcept {
    return view() == std::string_view{other};
  }

  template<typename Other>
  requires(
      !std::same_as<std::remove_cvref_t<Other>, StringOrView>
      && requires(const Other& other) { std::string_view{other}; })
  constexpr auto operator<=>(const Other& other) const noexcept {
    return view() <=> std::string_view{other};
  }

  template<typename Sink>
  friend void AbslStringify(Sink& sink, const StringOrView& value) {
    absl::Format(&sink, "%s", value.view());
  }

 private:
  std::variant<std::string_view, std::string> value_{std::string_view{}};
};

}  // namespace mbo

#endif  // MBO_TYPES_STRING_OR_VIEW_H_
