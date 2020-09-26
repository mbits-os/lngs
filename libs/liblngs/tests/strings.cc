#include <gtest/gtest.h>
#include <lngs/lngs.hpp>

extern fs::path TESTING_data_path;

namespace lngs::testing {
	using namespace ::std::literals;
	using ::testing::Test;
	using ::testing::ValuesIn;

	enum class id {
		YES = 1000,
		NO = 1001,
	};

	enum class ids { MAYBE = 1002 };

	using Singulars = SingularStrings<id>;
	using Plurals = PluralOnlyStrings<ids>;
	using Strings = StringsWithPlurals<id, ids>;

	struct strings : Test {
		Singulars tr1;
		Plurals tr2;
		Strings tr3;

		void SetUp() override {
			auto root = TESTING_data_path / "testset1.ext";
			tr1.path_manager<manager::ExtensionPath>(root, "pkg1");
			tr2.path_manager<manager::ExtensionPath>(root, "pkg1");
			tr3.path_manager<manager::ExtensionPath>(root, "pkg1");
		}
	};

	TEST_F(strings, Singulars) {
		auto& current = tr1;
		ASSERT_TRUE(current.open("foo", SerialNumber::UseAny));
		EXPECT_EQ("Meta (FOO)", current.attr(ATTR_LANGUAGE));
		EXPECT_EQ("", current.attr(ATTR_PLURALS));
		EXPECT_EQ("foo:yes", current(id::YES));
		EXPECT_EQ("foo:no", current(id::NO));
	}

	TEST_F(strings, Plurals) {
		auto& current = tr2;
		ASSERT_TRUE(current.open("bar", SerialNumber::UseAny));
		EXPECT_EQ("Meta (BAR)", current.attr(ATTR_LANGUAGE));
		EXPECT_EQ("", current.attr(ATTR_PLURALS));  // sic!
		EXPECT_EQ("bar:maybe", current(ids::MAYBE, 0));
		EXPECT_EQ("bar:maybe", current(ids::MAYBE, 1));
	}

	TEST_F(strings, Strings) {
		auto& current = tr3;
		ASSERT_FALSE(current.open("fred", SerialNumber::UseAny));
		ASSERT_TRUE(current.open("fred-XYZZY", SerialNumber::UseAny));
		EXPECT_EQ("Wilhelmina (Harker)", current.attr(ATTR_LANGUAGE));
		EXPECT_EQ("", current.attr(ATTR_PLURALS));  // sic!
		EXPECT_EQ("Pebble", current(id::YES));
		EXPECT_EQ("Bam!", current(id::NO));
		EXPECT_EQ("I'm home!", current(ids::MAYBE, 0));
		EXPECT_EQ("I'm home!", current(ids::MAYBE, 1));
	}
}  // namespace lngs::testing
