if (LOCALE_WALL_FLAGS)
  add_definitions(${ADDITIONAL_WALL_FLAGS})
endif()

add_subdirectory(liblocale)
add_subdirectory(liblocale_c)
add_subdirectory(liblngs)
