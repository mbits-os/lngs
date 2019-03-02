add_library(lngs_lite SHARED lite.cpp include/lngs/lite.h)
set_target_properties(lngs_lite PROPERTIES
	COMPILE_DEFINITIONS STRINGS_EXPORT
	VERSION ${LNGS_VERSION_SHORT}
	SOVERSION ${LNGS_VERSION_MAJOR}
  FOLDER libs)
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
