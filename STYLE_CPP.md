# C++ Style

The C++ coding style for helly25 repositories. It sits on top of two
machine-enforced config files and adds the human conventions below. When in doubt
the config files win; this document explains and extends them so a contributor (or
an AI assistant) can follow them without reverse-engineering the tooling.

## Toolchain (machine-enforced)

- C++20, compiled with `clang`. Compilation with GCC is kept working on a
  best-effort basis.
- **`clang-format`** with [`.clang-format`](.clang-format) formats all C++ code. Run
  it; do not hand-format against it. CI rejects any reformatting diff.
- **`clang-tidy`** with [`.clang-tidy`](.clang-tidy) (`WarningsAsErrors: true`) runs **locally**
  via `trunk` (a `trunk check` and the editor daemon) against a `compile_commands.json` you
  generate with `bazel run @hedron_compile_commands//:refresh_all`. **CI does not run it** (no
  compile DB there), so CI's hard gate is the compiler `-Werror` in the bazel matrix; still treat
  a clang-tidy finding as a must-fix before pushing. The enabled set is broad: `abseil-*`,
  `bugprone-*`, `cppcoreguidelines-*`, `google-*`, `misc-*`, `modernize-*`, `performance-*`,
  `portability-*`, `readability-*`.

### What `.clang-format` decides (do not fight it)

- Google base style, **column limit 120**.
- `AlignAfterOpenBracket: AlwaysBreak` + `BinPackParameters: false`: when arguments
  do not fit, each goes on its own line at one indent (not aligned to the paren).
- `PointerAlignment: Left` (`int* p`), `QualifierAlignment: Left` (`const int`).
- `IntegerLiteralSeparator` decimal every 3: write `1'000'000`.
- `RemoveSemicolon`, `InsertBraces` (always brace bodies), `SeparateDefinitionBlocks`.
- **`InsertTrailingCommas` is deliberately OFF** (it would force every aggregate to
  never-bin-pack). That is the lever behind the trailing-comma rule below: a _manual_
  trailing comma opts a _single_ aggregate into one-element-per-line.

### Naming (enforced by `.clang-tidy readability-identifier-naming`)

- Types / classes / structs / enums / aliases / functions: `CamelCase` (`LimitedMap`).
- Variables / parameters / data members / namespaces: `lower_case` (`flag_count`).
- Private data members: `lower_case` with a trailing `_` (`flags_`).
- constexpr / enum constants / global + static constants: `k` + `CamelCase` (`kMaxDepth`).
- Macros: `UPPER_CASE`, prefixed `MBO_` (`MBO_...`).

## Code organization

- All exported code lives under the top library directory (in mbo: `mbo/`).
- Each subdirectory uses namespace `mbo::{dir}` (and `mbo::{dir}::{sub}`).
  - Internal-only code: `mbo::{dir}::{sub}_internal`. No namespace component is ever
    just `internal`.
  - Exported-but-detail code may use a `detail` sub-namespace (fully qualify it, since
    the bare `detail` name can collide).
- Header guards: `{PATH}_{FILE}_` (path + filename, uppercased, non-alphanumerics ->
  `_`, trailing `_`). A file may not start with a sibling directory's basename + `_`:
  `foo/bar.h` and `foo_bar.h` would both yield `FOO_BAR_H_`.
- Forward-declared symbols must be fully implemented in the same source file (except
  the `Impl`-in-header / implementation-in-`.cc` pattern).
- Macros: avoid them. An **exported** macro (defined in a header for other code to use) is
  prefixed `MBO_` (private impl `MBO_PRIVATE_...`). A macro **local to one translation unit**
  - a `.cc` / test file, or a header helper you `#undef` right after its last use - may stay
    unprefixed (still `UPPER_CASE`); `#undef` a local macro as soon after its last use as
    possible.
- **C-library names: use the `std::` form; the unqualified global alias is optional.** A
  `<cstddef>` / `<cXXX>` header is only required to declare `size_t`, `int64_t`, `memcpy`,
  ... in `namespace std`; whether it _also_ injects the bare global name is
  implementation-defined, so we never rely on it.
  - In **headers**, always fully qualify (`std::size_t`); no namespace-level `using`.
  - In an **implementation file** (`.cc`), a `using std::size_t;` (or other `using std::...`)
    is fine where it reads better - bring the name in explicitly rather than leaning on the
    global alias. Be consistent within a file, but readability outranks consistency.
