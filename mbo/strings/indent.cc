
#include "mbo/strings/indent.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"

namespace mbo::strings {

std::vector<std::string_view> DropIndentAndSplit(std::string_view text) {
  // If the first line is empty, simply remove it and the new first line will
  // determine the indent to remove. Otherwise the first line will be left alone
  // and the second line will determine the indent to remove.
  // On the last line we remove any indent unconditionally.
  const size_t start = absl::ConsumePrefix(&text, "\n") ? 0 : 1;

  std::vector<std::string_view> lines = absl::StrSplit(text, '\n');
  if (lines.size() > start) {
    size_t pos = lines[start].find_first_not_of(" \t");
    if (pos == std::string_view::npos) {
      pos = lines[start].size();
    }
    const std::string prefix(lines[start], 0, pos);
    // std::cout << "First(" << start << "): <" << lines[start] << ">\n";
    // std::cout << "Prefix: <" << prefix << ">\n";

    if (lines.back().find_first_not_of(" \t") == std::string::npos) {
      lines.back() = "";
    }
    for (size_t i = start; i < lines.size(); ++i) {
      absl::ConsumePrefix(&lines[i], prefix);
    }
  }
  return lines;
}

std::string DropIndent(std::string_view text) {
  if (text.empty() || text == "\n") {
    return std::string(text);
  }
  return absl::StrJoin(DropIndentAndSplit(text), "\n");
}

}  // namespace mbo::strings
