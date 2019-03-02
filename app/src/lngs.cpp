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

#include <args/parser.hpp>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>
#include <lngs/translation.hpp>
#include "dirs.hpp"

#if defined(_WIN32) && defined(_UNICODE)
using XChar = wchar_t;
#define main wmain
#else
using XChar = char;
#endif

namespace lngs::app {
	namespace pot { int call(args::parser&); }
	namespace enums { int call(args::parser&); }
	namespace py { int call(args::parser&); }
	namespace make { int call(args::parser&); }
	namespace res { int call(args::parser&); }
	namespace freeze { int call(args::parser&); }
}

struct command {
	const char* name;
	const char* description;
	int(*call)(args::parser&);
};

command commands[] = {
	{ "make",  "Translates MO file to LNG file.", lngs::app::make::call },
	{ "pot",   "Creates POT file from message file.", lngs::app::pot::call },
	{ "enums", "Creates header file from message file.", lngs::app::enums::call },
	{ "py",    "Creates Python module with string keys.", lngs::app::py::call },
	{ "res",   "Creates C++ file with fallback resource for the message file.", lngs::app::res::call },
	{ "freeze","Reads the language description file and assigns values to new strings.", lngs::app::freeze::call },
};

int main(int argc, XChar* argv[])
{
	args::null_translator tr;
	args::parser base{ {}, argc > 1 ? 2 : 1, argv, &tr };
	base.usage("[-h] <command> [<args>]");

	if (base.args().empty())
		base.error("command missing");

	if (base.args().front() == "-h") {
		base.short_help();

		{
			args::fmt_list known(1);
			auto& chunk = known.front();
			chunk.title = "known commands";
			for (auto& cmd : commands)
				chunk.items.push_back(std::make_pair(cmd.name, cmd.description));
			args::printer{ stdout }.format_list(known);
		}

		printf(R"(
The flow for string management and creation:

1. String Manager:
   > lngs freeze + git 
2. Developer (compile existing list):
   > lngs enums
   > lngs res
   > git commit .hpp .cpp
3. Developer (add new string):
   [edit .lngs file]
   > git commit .lngs
4. Translator:
   > lngs pot
   > msgmerge (or msginit)
   > git commit .po
5. Developer (release build):
   > msgfmt
   > lngs make
)");
		return 0;
	}

	auto& name = base.args().front();
	for (auto& cmd : commands) {
		if (cmd.name != name)
			continue;

		args::parser sub{ cmd.description, argc - 1, argv + 1, &tr };
		sub.program(base.program() + " " + sub.program());

		return cmd.call(sub);
	}

	base.error("unknown command: " + std::string{ name.data(), name.length() });
}

namespace lngs::app {
	enum print_outname {
		with_outname = true,
		without_outname = false
	};
	inline print_outname print_if(bool verbose) {
		return verbose ? with_outname : without_outname;
	}

	template <typename Writer>
	int write(const std::string& progname, diagnostics& diag, const fs::path & outname, Writer writer, print_outname verbose = without_outname) {
		if (outname == "-")
			return writer(get_stdout());

		auto name = outname;
		name.make_preferred();

		auto src = diag.source(progname);
		if (verbose == with_outname)
			diag.push_back(src.position()[severity::verbose] << outname.string());

		auto outf = fs::fopen(outname, "wb");
		if (!outf) {
			diag.push_back(src.position()[severity::error] << arg(lng::ERR_FILE_MISSING, name.string()));
			return 1;
		}

		foutstream out{ std::move(outf) };
		return writer(out);
	}

	template <typename StringsImpl>
	struct locale_base {
		idl_strings strings;
		diagnostics diag;
		StringsImpl tr;

		~locale_base() {
			auto& out = get_stdout();
			for (const auto& diagnostic : diag.diagnostic_set())
				diagnostic.print(out, diag, tr);
		}

		int read_strings(args::parser& parser, const fs::path& in, bool verbose) {
			return app::read_strings(parser.program(), in, strings, verbose, diag) ? 0 : 1;
		}

		template <typename Writer>
		int write(args::parser& parser, const fs::path & outname, Writer writer, print_outname verbose = without_outname) {
			return app::write(parser.program(), diag, outname, std::move(writer), verbose);
		}
	};

	struct main_strings : public strings {
		main_strings() {
			m_prefix.path_manager<manager::ExtensionPath>(
				directory_info::prefix, "lngs"
				);
			m_build.path_manager<manager::ExtensionPath>(
				directory_info::build, "lngs"
				);

			auto system = system_locales();
			[&] {
				m_build.init();
				if (m_prefix.open_first_of(system)) {
					auto locale = m_prefix.attr(ATTR_CULTURE);
					if (!locale.empty()) {
						m_build.open(locale);
						return;
					}
				}
				m_build.open_first_of(system);
			} ();
		}
		std::string_view get(lng str) const final {
			auto result = m_prefix.get(str);
			if (result.empty())
				result = m_build.get(str);
			return result;
		}

