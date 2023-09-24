# MBO, a C++20 library

This C++20 library provides some general useful building blocks and integrates
with [Google's Abseil library](https://abseil.io/).

The C++ library is organized in functional groups each residing in their own directory.

* Container
    * `namespace mbo::container`
    * mbo/container:limited_set_cc, mbo/container/limited_set.h
        * class `LimitedSet`: A space loimited, constexpr compliant set.
    * mbo/container:limited_vector_cc, mbo/container/limited_vector.h
        * class `LimitedVector`: A space limited, constexpr compliant vector.
* Diff
    * `namespace mbo::diff`
    * mbo/diff:unified_dff_cc, mbo/diff/unified_diff.h
        * class `UnifiedDiff`: A class that implements unified-diffing.
    * mbo/diff:unified_diff
        * binary `unfied_diff`: A binary that performs a unified-diff on two files.
    * mbo/diff:diff_bzl, mbo/diff/diff.bzl
        * bzl-macro `difftest`: A test rule that compares an output versus a golden file.
* Files
    * `namespace mbo::files`
    * mbo/file:artefact_cc, mbo/file/artefact.h
        * struct `Artefact`: Holds information about a file (its data content, name, and modified time).
    * mbo/file:file_cc, mbo/file.file.h
        * function `GetContents`: Reads a file and returns its contents or an absl::Status error.
        * function `GetMTime`: Returns the last update/modified time of a file or an absl::Status error.
        * function `GetMaxLines`: Reads at most given number of text lines from a file or returns absl::Status error.
        * function `IsAbsolutePath`: Returns whether a given path is absolute.
        * function `JoinPaths`: Join multiple path elements.
        * function `JoinPathsRespectAbsolute`: Join multiple path elements respecting absolute path elements.
        * function `NormalizePath`: Normalizes a path.
        * function `Readable`: Returns whether a file is readable or an absl::Status error.
        * function `SetContents`: Writes contents to a file.
    * mbo/file/ini:ini_file_cc, mbo/file/ini/ini_file.h
        * class `IniFile`: A simple INI file reader.
* Mope
    * `namespace mbo::mope`
    * The `MOPE` templating engine. Run `bazel run //mbo/mope -- --help` for detailed documentation.
    * mbo/mope
        * binary `mope`.
    * mbo/mope:mope_cc, mbo/mope/mope.h
        * class `Template`: The mope template engine and data holder.
    * mbo/mope:ini_cc, mbo/mope/ini.h
        * function `ReadIniToTemlate`: Helper to initialize a mope Template from an INI file.
    * mbo/mope:mope_bzl, mbo/mope/mope.bzl
        * bzl-rule `mope`: A rule that expands a mope template file.
        * bazl-macro `mope_test`: A test rule compares mope template expansion against golden files. This
          supports `clang-format` and thus can be used for source-code generation and verification.
* Status
    * `namespace mbo::status`
    * mbo/status:status_macros_cc, mbo/status/status_macros.h
        * macro `MBO_RETURN_IF_ERROR`: Macro that simplifies handling functions returning `absl::Status`.
        * macro `MBO_ASSIGN_OR_RETURN`: Macro that simplifies handling functions returning `absl::StatusOr<T>`.
* Strings
    * `namespace mbo::strings`
    * mbo/strings:indent_cc, mbo/strings/indent.h
        * function `DropIndent`: Converts a raw-string text block as if it had no indent.
        * function `DropIndentAndSplit`: Variant of `DropIndent` that returns the result as lines.
    * mbo/strings:strip_cc, mbo/strings/strip.h
        * struct `StripCommentsArgs`: Arguments for `StripComments` and `StripLineComments`.
        * function `StripComments`: Strips comments from lines.
        * function `StripLineComments`: Strips comments from a single line.
        * struct `StripParsedCommentsArgs`: Arguments for `StripParsedComments` and `StripParsedLineComments`.
        * function `StripParsedComments`: Strips comments from parsed lines.
        * function `StripLineParsedComments`: Strips comments from a single parsed line.
* Testing
    * `namespace mbo::testing`
    * mbo/testing:status_cc, mbo/testing/status.h
        * gMock-matcher `IsOk`: Tests whether an absl::Status or absl::StatusOr is absl::OkStatus.
        * gMock-matcher `IsOkAndHolds`: Tests an absl::StatusOr for absl::OkStatus and contents.
        * gMock-Matcher `StatusIs`: Tests an absl::Status or absl::StatusOr against a specific status code and message.
        * macro `MBO_ASSERT_OK_AND_ASSIGN`: Simplifies testing with functions that return `absl::StatusOr<T>`.
* Types
    * `namespace mbo::types`
    * mbo/types:cases_cc, mbo/types/cases.h
        * meta-type `Cases`: Allows to switch types based on conditions.
        * meta-type `CaseIndex`: Evaluates the first non zero case (1-based, 0 if all zero).
        * meta-type `IfThen`: Helper type to generate if-then `Cases` types.
        * meta-type `IfElse`: Helper type to generate else `Cases` which are always true and must go last.
        * meta-type `IfFalseThenVoid`: Helper type that can be used to skip a case.
        * meta-type `IfTrueThenVoid`: Helper type to inject default cases and to ensure the required type expansion is always possible.
    * mbo/types:copy_convert_container_cc, mbo/types/copy_convert_container.h
        * conversion struct `CopyConvertContainer` simplifies copying containers to value convertible containers.
    * mbo/types:extend_cc, mbo/types/extend.h
        * crtp-struct `Extend`: Enables extending of struct/classe types with basic functionality.
        * crtp-struct `ExtendNoDefault` Like `Extend` but without default extender functionality.
        * `namespace extender`
            * extender-struct `AbslFormat`: Extender that injects functionality to make an `Extend`ed type work with abseil format/print functions.
            * extender-struct `AbslHashable`: Extender that injects functionality to make an `Extend`ed type work with abseil hashing (and also `std::hash`).
            * extender-struct `Comparable`: Extender that injects functionality to make an `Extend`ed type comparable. All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
            * extender-struct `Printable`:
                * Extender that injects functionality to make an `Extend`ed type get a `std::string ToString() const` function which can be used to convert a type into a `std::string`.
                * The output is a comma separated list of field values, e.g. `{ 25, 42 }`.
                * If available (Clang 16+) this function prints field names `{ first = 25, second = 42 }`.
            * extender-struct `Streamable`: Extender that injects functionality to make an `Extend`ed type streamable. This allows the type to be used directly with `std::ostream`s.
    * mbo/types:no_destruct_cc, mbo/types/no_destruct.h
        * struct `NoDestruct<T>`: Implements a type that allows to use any type as a static constant.
        * Mainly, this prevents calling the destructor and thus prevents termination issues (initialization order fiasco).
    * mbo/types:traits_cc, mbo/types/traits.h
        * concept `IsAggregate` determines whether a type is an aggregate.
        * concept `IsDecomposable` determines whether a type can be used in static-bindings.
        * concept `IsBracesConstructibleV` determines whether a type can be constructe from given argument types.
        * concept `ContainerIsForwardIteratable` determines whether a types can be used in forward iteration.
        * concept `ContainerHasEmplace` determines whether a container has `emplace`.
        * concept `ContainerHasEmplaceBack` determines whether a container has `emplace_back`.
        * concept `ContainerHasInsert` determines whether a container has `insert`.
        * concept `ContainerHasPushBack` determines whether a container has `push_back`.
    * mbo/types:tstring_cc, mbo/types/tstring.h
        * struct `tstring`: Implements type `tstring` a compile time string-literal type.
        * operator `operator"" _ts`: String literal support for Clang, GCC and derived compilers.

In addition some Bazel macros are implemented that are not direct part of the library:

* bzl:archive.bzl
    * `http_archive`: Simple wrapper that tests whether the archive was already loaded.
    * `github_archive`: Specialized archive wrapper that supports github that supports `tagged` releases or commits.

# Installation and requirements

This repository requires a C++20 compiler (in case of MacOS XCode 15 is needed).

This is done so that newer features like `std::source_location` can be used.

The project only comes with a Bazel BUILD.bazel file and can be added to other Bazel projects. Checkout [Releases](https://github.com/helly25/mbo/releases) or use head ref as follows:

```
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "com_helly25_proto",
  url = "https://github.com/helly25/mbo/archive/refs/heads/main.tar.gz",
  # See https://github.com/helly25/mbo/releases for releases.
)
```

The project is formatted with specific clang-format settings which require clang 16+ (in case of MacOs LLVM 16+ can be installed using brew).
