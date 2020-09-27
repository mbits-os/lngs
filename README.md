# Locale File Format And Processor

[![Travis (.org)](https://img.shields.io/travis/mbits-os/lngs?style=flat-square)](https://travis-ci.org/mbits-os/lngs)
[![Coveralls github](https://img.shields.io/coveralls/github/mbits-os/lngs?style=flat-square)](https://coveralls.io/github/mbits-os/lngs)
[![Cpp Standard](https://img.shields.io/badge/C%2B%2B-17-informational?style=flat-square)](https://en.wikipedia.org/wiki/C%2B%2B17)

`lngs` and `liblngs` are open-source tool and library for index-based translation files. It can be used as a replacement for GetText message files (especially on Windows), while still being able to use GetText tooling.

## Features

- String definitions supporting comment for translators (_extracted comments_), singular and plural forms.
- Automatic generation of `enum class` for C++ and identifiers for Python.
- Two different sets of enums are created, one for singular-only translations, one for plural-enabled strings. There will never be mixup between the two (unless one really tries).
- Simple API for extracting strings matching generated enums and only the generated enums.
- String lookup by `int`, instead of string or string-based hash.
- Net result is as if GetText catalogue had a unique context attached to every string.
- Builtin "resource file" with a copy of strings compiled into a binary form and turned into a `char` table.
- Ŵȧȓpêđ şŧȓïñğş ïñ bũïĺŧïñ ȓêşôũȓçêş
- Generation of GetText-compatible `.pot` template
- Compiling resulting binary files from either translated `.po` text or binary `.mo` file.

## Ŵȧȓpêđ şŧȓïñğş ïñ bũïĺŧïñ ȓêşôũȓçêş
The builtin table supports *şŧȓïñğ ŵȧȓpïñğ*, to help with yet-untranslated strings. However, if used with standard `%s` arguments, the warped version will contain `%ş`, so use of warped builtin with `printf` style arguments is discouraged. Warped strings work best with [`{fmt}`](https://github.com/fmtlib/fmt), or C++20 [`std::format`](https://en.cppreference.com/w/cpp/utility/format).

## Files encoding

The internal encoding of the strings in the `.po` and `.mo` files is never checked and is not copied over to binary `.lng` files. It is therefore assumed the encoding is consistent from one language to the next and is expected by the application author.

The internal encoding of the strings in the `.idl` is expected to be UTF-8. This is not enforced, but whatever content was in this file, it's copied over to the generated `.pot` template, which declares its strings to be UTF-8.

## See also

- [Simple workflow](docs/flow.md) involving both `lngs` and `liblngs`.
- [File syntax](docs/syntax.md) for string definition.
- [CMake snippets](docs/cmake.md), which propose `lngs` setup for new projects
- [Library usage](docs/usage.md), which describes `liblngs` setup for new projects and its later usage.
