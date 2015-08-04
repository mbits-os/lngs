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

#include <locale_storage.hpp>

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
#include "str.hpp"
#endif // GNU_LOCALES

namespace locale {
	static inline bool inside(const std::vector<std::string>& locales, const std::string& key)
	{
		for (auto&& locale : locales) {
			if (locale == key)
				return true;
		}
		return false;
	}

	static void appendLocale(std::vector<std::string>& locales, const std::string & locale)
	{
		auto candidate = locale;
		while (true) {
			if (inside(locales, candidate))
				break;

			locales.push_back(candidate);

			auto pos = candidate.find_last_of('-');
			if (pos == std::string::npos)
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

	template <size_t length>
	static inline bool starts_with(const char* value, size_t len, const char(&test)[length])
	{
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

		for (auto& s : split(value, ":")) {
			auto pos = s.find('.');
			if (pos != std::string::npos)
				s = s.substr(0, pos);
			for (auto& c : s) {
				if (c == '_')
					c = '-';
			}
			appendLocale(locales, s);
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
}
