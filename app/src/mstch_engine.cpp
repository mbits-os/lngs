// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <sys/stat.h>
#include <diags/streams.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/mstch_engine.hpp>
#include "build.hpp"
#ifdef LNGS_LINKED_RESOURCES
#include "templates.hpp"
#endif

using namespace std::literals;

namespace lngs::app {
	namespace {
#ifndef LNGS_LINKED_RESOURCES
		std::filesystem::path get_install_dir(
		    std::optional<std::filesystem::path> const& redirected) {
			return redirected ? *redirected / "templates"
			                  : app::build::directory_info::mstch_install;
		}
#endif
	}  // namespace

#ifndef LNGS_LINKED_RESOURCES
	mstch_engine::mstch_engine(
	    std::optional<std::filesystem::path> const& redirected,
	    std::optional<std::filesystem::path> const& additional)
	    : m_installed_templates(get_install_dir(redirected))
#ifndef NDEBUG
	    , m_srcdir_templates(app::build::directory_info::mstch_build)
#endif
	    , m_additional_templates{additional} {
	}

	std::pair<bool, std::filesystem::path> mstch_engine::stat(
	    std::string const& tmplt_name) const {
		if (m_additional_templates) {
			auto [additional_exists, additional, additional_path] =
			    stat(*m_additional_templates, tmplt_name);
			if (additional_exists) return {true, additional_path};
		}

		auto [installed_exists, installed, installed_path] =
		    stat(m_installed_templates, tmplt_name);
#ifndef NDEBUG
		auto [srcdir_exists, srcdir, srcdir_path] =
		    stat(m_srcdir_templates, tmplt_name);

		if (!installed_exists || installed < srcdir)
			return {srcdir_exists, srcdir_path};
#endif
		return {installed_exists, installed_path};
	}
#else   // !defined LNGS_LINKED_RESOURCES
	mstch_engine::mstch_engine(
	    std::optional<std::filesystem::path> const& additional)
	    : m_additional_templates{additional} {}
#endif  // !defined LNGS_LINKED_RESOURCES

	std::tuple<bool, time_t, std::filesystem::path> mstch_engine::stat(
	    std::filesystem::path const& root,
	    std::string const& tmplt_name) {
		std::error_code ec;
		auto path = root / (tmplt_name + ".mustache");

		struct stat st {};
		if (::stat(path.string().c_str(), &st)) {
			return {false, {}, {}};
		}
		return {true, st.st_mtime, std::move(path)};
	}

	std::string mstch_engine::read(std::filesystem::path const& path) {
		auto file = diags::fs::fopen(path);
		if (!file) return {};
		auto const data = file.read();
		return {reinterpret_cast<char const*>(data.data()), data.size()};
	}

	std::string mstch_engine::load(std::string const& partial) {
#ifdef LNGS_LINKED_RESOURCES
		if (m_additional_templates) {
			auto [additional_exists, additional, additional_path] =
			    stat(*m_additional_templates, partial);
			if (additional_exists) return read(additional_path);
		}

		auto const data = templates::get_resource(partial);
		if (!data) return {};
		return {data->data(), data->size()};
#else
		auto [exists, path] = stat(partial);
		if (!exists) return {};
		return read(path);
#endif
	}

	bool mstch_engine::need_update(const std::string&) const {
		return false;
	}

	bool mstch_engine::is_valid(std::string const& partial) const {
#ifdef LNGS_LINKED_RESOURCES
		if (m_additional_templates) {
			auto result = stat(*m_additional_templates, partial);
			if (std::get<0>(result)) return true;
		}

		return !!templates::get_resource(partial);
#else
		return stat(partial).first;
#endif
	}

	namespace {
		inline char lower(char c) {
			return static_cast<char>(
			    std::tolower(static_cast<unsigned char>(c)));
		}
		inline char upper(char c) {
			return static_cast<char>(
			    std::toupper(static_cast<unsigned char>(c)));
		}
		std::string snake_case(std::string_view key) {
			std::string result{};
			result.reserve(key.size());
			for (auto c : key) {
				result.push_back(lower(c));
			}
			return result;
		}

