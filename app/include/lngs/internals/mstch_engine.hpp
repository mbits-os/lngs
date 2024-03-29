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
#ifndef LNGS_LINKED_RESOURCES
		explicit mstch_engine(
		    std::optional<std::filesystem::path> const& redirected,
		    std::optional<std::filesystem::path> const& additional);
#else
		explicit mstch_engine(
		    std::optional<std::filesystem::path> const& additional);
#endif
		std::string load(std::string const& partial) final;
		bool need_update(const std::string& partial) const final;
		bool is_valid(std::string const& partial) const final;

	private:
		static std::tuple<bool, time_t, std::filesystem::path> stat(
		    std::filesystem::path const& root,
		    std::string const& tmplt_name);
		std::string read(std::filesystem::path const&);

#ifndef LNGS_LINKED_RESOURCES
		std::pair<bool, std::filesystem::path> stat(
		    std::string const& tmplt_name) const;

		std::filesystem::path m_installed_templates;
#ifndef NDEBUG
		std::filesystem::path m_srcdir_templates;
#endif
#endif  // !defined LNGS_LINKED_RESOURCES
		std::optional<std::filesystem::path> m_additional_templates;
	};

	std::string straighten(std::string const& str);

	struct str_transform {
		std::string (*value)(std::string const&) = straighten;
		std::string (*help)(std::string const&) = straighten;
		std::string (*plural)(std::string const&) = straighten;
		mstch::map from(idl_string const&) const;
	};

	struct mstch_env {
		diags::outstream& out;
		idl_strings const& defs;
#ifndef LNGS_LINKED_RESOURCES
		std::optional<std::filesystem::path> const& redirected;
#endif

		int write_mstch(
		    std::string const& tmplt_name,
		    mstch::map initial = {},
		    str_transform const& stringify = {},
		    std::optional<std::filesystem::path> const& additional = {}) const;
		mstch::map expand_context(mstch::map context,
		                          str_transform const& stringify = {}) const;
	};
}  // namespace lngs::app
