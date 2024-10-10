Some rules for the code layout and its development.

* Everything is under Apache 2 license, see fle `LICENSE`.
* All sources must be unix-text files: https://en.wikipedia.org/wiki/Text_file
  * Lines end in {LF}.
  * The files are either empty or end in {LF}.
* This library uses C++20 and is intended to be compiled with `clang`.
  * Reasonable effort is performed to ensure compilation with GCC is functional.
* All code must be formatted using `clang-format` with '.clang-format'.
  * This provides consistent formatting.
  * The choices are meant to enable fast structural reading by humans.
  * It cares less about writing because there is auto formatting and code is
    read much more often then written.
  * Auto formatting also prevents pointless discussions like where '*' goes.
  * The guide mostly follows Google style: https://google.github.io/styleguide/
* All exported library code is in the directory 'mbo'.
* Directory 'mbo' has no actual library rules, but may have test rules.
* No namespace may be named 'internal'.
* Each subdirectory:
  * has all its code in a namespace: "mbo::{dir}"
  * there may be functional sub-namespace, e.g.: "mbo::{dir}::{sub}"
  * internal code goes into special sub-namespace:
    * Code in these namespace is not allowed to be used outside their library
    * "mbo::{dir}::{dir}_internal
    * "mbo::{dir}::{sub}_internal
    * "mbo::{dir}::{sub}::{sub}_internal
    * This ensures that "internal" namespace do not conflict.
    * No namespace component `internal` is otherwise allowed.
  * Sub namespaces 'detail' are allowed.
    * Such sub-namespaces imply usable (exported) functionality
    * "mbo::{dir}::detail
    * "mbo::{dir}::{sub}::detail
    * In contrast to internal namespaces the detail namespaces use just "detail"
      which can result in namespace conflicts. To avoid that they have to be
      fully qualified.
* Forward declared symbols must be fully implemented in the same source file.
  * The exception are 'Impl' classes / structs in a header's class when the
    actual implementation is in a name-corresponding implementation file. This
    is useful for visibility and name resolution.
* All public / exported code must:
  * be tested,
  * have a documentaion.
* All headers must have macro-guards: "{PATH}_{FILE}_". That is path and filename in all caps, with all non alphanumeric chars replaced with '_' and followed by a final '_'.
  * A '__' betwen {PATH} and {FILE} would be better, but C++ does not allow it.
  * Filnames may not start with the basename of any directory in their same
    directory followed by an '_' as that would lead to conflicting macro guards.
    E.g.: "foo/bar.h" and "foo_bar.h" would have macro guard "FOO_BAR_H_".
* Macros are to be avoided at all costs, or prefixed by 'MBO_'.
  * Macros for local application must be undefined as close after the last use
    as possible.
  * Where possible macros should be named "{PATH}_{FILE}_{USECASE}". As a minimum they should have the prefix `MBO_`.
  * Private macro implementation details should be named "MBO_PRIVATE_{PATH}_{FILE}_{USECASE}_". That is, prefix `MBO_PRIVATE_` followed by all caps path/filename with all non alphanumeric chars replaced with `_`, followed by a final `_`.
* Use `std::size_t` as `size_t` **ONLY** in CC files with `using std::size_t`.
* Flags in libraries should be prefixed with their path/namespace. E.g. the flag
  `--mbo_log_timing_min_duration` has the prefix `mbo_log` as it is defined in
  `mbo/log/log_timing.cc` (path `mbo/log`) and uses namespace `mbo::log`.
