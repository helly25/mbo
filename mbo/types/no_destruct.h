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

namespace mbo::types {

// NOLINTBEGIN(*-pro-type-member-init)
template<typename T>
struct NoDestruct final {
  template<typename... Args>
  explicit NoDestruct(Args&&... args) noexcept {  // NOLINT(*-pro-type-member-init)
    (void)new (buf_) T(args...);
  }
  ~NoDestruct() = default;
  NoDestruct(const NoDestruct&) = delete;
  NoDestruct(NoDestruct&&) = delete;
  NoDestruct& operator=(const NoDestruct&) = delete;
  NoDestruct& operator=(NoDestruct&&) = delete;

  const T& operator*() const noexcept {
    // NOLINTNEXTLINE(coreguidelines-pro-type-reinterpret-cast)
    const T& res = reinterpret_cast<const T&>(buf_);
    return res;
  }

 private:
  alignas(T) unsigned char buf_[sizeof(T)];  // NOLINT(*-avoid-c-arrays)
};
// NOLINTEND(*-pro-type-member-init)

}  // namespace mbo::types

#endif  // MBO_TYPES_NO_DESTRUCT_H_
