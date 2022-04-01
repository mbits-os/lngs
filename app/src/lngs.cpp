// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <args/parser.hpp>
#include <diags/translator.hpp>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/mstch_engine.hpp>
#include <lngs/internals/strings.hpp>
#include <lngs/translation.hpp>
#include "build.hpp"
#include "embedded_languages.hpp"

using namespace std::literals;

#if defined(_WIN32) && defined(_UNICODE)
using XChar = wchar_t;
#define main wmain
#else
using XChar = char;
#endif

using lngs::app::lng;

ENUM_TRAITS_BEGIN(diags::color)
ENUM_TRAITS_NAME(never)
ENUM_TRAITS_NAME(always)
ENUM_TRAITS_NAME_EX(enum_stg::automatic, "auto")
ENUM_TRAITS_END(diags::color)

namespace lngs::app {
	enum print_outname { with_outname = true, without_outname = false };
	inline print_outname print_if(bool verbose) {
		return verbose ? with_outname : without_outname;
	}

	template <typename Writer>
	int write(const std::string& progname,
	          diags::sources& diag,
	          const std::filesystem::path& outname,
	          Writer writer,
	          print_outname verbose = without_outname) {
		if (outname == "-") return writer(diags::get_stdout());

		auto name = outname;
		name.make_preferred();

		auto src = diag.source(progname);
		if (verbose == with_outname)
			diag.push_back(src.position()[diags::severity::verbose]
			               << outname.string());

		auto outf = diags::fs::fopen(outname, "wb");
		if (!outf) {
			diag.push_back(src.position()[diags::severity::error]
			               << format(lng::ERR_FILE_MISSING, name.string()));
			return 1;
		}

		diags::foutstream out{std::move(outf)};
		return writer(out);
	}

	struct arguments {
		std::filesystem::path inname;
		std::filesystem::path outname;
		bool verbose = false;
		diags::color color_type{diags::color::never};

		template <typename Translator>
		void setup_parser(args::parser& parser,
		                  lng inname_usage,
		                  lng outname_usage,
		                  Translator _) {
			parser.provide_help(false);
			auto show_help = +[](args::parser& p) { p.help(); };

			parser.arg(inname)
			    .meta(_(lng::ARGS_APP_META_INPUT))
			    .help(_(inname_usage));
			parser.arg(outname, "o")
			    .meta(_(lng::ARGS_APP_META_FILE))
			    .help(_(outname_usage));
			parser.custom(show_help, "h", "help")
			    .help(_(lng::ARGS_HELP_DESCRIPTION))
			    .opt();
			parser.set<std::true_type>(verbose, "v", "verbose")
			    .help(_(lng::ARGS_APP_VERBOSE))
			    .opt();
			parser.arg(color_type, "color")
			    .meta(_(lng::ARGS_APP_META_WHEN))
			    .help(_(lng::ARGS_APP_COLOR))
			    .opt();
		}
	};
	template <typename StringsImpl>
	struct setup_base {
		idl_strings strings{};
		diags::sources diag{};
		StringsImpl tr;
		arguments common{};
		args::parser parser{{}, {}, {}, &tr};

		template <typename... Args>
		setup_base(Args&&... args) : tr{std::forward<Args>(args)...} {}

		~setup_base() { diag.print_diagnostics(); }

		void parser_common(lng inname_usage, lng outname_usage) {
			common.setup_parser(parser, inname_usage, outname_usage,
			                    [tr = &tr](lng id) { return tr->get(id); });
		}

		int read_strings() {
			using printer = diags::printer;
			diag.set_printer<printer>(&diags::get_stdout(), tr.make(),
			                          common.color_type);

			return app::read_strings(parser.program(), common.inname, strings,
			                         common.verbose, diag)
			           ? 0
			           : 1;
		}

		template <typename Writer>
		int write(Writer writer) {
			return app::write(parser.program(), diag, common.outname,
			                  std::move(writer), print_if(common.verbose));
		}

