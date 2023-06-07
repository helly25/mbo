# MBO, a C++20 library

This C++20 library provides some general useful building blocks and integrates
with [Google's Abseil library](https://abseil.io/).

The C++ library is organized in functional groups each residing in their own directory.

* Diff
    * `namespacembo::diff`
    * mbo/diff:unified_dff_cc, mbo/diff/unified_diff.h
        * class `UnifiedDiff`: A class that implements unified-diffing.
    * mbo/diff:unified_diff
        * binary `unfied_diff`: A binary that performs a unified-diff on two files.
    * mbo/diff:diff.bzl
        * bzl-macro `difftest`: A test rule that compares an output versus a golden file.
* Files
    * `namespace mbo::files`
    * mbo/file:artefact_cc, mbo/file/artefact.h
        * struct `Artefact`: Holds information about a file (its data content, name, and modified time).
    * mbo/file:file_cc, mbo/file.file.h
        * function `GetContents`: Reads a file and returns its contents or an absl::Status error.
        * function `GetMTime`: Returns the last update/modified time of a file or an absl::Status error.
        * function `IsAbsolutePath`: Returns whether a given path is absolute.
        * function `JoinPaths`: Join multiple path elements.
        * function `NormalizePath`: Normalizes a path.
        * function `Readable`: Returns whether a file is readable or an absl::Status error.
        * function `SetContents`: Writes contents to a file.
* Strings
    * `namespace mbo::strings`
    * mbo/strings:indent_cc, mbo/strings/indent.h
        * function `DropIndent`: Converts a raw-string text block as if it had no indent.
        * function `DropIndentAndSplit`: Variant of `DropIndent` that returns the result as lines.
* Testing
    * `namespace mbo::testing`
    * mbo/testing:status_cc, mbo/testing/status.h
        * gMock-matcher `IsOk`: Tests whether an absl::Status or absl::StatusOr is absl::OkStatus.
        * gMock-matcher `IsOkAndHolds`: Tests an absl::StatusOr for absl::OkStatus and contents.
        * gMock-Matcher `StatusIs`: Tests an absl::Status or absl::StatusOr against a specific status code and message.
* Types
    * `namespace mbo::types`
    * mbo/types:cases_cc, mbo/types/cases.h
        * meta-type `Cases`: Allows to switch types based on conditions.
        * meta-type `CaseIndex`: Evaluates the first non zero case (1-based, 0 if all zero).
        * meta-type `IfThen`: Helper type to generate if-then `Cases` types.
        * meta-type `IfElse`: Helper type to generate else `Cases` which are always true and must go last.
        * meta-type `IfFalseThenVoid`: Helper type that can be used to skip a case.
        * meta-type `IfTrueThenVoid`: Helper type to inject default cases and to ensure the required type expansion is always possible.
    * mbo/types:extend_cc, mbo/types/extend.h
        * crtp-struct `Extend`: Enables extending of struct/classe types with basic functionality.
        * crtp-struct `ExtendNoDefault` Like `Extend` but without default extender functionality.
        * `namespace extender`
            * extender-struct `AbslFormat`: Extender that injects functionality to make an `Extend`ed type work with abseil format/print functions.
            * extender-struct `AbslHashable`: Extender that injects functionality to make an `Extend`ed type work with abseil hashing (and also `std::hash`).
            * extender-struct `Comparable`: Extender that injects functionality to make an `Extend`ed type comparable. All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
            * extender-struct `Printable`: Extender that injects functionality to make an `Extend`ed type get a `Print` function which can be used to convert a type into a `std::string`.
            * extender-struct `Streamable`: Extender that injects functionality to make an `Extend`ed type streamable. This allows the type to be used directly with `std::ostream`s.
    * mbo/types:tstring_cc, mbo/types/tstring.h
        * struct `tstring`: Implements type `tstring` a compile time string-literal type.
        * operator `operator"" _ts`: String literal support for Clang, GCC and derived compilers.

In addition some Bazel macros are implemented that are not direct part of the library:

* bzl:archive.bzl
    * `http_archive`: Simple wrapper that tests whether the archive was already loaded.
    * `github_archive`: Specialized archive wrapper that supports github that supports `tagged` releases or commits.