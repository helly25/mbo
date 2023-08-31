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

#include "mbo/mope/mope.h"

#include <string>
#include <string_view>
#include <utility>

#include "absl/cleanup/cleanup.h"
#include "absl/log/absl_log.h"  // IWYU pragma: keep
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "mbo/status/status_macros.h"
#include "re2/re2.h"

namespace mbo::mope {
namespace {

std::pair<std::size_t, std::size_t> ExpandWhiteSpace(
    std::string_view output,
    std::size_t tag_pos,
    std::size_t tag_len) {
  std::size_t end = tag_pos + tag_len;
  // Is the tag followed by a new-line?
  if (end < output.size() && (output[end] == '\n' || output[end] == '\r')) {
    ++end;
    std::size_t pos = tag_pos;
    // reverse past all space
    while (pos > 0 && (output[pos - 1] == ' ' || output[pos - 1] == '\t')) {
      --pos;
    }
    if (pos == 0 || output[pos - 1] == '\n' || output[pos - 1] == '\r') {
      // Only preceded by whitespace and followed directly by a new-line.
      // Must we keep very last new-line, otherwise the result is not a text
      // file.
      if (end == output.length()) {
        return {pos, tag_pos - pos + tag_len};
      } else {
        return {pos, tag_pos - pos + tag_len + 1};
      }
    }
  }
  return {tag_pos, tag_len};
}

// NOxLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::pair<std::size_t, std::size_t> FindTag(std::string_view output, std::string_view tag) {
  const std::size_t tag_pos = output.find(tag);
  if (tag_pos == std::string::npos) {
    return {tag_pos, 0};
  }
  return ExpandWhiteSpace(output, tag_pos, tag.length());
}

}  // namespace

template<typename Sink>
void AbslStringify(Sink& sink, const Template::TagType& value) {
  absl::Format(&sink, "%s", [value] {
    switch (value) {
      case Template::TagType::kValue: return "TagType::Value";
      case Template::TagType::kSection: return "TagType::Section";
      case Template::TagType::kControl: return "TagType::Control";
    }
    return "TagType::UNKNOWN";
  }());
}

Template* Template::AddSectionDictionary(std::string_view name) {
  TagInfo tag{
      .name{name},
      .start = absl::StrCat("{{#", name, "}}"),
      .end = absl::StrCat("{{/", name, "}}"),
      .type = TagType::kSection,
  };
  auto [it, inserted] = data_.emplace(name, TagData<SectionDictionary>{.tag = std::move(tag), .data = {}});
  auto& dict = std::get<TagData<SectionDictionary>>(it->second).data;
  return &dict.emplace_back();
}

absl::Status Template::SetValue(std::string_view name, std::string_view value, bool allow_update) {
  TagInfo tag{
      .name{name},
      .start = absl::StrCat("{{", name, "}}"),
      .type = TagType::kValue,
  };
  auto [it, inserted] = data_.emplace(name, TagData<std::string>{.tag = std::move(tag), .data{value}});
  if (inserted) {
    return absl::OkStatus();
  }
  if (allow_update) {
    auto* data_str = std::get_if<TagData<std::string>>(&it->second);
    if (data_str == nullptr) {
      return absl::AlreadyExistsError(absl::StrCat("A value for '", name, "' already exists with a different type."));
    }
    data_str->data.assign(value);
    return absl::OkStatus();
  }
  return absl::AlreadyExistsError(absl::StrCat("A value for '", name, "' already exists."));
}

std::optional<const Template::TagInfo> Template::FindAndConsumeTag(std::string_view* pos, bool configured_only) {
  // Grab parts as string_view. Done in a separate block so these cannot escape.
  // They will be invalid upon the first change of `output`.
  static constexpr LazyRE2 kNormalTags = {"({{(#?)(\\w+)}})"};
  static constexpr LazyRE2 kConfiguredTags = {"({{(#?)(\\w+)(?:=((?:[^}]|[}][^}])+))?}})"};
  const RE2* regex = configured_only ? &*kConfiguredTags : &*kNormalTags;
  std::string_view start;
  std::string_view name;
  std::string_view config;
  std::string_view type;
  if (!RE2::FindAndConsume(pos, *regex, &start, &type, &name, &config)) {
    return std::nullopt;
  }
  return TagInfo{
      .name{name},
      .start{start},
      .end = type.empty() ? "" : absl::StrCat("{{/", name, "}}"),
      .config{config},
      .type = type.empty() ? (config.empty() ? TagType::kValue : TagType::kControl) : TagType::kSection,
  };
}

