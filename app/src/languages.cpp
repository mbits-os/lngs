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
	namespace {
		constexpr std::byte operator""_b(char c) {
			return static_cast<std::byte>(c);
		}

		unsigned char s2uc(char c) { return static_cast<unsigned char>(c); }

		unsigned char b2uc(std::byte b) {
			return static_cast<unsigned char>(b);
		}

		char32_t warped(char32_t c) {
			switch (c) {
				case U'a':
					return U'\u0227';  // 'ȧ'
				case U'b':
					return U'\u018b';  // 'Ƌ'
				case U'c':
					return U'\u00e7';  // 'ç'
				case U'd':
					return U'\u0111';  // 'đ'
				case U'e':
					return U'\u00ea';  // 'ê'
				case U'f':
					return U'\u0192';  // 'ƒ'
				case U'g':
					return U'\u011f';  // 'ğ'
				case U'h':
					return U'\u0125';  // 'ĥ'
				case U'i':
					return U'\u00ef';  // 'ï'
				case U'j':
					return U'\u0135';  // 'ĵ'
				case U'k':
					return U'\u0137';  // 'ķ'
				case U'l':
					return U'\u013a';  // 'ĺ'
				case U'n':
					return U'\u00f1';  // 'ñ'
				case U'o':
					return U'\u00f4';  // 'ô'
				case U'r':
					return U'\u0213';  // 'ȓ'
				case U's':
					return U'\u015f';  // 'ş'
				case U't':
					return U'\u0167';  // 'ŧ'
				case U'u':
					return U'\u0169';  // 'ũ'
				case U'w':
					return U'\u0175';  // 'ŵ'
				case U'y':
					return U'\u00ff';  // 'ÿ'
				case U'z':
					return U'\u0225';  // 'ȥ'
				case U'A':
					return U'\u00c4';  // 'Ä'
				case U'B':
					return U'\u00df';  // 'ß'
				case U'C':
					return U'\u00c7';  // 'Ç'
				case U'D':
					return U'\u00d0';  // 'Ð'
				case U'E':
					return U'\u0204';  // 'Ȅ'
				case U'F':
					return U'\u0191';  // 'Ƒ'
				case U'G':
					return U'\u0120';  // 'Ġ'
				case U'H':
					return U'\u0126';  // 'Ħ'
				case U'I':
					return U'\u00cd';  // 'Í'
				case U'J':
					return U'\u0134';  // 'Ĵ'
				case U'K':
					return U'\u0136';  // 'Ķ'
				case U'L':
					return U'\u023d';  // 'Ƚ'
				case U'N':
					return U'\u00d1';  // 'Ñ'
				case U'O':
					return U'\u00d6';  // 'Ö'
				case U'R':
					return U'\u0154';  // 'Ŕ'
				case U'S':
					return U'\u015e';  // 'Ş'
				case U'T':
					return U'\u023e';  // 'Ⱦ'
				case U'U':
					return U'\u00d9';  // 'Ù'
				case U'W':
					return U'\u0174';  // 'Ŵ'
				case U'Y':
					return U'\u00dd';  // 'Ý'
				case U'Z':
					return U'\u0224';  // 'Ȥ'
				case U'"':
					return U'?';  // '?'
			}
			return c;
		}
	}  // namespace

	std::string warp(const std::string& s) {
		auto u32 = utf::as_u32(s);
		for (auto& c : u32)
			c = warped(c);
		return utf::as_u8(u32);
	}

	std::vector<tr_string> translations(
	    const std::map<std::string, std::string>& gtt,
	    const std::vector<idl_string>& strings,
	    bool warp_missing,
	    bool verbose,
	    source_file& src,
	    diagnostics& diags) {
		std::vector<tr_string> out;
		out.reserve(gtt.empty() ? 0 : gtt.size() - 1);

		for (auto& str : strings) {
			auto it = gtt.find(str.key);
			if (it == gtt.end()) {
				if (verbose) {
					diags.push_back(
					    src.position()[severity::warning]
					    << arg(lng::ERR_MSGS_TRANSLATION_MISSING, str.key));
				}
				if (warp_missing) out.emplace_back(str.id, warp(str.value));
				continue;
			}
			out.emplace_back(str.id, it->second);
		}

		return out;
	}

	std::map<std::string, std::string, std::less<>> attrGTT(
	    std::string_view attrs) {
		std::map<std::string, std::string, std::less<>> out;

		auto c = std::begin(attrs);
		auto e = std::end(attrs);
		while (c != e) {
			while (c != e && std::isspace(s2uc(*c)))
				++c;

			auto s_name = c;
			while (c != e && !std::isspace(s2uc(*c))) {
				if (*c == ':') break;
				++c;
			}
			auto name = std::string{s_name, c};

			while (c != e && std::isspace(s2uc(*c)))
				++c;
			if (c == e || *c != ':') continue;
			++c;

			while (c != e && std::isspace(s2uc(*c)) && *c != '\n')
				++c;

			auto s_value = c;
			while (c != e && *c != '\n')
				++c;
			if (c != e) ++c;
			auto e_value = c;

			while (s_value != e_value && std::isspace(s2uc(e_value[-1])))
				--e_value;
			out[name] = {s_value, e_value};
		}

		return out;
	}

	static std::string gnu2iso(std::string s) {
		size_t i = 0;
		for (auto& c : s) {
			if (c == '_') {
				c = '-';
			} else if (c == '.') {  // ll_CC.code?
				s = s.substr(0, i);
				break;
			}

			++i;
		}

		return s;
	}

	template <attr_t Name>
	static inline void copy_attr(
	    const std::map<std::string, std::string, std::less<>>& src,
	    std::vector<tr_string>& dst,
	    std::string_view key) {
		auto attr = src.find(key);
		if (attr == end(src)) return;
		if constexpr (Name == ATTR_CULTURE)
			dst.push_back({Name, gnu2iso(attr->second)});
		else
			dst.push_back({Name, attr->second});
	}

	std::vector<tr_string> attributes(
	    const std::map<std::string, std::string>& gtt) {
		std::vector<tr_string> props;
		auto it = gtt.find("");
		auto attrs = attrGTT(it == gtt.end() ? std::string{} : it->second);

		copy_attr<ATTR_CULTURE>(attrs, props, "Language");
		copy_attr<ATTR_PLURALS>(attrs, props, "Plural-Forms");

		return props;
	}

	char nextc(instream& is) {
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

		enum result { next, done, error };

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

			while (!is.eof() && std::isspace(b2uc(is.peek())))
				adv(nextc(is));

			if (is.eof()) return done;

			const auto pos = is.position(line, column);

			while (!is.eof() && !std::isspace(b2uc(is.peek()))) {
				const auto c = nextc(is);
				code.push_back(c);
				adv(c);
			}

			const auto end_pos = is.position(line, column);

			while (!is.eof() && std::isspace(b2uc(is.peek()))) {
				auto c = nextc(is);
				if (c == '\n') {
					diags.push_back((pos / end_pos)[severity::error]
					                << arg(lng::ERR_UNANMED_LOCALE, code));
					return error;
				}
			}

			while (!is.eof() && is.peek() != '\n'_b) {
				const auto c = nextc(is);
				name.push_back(c);
				adv(c);
			}

			if (!is.eof()) adv(nextc(is));

			auto len = name.length();
			while (len > 0 && std::isspace(s2uc(name[len - 1])))
				--len;
			if (len != name.length()) name = name.substr(0, len);

			names[std::move(code)] = std::move(name);
			return next;
		}
	};

	bool ll_CC(source_file is,
	           diagnostics& diags,
	           std::map<std::string, std::string>& langs) {
		if (!is.valid()) {
			diags.push_back(is.position()[severity::error]
			                << lng::ERR_FILE_NOT_FOUND);
			return false;
		}

		ll_parse parser{langs, std::move(is), diags};
		ll_parse::result res = ll_parse::done;

		while (!(res = parser.next_code()))
			;

		return res != ll_parse::error;
	}

