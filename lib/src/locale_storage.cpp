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
#endif

#ifdef WIN32_LOCALES
#include <windows.h>
#endif // WIN32_LOCALES

namespace locale {
	inline bool inside(const std::vector<std::string>& locales, const std::string& key)
	{
		for (auto&& locale : locales) {
			if (locale == key)
				return true;
		}
		return false;
	}

	void appendLocale(std::vector<std::string>& locales, const std::string & locale)
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

	std::vector<std::string> system_locales()
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
#endif
}
