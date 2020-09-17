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
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace {
	unsigned char s2uc(char c) { return static_cast<unsigned char>(c); }
}  // namespace

inline std::vector<std::string_view> split_view(std::string_view str,
                                                std::string_view sep) {
	std::vector<std::string_view> out;
	auto pos = str.find(sep, 0);
	decltype(pos) prev = 0;
	while (pos != std::string_view::npos) {
		out.push_back(str.substr(prev, pos - prev));
		prev = pos + sep.length();
		pos = str.find(sep, prev);
	}

	out.push_back(str.substr(prev));
	return out;
}

inline std::vector<std::string_view> split_view(std::string_view str,
                                                std::string_view sep,
                                                size_t count) {
	std::vector<std::string_view> out;
	auto pos = str.find(sep, 0);  // noexcept
	decltype(pos) prev = 0;
	while (pos != std::string_view::npos && count) {
		--count;
		out.push_back(str.substr(prev, pos - prev));
		prev = pos + sep.length();
		pos = str.find(sep, prev);
	}

	out.push_back(str.substr(prev));
	return out;
}

inline std::string join(const std::vector<std::string_view>& list,
                        std::string_view sep) {
	if (list.empty()) return {};

	size_t len = sep.length() * (list.size() - 1);

	for (auto& item : list)
		len += item.length();

	std::string out;
	out.reserve(len + 1);
	bool first = true;
	for (auto& item : list) {
		if (first)
			first = false;
		else
			out += sep;
		out += item;
	}
	return out;
}

inline std::string strip(std::string_view s) {
	auto b = std::begin(s);
	auto e = std::end(s);

	while (b != e && std::isspace(s2uc(b[0])))
		++b;

	while (b != e && std::isspace(s2uc(e[-1])))
		--e;

	return {b, e};
}
