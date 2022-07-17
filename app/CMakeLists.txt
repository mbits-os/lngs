set(lngs_app_SRCS
  src/cmds/cmd_freeze_ids.cpp
  src/cmds/cmd_load_msgs.cpp
  src/cmds/cmd_write_enums.cpp
  src/cmds/cmd_write_pot.cpp
  src/cmds/cmd_write_py.cpp
  src/cmds/cmd_write_res.cpp
  src/gettext.cpp
  src/gettext_plain.cpp
  src/languages.cpp
  src/mstch_engine.cpp
  src/lang_codes.cpp
  src/lngs_res.cpp
  src/strings.cpp
)

set (lngs_app_INCS
  include/lngs/internals/commands.hpp
  include/lngs/internals/diagnostics.hpp
  include/lngs/internals/gettext.hpp
  include/lngs/internals/languages.hpp
  include/lngs/internals/mstch_engine.hpp
  include/lngs/internals/strings.hpp
  include/lngs/internals/strings/lngs.hpp
)

if (LNGS_LINKED_RESOURCES)
	file(GLOB MSTCH_FILES data/templates/*.mustache)
	add_custom_command(
		OUTPUT
			"${CMAKE_CURRENT_BINARY_DIR}/src/templates.cpp"
			"${CMAKE_CURRENT_BINARY_DIR}/src/templates.hpp"
		COMMAND
			"${Python3_EXECUTABLE}"
			"${LNGS_TOOLS_DIR}/embed.py"
			-O "${CMAKE_CURRENT_BINARY_DIR}/src/templates.cpp"
			--inc-dir "${CMAKE_CURRENT_BINARY_DIR}/src"
			--ns "lngs::templates"
			${MSTCH_FILES}
		DEPENDS
			"${LNGS_TOOLS_DIR}/embed.py"
			${MSTCH_FILES}
	)
	list(APPEND lngs_app_SRCS
			"${CMAKE_CURRENT_BINARY_DIR}/src/templates.cpp"
			"${CMAKE_CURRENT_BINARY_DIR}/src/templates.hpp"
	)
endif(LNGS_LINKED_RESOURCES)

add_library(lngs_app STATIC ${lngs_app_SRCS} ${lngs_app_INCS})
set_target_properties(lngs_app PROPERTIES
  FOLDER app
  IDL "${CMAKE_CURRENT_SOURCE_DIR}/src/lngs.idl")
target_compile_options(lngs_app PUBLIC ${ADDITIONAL_WALL_FLAGS})
target_compile_definitions(lngs_app PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(lngs_app
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  	${CMAKE_CURRENT_SOURCE_DIR}/src
  	${CMAKE_CURRENT_BINARY_DIR}/src)

if (LNGS_LINKED_RESOURCES)
	target_compile_definitions(lngs_app PUBLIC LNGS_LINKED_RESOURCES)
endif(LNGS_LINKED_RESOURCES)

target_link_libraries(lngs_app PUBLIC
	liblngs
	mbits::args
	fmt::fmt
	mbits-libmstch::libmstch
	mbits-utfconv::utfconv
	mbits-diags::diags
)

set(SHARE_DIR share/lngs-${PROJECT_VERSION_SHORT})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/build.in.hpp ${CMAKE_CURRENT_BINARY_DIR}/src/build.hpp @ONLY)

message(STATUS "SHARE: ${CMAKE_INSTALL_PREFIX}/${SHARE_DIR}")

file(GLOB PO_FILES data/strings/*.po)

set(SOURCES
	src/lngs.cpp
	src/build.in.hpp
	${CMAKE_CURRENT_BINARY_DIR}/src/build.hpp
	${PO_FILES})
if (LNGS_LINKED_RESOURCES)
	list(APPEND SOURCES
		src/embedded_languages.cpp
		src/embedded_languages.hpp
	)
endif()

add_executable(lngs ${SOURCES})
set_target_properties(lngs PROPERTIES
	VERSION ${PROJECT_VERSION_SHORT}
	FOLDER app)
target_include_directories(lngs
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_BINARY_DIR}/src"
)
target_link_libraries(lngs PRIVATE lngs_app)

if (LNGS_INSTALL)
	install(TARGETS lngs DESTINATION bin)
endif()

get_target_property(LNGS_IDL lngs_app IDL)

add_custom_target(lngs_pot
	COMMAND
		$<TARGET_FILE:lngs> pot -v
			"${LNGS_IDL}"
			-o "${CMAKE_CURRENT_SOURCE_DIR}/data/strings/lngs.pot"
			-a "Marcin Zdun <mzdun@midnightbits.com>"
			-c midnightBITS
			-t "Locale File Processor translation"

	DEPENDS
		lngs
		"${LNGS_IDL}"
	BYPRODUCTS
		"${CMAKE_CURRENT_SOURCE_DIR}/data/strings/lngs.pot"
	COMMENT "Regenerating POT file...")
set_target_properties(lngs_pot PROPERTIES FOLDER tools/strings)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/strings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR})

set(PO_NAMES)
set(LNG_FILES)
foreach(PO ${PO_FILES})
	get_filename_component(PO_NAME ${PO} NAME_WE)
	list(APPEND PO_NAMES ${PO_NAME})
	if (LNGS_LINKED_RESOURCES)
		set(LNG ${CMAKE_CURRENT_BINARY_DIR}/data/strings/${PO_NAME}.lng)
	else()
		set(LNG ${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR}/locale/${PO_NAME}/lngs.lng)
	endif()
	list(APPEND LNG_FILES ${LNG})

	if (MSVC)
		string(REPLACE / \\ PO "${PO}")
	endif()

	if (LNGS_LINKED_RESOURCES)
		add_custom_command(
			TARGET lngs POST_BUILD
			COMMAND
				$<TARGET_FILE:lngs> make
					"${LNGS_IDL}"
					-m "${PO}"
					-o "${LNG}"

			BYPRODUCTS ${LNG}
			DEPENDS "${PO}"
			IMPLICIT_DEPENDS $<TARGET_FILE:lngs>
			COMMENT "[${PO_NAME}] finalizing")
	else(LNGS_LINKED_RESOURCES)
		add_custom_command(
			TARGET lngs POST_BUILD
			COMMAND
				${CMAKE_COMMAND} -E
					make_directory ${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR}/locale/${PO_NAME}
			COMMAND
				$<TARGET_FILE:lngs> make
					"${LNGS_IDL}"
					-m "${PO}"
					-o "${LNG}"

			BYPRODUCTS ${LNG}
			DEPENDS "${PO}"
			IMPLICIT_DEPENDS $<TARGET_FILE:lngs>
			COMMENT "[${PO_NAME}] finalizing")
	endif(LNGS_LINKED_RESOURCES)
endforeach()

if (PO_NAMES)
	message(STATUS "Found strings for: ${PO_NAMES}")
endif()

if (LNGS_LINKED_RESOURCES)
	add_custom_target(
		lngs_embedd ALL
		BYPRODUCTS
			"${CMAKE_CURRENT_BINARY_DIR}/src/generated_languages.cpp"
			"${CMAKE_CURRENT_BINARY_DIR}/src/generated_languages.hpp"
		COMMAND
			"${Python3_EXECUTABLE}"
			"${LNGS_TOOLS_DIR}/embed.py"
			-O "${CMAKE_CURRENT_BINARY_DIR}/src/generated_languages.cpp"
			--inc-as "embedded_languages.hpp"
			--ns "lngs::languages"
			${LNG_FILES}
		COMMAND
			"${Python3_EXECUTABLE}"
			"${LNGS_TOOLS_DIR}/update_src.py"
			-S "${CMAKE_CURRENT_SOURCE_DIR}/src"
			-B "${CMAKE_CURRENT_BINARY_DIR}/src"
			generated_languages.cpp:embedded_languages.cpp
			generated_languages.hpp:embedded_languages.hpp
		DEPENDS
			"${LNGS_TOOLS_DIR}/embed.py"
			${LNG_FILES}
		COMMENT "Preparing languages for embedding"
	)
	set_target_properties(lngs_embedd PROPERTIES FOLDER tools/strings)
endif(LNGS_LINKED_RESOURCES)

if (LNGS_INSTALL AND NOT LNGS_LINKED_RESOURCES)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR}/ DESTINATION ${SHARE_DIR})
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/templates DESTINATION ${SHARE_DIR})
endif()

get_filename_component(IDL_DIR ${LNGS_IDL} DIRECTORY)

add_custom_target(lngs_enums
  COMMAND
    $<TARGET_FILE:lngs> res
			"${LNGS_IDL}"
			-o "${IDL_DIR}/lngs_res.cpp"
			--include lngs/internals/strings/lngs.hpp
  COMMAND
    $<TARGET_FILE:lngs> enums -r
			"${LNGS_IDL}"
			-o "${IDL_DIR}/../include/lngs/internals/strings/lngs.hpp"
  DEPENDS
		lngs
		"${LNGS_IDL}"
  COMMENT "Renegerating lng list")
set_target_properties(lngs_enums PROPERTIES FOLDER tools/strings)

##################################################################
##  TESTING
##################################################################

if (LNGS_TESTING)

add_test_executable(lngs-test DATA_PATH data LIBRARIES lngs_app)

add_test(NAME lngs.source_location COMMAND lngs-test --gtest_filter=source_location.*)
add_test(NAME lngs.diagnostics COMMAND lngs-test --gtest_filter=*diag_write.*:*/diag_ne.* --data_path=${DATA_DIR})
add_test(NAME lngs.read_idl COMMAND lngs-test --gtest_filter=*/read.*:*/read_file.* --data_path=${DATA_DIR})
add_test(NAME lngs.freeze COMMAND lngs-test --gtest_filter=*/frozen.*)
add_test(NAME lngs.res COMMAND lngs-test --gtest_filter=*/res_make.*:*/res_write.*:res_read.*)
add_test(NAME lngs.enums COMMAND lngs-test --gtest_filter=*/enums.*)
add_test(NAME lngs.python COMMAND lngs-test --gtest_filter=*/python.*)
add_test(NAME lngs.pot COMMAND lngs-test --gtest_filter=*/pot.*:*/pot_year.*)
add_test(NAME lngs.utf COMMAND lngs-test --gtest_filter=*/utf_conv.*:*/utf_errors.*)
add_test(NAME lngs.gettext COMMAND lngs-test --gtest_filter=*/gettext.*:*/gtt_attr_*.*:*/gtt_extract.*:*/mo_*.* --data_path=${DATA_DIR})
add_test(NAME lngs.gettext.plain COMMAND lngs-test --gtest_filter=*/gettext_plain.*:*/po_*.* --data_path=${DATA_DIR})
add_test(NAME lngs.streams COMMAND lngs-test --gtest_filter=streams.* --data_path=${DATA_DIR})
add_test(NAME lngs.mstch COMMAND lngs-test --gtest_filter=mstch.* --data_path=${DATA_DIR})
add_test(
  NAME lngs.exec
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/exe_driver.py"
	"$<TARGET_FILE_DIR:lngs>/$<TARGET_FILE_PREFIX:lngs>lngs$<TARGET_FILE_SUFFIX:lngs>"
	"${DATA_DIR}"
	"${CMAKE_INSTALL_PREFIX}/${SHARE_DIR}"
    )

endif()