	private:
		struct PrefixStrings : SingularStrings<lng> {
			std::string_view get(lng str) const noexcept
			{
				return get_string(static_cast<identifier>(str));
			}
		} m_prefix;

		struct Strings : lngs::app::Strings {
			std::string_view get(lng str) const noexcept
			{
				return get_string(static_cast<identifier>(str));
			}
		} m_build;
	};

	using locale_setup = locale_base<main_strings>;
}

namespace lngs::app::pot {
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
		parser.arg(nfo.title, "t", "title").meta("TITLE").help("some descriptive title").opt();
		parser.parse();

		locale_setup setup;
		if (int res = setup.read_strings(parser, inname, verbose))
			return res;

		return setup.write(parser, outname, [&](outstream& out) {
			return write(out, setup.strings, nfo);
		}, print_if(verbose));
	}
}

namespace lngs::app::enums {
	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;
		bool with_resource = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.set<std::true_type>(with_resource, "r", "resource").help("configures the Strings type to use `lngs res'.").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("C++ header file to write; if - is used, result is written to standard output");
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.parse();

		locale_setup setup;
		if (int res = setup.read_strings(parser, inname, verbose))
			return res;

		return setup.write(parser, outname, [&](outstream& out) {
			return write(out, setup.strings, with_resource);
		}, print_if(verbose));
	}
}

namespace lngs::app::py {
	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("C++ header file to write; if - is used, result is written to standard output");
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.parse();

		locale_setup setup;
		if (int res = setup.read_strings(parser, inname, verbose))
			return res;

		return setup.write(parser, outname, [&](outstream& out) {
			return write(out, setup.strings);
		}, print_if(verbose));
	}
}

namespace lngs::app::make {
	int call(args::parser& parser)
	{
		fs::path inname, outname;
		std::string moname, llname;
		bool verbose = false;
		bool warp_missing = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.set<std::true_type>(warp_missing, "w", "warp").help("replace missing strings with warped ones; resulting strings are always singular").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("LNG binary file to write; if - is used, result is written to standard output");
		parser.arg(moname, "m", "msgs").meta("MOFILE").help("GetText message file to read");
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.arg(llname, "l", "lang").meta("FILE").help("ATTR_LANGUAGE file with ll_CC names list").opt();
		parser.parse();

		locale_setup setup;
		if (int res = setup.read_strings(parser, inname, verbose))
			return res;

		auto file = load_mo(setup.strings, warp_missing, verbose, setup.diag.open(moname, "rb"), setup.diag);
		if (setup.diag.has_errors())
			return 1;

		if (!llname.empty())
			setup.diag.open(llname);

		if (auto mo = setup.diag.source(moname); !fix_attributes(file, mo, llname, setup.diag))
			return 1;

		return setup.write(parser, outname, [&](outstream& out) {
			return file.write(out);
		}, print_if(verbose));
	}
}

namespace lngs::app::res {
	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;
		bool warp_strings = false;
		bool with_keys = false;
		std::string include;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.set<std::true_type>(warp_strings, "w", "warp").help("replace all strings with warped ones; plural strings will still be plural (as if English)").opt();
		parser.set<std::true_type>(with_keys, "k", "keys").help("add string block with string keys").opt();
		parser.arg(outname, "o", "out").meta("FILE").help("LNG binary file to write; if - is used, result is written to standard output");
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.arg(include, "include").meta("FILE").help("File to include for the definition of the Resource class. Defaults to \"<project>.hpp\".").opt();
		parser.parse();

		locale_setup setup;
		if (int res = setup.read_strings(parser, inname, verbose))
			return res;

		if (include.empty())
			include = setup.strings.project + ".hpp";

		auto file = make_resource(setup.strings, warp_strings, with_keys);

		return setup.write(parser, outname, [&](outstream& out) {
			return update_and_write(out, file, include, setup.strings.project);
		}, print_if(verbose));
	}
}

namespace lngs::app::freeze {
	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.arg(outname, "o", "out").meta("FILE").help("LNGS message file to write; it may be the same as input; if - is used, result is written to standard output");
		parser.parse();

		locale_setup setup;
		if (int res = setup.read_strings(parser, inname, verbose))
			return res;

		if (!freeze::freeze(setup.strings)) {
			if (verbose) {
				auto src = setup.diag.source(inname.string()).position();
				setup.diag.push_back(src[severity::note] << lng::ERR_NO_NEW_STRINGS);
			}
			return 0;
		}

		auto contents = setup.diag.source(inname.string());
		return setup.write(parser, outname, [&](outstream& out) {
			return write(out, setup.strings, contents);
		});
	}
}