INCLUDE (CheckIncludeFileCXX)
check_include_file_cxx(experimental/filesystem CMAKE_HAVE_TS_FILESYSTEM)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmakeconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/cmakeconfig.h)
