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

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "absl/container/node_hash_map.h"
#include "absl/functional/function_ref.h"
#include "absl/status/status.h"
#include "mbo/types/extend.h"

namespace mbo::mope {

// MOPE: Mope Over Pump Ends - Is a simple templating system.
//
// Background: Pump.py (Pretty Useful for Meta Programming) is a templating system that allows to expand generic code
// mostly using simple for-loops and conditions. Its drawback is that it is written in Python and that it does not
// support structureed/hierarchical configuration. While moping over possible solutions, the idea came up to implement
// just the necessary dynamic pices combined with a structural templating system.
//
// While more dynamic features might be added in the future, it is expressily not a goal to become turing complete.
// There are many good choices available if that is necessary.
//
// MOPE understands single values and sections which are hierarchical dictionaries that are made up of sections and
// values.
//
// 1) A single value is identified by: '{{' <name> '}}'. The value can be set by calling `SetValue`.
//
// ```c++
// mbo::mope::Templaye mope;
// mope.SetValue("foo", "bar");
// std::string output("My {{foo}}.");
// mope.Expand(output);
// CHECK_EQ(output, "My bar.");
// ```
//
// 2) A section dictionary can be build by calling `AddSubDictionary` multiple times for the same `name` which becomes
// the section name. The section starts with '{{#' <name> '}}' and ends with '{{/' <name> '}};.
//
// ```c++
// mbo::mope::Templaye mope;
// mope.AddSectionDictinary("section")->SetValue("foo", "bar");
// mope.AddSectionDictinary("section")->SetValue("foo", "-");
// mope.AddSectionDictinary("section")->SetValue("foo", "baz");
// mope.AddSectionDictinary("other")->SetValue("many", "more");
// std::string output("My {{#section}}{{foo}}{{/section}}.");
// mope.Expand(output);
// CHECK_EQ(output, "My bar-baz.");
// ```
//
// 3) The template supports for-loops:
//
// '{{# <name> '=' <start> ';' <end> ( ';' <step> )?'}}'...'{{/' <name> '}}'
//
// * <step> is optional and defaults to 1.
// * <step> canot be set to zero.
// * The values <start>, <end> and <step> can either be a number or a name of
//   an existing section dictionary value.
// * <step> > 0: Iteration ends when the current value > <end>.
// * <step> < 0: Iteration ends when the current value < <end>.
//
// This creates an automatic 'section' with a dynamic value under <name> which
// can be accessed by '{{' <name> '}}'.
//
// ```c++
// mbo::mope::Templaye mope;
// mope.SetValue("max", "5");
// std::string output("My {{#foo=1;max;2}}{{foo}}.{{/foor}}");
// mope.Expand(output);
// CHECK_EQ(output, "My 1.3.5.");
// ```
//
// 4) The template allows to set tags from within the template. This allows to
// provide centralized configuration values for instance to for-loops.
//
// These are value tags with a configuration: '{{' <<name> '=' <value> '}}'
//
// ```c++
// mbo::mope::Templaye mope;
// std::string output(R"(
//    {{foo_start=1}}
//    {{foo_end=8}}
//    {{foo_step=2}}
//    My {{#foo=foo_start;foo_end;foo_step}}{{foo}}.{{/foor}})"R);
// mope.Expand(output);
// CHECK_EQ(output, "My 1.3.5.7.");
// ```
class Template {
 public:
  explicit Template() = default;

  // Sets template variable `name` to `value`.
  // The function by default only inserts new values, but if `allow_update` is true, it will also
  // overwrite existing values. It will however fail if an existing value is not a matching value type.
  // Returns whether `name` was newly added (or overwritten).
  absl::Status SetValue(std::string_view name, std::string_view value, bool allow_update = false);

  // Add a sub-dictionary under `name`.
  // The purpose of a sub-dictionary is to be filled, which can be done by calling `SetValue` or `AddSubDictionary`, on
  // the returned value (so it must be used to do so).
  // If the function is called repetedly with the same `name`, then each time a new dictionary for that `name` will be
  // added.
  [[nodiscard]] Template* AddSectionDictionary(std::string_view name);

  // Expands the template `output` in-place.
  [[nodiscard]] absl::Status Expand(std::string& output);

 private:
  enum class TagType {
    kValue,
    kSection,
    kControl,
  };

  struct TagInfo : mbo::types::Extend<TagInfo> {
    std::string name;
    std::string start;
    std::string end;
    std::string config;
    TagType type = TagType::kValue;
  };

  template<class DataType>
  struct TagData : mbo::types::Extend<TagData<DataType>> {
    const TagInfo tag;
    DataType data;
  };

  using SectionDictionary = std::vector<Template>;

  struct Range : mbo::types::Extend<Range> {
    int start = 0;
    int end = 0;
    int step = 1;
    mutable bool expanding = false;
    mutable int curr = 0;
  };

  struct RangeData : mbo::types::Extend<RangeData> {
    std::string start;
    std::string end;
    std::string step;
  };

  // Type `Data` holds all possible information variants. Each of these needs
  // to have a matching `Expand(const TagInfo<Data-Type>&, std::string*)`.
  using Data = std::variant<TagData<SectionDictionary>, TagData<Range>, TagData<std::string>>;

  static std::optional<const Template::TagInfo> FindAndConsumeTag(std::string_view* pos, bool configured_only);

  absl::Status RemoveTags(std::string& output);
  absl::Status MaybeLookup(const TagInfo& tag_info, std::string_view data, int& value) const;
  absl::Status ExpandRangeTag(const TagInfo& tag, Range& range, std::string& output);
  absl::Status ExpandRangeData(const TagInfo& tag, const RangeData& range_data, std::string& output);
  absl::Status ExpandConfiguredTag(const TagInfo& tag, std::string& output);
  absl::Status ExpandTags(
      bool configured_only,
      std::string& output,
      absl::FunctionRef<absl::Status(const TagInfo& tag, std::string&)> func);

  absl::Status Expand(TagData<SectionDictionary>& tag, std::string& output);
  static absl::Status Expand(const TagData<Range>& tag, std::string& output);
  absl::Status Expand(const TagData<std::string>& tag, std::string& output) const;

  template<typename Sink>
  friend void AbslStringify(Sink& sink, const TagType& value);

  absl::node_hash_map<std::string, Data> data_ = {};
};

}  // namespace mbo::mope
