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

#ifndef MBO_TYPES_INTERNAL_EXTENDER_H_
#define MBO_TYPES_INTERNAL_EXTENDER_H_

// IWYU pragma private, include "mbo/types/extend.h"

#include <array>
#include <concepts>  // IWYU pragma: keep
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "mbo/types/tuple.h"  // IWYU pragma: keep

namespace mbo::types {

// Base Extender implementation for CRTP functionality injection.
// This must always be present.
template<typename ActualType>
struct ExtendBase {
 private:
  // Struct `AnyBaseExtender` is a private member as it can confuse conversions throughout the code.
  struct AnyBaseExtender {
    template<typename U>
    operator U() const noexcept {  // NOLINT(*-explicit-con*)
      return {};
    }
  };

 public:
  using Type = ActualType;  // Used for type chaining in `UseExtender`

  // Construct from tuple.
  // The tuple elements must match the field types or the field types must allow conversion.
  template<typename... Args>
  requires std::constructible_from<Type, AnyBaseExtender, Args...>
  static Type ConstructFromTuple(std::tuple<Args...>&& args) {  // NOLINT(*-param-not-moved)
    return std::apply(ConstructFromArgs<Args...>, std::forward<std::tuple<Args...>>(args));
  }

  // Construct from separate arguments.
  // The arguments must match the field types or the field types must allow conversion.
  template<typename... Args>
  requires std::constructible_from<Type, AnyBaseExtender, Args...>
  static Type ConstructFromArgs(Args&&... args) {
    return Type{AnyBaseExtender{}, std::forward<Args>(args)...};
  }

 protected:  // DO NOT expose anything publicly.
  auto ToTuple() const { return StructToTuple(static_cast<const ActualType&>(*this)); }

 private:  // DO NOT expose anything publicly.
  template<typename U, typename Extender>
  friend struct UseExtender;
  friend class Stringify;
};

namespace types_internal {

// Determine whether type `Extended` is an `Extend`ed type.
//
// This is an incomplete concept and the public version `mbo::types::IsExtended` should be used.
template<typename Extended>
concept IsExtended = requires {
  typename Extended::Type;
  typename Extended::RegisteredExtenders;
  typename Extended::UnexpandedExtenders;
  requires std::is_base_of_v<ExtendBase<typename Extended::Type>, Extended>;
  requires mbo::types::IsTuple<typename Extended::RegisteredExtenders>;
  requires mbo::types::IsTuple<typename Extended::UnexpandedExtenders>;
  {
    Extended::RegisteredExtenderNames()
  } -> std::same_as<std::array<std::string_view, std::tuple_size_v<typename Extended::RegisteredExtenders>>>;
};

// Access to the extender implementation for constructing the CRTP chain using
// `ExtendBuildChain`.
// This is done via a distinct struct, so that the non specialized templates
// can be private.
template<typename ExtenderBase, typename Extender>
struct UseExtender : Extender::template Impl<ExtenderBase> {};

}  // namespace types_internal
}  // namespace mbo::types

#endif  // MBO_TYPES_INTERNAL_EXTENDER_H_
