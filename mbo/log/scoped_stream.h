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

#ifndef MBO_LOG_SCOPED_STREAM_H_
#define MBO_LOG_SCOPED_STREAM_H_

#include <iostream>
#include <source_location>
#include <sstream>

namespace mbo::log {
namespace log_internal {

template<typename OStreamT = std::ostream, typename StringStreamT = std::stringstream>
class ScopedStream {
 public:
  using OStream = OStreamT;
  using StringStream = StringStreamT;

  ScopedStream() = delete;

  explicit ScopedStream(const std::source_location& loc, OStream& out) : loc_(loc), out_(out) {}

  ~ScopedStream() {
    out_ << "[" << loc_.file_name() << ":" << loc_.line() << "] @" << loc_.function_name();
    if (str_) {
      out_ << " : " << str_.str();
    }
    out_ << "\n";
  }

  ScopedStream(const ScopedStream&) = delete;
  ScopedStream& operator=(const ScopedStream&) = delete;
  ScopedStream(ScopedStream&&) = delete;
  ScopedStream& operator=(ScopedStream&&) = delete;

  template<typename T>
  StringStream& operator<<(const T& val) {
    str_ << val;
    return str_;
  }

 private:
  friend struct ScopedStreamTestAccess;

  std::string TestGetStr() const { return std::string{str_.str()}; }

  const std::source_location loc_;
  OStream& out_;
  StringStream str_;
};

struct VoidStream {
  VoidStream() = default;
  ~VoidStream() = default;
  VoidStream(const VoidStream&) = delete;
  VoidStream& operator=(const VoidStream&) = delete;
  VoidStream(VoidStream&&) = delete;
  VoidStream& operator=(VoidStream&&) = delete;

  operator bool() const noexcept { return false; }  // NOLINT(*-explicit-*)

  // NOLINTNEXTLINE(*-identifier-naming,*-member-functions-to-static)
  constexpr std::string_view str() const noexcept { return {}; }

  template<typename T>
  VoidStream& operator<<(const T&) {
    return *this;
  }
};

}  // namespace log_internal

inline auto ScopedStreamOut(std::source_location loc = std::source_location::current()) {
  return log_internal::ScopedStream<std::ostream>(loc, std::cout);
}

template<typename Disallowed>
void ScopedStreamOut(const Disallowed&&) {
  static_assert(false, "Must not call ScopedStreamOut with arguments");
}

inline auto ScopedStreamErr(std::source_location loc = std::source_location::current()) {
  return log_internal::ScopedStream(loc, std::cerr);
}

template<typename Disallowed>
void ScopedStreamErr(const Disallowed&&) {
  static_assert(false, "Must not call ScopedStreamErr with arguments");
}

inline auto ScopedStreamVoid(std::source_location loc = std::source_location::current()) {
  using log_internal::VoidStream;
  static VoidStream void_stream;
  return log_internal::ScopedStream<VoidStream, VoidStream>(loc, void_stream);
}

template<typename Disallowed>
void ScopedStreamVoid(const Disallowed&&) {
  static_assert(false, "Must not call ScopedStreamVoid with arguments");
}

}  // namespace mbo::log

#endif  // MBO_LOG_SCOPED_STREAM_H_
