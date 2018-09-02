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
#include <map>
#include <vector>
#include <locale/locale_base.hpp>
#include <locale/file.hpp>

namespace locale {
	struct string {
		string_key key;
		std::string value;

		string();
		string(string&&);
		string& operator=(string&&);
		string(const string&);
		string& operator=(const string&);

		string(uint32_t id, const std::string& val) : value(val)
		{
			key.id = id;
			key.length = (uint32_t)value.length();
		}
	};

	struct String;

	std::string warp(const std::string& s);
	std::vector<string> attributes(const std::map<std::string, std::string>& gtt);
	std::vector<string> translations(const std::map<std::string, std::string>& gtt, const std::vector<String>& strings, bool warp_missing, bool verbose);
	bool ll_CC(const fs::path& in, std::map<std::string, std::string>& langs);

	struct outstream;
	struct file {
		uint32_t serial;
		std::vector<string> attrs;
		std::vector<string> strings;
		std::vector<string> keys;

		int write(outstream& os);
	};
}
