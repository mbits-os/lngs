#pragma once
#include <vector>
#include <string>
#include <cctype>

inline std::vector<std::string> split(const std::string& str, const std::string& sep)
{
	std::vector<std::string> out;
	auto pos = str.find(sep, 0);
	decltype(pos) prev = 0;
	while (pos != std::string::npos) {
		out.push_back(str.substr(prev, pos - prev));
		prev = pos + sep.length();
		pos = str.find(sep, prev);
	}

	out.push_back(str.substr(prev));
	return out;
}

inline std::vector<std::string> split(const std::string& str, const std::string& sep, size_t count)
{
	std::vector<std::string> out;
	auto pos = str.find(sep, 0);
	decltype(pos) prev = 0;
	while (pos != std::string::npos && count) {
		--count;
		out.push_back(str.substr(prev, pos - prev));
		prev = pos + sep.length();
		pos = str.find(sep, prev);
	}

	out.push_back(str.substr(prev));
	return out;
}

inline std::string join(const std::vector<std::string>& list, const std::string& sep)
{
	if (list.empty())
		return{};

	size_t len = sep.length() * (list.size() - 1);

	for (auto& item : list)
		len += item.length();

	std::string out;
	out.reserve(len + 1);
	bool first = true;
	for (auto& item : list) {
		if (first) first = false;
		else out += sep;
		out += item;
	}
	return out;
}

inline std::string strip(const std::string& s) {
	auto b = std::begin(s);
	auto e = std::end(s);

	while (b != e && std::isspace((uint8_t)b[0]))
		++b;

	while (b != e && std::isspace((uint8_t)e[-1]))
		--e;

	return{ b, e };
}
