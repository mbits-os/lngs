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

#include <locale/locale_file.hpp>
#include <cstring>

namespace locale {
	const string_key* lang_file::section::get(uint32_t id) const noexcept
	{
		auto end = keys + count;
		auto cur = keys;
		while (end != cur) {
			if (cur->id == id)
				return cur;
			++cur;
		}
		return nullptr;
	}

	const char* lang_file::section::string(uint32_t id) const noexcept
	{
		auto key = get(id);
		if (!key)
			return nullptr;

		return key->offset + strings;
	}

	const char* lang_file::section::string(const string_key* key) const noexcept
	{
		if (!key)
			return nullptr;
		return key->offset + strings;
	}

	lang_file::lang_file()
	{
	}

	uint32_t bitsIn(uint32_t test)
	{
		uint32_t acc = 0;
		while (test) {
			if (test & 1)
				++acc;
			test >>= 1;
		}

		return acc;
	}

	bool lang_file::open(const memory_view& view)
	{
		constexpr uint32_t header_size = sizeof(uint32_t) + sizeof(file_header);
		constexpr uint32_t ver_1_x = 0x0000FFFFu;

		if (!view.contents || view.size < header_size)
			return false;

		auto ints = view.size / sizeof(uint32_t);
		auto uints = reinterpret_cast<const uint32_t*>(view.contents);

		auto file_tag = *uints++;
		auto fhdr = reinterpret_cast<const file_header*>(uints);
		if (file_tag != langtext_tag || fhdr->id != hdrtext_tag)
			return false;
		if (fhdr->ints + 2 < sizeof(file_header) / sizeof(uint32_t))
			return false;
		if (fhdr->ints > ints)
			return false;

		if (fhdr->version != v1_0::version) { // == 1.0?
			if ((fhdr->version & ver_1_x) != v1_0::version) // == 1.x?
				return false;
		}

		auto read_strings = [](const string_header* sec, section& out) -> bool
		{
			out.close();

			if (sec->string_offset - sizeof(section_header) / sizeof(uint32_t) > sec->ints)
				return false;

			auto uints_for_keys = sec->string_offset - sizeof(string_header) / sizeof(uint32_t);
			if (uints_for_keys < sec->string_count * sizeof(string_key) / sizeof(uint32_t))
				return false;

			auto space_for_strings = (sec->ints + sizeof(section_header) / sizeof(uint32_t) - sec->string_offset) * sizeof(uint32_t);

			auto keys = reinterpret_cast<const string_key*>(sec + 1);
			auto strings = reinterpret_cast<const char*>(
				reinterpret_cast<const uint32_t*>(sec) + sec->string_offset
				);
			for (auto cur = keys, end = keys + sec->string_count;
			cur != end; ++cur) {
				auto& key = *cur;
				if (key.offset > space_for_strings)
					return false;
				if (key.offset + key.length > space_for_strings)
					return false;
				if (strings[key.offset + key.length] != 0)
					return false;
			}

			out.count = sec->string_count;
			out.strings = strings;
			out.keys = keys;
			return true;
		};

		auto sec = static_cast<const section_header*>(fhdr);
		while (sec->id != lasttext_tag) {
			uints += sec->ints + sizeof(section_header) / sizeof(uint32_t);
			ints -= sec->ints + sizeof(section_header) / sizeof(uint32_t);
			sec = reinterpret_cast<const section_header*>(uints);
			if (sec->ints > ints)
				return false;

			switch (sec->id) {
			case attrtext_tag:
				if (!read_strings((const string_header*)sec, attrs))
					return false;
				break;
			case strstext_tag:
				if (!read_strings((const string_header*)sec, strings))
					return false;
				break;
			case keystext_tag:
				if (!read_strings((const string_header*)sec, attrs))
					return false;
				break;
			}
		}

		return true;
	}

	void lang_file::close() noexcept
	{
		attrs.close();
		strings.close();
		keys.close();
	}

	const char* lang_file::get_string(uint32_t id) const noexcept
	{
		return strings.string(id);
	}

	const char* lang_file::get_string(intmax_t count, uint32_t id) const noexcept
	{
		auto key = strings.get(id);
		auto str = strings.string(key);
		if (!str)
			return str;

		intmax_t sub = calc_substring(count);

		auto cur = str;
		auto length = key->length;
		while (sub--) {
			auto len = std::strlen(cur) + 1;
			if (len >= length)
				return str; // return singular...
			cur += len;
			length -= (uint32_t)len;
		}

		return cur;
	}

	const char* lang_file::get_attr(uint32_t id) const noexcept
	{
		return attrs.string(id);
	}

	const char* lang_file::get_key(uint32_t id) const noexcept
	{
		return keys.string(id);
	}

	uint32_t lang_file::find_key(const char* id) const noexcept
	{
		if (!id)
			return (uint32_t)-1;

		for (auto cur = keys.keys,
			end = cur + keys.count;
			cur != end; ++cur) {
			auto key = keys.string(cur);

			if (!std::strcmp(id, key))
				return cur->id;
		}

		return (uint32_t)-1;
	}

	intmax_t lang_file::calc_substring(intmax_t count) const
	{
		if (!lex) {
			auto entry = attrs.string(ATTR_PLURALS);
			if (entry)
				lex = plurals::decode(entry);
			if (!lex)
				lex = plurals::decode("0");

			if (!lex)
				return 0;
		}

		return lex.eval(count);
	}
}