- Library flags are prefixed by their path/namespace (e.g. `--mbo_log_timing_min_duration`
  in `mbo/log`). Application entry-point binaries (`glob`, `diff`, `mope`) may use bare flag
  names (`--depth`, `--algorithm`, `--template`).

## Formatting conventions on top of clang-format

clang-format picks a layout per line; these habits steer it toward the readable one.

1. **No comment at the end of a long line.** Put the comment on its own line _above_
   the element. A trailing `// ...` that pushes a line past 120 makes clang-format
   explode the element across several lines.

   ```cpp
   // -exec run in the matched entry's directory
   {.name = "-execdir", .kind = Kind::kAction, .arity = -1},
   ```

2. **Trailing comma on the last field of a complex aggregate** breaks it one field per
   line. Because `InsertTrailingCommas` is off, the comma is your per-aggregate opt-in.
   Use it for long/complex initializers; a short one that reads on a single line stays.

   ```cpp
   const Drop drop{
       .line = line,
       .layer = Source::kProject,
       .safety = Safety::kSecurity,
   };
   ```

3. **Force a line break with a comment rather than let clang-format cram a value at the right
   margin.** A long argument - especially a raw string such as a proto `R"pb(...)pb"` - otherwise
   gets packed onto the call line and shoved against the 120 column, unreadable. A trailing
   comment makes clang-format keep the element on its own line. Mark it `// NL` ("new line"):

   ```cpp
   EXPECT_THAT(  // NL
       message,
       EqualsProto(R"pb(name: "n" value: 1)pb"));
   ```

   When there is a relevant reason, keep the prefix and add it: `// NL: <short reason>`, preferred
   over a bare `// NL`. Always keep the `NL` prefix - do not drop to a bare `//` or an unprefixed
   comment - for two reasons: it marks the comment as load-bearing for layout, so a reader knows
   that removing it re-crams the line; and the consistent marker is machine-checkable, so a
   pre-commit rule can verify these lines stay broken.

   Reserve `// clang-format off` / `on` for a genuine table or hand-aligned expression that
   clang-format cannot lay out - `// NL` only _inserts_ breaks, so it cannot keep an over-120
   line whole or stop a reflow (e.g. a one-line-per-case test table, or a multi-clause
   `requires(...)`). Do not use it to hand-place ordinary layout, and keep the `off`/`on` pair
   tight and adjacent so the `on` is never forgotten: everything between the two loses every
   formatting guarantee above.

## Idioms

- **Pass `absl::Status` by value**, not `const&`: it is a tagged pointer, and
  `.clang-tidy performance-unnecessary-value-param` allowlists it (with `absl::StatusOr`
  and `std::string_view`). `StatusOr<T>`'s cost depends on `T`.
- **Prefer container algorithms** from `absl/algorithm/container.h` (`absl::c_contains`,
  `c_any_of`, `c_find`, `c_equal`, `c_sort`, ...) or C++20 `std::ranges` over hand-rolled
  loops: range-based (no begin/end), well-named, constexpr-friendly. A loop that returns
  on the first match is an `absl::c_any_of` with a lambda; a reserve-then-push copy into a
  `std::vector` is range construction `std::vector<T>(src.begin(), src.end())`. Keep a raw
  loop only when it builds a non-trivial structure no algorithm expresses cleanly.
- **Container choice: prefer the Abseil containers over the bare `std::` ones.** The reason is
  in their favor, not against `std::`: the Abseil variants are faster, support transparent
  (heterogeneous) lookup - find in a `std::string`-keyed map with a `std::string_view`, no
  temporary `std::string` - and `flat_hash_*` stores elements inline for lower memory.
  - **Never `std::unordered_*`** - no `<unordered_map>` / `<unordered_set>` include, and none
    of `unordered_map` / `unordered_set` / `unordered_multimap` / `unordered_multiset`. Use
    `absl::flat_hash_map` / `flat_hash_set`, or the `node_hash_map` / `node_hash_set` variants
    when you need pointer/reference stability. This holds in tests too - the one exception is a
    container-compatibility test that deliberately exercises a `std::unordered_*` input.
  - **Avoid `std::map` / `std::set`** (and the `multi` variants) in library code; prefer
    `absl::btree_map` / `absl::btree_set`, which keep the same ordered interface but are
    cache-friendlier. In tests, `std::map` / `std::set` are fine.
  - Small compile-time tables: `mbo::container::LimitedMap` / `LimitedSet`.
  - A type-detection trait may name a `std::` container type freely - it must, to recognize it.
