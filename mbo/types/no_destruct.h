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

#ifndef MBO_TYPES_NO_DESTRUCT_H_
#define MBO_TYPES_NO_DESTRUCT_H_

#include <array>
#include <memory>  // IWYU pragma: keep
#include <utility>

namespace mbo::types {

// NOLINTBEGIN(*-pro-type-member-init)

// Template `struct NoDestruct` allows to create `static const` (global) variables.
// New compilers are necessary in order to allow for `static constexpr`, where the
// following issues are key:
// . In-place construction with operator `new` is not a `constexpr`, so instead
//   `std::construct_at` must be used which is marked `constexpr`. This requires
//   . GCC 10.1+ or Clang 16+
//   . Apple clang version 15.0.0 (clang-1500.0.34.3) (Beta 4+) works too, so
//     Clang 15.0.7 should work.
// . For `constexpr` to work the target (and thus all fields) must be able to be
//   `constexpr` constructable. One common issue is with `std::string` which need
//   . GCC >= 11,
//   . Clang 16+,
//   . Apple clang version 15.0.0 (clang-1500.0.34.3) (Beta 4+).
// https://godbolt.org/z/WahPP8z36
template<typename T>
class NoDestruct final {
 private:
  union Data {
    constexpr Data() noexcept {}
    constexpr ~Data() noexcept {};
    Data(const Data&) = delete;
    Data& operator=(const Data&) = delete;
    Data(Data&&) = delete;
    Data& operator=(Data&&) = delete;
    T value;
  };

 public:
  constexpr NoDestruct() noexcept { std::construct_at(&data_[0].value); }
  template<typename... Args>
  constexpr explicit NoDestruct(Args&&... args) noexcept { std::construct_at(&data_[0].value, std::forward<Args>(args)...); }
  
  constexpr ~NoDestruct() noexcept = default;

  NoDestruct(const NoDestruct&) = delete;
  NoDestruct(NoDestruct&&) = delete;
  NoDestruct& operator=(const NoDestruct&) = delete;
  NoDestruct& operator=(NoDestruct&&) = delete;

  constexpr const T& Get() const noexcept { return data_[0].value; }

  constexpr const T& operator*() const noexcept { return Get(); }
  constexpr const T* operator->() const noexcept { return &Get(); }

  T& Get() noexcept { return data_[0].value; }

  T& operator*() noexcept { return Get(); }
  T* operator->() noexcept { return &Get(); }

 private:
  std::array<Data, 1> data_;
};
// NOLINTEND(*-pro-type-member-init)

}  // namespace mbo::types

#endif  // MBO_TYPES_NO_DESTRUCT_H_
