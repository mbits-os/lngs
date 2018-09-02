include (BuildConfig)

set(locale_SRCS
	src/expr_parser.cpp
	src/format.cpp
	src/lang_file.cpp
	src/locale_storage.cpp
	src/plurals.cpp
	src/translation.cpp
)

set (locale_INCS
	locale/format.hpp
	locale/locale.hpp
	locale/locale_base.hpp
	locale/locale_file.hpp
	locale/locale_storage.hpp
	locale/plurals.hpp
	locale/translation.hpp
	src/expr_parser.hpp
	src/node.hpp
	src/str.hpp
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(EXTRA_LIBS)

add_library(locale STATIC ${locale_SRCS} ${locale_INCS})
set_target_properties(locale PROPERTIES
	VERSION ${LOCALE_VERSION}
	FOLDER library)
target_include_directories(locale
	PUBLIC
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	  $<INSTALL_INTERFACE:include>
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(locale PUBLIC ${EXTRA_LIBS})

install(TARGETS locale
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
install(DIRECTORY locale/ DESTINATION include/locale)
