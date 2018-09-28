/*
 * Copyright (C) 2015 midnightBITS
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
#include <locale/file.hpp>
#include <vector>

namespace lngs {
	struct idl_string {
		std::string key;
		std::string value;
		std::string help;
		std::string plural;
		int id = -2;
		int original_id = -2;
		int id_offset = -1;
	};

	struct idl_strings {
		std::string project;
		std::string version;
		uint32_t serial = 0;
		int serial_offset = -1;
		bool has_new = false;
		std::vector<idl_string> strings;
	};

	class diagnostics;
	class source_file;
	bool read_strings(source_file in, idl_strings& str, diagnostics& diag);
	bool read_strings(const std::string& progname, const fs::path& inname, idl_strings& str, bool verbose, diagnostics& diag);
}
