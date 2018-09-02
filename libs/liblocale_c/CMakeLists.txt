add_library(locale_c SHARED strings.cpp locale/strings.h)
set_target_properties(locale_c PROPERTIES
	COMPILE_DEFINITIONS STRINGS_EXPORT
	VERSION ${LOCALE_VERSION_SHORT}
	SOVERSION ${LOCALE_VERSION_MAJOR}
  FOLDER library)
target_include_directories(locale_c PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(locale_c locale)

install(TARGETS locale_c
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/locale/strings.h DESTINATION include/locale)
