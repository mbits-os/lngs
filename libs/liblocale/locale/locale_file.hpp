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

#include <locale/locale_base.hpp>
#include <locale/plurals.hpp>
#include <memory>
#include <string_view>

namespace locale {

	struct memory_view {
		const char* contents = nullptr;
		uintmax_t size = 0;
	};

	struct lang_file {
		enum class identifier : uint32_t {};
		enum class quantity : intmax_t {};
		lang_file() noexcept;
		~lang_file() noexcept { close(); }
		bool open(const memory_view& view) noexcept;
		void close() noexcept;
		std::string_view get_string(identifier id) const noexcept;
		std::string_view get_string(identifier id, quantity count) const noexcept;
		std::string_view get_attr(uint32_t id) const noexcept;
		std::string_view get_key(uint32_t id) const noexcept;
		uint32_t find_key(std::string_view id) const noexcept;
		uint32_t size() const noexcept { return strings.count; }
		intmax_t calc_substring(quantity count) const;
	private:
		struct section {
			uint32_t count = 0;
			const string_key* keys = nullptr;
			const char* strings = nullptr;
			void close() noexcept
			{
				count = 0; keys = nullptr; strings = nullptr;
			}
			const string_key* get(identifier id) const noexcept;
			std::string_view string(identifier id) const noexcept;
			std::string_view string(const string_key* key) const noexcept;
			std::string_view string(const string_key& key) const noexcept;

			const string_key* begin() const noexcept { return keys; }
			const string_key* end() const noexcept { return keys + count; }
		};
		section attrs;
		section strings;
		section keys;
		mutable plurals::lexical lex;
	};
}
