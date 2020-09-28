# Library usage

## Setup

Apart from the `enum class` itself, the `lngs enum` tool also provides a
`String` type alias. Depending on the type of enumerations and whether the
resources were requested or not, it might point to different combinations
of classes.

Object of this class needs to be prepared to be able to return compiled
strings. First, it needs to have builtin resources initialized and a path
manager attached. The path manager will be able to locate individual
language files and after it is set up, one of those files needs to be
selected and opened.

### Builtin strings

Builtin resource does not need to be opened by any given language id, but
they still need to be opened. If the language header was created with
support for them, then `Strings` class will have a `init_builtin()` method,
which loads the strings from the resource:

```cxx
foo::Strings tr{};
tr.init_builtin(); // to serve compiled-in version of string, if a string is missing...
```

### Path Managers

The library provides two managers in `lngs::manager` namespace. Both have
constructors with two arguments:

- `std::filesystem::path` base
- `std::string` filename

First argument will be the base directory for language collection, second
will be used as final missing piece in locating the language file. The two
managers differ in how they interpret the filename and language IDs:


| Class | Template |
|-------|------|
| `SubdirPath` | `<base>/<lang>/<filename>` |
| `ExtensionPath` | `<base>/<filename>.<lang>` |


#### SubdirPath

```cxx
foo::Strings tr{};
tr.path_manager<lngs::manager::SubdirPath>("/usr/share/foo", "foobar.lng");
if (!tr.open("en-US")) {
  tr.open("en");
}
```

The example above will attempt to open, in order:

1. `/usr/share/foo/en-US/foobar.lng`
2. `/usr/share/foo/en/foobar.lng`

#### ExtensionPath

```cxx
foo::Strings tr{};
tr.path_manager<lngs::manager::ExtensionPath>("/usr/share/foo", "foobar");
if (!tr.open("en-US")) {
  tr.open("en");
}
```

The example above will attempt to open, in order:

1. `/usr/share/foo/foobar.en-US`
2. `/usr/share/foo/foobar.en`

## Lists of languages

### System locale list

There is a function, `system_locales()`, which returns a vector of
system-defined locales. On POSIX, it checks the `$LANGUAGE`, `$LC_ALL`,
`$LC_MESSAGES` and `$LANG` environment variables, followed by `LC_ALL` and
`LC_MESSAGES` categories. On Windows, it checks `GetUserDefaultLocaleName`
and `GetSystemDefaultLocaleName` functions.

For every locale, if it contains a hyphen, the resulting list is expanded
to also contain shorter versions. So, if a `lang-REGION-Script` triplet is
returned by OS, `lang-REGION` and `lang` are also added.

Result of this function can be used to open first language on that list,
which is provided by the manager:

```cxx
foo::Strings tr{};
tr.path_manager<lngs::manager::SubdirPath>("/usr/share/foo", "foobar.lng");

tr.open_first_of(lngs::system_locales());
```

For instance, if the `$LANGUAGE` is equal to `"pl_PL:en_US:en"`, the list
will consist of `"pl-PL"`, `"pl"`, `"en-US"` and `"en"` and the example
above will try, in order:

1. `/usr/share/foo/pl-PL/foobar.lng`
2. `/usr/share/foo/pl/foobar.lng`
3. `/usr/share/foo/en-US/foobar.lng`
4. `/usr/share/foo/en/foobar.lng`


### HTTP `Accept-Language` header

In case of web-based system, a translation can be selected based on the
request. Function `http_accept_language()` takes a view of
`Accept-Language` header, splits and sorts the values according to the
quotients, expanding the hyphenated values, just like `system_locales()`
would:

```cxx
foo::Strings tr{};
tr.path_manager<lngs::manager::SubdirPath>("/usr/share/foo", "foobar.lng");

if (req.has(Headers::Accept_Language)) {
    tr.open_first_of(lngs::http_accept_language(
        req[Headers::Accept_Language]
    ));
} else {
    tr.open(fallback_language);
}
```

Here, if the request was sent with the header equal to `"pl,en-US;q=0.9,en;q=0.8"`,
the list will consist of `"pl"`, `"en-US"` and `"en"` and the example above
will try, in order:

1. `/usr/share/foo/pl/foobar.lng`
2. `/usr/share/foo/en-US/foobar.lng`
3. `/usr/share/foo/en/foobar.lng`

## Usage

### Language change callbacks

In case of long running application, which would allow translation changes,
there is callback, which can be set and which will be called each time a
new translation is opened by strings file.

First function, `add_onupdate`, takes a `void`-to-`void` callable and
returns a numerical cookie. Second function, `remove_onupdate`, takes this
cookie and removes the previously-added callable.

### Getting strings

The generated `enum class`es are named `lng` (for singular-only) and
`counted` (for strings with plural forms). 

Given string list containing those entries:
```idl
[project("my-tool"), namespace("foo"), serial(12)]
strings {
  ...

  [help("Header for list of items"), id(-1)]
  LIBRARY_HEADING = "You have";

  [plural("{0} foobars"), id(-1)]
  FOOBAR_COUNT = "one foobar";

  ...
}
```

then this code:

```cxx
foo::Strings tr{};
tr.init_builtin();
tr.path_manager<lngs::manager::ExtensionPath>(langdir, "my-tool");
tr.open_first_of(lngs::system_locales());

auto const foobar_count = 5;
auto const foobars = tr(foo::counted::FOOBAR_COUNT, foobar_count);

fmt::printf("{}:\n", tr(foo::lng::LIBRARY_HEADING));
fmt::printf("  {}\n", fmt::format(foobars, foobar_count));
```

might produce this output with `LANGUAGE=fr`, if `LIBRARY_HEADING` was
translated, but `FOOBAR_COUNT` was not and builtin strings were warped:

```
Vous avez:
  5 ƒôôƋȧȓş
```

or this, for `foobar_count` equal to 1 and builtin strings not warped:

```
Vous avez:
  one foobar
```
