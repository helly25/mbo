# MBO, a C++20 library

This C++20 library provides some general useful building blocks and integrates
with [Google's Abseil library](https://abseil.io/).

The library is organized in functional groups each residing in their own directory.

* Diff
    * `namespacembo::diff`
    * mbo/diff:unified_dff_cc, mbo/diff/unified_diff.h
        * `UnifiedDiff`
    * mbo/diff:unified_diff
        * Program `unfied_diff`
* Files
    * `namespace mbo::files`
    * mbo/file:artefact_cc, mbo/file/artefact.h
        * `Artefact`
    * mbo/file:file_cc, mbo/file.file.h
        * `GetContents`, `GetTime`, `IsAbsolutePath`, `JoinPaths`, `NormalizePath`, `Readable`, `SetContents`, 
* Strings
    * `namespace mbo::strings`
    * mbo/strings:indent_cc, mbo/strings/indent.h
        * `DropIndent`, `DropIndentAndSplit`
* Testing
    * `namespace mbo::testing`
    * mbo/testing:status_cc, mbo/testing/status.h
        * `IsOk`, `IsOkAndHolds`, `StatusIs`
* Types
    * `namespace mbo::types`
    * mbo/types:cases_cc, mbo/types/cases.h
        * `Cases`, `CaseIndex`, `IfThen`, `IfElse`.
    * mbo/types:extend_cc, mbo/types/extend.h
        * `Extend`, `ExtendNoDefault`
        * `namespace extender`
            * `AbslFormat`, `AbslHashable`, `Printable`, `Streamable`, `Comparable`
    * mbo/types:tstring_cc, mbo/types/tstring.h
        * `tstring`, `operator ""_ts`
