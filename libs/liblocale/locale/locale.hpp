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

#include <locale/locale_storage.hpp>
#include <fmt/core.h>

namespace locale {
	template <typename Enum, typename Storage = storage::FileBased>
	class SingularStrings : public Storage {
	public:
		std::string operator()(Enum val) const noexcept
		{
			auto ptr = Storage::get_string((uint32_t)val);
			return ptr ? ptr : std::string{ };
		}

		template <typename... Args>
		std::string operator()(Enum val, Args&&... args) const noexcept
		{
			auto ptr = Storage::get_string((uint32_t)val);
			return ptr ? fmt::format(ptr, std::forward<Args>(args)...) : std::string{ };
		}

		std::string attr(v1_0::attr_t val) const noexcept
		{
			auto ptr = Storage::get_attr(val);
			return ptr ? ptr : std::string{ };
		}
	};

	template <typename Enum, typename Storage = storage::FileBased>
	class PluralOnlyStrings : public Storage {
	public:
		std::string operator()(Enum val, intmax_t count) const noexcept
		{
			auto ptr = Storage::get_string((uint32_t)val, count);
			return ptr ? ptr : std::string{ };
		}

		template <typename... Args>
		std::string operator()(Enum val, intmax_t count, Args&&... args) const noexcept
		{
			auto ptr = Storage::get_string((uint32_t)val, count);
			return ptr ? fmt::format(ptr, std::forward<Args>(args)...) : std::string{ };
		}
	};

	template <typename SEnum, typename PEnum, typename Storage = storage::FileBased>
	class StringsWithPlurals : public SingularStrings<SEnum, Storage> {
	public:
		using SingularStrings<SEnum, Storage>::operator(); // un-hide
		std::string operator()(PEnum val, intmax_t count) const noexcept
		{
			auto ptr = SingularStrings<SEnum, Storage>::get_string((uint32_t)val, count);
			return ptr ? ptr : std::string{ };
		}

		template <typename... Args>
		std::string operator()(PEnum val, intmax_t count, Args&&... args) const noexcept
		{
			auto ptr = SingularStrings<SEnum, Storage>::get_string((uint32_t)val, count);
			return ptr ? fmt::format(ptr, std::forward<Args>(args)...) : std::string{ };
		}
	};
}
