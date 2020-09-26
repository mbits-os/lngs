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
#include "build.hpp"

using namespace std::literals;

#if defined(_WIN32) && defined(_UNICODE)
using XChar = wchar_t;
#define main wmain
#else
using XChar = char;
#endif

namespace lngs::app {
	template <typename StringsImpl>
	struct locale_base;
	struct main_strings;

	using locale_setup = locale_base<main_strings>;

	namespace pot {
		int call(args::parser&, locale_setup&);
	}
	namespace enums {
		int call(args::parser&, locale_setup&);
	}
	namespace py {
		int call(args::parser&, locale_setup&);
	}
	namespace make {
		int call(args::parser&, locale_setup&);
	}
	namespace res {
		int call(args::parser&, locale_setup&);
	}
	namespace freeze {
		int call(args::parser&, locale_setup&);
	}
}  // namespace lngs::app

using lngs::app::lng;

struct command {
	const char* name;
	const lng description;
	int (*call)(args::parser&, lngs::app::locale_setup&);
};

command commands[] = {
    {"make", lng::ARGS_APP_DESCR_CMD_MAKE, lngs::app::make::call},
    {"pot", lng::ARGS_APP_DESCR_CMD_POT, lngs::app::pot::call},
    {"enums", lng::ARGS_APP_DESCR_CMD_ENUMS, lngs::app::enums::call},
    {"py", lng::ARGS_APP_DESCR_CMD_PY, lngs::app::py::call},
    {"res", lng::ARGS_APP_DESCR_CMD_RES, lngs::app::res::call},
    {"freeze", lng::ARGS_APP_DESCR_CMD_FREEZE, lngs::app::freeze::call},
};

namespace lngs::app {
	enum print_outname { with_outname = true, without_outname = false };
	inline print_outname print_if(bool verbose) {
		return verbose ? with_outname : without_outname;
	}

	template <typename Writer>
	int write(const std::string& progname,
	          diagnostics& diag,
	          const fs::path& outname,
	          Writer writer,
	          print_outname verbose = without_outname) {
		if (outname == "-") return writer(get_stdout());

		auto name = outname;
		name.make_preferred();

		auto src = diag.source(progname);
		if (verbose == with_outname)
			diag.push_back(src.position()[severity::verbose]
			               << outname.string());

		auto outf = fs::fopen(outname, "wb");
		if (!outf) {
			diag.push_back(src.position()[severity::error]
			               << arg(lng::ERR_FILE_MISSING, name.string()));
			return 1;
		}

		foutstream out{std::move(outf)};
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

		int read_strings(args::parser& parser,
		                 const fs::path& in,
		                 bool verbose) {
			return app::read_strings(parser.program(), in, strings, verbose,
			                         diag)
			           ? 0
			           : 1;
		}

		template <typename Writer>
		int write(args::parser& parser,
		          const fs::path& outname,
		          Writer writer,
		          print_outname verbose = without_outname) {
			return app::write(parser.program(), diag, outname,
			                  std::move(writer), verbose);
		}
	};

	struct main_strings : public strings, public args::base_translator {
		main_strings() {
			m_prefix.path_manager<manager::ExtensionPath>(
			    build::directory_info::prefix, "lngs");
			m_build.path_manager<manager::ExtensionPath>(
			    build::directory_info::build, "lngs");

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
			}();
		}
		std::string_view get(lng str) const final {
			auto result = m_prefix.get(str);
			if (result.empty()) result = m_build.get(str);
			return result;
		}

		std::string operator()(args::lng id,
		                       std::string_view arg1,
		                       std::string_view arg2) const override {
			auto const ident = [](auto id) {
				switch (id) {
					case args::lng::usage:
						return lng::ARGS_USAGE;
					case args::lng::def_meta:
						return lng::ARGS_DEF_META;
					case args::lng::optionals:
						return lng::ARGS_OPTIONALS;
					case args::lng::help_description:
						return lng::ARGS_HELP_DESCRIPTION;
					case args::lng::unrecognized:
						return lng::ARGS_UNRECOGNIZED;
					case args::lng::needs_param:
						return lng::ARGS_NEEDS_PARAM;
					case args::lng::required:
						return lng::ARGS_REQUIRED;
					case args::lng::error_msg:
						return lng::ARGS_ERROR_MSG;
					// missing:
					// - args::lng::positionals
					// - args::lng::needs_number
					// - args::lng::needed_number_exceeded
					default:
						break;
				};
				return static_cast<lng>(0);
			}(id);
			return fmt::format(get(ident), arg1, arg2);
		}

	private:
		struct PrefixStrings : SingularStrings<lng> {
			std::string_view get(lng str) const noexcept {
				return get_string(static_cast<identifier>(str));
			}
		} m_prefix;

		struct Strings : lngs::app::Strings {
			std::string_view get(lng str) const noexcept {
				return get_string(static_cast<identifier>(str));
			}
		} m_build;
	};
}  // namespace lngs::app

