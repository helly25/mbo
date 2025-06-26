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

#ifndef MBO_TYPES_TYPED_VIEW_H
#define MBO_TYPES_TYPED_VIEW_H

#include <iterator>
#include <ranges>
#include <utility>

// NOLINTBEGIN(readability-identifier-naming)

namespace mbo::types {

// Wrapper for STL views that provides type definitions, most importantly `value_type`.
// That allows such views to be used with GoogleTest container matchers.
template<typename View>
class TypedView : std::ranges::view_interface<TypedView<View>> {
 private:
  using iterator_type = decltype(std::declval<View&>().begin());

 public:
  using value_type = std::iter_value_t<iterator_type>;
  using reference = std::iter_reference_t<iterator_type>;
  using difference_type = std::iter_difference_t<iterator_type>;

  TypedView() = delete;

  explicit TypedView(View&& view) : view_(std::move(view)) {}

  auto begin() const { return view_.begin(); }

  auto end() const { return view_.end(); }

 private:
  View view_;
};

}  // namespace mbo::types

// NOLINTEND(readability-identifier-naming)

#endif  // MBO_TYPES_TYPED_VIEW_H
