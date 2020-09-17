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

#line 26 "libs/liblngs/src/version.in.hpp"

#pragma once
#include <string_view>

namespace lngs {
	// clang-format off
	struct version_info {
		static constexpr unsigned major = @PROJECT_VERSION_MAJOR@;
		static constexpr unsigned minor = @PROJECT_VERSION_MINOR@;
		static constexpr unsigned patch = @PROJECT_VERSION_PATCH@;
		// "-alpha", "-beta", "-rc.1", "-rc.2", and ""
		static constexpr char stability[] = "@PROJECT_VERSION_STABILITY@";
		static constexpr char string[] = "@PROJECT_VERSION@";
		static constexpr char shrt[] = "@PROJECT_VERSION_SHORT@";
		static constexpr char commit[] = "@PROJECT_VERSION_COMMIT@";
		static constexpr bool has_commit = !!*commit;
	};
	// clang-format on

	struct version_type {
		unsigned major;
		unsigned minor;
		unsigned patch;
		std::string_view stability;
		std::string_view commit;
	};

	inline constexpr version_type version{
	    version_info::major, version_info::minor, version_info::patch,
	    version_info::stability, version_info::commit};

	version_type get_version();
}  // namespace lngs