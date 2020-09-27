// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <lngs/file.hpp>
#include <lngs/lngs_base.hpp>
#include <map>
#include <vector>

namespace lngs::app {
	struct tr_string {
		string_key key{};
		std::string value{};

		tr_string(uint32_t id, std::string val) : value(std::move(val)) {
			key.id = id;
			key.length = static_cast<uint32_t>(value.length());
		}
	};

	struct idl_string;
	class source_file;
	class diagnostics;

	std::string warp(const std::string& s);
	std::map<std::string, std::string, std::less<>> attrGTT(
	    std::string_view attrs);
	std::vector<tr_string> attributes(
	    const std::map<std::string, std::string>& gtt);
	std::vector<tr_string> translations(
	    const std::map<std::string, std::string>& gtt,
	    const std::vector<idl_string>& strings,
	    bool warp_missing,
	    bool verbose,
	    source_file& src,
	    diagnostics& diags);
	bool ll_CC(source_file is,
	           diagnostics& diags,
	           std::map<std::string, std::string>& langs);

	struct outstream;
	struct file {
		uint32_t serial{0};
		std::vector<tr_string> attrs{};
		std::vector<tr_string> strings{};
		std::vector<tr_string> keys{};

		int write(outstream& os);
	};
}  // namespace lngs::app
