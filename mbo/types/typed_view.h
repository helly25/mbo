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

#ifndef MBO_TYPES_TYPED_VIEW_H_
#define MBO_TYPES_TYPED_VIEW_H_

#include <iterator>
#include <ranges>
#include <utility>

// NOLINTBEGIN(readability-identifier-naming)

namespace mbo::types {

// Wrapper for STL views that provides type definitions, most importantly `value_type`.
// That allows such views to be used with GoogleTest container matchers.
template<typename View>
// `public` is load-bearing: `class` inherits privately by default, which makes every
// member `view_interface` exists to supply - empty(), size(), operator[], front() -
// inaccessible, and breaks its CRTP downcast outright. Deriving from it at all is
// pointless without this.
class TypedView : public std::ranges::view_interface<TypedView<View>> {
 private:
  using iterator_type = decltype(std::declval<View&>().begin());

 public:
  using value_type = std::iter_value_t<iterator_type>;
  using reference = std::iter_reference_t<iterator_type>;
  using difference_type = std::iter_difference_t<iterator_type>;

  TypedView() = delete;

  explicit TypedView(View&& view) : view_(std::move(view)) {}

  // NOTE: only const-iterable views can be wrapped. `filter_view` and
  // `drop_while_view` have no const `begin()` - finding the first element mutates
  // their cache - so `TypedView(std::views::filter(...))` does not compile. Adding
  // non-const overloads is not enough, because const consumers still need the const
  // pair; the const ones would have to be constrained on the underlying view.
  auto begin() const { return view_.begin(); }

  auto end() const { return view_.end(); }

 private:
  View view_;
};

}  // namespace mbo::types

// NOLINTEND(readability-identifier-naming)

#endif  // MBO_TYPES_TYPED_VIEW_H_
