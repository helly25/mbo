// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0
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

#include "mbo/log/log_timing.h"

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/time/time.h"

ABSL_FLAG(
    absl::Duration,
    log_timing_min_duration,
    absl::Seconds(2),
    "The minimum duration for a `LogTiming` statement to actually be logged.");

ABSL_FLAG(
    absl::LogSeverity,
    log_timing_min_severity_always,
    absl::LogSeverity::kError,
    "The minimum severity at which the duration will be ignored.");

namespace mbo::log::log_internal {

std::string_view StripFunctionName(std::string_view function) {
  if (auto pos = function.rfind(')'); pos != std::string_view::npos) {
    for (std::size_t level = 0; pos; --pos) {
      if (function[pos] == ')') {
        ++level;
      } else if (function[pos] == '(') {
        --level;
        if (!level) {
          static constexpr std::string_view kConversion = "operator()";
          if (function.substr(0, pos + 2).ends_with(kConversion)) {
            function.remove_suffix(function.length() - (pos + 2));
            pos -= kConversion.length();
            continue;
          }
          break;
        }
      }
    }
    function.remove_suffix(function.length() - pos);
    if (!function.empty()) {
      pos = function.length() - 1;
      for (std::size_t level = 0; pos; --pos) {
        if (function[pos] == ')') {
          ++level;
        } else if (function[pos] == '(') {
          --level;
        } else if (!level && function[pos] == ' ') {
          function.remove_prefix(pos + 1);
          break;
        }
      }
    }
  }
  return function;
}

void LogTimingImpl::Log() const {
  const absl::Duration min_duration = args_.min_duration.value_or(absl::GetFlag(FLAGS_log_timing_min_duration));
  const absl::Duration duration = absl::Now() - args_.start_time;
  if (duration < min_duration && args_.severity < absl::GetFlag(FLAGS_log_timing_min_severity_always)) {
    return;
  }

  LOG(LEVEL(args_.severity)).AtLocation(args_.src.file_name(), args_.src.line())
      << "LogTiming(" << duration << " @ " << StripFunctionName(args_.src.function_name()) << ")"
      << (message_.empty() ? "" : ": ") << message_;
}

}  // namespace mbo::log::log_internal