- **A by-value `std::string_view` is never `const`.** The characters it views are already
  `const`; making the view itself `const` only disables the view's own API
  (`remove_prefix`, `remove_suffix`, reassignment) for no benefit. This applies to locals
  and range-`for` variables. (Not to `std::string_view::size_type`, which is a `size_t`,
  nor to `absl::Span<const std::string_view>`, where `const` is the element type of an
  immutable span.)
- **Do not overload `const T*` to express "optional" or to conflate omission with value.** A
  raw pointer mixes nullability, the pointed-at value, and ownership/lifetime into one type
  the caller has to second-guess. For an optional reference use
  `std::optional<std::reference_wrapper<T>>` or, preferably, mbo's `mbo::types::OptionalRef<T>`
  (`mbo/types/optional_ref.h`) and its related types (e.g. `OptionalDataOrRef` when it may own
  a value or refer to one).
- **Mark a return value `[[nodiscard]]`** when silently ignoring it is a bug - an
  `absl::Status` / `StatusOr`, a parsed result, a `Consume...` "did it match?" flag, an acquired
  handle. The exception is a function _designed_ for its result to be optionally used: a builder
  method returning `*this` for chaining, or a mutator that also returns the previous value as a
  convenience. We disable `modernize-use-nodiscard` in `.clang-tidy` precisely because it would
  blanket-annotate every const method - apply `[[nodiscard]]` by judgment instead.
- **switch / case**: order case labels alphabetically. Where the cases are a uniform
  mapping (key -> handler or value), prefer a constexpr `mbo::container::LimitedMap`
  lookup over a switch.
- **No em-dashes** anywhere (code, comments, docs, commit messages). Use a spaced
  hyphen `-`.

## Error handling: `absl::Status` and the MBO status macros

Propagate errors with the macros from `mbo/status/status_macros.h`
(`@helly25_mbo//mbo/status:status_macros_cc`), not a hand-written
`if (!x.ok()) return x.status();`.

- **`MBO_RETURN_IF_ERROR(expr)`** evaluates a `Status` or `StatusOr` and returns early if
  it is not OK. It returns a `mbo::status::StatusBuilder`, which converts to the calling
  function's `absl::Status` or `absl::StatusOr<T>`, so it works in both.
- **`MBO_ASSIGN_OR_RETURN(Type var, expr)`** binds the value of a `StatusOr<Type>` to
  `var` (a new declaration - carry the type - or an existing variable) or returns the
  status. After it, `var` is the value (not a `StatusOr`); use `var`, not `*var`.
- **`MBO_MOVE_TO_OR_RETURN(expr, target)`** is the variant whose target may contain commas;
  the expression comes first so structured bindings work:
  `MBO_MOVE_TO_OR_RETURN(MakePair(), auto [a, b]);`.

```cpp
absl::StatusOr<Report> Build(std::string_view path) {
  MBO_ASSIGN_OR_RETURN(const Config config, LoadConfig(path));  // value, or return status
  Report report = Analyze(config);
  MBO_RETURN_IF_ERROR(Persist(report));                         // Status guard
  return report;
}
```

- When you only need the guard but the `StatusOr` value is large and used later through
  `*expr`, guard on the status to avoid copying the value: `MBO_RETURN_IF_ERROR(big.status());`
  then use `*big`.
- **Do not force the macros where they do not fit**: a recovery branch (e.g.
  `absl::IsNotFound(s)` -> return a default value), a function that returns `bool`/`int`
  rather than a status, or a CLI path that prints a message and returns an exit code.
- Pass `absl::Status` by value (see Idioms).

## Output, logging, and `AbslStringify`

- **Never use C strings or `.c_str()`** except to satisfy a C / system API (e.g. building
  `char* argv[]` for `execvp`); comment why at that one boundary. Keep `std::string` /
  `std::string_view` end to end - the round-trip out to a `const char*` and back is
  clutter, not interop.
- **Never write output through C stdio** (`printf` / `fprintf` / `fputs` to the `stdout` /
  `stderr` `FILE*`). Use the C++ streams `std::cout` / `std::cerr` (or the logging library).
  The anti-pattern to delete on sight: `std::fputs(absl::StrCat(...).c_str(), stderr)`.
- Format with Abseil: `absl::StrCat` for plain concatenation, `absl::StrFormat` for a
  formatted string, and **`absl::StreamFormat` to write a formatted line straight to a
  stream with no temporary**: `std::cerr << absl::StreamFormat("wrote %d entries\n", n);`.
  A plain `<<` is fine for a simple string. Choose per line, but **be consistent within a
  single function** (do not mix a `StreamFormat` and a `<<` chain in the same function).
