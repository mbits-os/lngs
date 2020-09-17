#include <gtest/gtest.h>
#include <lngs/file.hpp>
#include <lngs/lngs_file.hpp>

extern fs::path TESTING_data_path;

namespace lngs::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct lang_file_bad : TestWithParam<std::string> {
		template <typename T>
		static void write(fs::file& lngs_file, const T& data) {
			lngs_file.store(&data, sizeof(data));
		}

		static void write_lngs_head(fs::file& lngs_file,
		                            uint32_t tag_file = langtext_tag,
		                            uint32_t tag_hdr = hdrtext_tag,
		                            uint32_t hdr_ints = 2,
		                            uint32_t ver = v1_0::version,
		                            uint32_t serial = 123) {
			write(lngs_file, tag_file);
			write(lngs_file, tag_hdr);
			write(lngs_file, hdr_ints);
			write(lngs_file, ver);
			write(lngs_file, serial);
		}

		static void write_lngs_last(fs::file& lngs_file) {
			static constexpr uint32_t zero = 0;
			write(lngs_file, v1_0::lasttext_tag);
			write(lngs_file, zero);
		}

		static void SetUpTestCase() {
			if constexpr (false) {
				auto lngs_file =
				    fs::fopen(TESTING_data_path / "truncated.data", "wb");

				lngs_file = fs::fopen(TESTING_data_path / "zero.data", "wb");
				write_lngs_head(lngs_file, 0, 0, 2, 0, 0);

				lngs_file =
				    fs::fopen(TESTING_data_path / "file_tag.data", "wb");
				write_lngs_head(lngs_file, langtext_tag, 0, 2, 0, 0);

				lngs_file =
				    fs::fopen(TESTING_data_path / "header_2.0.data", "wb");
				write_lngs_head(lngs_file, langtext_tag, hdrtext_tag, 2,
				                0x0000'0200);

				lngs_file =
				    fs::fopen(TESTING_data_path / "header_small.data", "wb");
				write_lngs_head(lngs_file, langtext_tag, hdrtext_tag, 1);

				lngs_file =
				    fs::fopen(TESTING_data_path / "header_big.data", "wb");
				write_lngs_head(lngs_file, langtext_tag, hdrtext_tag, 4);

				lngs_file = fs::fopen(TESTING_data_path / "no_last.data", "wb");
				write_lngs_head(lngs_file);

				lngs_file =
				    fs::fopen(TESTING_data_path / "broken_strs_1.data", "wb");
				write_lngs_head(lngs_file);
				write(lngs_file, string_header{strstext_tag, 2, 0, 5});
				write_lngs_last(lngs_file);

				lngs_file =
				    fs::fopen(TESTING_data_path / "broken_strs_2.data", "wb");
				write_lngs_head(lngs_file);
				write(lngs_file, strstext_tag);
				write(lngs_file, string_header{strstext_tag, 2, 1, 4});
				write_lngs_last(lngs_file);

				lngs_file =
				    fs::fopen(TESTING_data_path / "broken_attr_1.data", "wb");
				write_lngs_head(lngs_file);
				write(lngs_file, string_header{attrtext_tag, 5, 1, 7});
				write(lngs_file, string_key{1000, 0, 5});
				write(lngs_file, string_header{strstext_tag, 2, 0, 4});
				write_lngs_last(lngs_file);

				constexpr static const char valuee[] = "valuee\0\0";
				lngs_file =
				    fs::fopen(TESTING_data_path / "broken_attr_2.data", "wb");
				write_lngs_head(lngs_file);
				write(lngs_file, string_header{attrtext_tag, 7, 1, 7});
				write(lngs_file, string_key{1000, 0, 5});
				lngs_file.store(valuee, sizeof(valuee) - 1);
				write(lngs_file, string_header{strstext_tag, 2, 0, 4});
				write_lngs_last(lngs_file);

				lngs_file =
				    fs::fopen(TESTING_data_path / "broken_keys_1.data", "wb");
				write_lngs_head(lngs_file);
				write(lngs_file, string_header{keystext_tag, 2, 1, 4});
				write_lngs_last(lngs_file);

				lngs_file =
				    fs::fopen(TESTING_data_path / "broken_keys_2.data", "wb");
				write_lngs_head(lngs_file);
				write(lngs_file, string_header{attrtext_tag, 5, 1, 7});
				write(lngs_file, string_key{1000, 10, 5});
				write_lngs_last(lngs_file);
			}
		}
	};

	TEST_P(lang_file_bad, load) {
		auto& path = GetParam();

		auto bytes = fs::fopen(TESTING_data_path / path, "rb").read();

		lang_file file;
		auto result = file.open({bytes.data(), bytes.size()});
		EXPECT_FALSE(result);
	}

	static const std::string files[] = {
	    "no-such.data",       "truncated.data",     "zero.data",
	    "file_tag.data",      "header_2.0.data",    "header_small.data",
	    "header_big.data",    "no_last.data",       "broken_strs_1.data",
	    "broken_strs_2.data", "broken_attr_1.data", "broken_attr_2.data",
	    "broken_keys_1.data", "broken_keys_2.data",
	};

	INSTANTIATE_TEST_SUITE_P(files, lang_file_bad, ValuesIn(files));
}  // namespace lngs::testing
