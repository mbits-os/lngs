set(lngs_SRCS
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
  include/lngs/commands.hpp
  include/lngs/diagnostics.hpp
  include/lngs/gettext.hpp
  include/lngs/languages.hpp
  include/lngs/streams.hpp
  include/lngs/strings.hpp
  include/lngs/strings/lngs.hpp
  include/lngs/utf8.hpp
)

add_library(liblngs STATIC ${lngs_SRCS} ${lngs_INCS})
set_target_properties(liblngs PROPERTIES
  FOLDER app
  IDL "${CMAKE_CURRENT_SOURCE_DIR}/src/lngs.idl"
  OUTPUT_NAME lngs)
target_compile_definitions(liblngs PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(liblngs
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(liblngs locale args fmt::fmt)

##################################################################
##  TESTING
##################################################################

if (LOCALE_TESTING)

add_test_executable(lngs-test DATA_PATH data LIBRARIES liblngs)

add_test(NAME lngs.source_location COMMAND lngs-test --gtest_filter=source_location.*)
add_test(NAME lngs.diagnostics COMMAND lngs-test --gtest_filter=*diag_write.*:*/diag_ne.* --data_path=${DATA_DIR})
add_test(NAME lngs.read_idl COMMAND lngs-test --gtest_filter=*/read.*:*/read_file.* --data_path=${DATA_DIR})
add_test(NAME lngs.freeze COMMAND lngs-test --gtest_filter=*/frozen.*)
add_test(NAME lngs.res COMMAND lngs-test --gtest_filter=*/res_make.*:*/res_write.*:res_read.*)
add_test(NAME lngs.enums COMMAND lngs-test --gtest_filter=*/enums.*)
add_test(NAME lngs.python COMMAND lngs-test --gtest_filter=*/python.*)
add_test(NAME lngs.pot COMMAND lngs-test --gtest_filter=*/pot.*)
add_test(NAME lngs.utf COMMAND lngs-test --gtest_filter=*/utf_conv.*:*/utf_errors.*)
add_test(NAME lngs.gettext COMMAND lngs-test --gtest_filter=*/gettext.*:*/gtt_attr_*.*:*/gtt_extract.*:*/mo_*.* --data_path=${DATA_DIR})

endif()
