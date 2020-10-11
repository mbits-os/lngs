#include <gtest/gtest.h>
#include "diag_helper.h"
#include "strings_helper.h"

extern std::filesystem::path TESTING_data_path;

namespace lngs::app::testing {
	using namespace ::std::literals;
	using ::testing::Test;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	using namespace ::diags;
	using namespace ::diags::testing;

	namespace {
		constexpr const char source_filename[] = "<source>";
	}

	struct compilation_result {
		const std::string_view text;
		const std::initializer_list<diagnostic> msgs;
		bool result{false};
		const idl_strings results{};
		bool verbose{false};

		friend std::ostream& operator<<(std::ostream& o,
		                                const compilation_result& res) {
			o << '"';
			for (auto c : res.text) {
				switch (c) {
					case '"':
						o << "\\\"";
						break;
					case '\n':
						o << "\\n";
						break;
					case '\r':
						o << "\\r";
						break;
					case '\t':
						o << "    ";
						break;
					default:
						o << c;
				}
			}
			return o << '"';
		}
	};

	class read : public TestWithParam<compilation_result> {
	public:
		void strings(const idl_strings& expected, const idl_strings& actual) {
#define COMPARE_EQ(fld) EXPECT_EQ(expected.fld, actual.fld)
			COMPARE_EQ(project);
			COMPARE_EQ(ns_name);
			COMPARE_EQ(version);
			COMPARE_EQ(serial);
			COMPARE_EQ(serial_offset);
			COMPARE_EQ(has_new);
			COMPARE_EQ(strings.size());
#undef COMPARE_EQ

			{
				auto left = begin(expected.strings);
				auto left_end = end(expected.strings);
				auto right = begin(actual.strings);
				auto right_end = end(actual.strings);
				for (; left != left_end && right != right_end;
				     ++left, ++right) {
					auto const& expected_str = *left;
					auto const& actual_str = *right;

#define COMPARE_EQ(fld) EXPECT_EQ(expected_str.fld, actual_str.fld)
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
		}

		template <typename Collection>
		void messages(const Collection& expected_msgs,
		              const std::vector<diagnostic>& actual_msgs,
		              unsigned src,
		              unsigned src1 = 0) {
			if (actual_msgs.size() > expected_msgs.size()) {
				EXPECT_EQ(expected_msgs.size(), actual_msgs.size())
				    << UnexpectedDiags{actual_msgs, expected_msgs.size()};
			} else {
				EXPECT_EQ(expected_msgs.size(), actual_msgs.size());
			}

			{
				auto left = begin(actual_msgs);
				auto left_end = end(actual_msgs);
				auto right = begin(expected_msgs);
				auto right_end = end(expected_msgs);

				for (; left != left_end && right != right_end;
				     ++left, ++right) {
					auto const& actual = *left;
					auto const& expected = *right;

					if (!src1 || actual.start().token != src1) {
						EXPECT_EQ(src, actual.start().token);
					}
					EXPECT_EQ(expected.start().line, actual.start().line);
					EXPECT_EQ(expected.start().column, actual.start().column);
					EXPECT_EQ(expected.severity(), actual.severity());
					EXPECT_EQ(expected.message(), actual.message());
				}
			}
		}
	};

	class read_file : public read {
		static void fix_datapath_str(std::string& msg) {
			if (msg.compare(0, 11, "$DATA_PATH/") == 0) {
				msg = (TESTING_data_path / msg.substr(11)).string();
			}
		}

		template <typename T>
		static constexpr bool is_string_ref_v = std::is_same_v<T, std::string&>;

	public:
		static void fix_datapath(noenum::string::argument& msg) {
			std::visit(
			    [](auto& val) {
				    if constexpr (is_string_ref_v<decltype(val)>) {
					    fix_datapath_str(val);
				    } else if constexpr (std::is_same_v<
				                             decltype(val),
				                             noenum::string::formatable&>) {
					    std::visit(
					        [](auto& inner) {
						        if constexpr (is_string_ref_v<decltype(
						                          inner)>) {
							        fix_datapath_str(inner);
						        }
					        },
					        val.subject());

					    for (auto& arg : val.arguments())
						    fix_datapath(arg);
				    }
			    },
			    static_cast<noenum::string::argument::base_class&>(msg));
		}

		static void fix_datapath(std::vector<diagnostic>& set) {
			for (auto& diag : set)
				fix_datapath(diag.message());
		}
	};

	TEST_P(read, strings) {
		auto const& param = GetParam();

		sources diag;
		diag.set_contents(source_filename, param.text);

		idl_strings actual;
		auto result = read_strings(diag.source(source_filename), actual, diag);

		const auto src = diag.source(source_filename).position().token;

		EXPECT_EQ(result, param.result);
		strings(param.results, actual);
		messages(param.msgs, diag.diagnostic_set(), src);
	}

	TEST_P(read_file, strings) {
		auto const& param = GetParam();

		auto src_name = TESTING_data_path / param.text;
		sources diag;
		idl_strings actual;
		auto result =
		    read_strings("prog", src_name, actual, param.verbose, diag);

		const auto src = diag.source(src_name).position().token;
		const auto src1 = diag.source("prog").position().token;

		EXPECT_EQ(result, param.result);
		strings(param.results, actual);
		auto set = std::vector(param.msgs);
		fix_datapath(set);
		messages(set, diag.diagnostic_set(), src, src1);
	}

	const idl_strings serial_0 = test_strings(0, 1).make();
	const idl_strings serial_123 = test_strings(123, 1).make();

	const idl_strings empty_project_name =
	    test_strings(ProjectStr{"project-name"}).make();

	const idl_strings empty_version =
	    test_strings(VersionStr{"version"}).make();

	const idl_strings empty_project_name_version =
	    test_strings(ProjectStr{"project-name"}, VersionStr{"version"}).make();

	const idl_strings serial_0_project_name =
	    test_strings(ProjectStr{"project-name"}, 0, 1).make();

	const idl_strings serial_0_version =
	    test_strings(VersionStr{"version"}, 0, 1).make();

	const idl_strings serial_0_project_name_version =
	    test_strings(ProjectStr{"project-name"}, VersionStr{"version"}, 0, 1)
	        .make();

	constexpr location pos(unsigned line, unsigned col) {
		return {0u, line, col};
	}

	static const compilation_result bad[] = {
	    {{},
	     {pos(1, 1)[severity::error]
	      << format(lng::ERR_EXPECTED, "`['", lng::ERR_EXPECTED_GOT_EOF)}},
	    {R"([] strings {})",
	     {pos(1, 2)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")}},
	    {R"(strings {})",
	     {pos(1, 1)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")}},
	    {R"([project(""), version("")] strings {})",
	     {pos(1, 26)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")}},
	    {R"([project("project-name"), version("")] strings {})",
	     {pos(1, 38)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")},
	     false,
	     empty_project_name},
	    {R"([project(""), version("version")] strings {})",
	     {pos(1, 33)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")},
	     false,
	     empty_version},
	    {R"([project("project-name"), version("version")] strings {})",
	     {pos(1, 45)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")},
	     false,
	     empty_project_name_version},
	    {R"([serial(abc)] strings {})",
	     {pos(1, 9)[severity::error]
	      << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_NUMBER, "`abc'")}},
	    {R"([serial(0)] strings {)",
	     {pos(1, 22)[severity::error]
	      << format(lng::ERR_EXPECTED, "`}'", lng::ERR_EXPECTED_GOT_EOF)},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { ID = "value"; })",
	     {pos(1, 23)[severity::warning]
	          << format(lng::ERR_ATTR_MISSING, "help"),
	      pos(1, 23)[severity::error]
	          << format(lng::ERR_REQ_ATTR_MISSING, "id")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("")] ID = "value"; })",
	     {pos(1, 24)[severity::warning] << format(lng::ERR_ATTR_EMPTY, "help"),
	      pos(1, 34)[severity::error]
	          << format(lng::ERR_REQ_ATTR_MISSING, "id")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help()] ID = "value"; })",
	     {pos(1, 29)[severity::error]
	      << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_STRING, "`)'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help] ID = "value"; })",
	     {pos(1, 28)[severity::error]
	      << format(lng::ERR_EXPECTED, "`('", "`]'")},
	     false,
	     serial_0},
	    {R"(%)",
	     {pos(1, 1)[severity::error]
	      << format(lng::ERR_EXPECTED,
	                "`['",
	                lng::ERR_EXPECTED_GOT_UNRECOGNIZED)},
	     false,
	     test_strings(0, -1).make()},
	    {R"([string)",
	     {pos(1, 8)[severity::error] << format(lng::ERR_EXPECTED,
	                                           lng::ERR_EXPECTED_ID,
	                                           lng::ERR_EXPECTED_GOT_EOF)},
	     false,
	     test_strings(0, -1).make()},
	    {R"([serial(+1000)",
	     {pos(1, 14)[severity::error]
	      << format(lng::ERR_EXPECTED, "`)'", lng::ERR_EXPECTED_GOT_EOF)},
	     false,
	     test_strings(1000, -1).make()},
	    {R"("string")",
	     {pos(1, 1)[severity::error]
	      << format(lng::ERR_EXPECTED, "`['", lng::ERR_EXPECTED_GOT_STRING)},
	     false,
	     test_strings(0, -1).make()},
	    {R"(123)",
	     {pos(1, 1)[severity::error]
	      << format(lng::ERR_EXPECTED, "`['", lng::ERR_EXPECTED_GOT_NUMBER)},
	     false,
	     test_strings(0, -1).make()},
	    {R"([)",
	     {pos(1, 2)[severity::error] << format(lng::ERR_EXPECTED,
	                                           lng::ERR_EXPECTED_ID,
	                                           lng::ERR_EXPECTED_GOT_EOF)},
	     false,
	     test_strings(0, -1).make()},
	    {R"([help("not finished)",
	     {pos(1, 20)[severity::error]
	      << format(lng::ERR_EXPECTED, "`)'", lng::ERR_EXPECTED_GOT_EOF)},
	     false,
	     test_strings(0, -1).make()},
	    {R"([id(+)",
	     {pos(1, 5)[severity::error]
	      << format(lng::ERR_EXPECTED,
	                lng::ERR_EXPECTED_STRING,
	                lng::ERR_EXPECTED_GOT_UNRECOGNIZED)},
	     false,
	     test_strings(0, -1).make()},
	    {R"([dummy(value)])",
	     {pos(1, 8)[severity::error]
	      << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_STRING, "`value'")},
	     false,
	     test_strings(0, -1).make()},
	    {R"([serial(0)] ropes {})",
	     {pos(1, 13)[severity::error]
	      << format(lng::ERR_EXPECTED, "`strings'", "`ropes'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings ())",
	     {pos(1, 21)[severity::error]
	      << format(lng::ERR_EXPECTED, "`{'", "`('")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { ID="value";})",
	     {pos(1, 23)[severity::warning]
	          << format(lng::ERR_ATTR_MISSING, "help"),
	      pos(1, 23)[severity::error]
	          << format(lng::ERR_REQ_ATTR_MISSING, "id")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"})",
	     {pos(1, 35)[severity::error]
	      << format(lng::ERR_EXPECTED, "`)'", "`}'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help") })",
	     {pos(1, 37)[severity::error]
	      << format(lng::ERR_EXPECTED, "`,'", "`}'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"), id(0})",
	     {pos(1, 42)[severity::error]
	      << format(lng::ERR_EXPECTED, "`)'", "`}'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"), id(0), dummy("value") dummy2})",
	     {pos(1, 60)[severity::error]
	      << format(lng::ERR_EXPECTED, "`,'", "`dummy2'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"), id(0)]; })",
	     {pos(1, 44)[severity::error]
	      << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_ID, "`;'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"), id(0)] ID; })",
	     {pos(1, 47)[severity::error]
	      << format(lng::ERR_EXPECTED, "`='", "`;'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"), id(0)] ID=; })",
	     {pos(1, 48)[severity::error]
	      << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_STRING, "`;'")},
	     false,
	     serial_0},
	    {R"([serial(0)] strings { [help("help"), id(0)] ID="value" })",
	     {pos(1, 56)[severity::error]
	      << format(lng::ERR_EXPECTED, "`;'", "`}'")},
	     false,
	     serial_0},
	};

	INSTANTIATE_TEST_SUITE_P(bad, read, ValuesIn(bad));

	static const compilation_result empties[] = {
	    {R"([serial(0)] strings {})", {}, true, serial_0},
	    {R"([serial(123)] strings {})", {}, true, serial_123},
	    {R"([serial(0), project(""), version("")] strings {})",
	     {},
	     true,
	     serial_0},
	    {R"([serial(0), project("project-name"), version("")] strings {})",
	     {},
	     true,
	     serial_0_project_name},
	    {R"([serial(0), project(""), version("version")] strings {})",
	     {},
	     true,
	     serial_0_version},
	    {R"([serial(0), project("project-name"), version("version")] strings {})",
	     {},
	     true,
	     serial_0_project_name_version},
	    {
	        R"(// line-end comment...
[serial(0)] strings {})",
	        {},
	        true,
	        test_strings(0, 24).make()},
	    {R"([serial(0), dummy1("project-name"), dummy2(5)] strings {})",
	     {},
	     true,
	     serial_0},
	};

	INSTANTIATE_TEST_SUITE_P(empty, read, ValuesIn(empties));

	const auto basic = test_strings(0, 1);

	static const compilation_result singles[] = {
	    {R"([serial(0)] strings { [id(-1)] ID = "value"; })",
	     {pos(1, 32)[severity::warning]
	      << format(lng::ERR_ATTR_MISSING, "help")},
	     true,
	     basic.make(test_string(1001, -1, "ID", "value").offset(23))},
	    {R"([serial(0)] strings { [id(-1), help("")] ID = "value"; })",
	     {pos(1, 32)[severity::warning] << format(lng::ERR_ATTR_EMPTY, "help")},
	     true,
	     basic.make(test_string(1001, -1, "ID", "value").offset(23))},
	    {R"([serial(0)] strings { [id(-1), help("help string")] ID = "value"; })",
	     {},
	     true,
	     basic.make(test_string(1001, -1, "ID", "value", HelpStr{"help string"})
	                    .offset(23))},
	    {R"([serial(0)] strings { [id(-1), plural("values")] ID = "value"; })",
	     {pos(1, 50)[severity::warning]
	      << format(lng::ERR_ATTR_MISSING, "help")},
	     true,
	     basic.make(test_string(1001, -1, "ID", "value", PluralStr{"values"})
	                    .offset(23))},
	    {R"([serial(0)] strings { [id(-1), help("help string"), plural("values")] ID = "value"; })",
	     {},
	     true,
	     basic.make(test_string(1001,
	                            -1,
	                            "ID",
	                            "value",
	                            HelpStr{"help string"},
	                            PluralStr{"values"})
	                    .offset(23))},
	    {R"([serial(0)] strings { [id(-1), plural("values"), help("help string")] ID = "value"; })",
	     {},
	     true,
	     basic.make(test_string(1001,
	                            -1,
	                            "ID",
	                            "value",
	                            HelpStr{"help string"},
	                            PluralStr{"values"})
	                    .offset(23))},
	};

	INSTANTIATE_TEST_SUITE_P(singles, read, ValuesIn(singles));

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
	            test_string(1001,
	                        -1,
	                        "ID",
	                        "value",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(24),
	            test_string(1002,
	                        -1,
	                        "ID",
	                        "value",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(87),
	            test_string(1003, -1, "ID2", "value2", HelpStr{"help string"})
	                .offset(150))},
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
	            test_string(1002,
	                        -1,
	                        "ID",
	                        "value",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(24),
	            test_string(1001,
	                        "ID2",
	                        "value2",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(87),
	            test_string(1003, -1, "ID3", "value3", HelpStr{"help string"})
	                .offset(154))},
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
	            test_string(1010,
	                        "ID",
	                        "value",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(24),
	            test_string(1100,
	                        "ID2",
	                        "value2",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(89),
	            test_string(1101, -1, "ID3", "value3", HelpStr{"help string"})
	                .offset(156))},
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
	            test_string(1010,
	                        "ID",
	                        "value",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(24),
	            test_string(1100,
	                        "ID2",
	                        "value2",
	                        HelpStr{"help string"},
	                        PluralStr{"values"})
	                .offset(89),
	            test_string(123456789, "ID3", "value3", HelpStr{"help string"})
	                .offset(156))},
	};

	INSTANTIATE_TEST_SUITE_P(multiple, read, ValuesIn(multiple));

	static const compilation_result files[] = {
	    {"no-such.idl",
	     {pos(0, 0)[severity::error]
	      << format(lng::ERR_FILE_MISSING, "$DATA_PATH/no-such.idl"s)},
	     false,
	     {},
	     false},
	    {"no-such.idl",
	     {pos(0, 0)[severity::verbose] << "$DATA_PATH/no-such.idl"s,
	      pos(0, 0)[severity::error]
	          << format(lng::ERR_FILE_MISSING, "$DATA_PATH/no-such.idl"s)},
	     false,
	     {},
	     true},
	    {"bad.idl",
	     {pos(1, 2)[severity::error]
	      << format(lng::ERR_REQ_ATTR_MISSING, "serial")},
	     false,
	     {},
	     false},
	    {"bad.idl",
	     {pos(0, 0)[severity::verbose] << "$DATA_PATH/bad.idl"s,
	      pos(1, 2)[severity::error]
	          << format(lng::ERR_REQ_ATTR_MISSING, "serial"),
	      pos(0, 0)[severity::error]
	          << format(lng::ERR_NOT_STRINGS_FILE, "$DATA_PATH/bad.idl")},
	     false,
	     {},
	     true},
	    {"empty.idl",
	     {},
	     true,
	     test_strings(ProjectStr{"testing"}, 0, 1).make(),
	     false},
	    {"empty.idl",
	     {pos(0, 0)[severity::verbose] << "$DATA_PATH/empty.idl"s},
	     true,
	     test_strings(ProjectStr{"testing"}, 0, 1).make(),
	     true},
	};

	INSTANTIATE_TEST_SUITE_P(files, read_file, ValuesIn(files));
}  // namespace lngs::app::testing