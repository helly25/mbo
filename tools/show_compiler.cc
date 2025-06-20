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

#include <iostream>

int main() {
#if defined(__GNUC__) && !defined(__clang__)
  std::cout << "GCC: " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
  return 0;
#elif defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__) && defined(__clang_patchlevel__)
  std::cout << "Clang: " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << " ("
            << __clang_version__ << ")\n";
  return 0;
#else   // __clang__ / __GNUC__
  std::cerr << "Unknown compiler!";
  return 1;
#endif  // __clang__ / __GNUC__
}
