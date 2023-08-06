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

#ifndef MBO_TYPES_COPY_CONVERT_CONTAINER_H_
#define MBO_TYPES_COPY_CONVERT_CONTAINER_H_

#include "mbo/types/traits.h"

namespace mbo::types {

template<typename ContainerIn, typename ContainerOut>
concept ContainerCopyConvertibleRaw = requires {
  ContainerIsForwardIteratable<ContainerIn>;
  ContainerIsForwardIteratable<ContainerOut>;
  ContainerHasEmplace<ContainerOut, typename ContainerIn::value_type>
      || ContainerHasEmplaceBack<ContainerOut, typename ContainerIn::value_type>
      || ContainerHasInsert<ContainerOut, typename ContainerIn::value_type>
      || ContainerHasPushBack<ContainerOut, typename ContainerIn::value_type>;
};

template<typename ContainerIn, typename ContainerOut>
concept ContainerCopyConvertible = ContainerCopyConvertibleRaw<std::remove_cvref_t<ContainerIn>, std::remove_reference_t<ContainerOut>>;

namespace internal {

template<IsForwardIteratable Container>
class CopyConvertContainer {
 public:
  using ContainerType = std::remove_reference_t<Container>;
  using ValueType = typename ContainerType::value_type;

  explicit CopyConvertContainer(const Container& container) noexcept : container_(container) {}
  ~CopyConvertContainer() = default;
  CopyConvertContainer(const CopyConvertContainer&) = delete;
  CopyConvertContainer& operator=(const CopyConvertContainer&) = delete;
  CopyConvertContainer(CopyConvertContainer&&) noexcept = default;
  CopyConvertContainer& operator=(CopyConvertContainer&&) noexcept = default;

  template<typename ContainerOut>
  requires ContainerCopyConvertible<Container, ContainerOut>
  operator ContainerOut() const {  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    ContainerOut result;
    for (const auto& v : container_) {
      if constexpr (ContainerHasEmplace<ContainerOut, ValueType>) {
        result.emplace(v);
      } else if constexpr (ContainerHasEmplaceBack<ContainerOut, ValueType>) {
        result.emplace_back(v);
      } else if constexpr (ContainerHasInsert<ContainerOut, ValueType>) {
        result.insert(v);
      } else if constexpr (ContainerHasPushBack<ContainerOut, ValueType>) {
        result.push_back(v);
      }
    }
    return result;
  }

 private:
  const ContainerType& container_;
};

}  // namespace internal

// Copies `Container` into `ContainerOut` while converting values as needed.
// Requires that the values in `Container` can be emplaced or inserted into `ContainerOut`.
//
// Example:
//
//   std::vector<std::string_view> input{"foo", "bar", "baz"};
//   std::vector<string> strs = CopyConvertContainer(input);

template<IsForwardIteratable Container>
internal::CopyConvertContainer<Container> CopyConvertContainer(const Container& container) {
  return internal::CopyConvertContainer(container);
}

}  // namespace mbo::types

#endif  // MBO_TYPES_COPY_CONVERT_CONTAINER_H_
