#include <gtest/gtest.h>
#include <lngs/file.hpp>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/gettext.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/strings.hpp>
#include "diag_helper.h"
#include "strings_helper.h"

#include <optional>

extern fs::path TESTING_data_path;

namespace lngs::app::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class Result {
		Unmodifed = false,
		Warped = true
	};

	enum class Reporting {
		Quiet = false,
		Verbose = true
	};

	struct mo_result {
		const std::string_view test_set;
		const std::vector<diagnostic> diags{};
		const std::map<std::string, std::string> strings{};
		const bool expect_file = true;
	};

	struct attr_result {
		const std::string_view input;
		const std::map<std::string, std::string> expected{};
	};

	using opt_attr = std::optional<std::string_view>;
	struct attrs {
		const std::string_view input;
		const opt_attr culture, language, plurals;
	};

	struct ll_cc {
		const std::string_view input;
		const std::map<std::string, std::string> expected;
		const std::vector<diagnostic> msgs{};
		const bool success = true;
		const bool create_file = true;
	};

	struct extract {
		const std::map<std::string, std::string> gtt{};
		const std::vector<idl_string> strings{};
		const Result warp_missing{ Result::Unmodifed };
		const Reporting verbose{ Reporting::Quiet };
		const std::vector<tr_string> expected{};
		const std::vector<diagnostic> msgs{};
	};

	struct mo_load_param {
		const idl_strings strings{};
		const Result warp_missing{ Result::Unmodifed };
		const Reporting verbose{ Reporting::Quiet };
		const std::string_view filename{};
		const file expected{};
		const std::vector<diagnostic> msgs{};
	};

	struct mo_fix_param {
		std::optional<std::string> attr_culture{};
		std::string ll_cc{};
		std::vector<tr_string> expected{};
		bool expected_result{ true };
		const std::vector<diagnostic> msgs{};
	};

	template <typename ParamType>
	struct TestWithMapParam : TestWithParam<ParamType> {
	protected:
		template <typename Key, typename Value,
			typename Less1, typename Alloc1,
			typename Less2, typename Alloc2>
		void ExpectEq(const std::map<Key, Value, Less1, Alloc1>& expected_values,
			const std::map<Key, Value, Less2, Alloc2>& actual_values)
		{
			for (const auto& expected : expected_values) {
				auto it = actual_values.find(expected.first);
				if (it == end(actual_values)) {
					std::remove_cv_t<std::remove_reference_t<decltype(expected)>> actual{};
					EXPECT_EQ(expected, actual);
					continue;
				}
				const auto& actual = *it;
				EXPECT_EQ(expected, actual);
			}

			for (const auto& actual : actual_values) {
				auto it = expected_values.find(actual.first);
				if (it == end(expected_values)) {
					std::remove_cv_t<std::remove_reference_t<decltype(actual)>> expected{};
					EXPECT_EQ(expected, actual);
					continue;
				}
			}
		}
	};

	class gettext : public TestWithDiagnostics<TestWithParam<mo_result>> {
	public:
		template <typename T>
		static void write(fs::file& mo_file, const T& data) {
			mo_file.store(&data, sizeof(data));
		}

		static uint32_t reverse(uint32_t val) {
			return ((val & 0xFF) << 24) |
				(((val >> 8) & 0xFF) << 16) |
				(((val >> 16) & 0xFF) << 8) |
				((val >> 24) & 0xFF);
		}

		static uint32_t direct(uint32_t val) {
			return val;
		}

		static void be(fs::file& mo_file, uint32_t val) {
			write(mo_file, reverse(val));
		}

		static void le(fs::file& mo_file, uint32_t val) {
			write(mo_file, direct(val));
		}

		template <typename Pred>
		static void write_mo_head(fs::file& mo_file, uint32_t count, uint32_t originals, uint32_t translations, uint32_t hash_size, uint32_t hash_offset, Pred store, uint32_t rev = 0u) {
			write(mo_file, store(0x950412deU));
			write(mo_file, store(rev));
			write(mo_file, store(count));
			write(mo_file, store(originals));
			write(mo_file, store(translations));
			write(mo_file, store(hash_size));
			write(mo_file, store(hash_offset));
		}

		static void SetUpTestCase() {
			if constexpr (false) {
				auto mo_file = fs::fopen(TESTING_data_path / "truncated.mo", "wb");
				mo_file = fs::fopen(TESTING_data_path / "zero.mo", "wb");
				write_mo_head(mo_file, 0, 0, 0, 0, 0, reverse);

				mo_file = fs::fopen(TESTING_data_path / "empty_BE.mo", "wb");
				write_mo_head(mo_file, 0, 28, 28, 0, 28, reverse);
				mo_file = fs::fopen(TESTING_data_path / "empty_LE.mo", "wb");
				write_mo_head(mo_file, 0, 28, 28, 0, 28, direct);

				mo_file = fs::fopen(TESTING_data_path / "empty_rev_nonzero_BE.mo", "wb");
				write_mo_head(mo_file, 0, 28, 28, 0, 28, reverse, 0x11223344);
				mo_file = fs::fopen(TESTING_data_path / "empty_rev_nonzero_LE.mo", "wb");
				write_mo_head(mo_file, 0, 28, 28, 0, 28, direct, 0x11223344);

				mo_file = fs::fopen(TESTING_data_path / "ab_BE.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);
				be(mo_file, 1u);
				be(mo_file, 44u + 2u);
				constexpr const char a[] = "a";
				constexpr const char b[] = "b";
				mo_file.store(a, sizeof(a));
				mo_file.store(b, sizeof(b));

				mo_file = fs::fopen(TESTING_data_path / "ab_LE.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 0, 44, direct);
				le(mo_file, 1u);
				le(mo_file, 44u);
				le(mo_file, 1u);
				le(mo_file, 44u + 2u);
				mo_file.store(a, sizeof(a));
				mo_file.store(b, sizeof(b));

				mo_file = fs::fopen(TESTING_data_path / "not_asciiz.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);
				be(mo_file, 1u);
				be(mo_file, 44u + 2u);
				constexpr const char noasciiz[] = { 'a', 'b' };
				mo_file.store(noasciiz, sizeof(noasciiz));

				mo_file = fs::fopen(TESTING_data_path / "string_missing.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);
				be(mo_file, 1u);
				be(mo_file, 44u + 2u);
				mo_file.store(a, sizeof(a));

				mo_file = fs::fopen(TESTING_data_path / "within_hash.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 2, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);

				mo_file = fs::fopen(TESTING_data_path / "over_the_top.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);

				mo_file = fs::fopen(TESTING_data_path / "no_space_for_0.mo", "wb");
				write_mo_head(mo_file, 1, 28, 36, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);
				be(mo_file, 1u);
				be(mo_file, 44u);
				mo_file.store(a, 1);

				mo_file = fs::fopen(TESTING_data_path / "no_space_for_strings_1.mo", "wb");
				write_mo_head(mo_file, 2, 28, 36, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);
				be(mo_file, 1u);
				be(mo_file, 44u);
				mo_file.store(a, sizeof(a));
				mo_file.store(b, sizeof(b));

				mo_file = fs::fopen(TESTING_data_path / "no_space_for_strings_2.mo", "wb");
				write_mo_head(mo_file, 2, 28, 44, 0, 44, reverse);
				be(mo_file, 1u);
				be(mo_file, 44u);
				be(mo_file, 1u);
				be(mo_file, 44u);
				mo_file.store(a, sizeof(a));
				mo_file.store(b, sizeof(b));
			}
		}
	};

	TEST_P(gettext, text) {
		auto[filename, exp_diag, expected, expect_file] = GetParam();
		auto mo = TESTING_data_path / filename;

		if (expect_file) {
			auto mo_file = fs::fopen(mo, "rb");
			if (!mo_file)
				GTEST_FAIL() << "  failed to open:\n    " << mo.string()
				<< "\n  canonical:\n    " << fs::weakly_canonical(mo).string();
		}
		else {
			auto mo_file = fs::fopen(mo, "rb");
			if (mo_file)
				GTEST_FAIL() << "  file exists:\n    " << mo.string()
				<< "\n  canonical:\n    " << fs::weakly_canonical(mo).string()
				<< "\n  expected:\n    no file available";
		}

		diagnostics diags;
		auto data = diags.open(mo.string(), "rb");
		auto actual = gtt::open(data, diags);
		EXPECT_EQ(expected, actual);

		ExpectDiagsEq(exp_diag, diags.diagnostic_set(), data.position().token);
	}

	struct gtt_attr_mo : public TestWithMapParam<attr_result> {};

	TEST_P(gtt_attr_mo, parse) {
		auto[input, expected] = GetParam();
		auto actual = attrGTT(input);
		ExpectEq(expected, actual);
	}

	struct gtt_attr_extr : public TestWithParam<attrs> {};

	TEST_P(gtt_attr_extr, parse) {
		auto[input, culture, language, plurals] = GetParam();
		std::map<std::string, std::string> attrs{ { "", std::string{input} } };
		auto result = app::attributes(attrs);

		size_t expected_attrs =
			(culture ? 1u : 0u) +
			(language ? 1u : 0u) +
			(plurals ? 1u : 0u);

		EXPECT_EQ(expected_attrs, result.size());

		for (auto const& actual : result) {
			switch (actual.key.id) {
#define EXPECT_ATTR(NAME, var) \
			NAME: \
				EXPECT_TRUE(var) << "Actual attribute value: " << actual.value; \
				if (var) { EXPECT_EQ(*var, actual.value); } \
				break

			case EXPECT_ATTR(ATTR_CULTURE, culture);
			case EXPECT_ATTR(ATTR_LANGUAGE, language);
			case EXPECT_ATTR(ATTR_PLURALS, plurals);
			default:
				GTEST_FAIL()
					<< " Unknown key: " << actual.key.id << "\n"
					<< " With value:  " << actual.value;
			}
		}
	}

	struct gtt_attr_llCC : public TestWithDiagnostics<TestWithMapParam<ll_cc>> {};

	TEST_P(gtt_attr_llCC, parse) {
		auto[input, expected, msgs, success, create_file] = GetParam();
		std::map<std::string, std::string> actual;
		diagnostics diag;
		if (create_file)
			diag.set_contents("<source>", input);
		auto result = ll_CC(diag.source("<source>"), diag, actual);
		ExpectEq(expected, actual);
		EXPECT_EQ(success, result);
		auto src = diag.source("<source>").position().token;
		ExpectDiagsEq(msgs, diag.diagnostic_set(), src);
	}

	struct gtt_extract : public TestWithDiagnostics<TestWithMapParam<extract>> {};

	TEST_P(gtt_extract, translations) {
		auto[gtt, strings, warp, verbose, expected, msgs] = GetParam();
		diagnostics diag;
		auto src = diag.source("");
		auto actual = app::translations(gtt, strings, warp == Result::Warped, verbose == Reporting::Verbose, src, diag);
		EXPECT_EQ(expected.size(), actual.size());
		auto size = std::min(expected.size(), actual.size());
		for (size_t i = 0; i < size; ++i) {
			auto const& exp_str = expected[i];
			auto const& act_str = actual[i];
			EXPECT_EQ(exp_str.key.id, act_str.key.id);
			EXPECT_EQ(exp_str.value, act_str.value);
		}

		ExpectDiagsEq(msgs, diag.diagnostic_set(), src.position().token);
	}

	struct mo_load : public TestWithDiagnostics<TestWithParam<mo_load_param>> {
		void ExpectEq(const std::vector<tr_string>& expected, const std::vector<tr_string>& actual, const char* coll) {
			EXPECT_EQ(expected.size(), actual.size()) << "Collection: " << coll;
			auto size = std::min(expected.size(), actual.size());
			for (size_t i = 0; i < size; ++i) {
				auto const& exp_str = expected[i];
				auto const& act_str = actual[i];
				EXPECT_EQ(exp_str.key.id, act_str.key.id);
				EXPECT_EQ(exp_str.value, act_str.value);
			}
		}
	};

	TEST_P(mo_load, translations) {
		auto[strings, warp, verbose, filename, expected, msgs] = GetParam();

		auto mo = TESTING_data_path / filename;

		diagnostics diag;
		auto actual = make::load_mo(strings,
			warp == Result::Warped,
			verbose == Reporting::Verbose,
			diag.open(mo.string()), diag);

		EXPECT_EQ(expected.serial, actual.serial);
		ExpectEq(expected.attrs, actual.attrs, "attr");
		ExpectEq(expected.strings, actual.strings, "strings");
		ExpectEq(expected.keys, actual.keys, "keys");
		ExpectDiagsEq(msgs, diag.diagnostic_set(), diag.source(mo.string()).position().token);
	}

	struct mo_fix : public TestWithDiagnostics<TestWithParam<mo_fix_param>> {};

	TEST_P(mo_fix, attributes) {
		auto[attr_culture, ll_cc, expected, expected_result, msgs] = GetParam();

		file actual;
		if (attr_culture)
			actual.attrs.emplace_back(ATTR_CULTURE, *attr_culture);

		diagnostics diag;
		auto mo = diag.source("");

		fs::path llcc;
		if (!ll_cc.empty()) {
			llcc = TESTING_data_path;
			llcc /= ll_cc;
			diag.open(llcc.string());
		}
		auto actual_result = make::fix_attributes(actual, mo, llcc.string(), diag);

		EXPECT_EQ(expected_result, actual_result);

		EXPECT_EQ(expected.size(), actual.attrs.size());
		auto size = std::min(expected.size(), actual.attrs.size());
		for (size_t i = 0; i < size; ++i) {
			auto const& exp_str = expected[i];
			auto const& act_str = actual.attrs[i];
			EXPECT_EQ(exp_str.key.id, act_str.key.id);
			EXPECT_EQ(exp_str.value, act_str.value);
		}
		ExpectDiagsEq(msgs, diag.diagnostic_set(), mo.position().token);
	}

	constexpr static const location binary{};
	constexpr static const auto error = binary[severity::error];
	constexpr static const auto warning = binary[severity::warning];
	constexpr static const auto note = binary[severity::note];

	static inline tr_string str(uint32_t key, std::string value) {
		return { key, std::move(value) };
	}

	const mo_result sources[] = {
		{ "no-such.mo", { error << lng::ERR_FILE_NOT_FOUND }, { }, false },
		{ "truncated.mo" },
		{ "zero.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_BLOCKS_OVERLAP)
		} },
		{ "empty_BE.mo" },
		{ "empty_LE.mo" },
		{ "empty_rev_nonzero_BE.mo" },
		{ "empty_rev_nonzero_LE.mo" },
		{ "ab_BE.mo", { },  { { "a", "b" } } },
		{ "ab_LE.mo", { },  { { "a", "b" } } },
		{ "not_asciiz.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_NOT_ASCIIZ)
		} },
		{ "string_missing.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_FILE_TRUNCATED)
		} },
		{ "within_hash.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_STRING_OUTSIDE)
		} },
		{ "over_the_top.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_FILE_TRUNCATED)
		} },
		{ "no_space_for_0.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_NOT_ASCIIZ)
		} },
		{ "no_space_for_strings_1.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_FILE_TRUNCATED)
		} },
		{ "no_space_for_strings_2.mo", {
			(error << lng::ERR_GETTEXT_FORMAT)
				.with(note << lng::ERR_GETTEXT_FILE_TRUNCATED)
		} },
	};

	INSTANTIATE_TEST_SUITE_P(sources, gettext, ValuesIn(sources));

	static const attr_result attributes[] = {
		{},
		{ "a\nb\n" },
		{
			"a:\nb:\n", { {"a", ""}, {"b", ""} }
		},
		{
			"a\nb:v\n", { {"b", "v"} }
		},
		{
			":x\n:y\n", { {"", "y"} }
		},
		{
			"a:x\nb:y\n", { {"b", "y"}, { "a", "x" } }
		},
		{
			"a:x\nb:y", { {"b", "y"}, { "a", "x" } }
		},
	};

	INSTANTIATE_TEST_SUITE_P(attributes, gtt_attr_mo, ValuesIn(attributes));

	static const attrs gtt_attribs[] = {
		{},
		{ "Language:en\nPlural-Forms:foo", { "en" }, { }, { "foo" } },
		{ "Language:en_US\nPlural-Forms:foo", { "en-US" }, { }, { "foo" } },
		{ "Language:en_US.UTF-8\nPlural-Forms:foo", { "en-US" }, { }, { "foo" } },
		{
			""
			"Project-Id-Version: language-selector\n"
			"Report-Msgid-Bugs-To: \n"
			"POT-Creation-Date: 2016-04-05 09:43+0000\n"
			"PO-Revision-Date: 2014-05-29 06:39+0000\n"
			"Last-Translator: Yan Palaznik <pivs@tut.by>\n"
			"Language-Team: Belarusian <be@li.org>\n"
			"MIME-Version: 1.0\n"
			"Content-Type: text/plain; charset=UTF-8\n"
			"Content-Transfer-Encoding: 8bit\n"
			"Plural-Forms: nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n"
			"%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\n"
			"X-Launchpad-Export-Date: 2016-04-13 22:04+0000\n"
			"X-Generator: Launchpad (build 17990)\n"
			"Language: be\n", {"be"}, {}, {"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);"}
		}
	};

	INSTANTIATE_TEST_SUITE_P(attributes, gtt_attr_extr, ValuesIn(gtt_attribs));

	static const ll_cc gtt_llcc[] = {
		{},
		{ {}, {}, { error << lng::ERR_FILE_NOT_FOUND }, false, false },
		{ "label value\nlabel-2 second value   ", { {"label","value"}, {"label-2","second value"} } },
		{ "   label value   \nlabel-2 second value", { {"label","value"}, {"label-2","second value"} } },
		{
			"label\nvalue",
			{
			},
			{
				error << arg(lng::ERR_UNANMED_LOCALE, "label")
			},
			false
		},
	};

	INSTANTIATE_TEST_SUITE_P(contents, gtt_attr_llCC, ValuesIn(gtt_llcc));

	static const extract translations[] = {
		{
			{ {"A", "a"}, {"B", "b"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Unmodifed,
			Reporting::Quiet,
			{ str(1001, "a"), str(1002, "b") }
		},
		{
			{ {"A", "a"}, {"B", "b"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Unmodifed,
			Reporting::Verbose,
			{ str(1001, "a"), str(1002, "b") },
			{
				warning << arg(lng::ERR_MSGS_TRANSLATION_MISSING, "C")
			}
		},
		{
			{ {"A", "a"}, {"B", "b"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Warped,
			Reporting::Verbose,
			{ str(1001, "a"), str(1002, "b"), str(1003, u8"\u0225") },
			{
				warning << arg(lng::ERR_MSGS_TRANSLATION_MISSING, "C")
			}
		},
		{
			{ {"A", "a"}, {"B", "b"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Warped,
			Reporting::Quiet,
			{ str(1001, "a"), str(1002, "b"), str(1003, u8"\u0225") }
		},
		{
			{ {"A", "a"}, {"B", "b"}, {"C", "c"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Unmodifed,
			Reporting::Quiet,
			{ str(1001, "a"), str(1002, "b"), str(1003, u8"c") }
		},
		{
			{ {"A", "a"}, {"B", "b"}, {"C", "c"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Warped,
			Reporting::Quiet,
			{ str(1001, "a"), str(1002, "b"), str(1003, u8"c") }
		},
		{
			{ {"A", "a"}, {"B", "b"}, {"C", "c"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Unmodifed,
			Reporting::Verbose,
			{ str(1001, "a"), str(1002, "b"), str(1003, u8"c") }
		},
		{
			{ {"A", "a"}, {"B", "b"}, {"C", "c"} },
			test_strings{}.make(
				test_string(1001, "A", "x"),
				test_string(1002, "B", "y"),
				test_string(1003, "C", "z")
			).strings,
			Result::Warped,
			Reporting::Verbose,
			{ str(1001, "a"), str(1002, "b"), str(1003, u8"c") }
		},
	};

	INSTANTIATE_TEST_SUITE_P(strings, gtt_extract, ValuesIn(translations));

	static const mo_load_param msgs[] = {
		{
			{}, Result::Unmodifed, Reporting::Quiet, "no-such.mo"sv, {}, { error << lng::ERR_FILE_NOT_FOUND }
		},
		{
			test_strings{123}.make(), Result::Unmodifed, Reporting::Quiet, "no-such.mo"sv, { 123 }, { error << lng::ERR_FILE_NOT_FOUND }
		},
		{
			test_strings{123}.make(), Result::Unmodifed, Reporting::Verbose, "no-such.mo"sv, { 123 }, { error << lng::ERR_FILE_NOT_FOUND }
		},
		{
			test_strings{123}.make(), Result::Unmodifed, Reporting::Quiet, "empty_LE.mo"sv, { 123 }, { }
		},
		{
			test_strings{123}.make(), Result::Unmodifed, Reporting::Verbose, "empty_BE.mo"sv, { 123 }, { }
		},
		{
			test_strings{123}.make(
				test_string(1000, "a", "x"),
				test_string(1001, "key", "value")
			),
			Result::Unmodifed, Reporting::Quiet, "empty_LE.mo"sv, { 123 }, { }
		},
		{
			test_strings{123}.make(
				test_string(1000, "a", "x"),
				test_string(1001, "key", "value")
			),
			Result::Unmodifed, Reporting::Verbose, "empty_BE.mo"sv, { 123 },
			{
				warning << arg(lng::ERR_MSGS_TRANSLATION_MISSING, "a"),
				warning << arg(lng::ERR_MSGS_TRANSLATION_MISSING, "key"),
			}
		},
		{
			test_strings{123}.make(
				test_string(1000, "a", "x"),
				test_string(1001, "key", "value")
			),
			Result::Unmodifed, Reporting::Quiet, "ab_BE.mo"sv,
			{
				123,
				{},
				{ {1000, "b" } },
				{}
			},
			{ }
		},
		{
			test_strings{123}.make(
				test_string(1000, "a", "x"),
				test_string(1001, "key", "value")
			),
			Result::Unmodifed, Reporting::Verbose, "ab_LE.mo"sv,
			{
				123,
				{},
				{ {1000, "b" } },
				{}
			},
			{
				warning << arg(lng::ERR_MSGS_TRANSLATION_MISSING, "key"),
			}
		},
	};

	INSTANTIATE_TEST_SUITE_P(msgs, mo_load, ValuesIn(msgs));

	static const mo_fix_param attrs_tests[] = {
		{
			{}, {}, {}, true, { warning << lng::ERR_MSGS_ATTR_LANG_MISSING }
		},
		{
			"cy", {}, { {ATTR_CULTURE, "cy"}, {ATTR_LANGUAGE, "\xC5\xB4\xC3\xAA\xC4\xBA\xC5\x9F\xC4\xA5"} }
		},
		{
			"cy", "no-such.txt", { {ATTR_CULTURE, "cy"} }, false, { location{2}[severity::error] << lng::ERR_FILE_NOT_FOUND }
		},
		{
			"cy", "languages.txt", { {ATTR_CULTURE, "cy"}, {ATTR_LANGUAGE, "Cymraeg"} }
		},
		{
			"pl", "languages.txt", { {ATTR_CULTURE, "pl"}, {ATTR_LANGUAGE, "polski"} }
		},
		{
			"pl-PL", "languages.txt", {
				{ATTR_CULTURE, "pl-PL"},
				{ATTR_LANGUAGE, "P\xC3\xB4\xC4\xBA\xC3\xAF\xC5\x9F\xC4\xA5 (P\xC3\xB4\xC4\xBA\xC8\xA7\xC3\xB1\xC4\x91)"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "pl-PL") }
		},
		{
			"de-AT", "languages.txt", {
				{ATTR_CULTURE, "de-AT"},
				{ATTR_LANGUAGE, "Deutch (\xC3\x96sterreich)"}
			}
		},
		{
			"de", "languages.txt", {
				{ATTR_CULTURE, "de"},
				{ATTR_LANGUAGE, "\xC4\xA0\xC3\xAA\xC8\x93m\xC8\xA7\xC3\xB1"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "de") }
		},
		{
			"da", "languages.txt", { {ATTR_CULTURE, "da"}, {ATTR_LANGUAGE, "dansk"} }
		},
		{
			"en", "languages.txt", {
				{ATTR_CULTURE, "en"},
				{ATTR_LANGUAGE, "\xC8\x84\xC3\xB1\xC4\x9F\xC4\xBA\xC3\xAF\xC5\x9F\xC4\xA5"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "en") }
		},
		{
			"noniso", "languages.txt", {
				{ATTR_CULTURE, "noniso"},
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "noniso") }
		},
		{
			"noniso-NONISO", "languages.txt", {
				{ATTR_CULTURE, "noniso-NONISO"},
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "noniso-NONISO") }
		},
		{
			"en-NONISO", "languages.txt", {
				{ATTR_CULTURE, "en-NONISO"},
				{ATTR_LANGUAGE, "\xC8\x84\xC3\xB1\xC4\x9F\xC4\xBA\xC3\xAF\xC5\x9F\xC4\xA5 "
										"(\xC3\x99\xC3\xB1\xC4\xB7\xC3\xB1\xC3\xB4\xC5\xB5\xC3\xB1 - "
										"\xC3\x91\xC3\x96\xC3\x91\xC3\x8D\xC5\x9E\xC3\x96)"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "en-NONISO") }
		},
		{
			"en-Zzzz-US", "languages.txt", {
				{ATTR_CULTURE, "en-Zzzz-US"},
				{ATTR_LANGUAGE, "\xC8\x84\xC3\xB1\xC4\x9F\xC4\xBA\xC3\xAF\xC5\x9F\xC4\xA5 "
										"(\xC3\x99\xC3\xB1\xC3\xAF\xC5\xA7\xC3\xAA\xC4\x91 "
										"\xC5\x9E\xC5\xA7\xC8\xA7\xC5\xA7\xC3\xAA\xC5\x9F)"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "en-Zzzz-US") }
		},
		{
			"en-Zzzz-US-NONISO", "languages.txt", {
				{ATTR_CULTURE, "en-Zzzz-US-NONISO"},
				{ATTR_LANGUAGE, "\xC8\x84\xC3\xB1\xC4\x9F\xC4\xBA\xC3\xAF\xC5\x9F\xC4\xA5 "
										"(\xC3\x99\xC3\xB1\xC3\xAF\xC5\xA7\xC3\xAA\xC4\x91 "
										"\xC5\x9E\xC5\xA7\xC8\xA7\xC5\xA7\xC3\xAA\xC5\x9F)"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "en-Zzzz-US-NONISO") }
		},
		{
			"en-Zzzz", "languages.txt", {
				{ATTR_CULTURE, "en-Zzzz"},
				{ATTR_LANGUAGE, "\xC8\x84\xC3\xB1\xC4\x9F\xC4\xBA\xC3\xAF\xC5\x9F\xC4\xA5"}
			}, true, { location{2}[severity::warning] << arg(lng::ERR_LOCALE_MISSING, "en-Zzzz") }
		},
	};

	INSTANTIATE_TEST_SUITE_P(attrs, mo_fix, ValuesIn(attrs_tests));
}