absl::Status Template::ExpandTags(
    bool configured_only,
    std::string& output,
    absl::FunctionRef<absl::Status(const TagInfo&, std::string&)> func) {
  std::string_view pos = output;
  while (true) {
    if (pos.empty()) {
      return absl::OkStatus();
    }
    std::optional<TagInfo> result = FindAndConsumeTag(&pos, configured_only);
    if (result == std::nullopt) {
      return absl::OkStatus();
    }
    const TagInfo tag = *std::move(result);
    if (tag.type == TagType::kSection && data_.find(tag.name) != data_.end()) {
      return absl::InvalidArgumentError(absl::StrCat("Tag name '", tag.name, "' appears twice."));
    }
    const std::size_t tag_pos = pos.data() - output.c_str() - tag.start.length();
    const auto [replace_pos, replace_tag_len] = ExpandWhiteSpace(output, tag_pos, tag.start.length());
    std::string replace_str;
    std::size_t replace_len = 0;
    switch (tag.type) {
      case TagType::kControl: {
        replace_len = replace_tag_len;
        MBO_STATUS_RETURN_IF_ERROR(func(tag, replace_str));
        break;
      }
      case TagType::kSection: {
        const auto tag_end_pos = output.find(tag.end, replace_pos + replace_tag_len);
        if (tag_end_pos == std::string_view::npos) {
          return absl::InvalidArgumentError(absl::StrCat("Tag name '", tag.name, "' has no end tag '", tag.end, "'."));
        }
        const auto [replace_end, replace_end_len] = ExpandWhiteSpace(output, tag_end_pos, tag.end.length());
        replace_len = replace_end + replace_end_len - replace_pos;  // whole replace incl. tags
        replace_str = output.substr(replace_pos + replace_tag_len, replace_len - replace_tag_len - replace_end_len);
        MBO_STATUS_RETURN_IF_ERROR(func(tag, replace_str));
        break;
      }
      case TagType::kValue: {
        const auto it = data_.find(tag.name);
        if (it != data_.end() && std::get_if<TagData<std::string>>(&it->second) != nullptr) {
          replace_len = replace_tag_len;
          replace_str = std::get<TagData<std::string>>(it->second).data;
        } else {
          pos = output;
          pos.remove_prefix(replace_pos + replace_tag_len);
          continue;
        }
        break;
      }
    }
    output.erase(replace_pos, replace_len);
    output.insert(replace_pos, replace_str);
    pos = output;
    pos.remove_prefix(replace_pos + replace_str.length());
  }
}

