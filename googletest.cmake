# We need thread support
find_package(Threads REQUIRED)

set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)

add_subdirectory(tools/googletest gtest)

set_target_properties(gmock PROPERTIES FOLDER tools)
set_target_properties(gmock_main PROPERTIES FOLDER tools)
set_target_properties(gtest PROPERTIES FOLDER tools)
set_target_properties(gtest_main PROPERTIES FOLDER tools)
