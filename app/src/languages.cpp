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

#include <assert.h>
#include <cctype>

#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>
#include <lngs/internals/utf8.hpp>

namespace lngs::app {

	std::string warp(const std::string& s)
	{
		auto u32 = utf::as_u32(s);
		for (auto& c : u32) {
			switch (c) {
			case U'a': c = U'\u0227'; break; // 'ȧ'
			case U'b': c = U'\u018b'; break; // 'Ƌ'
			case U'c': c = U'\u00e7'; break; // 'ç'
			case U'd': c = U'\u0111'; break; // 'đ'
			case U'e': c = U'\u00ea'; break; // 'ê'
			case U'f': c = U'\u0192'; break; // 'ƒ'
			case U'g': c = U'\u011f'; break; // 'ğ'
			case U'h': c = U'\u0125'; break; // 'ĥ'
			case U'i': c = U'\u00ef'; break; // 'ï'
			case U'j': c = U'\u0135'; break; // 'ĵ'
			case U'k': c = U'\u0137'; break; // 'ķ'
			case U'l': c = U'\u013a'; break; // 'ĺ'
			case U'n': c = U'\u00f1'; break; // 'ñ'
			case U'o': c = U'\u00f4'; break; // 'ô'
			case U'r': c = U'\u0213'; break; // 'ȓ'
			case U's': c = U'\u015f'; break; // 'ş'
			case U't': c = U'\u0167'; break; // 'ŧ'
			case U'u': c = U'\u0169'; break; // 'ũ'
			case U'w': c = U'\u0175'; break; // 'ŵ'
			case U'y': c = U'\u00ff'; break; // 'ÿ'
			case U'z': c = U'\u0225'; break; // 'ȥ'
			case U'A': c = U'\u00c4'; break; // 'Ä'
			case U'B': c = U'\u00df'; break; // 'ß'
			case U'C': c = U'\u00c7'; break; // 'Ç'
			case U'D': c = U'\u00d0'; break; // 'Ð'
			case U'E': c = U'\u0204'; break; // 'Ȅ'
			case U'F': c = U'\u0191'; break; // 'Ƒ'
			case U'G': c = U'\u0120'; break; // 'Ġ'
			case U'H': c = U'\u0126'; break; // 'Ħ'
			case U'I': c = U'\u00cd'; break; // 'Í'
			case U'J': c = U'\u0134'; break; // 'Ĵ'
			case U'K': c = U'\u0136'; break; // 'Ķ'
			case U'L': c = U'\u023d'; break; // 'Ƚ'
			case U'N': c = U'\u00d1'; break; // 'Ñ'
			case U'O': c = U'\u00d6'; break; // 'Ö'
			case U'R': c = U'\u0154'; break; // 'Ŕ'
			case U'S': c = U'\u015e'; break; // 'Ş'
			case U'T': c = U'\u023e'; break; // 'Ⱦ'
			case U'U': c = U'\u00d9'; break; // 'Ù'
			case U'W': c = U'\u0174'; break; // 'Ŵ'
			case U'Y': c = U'\u00dd'; break; // 'Ý'
			case U'Z': c = U'\u0224'; break; // 'Ȥ'
			case U'"': c = U'?';   break; // '?'
			};
		}
		return utf::as_u8(u32);
	}

	std::vector<tr_string> translations(const std::map<std::string, std::string>& gtt,
	                                    const std::vector<idl_string>& strings,
	                                    bool warp_missing, bool verbose,
	                                    source_file& src, diagnostics& diags)
	{
		std::vector<tr_string> out;
		out.reserve(gtt.empty() ? 0 : gtt.size() - 1);

		for (auto& str : strings) {
			auto it = gtt.find(str.key);
			if (it == gtt.end()) {
				if (verbose) {
					diags.push_back(
						src.position()[severity::warning]
							<< arg(lng::ERR_MSGS_TRANSLATION_MISSING, str.key)
					);
				}
				if (warp_missing)
					out.emplace_back(str.id, warp(str.value));
				continue;
			}
			out.emplace_back(str.id, it->second);
		}

		return out;
	}

	std::map<std::string, std::string, std::less<>> attrGTT(std::string_view attrs)
	{
		std::map<std::string, std::string, std::less<>> out;

		auto c = std::begin(attrs);
		auto e = std::end(attrs);
		while (c != e) {
			while (c != e && std::isspace((uint8_t)*c)) ++c;

			auto s_name = c;
			while (c != e && !std::isspace((uint8_t)*c)) {
				if (*c == ':')
					break;
				++c;
			}
			auto name = std::string{ s_name, c };

			while (c != e && std::isspace((uint8_t)*c)) ++c;
			if (c == e || *c != ':')
				continue;
			++c;

			while (c != e && std::isspace((uint8_t)*c) && *c != '\n') ++c;

			auto s_value = c;
			while (c != e && *c != '\n') ++c;
			if (c != e) ++c;
			auto e_value = c;

			while (s_value != e_value && std::isspace((uint8_t)e_value[-1])) --e_value;
			out[name] = { s_value, e_value };
		}

		return out;
	}

	static std::string gnu2iso(std::string s)
	{
		size_t i = 0;
		for (auto& c : s) {
			if (c == '_') {
				c = '-';
			} else if (c == '.') { // ll_CC.code?
				s = s.substr(0, i);
				break;
			}

			++i;
		}

		return s;
	}

	template <attr_t Name>
	static inline void copy_attr(const std::map<std::string, std::string, std::less<>>& src,
		std::vector<tr_string>& dst, std::string_view key) {
		auto attr = src.find(key);
		if (attr == end(src))
			return;
		if constexpr(Name == ATTR_CULTURE)
			dst.push_back({ Name, gnu2iso(attr->second) });
		else
			dst.push_back({ Name, attr->second });
	}

