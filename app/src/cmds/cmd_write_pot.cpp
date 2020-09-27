// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <algorithm>
#include <ctime>
#include <iterator>

#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>

namespace lngs::app {
	std::string straighten(std::string str) {
		for (auto& c : str)
			if (c == '\n') c = ' ';
		return str;
	}
}  // namespace lngs::app

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
				case '\\':
					out.append("\\\\");
					break;
				case '\a':
					out.append("\\a");
					break;
				case '\b':
					out.append("\\b");
					break;
				case '\f':
					out.append("\\f");
					break;
				case '\n':
					out.append("\\n");
					break;
				case '\r':
					out.append("\\r");
					break;
				case '\t':
					out.append("\\t");
					break;
				case '\v':
					out.append("\\v");
					break;
				case '\"':
					out.append("\\\"");
					break;
				default:
					out.push_back(c);
			}
		}
		return out;
	}

	int year_from_template(source_file file) {
		if (!file.valid()) return -1;

		static constexpr std::string_view prefix{"# Copyright (C) "};
		unsigned lineno{};
		while (true) {
			auto line = file.line(++lineno);
			if (line.empty() || line.front() != '#') break;
			if (line.size() <= prefix.size() ||
			    line.substr(0, prefix.size()) != prefix) {
				continue;
			}
			line = line.substr(prefix.size());
			int result = 0;
			size_t pos = 0;
			while (pos < line.size() &&
			       std::isdigit(static_cast<unsigned char>(line[pos]))) {
				result *= 10;
				result += static_cast<int>(line[pos]) - '0';
				++pos;
			}

			if (pos > 0 &&
			    (pos == line.size() ||
			     std::isspace(static_cast<unsigned char>(line[pos])))) {
				return result;
			}
		}
		return -1;
	}

	int write(outstream& out, const idl_strings& defs, const info& nfo) {
		auto has_plurals =
		    find_if(begin(defs.strings), end(defs.strings), [](auto& str) {
			    return !str.plural.empty();
		    }) != end(defs.strings);

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

		        /*0*/ nfo.year == -1 ? thisYear() : nfo.year,
		        /*1*/ defs.project, /*2*/ defs.version,
		        /*3*/ nfo.copy, /*4*/ nfo.first_author, /*5*/ nfo.title,
		        /*6*/ creationDate());

		if (has_plurals)
			out.fmt(R"("Plural-Forms: nplurals=2; plural=(n != 1);\n"
)");

		std::vector<std::string> ids;
		ids.reserve(defs.strings.size());
		transform(begin(defs.strings), end(defs.strings), back_inserter(ids),
		          [](auto& str) { return str.key; });
		sort(begin(ids), end(ids));

		for (auto& id : ids) {
			auto it = find_if(begin(defs.strings), end(defs.strings),
			                  [&](auto& str) { return str.key == id; });

			auto& def = *it;
			out.fmt("\n");
			if (!def.help.empty()) out.fmt("#. {}\n", def.help);
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
}  // namespace lngs::app::pot
