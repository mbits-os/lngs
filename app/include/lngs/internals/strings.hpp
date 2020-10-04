// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <lngs/file.hpp>
#include <vector>

namespace diags {
	class sources;
	class source_code;
}  // namespace diags

namespace lngs::app {
	struct idl_string {
		std::string key;
		std::string value;
		std::string help;
		std::string plural;
		int id = -2;
		int original_id = -2;
		int id_offset = -1;
	};

	struct idl_strings {
		std::string project;
		std::string version;
		std::string ns_name;
		uint32_t serial = 0;
		int serial_offset = -1;
		bool has_new = false;
		std::vector<idl_string> strings;
	};

	bool read_strings(diags::source_code in,
	                  idl_strings& str,
	                  diags::sources& diag);
	bool read_strings(const std::string& progname,
	                  const fs::path& inname,
	                  idl_strings& str,
	                  bool verbose,
	                  diags::sources& diag);
}  // namespace lngs::app