	std::vector<tr_string> attributes(const std::map<std::string, std::string>& gtt)
	{
		std::vector<tr_string> props;
		auto it = gtt.find("");
		auto attrs = attrGTT(it == gtt.end() ? std::string{ } : it->second);

		copy_attr<ATTR_CULTURE>(attrs, props, "Language");
		copy_attr<ATTR_PLURALS>(attrs, props, "Plural-Forms");

		return props;
	}

	char nextc(instream& is)
	{
		char c;
		is.read(&c, 1);
		return c;
	}

	struct ll_parse {
		std::map<std::string, std::string>& names;
		source_file is;
		diagnostics& diags;

		unsigned line = 1;
		unsigned column = 1;

		enum result {
			next,
			done,
			error
		};

		void adv(const char c) {
			++column;
			if (c == '\n') {
				++line;
				column = 1;
			}
		}

		result next_code() {
			std::string code;
			std::string name;

			while (!is.eof() && std::isspace((uint8_t)is.peek()))
				adv(nextc(is));

			if (is.eof())
				return done;

			const auto pos = is.position(line, column);

			while (!is.eof() && !std::isspace((uint8_t)is.peek())) {
				const auto c = nextc(is);
				code.push_back(c);
				adv(c);
			}

			const auto end_pos = is.position(line, column);

			while (!is.eof() && std::isspace((uint8_t)is.peek())) {
				auto c = nextc(is);
				if (c == '\n') {
					diags.push_back(
						(pos/end_pos)[severity::error]
							<< arg(lng::ERR_UNANMED_LOCALE, code)
					);
					return error;
				}
			}

			while (!is.eof() && is.peek() != (std::byte)'\n') {
				const auto c = nextc(is);
				name.push_back(c);
				adv(c);
			}

			if (!is.eof())
				adv(nextc(is));

			auto len = name.length();
			while (len > 0 && std::isspace((uint8_t)name[len - 1]))
				--len;
			if (len != name.length())
				name = name.substr(0, len);

			names[std::move(code)] = std::move(name);
			return next;
		}
	};

	bool ll_CC(source_file is, diagnostics& diags, std::map<std::string, std::string>& langs)
	{
		if (!is.valid()) {
			diags.push_back(is.position()[severity::error] << lng::ERR_FILE_NOT_FOUND);
			return false;
		}

		ll_parse parser{ langs, std::move(is), diags };
		ll_parse::result res = ll_parse::done;

		while (!(res = parser.next_code()));

		return res != ll_parse::error;
	}

#define WRITE(os, I) if (!os.write(I)) return -1;
#define WRITESTR(os, S) if (os.write(S) != (S).length()) return -1;
#define CARRY(ex) do { auto ret = (ex); if (ret) return ret; } while (0)

	namespace {
		void update_offsets(uint32_t& next_offset, std::vector<tr_string>& block)
		{
			for (auto& str : block) {
				str.key.offset = next_offset;
				next_offset += str.key.length;
				++next_offset;
			}
		}

		int list(outstream& os, std::vector<tr_string>& block)
		{
			for (auto& str : block)
				WRITE(os, str.key);

			return 0;
		}

		int data(outstream& os, std::vector<tr_string>& block)
		{
			for (auto& str : block) {
				WRITESTR(os, str.value);
				WRITE(os, '\0');
			}

			return 0;
		}

		int section(outstream& os, uint32_t section_id, std::vector<tr_string>& block)
		{
			if (block.empty())
				return 0;

			uint32_t key_size = sizeof(string_key) / sizeof(uint32_t);
			key_size *= (uint32_t)block.size();

			string_header hdr;
			hdr.id = section_id;
			hdr.string_offset = key_size + sizeof(string_header) / sizeof(uint32_t);
			hdr.ints = hdr.string_offset - sizeof(section_header) / sizeof(uint32_t);
			hdr.string_count = (uint32_t)block.size();

			uint32_t offset = 0;
			update_offsets(offset, block);
			uint32_t padding = (((offset + 3) >> 2) << 2) - offset;
			offset += padding;
			offset /= sizeof(uint32_t);
			hdr.ints += offset;

			WRITE(os, hdr);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#endif

			CARRY(list(os, block));
			CARRY(data(os, block));

#ifdef _MSC_VER
#pragma warning(pop)
#endif

			while (padding--) {
				WRITE(os, '\0');
			}

			return 0;
		}
	}

	tr_string::tr_string() = default;
	tr_string::~tr_string() = default;
	tr_string::tr_string(tr_string&&) = default;
	tr_string& tr_string::operator=(tr_string&&) = default;
	tr_string::tr_string(const tr_string&) = default;
	tr_string& tr_string::operator=(const tr_string&) = default;

	int file::write(outstream& os)
	{
		file_header hdr;
		hdr.id = hdrtext_tag;
		hdr.ints = (sizeof(file_header) - sizeof(section_header)) / sizeof(uint32_t);
		hdr.version = v1_0::version;
		hdr.serial = serial;

		WRITE(os, langtext_tag);
		WRITE(os, hdr);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#endif

		CARRY(section(os, attrtext_tag, attrs));
		CARRY(section(os, strstext_tag, strings));
		CARRY(section(os, keystext_tag, keys));

#ifdef _MSC_VER
#pragma warning(pop)
#endif

		WRITE(os, lasttext_tag);
		WRITE(os, (uint32_t)0);

		return 0;
	}

#undef WRITE
#undef WRITESTR
#undef CARRY
}
