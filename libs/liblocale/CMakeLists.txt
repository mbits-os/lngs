configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.in.hpp ${CMAKE_CURRENT_BINARY_DIR}/locale/version.hpp @ONLY)

set(locale_SRCS
	src/expr_parser.cpp
	src/file.cpp
	src/lang_file.cpp
	src/locale_storage.cpp
	src/plurals.cpp
	src/translation.cpp
	src/version.cpp
)

set (locale_INCS
	locale/file.hpp
	locale/locale.hpp
	locale/locale_base.hpp
	locale/locale_file.hpp
	locale/locale_storage.hpp
	locale/plurals.hpp
	locale/translation.hpp
	src/expr_parser.hpp
	src/node.hpp
	src/str.hpp
	src/version.in.hpp
	"${CMAKE_CURRENT_BINARY_DIR}/locale/version.hpp"
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
target_link_libraries(locale PUBLIC ${EXTRA_LIBS} fmt::fmt)

install(TARGETS locale
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
install(DIRECTORY locale/ DESTINATION include/locale)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/locale/version.hpp DESTINATION include/locale)

##################################################################
##  TESTING
##################################################################

if (LOCALE_TESTING)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test-data)
configure_file(${PROJECT_SOURCE_DIR}/default_data_path.in.hpp ${CMAKE_CURRENT_BINARY_DIR}/default_data_path.hpp @ONLY)
message(STATUS "${CMAKE_CURRENT_BINARY_DIR}/default_data_path.hpp")

file(GLOB TEST_SRCS tests/*.cc)

add_executable(locale-test "${CMAKE_SOURCE_DIR}/googletest.cpp" ${TEST_SRCS})
set_target_properties(locale-test PROPERTIES FOLDER tests)
target_link_libraries(locale-test locale lngs_impl gtest gmock ${CMAKE_THREAD_LIBS_INIT})


enable_testing()
add_test(NAME locale.file COMMAND locale-test --gtest_filter=file.*)
add_test(NAME locale.plurals COMMAND locale-test --gtest_filter=*/plurals.*:*/plural_ops.*)
add_test(NAME locale.lang_file COMMAND locale-test --gtest_filter=*/lang_file_*)
add_test(NAME locale.translation COMMAND locale-test --gtest_filter=*/translation.*:translation.*)
add_test(NAME locale.storage COMMAND locale-test --gtest_filter=*/storage_*:storage.*)

endif()