- **Make your types printable with `AbslStringify`, not a hand-rolled `operator<<` /
  `ToString`.** Abseil's string-conversion extension point is a hidden-friend hook:

  ```cpp
  struct Point {
    int x = 0;
    int y = 0;
    template<typename Sink>
    friend void AbslStringify(Sink& sink, const Point& p) {
      absl::Format(&sink, "(%d, %d)", p.x, p.y);
    }
  };
  ```

  One hook makes the type work with `absl::StrCat`, `absl::StrFormat` / `StreamFormat`
  `%v`, `absl::StrJoin`, and Abseil logging - and gives GoogleTest readable failure output.
  `absl::Status` / `StatusOr` already stringify; print them with `%v`.

## Concurrency and thread safety

These are the baseline for any multi-threaded code, not extra credit; threading bugs are
silent and data-dependent, so the annotations and the sanitizer are the standing guard.
Most of this repo is single-threaded today, so this section is the standard to apply _when
you add_ multi-threaded code, not a description of current breadth.

- **Use `absl::Mutex` + `absl::MutexLock`**, not `std::mutex` / `std::lock_guard` or
  atomics-as-synchronization. Construct the lock from a reference: `absl::MutexLock lock(mu_);`
  (the pointer constructor is deprecated).
- **Annotate everything** (`absl/base/thread_annotations.h`): `ABSL_GUARDED_BY(mu_)` on
  every member the mutex protects, and `ABSL_LOCKS_EXCLUDED` /
  `ABSL_EXCLUSIVE_LOCKS_REQUIRED` on methods. A comment must state **exactly which members a
  given mutex guards**, and call out any shared state deliberately left unguarded together
  with the invariant that keeps it safe (e.g. "each index is handed to exactly one worker,
  so distinct elements never alias").
- **If a type has more than one mutex, document their lock-acquisition order** (which is
  taken before which) so the ordering that prevents deadlock is explicit.
- **Enforce the annotations**: compile first-party code with clang's `-Wthread-safety`
  (with `-Werror` an unguarded access becomes a build failure), and add a
  **ThreadSanitizer CI job** that runs the threaded tests so races are also caught at
  runtime. Scope the tsan job to the threaded targets; exclude heavy third-party-linked
  ones so tsan does not rebuild them.

## Protocol Buffers

- **Generated repeated fields and maps are STL-compatible - range-iterate them**, do not
  index with `field_size()` + `field(i)`. `field_size()` is still fine for `reserve()` and
  `== 0` emptiness checks; compare two repeated fields with `absl::c_equal`.
- **Edition 2024**: string accessors default to `string_type = VIEW` and return
  `std::string_view` (so the by-value-`string_view` rule applies to them); singular fields
  have **explicit presence** by default, so `set_x(0)` serializes a `0`. To mean "absent",
  leave the field unset (never call its setter) rather than setting the default.
- **Build test protos from text, not setters**:
  `mbo::proto::ParseTextProtoOrDie(R"pb(field: 1 nested { k: "v" })pb")`
  (`@helly25_proto//mbo/proto:parse_text_proto_cc`). The `R"pb(...)pb"` raw string is what
  clang-format leaves alone, so the proto stays readable. Do not imperatively `set_` / `add_`
  your way to a fixture.
- **Assert on protos structurally** with `mbo::proto::EqualsProto`
  (`@helly25_proto//mbo/proto:matchers_cc`), which also accepts a text-proto string:
  `EXPECT_THAT(msg, EqualsProto(R"pb(field: 1)pb"));`. Match a subset with
  `Partially(EqualsProto(...))`. Never compare serialized strings.

## Testing (GoogleTest / GoogleMock)

All exported code must be tested, at every level (unit, integration, and end-to-end where
it applies). A one-shot manual check or a script you ran once is planning input, never a
substitute for a committed test. Tests use GoogleTest + GoogleMock with these conventions.

### Structure

- **Always `TEST_F` with a fixture**, never a bare `TEST`. Even an empty
  `struct FooTest : ::testing::Test {};` is preferred, so shared setup has a home.
- One behaviour per test; name the test for the behaviour it asserts.

### Assertions: gmock matchers, not `EXPECT_EQ`

- Assert with **`EXPECT_THAT` / `ASSERT_THAT` + a matcher** rather than the comparison macros
  `EXPECT_EQ` / `NE` / `GT` / `LT` / `GE` / `LE` (and their `ASSERT_` forms): matchers compose and
  give far better failure messages. The accepted exception is the boolean `EXPECT_TRUE` /
  `EXPECT_FALSE` (and `ASSERT_TRUE` / `ASSERT_FALSE`), which read fine on their own. Within a
  single test keep one style - do not mix, say, `EXPECT_TRUE(x)` and `EXPECT_THAT(y, IsTrue())`.
- **`Eq` is optional - a readability choice, not a rule.** `EXPECT_THAT(x, value)` auto-wraps a
  bare value in `Eq`, so both forms compile. The value of `EXPECT_THAT` is that the line reads as
  a sentence - `EXPECT_THAT(foo, Eq(25))` is "expect that foo equals 25" - so keep `Eq` where it
  makes the assertion read that way, and drop it where the value already carries the meaning
  (`EXPECT_THAT(name, "foo")`). Inside composite matchers prefer bare elements
  (`ElementsAre(1, 2, 3)`); `Optional` / `Pointee` are the exception and need the inner `Eq` (see
  below). Strings sometimes need `StrEq` (e.g. a `char*` subject, where bare `Eq` compares
  pointers). For booleans, `IsTrue()` / `IsFalse()` usually read better than a bare `true` /
  `false` or `Eq(true)`: `EXPECT_THAT(found, IsTrue())`.
- **Floats / doubles**: never `Eq` / `==`; use `FloatEq` / `DoubleEq` (or `Near`).
- **Optionals**: `EXPECT_THAT(opt, Eq(std::nullopt))` for empty, `Optional(...)` for a
  value. Nested matchers do **not** auto-wrap: `Optional(Eq("x"))` and `Pointee(Eq("x"))`
  need the explicit inner `Eq` (a bare value fails to compile there).
- **Containers**: `ElementsAre(...)` / `UnorderedElementsAre(...)` / `SizeIs(n)` /
  `IsEmpty()` cover size + order + contents in one matcher, instead of an
  `ASSERT_EQ(v.size(), n)` followed by indexed `EXPECT_EQ`s. `ElementsAre` auto-wraps
  each bare element in `Eq`.
- **Struct elements**: fold per-field checks into the element matcher with the
  **3-arg, named** `Field("member", &T::member, m)` + `AllOf`, ideally via a small
  `testing::Matcher<T> FooIs(...)` helper, so a mismatch names the field rather than
  reporting "whose given field is...".

  ```cpp
  testing::Matcher<ResolvedFlag> FlagIs(const std::string& flag, Source source) {
    return AllOf(Field("flag", &ResolvedFlag::flag, flag), Field("source", &ResolvedFlag::source, source));
  }
  EXPECT_THAT(Resolve(in), ElementsAre(FlagIs("--color", Source::kUser), FlagIs("--sort", Source::kUser)));
  ```

### Status matchers (`mbo/testing`)

- `EXPECT_THAT(status_or, mbo::testing::IsOk())`, or
  `StatusIs(absl::StatusCode::kInvalidArgument)` to match a specific code.
- **`IsOkAndHolds(m)`** matches an OK `StatusOr` whose value matches `m` - prefer it over
  `IsOk()` followed by dereferencing: `EXPECT_THAT(Parse(in), IsOkAndHolds(SizeIs(3)))`.
- `MBO_ASSERT_OK_AND_ASSIGN(const auto value, MakeThing())` asserts OK and binds in one step.
- `MBO_ASSERT_OK_AND_MOVE_TO(MakePair(), auto [a, b])` is the move variant whose target may
  contain commas (so the expression comes first) - the test mirror of `MBO_MOVE_TO_OR_RETURN`,
  for structured bindings and move-only types.

### Protocol-buffer fixtures and matchers

Build proto test data with `mbo::proto::ParseTextProtoOrDie(R"pb(...)pb")` and assert with
`mbo::proto::EqualsProto` / `Partially(EqualsProto(...))` - never imperative setters or
serialized-string comparison. See the Protocol Buffers section.

### Shell / binary-level tests

- Use **helly25/bashtest** (`bazel_dep(name = "helly25_bashtest")`), not a hand-rolled
  `sh_test`: `load("@helly25_bashtest//bashtest:bashtest.bzl", "bashtest")`, then a
  script that `source`s `"${helly25_bashtest}"`, defines `test::name()` functions using
  `expect_eq` / `expect_contains` / `expect_not_contains`, and ends with `test_runner`.
- It runs under macOS bash 3.2: **no `mapfile` / `readarray`** or other bash-4 features.
  Read lines with `while IFS= read -r line; do arr+=("$line"); done <<< "$out"`.
