# 0.2.1

* Fix issue with MOPE in case it runs as an external dependency.
* Add `LimitedSet::contains(other)` which performs contains-all functionality (not part of STL).

# 0.2

* Add support for GCC (11.4/Ubuntu 22.04).
* Change Print Extender's `Print` to `ToString` which is a more widely used name.
* Change Print Extender's `ToString` to print field names (if available, e.g. Clang 16).
* When compiling with Clang show field names with the `AbslFormatImpl` extender.
* Enable `static constexpr NoDestruct<>` for more cases and compiler versions.
* container:
    * Added:
        * class `LimitedSet`: A space loimited, constexpr compliant set.
        * class `LimitedVector`: A space limited, constexpr compliant vector.
* diff:
    * Updated `unified_diff` with many more diff features.
* file:
    * Added:
        * function `GetMaxLines`: Reads at most given number of text lines from a file or returns absl::Status error.
        * function `JoinPathsRespectAbsolute`: Join multiple path elements respecting absolute path elements.
        * class `IniFile`: A simple INI file reader.
* Mope:
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
* status:
    * Added:
        * macro `MBO_RETURN_IF_ERROR`: Macro that simplifies handling functions returning `absl::Status`.
        * macro `MBO_ASSIGN_OR_RETURN`: Macro that simplifies handling functions returning `absl::StatusOr<T>`.
* strings:
    * Added:
        * struct `StripCommentsArgs`: Arguments for `StripComments` and `StripLineComments`.
        * function `StripComments`: Strips comments from lines.
        * function `StripLineComments`: Strips comments from a single line.
        * struct `StripParsedCommentsArgs`: Arguments for `StripParsedComments` and `StripParsedLineComments`.
        * function `StripParsedComments`: Strips comments from parsed lines.
        * function `StripLineParsedComments`: Strips comments from a single parsed line.
* testing:
    * Added:
        * macro `MBO_ASSERT_OK_AND_ASSIGN`: Simplifies testing with functions that return `absl::StatusOr<T>`.
* types:
    * Addded:
        * concept `ContainerIsForwardIteratable` determines whether a types can be used in forward iteration.
        * concept `ContainerHasEmplace` determines whether a container has `emplace`.
        * concept `ContainerHasEmplaceBack` determines whether a container has `emplace_back`.
        * concept `ContainerHasInsert` determines whether a container has `insert`.
        * concept `ContainerHasPushBack` determines whether a container has `push_back`.
        * conversion struct `CopyConvertContainer` simplifies copying containers to value convertible containers.
        * concept `IsAggregate` determines whether a type is an aggregate.
        * concept `IsDecomposable` determines whether a type can be used in static-bindings.
        * concept `IsBracesConstructibleV` determines whether a type can be constructe from given argument types.
        * struct `NoDestruct<T>`: Implements a type that allows to use any type as a static constant.

# 0.1

* Initial release: Clang support only.
