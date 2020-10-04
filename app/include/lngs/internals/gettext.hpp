// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <map>
#include <string>

namespace diags {
	class source_code;
	class sources;
}  // namespace diags
namespace gtt {
	bool is_mo(diags::source_code& src);
	std::map<std::string, std::string> open_mo(diags::source_code& src,
	                                           diags::sources& diags);
	std::map<std::string, std::string> open_po(diags::source_code& src,
	                                           diags::sources& diags);
}  // namespace gtt
