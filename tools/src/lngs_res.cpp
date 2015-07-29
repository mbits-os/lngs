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

#include <filesystem.hpp>
#include <argparser.hpp>
#include <strings.hpp>
#include <streams.hpp>
#include <languages.hpp>

namespace res {
	class table_outstream : public locale::outstream {
		std::FILE* ptr;
		char header[9 * sizeof(uint32_t)];
		size_t attrs, strings, keys, header_size, data_offset;
		size_t offset = 0;
		bool new_string = true;
		bool seen_strings = false;
		bool header_printed = false;
	public:
		table_outstream(std::FILE* ptr, size_t attrs, size_t strings, size_t keys)
			: ptr(ptr)
			, attrs(attrs)
			, strings(strings)
			, keys(keys)
			, header_size(3)
			, data_offset(sizeof(locale::string_key)* (attrs + strings + keys) + sizeof(uint32_t) * 3)
		{
			if (attrs) {
				data_offset += sizeof(uint32_t) * 3;
				header_size += 2;
			}
			if (strings) {
				data_offset += sizeof(uint32_t) * 3;
				header_size += 2;
			}
			if (keys) {
				data_offset += sizeof(uint32_t) * 3;
				header_size += 2;
			}
			header_size *= sizeof(uint32_t);
		}
		std::size_t write(const void* data, std::size_t length) override;
	};

	std::size_t table_outstream::write(const void* data, std::size_t length)
	{
		auto chars = reinterpret_cast<const char*>(data);
		auto save = offset;

		while (offset < header_size && length) {
			header[offset] = *chars++;
			++offset;
			--length;
		}

		if (offset == header_size  && !header_printed) {
			header_printed = true;

			fprintf(ptr, "            // HEADER\n            \"");
			for (size_t i = 0; i < sizeof(uint32_t); ++i)
				putc(header[i], ptr);

			const char* sep[] = { "\"\n            \"", "\" \"" };
			int sep_ndx = 0;
			auto uints = header_size / sizeof(uint32_t);

			for (size_t uint = 1; uint < uints; ++uint) {
				fprintf(ptr, "%s", sep[sep_ndx]);
				sep_ndx += 1;
				sep_ndx %= 2;

				for (size_t i = 0; i < sizeof(uint32_t); ++i)
					fprintf(ptr, "\\x%02x", (uint8_t)header[i + uint * sizeof(uint32_t)]);
			}

			fprintf(ptr, "\"\n\n            // BLOCKS\n");
		}

		if (offset < data_offset) {
			for (size_t i = 0; i < length; ++i, ++offset, ++chars) {
				if (((offset - header_size) % 12) == 0)
					fprintf(ptr, "            \"");
				fprintf(ptr, "\\x%02x", (uint8_t)*chars);
				if (((offset + 1 - header_size) % 12) == 0)
					fprintf(ptr, "\"\n");
			}
		} else {
			if (!seen_strings) {
				if (((offset - 4) % 12) != 0)
					fprintf(ptr, "\"\n");

				seen_strings = true;
				fprintf(ptr, "\n            // STRINGS\n");
				new_string = true;
			}

			for (size_t i = 0; i < length; ++i, ++chars) {
				if (new_string) {
					fprintf(ptr, "            \"");
					new_string = false;
				}

				if (*chars)
					putc((uint8_t)*chars, ptr);
				else {
					fprintf(ptr, "\\0\"\n");
					new_string = true;
				}
			}

			offset += length;
		}

		return offset - save;
	}

	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;
		bool warp_strings = false;
		bool with_keys = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.set<std::true_type>(warp_strings, "w", "warp").help("replace all strings with warped ones; plural strings will still be plural (as if English)").opt();
		parser.set<std::true_type>(with_keys, "k", "keys").help("add string block with string keys").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("LNG binary file to write; if - is used, result is written to standard output");
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.parse();

		locale::Strings strings;
		if (!locale::read_strings(inname, strings, verbose))
			return -1;

		locale::file file;
		for (auto& string : strings.strings) {
			auto value = string.value;
			if (warp_strings)
				value = locale::warp(value);

			if (!string.plural.empty()) {
				value.push_back(0);
				if (warp_strings)
					value.append(locale::warp(string.plural));
				else
					value.append(string.plural);
			}

			if (with_keys)
				file.keys.emplace_back(string.id, string.key);
			file.strings.emplace_back(string.id, value);
		}

		std::unique_ptr<FILE, decltype(&fclose)> outf{ nullptr, fclose };
		FILE* output = stdout;
		if (outname != "-") {
			outf.reset(fs::fopen(outname, "w"));
			if (!outf) {
				fprintf(stderr, "could not open `%s'", outname.native().c_str());
				return -1;
			}

			output = outf.get();
		}

		fprintf(output, R"(// THIS FILE IS AUTOGENERATED
#include "%s.hpp"

inline namespace %s {
    namespace {
        const char __resource[] = {
)", strings.project.c_str(), strings.project.c_str());

		{
			table_outstream os{ output, file.attrs.size(), file.strings.size(), file.keys.size() };
			//locale::memoutstream os;
			auto ret = file.write(os);
			if (ret)
				return ret;
		}

		fprintf(output, R"(        }; // __resource
    } // namespace

    /*static*/ const char* Resource::data() { return __resource; }
    /*static*/ std::size_t Resource::size() { return sizeof(__resource); }
} // namespace %s
)", strings.project.c_str());
		return 0;
	}
}
