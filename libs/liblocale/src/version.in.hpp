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
#include <string_view>

namespace locale {
	struct version {
		constexpr static const unsigned major = @LOCALE_VERSION_MAJOR@;
		constexpr static const unsigned minor = @LOCALE_VERSION_MINOR@;
		constexpr static const unsigned patch = @LOCALE_VERSION_PATCH@;
		constexpr static const unsigned build = @LOCALE_VERSION_BUILD@;
		constexpr static const char stability[] = "@LOCALE_VERSION_STABILITY@"; // "-alpha", "-beta", "-rc.1", "-rc.2", and ""
		constexpr static const char string[] = "@LOCALE_VERSION@";
		constexpr static const char full[] = "@LOCALE_VERSION_FULL@";
		constexpr static const char commit[] = "@LOCALE_VERSION_COMMIT@";
		constexpr static const bool has_commit = !!*commit;
	};

	struct rt_version {
		const unsigned major;
		const unsigned minor;
		const unsigned patch;
		const unsigned build;
		const std::string_view stability;
		const std::string_view string;
		const std::string_view full;
		const std::string_view commit;
	};

	rt_version get_version();
};