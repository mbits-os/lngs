#include <gtest/gtest.h>
#include <locale/file.hpp>
#include <locale/locale_file.hpp>

#include <lngs/strings.hpp>
#include <lngs/languages.hpp>
#include <lngs/streams.hpp>

#include <src/str.hpp>

namespace locale::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct attrs_t {
		std::vector<std::pair<attr_t, std::string>> vals{};
		intmax_t(*plural_map)(intmax_t) = nullptr;

		attrs_t&& culture(std::string val) && {
			vals.emplace_back(ATTR_CULTURE, std::move(val));
			return std::move(*this);
		}

		attrs_t&& language(std::string val) && {
			vals.emplace_back(ATTR_LANGUAGE, std::move(val));
			return std::move(*this);
		}

		attrs_t&& plurals(std::string val) && {
			vals.emplace_back(ATTR_PLURALS, std::move(val));
			return std::move(*this);
		}

		template <class Lambda>
		attrs_t&& map(Lambda cb) && {
			plural_map = cb;
			return std::move(*this);
		}
	};

	struct builder {
		uint32_t serial{ 0 };
		template <typename ... Strings>
		lngs::idl_strings make(Strings ... strings) {
			return { {}, {}, serial, -1, false, { lngs::idl_string{ strings }... } };
		}
	};

	inline lngs::idl_string str(int id, std::string key, std::string value) {
		return { std::move(key), std::move(value), {}, {}, id, id };
	}

	std::vector<std::byte> build_strings(const lngs::idl_strings& defs, const attrs_t& attrs, bool with_keys) {
		lngs::file file;
		file.serial = defs.serial;

		file.strings.reserve(defs.strings.size());
		file.attrs.reserve(attrs.vals.size());
		if (with_keys)
			file.keys.reserve(defs.strings.size());

		for (auto& string : defs.strings) {
			file.strings.emplace_back(string.id, string.value);
			if (with_keys)
				file.keys.emplace_back(string.id, string.key);
		}

		for (auto[id, attr] : attrs.vals)
			file.attrs.emplace_back(id, attr);

		std::vector<std::byte> out;

		struct stream : lngs::outstream {
			std::vector<std::byte>& contents;

			stream(std::vector<std::byte>& contents) : contents{ contents } {}
			std::size_t write(const void* data, std::size_t length) noexcept final
			{
				auto b = static_cast<const std::byte*>(data);
				auto e = b + length;
				auto size = contents.size();
				contents.insert(end(contents), b, e);
				return contents.size() - size;
			}
		} output{ out };

		file.write(output);
		return out;
	}

	struct file_info {
		lngs::idl_strings defs{};
		attrs_t attrs{};
		bool with_keys{ true };
	};
	struct lang_file_base : TestWithParam<file_info> {};

	TEST_P(lang_file_base, load) {
		auto [defs, attrs, with_keys] = GetParam();

		auto bytes = build_strings(defs, attrs, with_keys);

		lang_file file;
		auto result = file.open({ bytes.data(), bytes.size() });
		EXPECT_TRUE(result);

		for (auto const& str : defs.strings) {
			auto expected = split_view(str.value, "\0"sv);
			EXPECT_EQ(expected[0], file.get_string(static_cast<lang_file::identifier>(str.id)));
			if (with_keys) {
				EXPECT_EQ(str.key, file.get_key(str.id));
			} else {
				if (!str.key.empty()) {
					EXPECT_NE(str.key, file.get_key(str.id));
				}
				EXPECT_EQ("", file.get_key(str.id));
			}
		}

		for (auto attr : { ATTR_CULTURE, ATTR_LANGUAGE, ATTR_PLURALS }) {
			for (auto const& [att_key, expected] : attrs.vals) {
				if (att_key != attr) continue;
				EXPECT_EQ(expected, file.get_attr(att_key));
			}
		}
	}

	TEST_P(lang_file_base, by_keys) {
		auto[defs, attrs, with_keys] = GetParam();

		auto bytes = build_strings(defs, attrs, with_keys);

		lang_file file;
		auto result = file.open({ bytes.data(), bytes.size() });
		EXPECT_TRUE(result);

		if (with_keys) {
			for (auto const& str : defs.strings) {
				auto expected = split_view(str.value, "\0"sv);
				auto id = static_cast<lang_file::identifier>(file.find_key(str.key));
				EXPECT_EQ(expected[0], file.get_string(id));
			}
		} else {
			for (auto const& str : defs.strings) {
				auto id = file.find_key(str.key);
				EXPECT_NE(str.id, (int)id);
				auto expected = split_view(str.value, "\0"sv);
				if (!expected[0].empty()) {
					EXPECT_NE(expected[0], file.get_string(static_cast<lang_file::identifier>(id)));
				} else {
					EXPECT_EQ(expected[0], file.get_string(static_cast<lang_file::identifier>(str.id)));
				}
			}
		}
	}

	TEST_P(lang_file_base, outside) {
		auto[defs, attrs, with_keys] = GetParam();

		auto bytes = build_strings(defs, attrs, with_keys);

		lang_file file;
		auto result = file.open({ bytes.data(), bytes.size() });
		EXPECT_TRUE(result);
		EXPECT_EQ(static_cast<uint32_t>(-1), file.find_key({}));
		static constexpr const auto max_id = static_cast<lang_file::identifier>(
			std::numeric_limits<std::underlying_type_t<lang_file::identifier>>::max()
			);
		EXPECT_EQ(""sv, file.get_string(max_id));
		EXPECT_EQ(""sv, file.get_string(max_id, lang_file::quantity{ 100 }));
	}

	TEST_P(lang_file_base, plurals) {
		auto[defs, attrs, with_keys] = GetParam();

		auto bytes = build_strings(defs, attrs, with_keys);

		lang_file file;
		auto result = file.open({ bytes.data(), bytes.size() });
		EXPECT_TRUE(result);

		for (auto const& str : defs.strings) {
			auto expected = split_view(str.value, "\0"sv);
			if (expected.size() > 1) {
				for (intmax_t count = -100; count < 100; ++count) {
					auto expected_plural = attrs.plural_map(count);
					if (expected_plural < 0 || ((size_t)expected_plural) >= expected.size()) {
						GTEST_FAIL() << "Mapping for " << count << " outside of tested range: " << expected_plural << " vs. " << str.value;
					}
					auto plural = (size_t)expected_plural;
					EXPECT_EQ(expected[plural],
						file.get_string(
							static_cast<lang_file::identifier>(str.id),
							static_cast<lang_file::quantity>(count)
						)
					);
				} 
			} else {
				for (intmax_t count = -100; count < 100; ++count) {
					EXPECT_EQ(expected[0],
						file.get_string(
							static_cast<lang_file::identifier>(str.id),
							static_cast<lang_file::quantity>(count)
						)
					);
				}
			}
		}

		if (with_keys) {
		}
		else {
			for (auto const& str : defs.strings) {
				auto id = file.find_key(str.key);
				EXPECT_NE(str.id, (int)id);
				auto expected = split_view(str.value, "\0"sv);
				if (!expected[0].empty()) {
					EXPECT_NE(expected[0], file.get_string(static_cast<lang_file::identifier>(id)));
				}
				else {
					EXPECT_EQ(expected[0], file.get_string(static_cast<lang_file::identifier>(str.id)));
				}
			}
		}
	}

	static const auto stringz = builder{ 123 }.make(
		str(1000, "KEY1", "VALUE1"),
		str(1001, "KEY2", "SINGLE VALUE\0{0} VALUES"s),
		str(1002, "KEY3", "VALUE3"),
		str(1003, "KEY4", "VALUE4"),
		str(1004, "KEY5", "VALUE5")
	);

	static const auto attrz = attrs_t{}
		.culture("ll_CC")
		.language("language (Region)")
		.plurals("nplurals=2; plural=(n != 1);")
		.map([](intmax_t n) -> intmax_t { return (n != 1); });

	static const auto attrz_broken = attrs_t{}.map([](intmax_t) -> intmax_t { return 0; });

	static const file_info files[] = {
		{ },
		{ stringz, attrz },
		{ stringz, attrz, false },
		{ stringz, attrz_broken },
		{ stringz, attrz_broken, false },
	};

	INSTANTIATE_TEST_CASE_P(files, lang_file_base, ValuesIn(files));
}
