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
- **`clang-tidy`** with [`.clang-tidy`](.clang-tidy) lints all C++ code with
  `WarningsAsErrors: true`, so every finding is a build failure. The enabled set is
  broad: `abseil-*`, `bugprone-*`, `cppcoreguidelines-*`, `google-*`, `misc-*`,
  `modernize-*`, `performance-*`, `portability-*`, `readability-*`.

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
- Macros: avoid them; if unavoidable prefix `MBO_` (private impl `MBO_PRIVATE_...`) and
  `#undef` a local macro as soon after its last use as possible.
- `std::size_t`: use the short `size_t` only in `.cc` files, via `using std::size_t`.
- Library flags are prefixed by their path/namespace (e.g. `--mbo_log_...`).

## Formatting conventions on top of clang-format

clang-format picks a layout per line; these two habits steer it toward the readable one.

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

## Idioms

- **Pass `absl::Status` by value**, not `const&`: it is a tagged pointer, and
  `.clang-tidy performance-unnecessary-value-param` allowlists it (with `absl::StatusOr`
  and `std::string_view`). `StatusOr<T>`'s cost depends on `T`.
- **Prefer container algorithms** from `absl/algorithm/container.h` (`absl::c_contains`,
  `c_any_of`, `c_find`, `c_sort`, ...) or C++20 `std::ranges` over hand-rolled loops:
  range-based (no begin/end), well-named, constexpr-friendly.
- **switch / case**: order case labels alphabetically. Where the cases are a uniform
  mapping (key -> handler or value), prefer a constexpr `mbo::container::LimitedMap`
  lookup over a switch.
- **No em-dashes** anywhere (code, comments, docs, commit messages). Use a spaced
  hyphen `-`.

## Testing (GoogleTest / GoogleMock)

All exported code must be tested. Tests use GoogleTest + GoogleMock with these conventions.

### Structure

- **Always `TEST_F` with a fixture**, never a bare `TEST`. Even an empty
  `struct FooTest : ::testing::Test {};` is preferred, so shared setup has a home.
- One behaviour per test; name the test for the behaviour it asserts.

### Assertions: gmock matchers, not `EXPECT_EQ`

- Assert with **`EXPECT_THAT` / `ASSERT_THAT` + a matcher**, not `EXPECT_EQ` /
  `ASSERT_EQ`: matchers compose and give far better failure messages.
- **No redundant `Eq` for POD/strings.** `EXPECT_THAT(x, value)` auto-wraps a bare
  value in `Eq`, so write `EXPECT_THAT(name, "foo")`, not `EXPECT_THAT(name, Eq("foo"))`.
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
- `ASSERT_OK_AND_ASSIGN(const auto value, MakeThing())` asserts OK and binds in one step.

### Shell / binary-level tests

- Use **helly25/bashtest** (`bazel_dep(name = "helly25_bashtest")`), not a hand-rolled
  `sh_test`: `load("@com_helly25_bashtest//bashtest:bashtest.bzl", "bashtest")`, then a
  script that `source`s `"${helly25_bashtest}"`, defines `test::name()` functions using
  `expect_eq` / `expect_contains` / `expect_not_contains`, and ends with `test_runner`.
- It runs under macOS bash 3.2: **no `mapfile` / `readarray`** or other bash-4 features.
  Read lines with `while IFS= read -r line; do arr+=("$line"); done <<< "$out"`.