#define WRITE(os, I) \
	if (!os.write(I)) return -1;
#define WRITESTR(os, S) \
	if (os.write(S) != (S).length()) return -1;
#define CARRY(ex)            \
	do {                     \
		auto ret = (ex);     \
		if (ret) return ret; \
	} while (0)

	namespace {
		void update_offsets(uint32_t& next_offset,
		                    std::vector<tr_string>& block) {
			for (auto& str : block) {
				str.key.offset = next_offset;
				next_offset += str.key.length;
				++next_offset;
			}
		}

		int list(outstream& os, std::vector<tr_string>& block) {
			for (auto& str : block)
				WRITE(os, str.key);

			return 0;
		}

		int data(outstream& os, std::vector<tr_string>& block) {
			for (auto& str : block) {
				WRITESTR(os, str.value);
				WRITE(os, '\0');
			}

			return 0;
		}

		int section(outstream& os,
		            uint32_t section_id,
		            std::vector<tr_string>& block) {
			if (block.empty()) return 0;

			uint32_t key_size = sizeof(string_key) / sizeof(uint32_t);
			key_size *= static_cast<uint32_t>(block.size());

			string_header hdr;
			hdr.id = section_id;
			hdr.string_offset =
			    key_size + sizeof(string_header) / sizeof(uint32_t);
			hdr.ints = static_cast<uint32_t>(
			    hdr.string_offset - sizeof(section_header) / sizeof(uint32_t));
			hdr.string_count = static_cast<uint32_t>(block.size());

			uint32_t offset = 0;
			update_offsets(offset, block);
			uint32_t padding = (((offset + 3) >> 2) << 2) - offset;
			offset += padding;
			offset /= sizeof(uint32_t);
			hdr.ints += offset;

			WRITE(os, hdr);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)
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
	}  // namespace

	int file::write(outstream& os) {
		file_header hdr;
		hdr.id = hdrtext_tag;
		hdr.ints =
		    (sizeof(file_header) - sizeof(section_header)) / sizeof(uint32_t);
		hdr.version = v1_0::version;
		hdr.serial = serial;

		WRITE(os, langtext_tag);
		WRITE(os, hdr);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

		CARRY(section(os, attrtext_tag, attrs));
		CARRY(section(os, strstext_tag, strings));
		CARRY(section(os, keystext_tag, keys));

#ifdef _MSC_VER
#pragma warning(pop)
#endif

		WRITE(os, lasttext_tag);
		WRITE(os, static_cast<uint32_t>(0));

		return 0;
	}

#undef WRITE
#undef WRITESTR
#undef CARRY
}  // namespace lngs::app
