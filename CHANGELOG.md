# 0.2.12

* Added `ConstScan` and `MakeConstScan`.
* Changed all `Make*Scan` types to create intermediate adapters.
* Made all `*Scan` and `Make*Scan` to accept `std::initializer` types. 
* Added support for move-only types.
* Improved `*scan` handling of initializer_lists.

# 0.2.11

* Made `AnyScan` constructor private.
* Separated `AnyScan` and `ConvertingScan` into distinct type with the same internal base.
* Restricted `AnyScanImpl` access to `MakeAnyScan` and `MakeConvertingScan`.
* Added `initializer_list` support for `AnyScan` and `ConvertingScan`.

# 0.2.10

* Added missing `LimitedVector` non-converting constructor for `std::initializer_list`.
* Added trait `ContainerConstIteratorValueType`.
* Added `ConvertingScan` and `MakeConvertingScan`.

# 0.2.9

* Added `mbo::container::AnyScan` for type erased container views (scanning).
* In `//mbo/types:traits_cc`:
  * Added concept `ContainerHasForwardIterator` determines whether a container has `begin`, `end` and `std::forward_iterator` compliant iterators.
  * Added concept `ContainerHasInputIterator` determines whether a container has `begin`, `end` and `std::input_iterator` compliant iterators.
  * Added struct `GetDifferenceType` is either set to the type's `difference_type` or `std::ptrdiff_t`.
  * Added concept `HasDifferenceType` determines whether a type has a `difference_type`.

# 0.2.8

* Made `//mbo/types:traits_cc` publicly visible.

# 0.2.7

* Improved `LimitedMap` and `LimitedSet`:
  * Better ASAN compatibility for constexpr.
  * Allow `LimitedOption` configuration instead of simple length specification.
    * Option to suppress calling clear in the destructor for cases where that is not a constexpr.
    * Option to require presorted input: Allows much large constexpr `LimitedSet`/`LimitedValue`.
* Renamed `Extender::AbslFormat` to `Extender::AbslStringify` to better reflect its purpose.
* Changed `Extender::AbslStringify` to print field names prefixed with a fot '.' (requires Clang).

# 0.2.6

* Made `RunfilesDir/OrDie` work correctly.
* Added a new comparator `mbo::types::CompareLess` which allows container optimizations.
* Moved most functionality of `LimitedMap` and `LimitedSet` to new base `internal::LimitedOrdered`.
* Added `LimitedMap::index_of` and `LimitedSet::index_of` (leaves one location to optimize search).
* Optimized `LimitedMap` and `LimitedSet` with `mbo::types::CompareLess`.

# 0.2.5

* Made `Limited{Map|Set|Vector}` iterators compliant with `std::contiguous_iterator`.
* Added `Limited{Map|Set|Vector}::data()`.

# 0.2.4

* Moved `CopyConvertContainer` to `mbo::container::ConvertContainer` and add conversion functions.
* Various traits fixes to correctly handle C++ concepts (pretty much every published explanation makes the same mistake).
* Added matcher `CapacityIs`.
* Added `LimitedMap`.

# 0.2.3

* For `LimitedVector` add an unused sentinal, so that `end` and other functions do not cause memory issues.

# 0.2.2

* Made `LimitedSet` and `LimitedVector` use C-arrays instead of `std::array` in order to solve some ASAN issues.
* Made `NoDestruct` ASAN friendly.
* Made tests PASS in mode ASAN out-of-the-box for Clang.
* Made `//mbo/types:tstring_test` PASS in ASAN mode.

# 0.2.1

* Fixed issue with MOPE in case it runs as an external dependency.
* Made tests work when run as external repository dependency.
* Added `LimitedSet::contains_all(other)` which performs contains-all-of functionality (not part of STL).
* Added `LimitedSet::contains_any(other)` which performs contains-any-of functionality (not part of STL).

# 0.2

* Added support for GCC (11.4/Ubuntu 22.04).
* Changed Print Extender's `Print` to `ToString` which is a more widely used name.
* Changed Print Extender's `ToString` to print field names (if available, e.g. Clang 16).
* When compiling with Clang show field names with the `AbslStringify` extender.
* Enabled `static constexpr NoDestruct<>` for more cases and compiler versions.
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
