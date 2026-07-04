# MBO, a C++20 library

This C++20 library provides some general useful building blocks and integrates
with [Google's Abseil library](https://abseil.io/).

The library is tested with Clang (20+) and GCC (13+) on Ubuntu and MacOS (arm) using continuous integration: [![Test](https://github.com/helly25/mbo/actions/workflows/main.yml/badge.svg)](https://github.com/helly25/mbo/actions/workflows/main.yml).

## Library organization

The C++ library is organized in functional groups each residing in their own directory:

- Config
  - `namespace mbo::config`
  - mbo/config:config_cc, mbo/config/config.h
    - Custom Bazel flag `--//mbo/config:limited_ordered_max_unroll_capacity` which controls the maximum unroll size for `LimitedOrdered` and thus `LimitedMap` and `LimitedSet`.
    - Custom Bazel flag `--//mbo/config:require_throws` which controls whether `MBO_CONFIG_REQUIRE` throw exceptions or use crash logging (the default `False` or `0`). This mostly affects containers.
  - mbo/config:require_cc, mbo/config/require.h
    - Marcos `MBO_CONFIG_REQUIRE(condition, message)` which allows to check a `condition` and either throw an exception or crash with Abseil FATAL logging. The behavior is controlled by `--//mbo/config:require_throws`.
- Container
  - `namespace mbo::container`
  - mbo/container:any_scan_cc, mbo/container/any_scan.h
    - class `AnyScan`: A container type independent iteration view - or scan over the container.
    - class `ConstScan`: A container type independent iteration view for const value_types.
    - class `ConvertingScan`: A container scanner that allows for conversions.
    - function `MakeAnyScan`: Helper function to create `AnyScan` instances.
    - function `MakeConstScan`: Helper function to create `ConstScan` instances.
    - function `MakeConvertingScan`: Helper function to create `ConvertingScan` instances.
  - mbo/container:convert_container_cc, mbo/container/convert_container.h
    - conversion struct `ConvertContainer` simplifies copying containers to value convertible containers.
  - mbo/container:limited_map_cc, mbo/container/limited_map.h
    - class `LimitedMap`: A space limited, constexpr compliant `map`.
  - mbo/container:limited_options_cc, mbo/container/limited_options.h
    - class `LimitedOptions`: A compile time configuration option for `LimitedSet` and `LimitedMap`.
  - mbo/container:limited_set_cc, mbo/container/limited_set.h
    - class `LimitedSet`: A space limited, constexpr compliant `set`.
  - mbo/container:limited_vector_cc, mbo/container/limited_vector.h
    - class `LimitedVector`: A space limited, constexpr compliant `vector`.
- Diff
  - `namespace mbo::diff`
  - mbo/diff:diff_cc, mbo/diff/diff.h
    - class `Diff`: A class that implements line based diffing in unified, context, normal or side-by-side output format (`DiffOptions::output_format`), using the Myers minimal diff algorithm by default (`DiffOptions::algorithm` also offers `naive` and `direct`).
  - mbo/diff
    - binary `diff`: A binary that diffs two files; defaults to unified format, `--format` selects `unified`, `context`, `normal` or `side-by-side` (`--width`), `--algorithm` selects `myers` (default), `naive` or `direct`; `--minimal` guarantees minimal `myers` diffs.
  - mbo/diff:diff_bzl, mbo/diff/diff.bzl
    - bzl-macro `diff_test`: A test rule that compares an output versus a golden file.
- Files
  - `namespace mbo::files`
  - mbo/file:artefact_cc, mbo/file/artefact.h
    - struct `Artefact`: Holds information about a file (its data content, name, and modified time).
  - mbo/file:file_cc, mbo/file/file.h
    - function `GetContents`: Reads a file and returns its contents or an absl::Status error.
    - function `GetMTime`: Returns the last update/modified time of a file or an absl::Status error.
    - function `GetMaxLines`: Reads at most given number of text lines from a file or returns absl::Status error.
    - function `IsAbsolutePath`: Returns whether a given path is absolute.
    - function `JoinPaths`: Join multiple path elements.
    - function `JoinPathsRespectAbsolute`: Join multiple path elements respecting absolute path elements.
    - function `NormalizePath`: Normalizes a path.
    - function `Readable`: Returns whether a file is readable or an absl::Status error.
    - function `SetContents`: Writes contents to a file.
  - mbo/file:glob_cc, mbo/file/glob.h
    - struct `Glob2Re2Options`: Control conversion of a [glob pattern](https://man7.org/linux/man-pages/man7/glob.7.html) into a [RE2 pattern](https://github.com/google/re2/wiki/Syntax).
    - struct `GlobEntry`: Stores data for a single globbed entry (file, dir, etc.).
    - struct `GlobOptions`: Options for functions `Glob2Re2` and `Glob2Re2Expression`.
    - enum `GlobEntryAction`: Allows GlobEntryFunc to control further glob progression.
    - type `GlobEntryFunc`: Callback for acceptable glob entries.
    - function `GlobRe2`: Performs recursive glob functionality using a RE2 pattern.
    - function `Glob`: Performs recursive glob functionality using a `fnmatch` style pattern.
    - function `GlobSplit`: Splits a pattern into root and pattern parts.
    - program `glob`: A recursive glob, see `glob --help`.
  - mbo/file/ini:ini_file_cc, mbo/file/ini/ini_file.h
    - class `IniFile`: A simple INI file reader.
- Hash
  - `namespace mbo::hash` - principles and library docs: [mbo/hash/README.md](mbo/hash/README.md)
  - mbo/hash:hash_cc, mbo/hash/hash.h
    - function `GetHash64<Algo>(std::string_view, seed)` / `GetHash128<Algo>(std::string_view, seed)`: A constexpr-safe, non-cryptographic hash; the algorithm defaults to `DefaultHashAlgorithm` (`rapidhash`; `GetHash128` defaults to the 128-bit-native `xxh3`) and can be replaced by any `IsHashAlgorithm` struct. Values are not stable across versions and not for persistence. Big-endian targets are supported by construction (byte-assembled loads) but not exercised by CI.
    - function `GetHash32<Algo>(std::string_view, seed)`: 32-bit companion; uses an algorithm's native 32-bit variant where provided, else the XOR-fold of the 64-bit hash.
    - function `GetHash<Algo, Seed>(std::string_view)`: as `GetHash64` but folded through `HashMangle`, so values may differ between builds.
    - concept `HasGetHash32` / `HasGetHash64` / `HasGetHash128`: Detect which static member functions an algorithm struct provides; `IsHashAlgorithm` requires a 64- or 128-bit one.
    - struct `Hasher<Algo>`: Completes any `IsHashAlgorithm` struct into the full `GetHash64`/`GetHash128`/`GetHash` interface, synthesizing what is missing (fold for 64; two decorrelated passes for 128 -- the second skips the first up-to-8 bytes and injects them via the seed; mangle for `GetHash`). Also a transparent functor over `GetHash64`, so it drops into `absl`/`std` hash containers with heterogeneous `string_view` lookup.
    - algorithm structs `mh::Algorithm`, `simple::Algorithm`, `fnv1a::Algorithm`, `xxh64::Algorithm`, `murmur3::Algorithm`: the per-algorithm plug-ins for `GetHash*<Algo>` / `Hasher<Algo>`.
    - function `HashMangle(uint64_t)`: XORs one of a small set of per-build (`__DATE__`/`__TIME__` bucketed) seeds into a hash; identity when compiled with `MBO_HASH_MANGLE=0` (for fully reproducible `GetHash` values).
    - struct `Hash128`: The 128-bit result type (`h1`, `h2`); ordered (`<=>`) and Abseil hash/stringify compatible.
    - function `Hash128To64(Hash128)`: Folds a 128-bit hash into a well-mixed 64-bit one, e.g. `Hash128To64(murmur3::GetHash128(data))`.
    - function `CombineHashes(uint64_t, uint64_t)`: Combines two hashes (order-dependent, well mixed).
    - function `Hash64To32(uint64_t)`: Shrinks a hash to 32 bits by XOR-folding the halves (the correct default for all algorithms, incl. weak-low-bit ones like FNV-1a).
    - concept `HasStreaming` / class `Streamer<Algo>`: incremental hashing (`Update(...).Update(...).Finalize()`), guaranteed equal to the one-shot value; provided by `mh`, `xxh64`, and `siphash` (rapidhash has no canonical streaming form).
    - function `simple::GetHash64(std::string_view)`: the previous hash implementation (`simple::GetHash` is deprecated).
    - function `fnv1a::GetHash64(std::string_view, seed)`: canonical FNV-1a 64 (constexpr-safe).
    - function `xxh64::GetHash64(std::string_view, seed)`: canonical XXH64 / xxHash 64-bit (constexpr-safe).
    - function `xxh3::GetHash64/GetHash128(std::string_view, seed)`: canonical XXH3 64- and 128-bit (modern xxHash generation, the fast file-checksum format; scalar; constexpr-safe).
    - function `murmur3::GetHash64/GetHash128(std::string_view, seed)`: canonical MurmurHash3 x64 128-bit (constexpr-safe; `GetHash64` is the customary `h1` truncation).
    - function `rapidhash::GetHash64(std::string_view, seed)`: canonical rapidhash V3 (wyhash family; best small-key latency; SMHasher3-clean; constexpr-safe). The default algorithm.
    - function `mh::GetHash64/GetHash128(std::string_view, seed)`: this library's own algorithm (fast hot-loop throughput, streaming-capable; not SMHasher3-clean, see mbo/hash/SMHASHER3.md). **Work in progress**: may be optimized, strengthened, replaced, or renamed.
    - function `siphash::GetHash64(std::string_view, key0, key1)` / `siphash::SipHash<C, D>(...)`: canonical SipHash-2-4 (and -1-3 via `GetHash64Sip13`) - keyed, hash-flooding resistant; adversarial protection requires a secret key.
- Json
  - `namespace mbo::json`
  - mbo/json:json_cc, mbo/json/json.h
    - class `Json`: A JSON value/document that can be built from almost any structured type (see `ConvertibleToJson`) and serialized to JSON text.
    - enum `Json::SerializeMode`: Selects `kCompact`, `kLine`, or `kPretty` JSON output.
    - function `Json::Serialize`: Returns the JSON value as a `std::string`.
    - function `Json::Stream`: Writes the JSON value to a `std::ostream`.
    - concept `ConvertibleToJson`: Determines whether a value can be stored in a `Json`.
- Log
  - `namespace mbo::log`
  - mbo/log:demangle_cc, mbo/log/demangle.h
    - functions `Demangle` to log de-mangled typeid names.
  - mbo/log:log_timing_cc, mbo/log/log_timing.h
    - functoin `LogTiming([args])` a simple timing logger.
- Mope
  - `namespace mbo::mope`
  - The `MOPE` templating engine. Run `bazel run //mbo/mope -- --help` for detailed documentation.
  - mbo/mope
    - binary `mope`.
  - mbo/mope:mope_cc, mbo/mope/mope.h
    - class `Template`: The mope template engine and data holder.
  - mbo/mope:ini_cc, mbo/mope/ini.h
    - function `ReadIniToTemplate`: Helper to initialize a mope Template from an INI file.
  - mbo/mope:mope_bzl, mbo/mope/mope.bzl
    - bzl-rule `mope`: A rule that expands a mope template file.
    - bzl-macro `mope_test`: A test rule that compares mope template expansion against golden files. This
      supports `clang-format` and thus can be used for source-code generation and verification.
- Status
  - `namespace mbo::status`
  - mbo/status:status_builder_cc, mbo/status/status_builder.h
    - class `StatusBuilder` which allows to extend the message of an `absl::Status`.
  - mbo/status:status_cc, mbo/status/status.h
    - function `GetStatus` allows to convert types to an `absl::Status`.
  - mbo/status:status_macros_cc, mbo/status/status_macros.h
    - macro `MBO_ASSIGN_OR_RETURN`: Macro that simplifies handling functions returning `absl::StatusOr<T>`.
    - macro `MBO_MOVE_TO_OR_RETURN`: Macro that simplifies handling functions returning `absl::StatusOr<T>` where the result requires commas, in particular structured bindings.
    - macro `MBO_RETURN_IF_ERROR`: Macro that simplifies handling functions returning `absl::Status` or `absl::StausOr<T>`.
- Strings
  - `namespace mbo::strings`
  - mbo/strings:indent_cc, mbo/strings/indent.h
    - function `DropIndent`: Converts a raw-string text block as if it had no indent.
    - function `DropIndentAndSplit`: Variant of `DropIndent` that returns the result as lines.
  - mbo/strings:numbers_cc, mbo/strings/numbers.h
    - function `BigNumber`: Convert integral number to string with thousands separator.
    - function `BigNumberLen`: Calculate required string length for `BigNumer`.
  - mbo/strings:parse_cc, mbo/strings/parse.h
    - function `ParseString`: Parses strings respecting C++ and custom escapes as well as quotes (all configurable).
    - function `ParseStringList`: Parses and splits strings respecting C++ and custom escapes as well as quotes (all configurable).
  - mbo/strings:split_cc, mbo/strings/split.h
    - struct `AtLast`: Allows `absl::StrSplit' to split on the last occurrence of a separator.
  - mbo/strings:strip_cc, mbo/strings/strip.h
    - function `ConsumePrefix`: Removes a prefix from a `std::string` (like `absl::ConsumePrefix`).
    - function `ConsumeSuffix`: Removes a suffix from a `std::string` (like `absl::ConsumeSuffix`).
    - function `StripPrefix`: Removes a prefix from a `std::string&&` (like `absl::StripPrefix`).
    - function `StripSuffix`: Removes a suffix from a `std::string&&` (like `absl::StripSuffix`).
    - struct `StripCommentsArgs`: Arguments for `StripComments` and `StripLineComments`.
    - function `StripComments`: Strips comments from lines.
    - function `StripLineComments`: Strips comments from a single line.
    - struct `StripParsedCommentsArgs`: Arguments for `StripParsedComments` and `StripParsedLineComments`.
    - function `StripParsedComments`: Strips comments from parsed lines.
    - function `StripLineParsedComments`: Strips comments from a single parsed line.
- Testing
  - mbo/testing:matchers_cc, mbo/testing/matchers.h
    - gmock-matcher `CapacityIs` which checks the capacity of a container.
    - gmock-matcher `EqualsText` which compares text using line by line unified text diff.
    - gmock-matcher `IsElementOf` which checks that a value equals at least one element of a container — the element-on-the-left orientation of gmock's `Contains`. Subject types do not need to be converted to the container's `value_type`.
    - gmock-matcher `IsKeyOf` which checks that a value equals at least one key of a map (shorthand for "is this key in the map?").
    - gmock-matcher `IsNullopt` which compares its argument agains `std::nullopt`.
    - gmock-matcher `IsValueOf` which checks that a value equals at least one mapped value of a map (shorthand for "is this value in the map?").
    - gmock-matcher `WhenTransformedBy` which allows to compare containers after transforming them. This sometimes allows for much more concise comparisons where a golden expectation is already available that only differs in a simple transformation.
    - gmock-matcher-modifier `WithDropIndent` which modifies `EqualsText` so that `DropIndent` will be applied to the expected text.
  - mbo/testing:status_cc, mbo/testing/status.h
    - gmock-matcher `IsOk`: Tests whether an `absl::Status` or `absl::StatusOr` is `absl::OkStatus`.
    - gmock-matcher `IsOkAndHolds`: Tests an `absl::StatusOr` for `absl::OkStatus` and contents.
    - gmock-matcher `StatusIs`: Tests an `absl::Status` or `absl::StatusOr` against a specific status code and message.
    - gmock-matcher `StatusHasPayload`: Tests whether an `absl::Status` or `absl::StatusOr` payload map has any payload, a specific payload url, or a payload url with specific content.
    - gmock-matcher `StatusPayloads` Tests whether an `absl::Status` or `absl::StatusOr` payload map matches.
    - macro `MBO_ASSERT_OK_AND_ASSIGN`: Simplifies testing with functions that return `absl::StatusOr<T>`.
    - macro `MBO_ASSERT_OK_AND_MOVE_TO`: Simplifies testing with functions that return `absl::StatusOr<T>` where the result requires commas, in particular structured bindings.
- Types
  - `namespace mbo::types`
  - mbo/types:cases_cc, mbo/types/cases.h
    - meta-type `Cases`: Allows to switch types based on conditions.
    - meta-type `CaseIndex`: Evaluates the first non zero case (1-based, 0 if all zero).
    - meta-type `IfThen`: Helper type to generate if-then `Cases` types.
    - meta-type `IfElse`: Helper type to generate else `Cases` which are always true and must go last.
    - meta-type `IfFalseThenVoid`: Helper type that can be used to skip a case.
    - meta-type `IfTrueThenVoid`: Helper type to inject default cases and to ensure the required type expansion is always possible.
  - mbo/types:compare_cc, mbo/types/compare.h
    - function `CompareArithmetic` which cmpares two values that are scalar-numbers (including foat/double, excluding pointers and references).
    - function `CompareFloat` which can compare two `float`, `double` or `long double` values returning `std::strong_ordering`.
    - function `CompareIntegral` which compares two values that are integral-numbers (no float/double, no pointers, no references).
    - comparator `CompareLess` which is compatible to std::Less but allows container optimizations.
    - function `CompareScalar` which compares two values that are scalar-numbers (including float/double and pointers, excluding references).
    - concept `ThreeWayComparableTo` which is similar to `std::three_way_comparable_with` but we only verify that `L <=> R` can be interpreted as `Cat` in the presented argument order.
    - function `WeakToStrong` which converts a `std::weak_ordering` to a `std::strong_ordering`.
  - mbo/types:container_proxy_cc, mbo/types/container_proxy.h
    - struct `ContainerProxy` which allows to add container access to other types including smart pointers of containers.
  - mbo/types:extend_cc, mbo/types/extend.h
    - crtp-struct `Extend`: Enables extending of struct/class types with basic functionality.
    - crtp-struct `ExtendNoDefault` Like `Extend` but without default extender functionality.
    - crtp-struct `ExtendNoPrint` Like `Extend` but without `Printable` and `Streamable` extender functionality.
    - `namespace extender`
      - extender-struct `AbslStringify`: Extender that injects functionality to make an `Extend`ed type work with abseil format/print functions. See `Stringify` for various API extension points.
      - extender-struct `AbslHashable`: Extender that injects functionality to make an `Extend`ed type work with abseil hashing (and also `std::hash`).
      - extender-struct `Comparable`: Extender that injects functionality to make an `Extend`ed type comparable. All comparators will be injected: `<=>`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
      - extender-struct `Printable`:
        - Extender that injects functionality to make an `Extend`ed type get a `std::string ToString() const` function which can be used to convert a type into a `std::string`.
        - The output is a comma separated list of field values, e.g. `{ 25, 42 }`.
        - If available (Clang 16+) this function prints field names `{ .first = 25, .second = 42 }`.
      - extender-struct `Streamable`: Extender that injects functionality to make an `Extend`ed type streamable. This allows the type to be used directly with `std::ostream`s.
  - mbo/types:no_destruct_cc, mbo/types/no_destruct.h
    - struct `NoDestruct<T>`: Implements a type that allows to use any type as a static constant.
    - Mainly, this prevents calling the destructor and thus prevents termination issues (initialization order fiasco).
  - mbo/types:opaque_cc, mbo/types/opaque.h
    - struct `OpaquePtr` an opaque alternative to `std::unique_ptr` which works with forward declared types.
    - struct `OpaqueValue` an `OpaquePtr` with direct access, comparison and hashing which will not allow a nullptr.
    - struct `OpaqueContainer` an `OpaqueValue` with direct container access.
  - mbo/types:optional_ref_cc, mbo/types/optional_data_or_ref.h
    - concept `IsOptionalDataOrRef` which determines whether a type is a `OptionalDataOrRef`.
    - struct `OptionalDataOrRef` similar to `std::optional` but can hold `std::nullopt`, a type `T` or a reference `T&`/`const T&`.
    - struct `OptionalDataOrConstRef` similar to `std::optional` but can hold `std::nullopt`, a type `T` or a const reference `const T&`.
  - mbo/types:optional_ref_cc, mbo/types/optional_ref.h
    - concept `IsOptionalRef` which determines whether a type is a `OptionalRef`.
    - struct `OptionalRef` similar to `std::optional` but can hold `std::nullopt` or a reference `T&`/`const T&`.
  - mbo/types:ref_wrap_cc, mbo/types/ref_wrap.h
    - template-type `RefWrap<T>`: similar to `std::reference_wrapper` but supports operators `->` and `*`.
  - mbo/types:required_cc, mbo/types/required.h
    - template-type `Required<T>`: similar to `RefWrap` but stores the actual type (and unlike `std::optional` cannot be reset).
  - mbo/types:stringify_cc, mbo/types/stringify.h
    - class `Stringify` a utility to convert structs into strings.
    - function `StringifyWithFieldNames` a format control adapter for `Stringify`.
    - struct `StringifyFieldOptions` which controls outer and inner options (both a `const StringifyOptions&`).
    - struct `StringifyOptions` which can be used to control `Stringify` formatting.
    - struct `StringifyRootOptions` which can be used to control outer/root options for streaming/printing structs.
    - API extension point type `MboTypesStringifySupport` which enables `Stringify` support even if not otherwise enabled (disables Abseil stringify support in `Stringify`).
    - API extension point function `MboTypesStringifyConvert(I, T, V)` allows to control conversion based on field types via a static call to the owning type, receiving the field index, the object and the field value.
    - API extension point type `MboTypesStringifyDisable` which disables `Stringify` support. This allows to prevent
      complex classes (and more importantly fields of complex types) from being printed/streamed using `Stringify`
    - API extension point type `MboTypesStringifyDoNotPrintFieldNames` which if present disables field names in `Stringify`.
    - API extension point function `MboTypesStringifyFieldNames` which adds field names to `Stringify`.
    - API extension point function `MboTypesStringifyOptions` which adds full format control to `Stringify`.
    - API extension point functoin `MboTypesStringifyValueAccess` which allows to replace a struct with a single value in `Stringify` processing.
  - mbo/types:stringify_ostream_cc, mbo/types/stringify_ostream.h
    - operator `std::ostream& operator<<(std::ostream&, const MboTypesStringifySupport auto& v)` - conditioanl automatic ostream support for structs using `Stringify`.
    - function `SetStringifyOstreamOutputMode` which sets global Stringify stream options by mode.
    - function `SetStringifyOstreamOptions` which sets global Stringify stream options.
  - mbo/types:traits_cc, mbo/types/traits.h
    - concept `ConstructibleFrom` implements a variant of `std::constructible_from` that works around its limitations to deal with array args.
    - concept `ConstructibleInto` determines whether one type can be constructed from another. Similar to `std::convertible_to` but with the argument order of `std::constructible_from`.
    - type alias `ContainerConstIteratorValueType` returned the value-type of the const_iterator of a container.
    - concept `ContainerIsForwardIteratable` determines whether a types can be used in forward iteration.
    - concept `ContainerHasEmplace` determines whether a container has `emplace`.
    - concept `ContainerHasEmplaceBack` determines whether a container has `emplace_back`.
    - concept `ContainerHasInsert` determines whether a container has `insert`.
    - concept `ContainerHasPushBack` determines whether a container has `push_back`.
    - concept `ContainerHasForwardIterator` determines whether a container has `begin`, `end` and `std::forward_iterator` compliant iterators.
    - concept `ContainerHasInputIterator` determines whether a container has `begin`, `end` and `std::input_iterator` compliant iterators.
    - type alias `GetDifferenceType` is either set to the type's `difference_type` or `std::ptrdiff_t`.
    - concept `HasDifferenceType` determines whether a type has a `difference_type`.
    - concept `IsAggregate` determines whether a type is an aggregate.
    - concept `IsArithmetic` uses `std::is_arithmetic_v<T>`.
    - concept `IsBracesConstructibleV` determines whether a type can be constructed from given argument types.
    - concept `IsCharArray` determines whether a type is a `char*` or `char[]` related type.
    - concept `IsDecomposable` determines whether a type can be used in static-bindings.
    - concept `IsEmptyType` determines whether a type is empty (calls `std::is_empty_v`).
    - concept `IsFloatingPoint` determines whether a type is a floating point type (uses `std::floating_point`).
    - concept `IsInitializerList` determines whether a type is `std::initializer<T> type.
    - concept `IsScalar` uses `std::is_scalar_v<T>`.
    - concept `IsOptional` determines whether a type is a `std::optional` type.
    - concept `IsPair` determines whether a type is a `std::pair` type.
    - concept `IsReferenceWrapper` determines whether a type is a `std::reference_wrapper`.
    - concept `IsSameAsAnyOf` which determines whether a type is the same as one of a list of types. Similar to `IsSameAsAnyOfRaw` but using exact types. The inversion is available as `NotSameAsAnyOf`.
    - concept `IsSameAsAnyOfRaw` which determines whether a type is one of a list of types. Similar to `IsSameAsAnyOf` but applies `std::remove_cvref_t` on all types. The inversion is available as `NotSameAsAnyOfRaw`.
    - concept `IsScalar` uses `std::is_scalar_v<T>`.
    - concept `IsSet` determines whether a type is a `std::set` type.
    - concept `IsSmartPtr` determines whether a type is a `std::shared_ptr`, `std::unique_ptr` or `std::weak_ptr`.
      - Can be extended with other smart pointers through `IsSmartPtrImpl`.
    - concept `IsStringKeyedContainer` which determines whether a type is a container whose elements are pairs and whose keys are convertible to a std::string_view.
    - concept `IsTuple` determines whether a type is a `std::tuple` type.
    - concept `IsVector` determines whether a type is a `std::vector` type.
  - mbo/types:template_search_cc, mbo/types/template_search.h:
    - template struct `BinarySearch` implements templated binary search algorithm.
    - template struct `LinearSearch` implements templated linear search algorithm.
    - template struct `ReverseSearch` implements templated reverse linear search algorithm.
    - template struct `MaxSearch` implements templated linear search for last match algorithm.
  - mbo/types:tstring_cc, mbo/types/tstring.h
    - struct `tstring`: Implements type `tstring` a compile time string-literal type.
    - operator `operator"" _ts`: String literal support for Clang, GCC and derived compilers.
  - mbo/types:tuple_cc, mbo/types/tuple.h
    - template struct `TupleCat` which concatenates tuple types.
  - mbo/types:typed_view_cc, mbo/types/typed_view.h
    - template struct `TypedView` a wrapper for STL views that provides type definitions, most importantly `value_type`. That allows such views to be used with GoogleTest container matchers.
  - mbo/types:variant_cc, mbo/types/variant.h
    - concept `IsVariant` determines whether a type is a `std::variant` type.
    - concept `IsVariantMemberType` determine whether a `Type` is any of the types in a `Variant`.
    - struct `Overloaded` which implements an Overload handler for `std::visit(std::variant<...>)` and `std::variant::visit`.

## Installation and requirements

This repository requires a C++20 compiler (in case of MacOS XCode 15 is needed). This is done so that newer features like `std::source_location` can be used.

The project only comes with a Bazel BUILD.bazel file and can be added to other Bazel projects.

The project is formatted with specific clang-format settings which require clang 16+ (in case of MacOs LLVM 16+ can be installed using brew). For simplicity in dev mode the project pulls the appropriate clang tools and can be compiled with those tools using `bazel [build|test] --config=clang ...`.

Lint and format are driven by [Trunk](https://docs.trunk.io/cli). Devs are **required** to install the CLI locally — `curl https://get.trunk.io -fsSL | bash` — then run `trunk check` and `trunk fmt` before pushing. The repo enables `trunk-fmt-pre-commit` and `trunk-check-pre-push` so the hooks run automatically once installed. CI runs `trunk check` only (no auto-fixing on the GitHub side); failing lint must be fixed locally and re-pushed.

### MODULES.bazel

Check [Releases](https://github.com/helly25/mbo/releases) for details. All that is needed is a `bazel_dep` instruction with the correct version.

```starlark
bazel_dep(name = "helly25_mbo", version = "0.0.0")
```

The [Bazel-Central-Registry](https://registry.bazel.build/modules/helly25_mbo) installation does not provide the LLVM tools and thus does not come with its own compiler — a restriction in how Bazel handles toolchains under bzlmod. To pull in the bundled toolchain, vendor `bazelmod/llvm.MODULE.bazel` as described in the release notes. Nonetheless all versions can be compiled with GCC 11+, Clang 17+ on Ubuntu and MacOs as enforced by CI. Other platforms and compilers are likely to work as well. However, Windows lacks some of the necessary tools and the library as well as its build system mostly assume Unix-style file and path names. That unfortunately means that on Windows some code cannot even be built.

## Presentations

## Third-party components

The hash library contains constexpr transcriptions of third-party algorithms
(rapidhash, xxHash/XXH3, MurmurHash3, SipHash, FNV-1a). Upstream copyright
notices and license texts are reproduced in the repository-root
[NOTICE](NOTICE) file; the project itself is Apache-2.0 (see
[LICENSE](LICENSE)).

### Practical Production-proven Constexpr API Elements

Presented at [C++ On Sea 2024](https://cpponsea.uk/2024/session/practical-production-proven-constexpr-api-elements), this presentation covers the theory behind:

- `mbo::hash::simple::GetHash64` (formerly `mbo::hash::simple::GetHash`),
- `mbo::container::LimitedVector`,
- `mbo::container::LimitedMap`, and
- `mbo::container::LimitedSet`.

Slides are available at:
<br/>
<br/>
[<img src="https://helly25.com/wp-content/uploads/2024/07/Practical-Production-proven-Constexpr-API-Elements_TitleCard-copy-980x551.png" alt="Practical Production Proven constexpr slides">](https://helly25.com/practical-production-proven-constexpr-api-elements/)
