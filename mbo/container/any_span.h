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

#ifndef MBO_CONTAINER_ANY_SPAN_H_
#define MBO_CONTAINER_ANY_SPAN_H_

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "absl/log/absl_check.h"
#include "mbo/types/traits.h"

// NOLINTBEGIN(readability-identifier-naming)

namespace mbo::container {
namespace container_internal {

template<typename Container>
concept ContainerHasInputIterator = requires {
  typename std::remove_cvref_t<Container>::const_iterator;
  requires std::forward_iterator<typename std::remove_cvref_t<Container>::const_iterator>;
};

template<typename T>
concept HasDifferenceType = requires { typename T::difference_type; };

template<typename T, bool = HasDifferenceType<T>>
struct GetDifferenceTypeImpl {
  using type = std::ptrdiff_t;
};

template<typename T>
struct GetDifferenceTypeImpl<T, true> {
  using type = T::difference_type;
};

template<typename T>
using GetDifferenceType = GetDifferenceTypeImpl<T>::type;

}  // namespace container_internal

template<typename ValueType, typename DifferenceType = std::ptrdiff_t>
class AnySpan {
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

  ~AnySpan() noexcept = default;
  AnySpan() = delete;

  AnySpan(const AnySpan&) noexcept = default;
  AnySpan& operator=(const AnySpan&) noexcept = default;
  AnySpan(AnySpan&&) noexcept = default;
  AnySpan& operator=(AnySpan&&) noexcept = default;

  template<container_internal::ContainerHasInputIterator Container>
  explicit AnySpan(Container&& container)  // NOLINT(bugprone-forwarding-reference-overload)
      : clone_([container = std::forward<Container>(container)] {
          using RawContainer = std::remove_cvref_t<Container>;
          using const_iterator = RawContainer::const_iterator;
          auto pos = std::shared_ptr<const_iterator>(new const_iterator(container.begin()));
          return std::make_shared<TripleFunc>(
              /* more */ [end = container.end(), pos]() -> bool { return *pos != end; },
              /* curr */ [pos]() -> const ValueType* { return &**pos; },
              /* next */ [pos] { ++*pos; });
        }) {}

  class const_iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = AnySpan::difference_type;
    using value_type = AnySpan::value_type;
    using pointer = AnySpan::const_pointer;
    using reference = AnySpan::const_reference;
    using element_type = AnySpan::value_type;

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

template<container_internal::ContainerHasInputIterator Container>
auto MakeAnySpan(Container&& container) noexcept {
  using RawContainer = std::remove_cvref_t<Container>;
  using ValueType = RawContainer::value_type;
  using DifferenceType = container_internal::GetDifferenceType<RawContainer>;
  return AnySpan<ValueType, DifferenceType>(std::forward<Container>(container));
}

}  // namespace mbo::container

// NOLINTEND(readability-identifier-naming)

#endif  // MBO_CONTAINER_ANY_SPAN_H_
