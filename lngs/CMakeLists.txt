include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libs/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

INCLUDE (BuildConfig)

set(lngs_SRCS
	argparser.cpp
	filesystem.cpp
	gettext.cpp
	lang_codes.cpp
	languages.cpp
	lngs.cpp
	lngs_enums.cpp
	lngs_freeze.cpp
	lngs_make.cpp
	lngs_pot.cpp
	lngs_py.cpp
	lngs_res.cpp
	streams.cpp
	strings.cpp
	utf8.cpp
)

set (lngs_INCS
	config.h
	argparser.hpp
	filesystem.hpp
	gettext.hpp
	languages.hpp
	streams.hpp
	strings.cpp
	utf8.hpp
)


if (NOT CMAKE_HAVE_TS_FILESYSTEM)
find_package (Boost)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
endif (NOT CMAKE_HAVE_TS_FILESYSTEM)

add_executable(lngs ${lngs_SRCS} ${lngs_INCS})
set_target_properties(lngs PROPERTIES
	CXX_STANDARD 14
	VERSION ${LOCALE_VERSION})

if (NOT CMAKE_HAVE_TS_FILESYSTEM)
target_link_libraries(lngs boost_system boost_filesystem)
endif (NOT CMAKE_HAVE_TS_FILESYSTEM)

install(TARGETS lngs DESTINATION bin)
