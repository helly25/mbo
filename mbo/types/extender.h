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
// * AbslStringify (default)
//
//   * Provides: friend void AbslStringify(Sink&, const Type& t)
//   * Provides: private void OStreamFields(std::ostream& os) const
//   * Enables use of the struct with `absl::Format` library.
//   * Required by: Printable, Streamable
//
// * Comparable
//
//   * Implements all comparison operators by means of `<=>` referring to the
//     `<=>` operator for the temporary `std::tuple` copies.
//
// * Hashable
//
//   * Make the extended type hashable.
//
#ifndef MBO_TYPES_EXTENDER_H_
#define MBO_TYPES_EXTENDER_H_

// IWYU pragma private, include "mbo/types/extend.h"

#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include "absl/hash/hash.h"  // IWYU pragma: keep
#include "absl/strings/str_format.h"
#include "mbo/types/internal/extender.h"      // IWYU pragma: export
#include "mbo/types/internal/struct_names.h"  // IWYU pragma: keep
#include "mbo/types/traits.h"                 // IWYU pragma: keep
#include "mbo/types/tstring.h"

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

template<typename ExtenderBase>
struct AbslStringify_ : ExtenderBase {  // NOLINT(readability-identifier-naming)
  using Type = typename ExtenderBase::Type;

  void OStreamFields(std::ostream& os) const { OStreamFieldsImpl(os, this->ToTuple()); }

  template<typename Sink>
  friend void AbslStringify(Sink& sink, const Type& value) {
    std::ostringstream os;
    value.OStreamFields(os);
    absl::Format(&sink, "%s", os.str());
  }

 private:
  template<typename... Ts>
  void OStreamFieldsImpl(std::ostream& os, const std::tuple<Ts...>& v) const {
    std::apply(
        [&os](const Ts&... fields) {
          os << '{';
          std::size_t idx{0};  // NOLINT(misc-const-correctness)
          (OStreamField(os, idx++, fields), ...);
          os << '}';
        },
        v);
  }

  template<typename C>
  requires(::mbo::types::ContainerIsForwardIteratable<C> && !std::convertible_to<C, std::string_view>)
  static void OStreamValue(std::ostream& os, const C& vs) {
    os << "{";
    std::string_view sep;
    for (const auto& v : vs) {
      os << sep;
      sep = ", ";
      OStreamValue(os, v);
    }
    os << "}";
  }

  template<typename V>
  static void OStreamValue(std::ostream& os, const V& v) {
    if constexpr (std::is_same_v<V, std::nullptr_t>) {
      os << absl::StreamFormat("nullptr_t");
    } else if constexpr (std::is_pointer_v<V>) {
      if (v) {
        os << "*{";
        OStreamValue(os, *v);
        os << "}";
      } else {
        os << absl::StreamFormat("<nullptr>");
      }
    } else if constexpr (std::is_convertible_v<V, std::string_view>) {
      os << absl::StreamFormat("\"%s\"", v);
    } else {
      os << absl::StreamFormat("%v", v);
    }
  }

  template<typename V>
  static void OStreamField(std::ostream& os, std::size_t idx, const V& v) {
    if (idx) {
      os << ", ";
    }
    if constexpr (!requires { typename Type::NoFieldNames; }) {
      static constexpr auto kNames = ::mbo::types::types_internal::GetFieldNames<Type>();
      if (idx < kNames.length() && !kNames[idx].empty()) {
        os << "." << kNames[idx] << ": ";
      }
    }
    OStreamValue(os, v);
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
  template<typename Lhs>
  friend auto operator<=>(const Lhs& lhs, const T& rhs) {
    return lhs <=> rhs.ToTupl();
  }

  template<typename Rhs>
  friend auto operator<=>(const T& lhs, const Rhs& rhs) {
    return lhs.ToTuple() <=> rhs;
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
// This default Extender is automatically available through `mb::types::Extend`.
//
// If the compiler and the structure support `__buildtin_dump_struct` (e.g. if
// compiled with Clang), then this automatically supports field names. However,
// this does not work with `union`s. Further, providing field names can be
// suppressed by providing a typename `NoFieldNames`, e.g.:
//   `using NoFieldNames = void;`
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
// This default Extender is automatically available through `mb::types::Extend`.
struct AbslHashable final : MakeExtender<"AbslHashable"_ts, AbslHashable_> {};

// Extender that injects functionality to make an `Extend`ed type comparable.
// All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
//
// This default Extender is automatically available through `mb::types::Extend`.
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

// The default extender is a wrapper around:
// * Streamable
// * Printable
// * Comparable
// * AbslHashable
// * AbslStringify
struct Default final {
  using RequiredExtender = void;

  static constexpr std::string_view GetExtenderName() { return decltype("Default"_ts)::str(); }

 private:
  // This friend is necessary to keep symbol visibility in check. But it has to
  // be either 'struct' or 'class' and thus results in `ImplT` being a struct.
  template<typename T, typename Extender>
  friend struct ::mbo::types::types_internal::UseExtender;

  // NOTE: The list of wrapped extenders needs to be in sync with `mbo::extender::Extend`.

  template<typename ExtenderOrActualType>
  struct Impl : Streamable_<Printable_<Comparable_<AbslHashable_<AbslStringify_<ExtenderOrActualType>>>>> {
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
