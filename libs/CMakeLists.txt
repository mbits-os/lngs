if (LNGS_WALL_FLAGS)
  add_definitions(${ADDITIONAL_WALL_FLAGS})
endif()

add_subdirectory(liblngs)

if (LNGS_LITE)
  add_subdirectory(liblngs_lite)
endif()
