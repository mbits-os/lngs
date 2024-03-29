// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <tuple>
#include <vector>

namespace diags {
	struct outstream;
	class source_code;
	class sources;
}  // namespace diags

namespace lngs::app {
	struct file;
	struct idl_strings;
	struct mstch_env;

	std::string straighten(std::string const& str);
}  // namespace lngs::app

namespace lngs::app::pot {
	struct info {
		std::string copy{"THE PACKAGE'S COPYRIGHT HOLDER"};
		std::string first_author{"FIRST AUTHOR <EMAIL@ADDRESS>"};
		std::string title{"SOME DESCRIPTIVE TITLE"};
		int year{-1};
	};

	int year_from_template(diags::source_code file);
	int write(mstch_env const& env, const info& nfo);
}  // namespace lngs::app::pot

namespace lngs::app::enums {
	int write(mstch_env const& env, bool with_resource);
}

namespace lngs::app::py {
	int write(mstch_env const& env);
}

namespace lngs::app::make {
	file load_msgs(const idl_strings& defs,
	               bool warp_missing,
	               bool verbose,
	               diags::source_code data,
	               diags::sources& diags);
	bool fix_attributes(file& file,
	                    diags::source_code& mo_file,
	                    const std::string& ll_CCs,
	                    diags::sources& diags);
}  // namespace lngs::app::make

namespace lngs::app::res {
	file make_resource(const idl_strings& defs,
	                   bool warp_strings,
	                   bool with_keys);
	int update_and_write(mstch_env const& env,
	                     file& data,
	                     std::string_view include);
}  // namespace lngs::app::res

namespace lngs::app::freeze {
	bool freeze(idl_strings& defs);
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          diags::source_code& data);
}  // namespace lngs::app::freeze

namespace lngs::app::mustache {
	int write(mstch_env const& env,
	          std::string const& template_name,
	          std::optional<std::string> const& additional_directory,
	          std::optional<std::filesystem::path> const& debug_out);
}  // namespace lngs::app::mustache
