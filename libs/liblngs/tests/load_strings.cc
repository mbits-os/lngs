#include <gtest/gtest.h>
#include "diag_helper.h"
#include "strings_helper.h"

namespace lngs::testing {
	using namespace ::std::literals;
	using ::testing::Test;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	namespace {
		constexpr const char source_filename[] = "<source>";
	}

	struct compilation_result {
		const std::string_view text;
		const std::initializer_list<expected_diagnostic> msgs;
		bool result{ false };
		const idl_strings results{};

		friend std::ostream& operator<<(std::ostream& o, const compilation_result& res) {
			o << '"';
			for (auto c : res.text) {
				switch(c) {
				case '"': o << "\\\""; break;
				case '\n': o << "\\n"; break;
				case '\r': o << "\\r"; break;
				case '\t': o << "    "; break;
				default:
					o << c;
				}
			}
			return o << '"';
		}
	};

	class read : public TestWithParam<compilation_result> {
	public:
		void compare(const argumented_string& lhs, const diagnostic_str& rhs) {
			EXPECT_EQ(
				std::holds_alternative<std::string>(lhs.value),
				rhs.use_string);

			if (rhs.use_string) {
				if (std::holds_alternative<std::string>(lhs.value))
					EXPECT_EQ(std::get<std::string>(lhs.value), rhs.str);
				else
					EXPECT_STREQ("", rhs.str) << "Unexpected lng::" << UnexpectedDiags::name(std::get<lng>(lhs.value));
			} else {
				if (std::holds_alternative<lng>(lhs.value))
					EXPECT_EQ(std::get<lng>(lhs.value), rhs.id);
				else
					EXPECT_EQ(std::get<std::string>(lhs.value), "") << "Unexpected lng::" << UnexpectedDiags::name(rhs.id);
			}

			EXPECT_EQ(lhs.args.size(), rhs.args.size());

			auto left = begin(lhs.args);
			auto left_end = end(lhs.args);
			auto right = begin(rhs.args);
			auto right_end = end(rhs.args);
			for (; left != left_end && right != right_end; ++left, ++right) {
				compare(*left, *right);
			}
		}
	};

	TEST_P(read, strings) {
		auto const& param = GetParam();

		diagnostics diag;
		diag.set_contents(source_filename, param.text);

		idl_strings output;
		auto result = read_strings(diag.source(source_filename), output, diag);

		EXPECT_EQ(result, param.result);

		//////////////////////////////////////////////////////////////////////
		// STRINGS

#define COMPARE_EQ(fld) EXPECT_EQ(output.fld, param.results.fld)
		COMPARE_EQ(project);
		COMPARE_EQ(version);
		COMPARE_EQ(serial);
		COMPARE_EQ(serial_offset);
		COMPARE_EQ(has_new);
		COMPARE_EQ(strings.size());
#undef COMPARE_EQ

		{
			auto left = begin(output.strings);
			auto left_end = end(output.strings);
			auto right = begin(param.results.strings);
			auto right_end = end(param.results.strings);
			for (; left != left_end && right != right_end; ++left, ++right) {
				auto const & lhs_str = *left;
				auto const & rhs_str = *right;

#define COMPARE_EQ(fld) EXPECT_EQ(lhs_str.fld, rhs_str.fld)
				COMPARE_EQ(key);
				COMPARE_EQ(value);
				COMPARE_EQ(help);
				COMPARE_EQ(plural);
				COMPARE_EQ(id);
				COMPARE_EQ(original_id);
				COMPARE_EQ(id_offset);
#undef COMPARE_EQ
			}
		}

		//////////////////////////////////////////////////////////////////////
		// MESSAGES

		const auto& diags = diag.diagnostic_set();
		if (diags.size() > param.msgs.size()) {
			EXPECT_EQ(diags.size(), param.msgs.size()) << UnexpectedDiags{ diags, param.msgs.size() };
		}
		else {
			EXPECT_EQ(diags.size(), param.msgs.size());
		}

		{
			auto left = begin(diags);
			auto left_end = end(diags);
			auto right = begin(param.msgs);
			auto right_end = end(param.msgs);
			auto source_file = diag.source(source_filename).position().token;

			for (; left != left_end && right != right_end; ++left, ++right) {
				auto const & lhs_diag = *left;
				auto const & rhs_diag = *right;

				EXPECT_EQ(lhs_diag.pos.token, source_file);
				EXPECT_EQ(lhs_diag.pos.line, rhs_diag.line);
				EXPECT_EQ(lhs_diag.pos.column, rhs_diag.column);
				EXPECT_EQ(lhs_diag.sev, rhs_diag.sev);

				compare(lhs_diag.message, rhs_diag.message);
			}
		}
	}

