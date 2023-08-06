# 0.1

* Initial release: Clang support only.

# 0.2

* Add support for GCC (11.4/Ubuntu 22.04).
* Change Print Extender's `Print` to `ToString` which is a more widely used name.
* Change Print Extender's `ToString` to print field names (if available, e.g. Clang 16).
* When compiling with Clang show field names with the `AbslFormatImpl` extender.
* Enable `static constexpr NoDestruct<>` for more cases and compiler versions.
* Addded:
  * Concept `ContainerIsForwardIteratable` determines whether a types can be used in forward iteration.
  * Concept `ContainerHasEmplace` determines whether a container has `emplace`.
  * Concept `ContainerHasEmplaceBack` determines whether a container has `emplace_back`.
  * Concept `ContainerHasInsert` determines whether a container has `insert`.
  * Concept `ContainerHasPushBack` determines whether a container has `push_back`.
  * Conversion struct `CopyConvertContainer` simplifies copying containers to value convertible containers.
