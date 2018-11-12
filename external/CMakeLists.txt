add_subdirectory(libfmt)
add_subdirectory(libargs)

set_target_properties(fmt PROPERTIES FOLDER "external")
target_compile_definitions(fmt INTERFACE FMT_USE_USER_DEFINED_LITERALS=0)

set_target_properties(args PROPERTIES FOLDER "external")
