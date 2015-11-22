include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

INCLUDE (BuildConfig)

set(locale_SRCS
	src/expr_parser.cpp
	src/format.cpp
	src/lang_file.cpp
	src/locale_storage.cpp
	src/plurals.cpp
	src/translation.cpp
)

set (locale_INCS
	include/locale/format.hpp
	include/locale/locale.hpp
	include/locale/locale_base.hpp
	include/locale/locale_file.hpp
	include/locale/locale_storage.hpp
	include/locale/plurals.hpp
	include/locale/translation.hpp
	src/expr_parser.hpp
	src/node.hpp
	src/str.hpp
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(locale STATIC ${locale_SRCS} ${locale_INCS})
set_target_properties(locale PROPERTIES
	CXX_STANDARD 14
	VERSION ${LOCALE_VERSION})

add_definitions(-DSTRINGS_EXPORT)
add_library(strings SHARED src/strings.cpp include/locale/strings.h)
set_target_properties(strings PROPERTIES
	CXX_STANDARD 14
	VERSION ${LOCALE_VERSION}
	SOVERSION ${LOCALE_VERSION_MAJOR})

target_link_libraries(strings locale)

install(TARGETS strings locale
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
install(TARGETS strings locale DESTINATION lib)
install(DIRECTORY include/locale/ DESTINATION include/locale)