		mstch_env env(diags::outstream& out) {
#ifdef LNGS_LINKED_RESOURCES
			return {out, this->strings};
#else
			return {out, this->strings, this->tr.redirected()};
#endif
		}
	};

	class MemoryBased {
		lang_file m_file{};
		memory_view m_data{};

		bool open(const std::string& lng, SerialNumber serial) {
			auto const check_serial = serial != SerialNumber::UseAny;
			auto const serial_to_check = static_cast<unsigned>(serial);

			auto const view = languages::get_resource(lng);
			if (!view) return false;
			m_data = memory_view{
			    reinterpret_cast<std::byte const*>(view->data()),
			    view->size(),
			};

			if (!m_file.open(m_data) ||
			    (check_serial && m_file.get_serial() != serial_to_check)) {
				m_file.close();
				m_data = memory_block{};
				return false;
			}
			return true;
		}

	public:
		using FileBased = lngs::storage::FileBased;
		template <typename C>
		std::enable_if_t<FileBased::is_range_of<std::string, C>::value, bool>
		open_first_of(C&& langs, SerialNumber serial) {
			for (auto& lang : langs) {
				if (open(lang, serial)) return true;
			}

			return false;
		}

		std::string_view get_string(lang_file::identifier id) const noexcept {
			return m_file.get_string(id);
		}
	};

	template <typename ResourceT>
	class MemoryWithBuiltin : private MemoryBased,
	                          private lngs::storage::Builtin<ResourceT> {
		using B1 = MemoryBased;
		using B2 = lngs::storage::Builtin<ResourceT>;

	protected:
		using identifier = lang_file::identifier;
		using quantity = lang_file::quantity;

		std::string_view get_string(identifier val) const noexcept {
			auto ret = B1::get_string(val);
			if (!ret.empty()) return ret;
			return B2::get_string(val);
		}

	public:
		using MemoryBased::open_first_of;
		using lngs::storage::Builtin<ResourceT>::init_builtin;
	};

	struct main_strings : public translator_type, public args::base_translator {
		main_strings(
#ifndef LNGS_LINKED_RESOURCES
		    std::optional<std::filesystem::path> const& redirected
#endif
		    )
#ifndef LNGS_LINKED_RESOURCES
		    : m_redirected {
			redirected
		}
#endif
		{
#ifndef LNGS_LINKED_RESOURCES
			if (m_redirected) {
				m_install.path_manager<manager::SubdirPath>(
				    *m_redirected / "locale", "lngs.lng");
			} else {
				m_install.path_manager<manager::SubdirPath>(
				    build::directory_info::lngs_install, "lngs.lng");
			}
			m_build.path_manager<manager::SubdirPath>(
			    build::directory_info::lngs_build, "lngs.lng");
#endif

			auto system = system_locales();
			[&] {
				m_build.init_builtin();
#ifndef LNGS_LINKED_RESOURCES
				if (m_install.open_first_of(system)) {
					auto locale = m_install.attr(ATTR_CULTURE);
					if (!locale.empty()) {
						m_build.open(locale);
						return;
					}
				}
#endif
				m_build.open_first_of(system);
			}();
		}
#ifndef LNGS_LINKED_RESOURCES
		std::optional<std::filesystem::path> const& redirected()
		    const noexcept {
			return m_redirected;
		}
#endif

		// diags::translator
		std::string_view get(lng str) const noexcept final {
#ifdef LNGS_LINKED_RESOURCES
			return m_build.get(str);
#else
			auto result = m_install.get(str);
			if (result.empty()) result = m_build.get(str);
			return result;
#endif
		}

		std::string_view get(diags::severity sev) const noexcept final {
			static constexpr std::pair<diags::severity, lng> sev_map[] = {
			    {diags::severity::verbose, lng{}},
			    {diags::severity::note, lng::SEVERITY_NOTE},
			    {diags::severity::warning, lng::SEVERITY_WARNING},
			    {diags::severity::error, lng::SEVERITY_ERROR},
			    {diags::severity::fatal, lng::SEVERITY_FATAL},
			};
			for (auto [from, to] : sev_map) {
				if (from == sev) return get(to);
			}
			return {};
		}

