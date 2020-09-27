# Example of flow supported by the `lngs`

1. A developer adds new strings with unfrozen ids.
2. The team uses unfrozen string list for day-to-day work.
3. Before a release, the translation manager freezes the strings and prepares `.pot` for translators.
4. Translators update `.po` files as needed.
5. When releasing, the binary translations are placed in a package

## Developer (adding new string)

```sh
vim .idl
git commit .idl
```

Edits the `.idl` file and commits the modified file for others to use.

Each new string needs to have a `help("")` attribute, a "new string id" of `id(-1)`, a name for C++ `enum` and initial value written in either *"engineering English"*, or preferably coordinated with a native speaker / Translation Manager.

## Developer (compiling existing list)

```sh
msgfmt [optional]
lngs enums
lngs res
lngs make
git commit .hpp .cpp [optional]
```

This should be provided as part of build system. The process can optionally start with transforming current set of `.po` files into `.mo` files. Either of those can later be "made" into binary translation files.

The `.idl` file can be used to update the resources and `enum`s. If the build system is smart enough to only call the `lngs enums` and `lngs res`, when the source `.idl` actually changed, then the header and resource can be kept out of repository.

See also [CMake snippets](cmake.md).

## Translation Manager

```sh
lngs freeze
git commit .idl
lngs pot
```

In preparation of a release, someone, either translation manager or some designated dev freezes the list identifiers assigned to new strings. This will updated the serial number and allow the generated files to be part of released package.

At the same time, translation manager updates the `.pot` template, which can be distributed to the translators.

## Translator:

```sh
msgmerge (or msginit)
poedit .po [e.g]
git commit .po
```

Translators update the `.po` files with the new strings, which then are committed to the repo and used by build process to create the final artifacts.

## Developer (releasing a build):

```sh
msgfmt [optional]
lngs make
```

Using just a part of build process, a person responsible for packaging places the binary translation files in a place, where the product can locate them. This place is specific to either the product, the system, or both.
