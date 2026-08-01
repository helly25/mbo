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

struct Voidifier {
  ~Voidifier() = default;
  Voidifier() = default;

  template<typename... T>
  Voidifier(const T&...) noexcept {}  // NOLINT(*-named-parameter,*-explicit-*)

  Voidifier(const Voidifier&) = default;
  Voidifier& operator=(const Voidifier&) = default;
  Voidifier(Voidifier&&) noexcept = default;
  Voidifier& operator=(Voidifier&&) noexcept = default;

  template<typename T>
  Voidifier& operator=(const T&&) noexcept {}  // NOLINT(*-named-parameter)
};

static_assert(std::is_empty_v<Voidifier>);
static_assert(sizeof(Voidifier) == 1);

// A stream handler that simply ignores all and any output.
struct VoidStream {
  ~VoidStream() = default;
  VoidStream() = default;

  VoidStream(const std::ostream&) {}  // NOLINT(*-named-parameter,*-explicit-*)

  VoidStream(const Voidifier&) {}  // NOLINT(*-named-parameter,*-explicit-*)

  VoidStream(const VoidStream&) = default;
  VoidStream& operator=(const VoidStream&) = default;
  VoidStream(VoidStream&&) noexcept = default;
  VoidStream& operator=(VoidStream&&) noexcept = default;

  VoidStream& operator=(const Voidifier&) noexcept { return *this; }

  operator bool() const noexcept { return false; }  // NOLINT(*-explicit-*)

  // NOLINTNEXTLINE(*-identifier-naming,*-member-functions-to-static)
  constexpr std::string_view str() const noexcept { return {}; }

  template<typename T>
  VoidStream& operator<<(const T&) noexcept {  // NOLINT(*-named-parameter)
    return *this;
  }
};

static_assert(std::is_empty_v<VoidStream>);
static_assert(sizeof(VoidStream) == 1);

enum class ScopedStreamMode {
  kContinue = 0,
  kQuickExit = 1,
};

namespace log_internal {

template<typename U, typename T>
concept IsOStreamOrT =
    std::is_base_of_v<std::ostream, std::remove_cvref_t<U>> || std::same_as<std::remove_cvref_t<U>, T>;

template<typename T>
concept HasOStreamOperator = requires(int val, T& obj) {
  { obj.operator<<(val) } -> IsOStreamOrT<T>;
};

template<typename T>
concept HasOStreamOperatorOrIsVoidifier = HasOStreamOperator<T> || std::same_as<T, Voidifier>;

template<typename T>
concept IsStringViewOrVoidifier = std::same_as<T, std::string_view> || std::same_as<T, Voidifier>;

}  // namespace log_internal

// The `ScopedStream` allows to collect some output and send it to an output stream on destruction.
// The stream will always start output with file/line/function information from the provided `loc`.
// Secondary output can be fully suppressed including most code, so the compiler should be able to
// optimize out all secondary stream code. This is done by setting `StringStreamT` to `VoidStream`.
//
// Type `StringStreamT` can be set to one of the following:
//   - `std::stringstream` to store a string stream and send output to it,
//   - `VoidStream` to avoid storing a string stream and suppress all output, or
//   - any other type that provides an `operator<<` to store a string stream and send output to it.
//   The type controls the additional output collection stream that is used to collect the output.
//   The default is `std::stringstream` which is the most common use case.
//
// Type `OStreamT` can be set to one of the following:
//   - `Voidifier` to avoid storing a reference to an output stream. In this case, the output
//     will always go to `std::cerr`, or
//   - `VoidStream` to avoid storing a reference to an output stream and suppress all output, or
//   - `std::ostream` to store a reference to an output stream and send output to it, or
//   - any other type that provides an `operator<<` to store a reference to an output stream
//     and send output to it.
//   The type controls the output stream used to send the output to and default to `std::ostream`.
//
// Type `MsgT` can be set to one of the following:
//   - `std::string_view` to store a string view and send output to it, or
//   - `Voidifier` to avoid storing a string view and suppress all output.
//   The type controls the additional message prefixthat is used to send output to.
//
// It is possible to reduce the storage amount used by the `ScopedStream` by using `VoidStream` and
// `Voidifier` types. The default types are chosen to be the most common use case and provide a good
// balance between functionality and storage size.
//
// The reson to use `std::string_view` for `MsgT` is that it is a non-owning type which does not
// require any additional storage. It is also a very common type to use for messages. That message
// con be a geneated `constexpr` and will be output even if the `StringStreamT` is `VoidStream`.
// The `ScopedStream` will not take ownership of the message and will not copy it. It will only
// store a reference to it. The user must ensure that the message is valid for the lifetime of the
// `ScopedStream`.
template<
    ScopedStreamMode kMode = ScopedStreamMode::kContinue,
    log_internal::HasOStreamOperator StringStreamT = std::stringstream,
    log_internal::HasOStreamOperatorOrIsVoidifier OStreamT = std::ostream,
    log_internal::IsStringViewOrVoidifier MsgT = std::string_view>
