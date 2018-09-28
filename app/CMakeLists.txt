if (LOCALE_WALL_FLAGS)
  add_definitions(${ADDITIONAL_WALL_FLAGS})
endif()

set(SHARE_DIR share/lngs-${LOCALE_VERSION_SHORT})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dirs.in.hpp ${CMAKE_CURRENT_BINARY_DIR}/dirs.hpp @ONLY)

message(STATUS "SHARE: ${CMAKE_INSTALL_PREFIX}/${SHARE_DIR}")

file(GLOB PO_FILES strings/*.po)

set(SOURCES
	lngs.cpp
	dirs.in.hpp
	${CMAKE_CURRENT_BINARY_DIR}/dirs.hpp
	${PO_FILES})

add_executable(lngs ${SOURCES})
set_target_properties(lngs PROPERTIES
	VERSION ${LOCALE_VERSION_SHORT}
	FOLDER tools)
target_include_directories(lngs
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_BINARY_DIR}"
		)
target_link_libraries(lngs liblngs)

install(TARGETS lngs DESTINATION bin)

get_target_property(LNGS_IDL liblngs IDL)

add_custom_target(lngs_pot
	COMMAND
		$<TARGET_FILE:lngs> pot -v
			-o '${CMAKE_CURRENT_SOURCE_DIR}/strings/lngs.pot'
			-i '${LNGS_IDL}'
			-a 'Marcin Zdun <mzdun@midnightbits.com>'
			-c midnightBITS
			-t "Locale File Processor translation"

	DEPENDS
		lngs
		"${LNGS_IDL}"
	BYPRODUCTS
		"${CMAKE_CURRENT_SOURCE_DIR}/strings/lngs.pot"
	COMMENT "Regenerating POT file...")
set_target_properties(lngs_pot PROPERTIES FOLDER tools/strings)

find_program(MSGFMT NAMES msgfmt)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/strings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR})

foreach(PO ${PO_FILES})
	get_filename_component(PO_NAME ${PO} NAME_WE)
	set(MO ${CMAKE_CURRENT_BINARY_DIR}/strings/${PO_NAME}.mo)
	set(LNG ${CMAKE_CURRENT_BINARY_DIR}/${SHARE_DIR}/lngs.${PO_NAME})

	if (MSVC)
		string(REPLACE / \\ PO "${PO}")
		string(REPLACE / \\ MO "${MO}")
	endif()

	add_custom_command(
		TARGET lngs POST_BUILD
		COMMAND ${MSGFMT} -o "${MO}" "${PO}"

		BYPRODUCTS ${MO}
		DEPENDS "${PO}"
		COMMENT "[${PO_NAME}] compiling")

	add_custom_command(
		TARGET lngs POST_BUILD
		COMMAND
			$<TARGET_FILE:lngs> make
				-i "${LNGS_IDL}"
				-m "${MO}"
				-o "${LNG}"

		BYPRODUCTS ${LNG}
		DEPENDS "${MO}"
		IMPLICIT_DEPENDS $<TARGET_FILE:lngs>
		COMMENT "[${PO_NAME}] finalizing")
endforeach()
install(DIRECTORY ${SHARE_DIR}/ DESTINATION ${SHARE_DIR})

get_filename_component(IDL_DIR ${LNGS_IDL} DIRECTORY)

add_custom_target(lngs_enums
  COMMAND
    $<TARGET_FILE:lngs> res
			-i "${LNGS_IDL}"
			-o "${IDL_DIR}/lngs_res.cpp"
			--include lngs/strings/lngs.hpp
  COMMAND
    $<TARGET_FILE:lngs> enums -r
			-i "${LNGS_IDL}"
			-o "${IDL_DIR}/../lngs/strings/lngs.hpp"
  DEPENDS
		lngs
		"${LNGS_IDL}")
set_target_properties(lngs_enums PROPERTIES FOLDER tools/strings)
