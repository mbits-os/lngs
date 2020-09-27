# Support from CMake

Snippets below describe, how to use `lngs` in build system.

## Enumerations and builtin resources

Automatically generating the files each time the string list definition changes.

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

## Binary translation files

Automatically generating the translation each time the  changes.

```cmake
set(LANGUAGES pl it ja)

foreach(LANG ${LANGUAGES})
  set(__po "${CMAKE_CURRENT_SOURCE_DIR}/data/strings/${LANG}.po")
  set(__lng "${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR}/pkgs.${LANG}")
  message(STATUS "${__po} -> ${__lng}")
  list(APPEND SRCS ${__po} ${__lng})

  add_custom_command(OUTPUT "${__lng}"
    COMMAND lngs
    ARGS make
      --in "${CMAKE_CURRENT_SOURCE_DIR}/src/pkgs-strings.idl"
      --msgs "${__po}"
      --lang "${CMAKE_CURRENT_SOURCE_DIR}/data/strings/llcc.txt"
      --out "${__lng}"
    DEPENDS ${__po}
    VERBATIM)
endforeach()
```

The `llcc.txt` file mentioned above should have languages mapped to names, e.g.:

```
pl polski
it italiana
ja 日本語
```
