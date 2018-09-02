set(lngs_SRCS
  src/argparser.cpp
  src/cmds/cmd_freeze_ids.cpp
  src/cmds/cmd_load_mo.cpp
  src/cmds/cmd_write_enums.cpp
  src/cmds/cmd_write_pot.cpp
  src/cmds/cmd_write_py.cpp
  src/cmds/cmd_write_res.cpp
  src/gettext.cpp
  src/languages.cpp
  src/lang_codes.cpp
  src/streams.cpp
  src/strings.cpp
  src/utf8.cpp
)

set (lngs_INCS
  lngs/argparser.hpp
  lngs/gettext.hpp
  lngs/languages.hpp
  lngs/streams.hpp
  lngs/strings.hpp
  lngs/utf8.hpp
)

add_library(lngs_impl STATIC ${lngs_SRCS} ${lngs_INCS})
set_target_properties(lngs_impl PROPERTIES FOLDER tools)
target_include_directories(lngs_impl
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(lngs_impl locale)
