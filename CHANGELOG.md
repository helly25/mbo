# 0.1

* Initial release: Clang support only.

# 0.2

* Add support for GCC (11.4/Ubuntu 22.04).
* Change Print Extender's `Print` to `ToString` which is a more widely used name.
* Change Print Extender's `ToString` to print field names (if available, e.g. Clang 16).
* When compiling with Clang show field names with the `AbslFormatImpl` extender.
* Enable `static constexpr NoDestruct<>` for more cases and compiler versions.
