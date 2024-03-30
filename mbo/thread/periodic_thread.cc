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

#include "mbo/thread/periodic_thread.h"

#include <atomic>
#include <thread>

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

namespace mbo::thread {

PeriodicThread::~PeriodicThread() noexcept {
  stop_ = true;
  Join();
}

PeriodicThread::PeriodicThread(Options options) noexcept : options_(std::move(options)), thread_([this] { Run(); }) {}

bool PeriodicThread::IsDone() const {
  absl::MutexLock lock(&mx_);
  return done_;
}

void PeriodicThread::Join() const {
  {
    absl::MutexLock lock(&mx_);
    mx_.Await(absl::Condition(&done_));
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void PeriodicThread::Run() {
  running_ = true;
  if (options_.initial_wait > absl::ZeroDuration()) {
    absl::SleepFor(options_.initial_wait);
  }
  std::size_t cycle = 0;
  absl::Time begin = absl::Now();
  absl::Time start = begin;
  absl::Duration adjust = absl::ZeroDuration();
  while (!stop_) {
    if (!options_.func() || stop_) {
      break;
    }
    ++cycle;
    const absl::Time end = absl::Now();
    const absl::Duration took = end - start + adjust;
    absl::Duration sleep = options_.interval >= took ? options_.interval - took : options_.min_interval;
    absl::SleepFor(sleep);
    start = absl::Now();
    // We adjust based on average divergence as well as the most recent runtime.
    // This allows for small overcorrections over time as well as handling inconsistent runtime of actual function.
    // Adjusting the average too little may result in an inability to keep to the intended interval time. Values in the
    // low percentages (1.01..1.1) appear to be working.
    // Adjusting the recent run strongly (e.g. >1) only works if callback runtimes are steady. Smaller values are
    // better at dealing with inconsisten runtimes, even for very small inconsistencies. Setting the value to 1 appears
    // to be a good compromise.
    // This is not configurable in the `Options` for now as that prevent using better algorithms later.
    static constexpr double kAdjustAverage = 1.05;
    static constexpr double kAdjustRecent = 1;
    adjust = kAdjustAverage * ((start - begin) / cycle - options_.interval) + kAdjustRecent * ((start - end) - sleep);
    static constexpr std::size_t kMaxCycleAdjustWindow = 1000;
    if (cycle % kMaxCycleAdjustWindow == 0) {
      // We actually reset the cycle to 0 as we otherwise would need to handle cycle overrun in the next window
      // `max(size_t) - cycle < kMaxCycleAdjustWindow`. Further we would need to use a modulo operation for cycle when
      // computing the adjustment (`cycle % (kMaxCycleAdjustWindow + 1)`).
      cycle = 0;
      begin = start;
    }
  }
  absl::MutexLock lock(&mx_);
  done_ = true;
}

}  // namespace mbo::thread
