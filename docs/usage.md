# Library usage

## Setup

Apart from the `enum class` itself, the `lngs enum` tool also provides a `String` type alias. Depending on the type of enumerations and whether the resources were requested or not, it might point to a variety of class combinations.

Object of this class needs to be prepared to be able to return compiled strings. First, it needs to have a manager attached, which will be able to locate individual language files, then one of those files needs to be selected and opened.

## Managers

The library provides two managers in `lngs::manager` namespace. Both have constructors with two arguments:

- `std::filesystem::path base`
- `std::string filename`

First argument will be the base directory for language collection, second will be used as final missing piece in locating the language file.


| Class | Template |
|-------|------|
| `SubdirPath` | `<base>/<lang>/<filename>` |
| `ExtensionPath` | `<base>/<filename>.<lang>` |


### SubdirPath

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

### ExtensionPath

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

## Using system locale list

There is a function, `system_locales()`, which returns a vector of system-defined locales. On POSIX, it checks the `$LANGUAGE`, `$LC_ALL`, `$LC_MESSAGES` and `$LANG` environment variables, followed by `LC_ALL` and `LC_MESSAGES` categories. On Windows, it checks `GetUserDefaultLocaleName` and `GetSystemDefaultLocaleName` functions.

For every locale, if it contains a hyphen, a shorter version is also added to the list. So, if a `lang-REGION-Script` triplet is returned by OS, `lang-REGION` and `lang` are also added.

Result of this function can be used to open first language on that list, which is provided by the manager:

```cxx
foo::Strings tr{};
tr.path_manager<lngs::manager::SubdirPath>("/usr/share/foo", "foobar.lng");

tr.open_first_of(lngs::system_locales());
```

For instance, if the `$LANGUAGE` is equal to `"pl_PL:en_US:en"`, the list will consist of `"pl-PL"`, `"pl"`, `"en-US"` and `"en"` and the example above will try, in order:

1. `/usr/share/foo/pl-PL/foobar.lng`
2. `/usr/share/foo/pl/foobar.lng`
3. `/usr/share/foo/en-US/foobar.lng`
4. `/usr/share/foo/en/foobar.lng`


## Using HTTP `Accept-Language` header

In case of web-based system, a translation can be selected based on the request. Function `http_accept_language()` takes a view of `Accept-Language` header, splits and sorts the values according to the quotients, expanding the hyphenated values, just like `system_locales()` would:

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

Here, if the request was sent with the header equal to `"pl,en-US;q=0.9,en;q=0.8"`, the list will consist of `"pl"`, `"en-US"` and `"en"` and the example above will try, in order:

1. `/usr/share/foo/pl/foobar.lng`
2. `/usr/share/foo/en-US/foobar.lng`
3. `/usr/share/foo/en/foobar.lng`

## Language change callbacks

In case of long running application, which would allow translation changes, there is callback, which can be set and which will be called each time a new translation is opened by strings file.

First function, `add_onupdate`, takes a `void`-to-`void` callable and returns a numerical cookie. Second function, `remove_onupdate`, takes this cookie and removes the previously-added callable.

## Getting strings

_TODO_
