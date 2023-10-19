// Copyright 2023 M. Boerger (helly25.com)
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

#ifndef MBO_CONTAINER_ANY_SCAN_H_
#define MBO_CONTAINER_ANY_SCAN_H_

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "absl/log/absl_check.h"
#include "mbo/types/traits.h"

// NOLINTBEGIN(readability-identifier-naming)

namespace mbo::container {

// Type `AnyScan` is similar to `std::span`, `std::range` and `absl::Span`. However it works with
// any container that supports `begin()`, `end()` and whose `iterator` (likely the `const_iterator`)
// is a `std::input_iterator`.
//
// That means this type supports pretty much all STL containers including `std::initializer_list`.
//
// IMPORTANT: this types is INDEPENDENT of the container type that means it is possible to write
// single functions that can take containers of any container type without needing further templates
// or overloads.
//
// HOWEVER: This types is slower as the aformentioned wrappers, as it need more abstraction layers
// to accomplish the independence of the container type.
//
// The wrappers of this type can only be instantiated/created through `MakeAnyScan`.
//
// The implementqtion is type-safe and does not use any `reinterpret_cast` of other unnecessary
// tricks.
//
// The actual `AnyScan` type depends on two types:
// - `ValueType` which is derived from the container's value-type.
// - `DifferenceType` which is either `std::ptrdiff_t` or if provided and not compvertible, the
//   container's `difference_type`. This is done because not all containers that are otherwise
//   compatible have a `difference_type` but in order for `AnyScan` itself to be compatible with
//   `std::input_iterator` it has to provide that type. Further, we aim to use a predictable type
//   as much as possible, so that the callers do not need to unnecessarily guess that type.
template<typename ValueType, typename DifferenceType = std::ptrdiff_t>
class AnyScan {
 private:
  using FuncMore = std::function<bool()>;
  using FuncCurr = std::function<const ValueType*()>;
  using FuncNext = std::function<void()>;

  struct TripleFunc {
    TripleFunc(FuncMore more, FuncCurr curr, FuncNext next)
        : more(std::move(more)), curr(std::move(curr)), next(std::move(next)) {}

    const FuncMore more;
    const FuncCurr curr;
    const FuncNext next;
  };

  using FuncClone = std::function<std::shared_ptr<TripleFunc>()>;

 public:
  using difference_type = DifferenceType;
  using value_type = const ValueType;
  using pointer = const ValueType*;
  using const_pointer = const ValueType*;
  using reference = const ValueType&;
  using const_reference = const ValueType&;

  ~AnyScan() noexcept = default;

  AnyScan() = delete;

  template<::mbo::types::ContainerHasInputIterator Container>
  explicit AnyScan(Container&& container)  // NOLINT(bugprone-forwarding-reference-overload)
      : clone_([container = std::forward<Container>(container)] {
          using RawContainer = std::remove_cvref_t<Container>;
          using const_iterator = RawContainer::const_iterator;
          auto pos = std::shared_ptr<const_iterator>(new const_iterator(container.begin()));
          return std::make_shared<TripleFunc>(
              /* more */ [end = container.end(), pos]() -> bool { return *pos != end; },
              /* curr */ [pos]() -> const ValueType* { return &**pos; },
              /* next */ [pos] { ++*pos; });
        }) {}

  AnyScan(const AnyScan&) noexcept = default;
  AnyScan& operator=(const AnyScan&) noexcept = default;
  AnyScan(AnyScan&&) noexcept = default;
  AnyScan& operator=(AnyScan&&) noexcept = default;

  class const_iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = AnyScan::difference_type;
    using value_type = AnyScan::value_type;
    using pointer = AnyScan::const_pointer;
    using reference = AnyScan::const_reference;
    using element_type = AnyScan::value_type;

    const_iterator() : funcs_(nullptr) {}

    explicit const_iterator(const std::shared_ptr<TripleFunc>& funcs) : funcs_(std::move(funcs)) {}

    const_reference operator*() const noexcept {
      // We check both the presence of `func_` as well as calling the actual `more` function.
      // That means we bypass any protection an iterator may have, but we can make this function
      // `noexcept` assuming the iterator is noexcept for access. On the other hand we expect that
      // out of bounds access may actually raise. So we effectively side step such exceptions.
      ABSL_CHECK(funcs_ && funcs_->more());
      return *funcs_->curr();
    }

    const_pointer operator->() const noexcept { return funcs_ ? funcs_->curr() : nullptr; }

    const_iterator& operator++() noexcept {
      if (funcs_) {
        funcs_->next();
      }
      return *this;
    }

    const_iterator operator++(int) noexcept {  // NOLINT(cert-dcl21-cpp)
      const_iterator result = *this;
      if (funcs_) {
        funcs_->next();
      }
      return result;
    }

    bool operator==(const const_iterator& other) const noexcept {
      if (this == &other || funcs_ == other.funcs_) {
        return true;
      }
      if (funcs_ == nullptr && !other.funcs_->more()) {
        return true;
      }
      if (other.funcs_ == nullptr && !funcs_->more()) {
        return true;
      }
      return false;
    }

    bool operator!=(const const_iterator& other) const noexcept { return !(*this == other); }

   private:
    std::shared_ptr<TripleFunc> funcs_;
  };

  using iterator = const_iterator;

  const_iterator begin() const noexcept { return const_iterator(std::move(clone_())); }

  const_iterator end() const noexcept { return const_iterator(nullptr); }

 private:
  const FuncClone clone_;
};

// Creates `AnyScan` instances.
template<::mbo::types::ContainerHasInputIterator Container>
auto MakeAnyScan(Container&& container) noexcept {
  using RawContainer = std::remove_cvref_t<Container>;
  using ValueType = RawContainer::value_type;
  using ActualDifferenceType = ::mbo::types::GetDifferenceType<RawContainer>;
  using DifferenceType = std::conditional_t<
      std::convertible_to<ActualDifferenceType, std::ptrdiff_t>, std::ptrdiff_t, ActualDifferenceType>;
  return AnyScan<ValueType, DifferenceType>(std::forward<Container>(container));
}

}  // namespace mbo::container

// NOLINTEND(readability-identifier-naming)

#endif  // MBO_CONTAINER_ANY_SCAN_H_
