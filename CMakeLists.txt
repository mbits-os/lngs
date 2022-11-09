cmake_minimum_required (VERSION 3.10)
project (lngs
  VERSION 0.7.1
  DESCRIPTION "Locale File Format And Processor"
  LANGUAGES CXX)

set(LNGS_TESTING ON CACHE BOOL "Compile and/or run self-tests")
set(LNGS_INSTALL ON CACHE BOOL "Create install targets")
set(LNGS_LITE ON CACHE BOOL "Create lngs_lite library")
set(LNGS_APP ON CACHE BOOL "Compile lngs application in addition to liblngs")
set(LNGS_IGNORE_CONAN OFF CACHE BOOL "Ignore conanbuild.cmake")
set(LNGS_LINKED_RESOURCES OFF CACHE BOOL "Compile templates and translations into the lngs binary")
set(LNGS_NO_PKG_CONFIG OFF CACHE BOOL "Skip lngs.pc installation")

set(PROJECT_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PROJECT_VERSION_STABILITY "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}")

if (NOT LNGS_IGNORE_CONAN)
  set(CONAN_CMAKE_SILENT_OUTPUT ON)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(TARGETS)
endif() # LNGS_LIBRARY_ONLY

if (MSVC)
  set(ADDITIONAL_WALL_FLAGS
      /D_CRT_SECURE_NO_WARNINGS
      /D_CRT_NONSTDC_NO_WARNINGS
      /utf-8
      /permissive-
      /Zc:__cplusplus
      /W4
      /w14242
      /w14254
      /w14263
      /w14265
      /w14287
      /we4289
      /w14296
      /w14311
      /w14545
      /w14546
      /w14547
      /w14549
      /w14555
      /w14619
      /w14640
      /w14826
      /w14905
      /w14906
      /w14928
      /w14946)
else()
  set(ADDITIONAL_WALL_FLAGS
      -Wall -Wextra
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Wcast-align
      -Wunused
      -Woverloaded-virtual
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
  )
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND ADDITIONAL_WALL_FLAGS -fcolor-diagnostics) # -Wlifetime
  else()
    list(APPEND ADDITIONAL_WALL_FLAGS
      -fdiagnostics-color
      -Wmisleading-indentation
      -Wduplicated-cond
      -Wduplicated-branches
      -Wlogical-op
      -Wuseless-cast
      )
  endif()
endif()

if (LNGS_APP)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  find_package(fmt REQUIRED CONFIG)
  find_package(args REQUIRED CONFIG)
  find_package(mbits-libmstch REQUIRED CONFIG)
  find_package(mbits-utfconv REQUIRED CONFIG)
  find_package(mbits-diags REQUIRED CONFIG)
endif() # LNGS_LIBRARY_ONLY
  
if (LNGS_TESTING)
  find_package(GTest REQUIRED CONFIG HINTS)

  foreach(MOD lngs)
    list(APPEND COVERALLS_DIRS
      libs/lib${MOD}/include/${MOD}
      libs/lib${MOD}/src)
  endforeach()

  list(APPEND COVERALLS_DIRS
    app/include
    app/src)

  include(tools/coveralls/Coveralls.cmake)
endif()

find_package(Git)

execute_process(
  COMMAND "${GIT_EXECUTABLE}" describe --tags HEAD
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  RESULT_VARIABLE PROJECT_VERSION_COMMIT_res
  OUTPUT_VARIABLE PROJECT_VERSION_COMMIT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT PROJECT_VERSION_COMMIT_res EQUAL 0)
  set(PROJECT_VERSION_COMMIT "")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (LNGS_TESTING)
  set(EXTERNAL_GTEST ON)
  include( ${CMAKE_CURRENT_SOURCE_DIR}/tools/testing/googletest.cmake )
endif()

define_property(TARGET PROPERTY IDL
  BRIEF_DOCS "The language file path"
  FULL_DOCS "The language file path")

set(LNGS_TOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/tools")
add_subdirectory(libs)

if (LNGS_APP)
  add_subdirectory(app)
  include(prepare_pack)
endif()