		std::string MACRO_CASE(std::string_view key) {
			std::string result{};
			result.reserve(key.size());
			for (auto c : key) {
				result.push_back(upper(c));
			}
			return result;
		}

		std::string kebab_case(std::string_view key) {
			std::string result{};
			result.reserve(key.size());
			for (auto c : key) {
				result.push_back(c == '_' ? '-' : lower(c));
			}
			return result;
		}

		std::string camelOrPascal(std::string_view key, bool pascal) {
			bool new_word{pascal};

			std::string result{};
			result.reserve(key.size());
			for (auto c : key) {
				if (c == '_') {
					new_word = true;
					continue;
				}
				result.push_back(new_word ? upper(c) : lower(c));
				new_word = false;
			}
			return result;
		}

		std::string PascalCase(std::string_view key) {
			return camelOrPascal(key, true);
		}

		std::string camelCase(std::string_view key) {
			return camelOrPascal(key, false);
		}
	}  // namespace

	mstch::map str_transform::from(idl_string const& str) const {
		return {
		    {"key", str.key},
		    {"value", (*value)(str.value)},
		    {"help", (*help)(str.help)},
		    {"plural", (*plural)(str.plural)},
		    {"id", static_cast<long long>(str.id)},
		    {
		        "case",
		        mstch::map{{"snake", snake_case(str.key)},
		                   {"macro", MACRO_CASE(str.key)},
		                   {"kebab", kebab_case(str.key)},
		                   {"pascal", PascalCase(str.key)},
		                   {"camel", camelCase(str.key)}},
		    },
		    {
		        "warpped",
		        mstch::map{{"value", lngs::app::warp((*value)(str.value))},
		                   {"plural", lngs::app::warp((*plural)(str.plural))}},
		    },
		};
	}

	void append(mstch::map& map, std::string const& key, mstch::node&& value) {
		auto lb = map.lower_bound(key);
		if (lb != map.end() && lb->first == key) {
			lb->second = std::move(value);
		} else {
			map.insert(lb, mstch::map::value_type{key, std::move(value)});
		}
	}

	int mstch_env::write_mstch(
	    std::string const& tmplt_name,
	    mstch::map ctx,
	    str_transform const& stringify,
	    std::optional<std::filesystem::path> const& additional) const {
#ifdef LNGS_LINKED_RESOURCES
		mstch_engine mstch{additional};
#else
		mstch_engine mstch{redirected, additional};
#endif
		out.write(mstch.render(tmplt_name,
		                       expand_context(std::move(ctx), stringify)));
		return 0;
	}

	mstch::map mstch_env::expand_context(mstch::map ctx,
	                                     str_transform const& stringify) const {
		mstch::array singular, plural, strings;

		mstch::array* refs[] = {&plural, &singular};
		for (auto& str : defs.strings) {
			auto& ref = *refs[str.plural.empty()];
			ref.push_back(stringify.from(str));
		}

		{
			std::vector<std::string> ids;
			ids.reserve(defs.strings.size());
			transform(begin(defs.strings), end(defs.strings),
			          back_inserter(ids), [](auto& str) { return str.key; });
			sort(begin(ids), end(ids));

			for (auto& id : ids) {
				auto it = find_if(begin(defs.strings), end(defs.strings),
				                  [&](auto& str) { return str.key == id; });

				auto& str = *it;
				strings.push_back(stringify.from(str));
			}
		}

		append(ctx, "project", defs.project);
		append(ctx, "version", defs.version);
		append(ctx, "ns_name", defs.ns_name);
		append(ctx, "serial", std::to_string(defs.serial));

		append(ctx, "with_singular", !singular.empty());
		append(ctx, "with_plural", !plural.empty());
		append(ctx, "with_strings", !strings.empty());

		append(ctx, "singular", std::move(singular));
		append(ctx, "plural", std::move(plural));
		append(ctx, "strings", std::move(strings));

		return ctx;
	}
}  // namespace lngs::app