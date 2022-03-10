// Copyright (c) 2020 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <lngs/internals/strings.hpp>
#include <mstch/mstch.hpp>
#include <optional>
#include <tuple>

namespace diags {
	struct outstream;
}

namespace lngs::app {
	class mstch_engine : public mstch::cache {
	public:
		explicit mstch_engine(
		    std::optional<std::filesystem::path> const& redirected);
		std::string load(std::string const& partial) final;
		bool is_valid(std::string const& partial) final;

	private:
		static std::tuple<bool, time_t, std::filesystem::path> stat(
		    std::filesystem::path const& root,
		    std::string const& tmplt_name);
		std::pair<bool, std::filesystem::path> stat(
		    std::string const& tmplt_name);
		std::string read(std::filesystem::path const&);

		std::filesystem::path m_installed_templates;
#ifndef NDEBUG
		std::filesystem::path m_srcdir_templates;
#endif
	};

	std::string straighten(std::string const& str);

	struct str_transform {
		std::string (*value)(std::string const&) = straighten;
		std::string (*help)(std::string const&) = straighten;
		std::string (*plural)(std::string const&) = straighten;
		mstch::map from(idl_string const&) const;
	};

	int write_mstch(diags::outstream& out,
	                const idl_strings& defs,
	                std::optional<std::filesystem::path> const& redirected,
	                std::string const& tmplt_name,
	                mstch::map initial = {},
	                str_transform const& stringify = {});
}  // namespace lngs::app
