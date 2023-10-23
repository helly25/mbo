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
// is a `std::input_iterator`. Type `ConstScan` is similar to `absl::Span<const T>` made from
// `absl::MakeConstSpan`.
//
// That means this type supports pretty much all STL containers including `std::initializer_list`.
//
// IMPORTANT: This type is INDEPENDENT of the container type that means it is possible to write
// single functions that can take containers of any container type without needing further templates
// or overloads.
//
// HOWEVER: This type is possibly slower as the aformentioned wrappers, as it need mores abstraction
// layers to accomplish the independence of the container type.
//
// The wrappers of this type can only be instantiated/created through `MakeAnyScan` or directly from
// a `std::initializer_list` argument.
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
//
// The more specialized `ConvertingScan` modifies `AnyScan` to accept `MakeConvertingScan` which
// allows type erased scanning using a specified type that can be constructed from the values of a
// compatible container. For instance this allows to scan containers of string-like values with a
// specific string-like container (see below).
//
// Example:
//
// ```
// void Report(const AnyScan<std::string>& data) {
//   for (auto [first, second] : data) {
//     Report(first, second);
//   }
// }
//
// void User() {
//   std::vector<std::string> data{"foo", "bar"};
//   Report(MakeAnyScan(data));
// }
// ```
//
// Here the caller used `std::vector`, but the caller may just switch to `std::set` or another
// caller may use a `std::array` or a `std::initializer_list` or whatever container. However, all
// callers must use containers with their `value_type` using `std::string`.
//
// WARNING: When scanning types like `std::set` or `std::map` or similar indexed containers, then it
// is important to know that their keys `value_type` (for set) or `value_type::first_type` (for map)
// is const and thus the type to scan over must be specified accordingly. In many cases it is more
// appropriate to work with `ConstScan` created by `MakeConstScan`. Example with correct `AnyScan`:
//
// void Foo(const AnyScan<std::pair<const std::string, int>& scan) {}
// std::map<std::string, int> data{{"foo", 25}, {"bar", 42}};
// Foo(MakeAnyScan(data));  // THIS WORKS because `Foo` has its `value_type::first_type` as const.
//
// NOTE: When working with `std::initializer_list`, then sometimes these can be assigned directly
// to the `AnyScan` without the need to invoke `MakeAnyScan`. However, the types must match
// const correctly. This cannot be fixed as for some types (e.g. std::string) that perform implicit
// conversions it would be an ambiguity to provide both `std::initializer_list<T>` as well as the
// const form `std::initializer_list<const T>` as parameters. This is not the case for `ConstScan`
// of `ConvertingScan`.
template<typename ValueType, typename DifferenceType = std::ptrdiff_t>
class AnyScan;

// A `ConstScan` is similar to an `AnyScan` and an `absl::Span<const T>` but unlike in the case of
// an `AnyScan` the values of a `ConstScan` are always const. That simplifies accepting const keyed
// containers like `std::set` or `std::map`. Exmaple:
//
// void Foo(const ConstScan<std::string>& scan) {}
// std::set<std::string> data{"foo", "bar"};
// Foo(MakeConstScan(data));  // THIS WORKS because we are using `ConstScan`.
//
// NOTE: A `ConstScan` is still referencing the results, so it is faster than a `ConvertingScan`.
// However, when working with pairs, then all containers must have their `value_type::first_type`
// const - or it is not possible to mix const key'd containers like `std::map` with non const key'd
// containers (e.g. `std::vector<std::pair<int, int>>` as opposed to `std::map<int, int>` which is
// mixable with `std::vector<std::pair<const int, int>>`).
template<typename ValueType, typename DifferenceType = std::ptrdiff_t>
class ConstScan;

// A `ConvertingScan` is very similar to `AnyScan`, however it allows scanning of any container
// whose `value_types` - as returned by its const_iterator - can be copy-converted into the
// specified type. This in particular allows scanning containers of any string-like type as either
// `std::string` or `std::string_view`. The emphasis is on COPY as this kind of conversion requires
// return by value. As such the `ConvertingScan`'s iterators do not have `operator->()`.
//
// The wrappers of this type can only be instantiated/created through `MakeConvertingScan` or
// directly from a `std::initializer_list` argument.
//
// Example:
//
// ```
// void Report(const ConvertingScan<std::pair<std::string_view, std::string_view>>& data) {
//   for (auto it = data.begin(); it != data.end(); ++it) {
//     Report((*it).first, (*it).second);  // Use `(*it)` iteration as `operator->` is missing (1).
//     // (1) Of course the iteration could just be: for (auto [first, second] : data)
//   }
// }
//
// void User() {
//   std::vector<std::string> data{{"foo", "25"}, {"bar", "42"}};
//   Report(MakeConvertingScan(data));
// }
// ```
//
// Here the caller used `std::vector<std::pair<std::string, std::string>>`, but the target `Report`
// wants the elements converted to `std::pair<std::string_view, std::string_view>`, so this had to
// be a `ConvertingScan` that handles the type difference.
//
// Note: ConvertingScan is less restrict about const than AnyScan as it works on copies.
template<typename ValueType, typename DifferenceType = std::ptrdiff_t>
class ConvertingScan;

