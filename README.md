# MBO, a C++20 library

This C++20 library provides some general useful building blocks and integrates
with [Google's Abseil library](https://abseil.io/).

The library is tested with Clang (17+) and GCC (13+) on Ubuntu and MacOS (arm) using continuous integration: [![Test](https://github.com/helly25/mbo/actions/workflows/main.yml/badge.svg)](https://github.com/helly25/mbo/actions/workflows/main.yml).

## The C++ library is organized in functional groups each residing in their own directory.

* Config
    * `namespace mbo::config`
    * mbo/config:config_cc, mbo/config/config.h
        * Custom Bazel flag `--//mbo/config:limited_ordered_max_unroll_capacity` which controls the maximum unroll size for `LimitedOrdered` and thus `LimitedMap` and `LimitedSet`.
        * Custom Bazel flag `--//mbo/config:require_throws` which controls whether `MBO_CONFIG_REQUIRE` throw exceptions or use crash logging (the default `False` or `0`). This mostly affects containers.
    * mbo/config:require_cc, mbo/config/require.h
        * Marcos `MBO_CONFIG_REQUIRE(condition, message)` which allows to check a `condition` and either throw an exception or crash with Abseil FATAL logging. The behavior is controlled by `--//mbo/config:require_throws`.
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
    * mbo/diff:dff_cc, mbo/diff/diff.h
        * class `Diff`: A class that implements unified-diffing.
    * mbo/diff
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
    * mbo/file:glob_cc, mbo/file/glob.h
        * struct `Glob2Re2Options`: Control conversion of a [glob pattern](https://man7.org/linux/man-pages/man7/glob.7.html) into a [RE2 pattern](https://github.com/google/re2/wiki/Syntax).
        * struct `GlobEntry`: Stores data for a single globbed entry (file, dir, etc.).
        * struct `GlobOptions`: Options for functions `Glob2Re2` and `Glob2Re2Expression`.
        * enum `GlobEntryAction`: Allows GlobEntryFunc to control further glob progression.
        * type `GlobEntryFunc`: Callback for acceptable glob entries.
        * function `GlobRe2`: Performs recursive glob functionality using a RE2 pattern.
        * function `Glob`: Performs recursive glob functionality using a `fnmatch` style pattern.
        * function `GlobSplit`: Splits a pattern into root and pattern parts.
        * program `glob`: A recursive glob, see `glob --help`.
    * mbo/file/ini:ini_file_cc, mbo/file/ini/ini_file.h
        * class `IniFile`: A simple INI file reader.
* Hash
    * `namespace mbo::hash`
    * mbo/hash:hash_cc, mbo/hash/hash.h
        * function `simple::GetHash(std::string_view)`: A constexpr capable hash function.
* Log
    * `namespace mbo::log`
    * mbo/log:demangle_cc, mbo/log/demangle.h
        * functions `Demangle` to log de-mangled typeid names.
    * mbo/log:log_timing_cc, mbo/log/log_timing.h
        * functoin `LogTiming([args])` a simple timing logger.
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
        * bzl-macro `mope_test`: A test rule that compares mope template expansion against golden files. This
          supports `clang-format` and thus can be used for source-code generation and verification.
* Status
    * `namespace mbo::status`
    * mbo/status:status_builder_cc, mbo/status/status_builder.h
        * class `StatusBuilder` which allows to extend the message of an `absl::Status`.
    * mbo/status:status_cc, mbo/status/status.h
        * function `GetStatus` allows to convert types to an `absl::Status`.
    * mbo/status:status_macros_cc, mbo/status/status_macros.h
        * macro `MBO_ASSIGN_OR_RETURN`: Macro that simplifies handling functions returning `absl::StatusOr<T>`.
        * macro `MBO_MOVE_TO_OR_RETURN`: Macro that simplifies handling functions returning `absl::StatusOr<T>` where the result requires commas, in particular structured bindings.
        * macro `MBO_RETURN_IF_ERROR`: Macro that simplifies handling functions returning `absl::Status` or `absl::StausOr<T>`.
* Strings
    * `namespace mbo::strings`
    * mbo/strings:indent_cc, mbo/strings/indent.h
        * function `DropIndent`: Converts a raw-string text block as if it had no indent.
        * function `DropIndentAndSplit`: Variant of `DropIndent` that returns the result as lines.
    * mbo/strings:numbers_cc, mbo/strings/numbers.h
        * function `BigNumber`: Convert integral number to string with thousands separator.
        * function `BigNumberLen`: Calculate required string length for `BigNumer`.
    * mbo/strings:parse_cc, mbo/strings/parse.h
        * function `ParseString`: Parses strings respecting C++ and custom escapes as well as quotes (all configurable).
        * function `ParseStringList`: Parses and splits strings respecting C++ and custom escapes as well as quotes (all configurable).
    * mbo/strings:split_cc, mbo/strings/split.h
        * struct `AtLast`: Allows `absl::StrSplit' to split on the last occurrence of a separator.
    * mbo/strings:strip_cc, mbo/strings/strip.h
        * function `ConsumePrefix`: Removes a prefix from a `std::string` (like `absl::ConsumePrefix`).
        * function `ConsumeSuffix`: Removes a suffix from a `std::string` (like `absl::ConsumeSuffix`).
        * function `StripPrefix`: Removes a prefix from a `std::string&&` (like `absl::StripPrefix`).
        * function `StripSuffix`: Removes a suffix from a `std::string&&` (like `absl::StripSuffix`).
        * struct `StripCommentsArgs`: Arguments for `StripComments` and `StripLineComments`.
        * function `StripComments`: Strips comments from lines.
        * function `StripLineComments`: Strips comments from a single line.
        * struct `StripParsedCommentsArgs`: Arguments for `StripParsedComments` and `StripParsedLineComments`.
        * function `StripParsedComments`: Strips comments from parsed lines.
        * function `StripLineParsedComments`: Strips comments from a single parsed line.
* Testing
    * mbo/testing:matchers_cc, mbo/testing/matchers.h
        * gmock-matcher `CapacityIs` which checks the capacity of a container.
        * gmock-matcher `EqualsText` which compares text using line by line unified text diff.
        * gmock-matcher `IsNullopt` which compares its argument agains `std::nullopt`.
        * gmock-matcher `WhenTransformedBy` which allows to compare containers after transforming them. This sometimes allows for much more concise comparisons where a golden expectation is already available that only differs in a simple transformation.
        * gmock-matcher-modifier `WithDropIndent` which modifies `EqualsText` so that `DropIndent` will be applied to the expected text.
    * mbo/testing:status_cc, mbo/testing/status.h
        * gmock-matcher `IsOk`: Tests whether an `absl::Status` or `absl::StatusOr` is `absl::OkStatus`.
        * gmock-matcher `IsOkAndHolds`: Tests an `absl::StatusOr` for `absl::OkStatus` and contents.
        * gmock-matcher `StatusIs`: Tests an `absl::Status` or `absl::StatusOr` against a specific status code and message.
        * gmock-matcher `StatusHasPayload`: Tests whether an `absl::Status` or `absl::StatusOr` payload map has any payload, a specific payload url, or a payload url with specific content.
        * gmock-matcher `StatusPayloads` Tests whether an `absl::Status` or `absl::StatusOr` payload map matches.
        * macro `MBO_ASSERT_OK_AND_ASSIGN`: Simplifies testing with functions that return `absl::StatusOr<T>`.
        * macro `MBO_ASSERT_OK_AND_MOVE_TO`: Simplifies testing with functions that return `absl::StatusOr<T>` where the result requires commas, in particular structured bindings.
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
        * comparator `CompareLess` which is compatible to std::Less but allows container optimizations.
    * mbo/types:container_proxy_cc, mbo/types/container_proxy.h
        * struct `ContainerProxy` which allows to add container access to other types including smart pointers of containers.
    * mbo/types:extend_cc, mbo/types/extend.h
        * crtp-struct `Extend`: Enables extending of struct/class types with basic functionality.
        * crtp-struct `ExtendNoDefault` Like `Extend` but without default extender functionality.
        * crtp-struct `ExtendNoPrint` Like `Extend` but without `Printable` and `Streamable` extender functionality.
        * `namespace extender`
            * extender-struct `AbslStringify`: Extender that injects functionality to make an `Extend`ed type work with abseil format/print functions. See `Stringify` for various API extension points.
            * extender-struct `AbslHashable`: Extender that injects functionality to make an `Extend`ed type work with abseil hashing (and also `std::hash`).
            * extender-struct `Comparable`: Extender that injects functionality to make an `Extend`ed type comparable. All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
            * extender-struct `Printable`:
                * Extender that injects functionality to make an `Extend`ed type get a `std::string ToString() const` function which can be used to convert a type into a `std::string`.
                * The output is a comma separated list of field values, e.g. `{ 25, 42 }`.
                * If available (Clang 16+) this function prints field names `{ .first = 25, .second = 42 }`.
            * extender-struct `Streamable`: Extender that injects functionality to make an `Extend`ed type streamable. This allows the type to be used directly with `std::ostream`s.
    * mbo/types:no_destruct_cc, mbo/types/no_destruct.h
        * struct `NoDestruct<T>`: Implements a type that allows to use any type as a static constant.
        * Mainly, this prevents calling the destructor and thus prevents termination issues (initialization order fiasco).
    * mbo/types:opaque_cc, mbo/types/opaque.h
        * struct `OpaquePtr` an opaque alternative to `std::unique_ptr` which works with forward declared types.
        * struct `OpaqueValue` an `OpaquePtr` with direct access, comparison and hashing which will not allow a nullptr.
        * struct `OpaqueContainer` an `OpaqueValue` with direct container access.
    * mbo/types:optional_ref_cc, mbo/types/optional_data_or_ref.h
        * struct `OptionalDataOrRef` similar to `std::optional` but can hold `std::nullopt`, a type `T` or a reference `T&`/`const T&`.
        * struct `OptionalDataOrConstRef` similar to `std::optional` but can hold `std::nullopt`, a type `T` or a const reference `const T&`.
    * mbo/types:optional_ref_cc, mbo/types/optional_ref.h
        * struct `OptionalRef` similar to `std::optional` but can hold `std::nullopt` or a reference `T&`/`const T&`.
    * mbo/types:ref_wrap_cc, mbo/types/ref_wrap.h
        * template-type `RefWrap<T>`: similar to `std::reference_wrapper` but supports operators `->` and `*`.
    * mbo/types:required_cc, mbo/types/required.h
        * template-type `Required<T>`: similar to `RefWrap` but stores the actual type (and unlike `std::optional` cannot be reset).
    * mbo/types:stringify_cc, mbo/types/stringify.h
        * class `Stringify` a utility to convert structs into strings.
        * function `StringifyWithFieldNames` a format control adapter for `Stringify`.
        * struct `StringifyOptions` which can be used to control `Stringify` formatting.
        * API extension point type `MboTypesStringifySupport` which enables `Stringify` support even if not otherwise enabled (disables Abseil stringify support in `Stringify`).
        * API extension point type `MboTypesStringifyDisable` which disables `Stringify` support. This allows to prevent
        complex classes (and more importantly fields of complex types) from being printed/streamed using `Stringify`
        * API extension point type `MboTypesStringifyDoNotPrintFieldNames` which if present disables field names in `Stringify`.
        * API extension point function `MboTypesStringifyFieldNames` which adds field names to `Stringify`.
        * API extension point function `MboTypesStringifyOptions` which adds full format control to `Stringify`.
        * API extension point function `MboTypesStringifyConvert(I, T, V)` allows to control conversion based on field types via a static call to the owning type, receiving the field index, the object and the field value.
    * mbo/types:stringify_ostream_cc, mbo/types/stringify_ostream.h
        * operator `std::ostream& operator<<(std::ostream&, const MboTypesStringifySupport auto& v)` - conditioanl automatic ostream support for structs using `Stringify`.
    * mbo/types:traits_cc, mbo/types/traits.h
        * concept `ConstructibleInto` determined whether one type can be constructed from another. Similar to `std::convertible_to` but with the argument order of `std::constructible_from`.
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
        * concept `IsOptional` determines whether a type is a `std::optional` type.
        * concept `IsPair` determines whether a type is a `std::pair` type.
        * concept `IsSet` determines whether a type is a `std::set` type.
        * concept `IsSameAsAnyOf` which determines whether a type is the same as one of a list of types. Similar to `IsSameAsAnyOfRaw` but using exact types.
        * concept `IsSameAsAnyOfRaw` / `NotSameAsAnyOfRaw` which determines whether a type is one of a list of types. Similar to `IsSameAsAnyOf` but applies `std::remove_cvref_t` on all types.
        * concept `IsSmartPtr` determines whether a type is a `std::shared_ptr`, `std::unique_ptr` or `std::weak_ptr`.
          * Can be extended with other smart pointers through `IsSmartPtrImpl`.
        * concept `IsTuple` determines whether a type is a `std::tuple` type.
        * concept `IsVariant` determines whether a type is a `std::variant` type.
        * concept `IsVector` determines whether a type is a `std::vector` type.
    * mbo/types:template_search_cc, mbo/types/template_search.h:
        * template struct `BinarySearch` implements templated binary search algorithm.
        * template struct `LinearSearch` implements templated linear search algorithm.
        * template struct `ReverseSearch` implements templated reverse linear search algorithm.
        * template struct `MaxSearch` implements templated linear search for last match algorithm.
    * mbo/types:tstring_cc, mbo/types/tstring.h
        * struct `tstring`: Implements type `tstring` a compile time string-literal type.
        * operator `operator"" _ts`: String literal support for Clang, GCC and derived compilers.
    * mbo/types:tuple_cc, mbo/types/tuple.h
        * template struct `TupleCat` which concatenates tuple types.

## In addition some Bazel macros are implemented that are not direct part of the library:

* bzl:archive.bzl
    * `http_archive`: Simple wrapper that tests whether the archive was already loaded.
    * `github_archive`: Specialized archive wrapper that supports github that supports `tagged` releases or commits.

## Installation and requirements

This repository requires a C++20 compiler (in case of MacOS XCode 15 is needed). This is done so that newer features like `std::source_location` can be used.

The project only comes with a Bazel BUILD.bazel file and can be added to other Bazel projects.

The project is formatted with specific clang-format settings which require clang 16+ (in case of MacOs LLVM 16+ can be installed using brew). For simplicity in dev mode the project pulls the appropriate clang tools and can be compiled with those tools using `bazel [build|test] --config=clang ...`.

### WORKSPACE

Checkout [Releases](https://github.com/helly25/mbo/releases) or use head ref as follows:

```
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "com_helly25_mbo",
  url = "https://github.com/helly25/mbo/archive/refs/heads/main.tar.gz",
  # See https://github.com/helly25/mbo/releases for releases.
)
```

### MODULES.bazel

Check [Releases](https://github.com/helly25/mbo/releases) for details. All that is needed is a `bazel_dep` instruction with the correct version.

```
bazel_dep(name = "helly25_mbo", version = "0.0.0")
```

Unlike the `WORKSPACE` installation the `MODULES.bazel` installation from source checkout, the above [Bazel-Central-Registry](https://registry.bazel.build/modules/helly25_mbo) installation does not provide the LLVM tools and thus does not come with its own compiler. This is due to a restriction in Bazel's ability to handle toolchains when Bazel uses MODULES (`--enable_bzlmod` as opposed to `--enable_workspace`). Nonetheless all versions, all versions can be compiled with GCC 11+, Clang 17+ on Ubuntu and MacOs as enforced by CI. Other platforms and compilers are likely to work as well. However, Windows lacks some of the necessary tools and the library as well as its build system mostly assume Unix-style file and path names. That unfortunately means that on Windows some code cannot even be built.

## Presentations

### Practical Production-proven Constexpr API Elements

Presented at [C++ On Sea 2024](https://cpponsea.uk/2024/session/practical-production-proven-constexpr-api-elements), this presentation covers the theory behind:
* `mbo::hash::simple::GetHash`,
* `mbo::container::LimitedVector`,
* `mbo::container::LimitedMap`, and
* `mbo::container::LimitedSet`.

Slides are available at:
<br/>
<br/>
[<img src="https://helly25.com/wp-content/uploads/2024/07/Practical-Production-proven-Constexpr-API-Elements_TitleCard-copy-980x551.png">](https://helly25.com/practical-production-proven-constexpr-api-elements/)
