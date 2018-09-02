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

#include <ctime>
#include <iterator>
#include <algorithm>

#include <lngs/commands.hpp>
#include <lngs/strings.hpp>
#include <lngs/streams.hpp>

namespace lngs::pot {
	auto now_recalc() {
		struct tm tm;
		time_t now;
		std::time(&now);
		tm = *localtime(&now);
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

	int write(outstream& out, const idl_strings& defs, const info& nfo)
	{
		auto has_plurals = find_if(begin(defs.strings), end(defs.strings),
			[](auto& str) { return !str.plural.empty(); }) != end(defs.strings);

		out.printf(R"(# Copyright (C) %d %s
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
			out.printf(R"("Plural-Forms: nplurals=2; plural=(n != 1);\n"
)");

		std::vector<std::string> ids;
		ids.reserve(defs.strings.size());
		transform(begin(defs.strings), end(defs.strings), back_inserter(ids), [](auto& str) {return str.key; });
		sort(begin(ids), end(ids));

		for (auto& id : ids) {
			auto it = find_if(begin(defs.strings), end(defs.strings), [&](auto& str) { return str.key == id; });

			auto& def = *it;
			out.printf("\n");
			if (!def.help.empty())
				out.printf("#. {}\n", def.help.c_str());
			out.printf("msgctxt \"{}\"\n", def.key.c_str());
			out.printf("msgid \"{}\"\n", escape(def.value).c_str());
			if (def.plural.empty())
				out.printf("msgstr \"\"\n");
			else {
				out.printf("msgid_plural \"{}\"\n", escape(def.plural).c_str());
				out.printf("msgstr[0] \"\"\n");
				out.printf("msgstr[1] \"\"\n");
			}
		}

		out.printf("\n");

		return 0;
	}
}
