set(locale_SRCS
	src/expr_parser.cpp
	src/file.cpp
	src/format.cpp
	src/lang_file.cpp
	src/locale_storage.cpp
	src/plurals.cpp
	src/translation.cpp
)

set (locale_INCS
	locale/file.hpp
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

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	include(CheckIncludeFileCXX)
	check_include_file_cxx(__config HAS_LIBCXX)
	if (HAS_LIBCXX)
		list(APPEND EXTRA_LIBS c++abi c++fs)
	else()
		list(APPEND EXTRA_LIBS stdc++fs)
	endif()
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	list(APPEND EXTRA_LIBS stdc++fs)
endif()

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