	const idl_strings serial_0 = strings(0, 1).make();
	const idl_strings serial_123 = strings(123, 1).make();

	const idl_strings empty_project_name = strings(
		ProjectStr{ "project-name" }
	).make();

	const idl_strings empty_version = strings(
		VersionStr{ "version" }
	).make();

	const idl_strings empty_project_name_version = strings(
		ProjectStr{ "project-name" },
		VersionStr{ "version" }
	).make();

	const idl_strings serial_0_project_name = strings(
		ProjectStr{ "project-name" },
		0, 1
	).make();

	const idl_strings serial_0_version = strings(
		VersionStr{ "version" },
		0, 1
	).make();

	const idl_strings serial_0_project_name_version = strings(
		ProjectStr{ "project-name" },
		VersionStr{ "version" },
		0, 1
	).make();

	static const compilation_result bad[] = {
		{
			{},
			{
				{ "", 1, 1, severity::error, str(lng::ERR_EXPECTED, str("`['"), str(lng::ERR_EXPECTED_GOT_EOF)) }
			}
		},
		{
			R"([] strings {})",
			{
				{ "", 1, 2, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("serial")) }
			}
		},
		{
			R"([project(""), version("")] strings {})",
			{
				{ "", 1, 26, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("serial")) }
			}
		},
		{
			R"([project("project-name"), version("")] strings {})",
			{
				{ "", 1, 38, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("serial")) }
			},
			false,
			empty_project_name
		},
		{
			R"([project(""), version("version")] strings {})",
			{
				{ "", 1, 33, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("serial")) }
			},
			false,
			empty_version
		},
		{
			R"([project("project-name"), version("version")] strings {})",
			{
				{ "", 1, 45, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("serial")) }
			},
			false,
			empty_project_name_version
		},
		{
			R"([serial(abc)] strings {})",
			{
				{ "", 1, 9, severity::error, str(lng::ERR_EXPECTED, str(lng::ERR_EXPECTED_STRING), str("`abc'")) }
			}
		},
		{
			R"([serial(0)] strings {)",
			{
				{ "", 1, 22, severity::error, str(lng::ERR_EXPECTED, str("`}'"), str(lng::ERR_EXPECTED_GOT_EOF)) }
			},
			false,
			serial_0
		},
		{
			R"([serial(0)] strings { ID = "value"; })",
			{
				{ "", 1, 23, severity::warning, str(lng::ERR_ATTR_MISSING, str("help")) },
				{ "", 1, 23, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("id")) },
			},
			false,
			serial_0
		},
		{
			R"([serial(0)] strings { [help("")] ID = "value"; })",
			{
				{ "", 1, 24, severity::warning, str(lng::ERR_ATTR_EMPTY, str("help")) },
				{ "", 1, 34, severity::error, str(lng::ERR_REQ_ATTR_MISSING, str("id")) },
			},
			false,
			serial_0
		},
		{
			R"([serial(0)] strings { [help()] ID = "value"; })",
			{
				{ "", 1, 29, severity::error, str(lng::ERR_EXPECTED, str(lng::ERR_EXPECTED_STRING), str("`)'")) },
			},
			false,
			serial_0
		}
	};

	INSTANTIATE_TEST_CASE_P(bad, read, ValuesIn(bad));

	static const compilation_result empties[] = {
		{
			R"([serial(0)] strings {})",
			{},
			true,
			serial_0
		},
		{
			R"([serial(123)] strings {})",
			{},
			true,
			serial_123
		},
		{
			R"([serial(0), project(""), version("")] strings {})",
			{},
			true,
			serial_0
		},
		{
			R"([serial(0), project("project-name"), version("")] strings {})",
			{},
			true,
			serial_0_project_name
		},
		{
			R"([serial(0), project(""), version("version")] strings {})",
			{},
			true,
			serial_0_version
		},
		{
			R"([serial(0), project("project-name"), version("version")] strings {})",
			{},
			true,
			serial_0_project_name_version
		},
	};

	INSTANTIATE_TEST_CASE_P(empty, read, ValuesIn(empties));

	const auto basic = strings(0, 1);

	static const compilation_result singles[] = {
		{
			R"([serial(0)] strings { [id(-1)] ID = "value"; })",
			{
				{ "", 1, 32, severity::warning, str(lng::ERR_ATTR_MISSING, str("help")) },
			},
			true,
			basic.make(string(1001, -1, "ID", "value").offset(23))
		},
		{
			R"([serial(0)] strings { [id(-1), help("")] ID = "value"; })",
			{
				{ "", 1, 32, severity::warning, str(lng::ERR_ATTR_EMPTY, str("help")) },
			},
			true,
			basic.make(string(1001, -1, "ID", "value").offset(23))
		},
		{
			R"([serial(0)] strings { [id(-1), help("help string")] ID = "value"; })",
			{},
			true,
			basic.make(string(1001, -1, "ID", "value", HelpStr{ "help string" }).offset(23))
		},
		{
			R"([serial(0)] strings { [id(-1), plural("values")] ID = "value"; })",
			{
				{ "", 1, 50, severity::warning, str(lng::ERR_ATTR_MISSING, str("help")) },
			},
			true,
			basic.make(string(1001, -1, "ID", "value", PluralStr{ "values" }).offset(23))
		},
		{
			R"([serial(0)] strings { [id(-1), help("help string"), plural("values")] ID = "value"; })",
			{},
			true,
			basic.make(string(1001, -1, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(23))
		},
		{
			R"([serial(0)] strings { [id(-1), plural("values"), help("help string")] ID = "value"; })",
			{},
			true,
			basic.make(string(1001, -1, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(23))
		},
	};

	INSTANTIATE_TEST_CASE_P(singles, read, ValuesIn(singles));

	static const compilation_result multiple[] = {
		{
			R"([serial(0)]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(-1), plural("values"), help("help string")] ID = "value";
	[id(-1), help("help string")] ID2 = "value2";
})",
			{},
			true,
			basic.make(
				string(1001, -1, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(24),
				string(1002, -1, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(87),
				string(1003, -1, "ID2", "value2", HelpStr{ "help string" }).offset(150)
			)
		},
		{
			R"([serial(0)]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(1001), plural("values"), help("help string")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
})",
			{},
			true,
			basic.make(
				string(1002, -1, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(24),
				string(1001, "ID2", "value2", HelpStr{ "help string" }, PluralStr{ "values" }).offset(87),
				string(1003, -1, "ID3", "value3", HelpStr{ "help string" }).offset(154)
			)
		},
		{
			R"([serial(0)]
strings {
	[id(1010), help("help string"), plural("values")] ID = "value";
	[id(1100), plural("values"), help("help string")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
})",
			{},
			true,
			basic.make(
				string(1010, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(24),
				string(1100, "ID2", "value2", HelpStr{ "help string" }, PluralStr{ "values" }).offset(89),
				string(1101, -1, "ID3", "value3", HelpStr{ "help string" }).offset(156)
			)
		},
		{
			R"([serial(0)]
strings {
	[id(1010), help("help string"), plural("values")] ID = "value";
	[id(1100), plural("values"), help("help string")] ID2 = "value2";
	[id(0123456789), help("help string")] ID3 = "value3";
})",
			{},
			true,
			basic.make(
				string(1010, "ID", "value", HelpStr{ "help string" }, PluralStr{ "values" }).offset(24),
				string(1100, "ID2", "value2", HelpStr{ "help string" }, PluralStr{ "values" }).offset(89),
				string(123456789, "ID3", "value3", HelpStr{ "help string" }).offset(156)
			)
		},
	};

	INSTANTIATE_TEST_CASE_P(multiple, read, ValuesIn(multiple));
}