// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <lngs/file.hpp>
#include <map>
#include <string>

namespace lngs::app {
	class source_file;
	class diagnostics;
}  // namespace lngs::app
namespace gtt {
	bool is_mo(lngs::app::source_file& src);
	std::map<std::string, std::string> open_mo(lngs::app::source_file& src,
	                                           lngs::app::diagnostics& diags);
	std::map<std::string, std::string> open_po(lngs::app::source_file& src,
	                                           lngs::app::diagnostics& diags);
}  // namespace gtt
