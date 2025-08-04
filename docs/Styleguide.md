# Styleguide

## Guidelines

In general CBM follows the ALICE styleguide. It can be found here:

- [Coding guidelines](https://rawgit.com/AliceO2Group/CodingGuidelines/master/coding_guidelines.html)
- [Naming and formatting conventions](https://rawgit.com/AliceO2Group/CodingGuidelines/master/naming_formatting.html)
- [Comments guidelines](https://rawgit.com/AliceO2Group/CodingGuidelines/master/comments_guidelines.html)

## Exceptions

Note that we've adopted some exceptions to these rules:

### Header files

Use `#pragma once` for header files instead of `#define` guards.

_Reason_: Less error prone and doesn't have to be altered when the file path changes. Potential drawbacks (issues with symlinks, compiler support) don't seem relevant for CbmRoot.

### Naming

#### Function names

Function names should start with a capital letter. (Instead of lower case in ALICE)

_Reason_: Compatibility with old code.

#### Class members

Class members should be prefixed with f (instead of m in ALICE). (Struct members are never prefixed!)

_Reason_: No advantage in changing prefix. Stay compatible with existing code.

### Formatting

Formatting differs in details from ALICE. Differences are caught with clang-format.
