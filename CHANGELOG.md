# 0.10.0

* Bumped minimum GCC to 13.
* Added direct support for `-fno-exceptions` irrespective of config setting.
* Added support for ASAN symbolizer with `--config=clang`.
* Added AbslStringify and hash support to `NoDestruct`, `RefWrap`, `Required`.
* Added struct `OptionalDataOrRef` similar to `std::optional` but can hold `std::nullopt`, a type `T` or a reference `T&`/`const T&`.
* Added struct `OptionalDataOrConstRef` similar to `std::optional` but can hold `std::nullopt`, a type `T` or a const reference `const T&`.
* Added struct `OptionalRef` similar to `std::optional` but can hold `std::nullopt` or a reference `T&`/`const T&`.
* Improved `Stringify` (breaking change):
    * Improved control for `Stringify` with Json output.
    * Updated `StringifyOptions` (breaking change).
    * Updated `StringifyWithFieldNames` (breaking change).
    * Moved `StringifyOptions` factories to `Stringify` so they can be constexpr even in C++20.
    * Modified `MboTypesStringifyOptions` (breaking change) to be more flexible - but users must deal with the change.
    * Added ability to detect bad `MboTypesStringifyOptions` signatures.
    * Added `StringifyFieldOptions` which holds outer and inner `StringifyOptions`.
    * Added `Stringify::AsJsonPretty()` and `StringifyOptions::AsJsonPretty()`.
    * Added API extension point functoin `MboTypesStringifyValueAccess` which allows to replace a struct with a single value in `Stringify` processing.
    * Added `Stringify` support for empty types and types that match `IsStringKeyedContainer`.
    * Fixed `Stringify` null* representations to stream as `'null'` as opposed to `0`.
    * Added ability to sort string keyed containers.
    * Added function `SetStringifyOstreamOutputMode` which sets global Stringify stream options by mode.
    * Added function `SetStringifyOstreamOptions` which sets global Stringify stream options.
    * Changed `Stringify::ToString` and `Stringify::Stream` to not depend on any mutable member of `Stringify`.
* Improved traits:
    * Added concept `ConstructibleFrom` implements a variant of `std::constructible_from` that works around its limitations to deal with array args.
    * Added concept `IsEmptyType` determines whether a type is empty (calls `std::is_empty_v`).
    * Fixed concept `IsAggregate` (breaking change).
    * Added concept `IsStringKeyedContainer` which determines whether a type is a container whose elements are pairs and whose keys are convertible to a std::string_view.
    * Added concept `IsReferenceWrapper` determines whether a type is a `std::reference_wrapper`.
    * Added concept `IsSameAsAnyOf` which determines whether a type is the same as one of a list of types. Similar to `IsSameAsAnyOfRaw` but using exact types.
    * Added template struct `TypedView` a wrapper for STL views that provides type definitions, most importantly `value_type`. That allows such views to be used with GoogleTest container matchers.
    * Added concept `IsOptionalDataOrRef` which determines whether a type is a `OptionalDataOrRef`.
    * Added concept `IsOptionalRef` which determines whether a type is a `OptionalRef`.
* Added `Demangle` to log de-mangled typeid names.
* Added struct `Overloaded` which implements an Overload handler for `std::visit(std::variant<...>)` and `std::variant::visit` (technically moved).
* Added function `CompareFloat` which can compare two `float`, `double` or `long double` values returning `std::strong_ordering`.
* Added function `WeakToStrong` which converts a `std::weak_ordering` to a `std::strong_ordering`.
* Fixed some IWYU pragmas.

# 0.9.0

* Added gmomck-matcher `mbo::testing::EqualsText` which compares text using line by line unified text diff.
* Added gmock-matcher-modifier `mbo::testing::WithDropIndent` which modifies `EqualsText` so that `mbo::strings::DropIndent` will be applied to the expected text.

# 0.8.0

* Renamed diff tooling options `ignore_space_change` to `ignore_trailing_space.
* Renamed `lhs_regex_replace` and `rhs_regex_replace` to `regex_replace_lhs` and `regex_replace_rhs` respectively.
* Added `regex_replace_lhs` and `regex_replace_rhs` to bzl rules `//mbo/diff:diff_test` and `//mbo/diff/tests:diff_test_test`.
* Renamed `mbo::diff::UnifiedDiff` to `mbo::diff::Diff`.
* Implemented diff algorithm `kDirect` which performs a direct side by side comparison.
* Renamed `//mbo/diff/unified_diff(_main).cc/h` to `//mbo/diff/diff(_main).cc/h`.
* Renamed `//mbo/diff:unified_diff` to `//mbo/diff`.
* Renamed `unified` to `context` for flags and attributes.
* Reorganized files and rules in directory `mbo/diff`.

# 0.7.0

