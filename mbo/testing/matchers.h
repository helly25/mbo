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

#ifndef MBO_TESTING_MATCHERS_H_
#define MBO_TESTING_MATCHERS_H_

#include <concepts>  // IWYU pragma: keep
#include <type_traits>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "mbo/types/traits.h"  // IWYU pragma: keep

namespace mbo::testing {
namespace testing_internal {

// Implements the polymorphic `CapacityIs` matcher, which
// can be used as a Matcher<T> as long as T is either a container that defines
// capacity() (e.g. std::vector or std::string), or a C-style string.
template<typename CapacityMatcher>
class CapacityIsMatcher {
 public:
  explicit CapacityIsMatcher(const CapacityMatcher& capacity_matcher) : capacity_matcher_(capacity_matcher) {}

  template<typename Container>
  operator ::testing::Matcher<Container>() const {  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    return ::testing::Matcher<Container>(new Impl<const Container&>(capacity_matcher_));
  }

  template<typename Container>
  class Impl : public ::testing::MatcherInterface<Container> {
   public:
    using CapacityType = decltype(std::declval<Container>().capacity());

    explicit Impl(const CapacityMatcher& capacity_matcher)
        : capacity_matcher_(::testing::MatcherCast<CapacityType>(capacity_matcher)) {}

    void DescribeTo(::std::ostream* os) const override {
      *os << "has a capacity that ";
      capacity_matcher_.DescribeTo(os);
    }

    void DescribeNegationTo(::std::ostream* os) const override {
      *os << "has a capacity that ";
      capacity_matcher_.DescribeNegationTo(os);
    }

    bool MatchAndExplain(Container container, ::testing::MatchResultListener* listener) const override {
      CapacityType capacity = container.capacity();
      ::testing::StringMatchResultListener capacity_listener;
      const bool result = capacity_matcher_.MatchAndExplain(capacity, &capacity_listener);
      *listener << "whose capacity " << capacity << (result ? " matches" : " doesn't match");
      ::testing::internal::PrintIfNotEmpty(capacity_listener.str(), listener->stream());
      return result;
    }

   private:
    const ::testing::Matcher<CapacityType> capacity_matcher_;
  };

 private:
  const CapacityMatcher capacity_matcher_;
};

}  // namespace testing_internal

// Returns a matcher that matches the container capacity. The container must support both capacity() and size_type which
// all STL-like containers provide. Note that the parameter 'capacity' can be a value of type size_type as well as
// matcher. For instance:
//   EXPECT_THAT(container, CapacityIs(2));     // Checks container has a capacity for 2 elements.
//   EXPECT_THAT(container, CapacityIs(Le(2));  // Checks container has a capacity for at most 2.
template<typename SizeMatcher>
inline testing_internal::CapacityIsMatcher<SizeMatcher> CapacityIs(const SizeMatcher& size_matcher) {
  return testing_internal::CapacityIsMatcher<SizeMatcher>(size_matcher);
}

namespace testing_internal {

template<typename Transformer, typename ContainerMatcher>
class WhenTransformedByMatcher {
 public:
  WhenTransformedByMatcher(const Transformer& transformer, const ContainerMatcher& container_matcher)
      : transformer_(transformer), container_matcher_(container_matcher) {}

  template<typename Container>
  requires(
      std::is_invocable_v<Transformer, typename std::remove_cvref_t<Container>::value_type>
      && !std::same_as<void, std::invoke_result_t<Transformer, typename std::remove_cvref_t<Container>::value_type>>)
  operator ::testing::Matcher<Container>() const {  // NOLINT(*-explicit-*)
    return ::testing::Matcher<Container>(new Impl<const Container&>(transformer_, container_matcher_));
  }

 private:
  template<typename Container>
  class Impl : public ::testing::MatcherInterface<Container> {
   public:
    using ContainerType = std::remove_cvref_t<Container>;
    using TransformedType = std::invoke_result_t<Transformer, typename ContainerType::value_type>;
    using TransformedContainerType = std::vector<TransformedType>;
    using MatcherType = ::testing::Matcher<TransformedContainerType>;

    explicit Impl(const Transformer& transformer, const ContainerMatcher& container_matcher)
        : transformer_(transformer),
          container_matcher_(::testing::MatcherCast<TransformedContainerType>(container_matcher)) {}

    void DescribeTo(::std::ostream* os) const override {
      *os << "when transformed ";
      container_matcher_.DescribeTo(os);
    }

    void DescribeNegationTo(::std::ostream* os) const override {
      *os << "when transformed ";
      container_matcher_.DescribeNegationTo(os);
    }

    bool MatchAndExplain(const Container& container, ::testing::MatchResultListener* listener) const override {
      TransformedContainerType transformed;
      transformed.reserve(container.size());
      for (const auto& v : container) {
        transformed.emplace_back(transformer_(v));
      }
      ::testing::StringMatchResultListener container_listener;
      const bool result = container_matcher_.MatchAndExplain(transformed, &container_listener);
      *listener << "which (when transformed";
      constexpr std::size_t kShowMaxElements = 0;
      if constexpr (kShowMaxElements > 0) {
        *listener << " is {";
        std::size_t index = 0;
        for (const auto& v : transformed) {
          if (index++ > 0) {
            *listener << ", ";
          }
          *listener << ::testing::PrintToString(v);
          if (index >= kShowMaxElements) {
            *listener << ", ...";
            break;
          }
        }
        *listener << "}";
      }
      *listener << ") " << (result ? "matches" : "doesn't match");
      ::testing::internal::PrintIfNotEmpty(container_listener.str(), listener->stream());
      return result;
    }

   private:
    const Transformer& transformer_;
    const MatcherType container_matcher_;
  };

  const Transformer& transformer_;
  const ContainerMatcher& container_matcher_;
};

}  // namespace testing_internal

// Matcher that allows to compare containers after transforming them. This sometimes allows for much
// more concise comparisons where a golden expectation is already available that only differs in a
// simple transformation. The matcher transformation must convert the elements of the container arg
// from their `value_type` to the matchers value type.
//
// That means that a `std::vector` of `int` can be compared to a `std::set` of `std::string`, if the
// transformer takes `int` as an argument and returns `std::string` as its result.
//
// Example:
// ```
// std::vector<int> numbers{1, 2, 3};
// std::set<std::string> strs{"1", "2", "3"};
// EXPECT_THAT(numbers, WhenTransformedBy([](int v) { return absl::StrCat(v); }, ElementsAre(strs)));
// ```
//
// More practical: Instead of using `Key` for every value in `ElementsAre` when checking a mapped
// container's keys, you can transform the input to just return the keys and then compare:
// ```
//  const std::map<int, int> map{{1, 2}, {3, 4}};
//  EXPECT_THAT(
//      map,
//      WhenTransformedBy([](const std::pair<int, int>& v) { return v.first; }), ElementsAre(1, 2));
// ```
//
// The internal comparison will always be performed on a `std::vector` whose elements are the result
// type of the `transformer`. This means that the resultign elements are exactly in the order of the
// default iteration of the argument. The matcher adapter `WhenSorted` or unordered versions of the
// standard container matchers work as expected.
template<typename Transformer, typename ContainerMatcher>
inline auto WhenTransformedBy(const Transformer& transformer, const ContainerMatcher& container_matcher) {
  return testing_internal::WhenTransformedByMatcher(transformer, container_matcher);
}

}  // namespace mbo::testing

#endif  // MBO_TESTING_MATCHERS_H_
