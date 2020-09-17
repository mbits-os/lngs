/*
 * Copyright (C) 2018 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <lngs/file.hpp>

namespace lngs::app::build {
	fs::path get_exec_dir();

	// clang-format off
	struct directory_info {
		constexpr static const char share[] = "@SHARE_DIR@";
		constexpr static const char prefix[] = "@CMAKE_INSTALL_PREFIX@/@SHARE_DIR@";
		constexpr static const char build[] = "@CMAKE_CURRENT_BINARY_DIR@/@SHARE_DIR@";
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