* Added caching for unified `mbo::diff::UnifiedDiff` algorithm.
* Added `--lhs_regex_replace` and `--rhs_regex_replace` flags to `//mbo/diff:unified_diff`.
* Dropped all `using std::size_t` declarations.

# 0.6.0

* Moved bashtest out into `@com_helly25_bashtest` and used it from there.
* Added dependency on `@com_helly25_bashtest`.
* Dropped all remaining bashtest components.
* Added concept `ConstructibleInto` determined whether one type can be constructed from another. Similar to `std::convertible_to` but with the argument order of `std::constructible_from`.

# 0.5.0

* Added CI config LLVM-20.1.0 for Linux-Arm64 and MacOs-Arm64 platforms.
* Applied DWYU cleanup.
* Added field `mbo::file::GlobOptions::recursive` to select between recursive and flat globbing.
* Added function `mbo::file::GlobSplit` that splits a pattern into root and pattern.
* Improved program `//mbo/file:glob` to automatically split a single arg pattern.
* Added API extension point function `MboTypesStringifyConvert(I, T, V)` which allows to control conversion based on field types via a static call to the owning type, receiving the field index, the object and the field value.
* Added (experimental, removed in 0.6.0, moved to @helly25/bashtest) sh_library bashtest which provides a test runner for complex shell tests involving golden files that provides built-in golden update functionality (see `(. mbo/testing/bashtest.sh --help)`).

# 0.4.4

* Added `--config=cpp23` for `-std=c++23` to bazelrc and CI testing.
* Updated code to comply with current clang-tidy warnings.
* Enabled Bazel layering_check.

# 0.4.3

* Address const-ness issues in constexpr functions found by Clang 20.1.0.
* Dropped space in front of string-literal notation found by Clang 20.1.0.

# 0.4.2

* Tweaked automated release tooling.
* Switched to matrix for merge testing that verifies various GCC and Clang setups.
* Fixed `mbo::types::ContainerProxy` for GCC opt builds (issue with aliasing interpretation).
* Added `mbo::strings::StripPrefix` and `mbo::strings::StripSuffix` for temp `std::string&&`.

# 0.4.1

* Added load statements for all cc_binary, cc_library, cc_test functions in all bazel files.
* Added `mbo::strings::ConsumePrefix` and `mbo::strings::ConsumeSuffix`.

# 0.4.0

