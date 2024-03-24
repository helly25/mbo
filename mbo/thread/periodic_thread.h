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
#include <thread>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

namespace mbo::thread {

// The `PeriodicThread` runs a function periodically in its own thread.
//
// The implementation uses simple heuristics to adjust to the intended interval over time using a
// sliding window. This allows for inconsistent function run times to eventually lead to closely
// following the interval on average. The implementation may still not be able to respect the
// interval correctly, even over longer periods of time. However, the divergence from the intended
// interval should be very small (high accuracy). Use binary `//mbo/thread:periodic` to determine
// the accuracy for actual machines.
//
// The advantage of 'allowing' the imperfect interval handling is that the management has a low cost
// and thus the most real time can be spent sleeping.
//
// The `PeriodicThread`:
// * Automatically stops if the function returns `false`.
// * Automatically starts on creation, but an `initial_wait` time can be configured.
// * Cannot be restarted.
// * Can be stopped, but may have to sleep for a full interval time before actually stopping.
// * The destructor will wait for the thread to stop if it is running.
// * The behavior is undefined if the function runtime exceeds the interval (or does not allow time
//   for interval management).
class PeriodicThread {
 public:
  struct Options {
    absl::Duration interval;
    absl::Duration min_interval = absl::Milliseconds(1);
    absl::Duration initial_wait = absl::ZeroDuration();
    std::function<bool()> func;  // Return `true` for continue, `false` for stop.
  };

  ~PeriodicThread() noexcept;
  explicit PeriodicThread(Options options) noexcept;

  PeriodicThread(const PeriodicThread&) = delete;
  PeriodicThread& operator=(const PeriodicThread&) = delete;
  PeriodicThread(PeriodicThread&& other) = delete;
  PeriodicThread& operator=(PeriodicThread&&) = delete;

  void Stop() { stop_ = true; }

  bool IsStopping() const { return stop_; }

  bool IsRunning() const { return running_; }

  bool IsDone() const;

  void Join() const;

 private:
  void Run();

  const Options options_;
  mutable absl::Mutex mx_;
  std::atomic_bool stop_ = false;
  std::atomic_bool running_ = false;
  bool done_ ABSL_GUARDED_BY(mx_) = false;
  mutable std::thread thread_;
};

}  // namespace mbo::thread