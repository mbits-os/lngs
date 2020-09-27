// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

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
