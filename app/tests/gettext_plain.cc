#include <gtest/gtest.h>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/gettext.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/strings.hpp>
#include "diag_helper.h"
#include "strings_helper.h"

#include <optional>

extern std::filesystem::path TESTING_data_path;

namespace lngs::app::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	using namespace ::diags;
	using namespace ::diags::testing;

	enum class Result { Unmodifed = false, Warped = true };

	enum class Reporting { Quiet = false, Verbose = true };

	struct po_result {
		const std::string_view test_set;
		const std::vector<diagnostic> diags{};
		const std::map<std::string, std::string> strings{};

		friend std::ostream& operator<<(std::ostream& out,
		                                po_result const& po) {
			return out << po.test_set;
		}
	};

	struct po_load_param {
		const idl_strings strings{};
		const Result warp_missing{Result::Unmodifed};
		const Reporting verbose{Reporting::Quiet};
		const std::string_view filename{};
		const file expected{};
		const std::vector<diagnostic> msgs{};
	};

	template <typename ParamType>
	struct TestWithMapParam : TestWithParam<ParamType> {
	protected:
		template <typename Key,
		          typename Value,
		          typename Less1,
		          typename Alloc1,
		          typename Less2,
		          typename Alloc2>
		void ExpectEq(
		    const std::map<Key, Value, Less1, Alloc1>& expected_values,
		    const std::map<Key, Value, Less2, Alloc2>& actual_values) {
			for (const auto& expected : expected_values) {
				auto it = actual_values.find(expected.first);
				if (it == end(actual_values)) {
					std::remove_cv_t<
					    std::remove_reference_t<decltype(expected)>>
					    actual{};
					EXPECT_EQ(expected, actual);
					continue;
				}
				const auto& actual = *it;
				EXPECT_EQ(expected, actual);
			}

			for (const auto& actual : actual_values) {
				auto it = expected_values.find(actual.first);
				if (it == end(expected_values)) {
					std::remove_cv_t<std::remove_reference_t<decltype(actual)>>
					    expected{};
					EXPECT_EQ(expected, actual);
					continue;
				}
			}
		}
	};

	class gettext_plain : public TestWithDiagnostics<TestWithParam<po_result>> {
	public:
		static void write_text_file(std::string_view filename,
		                            std::string_view contents) {
			auto po_file = diags::fs::fopen(TESTING_data_path / filename, "w");
			po_file.store(contents.data(), contents.size());
		}
		static void SetUpTestCase() {
			if constexpr (true) {
				write_text_file("comments.po", R"(# This is just a comment...
# There is nothing else,
    # But comments
   				   # Yo!
)");
				write_text_file("A_a_b.po", R"(msgctxt "A"
msgid "a"# comment at the end of the line
msgstr		"b")");
				write_text_file("plural.po", R"(msgctxt "A"
msgid "one A"
msgid_plural "{0} As"
msgstr[0] "one B"
msgstr[1] "{0} Bs")");
				write_text_file("not-number.po", R"(msgstr[abcd] "")");
				write_text_file("escaped.po", R"(msgid "\a\b"
	"\f\n\r\t\v"
msgstr "a\gb\\f\"n\'r\?t\[\]v")");
				write_text_file("unescaped.po", R"(msgid ""
    "\a\b\f\n\r\
msgstr "a\gb\\f\"n\'r\?t\[\]v")");
				write_text_file("unfinished.po", R"(msgid "\a\b\f\n\r\t\v
msgstr "a\gb\\f\"n\'r\?t\[\]v")");
				write_text_file("not-number.po", R"(msgstr[abcd] "")");
				write_text_file("empty-index.po", R"(msgctxt[] "")");
				write_text_file("unknown-field.po", R"(something "")");
				write_text_file("word-word.po", R"(something else "")");
				write_text_file("word-string-word.po",
				                R"(msgstr "" not-end-of-line)");
			}
		}
	};

	TEST_P(gettext_plain, text) {
		auto [filename, exp_diag, expected] = GetParam();
		auto po = TESTING_data_path / filename;

		{
			auto po_file = diags::fs::fopen(po);
			if (!po_file)
				GTEST_FAIL() << "  failed to open:\n    " << po.string()
				             << "\n  canonical:\n    "
				             << std::filesystem::weakly_canonical(po).string();
		}

		sources diags;
		auto data = diags.open(po.string());
		EXPECT_FALSE(gtt::is_mo(data));
		auto actual = gtt::open_po(data, diags);
		EXPECT_EQ(expected, actual);

		ExpectDiagsEq(exp_diag, diags.diagnostic_set(), data.position().token);
	}

	constexpr static location binary{};
	constexpr static auto error = binary[severity::error];
	constexpr static auto warning = binary[severity::warning];
	constexpr static auto note = binary[severity::note];

	static inline tr_string str(uint32_t key, std::string value) {
		return {key, std::move(value)};
	}

	const po_result sources[] = {
	    {"comments.po"},
	    {"A_a_b.po", {}, {{"A", "b"}}},
	    {"plural.po", {}, {{"A", "one B\000{0} Bs"s}}},
	    {"escaped.po",
	     {
	         warning << format(lng::ERR_GETTEXT_UNRECOGNIZED_ESCAPE, "g"),
	         warning << format(lng::ERR_GETTEXT_UNRECOGNIZED_ESCAPE, "["),
	         warning << format(lng::ERR_GETTEXT_UNRECOGNIZED_ESCAPE, "]"),
	     },
	     {{"\a\b\f\n\r\t\v", "agb\\f\"n\'r\?t[]v"}}},
	    {"unescaped.po",
	     {error << format(lng::ERR_EXPECTED,
	                      lng::ERR_EXPECTED_STRING,
	                      lng::ERR_EXPECTED_GOT_EOL)}},
	    {"unfinished.po",
	     {error << format(lng::ERR_EXPECTED,
	                      "`\"'",
	                      lng::ERR_EXPECTED_GOT_EOL)}},
	    {"not-number.po",
	     {
	         error << format(lng::ERR_EXPECTED,
	                         lng::ERR_EXPECTED_NUMBER,
	                         lng::ERR_EXPECTED_GOT_UNRECOGNIZED),
	         error << format(lng::ERR_EXPECTED,
	                         lng::ERR_EXPECTED_STRING,
	                         lng::ERR_EXPECTED_GOT_UNRECOGNIZED),
	     }},
	    {"empty-index.po",
	     {error << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_NUMBER, "`]'")}},
	    {"unknown-field.po",
	     {warning << format(lng::ERR_GETTEXT_UNRECOGNIZED_FIELD, "something")}},
	    {"word-word.po",
	     {
	         warning << format(lng::ERR_GETTEXT_UNRECOGNIZED_FIELD,
	                           "something"),
	         error << format(lng::ERR_EXPECTED,
	                         lng::ERR_EXPECTED_STRING,
	                         lng::ERR_EXPECTED_GOT_UNRECOGNIZED),
	     }},
	    {"word-string-word.po",
	     {error << format(lng::ERR_EXPECTED,
	                      lng::ERR_EXPECTED_EOL,
	                      lng::ERR_EXPECTED_GOT_UNRECOGNIZED)}},
	};

	INSTANTIATE_TEST_SUITE_P(sources, gettext_plain, ValuesIn(sources));
}  // namespace lngs::app::testing