		// args::base_translator
		std::string operator()(args::lng id,
		                       std::string_view arg1,
		                       std::string_view arg2) const override {
			auto const ident = [](auto id) {
				static constexpr std::pair<args::lng, lng> lng_map[] = {
				    {args::lng::usage, lng::ARGS_USAGE},
				    {args::lng::def_meta, lng::ARGS_DEF_META},
				    {args::lng::positionals, lng::ARGS_POSITIONALS},
				    {args::lng::optionals, lng::ARGS_OPTIONALS},
				    {args::lng::help_description, lng::ARGS_HELP_DESCRIPTION},
				    {args::lng::unrecognized, lng::ARGS_UNRECOGNIZED},
				    {args::lng::needs_param, lng::ARGS_NEEDS_PARAM},
				    {args::lng::needs_no_param, lng::ARGS_NEEDS_NO_PARAM},
				    {args::lng::needs_number, lng::ARGS_NEEDS_NUMBER},
				    {args::lng::needed_number_exceeded,
				     lng::ARGS_NEEDED_NUMBER_EXCEEDED},
				    {args::lng::required, lng::ARGS_REQUIRED},
				    {args::lng::error_msg, lng::ARGS_ERROR_MSG},
				    {args::lng::needed_enum_unknown,
				     lng::ARGS_NEEDS_ENUM_UNKNOWN},
				    {args::lng::needed_enum_known_values,
				     lng::ARGS_NEEDS_ENUM_KNOWN_VALUES},
				};
				for (auto [from, to] : lng_map) {
					if (from == id) return to;
				}
				return static_cast<lng>(0);
			}(id);
			return fmt::format(get(ident), arg1, arg2);
		}

	private:
#ifdef LNGS_LINKED_RESOURCES
		struct BuiltStrings : lngs::app::Strings::rebind<
		                          MemoryWithBuiltin<lngs::app::Resource>> {
			std::string_view get(lng str) const noexcept {
				return get_string(static_cast<identifier>(str));
			}
		} m_build;
#else
		struct InstalledStrings : lngs::app::Strings::rebind<> {
			std::string_view get(lng str) const noexcept {
				return get_string(static_cast<identifier>(str));
			}
		} m_install;

		struct BuiltStrings : lngs::app::Strings {
			std::string_view get(lng str) const noexcept {
				return get_string(static_cast<identifier>(str));
			}
		} m_build;
		std::optional<std::filesystem::path> m_redirected;
#endif
	};

	using application_setup = setup_base<main_strings>;
}  // namespace lngs::app

namespace lngs::app::pot {
	int call(application_setup& setup) {
		info nfo;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		setup.parser_common(lng::ARGS_APP_IN_IDL, lng::ARGS_APP_OUT_POT);
		setup.parser.arg(nfo.copy, "c", "copy")
		    .meta(_(lng::ARGS_APP_META_HOLDER))
		    .help(_(lng::ARGS_APP_COPYRIGHT))
		    .opt();
		setup.parser.arg(nfo.first_author, "a", "author")
		    .meta(_(lng::ARGS_APP_META_EMAIL))
		    .help(_(lng::ARGS_APP_AUTHOR));
		setup.parser.arg(nfo.title, "t", "title")
		    .meta(_(lng::ARGS_APP_META_TITLE))
		    .help(_(lng::ARGS_APP_TITLE))
		    .opt();
		setup.parser.parse();

		if (int res = setup.read_strings()) return res;

		nfo.year = year_from_template(setup.diag.open(setup.common.outname));

		return setup.write(
		    [&](diags::outstream& out) { return write(setup.env(out), nfo); });
	}
}  // namespace lngs::app::pot

