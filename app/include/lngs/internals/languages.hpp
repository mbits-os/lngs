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
#include <lngs/lngs_base.hpp>
#include <lngs/file.hpp>

namespace lngs::app {
	struct tr_string {
		string_key key;
		std::string value;

		tr_string();
		~tr_string();
		tr_string(tr_string&&);
		tr_string& operator=(tr_string&&);
		tr_string(const tr_string&);
		tr_string& operator=(const tr_string&);

		tr_string(uint32_t id, std::string val) : value(std::move(val))
		{
			key.id = id;
			key.length = (uint32_t)value.length();
		}
	};

	struct idl_string;
	class source_file;
	class diagnostics;

	std::string warp(const std::string& s);
	std::map<std::string, std::string, std::less<>> attrGTT(std::string_view attrs);
	std::vector<tr_string> attributes(const std::map<std::string, std::string>& gtt);
	std::vector<tr_string> translations(const std::map<std::string, std::string>& gtt,
	                                    const std::vector<idl_string>& strings,
	                                    bool warp_missing, bool verbose,
	                                    source_file& src, diagnostics& diags);
	bool ll_CC(source_file is, diagnostics& diags, std::map<std::string, std::string>& langs);

	struct outstream;
	struct file {
		uint32_t serial{0};
		std::vector<tr_string> attrs{};
		std::vector<tr_string> strings{};
		std::vector<tr_string> keys{};

		int write(outstream& os);
	};
}
