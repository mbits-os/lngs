#include <gtest/gtest.h>
#include <lngs/file.hpp>
#include <lngs/internals/streams.hpp>

#ifdef _WIN32
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

namespace lngs::app::testing {
	namespace {
		static constexpr std::string_view line{"Hello, World!" NEWLINE};
	}

	TEST(streams, stdout) {
		outstream& ref = get_stdout();
		auto written = ref.print("Hello, {}!\n", "World");
		EXPECT_EQ(line.size(), written);
	}

	TEST(streams, file) {
		fs::error_code ec;

		auto temp = fs::temp_directory_path(ec);
		ASSERT_FALSE(ec);

		temp /= "locale-test";
		fs::create_directories(temp, ec);
		ASSERT_FALSE(ec);

		printf("TEMP: %s\n", temp.string().c_str());
		auto file = fs::fopen(temp / "file.txt", "w");
		EXPECT_TRUE(file);

		{
			foutstream out{std::move(file)};
			EXPECT_FALSE(file);

			outstream& ref = out;
			auto written = ref.print("Hello, {}!\n", "World");
			EXPECT_EQ(line.size(), written);
		}

		{
			file = fs::fopen(temp / "file.txt", "rb");
			EXPECT_TRUE(file);

			std::vector<char> data(10 * line.size());
			auto actual_size = file.load(data.data(), data.size());
			printf("? %d\n", errno);
			auto in_line = std::string_view{data.data(), actual_size};
			EXPECT_EQ(in_line, line);
		}
	}
}  // namespace lngs::app::testing