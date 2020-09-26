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

#include <cstring>
#include <limits>
#include <lngs/lngs_file.hpp>

namespace lngs {
	const string_key* lang_file::section::get(identifier id) const noexcept {
		const auto comp = static_cast<uint32_t>(id);
		auto end = keys + count;
		auto cur = keys;
		while (end != cur) {
			if (cur->id == comp) return cur;
			++cur;
		}
		return nullptr;
	}

	std::string_view lang_file::section::string(identifier id) const noexcept {
		auto key = get(id);
		if (!key) return {};

		return {key->offset + strings, key->length};
	}

	std::string_view lang_file::section::string(
	    const string_key& key) const noexcept {
		return {key.offset + strings, key.length};
	}

	lang_file::lang_file() noexcept {}

	bool lang_file::section::read_strings(const string_header* sec) noexcept {
		close();

		if (sec->string_offset - sizeof(section_header) / sizeof(uint32_t) >
		    sec->ints)
			return false;

		auto uints_for_keys =
		    sec->string_offset - sizeof(string_header) / sizeof(uint32_t);
		if (uints_for_keys <
		    sec->string_count * sizeof(string_key) / sizeof(uint32_t))
			return false;

		const auto space_for_strings =
		    (sec->ints + sizeof(section_header) / sizeof(uint32_t) -
		     sec->string_offset) *
		    sizeof(uint32_t);

		auto in_keys = reinterpret_cast<const string_key*>(sec + 1);
		auto in_strings = reinterpret_cast<const char*>(
		    reinterpret_cast<const uint32_t*>(sec) + sec->string_offset);
		for (auto cur = in_keys, end = in_keys + sec->string_count; cur != end;
		     ++cur) {
			auto& key = *cur;
			if (key.offset > space_for_strings) return false;
			if (key.offset + key.length > space_for_strings) return false;
			if (in_strings[key.offset + key.length] != 0) return false;
		}

		count = sec->string_count;
		strings = in_strings;
		keys = in_keys;
		return true;
	}

	bool lang_file::open(const memory_view& view) noexcept {
		constexpr uint32_t header_size = sizeof(uint32_t) + sizeof(file_header);
		constexpr uint32_t ver_1_x = 0x0000FFFFu;

		if (!view.contents || view.size < header_size) return false;

		auto ints = view.size / sizeof(uint32_t);
		auto uints = reinterpret_cast<const uint32_t*>(view.contents);

		auto file_tag = *uints++;
		--ints;
		auto fhdr = reinterpret_cast<const file_header*>(uints);
		if (file_tag != langtext_tag || fhdr->id != hdrtext_tag) return false;
		if ((fhdr->ints + 2) < (sizeof(file_header) / sizeof(uint32_t)))
			return false;

		if (fhdr->version != v1_0::version) {                // == 1.0?
			if ((fhdr->version & ver_1_x) != v1_0::version)  // == 1.x?
				return false;
		}

		serial = fhdr->serial;

		auto sec = static_cast<section_header const*>(fhdr);
		while (sec->id != lasttext_tag) {
			const auto sec_ints =
			    sec->ints + sizeof(section_header) / sizeof(uint32_t);
			if (sec_ints >= ints) return false;
			uints += sec_ints;
			ints -= sec_ints;
			sec = reinterpret_cast<section_header const*>(uints);
			auto strsec = static_cast<string_header const*>(sec);

			switch (sec->id) {
				case attrtext_tag:
					if (!attrs.read_strings(strsec)) return false;
					break;
				case strstext_tag:
					if (!strings.read_strings(strsec)) return false;
					break;
				case keystext_tag:
					if (!keys.read_strings(strsec)) return false;
					break;
			}
		}

		return true;
	}

	void lang_file::close() noexcept {
		attrs.close();
		strings.close();
		keys.close();
	}

	unsigned lang_file::get_serial() const noexcept { return serial; }

	std::string_view lang_file::get_string(identifier id) const noexcept {
		const auto ret = strings.string(id);
		return ret.substr(0, ret.find('\x00', 0));
	}

	std::string_view lang_file::get_string(identifier id,
	                                       quantity count) const noexcept {
		const auto str = strings.string(id);
		if (str.empty()) return str;

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
			length -= static_cast<uint32_t>(pos);
		}

		return cur.substr(0, cur.find('\x00', 0));
	}

	std::string_view lang_file::get_attr(uint32_t id) const noexcept {
		return attrs.string(static_cast<identifier>(id));
	}

	std::string_view lang_file::get_key(uint32_t id) const noexcept {
		return keys.string(static_cast<identifier>(id));
	}

	uint32_t lang_file::find_key(std::string_view id) const noexcept {
		if (id.empty()) return std::numeric_limits<uint32_t>::max();

		for (auto const& cur : keys) {
			auto key = keys.string(cur);

			if (id == key) return cur.id;
		}

		return std::numeric_limits<uint32_t>::max();
	}

	intmax_t lang_file::calc_substring(quantity count) const {
		if (!lex) {
			auto entry = attrs.string(static_cast<identifier>(ATTR_PLURALS));
			if (!entry.empty()) lex = plurals::decode(entry);
			if (!lex) lex = plurals::decode("nplurals=1;plural=0");

			if (!lex) return 0;
		}

		return lex.eval(static_cast<intmax_t>(count));
	}
}  // namespace lngs
