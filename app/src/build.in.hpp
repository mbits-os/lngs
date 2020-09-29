// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <lngs/file.hpp>

namespace lngs::app::build {
	fs::path get_exec_dir();

	// clang-format off
	struct directory_info {
		constexpr static const char share[] = "@SHARE_DIR@";
		constexpr static const char data_dir[] = "@CMAKE_INSTALL_PREFIX@/@SHARE_DIR@";
		constexpr static const char lngs_install[] = "@CMAKE_INSTALL_PREFIX@/@SHARE_DIR@/locale";
		constexpr static const char lngs_build[] = "@CMAKE_CURRENT_BINARY_DIR@/@SHARE_DIR@/locale";
		constexpr static const char mstch_install[] = "@CMAKE_INSTALL_PREFIX@/@SHARE_DIR@/templates";
		constexpr static const char mstch_build[] = "@CMAKE_CURRENT_SOURCE_DIR@/data/templates";
	};

	struct version {
		constexpr static const unsigned major = @PROJECT_VERSION_MAJOR@;
		constexpr static const unsigned minor = @PROJECT_VERSION_MINOR@;
		constexpr static const unsigned patch = @PROJECT_VERSION_PATCH@;
		// "-alpha", "-beta", "-rc.1", "-rc.2", and ""
		constexpr static const char stability[] = "@PROJECT_VERSION_STABILITY@";
		constexpr static const char string[] = "@PROJECT_VERSION@";
	};
	// clang-format on
}  // namespace lngs::app::build
