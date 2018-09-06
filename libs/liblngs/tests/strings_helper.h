#pragma once
#include <lngs/strings.hpp>

namespace lngs::testing {
	template <typename Tag, typename Storage> struct tagged_t { Storage val; };
	using ProjectStr = tagged_t<struct ProjectStrTag, std::string>;
	using VersionStr = tagged_t<struct VersionStrTag, std::string>;
	using HelpStr = tagged_t<struct HelpStrTag, std::string>;
	using PluralStr = tagged_t<struct PluralStrTag, std::string>;

	struct test_string {
		int id = -2;
		std::string key;
		std::string value;
		HelpStr help;
		PluralStr plural;
		int original_id = -2;
		int id_offset = -1;

		test_string() = default;
		test_string(int id, int original, std::string key, std::string value, HelpStr help = {}, PluralStr plural = {})
			: id{ id }
			, key{ std::move(key) }
			, value{ std::move(value) }
			, help{ std::move(help) }
			, plural{ std::move(plural) }
			, original_id{ original }
		{}
		test_string(int id, std::string key, std::string value, HelpStr help = {}, PluralStr plural = {})
			: test_string{ id, id, std::move(key), std::move(value), std::move(help), std::move(plural) }
		{}
		test_string(int id, int original, std::string key, std::string value, PluralStr plural)
			: test_string{ id, original, std::move(key), std::move(value), {}, std::move(plural) }
		{}
		test_string(int id, std::string key, std::string value, PluralStr plural)
			: test_string{ id, id, std::move(key), std::move(value), {}, std::move(plural) }
		{}

		test_string&& original(int orig_id) && {
			original_id = orig_id;
			return std::move(*this);
		}
		test_string&& offset(int offset) && {
			id_offset = offset;
			return std::move(*this);
		}

		operator idl_string() && {
			return {
				std::move(key),
				std::move(value),
				std::move(help.val),
				std::move(plural.val),
				id, original_id, id_offset
			};
		}
	};

	struct test_strings {
		ProjectStr project;
		VersionStr version;
		uint32_t serial = 0;
		int serial_offset = -1;
		test_strings() = default;
		test_strings(ProjectStr project, VersionStr version, uint32_t serial, int serial_offset = -1)
			: project{ std::move(project) }
			, version{ std::move(version) }
			, serial{ serial }
			, serial_offset{ serial_offset }
		{}
		test_strings(uint32_t serial, int serial_offset = -1)
			: test_strings{ {}, {}, serial, serial_offset }
		{}
		test_strings(ProjectStr project, VersionStr version = {})
			: test_strings{ std::move(project), std::move(version), 0, -1 }
		{}
		test_strings(VersionStr version)
			: test_strings{ {}, std::move(version), 0, -1 }
		{}
		test_strings(ProjectStr project, uint32_t serial, int serial_offset = -1)
			: test_strings{ std::move(project), {}, serial, serial_offset }
		{}
		test_strings(VersionStr version, uint32_t serial, int serial_offset = -1)
			: test_strings{ {}, std::move(version), serial, serial_offset }
		{}

		template <typename ... StringType>
		idl_strings make(StringType... str) const {
			const auto has_new = ((str.original_id <= 0) || ...);
			return {
				project.val,
				version.val,
				serial,
				serial_offset,
				has_new,
				{ std::move(str)... }
			};
		}
	};
}