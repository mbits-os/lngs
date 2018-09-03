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

#include <lngs/argparser.hpp>
#include <lngs/commands.hpp>
#include <lngs/diagnostics.hpp>
#include <lngs/languages.hpp>
#include <lngs/strings.hpp>
#include <lngs/streams.hpp>
#include <locale/translation.hpp>
#include "dirs.hpp"

#if defined(_WIN32) && defined(_UNICODE)
using XChar = wchar_t;
#define main wmain
#else
using XChar = char;
#endif

namespace lngs {
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
	{ "make",  "Translates MO file to LNG file.", lngs::make::call },
	{ "pot",   "Creates POT file from message file.", lngs::pot::call },
	{ "enums", "Creates header file from message file.", lngs::enums::call },
	{ "py",    "Creates Python module with string keys.", lngs::py::call },
	{ "res",   "Creates C++ file with fallback resource for the message file.", lngs::res::call },
	{ "freeze","Reads the language description file and assigns values to new strings.", lngs::freeze::call },
};

int main(int argc, XChar* argv[])
{
	args::parser base{ {}, argc > 1 ? 2 : 1, argv };
	base.usage("[-h] <command> [<args>]");

	if (base.args().empty())
		base.error("command missing");

	if (base.args().front() == "-h") {
		base.short_help();
		printf("\nknown commands:\n\n");

		std::vector<std::pair<std::string, std::string>> info;

		for (auto& cmd : commands)
			info.push_back(std::make_pair(cmd.name, cmd.description));

		base.format_list(info);
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

		args::parser sub{ cmd.description, argc - 1, argv + 1 };
		sub.program(base.program() + " " + sub.program());

		return cmd.call(sub);
	}

	base.error("unknown command: " + name);
}

namespace lngs {
	enum print_outname {
		with_outname = true,
		without_outname = false
	};
	inline print_outname print_if(bool verbose) {
		return verbose ? with_outname : without_outname;
	}

	template <typename Writer>
	int write(const fs::path & outname, Writer writer, print_outname verbose = without_outname) {
		if (outname == "-")
			return writer(get_stdout());

		auto name = outname;
		name.make_preferred();

		auto outf = fs::fopen(outname, "wb");
		if (!outf) {
			fmt::print(stderr, "could not open `{}'", name.string());
			return 1;
		}

		if (verbose == with_outname)
			fmt::print("{}\n", name.string());

		foutstream out{ std::move(outf) };
		return writer(out);
	}

	template <typename StringsImpl>
	struct locale_base {
		idl_strings strings;
		diagnostics diag;
		StringsImpl tr;

		~locale_base() {
			printer_anchor tmp{ get_stdout(), diag, tr };
		}

		int read_strings(args::parser&, const fs::path& in, bool verbose) {
			if (!lngs::read_strings(in, strings, verbose)) {
				fmt::print(stderr, "`{}' is not strings file.\n", in.string());
				return 1;
			}
			return 0;
		}

		template <typename Writer>
		int write(args::parser&, const fs::path & outname, Writer writer, print_outname verbose = without_outname) {
			return lngs::write(outname, std::move(writer), verbose);
		}
	};

	struct main_strings : public strings {
		main_strings() {
			m_prefix.path_manager<locale::manager::ExtensionPath>(
				directory_info::prefix, "lngs"
				);
			m_build.path_manager<locale::manager::ExtensionPath>(
				directory_info::build, "lngs"
				);

			auto system = locale::system_locales();
			[&] {
				m_build.init();
				if (m_prefix.open_first_of(system)) {
					auto locale = m_prefix.attr(locale::ATTR_CULTURE);
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
		struct PrefixStrings : locale::SingularStrings<lng> {
			std::string_view get(lng str) const noexcept
			{
				return get_string(static_cast<identifier>(str));
			}
		} m_prefix;

		struct Strings : lngs::Strings {
			std::string_view get(lng str) const noexcept
			{
				return get_string(static_cast<identifier>(str));
			}
		} m_build;
	};

	using locale_setup = locale_base<main_strings>;
}

namespace lngs::pot {
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

namespace lngs::enums {
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

namespace lngs::py {
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

namespace lngs::make {
	int call(args::parser& parser)
	{
		fs::path moname, inname, outname, llname;
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

		auto file = load_mo(setup.strings, warp_missing, verbose, moname);
		if (!fix_attributes(file, llname))
			return 1;

		if (outname == "-")
			return file.write(get_stdout());

		auto name = outname;
		name.make_preferred();

		auto outf = fs::fopen(outname, "wb");
		if (!outf) {
			fmt::print(stderr, "could not open `{}'", name.string());
			return 1;
		}

		if (verbose)
			fmt::print("{}\n", name.string());
		foutstream out{ std::move(outf) };
		return file.write(out);
	}
}

namespace lngs::res {
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

namespace lngs::freeze {
	int call(args::parser& parser)
	{
		fs::path inname, outname;
		bool verbose = false;

		parser.set<std::true_type>(verbose, "v", "verbose").help("show more info").opt();
		parser.arg(inname, "i", "in").meta("FILE").help("LNGS message file to read");
		parser.arg(outname, "o", "out").meta("FILE").help("LNGS message file to write; it may be the same as input; if - is used, result is written to standard output");
		parser.parse();

		inname.make_preferred();
		if (verbose)
			fmt::print("{}\n", inname.string());

		idl_strings strings;
		std::vector<std::byte> contents;

		{
			auto inf = fs::fopen(inname, "rb");

			if (!inf) {
				fmt::print(stderr, "could not open `{}'", inname.string());
				return -1;
			}
			contents = inf.read();
		}

		{
			meminstream is{ contents.data(), contents.size() };
			if (!read_strings(is, inname.string(), strings)) {
				if (verbose)
					fmt::print(stderr, "`{}' is not strings file.\n", inname.string().c_str());
				return -1;
			}
		}

		if (!freeze::freeze(strings)) {
			if (verbose)
				fmt::print("No new strings found\n");
			return 0;
		}

		return write_result(outname, [&](outstream& out) {
			return write(out, strings, contents);
		});
	}
}