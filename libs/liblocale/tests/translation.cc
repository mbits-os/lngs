#include <gtest/gtest.h>
#include <locale/translation.hpp>
#include "lang_file_helpers.h"

extern fs::path LOCALE_data_path;

namespace locale {
	class translation_tests {
	public:
		static void set_nextupdate(translation& tr, uint32_t value) {
			tr.m_nextupdate = value;
		}
	};
}
namespace locale::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct simple_culture {
		std::string lang;
		simple_culture(const char* lang) : lang{ lang } {}
		bool operator==(const culture& rhs) const { return lang == rhs.lang; }
		friend bool operator==(const culture& lhs, const simple_culture& rhs) { return rhs == lhs; }
	};

	struct tr_info {
		std::string root;
		std::string name;
		std::initializer_list<simple_culture> expected_known;
		std::initializer_list<std::string> keys;

		friend std::ostream& operator<<(std::ostream& o, const tr_info& info) {
			return o << info.root << '{' << info.name << '}';
		}
	};

	struct translation : TestWithParam<tr_info> {
		static void write_strings(const fs::path& dst, const lngs::idl_strings& defs, const helper::attrs_t& attrs, bool with_keys = true) {
			auto file = lngs::foutstream{ fs::fopen(dst, "wb") };
			helper::build_strings(file, defs, attrs, with_keys);
		}

		static void SetUpTestCase() {
			if constexpr (false) {
				auto testset_1 = LOCALE_data_path / "testset1.ext";
				auto testset_2 = LOCALE_data_path / "testset2.sub";

				try {
					fs::create_directories(testset_1 / "wibbly-WOBBLY");
					fs::create_directories(testset_2 / "foo");
					fs::create_directories(testset_2 / "bar");
					fs::create_directories(testset_2 / "fred-XYZZY");
					fs::create_directories(testset_2 / "baz-QUUX" / "wibbly-WOBBLY");
				}
				catch (fs::filesystem_error& err) {
					std::cerr << "error: " << err.what() << '\n';
					std::exit(1);
				}

				using namespace helper;
				const auto foo = attrs_t{}.culture("foo").language("Meta (FOO)");
				const auto bar = attrs_t{}.culture("bar").language("Meta (BAR)");
				const auto baz = attrs_t{}.culture("baz-QUUX").language("Meta (BAZ/QUUX)");
				const auto fred = attrs_t{}.culture("fred-XYZZY").language("Wilhelmina (Harker)");

				const auto pkg1_foo = builder{ 123 }.make(
					str(1000, "YES", "foo:yes"),
					str(1001, "NO", "foo:no"),
					str(1002, "MAYBE", "foo:maybe")
				);

				const auto pkg1_bar = builder{ 123 }.make(
					str(1000, "YES", "bar:yes"),
					str(1001, "NO", "bar:no"),
					str(1002, "MAYBE", "bar:maybe")
				);

				const auto pkg1_fred = builder{ 123 }.make(
					str(1000, "YES", "Pebble"),
					str(1001, "NO", "Bam!"),
					str(1002, "MAYBE", "I'm home!")
				);

				const auto pkg2_foo = builder{ 456 }.make(
					str(1000, "LEFT", "foo:left"),
					str(1001, "RIGHT", "foo:right"),
					str(1002, "FRONT", "foo:front"),
					str(1003, "BACK", "foo:back")
				);

				const auto pkg2_bar = builder{ 456 }.make(
					str(1000, "LEFT", "bar:left"),
					str(1001, "RIGHT", "bar:right"),
					str(1002, "FRONT", "bar:front"),
					str(1003, "BACK", "bar:back")
				);

				const auto pkg3_foo = builder{ 789 }.make(
					str(1000, "ADJ1", "foo:advanced"),
					str(1001, "ADJ2", "foo:freeware"),
					str(1002, "ADJ3", "foo:audio"),
					str(1003, "NOUN", "foo:player")
				);

				const auto pkg3_bar = builder{ 789 }.make(
					str(1000, "ADJ1", "bar:advanced"),
					str(1001, "ADJ2", "bar:freeware"),
					str(1002, "ADJ3", "bar:audio"),
					str(1003, "NOUN", "bar:player")
				);

				const auto pkg3_baz = builder{ 789 }.make(
					str(1000, "ADJ1", "baz:advanced"),
					str(1001, "ADJ2", "baz:freeware"),
					str(1002, "ADJ3", "baz:audio"),
					str(1003, "NOUN", "baz:player")
				);

				write_strings(testset_1 / "pkg1.foo", pkg1_foo, foo);
				write_strings(testset_1 / "pkg1.bar", pkg1_bar, bar);
				write_strings(testset_1 / "pkg1.fred-XYZZY", pkg1_fred, fred);
				write_strings(testset_1 / "pkg2.foo", pkg2_foo, foo);
				write_strings(testset_1 / "pkg2.bar", pkg2_bar, bar);
				write_strings(testset_1 / "pkg3.foo", pkg3_foo, foo);
				write_strings(testset_1 / "pkg3.bar", pkg3_bar, bar);
				write_strings(testset_1 / "pkg3.baz-QUUX", pkg3_baz, baz);
				write_strings(testset_2 / "foo" / "pkg1", pkg1_foo, foo);
				write_strings(testset_2 / "bar" / "pkg1", pkg1_bar, bar);
				write_strings(testset_2 / "fred-XYZZY" / "pkg1", pkg1_fred, fred);
				write_strings(testset_2 / "foo" / "pkg2", pkg2_foo, foo);
				write_strings(testset_2 / "bar" / "pkg2", pkg2_bar, bar);
				write_strings(testset_2 / "foo" / "pkg3", pkg3_foo, foo);
				write_strings(testset_2 / "bar" / "pkg3", pkg3_bar, bar);
				write_strings(testset_2 / "baz-QUUX" / "pkg3", pkg3_baz, baz);

				write_strings(testset_1 / "pkg1.baz-QUUX", pkg1_foo, foo);
				write_strings(testset_2 / "baz-QUUX" / "pkg1", pkg1_foo, foo);
				fs::fopen(testset_2 / "baz-QUUX" / "pkg2", "w");

				fs::fopen(testset_1 / "package1.timey-WIMEY", "w");
				fs::fopen(testset_2 / "baz-QUUX" / "package1.timey-WIMEY", "w");
				fs::fopen(testset_1 / "wibbly-WOBBLY" / "stuff", "w");
				fs::fopen(testset_2 / "baz-QUUX" / "wibbly-WOBBLY" / "stuff", "w");
				fs::fopen(testset_2 / "baz-QUUX.stuff", "w");
			}
		}
	};

	TEST_P(translation, known) {
		auto& param = GetParam();

		auto root = LOCALE_data_path / param.root;
		locale::translation tr;
		if (root.extension() == ".ext") {
			tr.path_manager<manager::ExtensionPath>(root, param.name);
		} else {
			tr.path_manager<manager::SubdirPath>(root, param.name);
		}

		auto actual = tr.known();

		auto expected = std::vector(param.expected_known);
		auto comp = [](const auto& lhs, const auto& rhs) {
			return lhs.lang < rhs.lang;
		};
		sort(begin(actual), end(actual), comp);
		sort(begin(expected), end(expected), comp);
		ASSERT_EQ(expected.size(), actual.size());
		auto it = begin(expected);
		for (auto const& act : actual) {
			auto const& exp = *it++;
			EXPECT_EQ(exp, act);
		}
	}

	TEST_P(translation, onupdate) {
		auto& param = GetParam();

		auto root = LOCALE_data_path / param.root;
		locale::translation tr;
		if (root.extension() == ".ext") {
			tr.path_manager<manager::ExtensionPath>(root, param.name);
		} else {
			tr.path_manager<manager::SubdirPath>(root, param.name);
		}

		size_t counter{ 0 };
		std::string expected_culture;
		auto callback = [&] {
			++counter;
			auto actual_culture = tr.get_attr(ATTR_CULTURE);
			EXPECT_EQ(expected_culture, actual_culture);
		};

		translation_tests::set_nextupdate(tr, std::numeric_limits<uint32_t>::max());
		auto token = tr.add_onupdate(callback);

		for (auto const& ll_CC : param.expected_known) {
			expected_culture = ll_CC.lang;
			EXPECT_TRUE(tr.open(expected_culture));
		}
		EXPECT_EQ(param.expected_known.size(), counter);

		expected_culture.clear();
		EXPECT_FALSE(tr.open("timey-WIMEY"));

		tr.remove_onupdate(token);
		tr.add_onupdate({});
	}

	TEST_P(translation, keys) {
		auto& param = GetParam();

		auto root = LOCALE_data_path / param.root;
		locale::translation tr;
		if (root.extension() == ".ext") {
			tr.path_manager<manager::ExtensionPath>(root, param.name);
		}
		else {
			tr.path_manager<manager::SubdirPath>(root, param.name);
		}

		for (auto const& ll_CC : param.expected_known) {
			EXPECT_TRUE(tr.open(ll_CC.lang));

			for (auto const& key : param.keys) {
				auto id = tr.find_key(key);
				EXPECT_EQ(key, tr.get_key(id));

				auto ident = static_cast<lang_file::identifier>(id);
				auto s = tr.get_string(ident);
				EXPECT_FALSE(s.empty());
				EXPECT_EQ(s, tr.get_string(ident, static_cast<lang_file::quantity>(2)));
			}
		}
	}

	static const std::initializer_list<std::string> pkg1_keys{ "YES", "NO", "MAYBE" };
	static const std::initializer_list<std::string> pkg2_keys{ "LEFT", "RIGHT", "FRONT", "BACK" };
	static const std::initializer_list<std::string> pkg3_keys{ "ADJ1", "ADJ2", "ADJ3", "NOUN" };

	static const tr_info packages[] = {
		{ "testset1.ext", "pkg1", { "foo", "bar", "fred-XYZZY" }, pkg1_keys },
		{ "testset1.ext", "pkg2", { "foo", "bar" }, pkg2_keys },
		{ "testset1.ext", "pkg3", { "foo", "bar", "baz-QUUX" }, pkg3_keys },
		{ "testset2.sub", "pkg1", { "foo", "bar", "fred-XYZZY" }, pkg1_keys },
		{ "testset2.sub", "pkg2", { "foo", "bar" }, pkg2_keys },
		{ "testset2.sub", "pkg3", { "foo", "bar", "baz-QUUX" }, pkg3_keys },
	};

	INSTANTIATE_TEST_CASE_P(packages, translation, ValuesIn(packages));
}
