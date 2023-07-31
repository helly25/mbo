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

#ifndef MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
#define MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_

#include <string_view>
#include <type_traits>

#include "absl/strings/match.h"
#include "absl/strings/strip.h"
#include "absl/types/span.h"
#include "mbo/types/internal/decompose_count.h"

// IWYU pragma: private, include "mbo/types/traits.h"

namespace mbo::types::types_internal {

#ifdef __clang__

static constexpr bool kStructNameSupport = true;

template<typename T>
class StructMeta {
 private:
  static constexpr std::size_t kMaxFieldNameLength = 64;
  static constexpr std::size_t kNumFields = DecomposeCountImpl<T>::value;

  struct FieldStorage {
    char name[kMaxFieldNameLength]{0};  // NOLINT(*-c-arrays)
  };

 public:
  static absl::Span<const std::string_view> GetNames(const T& v) {
    if (!initialized) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
      __builtin_dump_struct(&v, &StructMeta<T>::DumpStructVisitor);
      initialized = true;
    }
    return absl::MakeConstSpan(field_names);
  }

  // NOLINTNEXTLINE(cert-dcl50-cpp)
  static int DumpStructVisitor(std::string_view format, ...) {
    static std::size_t field_index = 0;
    // NOLINTBEGIN(*-array-to-pointer-decay,*-avoid-c-arrays,*-magic-numbers,*-no-array-decay,*-vararg)
    if (field_index >= kNumFields || !absl::StrContains(format, '=')) {
      va_list vap = nullptr;
      va_start(vap, format);
      va_end(vap);
      return 0;  // Not a field.
    }
    auto& field = field_storage[field_index];
    if (format.starts_with("%s%s %s = %")) {
      va_list vap = nullptr;
      va_start(vap, format);
      const char* indent = va_arg(vap, const char*);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      if (indent[0] != ' ' || indent[1] != ' ' || indent[2] != '\0') {
        va_end(vap);
        return 0;
      }
      (void)va_arg(vap, const char*); // type
      const char* name = va_arg(vap, const char*);
      // index 3 is the init value
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      field_names[field_index] = std::string_view(name);

      va_end(vap);      
    } else {
      std::string_view line;
      char buf[1024 * 10];
      va_list vap = nullptr;
      va_start(vap, format);
      const int len = vsnprintf(buf, sizeof(buf), format.data(), vap);
      va_end(vap);

      line = std::string_view(buf, len);

      if (!absl::ConsumePrefix(&line, "  ") || line[0] == ' ') {
        return 0;  // Not the correct level or type of info.
      }
      line.remove_suffix(line.length() - line.find('='));
      line = absl::StripTrailingAsciiWhitespace(line);
      // Now we have: <type> ' ' <field_name>
      // The field_name cannot have a space, so we cut there.
      auto pos = line.find_last_of(' ');
      if (pos != std::string_view::npos) {
        line.remove_prefix(pos + 1);
      }

      // Copy what we found
      auto copy_len = std::min(line.length(), kMaxFieldNameLength - 1);
      std::strncpy(field.name, line.data(), copy_len);
      field.name[kMaxFieldNameLength - 1] = '\0';
      field_names[field_index] = std::string_view(field.name, copy_len);
    }
    // NOLINTEND(*-array-to-pointer-decay,*-avoid-c-arrays,*-magic-numbers,*-no-array-decay,*-vararg)

    ++field_index;
    return 0;
  }

  // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
  inline static std::array<FieldStorage, kNumFields> field_storage;
  inline static std::array<std::string_view, kNumFields> field_names;
  inline static bool initialized = false;
  // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
};

template<typename T>
inline absl::Span<const std::string_view> GetFieldNames(const T& v) {
  return StructMeta<T>::GetNames(v);
}

#else  // __clang__

static constexpr bool kStructNameSupport = false;

#endif  // __clang__

}  // namespace mbo::types::types_internal

#endif  // MBO_TYPES_INTERNAL_STRUCT_NAMES_CLANG_H_
