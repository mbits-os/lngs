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

#include <ctime>
#include <iterator>

#include <filesystem.hpp>
#include <argparser.hpp>
#include <strings.hpp>

namespace pot {
	struct info {
		std::string copy{ "THE PACKAGE'S COPYRIGHT HOLDER" };
		std::string first_author{ "FIRST AUTHOR <EMAIL@ADDRESS>" };
		std::string lang_team{ "LANGUAGE <LL@li.org>" };
	};

	void write(FILE* out, const locale::Strings& defs, const info& nfo);

	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;
		info nfo;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("set POT file to write; if - is used, result is written to standard output");
		parser.arg(inname, "i", "in").meta("FILE").help("set message file to read");
		parser.arg(nfo.copy, "c", "copy").meta("HOLDER").help("the name of copyright holder").opt();
		parser.arg(nfo.first_author, "a", "author").meta("EMAIL").help("the name and address of first author");
		parser.arg(nfo.lang_team, "l", "langteam").meta("EMAIL").help("the name and address of language team").opt();
		parser.parse();

		locale::Strings strings;
		if (!locale::read_strings(inname, strings, verbose))
			return -1;

		if (outname == "-") {
			write(stdout, strings, nfo);
			return 0;
		}

		std::unique_ptr<FILE, decltype(&fclose)> outf{ fs::fopen(outname, "w"), fclose };

		if (!outf) {
			fprintf(stderr, "could not open `%s'", outname.native().c_str());
			return -1;
		}

		write(outf.get(), strings, nfo);
		return 0;
	}

	auto now_recalc() {
		struct tm tm;
		time_t now;
		std::time(&now);
		localtime_s(&tm, &now);
		return tm;
	}

	const auto& now() {
		static const auto tm = now_recalc();
		return tm;
	}

	auto thisYear() {
		static const auto year = 1900 + now().tm_year;
		return year;
	}

	std::string creationDate() {
		char buffer[100];
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M%z", &now());
		// 2012-10-22 20:47+0200
		return buffer;
	}

	std::string escape(const std::string& in) {
		std::string out;
		out.reserve(in.length() * 11 / 10);
		for (auto c : in) {
			switch (c) {
			case '\\': out.append("\\\\"); break;
			case '\a': out.append("\\a"); break;
			case '\b': out.append("\\b"); break;
			case '\f': out.append("\\f"); break;
			case '\n': out.append("\\n"); break;
			case '\r': out.append("\\r"); break;
			case '\t': out.append("\\t"); break;
			case '\v': out.append("\\v"); break;
			case '\"': out.append("\\\""); break;
			default:
				out.push_back(c);
			}
		}
		return out;
	}

	void write(FILE* out, const locale::Strings& defs, const info& nfo)
	{
		auto has_plurals = std::find_if(std::begin(defs.strings), std::end(defs.strings), [](auto& str) { return !str.plural.empty(); }) != std::end(defs.strings);

		fprintf(out, R"(# Copyright (C) %d %s
# This file is distributed under the same license as the %s package.
# %s, %d.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: %s %s\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: %s\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: %s\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
)",

thisYear(), nfo.copy.c_str(),
defs.project.c_str(),
nfo.first_author.c_str(), thisYear(),
defs.project.c_str(), defs.version.c_str(),
creationDate().c_str(),
nfo.lang_team.c_str());

		if (has_plurals)
			fprintf(out, R"("Plural-Forms: nplurals=2; plural=(n != 1);\n"
)");

		std::vector<std::string> ids;
		ids.reserve(defs.strings.size());
		std::transform(std::begin(defs.strings), std::end(defs.strings), std::back_inserter(ids), [](auto& str) {return str.key; });
		std::sort(std::begin(ids), std::end(ids));

		for (auto& id : ids) {
			auto it = std::find_if(std::begin(defs.strings), std::end(defs.strings), [&](auto& str) { return str.key == id; });
			if (it == std::end(defs.strings))
				continue;

			auto& def = *it;
			fprintf(out, "\n");
			if (!def.help.empty())
				fprintf(out, "#. %s\n", def.help.c_str());
			fprintf(out, "msgctxt \"%s\"\n", def.key.c_str());
			fprintf(out, "msgid \"%s\"\n", escape(def.value).c_str());
			if (def.plural.empty())
				fprintf(out, "msgstr \"\"\n");
			else {
				fprintf(out, "msgid_plural \"%s\"\n", escape(def.plural).c_str());
				fprintf(out, "msgstr[0] \"\"\n");
				fprintf(out, "msgstr[1] \"\"\n");
			}
		}

		fprintf(out, "\n");
	}
}
