// Copyright 2024 M. Boerger (helly25.com)
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

#include <atomic>
#include <cstdint>

#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "mbo/thread/periodic_thread.h"

void Test() {
  static constexpr uint64_t kMaxCycle = 9'999;
  static constexpr absl::Duration kInterval = absl::Milliseconds(100);
  absl::Time start;
  absl::Duration total_correction;
  std::atomic_size_t cycle = 0;
  mbo::thread::PeriodicThread periodic_thread({
      .interval = kInterval,
      .initial_wait = kInterval,
      .func =
          [&] {
            const absl::Time now = absl::Now();
            if (cycle == 0) {
              start = now;
              const std::string time = absl::FormatTime("%Y-%m-%d at %H:%M:%E6S", start, absl::LocalTimeZone());
              absl::PrintF("[%04d]: %s %13s  %12s %13s %12s\n", cycle, time, "duration", "average", "correction", "avg-corr");
            } else {
              const std::size_t this_cycle = cycle;
              const absl::Duration dur = now - start;
              const absl::Duration avg = dur / this_cycle;
              const double dur_sec = absl::ToDoubleSeconds(dur);
              const double avg_sec = absl::ToDoubleSeconds(avg);
              const absl::Duration correction = avg - kInterval;
              total_correction += correction;
              const std::string time_str = absl::FormatTime("%Y-%m-%d at %H:%M:%E6S", now, absl::LocalTimeZone());
              const std::string corr_str = absl::FormatDuration(correction);
              const std::string avg_corr_str = absl::FormatDuration(total_correction / this_cycle);
              absl::PrintF(
                  "[%04d]: %s %+13.6f  ~%11.9f %13s %12s\n", this_cycle, time_str, dur_sec, avg_sec, corr_str,
                  avg_corr_str);
            }
            return ++cycle <= kMaxCycle;
          },
  });
  periodic_thread.Join();
}

int main() {
  Test();
  absl::PrintF("All done!\n");
  return 0;
}