namespace lngs::app::enums {
	int call(application_setup& setup) {
		bool with_resource = false;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		setup.parser_common(lng::ARGS_APP_IN_IDL, lng::ARGS_APP_OUT_CPP);
		setup.parser.set<std::true_type>(with_resource, "r", "resource")
		    .help(_(lng::ARGS_APP_RESOURCE))
		    .opt();
		setup.parser.parse();

		if (int res = setup.read_strings()) return res;

		return setup.write([&](diags::outstream& out) {
			return write(setup.env(out), with_resource);
		});
	}
}  // namespace lngs::app::enums

namespace lngs::app::py {
	int call(application_setup& setup) {
		setup.parser_common(lng::ARGS_APP_IN_IDL, lng::ARGS_APP_OUT_PY);
		setup.parser.parse();

		if (int res = setup.read_strings()) return res;

		return setup.write(
		    [&](diags::outstream& out) { return write(setup.env(out)); });
	}
}  // namespace lngs::app::py

namespace lngs::app::make {
	int call(application_setup& setup) {
		std::string moname, llname;
		bool warp_missing = false;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		setup.parser_common(lng::ARGS_APP_IN_IDL, lng::ARGS_APP_OUT_LNG);
		setup.parser.set<std::true_type>(warp_missing, "w", "warp")
		    .help(_(lng::ARGS_APP_WARP_MISSING_SINGULAR))
		    .opt();
		setup.parser.arg(moname, "m", "msgs")
		    .meta(_(lng::ARGS_APP_META_PO_MO_FILE))
		    .help(_(lng::ARGS_APP_IN_PO_MO));
		setup.parser.arg(llname, "l", "lang")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_IN_LLCC))
		    .opt();
		setup.parser.parse();

		if (int res = setup.read_strings()) return res;

		auto file = load_msgs(setup.strings, warp_missing, setup.common.verbose,
		                      setup.diag.open(moname, "rb"), setup.diag);
		if (setup.diag.has_errors()) return 1;

		if (!llname.empty()) setup.diag.open(llname);

		if (auto mo = setup.diag.source(moname);
		    !fix_attributes(file, mo, llname, setup.diag))
			return 1;

		return setup.write(
		    [&](diags::outstream& out) { return file.write(out); });
	}
}  // namespace lngs::app::make

namespace lngs::app::res {
	int call(application_setup& setup) {
		bool warp_strings = false;
		bool with_keys = false;
		std::string include;

		auto _ = [&setup](auto id) { return setup.tr.get(id); };

		setup.parser_common(lng::ARGS_APP_IN_IDL, lng::ARGS_APP_OUT_RES);
		setup.parser.set<std::true_type>(warp_strings, "w", "warp")
		    .help(_(lng::ARGS_APP_WARP_ALL_PLURAL))
		    .opt();
		setup.parser.set<std::true_type>(with_keys, "k", "keys")
		    .help(_(lng::ARGS_APP_WITH_KEY_BLOCK))
		    .opt();
		setup.parser.arg(include, "include")
		    .meta(_(lng::ARGS_APP_META_FILE))
		    .help(_(lng::ARGS_APP_ALT_INCLUDE))
		    .opt();
		setup.parser.parse();

		if (int res = setup.read_strings()) return res;

		if (include.empty()) include = setup.strings.project + ".hpp";

		auto file = make_resource(setup.strings, warp_strings, with_keys);

		return setup.write([&](diags::outstream& out) {
			return update_and_write(setup.env(out), file, include);
		});
	}
}  // namespace lngs::app::res

namespace lngs::app::freeze {
	int call(application_setup& setup) {
		setup.parser_common(lng::ARGS_APP_IN_IDL, lng::ARGS_APP_OUT_IDL);
		setup.parser.parse();

		if (int res = setup.read_strings()) return res;

		if (!freeze::freeze(setup.strings)) {
			if (setup.common.verbose) {
				auto src = setup.diag.source(setup.common.inname).position();
				setup.diag.push_back(src[diags::severity::note]
				                     << lng::ERR_NO_NEW_STRINGS);
			}
			return 0;
		}

		auto contents = setup.diag.source(setup.common.inname);
		return setup.write([&](diags::outstream& out) {
			return write(out, setup.strings, contents);
		});
	}
}  // namespace lngs::app::freeze