[[noreturn]] void show_help(args::parser& p, lngs::app::main_strings& tr) {
	auto _ = [&tr](auto id) { return tr.get(id); };
	auto args = p.printer_arguments();
	args.resize(args.size() + 1);
	args.back().title = _(lng::ARGS_APP_KNOWN_CMDS);
	args.back().items.reserve(sizeof(commands) / sizeof(commands[0]));
	for (auto const& cmd : commands) {
		auto view = _(cmd.description);
		args.back().items.push_back({cmd.name, {view.data(), view.size()}});
	}

	p.short_help();
	args::printer{stdout}.format_list(args);
	fmt::print(R"(
{}:

1. {}:
   > lngs freeze + git
2. {}:
   > lngs enums
   > lngs res
   > git commit .hpp .cpp
3. {}:
   [edit .lngs file]
   > git commit .lngs
4. {}:
   > lngs pot
   > msgmerge (or msginit)
   > git commit .po
5. {}:
   > msgfmt
   > lngs make
)",
	           _(lng::ARGS_APP_FLOW_TITLE), _(lng::ARGS_APP_FLOW_ROLE_STRMGR),
	           _(lng::ARGS_APP_FLOW_ROLE_DEV_COMPILE),
	           _(lng::ARGS_APP_FLOW_ROLE_DEV_ADD),
	           _(lng::ARGS_APP_FLOW_ROLE_TRANSLATOR),
	           _(lng::ARGS_APP_FLOW_ROLE_DEV_RELEASE));

	std::exit(0);
}

[[noreturn]] void show_version() {
	using ver = lngs::app::build::version;
	fmt::print("lngs {}{}\n", ver::string, ver::stability);
	std::exit(0);
}

int main(int argc, XChar* argv[]) {
	lngs::app::locale_setup setup;

	auto show_help_tr = [&setup](args::parser& p) { show_help(p, setup.tr); };
	auto _ = [&setup](auto id) { return setup.tr.get(id); };
	auto _s = [&_](auto id) {
		auto view = _(id);
		return std::string{view.data(), view.size()};
	};

	args::parser base{{}, args::from_main(argc, argv), &setup.tr};
	base.usage(_(lng::ARGS_APP_DESCR_USAGE));
	base.provide_help(false);
	base.custom(show_help_tr, "h", "help")
	    .help(_(lng::ARGS_HELP_DESCRIPTION))
	    .opt();
	base.custom(show_version, "v", "version")
	    .help(_(lng::ARGS_APP_VERSION))
	    .opt();

	auto const unparsed = base.parse(args::parser::allow_subcommands);
	if (unparsed.empty()) base.error(_s(lng::ARGS_APP_NO_COMMAND));

	auto const name = unparsed[0];
	for (auto& cmd : commands) {
		if (cmd.name != name) continue;

		auto view = setup.tr.get(cmd.description);
		args::parser sub{{view.data(), view.size()}, unparsed, &setup.tr};
		sub.program(base.program() + " " + sub.program());

		return cmd.call(sub, setup);
	}

	base.error(fmt::format(_(lng::ARGS_APP_UNK_COMMAND), name));
}

namespace lngs::app::pot {
	int call(args::parser& parser, locale_setup& setup) {
		fs::path inname, outname;
		bool verbose = false;
		info nfo;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		parser.set<std::true_type>(verbose, "v", "verbose")
		    .help(_(lng::ARGS_APP_VERBOSE))
		    .opt();
		parser.arg(inname, "i", "in")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_IDL));
		parser.arg(nfo.copy, "c", "copy")
		    .meta(_(lng::ARGS_APP_META_HOLDER))
		    .help(_(lng::ARGS_APP_COPYRIGHT))
		    .opt();
		parser.arg(nfo.first_author, "a", "author")
		    .meta(_(lng::ARGS_APP_META_EMAIL))
		    .help(_(lng::ARGS_APP_AUTHOR));
		parser.arg(nfo.title, "t", "title")
		    .meta(_(lng::ARGS_APP_META_TITLE))
		    .help(_(lng::ARGS_APP_TITLE))
		    .opt();
		parser.arg(outname, "o", "out")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_OUT_POT));
		parser.parse();

		if (int res = setup.read_strings(parser, inname, verbose)) return res;

		return setup.write(
		    parser, outname,
		    [&](outstream& out) { return write(out, setup.strings, nfo); },
		    print_if(verbose));
	}
}  // namespace lngs::app::pot

namespace lngs::app::enums {
	int call(args::parser& parser, locale_setup& setup) {
		fs::path inname, outname;
		bool verbose = false;
		bool with_resource = false;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		parser.set<std::true_type>(verbose, "v", "verbose")
		    .help(_(lng::ARGS_APP_VERBOSE))
		    .opt();
		parser.set<std::true_type>(with_resource, "r", "resource")
		    .help(_(lng::ARGS_APP_RESOURCE))
		    .opt();
		parser.arg(inname, "i", "in")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_IDL));
		parser.arg(outname, "o", "out")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_OUT_CPP));
		parser.parse();

		if (int res = setup.read_strings(parser, inname, verbose)) return res;

		return setup.write(
		    parser, outname,
		    [&](outstream& out) {
			    return write(out, setup.strings, with_resource);
		    },
		    print_if(verbose));
	}
}  // namespace lngs::app::enums

