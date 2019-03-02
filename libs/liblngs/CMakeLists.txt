configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.in.hpp ${CMAKE_CURRENT_BINARY_DIR}/include/lngs/version.hpp @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lngs.pc.in ${CMAKE_CURRENT_BINARY_DIR}/lngs.pc @ONLY)

set(liblngs_SRCS
	src/expr_parser.cpp
	src/file.cpp
	src/lang_file.cpp
	src/lngs_storage.cpp
	src/plurals.cpp
	src/translation.cpp
	src/version.cpp
)

set (liblngs_INCS
	include/lngs/file.hpp
	include/lngs/lngs.hpp
	include/lngs/lngs_base.hpp
	include/lngs/lngs_file.hpp
	include/lngs/lngs_storage.hpp
	include/lngs/plurals.hpp
	include/lngs/translation.hpp
	src/expr_parser.hpp
	src/node.hpp
	src/str.hpp
	src/version.in.hpp
	"${CMAKE_CURRENT_BINARY_DIR}/include/lngs/version.hpp"
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

add_library(liblngs STATIC ${liblngs_SRCS} ${liblngs_INCS})
set_target_properties(liblngs PROPERTIES
	VERSION ${LNGS_VERSION}
	FOLDER libs
	OUTPUT_NAME lngs)
target_include_directories(liblngs
	PUBLIC
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
	  $<INSTALL_INTERFACE:include>
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(liblngs PUBLIC ${EXTRA_LIBS})

install(TARGETS liblngs
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
install(DIRECTORY include/lngs/ DESTINATION include/lngs)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/lngs/version.hpp DESTINATION include/lngs)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lngs.pc DESTINATION share/pkgconfig)

##################################################################
##  TESTING
##################################################################

if (LNGS_TESTING)

add_test_executable(liblngs-test DATA_PATH data LIBRARIES liblngs lngs_app)

add_test(NAME liblngs.file COMMAND liblngs-test --gtest_filter=file.*)
add_test(NAME liblngs.plurals COMMAND liblngs-test --gtest_filter=*/plurals.*:*/plural_ops.*)
add_test(NAME liblngs.lang_file COMMAND liblngs-test --gtest_filter=*/lang_file_*)
add_test(NAME liblngs.translation COMMAND liblngs-test --gtest_filter=*/translation.*:translation.*)
add_test(NAME liblngs.storage COMMAND liblngs-test --gtest_filter=*/storage_*:storage.*)
add_test(NAME liblngs.strings COMMAND liblngs-test --gtest_filter=strings.*)

endif()
