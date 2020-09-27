// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <string_view>

namespace lngs {
	// clang-format off
	struct version_info {
		static constexpr unsigned major = @PROJECT_VERSION_MAJOR@;
		static constexpr unsigned minor = @PROJECT_VERSION_MINOR@;
		static constexpr unsigned patch = @PROJECT_VERSION_PATCH@;
		// "-alpha", "-beta", "-rc.1", "-rc.2", and ""
		static constexpr char stability[] = "@PROJECT_VERSION_STABILITY@";
		static constexpr char string[] = "@PROJECT_VERSION@";
		static constexpr char shrt[] = "@PROJECT_VERSION_SHORT@";
		static constexpr char commit[] = "@PROJECT_VERSION_COMMIT@";
		static constexpr bool has_commit = !!*commit;
	};
	// clang-format on

	struct version_type {
		unsigned major;
		unsigned minor;
		unsigned patch;
		std::string_view stability;
		std::string_view commit;
	};

	inline constexpr version_type version{
	    version_info::major, version_info::minor, version_info::patch,
	    version_info::stability, version_info::commit};
}  // namespace lngs