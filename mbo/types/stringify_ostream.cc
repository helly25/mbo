// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
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

#include <memory>

#include "absl/synchronization/mutex.h"
#include "mbo/types/stringify.h"

namespace mbo::types {
namespace {

absl::Mutex g_mx(absl::kConstInit);                      // NOLINT(*-avoid-non-const-global-variables)
std::shared_ptr<const Stringify> g_stringify = nullptr;  // NOLINT(*-avoid-non-const-global-variables)

}  // namespace

namespace types_internal {

std::shared_ptr<const Stringify> GetStringifyForOstream() {
  absl::MutexLock lock(&g_mx);
  if (g_stringify == nullptr) {
    g_stringify = std::make_shared<Stringify>();
  }
  return g_stringify;
}

}  // namespace types_internal

void SetStringifyOstreamOutputMode(Stringify::OutputMode output_mode) {
  absl::MutexLock lock(&g_mx);
  g_stringify.reset(new Stringify(output_mode));  // NOLINT
}

void SetStringifyOstreamOptions(const StringifyOptions& options) {
  absl::MutexLock lock(&g_mx);
  g_stringify.reset(new Stringify(options));  // NOLINT
}

}  // namespace mbo::types
