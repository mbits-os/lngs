// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <algorithm>
#include <ctime>
#include <iterator>

#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app {
	std::string straighten(std::string const& input) {
		auto str = input;
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

	int year_from_template(diags::source_code file) {
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

	int write(diags::outstream& out,
	          const idl_strings& defs,
	          std::optional<std::filesystem::path> const& redirected,
	          const info& nfo) {
		return write_mstch(
		    out, defs, redirected, "pot",
		    {
		        {"title", nfo.title},
		        {"first_author", nfo.first_author},
		        {"creation_date", creationDate()},
		        {
		            "copy",
		            mstch::map{
		                {"year", nfo.year == -1 ? thisYear() : nfo.year},
		                {"holder", nfo.copy},
		            },
		        },
		    },
		    {escape});
	}
}  // namespace lngs::app::pot
