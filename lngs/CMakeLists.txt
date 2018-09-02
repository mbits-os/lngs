add_executable(lngs lngs.cpp)
set_target_properties(lngs PROPERTIES
	VERSION ${LOCALE_VERSION_SHORT}
	FOLDER tools)
target_include_directories(lngs
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lngs lngs_impl)

install(TARGETS lngs DESTINATION bin)