namespace container_internal {

enum class ScanMode { kAny, kConst, kConverting };

template<typename ValueType, typename DifferenceType = std::ptrdiff_t, ScanMode S = ScanMode::kAny>
class AnyScanImpl;

template<typename Container>
concept AcceptableContainer =
    ::mbo::types::ContainerHasInputIterator<Container> || ::mbo::types::IsInitializerList<Container>;

template<
    AcceptableContainer Container,
    ScanMode ScanModeVal,
    bool IsMoveOnly = std::movable<Container> && !::mbo::types::IsInitializerList<std::remove_cvref_t<Container>>>
class MakeAnyScanData {
 public:
  template<typename V, typename D, ScanMode S>
  friend class AnyScanImpl;

  static constexpr ScanMode kScanMode = ScanModeVal;

  MakeAnyScanData() = delete;

  explicit MakeAnyScanData(Container container) : container_(std::make_shared<Container>(std::move(container))) {}

 private:
  Container& container() const noexcept { return *container_; }

  std::shared_ptr<Container> container_;
};

template<AcceptableContainer Container, ScanMode ScanModeVal>
class MakeAnyScanData<Container, ScanModeVal, false> {
 public:
  template<typename V, typename D, ScanMode S>
  friend class AnyScanImpl;

  static constexpr ScanMode kScanMode = ScanModeVal;

  MakeAnyScanData() = delete;

  explicit MakeAnyScanData(const Container& container) : container_(container) {}

 private:
  const Container& container() const noexcept { return container_; }

  const Container& container_;
};

template<typename ValueType, typename DifferenceType, ScanMode ScanModeVal>
class AnyScanImpl {
 private:
  static constexpr ScanMode kScanMode = ScanModeVal;
  static constexpr bool kAccessByRef = kScanMode != ScanMode::kConverting;

  using AccessType = std::conditional_t<kAccessByRef, ValueType&, ValueType>;

  using FuncSave = std::function<void()>;
  using FuncMore = std::function<bool()>;
  using FuncCurr = std::function<AccessType()>;
  using FuncNext = std::function<void()>;

  struct AccessFuncs {
    AccessFuncs(FuncSave save, FuncMore more, FuncCurr curr, FuncNext next)
        : save(std::move(save)), more(std::move(more)), curr(std::move(curr)), next(std::move(next)) {}

    FuncSave save;
    FuncMore more;
    FuncCurr curr;
    FuncNext next;
  };

  using FuncClone = std::function<std::shared_ptr<AccessFuncs>()>;

  template<typename T, typename U, bool ArePairs = ::mbo::types::IsPair<T>&& ::mbo::types::IsPair<U>>
  struct IsCompatibleImpl
      : std::conditional_t<
            std::is_const_v<T>,
            std::bool_constant<std::same_as<T, const U>>,
            std::bool_constant<std::same_as<T, U>>> {};

  template<typename T, typename U>
  struct IsCompatibleImpl<T, U, true>
      : std::bool_constant<
            std::same_as<typename T::first_type, typename U::first_type>
            && std::same_as<typename T::second_type, typename U::second_type>> {};

  template<typename T, typename U>
  static constexpr void CheckCompatible() noexcept {
    if constexpr (::mbo::types::IsPair<T> && ::mbo::types::IsPair<U>) {
      static_assert(
          IsCompatibleImpl<T, U>::value,
          "A common reason for this to fail is scanning over `std::map` and similar indexed containers "
          "without specifying the scan type's `first_type` as const.");
    } else {
      static_assert(IsCompatibleImpl<T, U>::value);
    }
  }

  template<typename V, typename D>
  friend class ::mbo::container::AnyScan;

  template<typename V, typename D>
  friend class ::mbo::container::ConstScan;

  template<typename V, typename D>
  friend class ::mbo::container::ConvertingScan;

  template<typename C>
  using ContainerIterator = std::conditional_t<
      std::is_const_v<C>,
      typename std::remove_cvref_t<C>::const_iterator,
      typename std::remove_cvref_t<C>::iterator>;

