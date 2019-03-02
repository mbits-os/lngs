if (LNGS_WALL_FLAGS)
  add_definitions(${ADDITIONAL_WALL_FLAGS})
endif()

add_subdirectory(liblngs)
add_subdirectory(liblngs_lite)