class ScopedStream {
 public:
  using Mode = ScopedStreamMode;
  using StringStream = std::remove_cvref_t<StringStreamT>;
  using OStreamRaw = std::remove_cvref_t<OStreamT>;
  static constexpr bool kSaveOStream = !std::same_as<OStreamRaw, Voidifier> && !std::same_as<OStreamRaw, VoidStream>;
  using OStreamParam = OStreamRaw;
  using OStreamField = std::conditional_t<kSaveOStream, OStreamT&, Voidifier>;

  ScopedStream() = delete;

  explicit ScopedStream(const std::source_location& loc, OStreamField out = std::cerr, const MsgT& msg = {})
      : loc_(loc), out_(out), msg_(msg) {}

  ~ScopedStream() {
    // Technically a compiler could optimize this for `VoidStream`, but we drop this explicitly.
    if constexpr (std::same_as<OStreamT, Voidifier>) {
      OutPrefix(std::cerr);
    } else if constexpr (!std::same_as<OStreamT, VoidStream>) {
      OutPrefix(out_);
    }
    switch (kMode) {
      case ScopedStreamMode::kContinue: break;
      case ScopedStreamMode::kQuickExit: {
        if constexpr (std::same_as<OStreamT, Voidifier>) {
          OutFlush(std::cerr);
        } else if constexpr (!std::same_as<OStreamT, VoidStream>) {
          OutFlush(out_);
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

  const StringStream& Stream() const noexcept { return str_; }

 private:
  friend struct ScopedStreamTest;

  void OutPrefix(std::ostream& out) {
    out << "[" << loc_.file_name() << ":" << loc_.line() << "] @" << loc_.function_name();
    if constexpr (!std::same_as<OStreamT, Voidifier>) {
      if (!msg_.empty()) {
        out << " : " << msg_;
      }
    }
    if (str_) {
      out << " : " << str_.str();
    }
    out << "\n";
  }

  void OutFlush(std::ostream& out) { out << std::flush; }

  std::string TestGetStr() const { return std::string{str_.str()}; }

  const std::source_location& loc_;
  [[no_unique_address]] StringStream str_;
  [[no_unique_address]] OStreamField out_;
  [[no_unique_address]] MsgT msg_;
};

// NOLINTBEGIN(*-magic-numbers)

static_assert(sizeof(ScopedStream<>) == (2 * sizeof(void*)) + sizeof(std::string_view) + sizeof(std::stringstream));
static_assert(sizeof(ScopedStream<ScopedStreamMode::kContinue, VoidStream>) <= 32);
static_assert(sizeof(ScopedStream<ScopedStreamMode::kContinue, VoidStream, std::ostream>) <= 32);
static_assert(sizeof(ScopedStream<ScopedStreamMode::kContinue, VoidStream, VoidStream>) <= 24);
static_assert(sizeof(ScopedStream<ScopedStreamMode::kContinue, VoidStream, std::ostream, Voidifier>) <= 16);
static_assert(sizeof(ScopedStream<ScopedStreamMode::kContinue, VoidStream, VoidStream, Voidifier>) <= 16);
static_assert(sizeof(ScopedStream<ScopedStreamMode::kContinue, VoidStream, Voidifier, Voidifier>) <= 16);

// NOLINTEND(*-magic-numbers)

template<ScopedStreamMode kMode = ScopedStreamMode::kContinue>
MBO_FORCE_INLINE auto ScopedStreamOut(const std::source_location& loc = std::source_location::current()) {
  return ScopedStream<kMode>(loc, std::cout);
}

template<typename Disallowed>
void ScopedStreamOut(const Disallowed&&) {  // NOLINT(*-named-parameter)
  static_assert(false, "Must not call ScopedStreamOut with arguments");
}

template<ScopedStreamMode kMode = ScopedStreamMode::kContinue>
MBO_FORCE_INLINE auto ScopedStreamErr(const std::source_location& loc = std::source_location::current()) {
  return ScopedStream<kMode, std::stringstream, Voidifier>(loc);
}

template<typename Disallowed>
void ScopedStreamErr(const Disallowed&&) {  // NOLINT(*-named-parameter)
  static_assert(false, "Must not call ScopedStreamErr with arguments");
}

template<ScopedStreamMode kMode = ScopedStreamMode::kContinue>
MBO_FORCE_INLINE auto ScopedStreamVoid(const std::source_location& loc = std::source_location::current()) {
  static VoidStream void_stream;
  return ScopedStream<kMode, VoidStream, VoidStream>(loc, void_stream);
}

template<typename Disallowed>
void ScopedStreamVoid(const Disallowed&&) {  // NOLINT(*-named-parameter)
  static_assert(false, "Must not call ScopedStreamVoid with arguments");
}

}  // namespace mbo::log

#undef MBO_FORCE_INLINE

#endif  // MBO_LOG_SCOPED_STREAM_H_
