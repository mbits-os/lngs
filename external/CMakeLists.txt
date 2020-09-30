add_subdirectory(args)
set_target_properties(args PROPERTIES FOLDER "external")

option(WITH_INSTALLER "enable installer" OFF)
add_subdirectory(libmstch)
set_target_properties(args PROPERTIES FOLDER "external")

add_subdirectory(utfconv)