absl::Status Template::MaybeLookup(const TagInfo& tag_info, std::string_view data, std::string& value) const {
  if (data.empty()) {
    return absl::OkStatus();
  }
  if ((data.starts_with('"') && data.ends_with('"')) || (data.starts_with('\'') && data.ends_with('\''))) {
    data.remove_prefix(1);
    data.remove_suffix(1);
    value = data;
    return absl::OkStatus();
  }
  auto data_it = data_.find(data);
  if (data_it == data_.end()) {
    return absl::NotFoundError(absl::StrCat("Tag '", tag_info.name, "' references '", data, "' which was not found."));
  }
  const auto& found = data_it->second;
  if (const auto* tag_data = std::get_if<TagData<std::string>>(&found)) {
    value = tag_data->data;
    return absl::OkStatus();
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Tag '", tag_info.name, "' refrences '", data, "' which has unsupported data type."));
}

absl::Status Template::MaybeLookup(const TagInfo& tag_info, std::string_view data, int& value) const {
  if (data.empty() || absl::SimpleAtoi(data, &value)) {
    return absl::OkStatus();
  }
  auto data_it = data_.find(data);
  if (data_it == data_.end()) {
    return absl::NotFoundError(absl::StrCat("Tag '", tag_info.name, "' references '", data, "' which was not found."));
  }
  const auto& found = data_it->second;
  if (const auto* tag_data = std::get_if<TagData<std::string>>(&found)) {
    if (absl::SimpleAtoi(tag_data->data, &value)) {
      return absl::OkStatus();
    }
    return absl::InvalidArgumentError(absl::StrCat(
        "Tag '", tag_info.name, "' refrences '", data, "' which has non numeric value '", tag_data->data, "'"));
  } else if (const auto* tag_data = std::get_if<TagData<Range>>(&found)) {
    if (tag_data->data.expanding) {
      value = tag_data->data.curr;
      return absl::OkStatus();
    }
    return absl::InvalidArgumentError(
        absl::StrCat("Tag '", tag_info.name, "' refrences '", data, "' which is not being expanded."));
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Tag '", tag_info.name, "' refrences '", data, "' which has unsupported data type."));
}

absl::Status Template::ExpandRangeTag(const TagInfo& /*tag*/, Range& range, std::string& output) {
  const std::string original = std::move(output);
  output = "";
  if (range.step == 0) {
    return absl::InternalError(absl::StrCat("A range should never have step == 0."));
  }
  range.expanding = true;
  const absl::Cleanup cleanup = [&range] { range.expanding = false; };
  for (range.curr = range.start; range.step > 0 ? range.curr <= range.end : range.curr >= range.end;
       range.curr += range.step) {
    if (!range.join.empty() && range.curr != range.start) {
      output.append(range.join);
    }
    std::string expanded = original;
    MBO_STATUS_RETURN_IF_ERROR(Expand(expanded));
    output.append(expanded);
  }
  return absl::OkStatus();
}

absl::Status Template::ExpandRangeData(const TagInfo& tag, const RangeData& range_data, std::string& output) {
  Range range;
  MBO_STATUS_RETURN_IF_ERROR(MaybeLookup(tag, range_data.start, range.start));
  MBO_STATUS_RETURN_IF_ERROR(MaybeLookup(tag, range_data.end, range.end));
  MBO_STATUS_RETURN_IF_ERROR(MaybeLookup(tag, range_data.step, range.step));
  MBO_STATUS_RETURN_IF_ERROR(MaybeLookup(tag, range_data.join, range.join));
  if (range.step == 0) {
    return absl::InvalidArgumentError(absl::StrCat("Tag '", tag.name, "' cannot have step == 0."));
  }
  // We could check:
  // * range.step > 0 && range.start > range.end
  // * range.step < 0 && range.start < range.end
  // But that would not work well with values that were looked up or computed.
  auto [r_tag_it, inserted] = data_.emplace(tag.name, TagData<Range>{.tag = tag, .data = range});
  if (!inserted) {
    return absl::InvalidArgumentError(absl::StrCat("Tag '", tag.name, "' appears twice."));
  }
  auto& tag_range = std::get<TagData<Range>>(r_tag_it->second).data;
  auto result = ExpandRangeTag(tag, tag_range, output);
  data_.erase(tag.name);
  return result;
}

absl::StatusOr<std::vector<std::string>> Template::ParseStringList(std::string_view data) {
  std::vector<std::string> result;
  std::string curr;
  bool quote = false;
  for (std::size_t pos = 0; pos < data.size(); ++pos) {
    char chr = data[pos];
    switch (chr) {
      default: break;  // normal character, just add.
      case ',':
        if (!quote) {
          // A Split!
          result.push_back(curr);
          curr.clear();
          if (pos + 1 == data.size()) {
            result.emplace_back("");
            return absl::OkStatus();
          }
          continue;
        }
        break;
      case '"': quote = !quote; break;
      case '\\': {
        if (++pos == data.size()) {
          return absl::InvalidArgumentError("StringList ends in '\\'.");
        }
        chr = data[pos];
        switch (chr) {
          case '{':
          case '}':
          case ',':  // CUSTOM all other once follow
                     // https://en.cppreference.com/w/cpp/language/escape
          case '\'':
          case '"':
          case '?':
          case '\\': curr += chr; continue;
          case 'a': curr += '\a'; continue;
          case 'b': curr += '\b'; continue;
          case 'f': curr += '\f'; continue;
          case 'n': curr += '\n'; continue;
          case 'r': curr += '\r'; continue;
          case 't': curr += '\t'; continue;
          case 'v': curr += '\v'; continue;
          case '0':  // octal
          case '1':  // octal
          case '2':  // octal
          case '3':  // octal
          case '4':  // octal
          case '5':  // octal
          case '6':  // octal
          case '7':  // octal
          case 'x':  // \x{n...} C++23: hex; without {} is pre-C++23
          case 'o':  // \o{n...} C++23: octal 1..3
          case 'u':  // \u{nn..}: unicode 4 hex C++23
          case 'U':  // Unicode 8-hex
          case 'N':  // \N{Name}: Named unicode char
            return absl::UnimplementedError("StringList has not yet supported escape sequence.");
          default: return absl::InvalidArgumentError("StringList has unsupported escape sequence.");
        }  // switch for escape
      }    // case escape
    }      // switch char
    curr += chr;
  }
  result.push_back(curr);
  return result;
}

absl::Status Template::ExpandConfiguredSection(
    std::string_view name,
    std::vector<std::string> str_list,  // NOLINT(performance-unnecessary-value-param): Incorrect, we move strings out.
    std::string& output) {
  const std::string original(std::move(output));
  output = "";
  for (auto& str : str_list) {
    MBO_STATUS_RETURN_IF_ERROR(SetValue(name, std::move(str), true));
    std::string result(original);
    MBO_STATUS_RETURN_IF_ERROR(Expand(result));
    output.append(result);
  }
  return absl::OkStatus();
}

absl::Status Template::ExpandConfiguredList(const TagInfo& tag, std::string_view str_list_data, std::string& output) {
  MBO_STATUS_ASSIGN_OR_RETURN(auto str_list, ParseStringList(str_list_data));
  data_.erase(tag.name);  // Cannot be present.
  // CONSIDER: A specialized type would make this faster. But also less generic
  // and thus complicate extensions.
  MBO_STATUS_RETURN_IF_ERROR(ExpandConfiguredSection(tag.name, std::move(str_list), output));
  data_.erase(tag.name);  // Remove, cannot be left over.
  return absl::OkStatus();
}

absl::Status Template::ExpandConfiguredTag(const TagInfo& tag, std::string& output) {
  // Parse tag config and translate into whatever that says...
  switch (tag.type) {
    case TagType::kSection: {
      static constexpr LazyRE2 kReFor = {
          R"(\s*(-?\d+|[_a-zA-Z]\w*)\s*;\s*(-?\d+|[_a-zA-Z]\w*)\s*(?:;\s*(|-?\d+|[_a-zA-Z]\w*)\s*(?:;([^;]*))?)?)"};
      RangeData range;
      if (RE2::FullMatch(tag.config, *kReFor, &range.start, &range.end, &range.step, &range.join)) {
        return ExpandRangeData(tag, range, output);
      }
      if (tag.config.starts_with('[') && tag.config.ends_with(']')) {
        std::string_view str_list_data(tag.config);
        str_list_data.remove_prefix(1);
        str_list_data.remove_suffix(1);
        return ExpandConfiguredList(tag, str_list_data, output);
      }
      break;
    }
    case TagType::kControl: return SetValue(tag.name, tag.config, true);
    case TagType::kValue: break;  // Unexpected. Ignore.
  }
  return absl::UnimplementedError(absl::StrCat("Tag '", tag.name, "' has unknown config format '", tag.config, "'."));
}

absl::Status Template::RemoveTags(std::string& output) {
  return ExpandTags(false, output, [](const TagInfo&, std::string& output) {
    output.clear();
    return absl::OkStatus();
  });
}

absl::Status Template::Expand(std::string& output) {
  MBO_STATUS_RETURN_IF_ERROR(
      ExpandTags(true, output, [this](const TagInfo& tag, std::string& out) { return ExpandConfiguredTag(tag, out); }));
  for (auto& [name, data] : data_) {
    MBO_STATUS_RETURN_IF_ERROR(std::visit([&](auto& typed_data) { return Expand(typed_data, output); }, data));
  }
  return RemoveTags(output);
}

absl::Status Template::Expand(TagData<SectionDictionary>& tag, std::string& output) {
  if (absl::StrContains(output, absl::StrCat("{{", tag.tag.name, "}}"))) {
    return absl::InvalidArgumentError(absl::StrCat("Simple tag '", tag.tag.name, "' used in SectionDictionary."));
  }
  while (true) {
    const auto [pos_start, start_len] = FindTag(output, tag.tag.start);
    const auto [pos_end, end_len] = FindTag(output, tag.tag.end);
    if (pos_start == std::string::npos) {
      if (pos_end != std::string::npos) {
        return absl::InvalidArgumentError(absl::StrCat("Section '", tag.tag.name, "' has end but no start marker."));
      }
      break;
    }
    if (pos_end == std::string::npos) {
      return absl::InvalidArgumentError(absl::StrCat("Section '", tag.tag.name, "' has start but no end marker."));
    }
    if (pos_end < pos_start) {
      return absl::InvalidArgumentError(absl::StrCat("Section '", tag.tag.name, "' has end before start marker."));
    }
    const std::size_t tmpl_len = pos_end - pos_start - start_len;
    std::size_t pos_insert = pos_end + end_len;
    for (auto& section : tag.data) {
      std::string value = output.substr(pos_start + start_len, tmpl_len);  // We still hold the "input" part.
      MBO_STATUS_RETURN_IF_ERROR(section.Expand(value));
      output.insert(pos_insert, value);
      pos_insert += value.length();
    }
    output.replace(pos_start, tmpl_len + start_len + end_len, "");  // Now delete "input".
  }
  return absl::OkStatus();
}

absl::Status Template::Expand(const TagData<Range>& tag, std::string& output) {
  // Unlike when expanding sections, here the tag is both the section and the value.
  if (!tag.data.expanding) {
    return absl::InvalidArgumentError(absl::StrCat("Tag '", tag.tag.name, "' cannot be expanded."));
  }
  std::string tag_replace = absl::StrCat("{{", tag.tag.name, "}}");
  std::string value = absl::StrCat(tag.data.curr);
  absl::StrReplaceAll({{tag_replace, value}}, &output);
  return absl::OkStatus();
}

absl::Status Template::Expand(const TagData<std::string>& tag, std::string& output) {
  absl::StrReplaceAll({{tag.tag.start, tag.data}}, &output);
  return absl::OkStatus();
}

}  // namespace mbo::mope
