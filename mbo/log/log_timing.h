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

#ifndef MBO_LOG_LOG_TIMING_H_
#define MBO_LOG_LOG_TIMING_H_

#include <optional>
#include <source_location>
#include <string>
#include <string_view>

#include "absl/log/log.h"  // IWYU pragma: keep
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"

namespace mbo::log {
namespace log_internal {

class LogTimingImpl final {
 public:
  struct LogTimingArgs {
    absl::LogSeverity severity = absl::LogSeverity::kInfo;
    const std::optional<absl::Duration> min_duration = std::nullopt;
    const absl::Time start_time = absl::Now();
    const std::source_location src = std::source_location::current();
  };

  explicit LogTimingImpl(const LogTimingArgs& args) : args_(args) {}

  LogTimingImpl() = delete;

  LogTimingImpl(const LogTimingImpl&) = delete;
  LogTimingImpl& operator=(const LogTimingImpl&) = delete;
  LogTimingImpl(LogTimingImpl&& other) noexcept = default;
  LogTimingImpl& operator=(LogTimingImpl&&) = delete;

  template<typename T>
  LogTimingImpl&& operator<<(T&& arg) && {
    absl::StrAppend(&message_, arg);
    return std::move(*this);
  }

  ~LogTimingImpl() {
    if (log_once_) {
      Log();
    }
  }

 private:
  class Once final {
   public:
    ~Once() noexcept = default;
    Once() noexcept = default;
    Once(const Once&) = delete;
    Once& operator=(const Once&) = delete;

    Once(Once&& other) noexcept { other.once_ = false; }

    Once& operator=(Once&&) = delete;

    explicit operator bool() const noexcept { return once_; }

   private:
    bool once_ = true;
  };

  void Log() const;

  Once log_once_;
  std::string message_ = "";
  const LogTimingArgs args_;
};

std::string_view StripFunctionName(std::string_view function);

}  // namespace log_internal

// A Simple timing logger that logs the time taken in its scope.
//
// The function creates an RAII object that captures the location and start time of the caller.
// Upon completion of the scope it logs the time spent between the scope's creation and termination.
//
// Usage:
//   auto done = LogTiming() << "Some log message";
//
// An optional `min_duration` can be specified to suppress fast scopes from being logged.
//
//   auto done  = LogTiming({.min_duration = absl::Seconds(10)});
inline log_internal::LogTimingImpl LogTiming(const log_internal::LogTimingImpl::LogTimingArgs& args = {}) {
  return log_internal::LogTimingImpl(args);
}

}  // namespace mbo::log

#endif  // MBO_LOG_LOG_TIMING_H_
