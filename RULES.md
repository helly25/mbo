Some rules for the code layout and its development.

* Everything is under Apache 2 license, see fle `LICENSE`.
* All sources must be unix-text files: https://en.wikipedia.org/wiki/Text_file
  * Lines end in {LF}.
  * The files are either empty or end in {LF}.
* This library uses C++20 and is intended to be compiled with `clang`.
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
    * Code in these namespace is not allowed to be used outside this library
    * "mbo::{dir}::{dir}_internal
    * "mbo::{dir}::{sub}_internal
    * "mbo::{dir}::{sub}::{sub}_internal
    * This ensures that "internal" namespace do not conflict.
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
* All headers must have macro-guards: "{path}_{file}_" with '/' becoming '_'.
  * A '__' betwenn {path} and {file} would be better, but C++ does not allow it.
  * Filnames may not start with the basename of any directory in their same
    directory followed by an '_' as that would lead to conflicting macro guards.
    E.g.: "foo/bar.h" and "foo_bar.h" would have macro guard "FOO_BAR_H_".
* Macros are to be avoided at all costs, or prefixed by 'MBO_'.
  * Macros for local application must be undefined as close after the last use
    as possible.
  * Where possible macros should be named "{path}_{file}_{usecase}".
* Use `std::size_t` as `size_t`.
