add_subdirectory(libfmt)

set_target_properties(fmt PROPERTIES FOLDER "external")
target_compile_definitions(fmt INTERFACE FMT_USE_USER_DEFINED_LITERALS=0)
