#include <gtest/gtest.h>
#include <lngs/file.hpp>
#include <lngs/lngs_base.hpp>

extern fs::path TESTING_data_path;

namespace fs::testing {
	using namespace ::std::literals;

	TEST(file, write) {
		error_code ec;

		auto temp = temp_directory_path(ec);
		ASSERT_FALSE(ec);

		temp /= "locale-test";
		create_directories(temp, ec);
		ASSERT_FALSE(ec);

		auto file = fopen(temp / "A.txt", "wb");
		EXPECT_TRUE(file);

		constexpr const char pangram[] = R"(The quick brown fox
jumps over
the lazy dog)";

		auto result = file.store(pangram, sizeof(pangram) - 1);
		EXPECT_EQ((sizeof(pangram) - 1), result);
	}

	TEST(file, read) {
		auto file = fopen(TESTING_data_path / "no_last.data");
		ASSERT_TRUE(file) << TESTING_data_path / "no_last.data";
		uint32_t tag;
		auto read = file.load(&tag, sizeof(uint32_t));
		EXPECT_EQ(sizeof(uint32_t), read);
		EXPECT_EQ(lngs::v1_0::langtext_tag, tag);

		read = file.load(&tag, sizeof(uint32_t));
		EXPECT_EQ(sizeof(uint32_t), read);
		EXPECT_EQ(lngs::v1_0::hdrtext_tag, tag);

		uint32_t ints;
		read = file.load(&ints, sizeof(uint32_t));
		EXPECT_EQ(sizeof(uint32_t), read);
		EXPECT_EQ(2u, ints);

		uint32_t version;
		read = file.load(&version, sizeof(uint32_t));
		EXPECT_EQ(sizeof(uint32_t), read);
		EXPECT_EQ(lngs::v1_0::version, version);
	}
}