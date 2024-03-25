// Copyright M. Boerger (helly25.com)
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

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

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

}  // namespace mbo::testing

#endif  // MBO_TESTING_MATCHERS_H_