namespace lngs::app::py {
	int call(args::parser& parser, locale_setup& setup) {
		fs::path inname, outname;
		bool verbose = false;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		parser.set<std::true_type>(verbose, "v", "verbose")
		    .help(_(lng::ARGS_APP_VERBOSE))
		    .opt();
		parser.arg(inname, "i", "in")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_IDL));
		parser.arg(outname, "o", "out")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_OUT_PY));
		parser.parse();

		if (int res = setup.read_strings(parser, inname, verbose)) return res;

		return setup.write(
		    parser, outname,
		    [&](outstream& out) { return write(out, setup.strings); },
		    print_if(verbose));
	}
}  // namespace lngs::app::py

namespace lngs::app::make {
	int call(args::parser& parser, locale_setup& setup) {
		fs::path inname, outname;
		std::string moname, llname;
		bool verbose = false;
		bool warp_missing = false;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		parser.set<std::true_type>(verbose, "v", "verbose")
		    .help(_(lng::ARGS_APP_VERBOSE))
		    .opt();
		parser.set<std::true_type>(warp_missing, "w", "warp")
		    .help(_(lng::ARGS_APP_WARP_MISSING_SINGULAR))
		    .opt();
		parser.arg(inname, "i", "in")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_IDL));
		parser.arg(moname, "m", "msgs")
		    .meta(_(lng::ARGS_APP_META_PO_MO_FILE))
		    .help(_(lng::ARGS_APP_IN_PO_MO));
		parser.arg(llname, "l", "lang")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_LLCC))
		    .opt();
		parser.arg(outname, "o", "out")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_OUT_LNG));
		parser.parse();

		if (int res = setup.read_strings(parser, inname, verbose)) return res;

		auto file = load_msgs(setup.strings, warp_missing, verbose,
		                      setup.diag.open(moname, "rb"), setup.diag);
		if (setup.diag.has_errors()) return 1;

		if (!llname.empty()) setup.diag.open(llname);

		if (auto mo = setup.diag.source(moname);
		    !fix_attributes(file, mo, llname, setup.diag))
			return 1;

		return setup.write(
		    parser, outname, [&](outstream& out) { return file.write(out); },
		    print_if(verbose));
	}
}  // namespace lngs::app::make

namespace lngs::app::res {
	int call(args::parser& parser, locale_setup& setup) {
		fs::path inname, outname;
		bool verbose = false;
		bool warp_strings = false;
		bool with_keys = false;
		std::string include;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		parser.set<std::true_type>(verbose, "v", "verbose")
		    .help(_(lng::ARGS_APP_VERBOSE))
		    .opt();
		parser.set<std::true_type>(warp_strings, "w", "warp")
		    .help(_(lng::ARGS_APP_WARP_ALL_PLURAL))
		    .opt();
		parser.set<std::true_type>(with_keys, "k", "keys")
		    .help(_(lng::ARGS_APP_WITH_KEY_BLOCK))
		    .opt();
		parser.arg(inname, "i", "in")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_IDL));
		parser.arg(include, "include")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_ALT_INCLUDE))
		    .opt();
		parser.arg(outname, "o", "out")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_OUT_RES));
		parser.parse();

		if (int res = setup.read_strings(parser, inname, verbose)) return res;

		if (include.empty()) include = setup.strings.project + ".hpp";

		auto file = make_resource(setup.strings, warp_strings, with_keys);

		return setup.write(
		    parser, outname,
		    [&](outstream& out) {
			    auto const& ns_name = setup.strings.ns_name.empty()
			                              ? setup.strings.project
			                              : setup.strings.ns_name;
			    return update_and_write(out, file, include, ns_name);
		    },
		    print_if(verbose));
	}
}  // namespace lngs::app::res

namespace lngs::app::freeze {
	int call(args::parser& parser, locale_setup& setup) {
		fs::path inname, outname;
		bool verbose = false;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		parser.set<std::true_type>(verbose, "v", "verbose")
		    .help(_(lng::ARGS_APP_VERBOSE))
		    .opt();
		parser.arg(inname, "i", "in")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_IDL));
		parser.arg(outname, "o", "out")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_OUT_IDL));
		parser.parse();

		if (int res = setup.read_strings(parser, inname, verbose)) return res;

		if (!freeze::freeze(setup.strings)) {
			if (verbose) {
				auto src = setup.diag.source(inname.string()).position();
				setup.diag.push_back(src[severity::note]
				                     << lng::ERR_NO_NEW_STRINGS);
			}
			return 0;
		}

		auto contents = setup.diag.source(inname.string());
		return setup.write(parser, outname, [&](outstream& out) {
			return write(out, setup.strings, contents);
		});
	}
}  // namespace lngs::app::freeze