struct command {
	const char* name;
	const lng description;
	int (*call)(lngs::app::application_setup&);
};

command commands[] = {
    {"make", lng::ARGS_APP_DESCR_CMD_MAKE, lngs::app::make::call},
    {"pot", lng::ARGS_APP_DESCR_CMD_POT, lngs::app::pot::call},
    {"enums", lng::ARGS_APP_DESCR_CMD_ENUMS, lngs::app::enums::call},
    {"py", lng::ARGS_APP_DESCR_CMD_PY, lngs::app::py::call},
    {"res", lng::ARGS_APP_DESCR_CMD_RES, lngs::app::res::call},
    {"freeze", lng::ARGS_APP_DESCR_CMD_FREEZE, lngs::app::freeze::call},
};

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
   > vim .idl
   > git commit .idl
2. {}:
   > lngs enums
   > lngs res
   > git commit .hpp .cpp
3. {}:
   > msgfmt [optional]
   > lngs enums
   > lngs res
   > lngs make
   > git commit .hpp .cpp [optional]
4. {}:
   > msgmerge (or msginit)
   > e.g. poedit .po
   > git commit .po
5. {}:
   > msgfmt (opt)
   > lngs make
   > tar -c
)",
	           _(lng::ARGS_APP_FLOW_TITLE), _(lng::ARGS_APP_FLOW_ROLE_DEV_ADD),
	           _(lng::ARGS_APP_FLOW_ROLE_DEV_COMPILE),
	           _(lng::ARGS_APP_FLOW_ROLE_STRMGR),
	           _(lng::ARGS_APP_FLOW_ROLE_TRANSLATOR),
	           _(lng::ARGS_APP_FLOW_ROLE_DEV_RELEASE));

	std::exit(0);
}

[[noreturn]] void show_version() {
	using ver = lngs::app::build::version;
	fmt::print("lngs {}{}\n", ver::string, ver::stability);
	std::exit(0);
}

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned);
#endif
int main(int argc, XChar* argv[]) {
#ifdef _WIN32
	SetConsoleOutputCP(65001);
#endif
#ifndef LNGS_LINKED_RESOURCES
	std::optional<std::filesystem::path> redirected_share{};
#endif

	{
		auto noop = [] {};
		args::null_translator tr{};
		args::parser base{{}, args::from_main(argc, argv), &tr};
		base.usage({});
		base.provide_help(false);
		base.custom(noop, "h", "help").opt();
		base.custom(noop, "v", "version").opt();
#ifndef LNGS_LINKED_RESOURCES
		base.arg(redirected_share, "share");
#endif
		base.parse(args::parser::allow_subcommands);
	}

#ifdef LNGS_LINKED_RESOURCES
	lngs::app::application_setup setup{};
#else
	lngs::app::application_setup setup{redirected_share};
#endif

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
#ifndef LNGS_LINKED_RESOURCES
	base.custom([](std::string const&) {}, "share")
	    .meta(_(lng::ARGS_APP_META_DIR))
	    .help(fmt::format(_(lng::ARGS_APP_SHARE_REDIR),
	                      lngs::app::build::directory_info::data_dir))
	    .opt();
#endif
	auto const unparsed = base.parse(args::parser::allow_subcommands);
	if (unparsed.empty()) base.error(_s(lng::ARGS_APP_NO_COMMAND));

	auto const name = unparsed[0];
	for (auto& cmd : commands) {
		if (cmd.name != name) continue;

		auto view = setup.tr.get(cmd.description);
		setup.parser =
		    args::parser{{view.data(), view.size()}, unparsed, &setup.tr};
		setup.parser.program(base.program() + " " + setup.parser.program());

		return cmd.call(setup);
	}

	base.error(fmt::format(_(lng::ARGS_APP_UNK_COMMAND), name));
}
