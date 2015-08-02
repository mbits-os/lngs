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
#include <streams.hpp>
#include <strings.hpp>
#include <languages.hpp>
#include <gettext.hpp>

namespace make {
	int call(args::parser& parser)
	{
		fs::path moname, inname, llname, outname;
		bool verbose = false;
		bool warp_missing = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.set<std::true_type>(warp_missing, "w", "warp").help("replace missing strings with warped ones; resulting strings are always singular").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("LNG binary file to write; if - is used, result is written to standard output");
		parser.arg(moname, "m", "msgs").meta("MOFILE").help("GetText message file to read");
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.arg(llname, "l", "lang").meta("FILE").help("ATTR_LANGUAGE file with ll_CC names list").opt();
		parser.parse();

		locale::Strings strings;
		if (!locale::read_strings(inname, strings, verbose))
			return -1;

		locale::file file;
		file.serial = strings.serial;
		auto map = gtt::open(moname);
		file.strings = locale::translations(map, strings.strings, warp_missing, verbose);
		file.attrs = locale::attributes(map);

		if (!llname.empty()) {
			auto prop = std::find_if(std::begin(file.attrs), std::end(file.attrs), [](auto& item) { return item.key.id == locale::ATTR_CULTURE; });
			if (prop == std::end(file.attrs) || prop->value.empty()) {
				printf("warning: message file does not contain Language attribute.\n");
			} else {
				std::map<std::string, std::string> names;
				if (!locale::ll_CC(llname, names))
					return -1;

				auto it = names.find(prop->value);
				if (it == names.end())
					printf("warning: no %s in %s\n", prop->value.c_str(), llname.string().c_str());
				else {
					file.attrs.emplace_back(locale::ATTR_LANGUAGE, it->second);
					std::sort(std::begin(file.attrs), std::end(file.attrs), [](auto& lhs, auto& rhs) { return lhs.key.id < rhs.key.id; });
				}
			}
		}

		std::unique_ptr<FILE, decltype(&fclose)> outf{ nullptr, fclose };
		FILE* output = stdout;
		if (outname != "-") {
			outf.reset(fs::fopen(outname, "wb"));
			if (!outf) {
				fprintf(stderr, "could not open `%s'", outname.native().c_str());
				return -1;
			}

			output = outf.get();
		}

		locale::foutstream os{ output };
		return file.write(os);
	}
}
