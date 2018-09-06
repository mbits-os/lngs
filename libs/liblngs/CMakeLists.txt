set(lngs_SRCS
  src/argparser.cpp
  src/cmds/cmd_freeze_ids.cpp
  src/cmds/cmd_load_mo.cpp
  src/cmds/cmd_write_enums.cpp
  src/cmds/cmd_write_pot.cpp
  src/cmds/cmd_write_py.cpp
  src/cmds/cmd_write_res.cpp
  src/diagnostics.cpp
  src/gettext.cpp
  src/languages.cpp
  src/lang_codes.cpp
  src/lngs_res.cpp
  src/streams.cpp
  src/strings.cpp
  src/utf8.cpp
)

set (lngs_INCS
  lngs/argparser.hpp
  lngs/commands.hpp
  lngs/diagnostics.hpp
  lngs/gettext.hpp
  lngs/languages.hpp
  lngs/streams.hpp
  lngs/strings.hpp
  lngs/strings/lngs.hpp
  lngs/utf8.hpp
)

add_library(lngs_impl STATIC ${lngs_SRCS} ${lngs_INCS})
set_target_properties(lngs_impl PROPERTIES FOLDER tools IDL "${CMAKE_CURRENT_SOURCE_DIR}/src/lngs.idl")
target_compile_definitions(lngs_impl PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(lngs_impl
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(lngs_impl locale)

##################################################################
##  TESTING
##################################################################

if (LOCALE_TESTING)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test-data)

file(GLOB TEST_SRCS tests/*.cc)

add_executable(lngs-test "${CMAKE_SOURCE_DIR}/googletest.cpp" ${TEST_SRCS})
set_target_properties(lngs-test PROPERTIES FOLDER tests)
target_link_libraries(lngs-test lngs_impl gtest gmock ${CMAKE_THREAD_LIBS_INIT})

enable_testing()
add_test(NAME lngs.source_location COMMAND lngs-test --gtest_filter=source_location.*)
add_test(NAME lngs.diagnostics COMMAND lngs-test --gtest_filter=*/diag_write.*)
add_test(NAME lngs.read_idl COMMAND lngs-test --gtest_filter=*/read.*)
add_test(NAME lngs.freeze COMMAND lngs-test --gtest_filter=*/frozen.*)
add_test(NAME lngs.res COMMAND lngs-test --gtest_filter=*/res_make.*:*/res_write.*)
add_test(NAME lngs.utf COMMAND lngs-test --gtest_filter=*/utf_conv.*)
add_test(NAME lngs.gettext COMMAND lngs-test --gtest_filter=*/gettext.*:*/gtt_attr_*.*:*/gtt_extract.*:*/mo_*.* --data_path=${DATA_DIR})

endif()