  template<typename C>
  static auto MakeSharedIterator(C& container) {
    using Iterator = ContainerIterator<C>;
    return std::shared_ptr<Iterator>(new Iterator(container.begin()));
  }

 public:
  using difference_type = DifferenceType;
  using element_type = std::remove_cvref_t<ValueType>;
  using value_type = ValueType;
  using pointer = ValueType*;
  using const_pointer = const ValueType*;
  using reference = ValueType&;
  using const_reference = const ValueType&;

  AnyScanImpl() = delete;

 private:
  // NOLINTBEGIN(bugprone-forwarding-reference-overload)

  // For MakAnyScan / MakeConstScan
  template<AcceptableContainer Container>
  requires(kAccessByRef)
  explicit AnyScanImpl(MakeAnyScanData<Container, kScanMode> data)
      : clone_([data = std::move(data)]() {
          // CheckCompatible<AccessType, decltype(*std::declval<ContainerIterator<Container>&>())>();
          auto pos = MakeSharedIterator(data.container());
          return std::make_shared<AccessFuncs>(
              /* save */ [save = pos] {},
              /* more */ [end = data.container().end(), &it = *pos]() -> bool { return it != end; },
              /* curr */ [&it = *pos]() -> AccessType { return *it; },
              /* next */ [&it = *pos] { ++it; });
        }) {}

  // For MakConvertingScan
  template<AcceptableContainer Container>
  requires(
      !kAccessByRef  // This is the ConvertingScan constructor
      && std::constructible_from<AccessType, ::mbo::types::ContainerConstIteratorValueType<Container>>
      && std::constructible_from<value_type, AccessType>)
  explicit AnyScanImpl(MakeAnyScanData<Container, kScanMode> data)
      : clone_([data = std::move(data)] {
          auto pos = MakeSharedIterator(data.container());
          return std::make_shared<AccessFuncs>(
              /* save */ [pos = pos] {},
              /* more */ [end = data.container().end(), &it = *pos]() -> bool { return it != end; },
              /* curr */ [&it = *pos]() -> AccessType { return AccessType(*it); },
              /* next */ [&it = *pos] { ++it; });
        }) {}

  // NOLINTEND(bugprone-forwarding-reference-overload)

 public:
  template<typename ItElementType, typename ItValueType, typename ItPointer, typename ItReference>
  class iterator_impl {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = AnyScanImpl::difference_type;
    using element_type = ItElementType;
    using value_type = ItValueType;
    using pointer = ItPointer;
    using reference = ItReference;

    iterator_impl() : funcs_(nullptr) {}

    explicit iterator_impl(const std::shared_ptr<AccessFuncs>& funcs) : funcs_(std::move(funcs)) {}

    reference operator*() const noexcept
    requires kAccessByRef
    {
      // We check both the presence of `funcs_` as well as calling the actual `more` function.
      // That means we bypass any protection an iterator may have, but we can make this function
      // `noexcept` assuming the iterator is noexcept for access. On the other hand we expect that
      // out of bounds access may actually raise. So we effectively side step such exceptions.
      ABSL_CHECK(funcs_->more());
      return funcs_->curr();
    }

    value_type operator*() const noexcept
    requires(!kAccessByRef)
    {
      ABSL_CHECK(funcs_->more());
      return value_type(funcs_->curr());
    }

    const_pointer operator->() const noexcept
    requires kAccessByRef
    {
      return funcs_->more() ? &funcs_->curr() : nullptr;
    }

    iterator_impl& operator++() noexcept {
      funcs_->next();
      return *this;
    }

    iterator_impl operator++(int) noexcept {  // NOLINT(cert-dcl21-cpp)
      iterator_impl result = *this;
      funcs_->next();
      return result;
    }

    template<typename OE, typename OV, typename OP, typename OR>
    bool operator==(const iterator_impl<OE, OV, OP, OR>& other) const noexcept {
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

    template<typename OE, typename OV, typename OP, typename OR>
    bool operator!=(const iterator_impl<OE, OV, OP, OR>& other) const noexcept {
      return !(*this == other);
    }

   private:
    std::shared_ptr<AccessFuncs> funcs_;
  };

  using iterator = iterator_impl<element_type, value_type, pointer, reference>;
  using const_iterator = iterator_impl<element_type, const value_type, const_pointer, const_reference>;

  iterator begin() noexcept { return iterator(std::move(clone_())); }

  iterator end() noexcept { return iterator(nullptr); }

