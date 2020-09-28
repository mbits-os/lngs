# Locale File Format And Processor

[![Travis (.org)][Travis badge]][Travis]
[![Coveralls github][Coveralls badge]][Coveralls]
[![Cpp Standard][17-badge]][17]

`lngs` and `liblngs` are open-source tool and library for index-based
translation files. It can be used as a replacement for GetText message files
(especially on Windows), while still being able to use editors for GetText
catalogues.

## Features

- String definitions supporting comment for translators (_extracted comments_)
  and singular and plural forms.
- Automatic generation of `enum class` for C++ and identifiers for Python.
- Two different sets of enums are created, one for singular-only translations,
  one for plural-enabled strings. There will never be mixup between the two
  (unless one really tries).
- Simple API for extracting strings matching generated enums and only
  the generated enums.
- String lookup by a number, instead of string or string-based hash.
- Net result is as if GetText catalogue had a unique context attached to every
  string.
- Builtin "resource file" with a copy of untranslated strings compiled into
  a binary form and turned into a `char` table.
- Ŵȧȓpêđ şŧȓïñğş ïñ bũïĺŧïñ ȓêşôũȓçêş
- Generation of GetText-compatible `.pot` template
- Compiling resulting binary files from either translated `.po` text or binary
  `.mo` file.

## Ŵȧȓpêđ şŧȓïñğş ïñ bũïĺŧïñ ȓêşôũȓçêş
The builtin table supports *şŧȓïñğ ŵȧȓpïñğ*, to help with yet-untranslated
strings. The idea is, that while providing semi-legible text, those warped
strings stand out in application output. They are legible enough to be
recognized and noted for translation and illegible enough to be recognizable
as hint of an issue.

However, if translation is used with `printf` family and the placeholders are
using `%s` arguments, the warped version will replace them with `%ş`,
rendering them unusable. The use of warped builtin with `printf` style
arguments is therefore discouraged. Warped strings work best with [`{fmt}`],
or C++20 [`std::format`], which are Python-inspired and support
position-based `{0}` arguments.

## Files encoding

The internal encoding of the strings in the `.po` and `.mo` files is never
checked and is not copied over to binary `.lng` files. It is therefore
assumed the encoding is consistent from one language file to the next and is
expected by the application author.

The internal encoding of the strings in the `.idl` is expected to be UTF-8.
This is not enforced, but whatever content was in this file, it's copied over
to the generated `.pot` template, which declares its strings to be UTF-8.

And since [UTF-8 won], it is advised here to use
UTF-8 for string encoding all across the board. For Windows-targeted
applications an analogue of `chcp 65001` still needs to be performed at the
top of the `main()`, although improvements in system console has gone a long
way already and may be UTF-8 by default someday.

## See also

- [Simple workflow](docs/flow.md) involving both `lngs` and `liblngs`.
- [File syntax](docs/syntax.md) for string definition.
- [CMake snippets](docs/cmake.md), which propose `lngs` setup for new projects
- [Library usage](docs/usage.md), which describes `liblngs` setup for new
  projects and its later usage.


[Travis badge]: https://img.shields.io/travis/mbits-os/lngs?style=flat-square
[Travis]: https://travis-ci.org/mbits-os/lngs "Travis-CI"
[Coveralls badge]: https://img.shields.io/coveralls/github/mbits-os/lngs?style=flat-square
[Coveralls]: https://coveralls.io/github/mbits-os/lngs "Coveralls"
[17-badge]: https://img.shields.io/badge/C%2B%2B-17-informational?style=flat-square
[17]: https://en.wikipedia.org/wiki/C%2B%2B17 "Wikipedia C++17"
[`{fmt}`]: https://github.com/fmtlib/fmt
[`std::format`]: https://en.cppreference.com/w/cpp/utility/format
[UTF-8 won]: http://utf8everywhere.org/