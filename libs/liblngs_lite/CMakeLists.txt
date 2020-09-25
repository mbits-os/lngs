add_library(lngs_lite SHARED lite.cpp include/lngs/lite.h)
set_target_properties(lngs_lite PROPERTIES
	COMPILE_DEFINITIONS STRINGS_EXPORT
	VERSION ${PROJECT_VERSION_SHORT}
	SOVERSION ${PROJECT_VERSION_MAJOR}
  FOLDER libs)
target_compile_options(lngs_lite PRIVATE ${ADDITIONAL_WALL_FLAGS})
target_include_directories(lngs_lite PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(lngs_lite liblngs)

install(TARGETS lngs_lite
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/lngs/lite.h DESTINATION include/lngs)
