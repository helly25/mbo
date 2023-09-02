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

#ifndef MBO_FILE_INI_INI_FILE_H_
#define MBO_FILE_INI_INI_FILE_H_

#include <string_view>
#include <type_traits>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"

namespace mbo::file {

// A simple INI file class.
//
// See https://en.wikipedia.org/wiki/INI_file
//
// Comments start with a ';' or a '#' and can only be preceeded by whitespace.
//
// Groups consist of plain strings. Anything is allowed (including whitespace), but outer whitespace
// will be removed and group lines must start with '[' and end with ']'.
//
// Keys and values have their outer whitespace stipped.
//
class IniFile {
 public:
  struct GroupKey {
    std::string_view group;
    std::string_view key;
  };

  static absl::StatusOr<IniFile> Read(std::string_view filename);

  static IniFile NewEmpty() { return {}; }

  static GroupKey Clean(const GroupKey& group_key) {
    return {
        .group = absl::StripAsciiWhitespace(group_key.group),
        .key = absl::StripAsciiWhitespace(group_key.key),
    };
  }

  bool HasKey(const GroupKey& group_key) const {
    const auto [group, key] = Clean(group_key);
    const auto group_it = data_.find(group);
    return group_it != data_.end() && group_it->second.contains(key);
  }

  absl::StatusOr<std::string> GetKeyOrStatus(const GroupKey& group_key) const;

  std::string GetKeyOrDefault(const GroupKey& group_key, std::string_view default_value = "") const {
    const auto [group, key] = Clean(group_key);
    const auto group_it = data_.find(group);
    if (group_it == data_.end()) {
      return std::string(default_value);
    }
    const auto value_it = group_it->second.find(key);
    if (value_it == group_it->second.end()) {
      return std::string(default_value);
    }
    return value_it->second;
  }

  void SetKey(const GroupKey& group_key, std::string&& new_value = "") {
    const auto [group, key] = Clean(group_key);
    data_[group][key].assign(std::forward<std::string>(new_value));
  }

  template<
      typename T,
      typename = std::enable_if_t<!std::is_same_v<T, std::string>>,
      typename = std::void_t<decltype(absl::StrCat(std::declval<const T&>()))>>
  void SetKey(const GroupKey& group_key, const T& new_value = "") {
    SetKey(group_key, absl::StrCat(new_value));
  }

  bool empty() const { return data_.empty(); }  // NOLINT(readability-identifier-naming)

  std::size_t size() const;  // NOLINT(readability-identifier-naming)

  absl::Status Write(std::string_view filename) const;

 private:
  IniFile() = default;

  absl::btree_map<std::string, absl::btree_map<std::string, std::string>> data_;
};

}  // namespace mbo::file

#endif  // MBO_FILE_INI_INI_FILE_H_
