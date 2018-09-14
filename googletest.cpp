/*
	This file is taken almost verbatim from gmock_main.cc. Therefore
	copyright claim is retained from original file.
*/

// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: wan@google.com (Zhanyong Wan)

/*
	Additionally, any changes to the source code are covered by:
*/

/*
 * Copyright (C) 2016 midnightBITS
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

#include <iostream>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <locale/file.hpp>

#if __has_include(<default_data_path.hpp>)
#include <default_data_path.hpp>
#define HAS_DEFAULT_DATA_PATH
#endif

#ifdef HAS_DEFAULT_DATA_PATH
fs::path LOCALE_data_path{ lngs::testing::directory_info::sources };
#else
fs::path LOCALE_data_path{};
#endif

namespace {
	// Parses a string as a command line flag.  The string should have
	// the format "--flag=value".  When def_optional is true, the "=value"
	// part can be omitted.
	//
	// Returns the value of the flag, or NULL if the parsing failed.
	static const char* ParseFlagValue(const char* str, const char* flag,
		bool def_optional) {
		// str and flag must not be NULL.
		if (str == NULL || flag == NULL) return NULL;

		// The flag must start with "--" followed by GTEST_FLAG_PREFIX_.
		const std::string flag_str = std::string("--") + flag;
		const size_t flag_len = flag_str.length();
		if (strncmp(str, flag_str.c_str(), flag_len) != 0) return NULL;

		// Skips the flag name.
		const char* flag_end = str + flag_len;

		// When def_optional is true, it's OK to not have a "=value" part.
		if (def_optional && (flag_end[0] == '\0')) {
			return flag_end;
		}

		// If def_optional is true and there are more characters after the
		// flag name, or if def_optional is false, there must be a '=' after
		// the flag name.
		if (flag_end[0] != '=') return NULL;

		// Returns the string after "=".
		return flag_end + 1;
	}

	// Parses a string for a string flag, in the form of
	// "--flag=value".
	//
	// On success, stores the value of the flag in *value, and returns
	// true.  On failure, returns false without changing *value.
	template <typename String>
	static bool ParseStringFlag(const char* str, const char* flag, String* value) {
		// Gets the value of the flag as a string.
		const char* const value_str = ParseFlagValue(str, flag, false);

		// Aborts if the parsing failed.
		if (value_str == NULL) return false;

		// Sets *value to the value of the flag.
		*value = value_str;
		return true;
	}
}

#if GTEST_OS_WINDOWS
# include <tchar.h>
int _tmain(int argc, TCHAR** argv)
#else
int main(int argc, char** argv)
#endif
{
	std::cout << "Running main() from googletest.cpp\n";
	testing::InitGoogleMock(&argc, argv);
	if (argc > 0) {
		using namespace testing::internal;
		for (int i = 1; i != argc; i++) {
			std::string value;
			if (ParseStringFlag(argv[i], "data_path", &value)
				&& !value.empty()) {
				printf("Note: data used from path: %s\n", value.c_str());
				LOCALE_data_path = value;
			}
		}
	}

	return RUN_ALL_TESTS();
}