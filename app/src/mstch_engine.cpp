// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <sys/stat.h>
#include <diags/streams.hpp>
#include <lngs/internals/mstch_engine.hpp>
#include "build.hpp"

namespace lngs::app {
	namespace {
		std::filesystem::path get_install_dir(
		    std::optional<std::filesystem::path> const& redirected) {
			return redirected ? *redirected / "templates"
			                  : app::build::directory_info::mstch_install;
		}
	}  // namespace

	mstch_engine::mstch_engine(
	    std::optional<std::filesystem::path> const& redirected)
	    : m_installed_templates(get_install_dir(redirected))
#ifndef NDEBUG
	    , m_srcdir_templates(app::build ::directory_info::mstch_build)
#endif
	{
	}

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

	std::pair<bool, std::filesystem::path> mstch_engine::stat(
	    std::string const& tmplt_name) {
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

	std::string mstch_engine::read(std::filesystem::path const& path) {
		auto file = diags::fs::fopen(path);
		if (!file) return {};
		auto const data = file.read();
		return {reinterpret_cast<char const*>(data.data()), data.size()};
	}

	std::string mstch_engine::load(std::string const& partial) {
		auto [exists, path] = stat(partial);
		if (!exists) return {};
		return read(path);
	}

	bool mstch_engine::is_valid(std::string const& partial) {
		return stat(partial).first;
	}

	mstch::map str_transform::from(idl_string const& str) const {
		return {
		    {"key", str.key},
		    {"value", (*value)(str.value)},
		    {"help", (*help)(str.help)},
		    {"plural", (*plural)(str.plural)},
		    {"id", str.id},
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

	int write_mstch(diags::outstream& out,
	                const idl_strings& defs,
	                std::optional<std::filesystem::path> const& redirected,
	                std::string const& tmplt_name,
	                mstch::map ctx,
	                str_transform const& stringify) {
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

		mstch_engine mstch{redirected};
		out.write(mstch.render(tmplt_name, ctx));
		return 0;
	}
}  // namespace lngs::app