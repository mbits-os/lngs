if (NOT TARGET semver)
add_subdirectory(semver)
set_target_properties(semver PROPERTIES FOLDER "external")
endif()

option(WITH_INSTALLER "enable installer" OFF)
add_subdirectory(libmstch)
set_target_properties(mstch PROPERTIES FOLDER "external")

add_subdirectory(utfconv)
set_target_properties(utfconv PROPERTIES FOLDER "external")

add_subdirectory(diags)
set_target_properties(diags PROPERTIES FOLDER "external")
