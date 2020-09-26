cmake_minimum_required (VERSION 3.10)
project (lngs
  VERSION 0.4.0
  DESCRIPTION "Locale File Format And Processor"
  LANGUAGES CXX)

set(PROJECT_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PROJECT_VERSION_STABILITY "-alpha")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(LNGS_TESTING ON CACHE BOOL "Compile and/or run self-tests")

if (MSVC)
  set(ADDITIONAL_WALL_FLAGS
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

foreach(MOD lngs)
  list(APPEND COVERALLS_DIRS
    libs/lib${MOD}/include/${MOD}
    libs/lib${MOD}/src)
endforeach()

list(APPEND COVERALLS_DIRS
  app/include
  app/src)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
  
include(tools/coveralls/Coveralls.cmake)

if (COVERALLS AND NOT LNGS_TESTING)
  message(FATAL_ERROR "Coveralls are useless without -DLNGS_TESTING=ON")
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

add_subdirectory(external)
add_subdirectory(libs)
add_subdirectory(app)

include(prepare_pack)
