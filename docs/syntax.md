# String definition syntax

The system uses a format roughly inspired by MS MIDL / WebIDL languages.

## Augmented BNF syntax

### Non-terminal symbols

The low-level non-terminal symbols are `<number>`, `<c-identifier>` and `<c-string>`:

```abnf
; C-like identifier: underscore or a letter, followed by underscore, letter or digit
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
SPACE = WSP / CRLF
BACKSLASH = %x5D
```

Following definitions can have any number of whitespace (`SPACE`) between symbols, for readability.

### Attributes

On both file level and string level, an IDL-like attribute list is used:

```abnf
attributes = "[" [ attribute-list ] "]"
attribute-list = attribute / attribute "," attribute-list
attribute = c-identifier "(" ( c-string / number ) ")"
```

### Top-level structures

```abnf
top-level = attributes "strings" "{" *string "}"
string = attributes c-identifier "=" c-string ";"
```

## Top-level attributes

| Name | Type | M/O |
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

Indicates the generation of string list definition and will be incremented on each freeze. New strings file should use `serial(0)`.

### The `project` attribute

If provided, will be used for naming the namespace, where `enum class` is defined, as well as `.pot` template metadata for `Project-Id-Version`. For example, the definition above would result in:

```pot
msgid ""
msgstr ""
"Project-Id-Version: lngs 0.4\n"
# ...
```

### The `namespace` attribute

If provided, will override the `project`, when naming the namespace, where `enum class` is defined. For example, the definition above would result in:

```cxx
namespace lngs::app {
//...
```

### The `version` attribute

If provided, will be used for `.pot` template metadata for `Project-Id-Version`.


## Per-string attributes

| Name | Type | M/O | 
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

It is the value of the index associated with the string. New strings must be set to `id(-1)`, as this will guarantee all identifiers are unique. Sending the attribute by hand to any other value can result in duplicates, especially if one copies an older string to create a new one.

### The `help` attribute

Will be provided in the `.pot` template as a hint for the translators. While technically optional, the tool will warn, if missing or empty.

### The `plural` attribute

- Must be provided, if the string depends on some counter and should represent plural forms.
- Must **not** be provided, if the string should have only one form.
