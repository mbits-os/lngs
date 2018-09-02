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
	const string_key* lang_file::section::get(identifier id) const noexcept
	{
		const auto comp = (uint32_t)id;
		auto end = keys + count;
		auto cur = keys;
		while (end != cur) {
			if (cur->id == comp)
				return cur;
			++cur;
		}
		return nullptr;
	}

	std::string_view lang_file::section::string(identifier id) const noexcept
	{
		auto key = get(id);
		if (!key)
			return {};

		return { key->offset + strings, key->length };
	}

	std::string_view lang_file::section::string(const string_key* key) const noexcept
	{
		if (!key)
			return nullptr;
		return { key->offset + strings, key->length };
	}

	std::string_view lang_file::section::string(const string_key& key) const noexcept
	{
		return { key.offset + strings, key.length };
	}

	lang_file::lang_file() noexcept
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

	bool lang_file::open(const memory_view& view) noexcept
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

		auto read_strings = [](const string_header* sec, section& out) noexcept -> bool
		{
			out.close();

			if (sec->string_offset - sizeof(section_header) / sizeof(uint32_t) > sec->ints)
				return false;

			auto uints_for_keys = sec->string_offset - sizeof(string_header) / sizeof(uint32_t);
			if (uints_for_keys < sec->string_count * sizeof(string_key) / sizeof(uint32_t))
				return false;

			const auto space_for_strings = (sec->ints + sizeof(section_header) / sizeof(uint32_t) - sec->string_offset) * sizeof(uint32_t);

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

	std::string_view lang_file::get_string(identifier id) const noexcept
	{
		const auto ret = strings.string(id);
		return ret.substr(0, ret.find('\x00', 0));
	}

	std::string_view lang_file::get_string(identifier id, quantity count) const noexcept
	{
		auto key = strings.get(id);
		auto str = strings.string(key);
		if (str.empty())
			return str;

		intmax_t sub = calc_substring(count);

		auto cur = str;
		auto length = str.length();

		while (sub--) {
			auto pos = cur.find('\x00', 0);
			if (pos == std::string_view::npos) {
				// return singular...
				return str.substr(0, str.find('\x00', 0));
			}

			++pos;
			cur = cur.substr(pos);
			length -= (uint32_t)(pos);
		}

		return cur.substr(0, cur.find('\x00', 0));
	}

	std::string_view lang_file::get_attr(uint32_t id) const noexcept
	{
		return attrs.string((identifier)id);
	}

	std::string_view lang_file::get_key(uint32_t id) const noexcept
	{
		return keys.string((identifier)id);
	}

	uint32_t lang_file::find_key(std::string_view id) const noexcept
	{
		if (id.empty())
			return (uint32_t)-1;

		for (auto const& cur : keys) {
			auto key = keys.string(cur);

			if (id == key)
				return cur.id;
		}

		return (uint32_t)-1;
	}

	intmax_t lang_file::calc_substring(quantity count) const
	{
		if (!lex) {
			auto entry = attrs.string((identifier)ATTR_PLURALS);
			if (!entry.empty())
				lex = plurals::decode(entry);
			if (!lex)
				lex = plurals::decode("0");

			if (!lex)
				return 0;
		}

		return lex.eval((intmax_t) count);
	}
}
