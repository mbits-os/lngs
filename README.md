# Locale File Format And Processor

## `lngs` and `liblngs`

[![Travis (.org)](https://img.shields.io/travis/mbits-os/lngs?style=for-the-badge)](https://travis-ci.org/mbits-os/lngs)
[![Coveralls github](https://img.shields.io/coveralls/github/mbits-os/lngs?style=for-the-badge)](https://coveralls.io/github/mbits-os/lngs)
[![Cpp Standard](https://img.shields.io/badge/C%2B%2B-17-informational?style=for-the-badge)](https://en.wikipedia.org/wiki/C%2B%2B17)

### Library

The `liblngs` looks up the translated strings by indexes, not by another strings. This means that

1. One *"engineering English"* string does not need to match to only one translated entry.
2. Source code uses integers to refer to strings. In fact, the tools provided will create an `enum class` with string identifiers.
3. Two different sets of enums are created, one for singular-only translations, one for plural-enabled strings. There will never be mixup between the two (unless one really tries).

### Tool app
The `lngs` operates on a simplified `.idl` file to produce all necessary artifacts:

1. The aforementioned `enum class`es with file managers tailored for those `enum`s.
2. Special "resource file" with a copy of strings compiled into a binary form and turned into a `char` table.
3. GetText-compatible `.pot` template, where each string has identifier attached as message context.
4. Binary translation files, built either from translated `.po` text or binary `.mo` file.

### Ŵȧȓpêđ şŧȓïñğş ïñ Ƌũïĺŧïñ ȓêşôũȓçêş
The builtin table supports *şŧȓïñğ ŵȧȓpïñğ*, to help with yet-untranslated strings. However, if used with standard `%s` arguments, the warped version will contain `%ş`, so use of warped builtin with `printf` style arguments is discouraged. Warped strings work best with [`{fmt}`](https://github.com/fmtlib/fmt), or C++20 [`std::format`](https://en.cppreference.com/w/cpp/utility/format).

### `.idl` definition / GetText files encoding

The internal encoding of the strings in the `.po` and `.mo` files is never checked and is not copied over to binary `.lng` files. It is therefore assumed the encoding is consistent from one language to the next and is expected by the application author.

The internal encoding of the strings in the `.idl` is expected to be UTF-8. This is not enforced, but whatever content was in this file, it's copied over to the generated `.pot` template, which declares its strings are UTF-8.

## Example of flow supported by the `lngs`

1. A developer adds new strings with unfrozen ids.
2. The team uses unfrozen string list for day-to-day work.
3. Before a release, the translation manager freezes the strings and prepares `.pot` for translators.
4. Translators update `.po` files as needed.
5. When releasing, the binary translations are placed in a package

### Developer (adding new string)

```sh
vim .idl
git commit .idl
```

Edits the `.idl` file and commit the modified file for others to use.

Each new string needs to have a `help("")` attribute, a "new string id" of `id(-1)`, and name for C++ `enum` and initial value written in either *"engineering English"*, or preferably coordinated with a native speaker / Translation Manager.

The `help("")` attribute will be provided in the `.pot` template as a hint for the translators.

It is crucial for the new string to have `-1` as id, as this will guarantee all identifiers are unique. Otherwise, it is very easy to create duplicates, especially if one copies an older string to create a new one.

A `plural("")` attribute, if used, indicates the string would need plural forms.

```
[help("Header for list of positional arguments"), id(-1)]
ARGS_POSITIONALS = "positional arguments";

[help("A plural string"), plural("you have {0} foobars"), id(-1)]
FOOBAR_COUNT = "you have one foobar";
```

#### New string list

When a new string list is created, top-level attributes attached to the "strings" pseudo-interface can be:

- `serial(0)` (mandatory), which indicates the generation of string list definition and will be incremented on each freeze, 
- `project("name")`, which will be used everywhere a project name would be needed
- `namespace("name")`, which will be used a namespace of the `enum class`es; if missing, will fall back to `project`
- `version("X.Y.Z")`, which together with `project` will be used in `.pot` to populate `Project-Id-Version` attribute there

```
[
	project("lngs"),
	namespace("lngs::app"),
	version("0.4"),
	serial(5)
] strings {
.
.
.
}
```

### Developer (compiling existing list)

```sh
msgfmt [optional]
lngs enums
lngs res
lngs make
git commit .hpp .cpp [optional]
```

This should be provided as part of build system. The process can optionally start with transforming current set of `.po` files into `.mo` files. Either of those can later be "made" into binary translation files.

The `.idl` file can be used to update the resources and `enum`s. If the build system is smart enough to only call the `lngs enums` and `lngs res`, when the source `.idl` actually changed, then the header and resource can be kept out of repository.

```cmake
set(SRCS
  # ...
  "${CMAKE_CURRENT_BINARY_DIR}/src/resources.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/src/lngs.hh"
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/src/resources.cc"
  COMMAND lngs
  ARGS res
    --in "${CMAKE_CURRENT_SOURCE_DIR}/src/strings.idl"
    --include "lngs.hh"
    --out "${CMAKE_CURRENT_BINARY_DIR}/src/resources.cc"
  DEPENDS src/strings.idl
  VERBATIM)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/src/lngs.hh"
  COMMAND lngs
  ARGS enums
    --resource 
    --in "${CMAKE_CURRENT_SOURCE_DIR}/src/strings.idl"
    --out "${CMAKE_CURRENT_BINARY_DIR}/src/lngs.hh"
  DEPENDS src/strings.idl
  VERBATIM)

```

### Translation Manager

```sh
lngs freeze
git commit .idl
lngs pot
```

In preparation of a release, someone, either translation manager or some designated dev freezes the list identifiers assigned to new strings. This will updated the serial number and allow the generated files to be part of released package.

At the same time, translation manager updates the `.pot` template, which can be distributed to the translators.

### Translator:

```sh
msgmerge (or msginit)
poedit .po [e.g]
git commit .po
```

Translators update the `.po` files with the new strings, which then are committed to the repo and used by build process to create the final artifacts.

### Developer (releasing a build):

```sh
msgfmt [optional]
lngs make
```

Using just a part of build process, a person responsible for packaging places the binary translation files in a place, where the product can locate them. This place is specific to either the product, the system, or both.

## Using `liblngs`

### Setup

_TODO:_
- Why `get_string` is protected
- System locale list
- HTTP `Accept-Language`
- Locating translation by extension or by subdir
- Language change callbacks

### Getting strings

_TODO_

### IDL syntax

_TODO_