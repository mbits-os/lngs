// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>
#include <lngs/file.hpp>
#include <tuple>
#include <vector>

namespace lngs::app {
	struct file;
	struct outstream;
	struct idl_strings;
	class source_file;
	class diagnostics;

	std::string straighten(std::string str);
}  // namespace lngs::app

namespace lngs::app::pot {
	struct info {
		std::string copy{"THE PACKAGE'S COPYRIGHT HOLDER"};
		std::string first_author{"FIRST AUTHOR <EMAIL@ADDRESS>"};
		std::string title{"SOME DESCRIPTIVE TITLE"};
	};

	int write(outstream& out, const idl_strings& defs, const info& nfo);
}  // namespace lngs::app::pot

namespace lngs::app::enums {
	int write(outstream& out, const idl_strings& defs, bool with_resource);
}

namespace lngs::app::py {
	int write(outstream& out, const idl_strings& defs);
}

namespace lngs::app::make {
	file load_msgs(const idl_strings& defs,
	               bool warp_missing,
	               bool verbose,
	               source_file data,
	               diagnostics& diags);
	bool fix_attributes(file& file,
	                    source_file& mo_file,
	                    const std::string& ll_CCs,
	                    diagnostics& diags);
}  // namespace lngs::app::make

namespace lngs::app::res {
	file make_resource(const idl_strings& defs,
	                   bool warp_strings,
	                   bool with_keys);
	int update_and_write(outstream& out,
	                     file& data,
	                     std::string_view include,
	                     std::string_view ns_name);
}  // namespace lngs::app::res

namespace lngs::app::freeze {
	bool freeze(idl_strings& defs);
	int write(outstream& out, const idl_strings& defs, source_file& data);
}  // namespace lngs::app::freeze