  const_iterator begin() const noexcept { return const_iterator(std::move(clone_())); }

  const_iterator end() const noexcept { return const_iterator(nullptr); }

  const_iterator cbegin() const noexcept { return const_iterator(std::move(clone_())); }

  const_iterator cend() const noexcept { return const_iterator(nullptr); }

 private:
  FuncClone clone_;
};

}  // namespace container_internal

template<typename ValueType, typename DifferenceType>
class AnyScan : public container_internal::AnyScanImpl<ValueType, DifferenceType, container_internal::ScanMode::kAny> {
 private:
  using AnyScanImpl = container_internal::AnyScanImpl<ValueType, DifferenceType, container_internal::ScanMode::kAny>;

 public:
  AnyScan(const std::initializer_list<ValueType>& data)
      : AnyScanImpl(
          container_internal::MakeAnyScanData<std::initializer_list<ValueType>, container_internal::ScanMode::kAny>(
              data)) {}

  template<::mbo::types::ContainerHasInputIterator Container>
  requires(std::constructible_from<ValueType, typename std::remove_cvref_t<Container>::value_type>)
  AnyScan(  // NOLINT(*-explicit-constructor,*-explicit-conversions)
      container_internal::MakeAnyScanData<Container, container_internal::ScanMode::kAny> data)
      : AnyScanImpl(std::move(data)) {}

  using AnyScanImpl::begin;
  using AnyScanImpl::end;
};

template<typename ValueType, typename DifferenceType>
class ConstScan
    : public container_internal::AnyScanImpl<const ValueType, DifferenceType, container_internal::ScanMode::kConst> {
 private:
  using AnyScanImpl =
      container_internal::AnyScanImpl<const ValueType, DifferenceType, container_internal::ScanMode::kConst>;

 public:
  ConstScan(const std::initializer_list<ValueType>& data)
      : AnyScanImpl(
          container_internal::MakeAnyScanData<std::initializer_list<ValueType>, container_internal::ScanMode::kConst>(
              data)) {}

  template<container_internal::AcceptableContainer Container>
  requires(std::constructible_from<const ValueType, typename std::remove_cvref_t<Container>::value_type>)
  ConstScan(  // NOLINT(*-explicit-constructor,*-explicit-conversions)
      container_internal::MakeAnyScanData<Container, container_internal::ScanMode::kConst> data)
      : AnyScanImpl(std::move(data)) {}

  using AnyScanImpl::begin;
  using AnyScanImpl::end;
};

template<typename ValueType, typename DifferenceType>
class ConvertingScan
    : public container_internal::AnyScanImpl<ValueType, DifferenceType, container_internal::ScanMode::kConverting> {
 private:
  using AnyScanImpl =
      container_internal::AnyScanImpl<ValueType, DifferenceType, container_internal::ScanMode::kConverting>;

 public:
  ConvertingScan(const std::initializer_list<ValueType>& data)
      : AnyScanImpl(
          container_internal::
              MakeAnyScanData<std::initializer_list<ValueType>, container_internal::ScanMode::kConverting>(data)) {}

  template<container_internal::AcceptableContainer Container>
  requires(std::constructible_from<ValueType, typename std::remove_cvref_t<Container>::value_type>)
  ConvertingScan(  // NOLINT(*-explicit-constructor,*-explicit-conversions)
      container_internal::MakeAnyScanData<Container, container_internal::ScanMode::kConverting> data)
      : AnyScanImpl(std::move(data)) {}

  using AnyScanImpl::begin;
  using AnyScanImpl::end;
};

// Creates adapters that can be received by `AnyScan` parameters.
template<container_internal::AcceptableContainer Container>
auto MakeAnyScan(Container&& container) noexcept {
  return container_internal::MakeAnyScanData<Container, container_internal::ScanMode::kAny>(
      std::forward<Container>(container));
}

// Creates adapters that can be received by `ConstScan` parameters.
template<container_internal::AcceptableContainer Container>
auto MakeConstScan(Container&& container) noexcept {
  return container_internal::MakeAnyScanData<Container, container_internal::ScanMode::kConst>(
      std::forward<Container>(container));
}

// Creates adapters that can be received by `ConvertingScan` parameters.
template<container_internal::AcceptableContainer Container>
auto MakeConvertingScan(Container&& container) noexcept {
  return container_internal::MakeAnyScanData<Container, container_internal::ScanMode::kConverting>(
      std::forward<Container>(container));
}

}  // namespace mbo::container

// NOLINTEND(readability-identifier-naming)

#endif  // MBO_CONTAINER_ANY_SCAN_H_
