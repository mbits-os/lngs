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

#include <lngs/internals/commands.hpp>
#include <lngs/internals/strings.hpp>
#include <lngs/internals/streams.hpp>

namespace lngs::app {
	std::string straighten(std::string str) {
		for (auto& c : str)
			if (c == '\n') c = ' ';
		return str;
	}
}

namespace lngs::app::pot {
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

		out.fmt(R"(# {5}.
# Copyright (C) {0} {3}
# This file is distributed under the same license as the {1} package.
# {4}, {0}.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: {1} {2}\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {6}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
)",

/*0*/ thisYear(),
/*1*/ defs.project, /*2*/ defs.version,
/*3*/ nfo.copy, /*4*/ nfo.first_author, /*5*/ nfo.title,
/*6*/ creationDate());

		if (has_plurals)
			out.fmt(R"("Plural-Forms: nplurals=2; plural=(n != 1);\n"
)");

		std::vector<std::string> ids;
		ids.reserve(defs.strings.size());
		transform(begin(defs.strings), end(defs.strings), back_inserter(ids), [](auto& str) {return str.key; });
		sort(begin(ids), end(ids));

		for (auto& id : ids) {
			auto it = find_if(begin(defs.strings), end(defs.strings), [&](auto& str) { return str.key == id; });

			auto& def = *it;
			out.fmt("\n");
			if (!def.help.empty())
				out.fmt("#. {}\n", def.help);
			out.fmt("msgctxt \"{}\"\n", def.key);
			out.fmt("msgid \"{}\"\n", escape(def.value));
			if (def.plural.empty())
				out.fmt("msgstr \"\"\n");
			else {
				out.fmt("msgid_plural \"{}\"\n", escape(def.plural));
				out.fmt("msgstr[0] \"\"\n");
				out.fmt("msgstr[1] \"\"\n");
			}
		}

		out.fmt("\n");

		return 0;
	}
}
