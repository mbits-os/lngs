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

#include <lngs/lngs_storage.hpp>

#ifdef WIN32
#define WIN32_LOCALES
#else
#define GNU_LOCALES
#endif

#ifdef WIN32_LOCALES
#include <windows.h>
#endif // WIN32_LOCALES

#ifdef GNU_LOCALES
#include <clocale>
#include <cstdlib>
#include <memory.h>
#endif // GNU_LOCALES

#include "str.hpp"

namespace lngs {
	static inline bool inside(const std::vector<std::string>& locales, std::string_view key) noexcept
	{
		for (auto&& locale : locales) {
			if (locale == key)
				return true;
		}
		return false;
	}

	static void appendLocale(std::vector<std::string>& locales, std::string_view locale)
	{
		auto candidate = locale;
		while (true) {
			if (inside(locales, candidate))
				break;

			locales.push_back({ candidate.data(), candidate.length() });

			auto pos = candidate.find_last_of('-');
			if (pos == std::string_view::npos)
				break; // no tags in the language range

			candidate = candidate.substr(0, pos);
		};
	}

#ifdef WIN32_LOCALES

	static std::string narrow(const wchar_t* s)
	{
		if (!s)
			return { };

		auto size = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
		std::unique_ptr<char[]> out { new char[size + 1] };
		WideCharToMultiByte(CP_UTF8, 0, s, -1, out.get(), size + 1, nullptr, nullptr);
		return out.get();
	}

	std::vector<std::string> system_locales(bool)
	{
		auto get_locale = [](auto&& fun) {
			wchar_t locale[LOCALE_NAME_MAX_LENGTH];

			fun(locale, LOCALE_NAME_MAX_LENGTH);
			return narrow(locale);
		};

		std::vector<std::string> out;

		appendLocale(out, get_locale(GetUserDefaultLocaleName));
		appendLocale(out, get_locale(GetSystemDefaultLocaleName));
		appendLocale(out, "en");

		return out;
	}

#elif defined(GNU_LOCALES)

	template <size_t size>
	static inline bool starts_with(const char* value, size_t len, const char(&test)[size])
	{
		constexpr const auto length = size ? size - 1 : 0;
		// if value == test || value == test + "." + something else
		if ((length == len || (length > len && value[length] == '.')) && !strncmp(value, test, length))
			return true;

		return false;
	}

	static void expand(std::vector<std::string>& locales, const char* value)
	{
		if (!value || !*value)
			return;

		auto len = strlen(value);
		if (starts_with(value, len, "C"))
			return;
		if (starts_with(value, len, "POSIX"))
			return;

		if (len > 3 && !strncmp(value, "LC_", 3) && strchr(value, ';') && strchr(value, '='))
			return; // LC_ALL might be "LC_CTYPE=...;LC_...=...;..." if one has outstanding value

		for (auto& s : split_view(value, ":")) {
			auto pos = s.find('.');
			if (pos != std::string::npos)
				s = s.substr(0, pos);
			auto str = std::string{ s };
			for (auto& c : str) {
				if (c == '_')
					c = '-';
			}
			appendLocale(locales, str);
		}
	}

	static void try_locale(std::vector<std::string>& locales, int cat)
	{
		expand(locales, std::setlocale(cat, nullptr));
	}

	static void try_environment(std::vector<std::string>& locales, const char* name)
	{
		expand(locales, std::getenv(name));
	}

	std::vector<std::string> system_locales(bool init_setlocale)
	{
		if (init_setlocale)
			std::setlocale(LC_ALL, "");

		std::vector<std::string> locales;
		try_environment(locales, "LANGUAGE");
		try_locale(locales, LC_ALL);
#ifdef LC_MESSAGES
		try_locale(locales, LC_MESSAGES);
#endif
		try_environment(locales, "LC_ALL");
		try_environment(locales, "LC_MESSAGES");
		try_environment(locales, "LANG");
		appendLocale(locales, "en");
		return locales;
	}

#endif

	struct ListItem {
		std::string m_value;
		size_t m_pos;
		size_t m_q;

		// SORT q DESC, pos ASC
		bool operator < (const ListItem& right) const
		{
			if (m_q != right.m_q) return m_q > right.m_q;
			return m_pos < right.m_pos;
		}

#define WS do { while (isspace((unsigned char)*c) && c < end) c++; } while(0)
#define LOOK_FOR(ch) do { while (!isspace((unsigned char)*c) && *c != ',' && *c != (ch) && c < end) c++; } while(0)

		const char* read(size_t pos, const char* c, const char* end)
		{
			m_pos = pos;
			m_q = 1000;

			WS;
			const char* token = c;
			LOOK_FOR(';');
			m_value.assign(token, c);
			WS;
			while (*c == ';') {
				c++;
				WS;
				token = c;
				LOOK_FOR('=');
				if (c - token == 1 && *token == 'q') {
					WS;
					if (*c == '=') {
						c++;
						WS;
						token = c;
						LOOK_FOR(';');
						m_q = quality(token, c);
						WS;
					}
				} else LOOK_FOR(';');
			}
			return ++c;
		}
		static size_t quality(const char* c, const char* end)
		{
			size_t q = 0;
			size_t pow = 1000;
			bool seen_dot = false;
			while (c != end) {
				switch (*c) {
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					q *= 10;
					q += *c - '0';
					if (seen_dot) {
						pow /= 10;
						if (pow == 1)
							return q;
					}
					break;
				case '.':
					if (seen_dot)
						return q * pow;
					seen_dot = true;
					break;
				default:
					return q * pow;
				};
				++c;
			}
			return q * pow;
		}
	};

	std::vector<std::string> priority_list(std::string_view header)
	{
		std::vector<ListItem> items;

		const char* c = header.data();
		const char* e = c + header.length();
		size_t pos = 0;
		while (c < e) {
			ListItem it;
			c = it.read(pos++, c, e);
			items.push_back(it);
		}

		std::stable_sort(items.begin(), items.end());
		std::vector<std::string> out;
		out.reserve(items.size());
		for (auto& item : items) {
			if (!inside(out, item.m_value))
				out.push_back(std::move(item.m_value));
		}
		return out;
	}

	bool expand_list(std::vector<std::string>& langs)
	{
		for (auto&& lang : langs) {
			auto pos = lang.find_last_of('-');
			if (pos == std::string::npos)
				continue; // no tags in the language range

			auto sub = lang.substr(0, pos);
			if (inside(langs, sub))
				continue; // sub-range already in list

			auto sub_len = sub.length();
			auto insert = langs.end();
			auto cur = langs.begin(), end = langs.end();
			for (; cur != end; ++cur) {
				if (
					cur->compare(0, sub_len, sub) == 0 &&
					cur->length() > sub_len &&
					cur->at(sub_len) == '-'
					) {
					insert = cur;
				}
			}

			if (insert != langs.end())
				++insert;

			langs.insert(insert, sub);
			return true;
		}
		return false;
	}

	std::vector<std::string> http_accept_language(std::string_view header)
	{
		auto out = priority_list(header);
		while (expand_list(out));

		if (!inside(out, "en"))
			out.push_back("en");

		return out;
	}
}
