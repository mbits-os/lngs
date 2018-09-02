add_subdirectory(libfmt)

set_target_properties(fmt PROPERTIES FOLDER "3rd Party")
target_compile_definitions(fmt INTERFACE FMT_USE_USER_DEFINED_LITERALS=0)

add_subdirectory(liblocale)
add_subdirectory(liblocale_c)
add_subdirectory(liblngs)
