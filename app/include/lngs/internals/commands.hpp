// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>
#include <lngs/file.hpp>
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
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          std::optional<fs::path> const& redirected,
	          const info& nfo);
}  // namespace lngs::app::pot

namespace lngs::app::enums {
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          std::optional<fs::path> const& redirected,
	          bool with_resource);
}

namespace lngs::app::py {
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          std::optional<fs::path> const& redirected);
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
	int update_and_write(diags::outstream& out,
	                     file& data,
	                     const idl_strings& defs,
	                     std::string_view include,
	                     std::optional<fs::path> const& redirected);
}  // namespace lngs::app::res

namespace lngs::app::freeze {
	bool freeze(idl_strings& defs);
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          diags::source_code& data);
}  // namespace lngs::app::freeze
