# MBO, a C++20 library

This C++20 library provides some general useful building blocks and integrates
with [Google's Abseil library](https://abseil.io/).

The library is tested with Clang and GCC using continuous integration: [![Test](https://github.com/helly25/mbo/actions/workflows/main.yml/badge.svg)](https://github.com/helly25/mbo/actions/workflows/main.yml). Manual testing with native Apple clang version 15.0.0 ARM is also done.

The C++ library is organized in functional groups each residing in their own directory.

* Container
    * `namespace mbo::container`
    * mbo/container:any_scan_cc, mbo/container/any_scan.h
        * class `AnyScan`: A container type independent iteration view - or scan over the container.
        * class `ConstScan`: A container type independent iteration view for const value_types.
        * class `ConvertingScan`: A container scanner that allows for conversions.
        * function `MakeAnyScan`: Helper function to create `AnyScan` instances.
        * function `MakeConstScan`: Helper function to create `ConstScan` instances.
        * function `MakeConvertingScan`: Helper function to create `ConvertingScan` instances.
    * mbo/container:convert_container_cc, mbo/container/convert_container.h
        * conversion struct `ConvertContainer` simplifies copying containers to value convertible containers.
    * mbo/container:limited_map_cc, mbo/container/limited_map.h
        * class `LimitedMap`: A space limited, constexpr compliant `map`.
    * mbo/container:limited_options_cc, mbo/container/limited_options.h
        * class `LimitedOptions`: A compile time configuration option for `LimitedSet` and `LimitedMap`.
    * mbo/container:limited_set_cc, mbo/container/limited_set.h
        * class `LimitedSet`: A space limited, constexpr compliant `set`.
    * mbo/container:limited_vector_cc, mbo/container/limited_vector.h
        * class `LimitedVector`: A space limited, constexpr compliant `vector`.
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
    * mbo/file:file_cc, mbo/file/file.h
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
* Hash
    * `namespace mbo::hash`
    * mbo/hash:hash_cc, mbo/hash/hash.h
        * function `simple::GetHash(std::string_view)`: A constexpr capable hash function.
* Log
    * `namespace mbo::log`
    * mbo/log:log_timing_cc, mbo/log/log_timing.h
        * functoin `mbo::log::LogTiming([args])` a simple timing logger.
* Mope
    * `namespace mbo::mope`
    * The `MOPE` templating engine. Run `bazel run //mbo/mope -- --help` for detailed documentation.
    * mbo/mope
        * binary `mope`.
    * mbo/mope:mope_cc, mbo/mope/mope.h
        * class `Template`: The mope template engine and data holder.
    * mbo/mope:ini_cc, mbo/mope/ini.h
        * function `ReadIniToTemplate`: Helper to initialize a mope Template from an INI file.
    * mbo/mope:mope_bzl, mbo/mope/mope.bzl
        * bzl-rule `mope`: A rule that expands a mope template file.
        * bazl-macro `mope_test`: A test rule that compares mope template expansion against golden files. This
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
    * mbo/strings:parse_cc, mbo/strings/parse.h
        * function `ParseString`: Parses strings respecting C++ and custom escapes as well as quotes (all configurable).
        * function `ParseStringList`: Parses and splits strings respecting C++ and custom escapes as well as quotes (all configurable).
    * mbo/strings:strip_cc, mbo/strings/strip.h
        * struct `StripCommentsArgs`: Arguments for `StripComments` and `StripLineComments`.
        * function `StripComments`: Strips comments from lines.
        * function `StripLineComments`: Strips comments from a single line.
        * struct `StripParsedCommentsArgs`: Arguments for `StripParsedComments` and `StripParsedLineComments`.
        * function `StripParsedComments`: Strips comments from parsed lines.
        * function `StripLineParsedComments`: Strips comments from a single parsed line.
* Testing
    * `namespace mbo::testing`
    * mbo/testing:matchers_cc, mbo/testing/matchers.h
        * gMock-Matcher `CapacityIs` which checks the capacity of a container.
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
    * mbo/types:compare_cc, mbo/types/compare.h
        * comparator `mbo::types::CompareLess` which is compatible to std::Less but allows container optimizations.
    * mbo/types:extend_cc, mbo/types/extend.h
        * crtp-struct `Extend`: Enables extending of struct/class types with basic functionality.
        * crtp-struct `ExtendNoDefault` Like `Extend` but without default extender functionality.
        * crtp-struct `ExtendNoPrint` Like `Extend` but without `Printable` and `Streamable` extender functionality.
        * `namespace extender`
            * extender-struct `AbslStringify`: Extender that injects functionality to make an `Extend`ed type work with abseil format/print functions.
            * extender-struct `AbslHashable`: Extender that injects functionality to make an `Extend`ed type work with abseil hashing (and also `std::hash`).
            * extender-struct `Comparable`: Extender that injects functionality to make an `Extend`ed type comparable. All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
            * extender-struct `Printable`:
                * Extender that injects functionality to make an `Extend`ed type get a `std::string ToString() const` function which can be used to convert a type into a `std::string`.
                * The output is a comma separated list of field values, e.g. `{ 25, 42 }`.
                * If available (Clang 16+) this function prints field names `{ first = 25, second = 42 }`.
            * extender-struct `Streamable`: Extender that injects functionality to make an `Extend`ed type streamable. This allows the type to be used directly with `std::ostream`s.
        * struct `AbslStringifyFieldOptions` which can be used to control `AbslStringify` formatting.
    * mbo/types:no_destruct_cc, mbo/types/no_destruct.h
        * struct `NoDestruct<T>`: Implements a type that allows to use any type as a static constant.
        * Mainly, this prevents calling the destructor and thus prevents termination issues (initialization order fiasco).
    * mbo/types:ref_wrap_cc, mbo/types/ref_wrap.h
        * template-type `RefWrap<T>`: similar to `std::reference_wrapper` but supports operators `->` and `*`.
    * mbo/types:template_search_cc, mbo/types/template_search.h:
        * template struct `BinarySearch` implements templated binary search algorithm.
        * template struct `LinearSearch` implements templated linear search algorithm.
        * template struct `ReverseSearch` implements templated reverse linear search algorithm.
        * template struct `MaxSearch` implements templated linear search for last match algorithm.
    * mbo/types:traits_cc, mbo/types/traits.h
        * type alias `ContainerConstIteratorValueType` returned the value-type of the const_iterator of a container.
        * concept `ContainerIsForwardIteratable` determines whether a types can be used in forward iteration.
        * concept `ContainerHasEmplace` determines whether a container has `emplace`.
        * concept `ContainerHasEmplaceBack` determines whether a container has `emplace_back`.
        * concept `ContainerHasInsert` determines whether a container has `insert`.
        * concept `ContainerHasPushBack` determines whether a container has `push_back`.
        * concept `ContainerHasForwardIterator` determines whether a container has `begin`, `end` and `std::forward_iterator` compliant iterators.
        * concept `ContainerHasInputIterator` determines whether a container has `begin`, `end` and `std::input_iterator` compliant iterators.
        * type alias `GetDifferenceType` is either set to the type's `difference_type` or `std::ptrdiff_t`.
        * concept `HasDifferenceType` determines whether a type has a `difference_type`.
        * concept `IsAggregate` determines whether a type is an aggregate.
        * concept `IsCharArray` determines whether a type is a `char*` or `char[]` related type.
        * concept `IsDecomposable` determines whether a type can be used in static-bindings.
        * concept `IsInitializerList` determines whether a type is `std::initializer<T> type.
        * concept `IsBracesConstructibleV` determines whether a type can be constructe from given argument types.
        * concept `IsPair` determines whether a type is a `std::pair`.
        * concept `IsSameAsAnyOfRaw` / `NotSameAsAnyOfRaw` which determine whether type is one of a list of types.
        * concept `IsTuple` determines whether a type is a `std::tuple`.
        * concept `IsVariant` determines whether a type is a `std::variant` type.
    * mbo/types:tstring_cc, mbo/types/tstring.h
        * struct `tstring`: Implements type `tstring` a compile time string-literal type.
        * operator `operator"" _ts`: String literal support for Clang, GCC and derived compilers.
    * mbo/types:tuple_cc, mbo/types/tuple.h
        * template struct `mbo::types::TupleCat` which concatenates tuple types.

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

# Presentations

## Practical Production-proven Constexpr API Elements

Presented at [C++ On Sea 2024](https://cpponsea.uk/2024/session/practical-production-proven-constexpr-api-elements), this presentation covers the theory behind:
* `mbo::hash::simple::GetHash`,
* `mbo::container::LimitedVector`,
* `mbo::container::LimitedMap`, and
* `mbo::container::LimitedSet`.

Slides are available at:
<br/>
<br/>
[<img src="https://helly25.com/wp-content/uploads/2024/07/Practical-Production-proven-Constexpr-API-Elements_TitleCard-copy-980x551.png">](https://helly25.com/practical-production-proven-constexpr-api-elements/)
