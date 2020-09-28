# String definition syntax

The system uses a format roughly inspired by MS MIDL / WebIDL languages.

## Augmented BNF syntax

### Non-terminal symbols

The low-level non-terminal symbols are `<number>`, `<c-identifier>` and
`<c-string>` (`CRLF` below is really `(CR/LF/CRLF)`):

```abnf
; C-like identifier: underscore or a letter,
; followed by underscore, letter or digit
c-identifier = first-char / c-identifier following-char
first-char = ALPHA / "_"
following-char = ALPHA / DIGIT / "_"

; simple sequence of digits, with negatives allowed for id(-1)
number = ["-"] 1*DIGIT

; C string literal, but only simple escape sequences, no octets, no hexes
c-string = DQUOTE *(c-string-char / escape-sequence) DQUOTE
c-string-char = %x21 / %x23-5B / %x5D-7E ; VCHAR - (DQUOTE / BACKSLASH)
escape-sequence = BACKSLASH ( DQUOTE / "'" / "?" / BACKSLASH / "a" / "b" / "f" / "n" / "r" / "t" / "v" )

; helpers
SPACE = WSP / CRLF / COMMENT
BACKSLASH = %x5D
COMMENT = "//" *(WSP / VCHAR) CRLF
```

For readability, all definitions below this point can have any number of
whitespace and comments between symbols (`SPACE` above).

### Attributes

On both file level and string level, an IDL-like attribute list is used:

```abnf
attributes = "[" [ attribute-list ] "]"
attribute-list = attribute / attribute "," attribute-list
attribute = c-identifier "(" ( c-string / number ) ")"
```

### Top-level structures

```abnf
top-level = attributes %s"strings" "{" *string "}"
string = attributes c-identifier "=" c-string ";"
```

## Top-level attributes

| Name | Argument | M/O |
|------|------|:---:|
| `serial` | number | M |
| `project` | string | O |
| `namespace` | string | O |
| `version` | string | O |

### Example

```idl
[
    project("lngs"),
    namespace("lngs::app"),
    version("0.4"),
    serial(5)
] strings {
...
```

### The `serial` attribute

Indicates the current generation of string list definition and will be
incremented on each freeze. New strings file should use `serial(0)`.
Interpretation is left largely to the application author, except for
opening the translation files, where serial in the translation must match
serial in the `Strings` class.

### The `project` attribute

If provided, will be used for naming the namespace, where `enum class` is
defined, as well as `.pot` template metadata for `Project-Id-Version`. For
example, the definition above (combined with `version` attribute) would
result in:

```pot
msgid ""
msgstr ""
"Project-Id-Version: lngs 0.4\n"
# ...
```

### The `namespace` attribute

If provided, will override the `project`, when naming the namespace, where
`enum class` is defined. For example, the definition above would result in:

```cxx
namespace lngs::app {
//...
```

### The `version` attribute

If provided, will be used for `.pot` template metadata for `Project-Id-Version`.


## Per-string attributes

| Name | Argument | M/O | 
|------|------|:---:|
| `id` | number | M | 
| `help` | string | O |
| `plural` | string | O |  |

### Example

```idl
[help("Header for list of positional arguments"), id(-1)]
ARGS_POSITIONALS = "positional arguments";

[help("A plural string example"), plural("you have {0} foobars"), id(-1)]
FOOBAR_COUNT = "you have one foobar";
```

### The `id` attribute

Provides the value of the index associated with the string. New strings
must be set to `id(-1)`, as this will guarantee all identifiers are unique.
Setting the attribute by hand to any other value can result in duplicates,
especially if new string is created by copying an older one.

### The `help` attribute

Provides hint for the translators and will be added in the `.pot` template.
While technically optional, the tool warns, if `help()` is either missing
or empty.

### The `plural` attribute

Must be provided, if the string depends on some counter and should also
contain plural forms. When present, the value of the attribute will be used
as `msgid_plural` contents.

Must **not** be provided, if the string should have only one form.
