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

#include <concepts>  // IWYU pragma: keep
#include <iostream>
#include <source_location>
#include <sstream>

#if defined(__GNUC__) || defined(__clang__)
# define MBO_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define MBO_FORCE_INLINE __forceinline
#else
# define MBO_FORCE_INLINE inline
#endif

namespace mbo::log {

// A stream handler that simply ignores all and any output.
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
  VoidStream& operator<<(const T&) noexcept {  // NOLINT(*-named-parameter)
    return *this;
  }
};

enum class ScopedStreamMode {
  kContinue = 0,
  kQuickExit = 1,
};

// The `ScopedStream` allows to collect some output and send it to an output stream on destruction.
// The stream will always start output with file/line/function information from the provided `loc`.
// Secondary output can be fully suppressed including most code, so the compiler should be able to
// optimize out all secondary stream code. This is done by setting `StringStreamT` to `VoidStream`.
template<
    ScopedStreamMode kMode = ScopedStreamMode::kContinue,
    typename StringStreamT = std::stringstream,
    typename OStreamT = std::ostream,
    typename MsgT = std::string_view>
class ScopedStream {
 public:
  using Mode = ScopedStreamMode;
  using StringStream = StringStreamT;
  using OStream = OStreamT;

  ScopedStream() = delete;

  explicit ScopedStream(const std::source_location& loc, OStream& out, const MsgT&& msg = {})
      : loc_(loc), out_(out), msg_(std::forward<const MsgT>(msg)) {}

  ~ScopedStream() {
    // Technically a compiler could optimize this for `VoidStream`, but we drop this explicitly.
    if constexpr (!std::same_as<OStream, VoidStream>) {
      out_ << "[" << loc_.file_name() << ":" << loc_.line() << "] @" << loc_.function_name();
      if (!msg_.empty()) {
        out_ << " : " << msg_;
      }
      if (str_) {
        out_ << " : " << str_.str();
      }
      out_ << "\n";
    }
    switch (kMode) {
      case ScopedStreamMode::kContinue: break;
      case ScopedStreamMode::kQuickExit: {
        if constexpr (!std::same_as<OStream, VoidStream>) {
          out_ << std::flush;
        }
        std::quick_exit(1);
      }
    }
  }

  // It is important for these to be deleted since that ensures they cannot leave local scope.
  ScopedStream(const ScopedStream&) = delete;
  ScopedStream& operator=(const ScopedStream&) = delete;
  ScopedStream(ScopedStream&&) = delete;
  ScopedStream& operator=(ScopedStream&&) = delete;

  template<typename T>
  StringStream& operator<<(const T& val) {
    str_ << val;
    return str_;
  }

  StringStream& Stream() noexcept { return str_; }

 private:
  friend struct ScopedStreamTest;

  std::string TestGetStr() const { return std::string{str_.str()}; }

  const std::source_location& loc_;
  OStream& out_;
  StringStream str_;
  const MsgT msg_;
};

template<ScopedStreamMode kMode = ScopedStreamMode::kContinue>
MBO_FORCE_INLINE auto ScopedStreamOut(std::source_location loc = std::source_location::current()) {
  return ScopedStream<kMode>(loc, std::cout);
}

template<typename Disallowed>
void ScopedStreamOut(const Disallowed&&) {
  static_assert(false, "Must not call ScopedStreamOut with arguments");
}

template<ScopedStreamMode kMode = ScopedStreamMode::kContinue>
MBO_FORCE_INLINE auto ScopedStreamErr(std::source_location loc = std::source_location::current()) {
  return ScopedStream<kMode>(loc, std::cerr);
}

template<typename Disallowed>
void ScopedStreamErr(const Disallowed&&) {
  static_assert(false, "Must not call ScopedStreamErr with arguments");
}

template<ScopedStreamMode kMode = ScopedStreamMode::kContinue>
MBO_FORCE_INLINE auto ScopedStreamVoid(std::source_location loc = std::source_location::current()) {
  static VoidStream void_stream;
  return ScopedStream<kMode, VoidStream, VoidStream>(loc, void_stream);
}

template<typename Disallowed>
void ScopedStreamVoid(const Disallowed&&) {
  static_assert(false, "Must not call ScopedStreamVoid with arguments");
}

}  // namespace mbo::log

#undef MBO_FORCE_INLINE

#endif  // MBO_LOG_SCOPED_STREAM_H_
