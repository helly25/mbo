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

#include "mbo/file/ini/ini_file.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/absl_log.h"  // IWYU pragma: keep
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "mbo/file/file.h"
#include "mbo/status/status_macros.h"

namespace mbo::file {

absl::StatusOr<IniFile> IniFile::Read(std::string_view filename) {
  // TODO(helly25): Implement LineReader?
  MBO_ASSIGN_OR_RETURN(const std::string content, GetContents(filename));
  std::vector<std::string_view> lines = absl::StrSplit(content, '\n', absl::SkipEmpty());
  IniFile ini;
  std::string_view group;
  for (auto line : lines) {
    line = absl::StripAsciiWhitespace(line);
    if (line.starts_with('[') && line.ends_with(']')) {
      line.remove_prefix(1);
      line.remove_suffix(1);
      group = line;
      continue;
    }
    if (line.starts_with(';') || line.starts_with('#')) {
      continue;
    }
    std::pair<std::string_view, std::string_view> key_val = absl::StrSplit(line, absl::MaxSplits('=', 1));
    auto [key, val] = key_val;
    key = absl::StripAsciiWhitespace(key);
    val = absl::StripAsciiWhitespace(val);
    ini.SetKey({.group = group, .key = key}, val);
  }
  return ini;
}

absl::StatusOr<std::string> IniFile::GetKeyOrStatus(const GroupKey& group_key) const {
  const auto [group, key] = Clean(group_key);
  const auto group_it = data_.find(group);
  if (group_it == data_.end()) {
    return absl::NotFoundError(absl::StrCat("Group [", group, "] not found."));
  }
  const auto value_it = group_it->second.find(key);
  if (value_it == group_it->second.end()) {
    return absl::NotFoundError(absl::StrCat("Group [", group, "] has no key '", key, "'."));

  }
  return value_it->second;
}

absl::Status IniFile::Write(std::string_view filename) const {
  std::string content;
  for (const auto& [group, key_val] : data_) {
    if (!group.empty()) {
      if (!content.empty()) {
        absl::StrAppend(&content, "\n");
      }
      absl::StrAppend(&content, "[", group, "]\n");
    }
    for (const auto& [key, val] : key_val) {
      absl::StrAppend(&content, key, "=", val, "\n");
    }
  }
  return SetContents(filename, content);
}

std::size_t IniFile::size() const {
  std::size_t size = 0;
  for (const auto& [_, kvs] : data_) {
    size += kvs.size();
  }
  return size;
}

}  // namespace mbo::file
