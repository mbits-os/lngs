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

#include "pch.h"

#include <assert.h>
#include <cctype>

#include <languages.hpp>
#include <streams.hpp>
#include <strings.hpp>

namespace locale {

	std::string warp(const std::string& s)
	{
		auto w = utf::widen(s);
		for (auto& c : w) {
			switch (c) {
			case L'a': c = 0x0227; break; // 'ȧ'
			case L'b': c = 0x018b; break; // 'Ƌ'
			case L'c': c = 0x00e7; break; // 'ç'
			case L'd': c = 0x0111; break; // 'đ'
			case L'e': c = 0x00ea; break; // 'ê'
			case L'f': c = 0x0192; break; // 'ƒ'
			case L'g': c = 0x011f; break; // 'ğ'
			case L'h': c = 0x0125; break; // 'ĥ'
			case L'i': c = 0x00ef; break; // 'ï'
			case L'j': c = 0x0135; break; // 'ĵ'
			case L'k': c = 0x0137; break; // 'ķ'
			case L'l': c = 0x013a; break; // 'ĺ'
			case L'n': c = 0x00f1; break; // 'ñ'
			case L'o': c = 0x00f4; break; // 'ô'
			case L'r': c = 0x0213; break; // 'ȓ'
			case L's': c = 0x015f; break; // 'ş'
			case L't': c = 0x0167; break; // 'ŧ'
			case L'u': c = 0x0169; break; // 'ũ'
			case L'w': c = 0x0175; break; // 'ŵ'
			case L'y': c = 0x00ff; break; // 'ÿ'
			case L'z': c = 0x0225; break; // 'ȥ'
			case L'A': c = 0x00c4; break; // 'Ä'
			case L'B': c = 0x00df; break; // 'ß'
			case L'C': c = 0x00c7; break; // 'Ç'
			case L'D': c = 0x00d0; break; // 'Ð'
			case L'E': c = 0x0204; break; // 'Ȅ'
			case L'F': c = 0x0191; break; // 'Ƒ'
			case L'G': c = 0x0120; break; // 'Ġ'
			case L'H': c = 0x0126; break; // 'Ħ'
			case L'I': c = 0x00cd; break; // 'Í'
			case L'J': c = 0x0134; break; // 'Ĵ'
			case L'K': c = 0x0136; break; // 'Ķ'
			case L'L': c = 0x023d; break; // 'Ƚ'
			case L'N': c = 0x00d1; break; // 'Ñ'
			case L'O': c = 0x00d6; break; // 'Ö'
			case L'R': c = 0x0154; break; // 'Ŕ'
			case L'S': c = 0x015e; break; // 'Ş'
			case L'T': c = 0x023e; break; // 'Ⱦ'
			case L'U': c = 0x00d9; break; // 'Ù'
			case L'W': c = 0x0174; break; // 'Ŵ'
			case L'Y': c = 0x00dd; break; // 'Ý'
			case L'Z': c = 0x0224; break; // 'Ȥ'
			case L'"': c = L'?';   break; // '?'
			};
		}
		return utf::narrowed(w);
	}

	std::vector<string> translations(const std::map<std::string, std::string>& gtt, const std::vector<locale::String>& strings, bool warp_missing, bool verbose)
	{
		std::vector<string> out;
		out.reserve(gtt.empty() ? 0 : gtt.size() - 1);

		for (auto& str : strings) {
			auto it = gtt.find(str.key);
			if (it == gtt.end()) {
				if (verbose)
					printf("warning: translation for `%s' missing.\n", str.key.c_str());
				if (warp_missing)
					out.emplace_back(str.id, warp(str.value));
				continue;
			}
			out.emplace_back(str.id, it->second);
		}

		return std::move(out);
	}

	std::map<std::string, std::string> attrGTT(const std::string& attrs)
	{
		std::map<std::string, std::string> out;

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

	std::vector<string> attributes(const std::map<std::string, std::string>& gtt)
	{
		std::vector<string> props;
		auto it = gtt.find("");
		auto attrs = attrGTT(it == gtt.end() ? std::string{ } : it->second);

		props.push_back({ ATTR_CULTURE, gnu2iso(attrs["Language"]) });
		auto attr = attrs.find("Plural-Forms");
		if (attr != attrs.end())
			props.push_back({ ATTR_PLURALS, attr->second });
		// TODO: ATTR_LANGUAGE

		return std::move(props);
	}

	char nextc(locale::instream& is)
	{
		char c;
		is.read(&c, 1);
		return c;
	}

	bool ll_code(locale::instream& is, std::string& code, std::string& name, const std::string& fname, bool ret)
	{
		code.clear();
		name.clear();

		while (!is.eof() && std::isspace((uint8_t)is.peek()))
			nextc(is);

		if (is.eof())
			return false;

		while (!is.eof() && !std::isspace((uint8_t)is.peek()))
			code.push_back(nextc(is));

		while (!is.eof() && std::isspace((uint8_t)is.peek())) {
			auto c = nextc(is);
			if (c == '\n') {
				fprintf(stderr, "error: `%s' does not contain name in %s\n", code.c_str(), fname.c_str());
				ret = false;
				return false;
			}
		}

		while (!is.eof() && is.peek() != '\n')
			name.push_back(nextc(is));

		if (!is.eof())
			nextc(is);

		auto len = name.length();
		while (len > 0 && std::isspace((uint8_t)name[len - 1]))
			--len;
		if (len != name.length())
			name = name.substr(0, len);

		return true;
	}

	bool ll_CC(const fs::path& in, std::map<std::string, std::string>& langs)
	{
		auto inname = in;
		inname.make_preferred();

		std::unique_ptr<FILE, decltype(&fclose)> inf{ fs::fopen(in, "rb"), fclose };
		if (!inf) {
			fprintf(stderr, "could not open `%s'", inname.string().c_str());
			return false;
		}

		locale::finstream is{ inf.get() };
		std::string code, name;
		bool ret = true;
		while (ll_code(is, code, name, inname.string(), ret) && ret)
			langs[code] = name;

		return ret;
	}

#define WRITE(os, I) if (!os.write(I)) return -1;
#define WRITESTR(os, S) if (os.write(S) != (S).length()) return -1;
#define CARRY(ex) do { auto ret = (ex); if (ret) return ret; } while (0)

	namespace {
		int header(locale::outstream& os, uint32_t& next_offset, std::vector<string>& block)
		{
			if (block.empty())
				return 0;

			WRITE(os, (uint32_t)block.size());
			WRITE(os, next_offset);
			next_offset += (uint32_t)(sizeof(string_key) * block.size());
			return 0;
		}

		void update_offsets(uint32_t& next_offset, std::vector<string>& block)
		{
			for (auto& str : block) {
				str.key.offset = next_offset;
				next_offset += str.key.length;
				++next_offset;
			}
		}

		int list(locale::outstream& os, std::vector<string>& block)
		{
			if (block.empty())
				return 0;

			for (auto& str : block)
				WRITE(os, str.key);

			return 0;
		}

		int data(locale::outstream& os, std::vector<string>& block)
		{
			if (block.empty())
				return 0;

			for (auto& str : block) {
				WRITESTR(os, str.value);
				WRITE(os, '\0');
			}

			return 0;
		}

		int section(locale::outstream& os, uint32_t section_id, std::vector<string>& block)
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

	int file::write(locale::outstream& os)
	{
		constexpr uint32_t header_size = 3 * sizeof(uint32_t);

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
		WRITE(os, (size_t)0);

		return 0;
	}

#undef WRITE
#undef WRITESTR
#undef CARRY
}
