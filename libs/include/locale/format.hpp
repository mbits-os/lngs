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

#include <string>
#include <vector>

namespace str {
	namespace detail {
		std::string format(const char* fmt, const std::vector<std::string>& packed);

		inline void pack(std::vector<std::string>&) {}

		template <typename T>
		struct conv {
			static std::string to_string(const T& cref)
			{
				return std::to_string(cref);
			}
		};

		template <> struct conv<std::string> {
			static std::string to_string(const std::string& cref) { return cref; }
			static std::string to_string(std::string& cref) { return cref; }
			static std::string to_string(std::string&& cref) { return std::move(cref); }
		};

		template <> struct conv<const char*> {
			static std::string to_string(const char* cref) { return cref; }
		};

		template <> struct conv<char*> {
			static std::string to_string(char* cref) { return cref; }
		};

		template <typename T, typename... Args>
		inline void pack(std::vector<std::string>& packed, T&& arg, Args&&... args)
		{
			packed.push_back(std::move(detail::conv<std::decay_t<T>>::to_string(std::forward<T>(arg))));
			pack(packed, std::forward<Args>(args)...);
		}
	}
	template <typename... Args>
	inline std::string format(const char* fmt, Args&&... args)
	{
		std::vector<std::string> packed;
		packed.reserve(sizeof...(args));
		detail::pack(packed, std::forward<Args>(args)...);
		return detail::format(fmt, packed);
	}

	template <typename... Args>
	inline std::string format(const std::string& fmt, Args&&... args)
	{
		return format(fmt.c_str(), std::forward<Args>(args)...);
	}
}