* Added function `mbo::strings::BigNumber`: Convert integral number to string with thousands separator.
* Added function `mbo::strings::BigNumberLen`: Calculate required string length for `BigNumer`.
* Added glob functionality:
  * Added struct `mbo::file::Glob2Re2Options`: Control conversion of a [glob pattern](https://man7.org/linux/man-pages/man7/glob.7.html) into a [RE2 pattern](https://github.com/google/re2/wiki/Syntax).
  * Added struct `mbo::file::GlobEntry`: Stores data for a single globbed entry (file, dir, etc.).
  * Added struct `mbo::file::GlobOptions`: Options for functions `Glob2Re2` and `Glob2Re2Expression`.
  * Added enum `mbo::file::GlobEntryAction`: Allows GlobEntryFunc to control further glob progression.
  * Added type `mbo::file::GlobEntryFunc`: Callback for acceptable glob entries.
  * Added function `mbo::file::GlobRe2`: Performs recursive glob functionality using a RE2 pattern.
  * Added function `mbo::file::Glob`: Performs recursive glob functionality using a `fnmatch` style pattern.
  * Added program `glob`: A recursive glob, see `glob --help`.
* WORKSPACE support:
  * Added rules_python and brought back com_google_protobuf support.
* Fixed `RunfilesDir/OrDie` to use environment variable `TEST_WORKSPACE` if present.
* Fixed formatting issue with `mbo/types/internal/decompose_count.h`.
* Downgraded Clang from 19.1.7 to 19.1.6.
* Updated GitHub workflow to test both Bazel flavors.
* Fixed issue in with struct names generation when compiling with Clang in mode ASAN.
* Added ability for unified_diff tool flag `--strip_file_header_prefix` to accept re2 expressions.
* Fixed various issues for bazelmod based builds.
* Improved `mbo::testing::RunfilesDir/OrDie` to support a single param variant that understands bazel labels. Further add support for other repos than the current one by reading the repo mapping.
* Fixed `//mbo/file/ini:ini_file_test` to be able to pass when run as remote repository.
* Added struct `mbo::types::ContainerProxy` which allows to add container access to other types including smart pointers of containers.
* Added struct `mbo::types::OpaquePtr` an opaque alternative to `std::unique_ptr` which works with forward declared types.
* Added struct `mbo::types::OpaqueValue` an `OpaquePtr` with direct access, comparison and hashing which will not allow a nullptr.
* Added struct `mbo::types::OpaqueContainer` an `OpaqueValue` with direct container access.
* Changed pre-commit to use clang-format 19.1.6.
* Added ability to construct Extended types from conversions (`ConstructFromConversions`).
* Removed support for type marker `MboTypesExtendDoNotPrintFieldNames`.

# 0.3.0

* Added struct `mbo::strings::AtLast` which allows `absl::StrSplit' to split on the last occurrence of a separator.
* Added concept `mbo::types::IsSet`.
* Added concept `mbo::types::IsVector`.
* Fixed concept `mbo::types::IsPair` and `mbo::types::IsPairFirstString` to not remove cvref.
* Fixed function `mbo::file::SetContents` to explicitly use binary mode.
* Added bazelmod support.
* Updated Clang to 19.1.7 (17.0.4 still ok).
* Fixed error/exception handling in `LimitedOrdered/Map/Set` to respect the configuration.

# 0.2.35

* Added ability to retrieve field names for structs without default constructor (e.g. due to reference fields).

# 0.2.34

* Added `mbo::types::Required` which is similar to `RefWrap` but stores the actual type (and unlike `std::optional` cannot be reset).
* Added `mbo::testing::WhenTransformedBy` which allows to compare containers after transforming them.
* Added custom Bazel flag `--//mbo/config:require_throws` which controls whether `MBO_CONFIG_REQUIRE` throw exceptions or use crash logging (the default `False` or `0`). This mostly affects containers.
* Added custom Bazel flag `--//mbo/config:limited_ordered_max_unroll_capacity`. This was undocumented as `--//mbo/container:limited_ordered_max_unroll_capacity` until now (though listed in the changelog). It controls the maximum unroll size for LimitedOrdered/Map/Set.

# 0.2.33

* Improved `MBO_RETURN_IF_ERROR` to correctly accept `absl::StatusOr` expressions independently of their implementation.
* Added `mbo::status::StatusBuilder` which allows to modify the message of an `absl::Status`.
* Added `mbo::status::GetStatus` which allows to convert its argument to an `absl::Status` if supported.
* Added macro `MBO_MOVE_TO_OR_RETURN` which can assign to structured binadings and other complex types.
* Added macro `MBO_ASSERT_OK_AND_MOVE_TO` which can assign to structured binadings and other complex types.
* Added matcher `mbo::testing::StatusHasPayload` that matches presence of any, a specific payload url, or a payload url with specific content.
* Added matcher `mbo::testing::StatusPayloads` that matches against `Status`/`StatusOr<>` payload maps.

# 0.2.32

* Added support for smart pointer types in `Stringify`.
* Added support for std::optional in `Stringify`.
* Added builtin ability to suppress `nullptr` and `nullopt` field values (in particular for Json output).
* Added support for move only decomposing in `StructToTuple`.
* Added (extendable) concept `IsSmartPtr`.
* Added concept `IsOptional`.
* Added extension API type `MboTypesStringifyDisable` which can be used to suppress printing.

# 0.2.31

* Prevent clangd indexing issues with `DecomposeCount` implementation.

# 0.2.30

* Added class `Stringify` which can turn arbitrary structs into strings.
* Added formatting control for `AbslStringify` externder using struct `StringifyOptions`.
* Added API extension point type `MboTypesStringifySupport` which enables `Stringify` support even if not otherwise enabled (disables Abseil stringify support in `Stringify`).
* Added API extension point type `MboTypesStringifyDoNotPrintFieldNames` which if present disables field names in `Stringify`.
* Added `AbslStringify` ADL API extension entry points `MboTypesStringifyFieldNames` and `MboTypesStringifyOptions`.
* Added function `StringifyWithFieldNames` a format control adapter for `AbslStringify`.
* Added field name support for non literal types in Clang.
* Added numeric field name (aka key) support and enforced for Json output.
* Added rule `mbo/types:stringify_ostream_cc` and header `mbo/types/stringify_ostream.h` for automatic ostream support using `Stringify`.
* Added static constructors `Extend::ConstructFromTuple` and `Extend::ConstructFromArgs`.
* Fixed a bug where Mope did not correctly enable and disable sections.
* Added documentation and tests on how to use a mope's `--set` flag to enable/disable sections.
* Fixed a bug with `DecomposeCount`. It must never return 0 (an empty aggregate cannot be decomposed using structured bindings).
* Changed Clang to use more reliable overload sets (they cannot be used in GCC).

# 0.2.29

* Added crtp-struct `ExtendNoPrint` Like `Extend` but without `Printable` and `Streamable` extender functionality.
* Added support for type `char` in extender `AbslStringify`.
* Added `mbo::types::TupleCat` which concatenates tuple types.
* Fixed `//mbo/log:log_timing_test` for systems with limited `std::source_location` support (e.g. MacOS).

# 0.2.28

* Improved `LimitedVector::insert` to deal better with conversions and complex types.
* Improved `LimitedMap`, `LimitedOrdered`, `LimitedSet` and `LimitedVector` ability to handle conversions.
* Added diff tooling options `ignore_all_space` and `ignore_consecutive_space`.
* Changed diff tooling options `ignore_space_change` to only ignore trailing space to match `git diff`.
* Added `mbo::log::LogTiming` a simple timing logger.
* Pinned Bazel version to 7.2.1.
* Updated hedron compile commands to include a patch for pre-processed headers.

# 0.2.27

* Implemented `LimitedVector::insert`.
* Fixed a bug in `LimitedVector::assign`.
* Updated https://github.com/bazel-contrib/toolchains_llvm past version 1.0.0.
* Changed `LimitedMap` and `LimitedSet` to not verify whether input is sorted, if they use `kRequireSortedInput` and `NDEBUG` is defined.

# 0.2.26

* Fixed comparison of `Extend` types with other types. Requires the other type can be turned into a tuple.
* Fixed internal consistencies.
* Updated `mope` to allow comments by setting a section to nothing: `{{#section=}}...{{/section}}`.
* Added concept `IsTuple` which determines whether a type is a `std::tuple`.
* Added `mbo::hash::simple::GetHash(std::string_view)` which is constexpr safe.
* Added hash support to tstring.

# 0.2.25

* Added concept `mbo::types::IsVariant`.
* Added concept `mbo::types::HasVariantMember`.
* Fixed issue with `Extend` ability to handle struct names when compiled with Clang.
* Fixed issue with `Extend` handling move only types when used in decompose assignment.
* Added template struct `BinarySearch` implements templated binary search algorithm.
* Added template struct `LinearSearch` implements templated linear search algorithm.
* Added template struct `ReverseSearch` implements templated reverse linear search algorithm.
* Added template struct `MaxSearch` implements templated linear search for last match algorithm.

# 0.2.24

* Improved `Extend` support.
* Renamed `MboExtendDoNotPrintFieldNames` to `MboTypesStringifyDoNotPrintFieldNames` which is the logical naming that follows the internal structure.

# 0.2.23

* Added llvm/clang which can be triggered with `bazel ... --config=clang`.
* Shortened generated extender names drastically (< 1/3rd).
* Added union member identification (enables union members for `Extend`'s printing/streaming).
* Added ability to suppress field name support in `Extend` by adding `using MboExtendDoNotPrintFieldNames = void;`.

# 0.2.22

* Changed the way `mbo::types::Extend` types are constructed to support more complex and deeper nested types.
* Provided a new `mbo::extender::Default` which wraps all default extenders.

# 0.2.21

* Optimized `AnyScan`, `ConstScan` and `ConvertingScan` by dropping clone layer. We also explicitly support multiple iterations on one object.

# 0.2.20

* Added `empty` and `size` to `AnyScan`, `ConstScan` and `ConvertingScan`.

# 0.2.19

* Changed `Extender` print and stream ability to output pointers to containers.

# 0.2.18

* Changed `LimitedSet` and `LimitedMap`:
  * Changed to new optimized code for `std::less` and make that the default.
  * Added custom bazel flag `--//mbo/container:limited_ordered_max_unroll_capacity=N`. This controls the maximum capacity `N` for which `index_of` will be unrolled (defaults to 16, range [4...32], see `mbo::container::container_internal::kUnrollMaxCapacityDefault`.
* Changed `RefWrap`:
  * Added constexpr support.
  * Made constructor implicit.

# 0.2.17

* Added template-type `RefWrap<T>`: similar to `std::reference_wrapper` but supports operators `->` and `*`.
* Fixed double computation in `MBO_RETURN_IF_ERROR`.

# 0.2.16

* Fixed `LimitedOptionsFlag::kEmptyDestructor` use in `LimitedVector`.

# 0.2.15

* Added `LimitedOptions` support to `LimitedVector`.

# 0.2.14

* Changed `tstring::find_first_of` and `tstring::find_last_of` solely with `string_view::find` to solve ASAN issue.
* Added `LimitedSet::at_index` and `LimitedMap::at_index`.

# 0.2.13

* Applied several tweaks for `*Scan`.
* Improved compiler error message when using `AnyScan` with incompatible pairs due
  to `first_type` being const on the right side.
* Added concept `IsSameAsAnyOfRaw` / `NotSameAsAnyOfRaw` which determine whether type is one of a list of types.
* Added `const char* tstring::c_str` which is occasionally needed.

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
* Renamed `types::AbslFormat` to `types::AbslStringify` to better reflect its purpose.
* Changed `types::AbslStringify` to print field names prefixed with a fot '.' (requires Clang).

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
