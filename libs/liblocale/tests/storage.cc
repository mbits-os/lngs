#include <gtest/gtest.h>
#include <locale/locale_storage.hpp>
#include "lang_file_helpers.h"
#include <cstdlib>
#include <clocale>

extern fs::path LOCALE_data_path;

namespace locale::storage::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct simple_culture {
		std::string lang;
		simple_culture(const char* lang) : lang{ lang } {}
		bool operator==(const culture& rhs) const { return lang == rhs.lang; }
		friend bool operator==(const culture& lhs, const simple_culture& rhs) { return rhs == lhs; }
	};

	struct stg_info {
		std::string root;
		std::string name;
		std::initializer_list<simple_culture> expected_known;
		std::initializer_list<std::string> keys;
		std::initializer_list<std::string> builtin_keys;
		const std::vector<std::byte>& (*generator)();

		friend std::ostream& operator<<(std::ostream& o, const stg_info& info) {
			return o << info.root << '{' << info.name << '}';
		}
	};

	struct vector_resource {
		static const std::vector<std::byte>* bytes;
		static const char* data() { return (const char*)bytes->data(); }
		static std::size_t size() { return bytes->size(); }
	};
	const std::vector<std::byte>* vector_resource::bytes{nullptr};

	using vector_builtin = Builtin<vector_resource>;
	using vector_file_builtin = FileWithBuiltin<vector_resource>;

	template <typename Storage, typename = void> struct has_builtin : std::false_type {};
	template <typename Storage> struct has_builtin<Storage, std::enable_if_t<
		std::is_same_v<decltype(std::declval<Storage>().init()), bool>
		>> : std::true_type {};

	template <typename Storage, typename = void> struct has_file_based : std::false_type {};
	template <typename Storage> struct has_file_based<Storage, std::void_t<
		std::enable_if_t< std::is_same_v<decltype(std::declval<Storage>().open(std::declval<std::string>())), bool> >,
		std::enable_if_t< std::is_same_v<decltype(std::declval<Storage>().known()), std::vector<culture>> >
		>> : std::true_type {};

	template <typename Storage> constexpr bool has_builtin_v = has_builtin<Storage>::value;
	template <typename Storage> constexpr bool has_file_based_v = has_file_based<Storage>::value;

	static_assert(has_builtin_v<vector_builtin>);
	static_assert(has_builtin_v<vector_file_builtin>);
	static_assert(!has_builtin_v<FileBased>);

	static_assert(!has_file_based_v<vector_builtin>);
	static_assert(has_file_based_v<vector_file_builtin>);
	static_assert(has_file_based_v<FileBased>);

	template <typename Storage>
	struct storage : TestWithParam<stg_info> {
		Storage tr;

		void SetUp() override {
			auto& param = GetParam();

			if constexpr (has_file_based_v<Storage>) {
				auto root = LOCALE_data_path / param.root;
				if (root.extension() == ".ext") {
					tr.template path_manager<manager::ExtensionPath>(root, param.name);
				}
				else {
					tr.template path_manager<manager::SubdirPath>(root, param.name);
				}
			}

			if constexpr (has_builtin_v<Storage>) {
				vector_resource::bytes = &(*param.generator)();
				tr.init();
			}
		};

		template <typename StorageType=Storage>
		std::enable_if_t<has_file_based_v<StorageType>>
			test_known()
		{
			auto& param = GetParam();

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

		template <typename StorageType = Storage>
		std::enable_if_t<has_file_based_v<StorageType>>
			test_onupdate()
		{
			auto& param = GetParam();

			size_t counter{ 0 };
			std::string expected_culture;
			auto callback = [&] {
				++counter;
				auto actual_culture = tr.get_attr(ATTR_CULTURE);
				EXPECT_EQ(expected_culture, actual_culture);
			};

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

		void test_keys() {
			auto& param = GetParam();

			if constexpr (has_file_based_v<Storage>) {
				for (auto const& ll_CC : param.expected_known) {
					EXPECT_TRUE(tr.open(ll_CC.lang));
					test_find_key();
				}
			} else {
				test_find_key();
			}
		}

		template <typename StorageType = Storage>
		std::enable_if_t<has_builtin_v<StorageType>> test_find_key() {
			test_expected_keys();
			auto& param = GetParam();

			for (auto const& key : param.builtin_keys) {
				auto id = tr.find_key(key);
				EXPECT_EQ(key, tr.get_key(id));

				auto ident = static_cast<lang_file::identifier>(id);
				auto s = tr.get_string(ident);
				EXPECT_FALSE(s.empty());
				EXPECT_EQ(s, tr.get_string(ident, static_cast<lang_file::quantity>(2)));
			}
		}

		template <typename StorageType = Storage>
		std::enable_if_t<!has_builtin_v<StorageType>> test_find_key() {
			test_expected_keys();
			auto& param = GetParam();

			for (auto const& key : param.builtin_keys) {
				auto id = tr.find_key(key);
				EXPECT_EQ(std::numeric_limits<uint32_t>::max(), id);
				EXPECT_NE(key, tr.get_key(id));

				auto ident = static_cast<lang_file::identifier>(id);
				auto s = tr.get_string(ident);
				EXPECT_TRUE(s.empty());
				EXPECT_EQ(s, tr.get_string(ident, static_cast<lang_file::quantity>(2)));
			}
		}

		void test_expected_keys() {
			auto& param = GetParam();

			for (auto const& key : param.keys) {
				auto id = tr.find_key(key);
				EXPECT_EQ(key, tr.get_key(id));

				auto ident = static_cast<lang_file::identifier>(id);
				auto s = tr.get_string(ident);
				EXPECT_FALSE(s.empty());
				EXPECT_EQ(s, tr.get_string(ident, static_cast<lang_file::quantity>(2)));
			}
		}
	};

	template <typename Storage>
	struct publicize : Storage {
		using Storage::get_string;
		using Storage::get_attr;
		using Storage::get_key;
		using Storage::find_key;
	};

	using storage_FileBased = storage<publicize<FileBased>>;
	using storage_Builtin = storage<publicize<vector_builtin>>;
	using storage_FileWithBuiltin = storage<publicize<vector_file_builtin>>;

	TEST_P(storage_FileBased, known) { test_known(); }
	TEST_P(storage_FileWithBuiltin, known) { test_known(); }

	TEST_P(storage_FileBased, onupdate) { test_onupdate(); }
	TEST_P(storage_FileWithBuiltin, onupdate) { test_onupdate(); }

	TEST_P(storage_FileBased, keys) { test_keys(); }
	TEST_P(storage_Builtin, keys) { test_keys(); }
	TEST_P(storage_FileWithBuiltin, keys) { test_keys(); }

	struct open_first_of {
		std::string package;
		std::initializer_list<std::string> one_of;
		bool expected{ true };
	};

	struct storage_FileBased_open : TestWithParam<open_first_of> {};

	TEST_P(storage_FileBased_open, first_of) {
		auto& param = GetParam();

		publicize<FileBased> tr;

		tr.path_manager<manager::ExtensionPath>(
			LOCALE_data_path / "testset1.ext",
			param.package);

		auto vectored = std::vector(param.one_of);
		EXPECT_EQ(param.expected, tr.open_first_of(vectored));
		EXPECT_EQ(param.expected, tr.open_first_of(param.one_of));
	}

	struct header {
		std::string_view contents;
		std::vector<std::string> expected;
	};

	struct storage_AcceptLanguage : TestWithParam<header> {};

	TEST_P(storage_AcceptLanguage, list) {
		auto& param = GetParam();

		auto act = http_accept_language(param.contents);

		ASSERT_EQ(param.expected.size(), act.size());

		auto it = begin(param.expected);
		for (auto const& actual : act) {
			auto const& expected = *it++;
			EXPECT_EQ(expected, actual);
		}
	}

	namespace helper = locale::testing::helper;

	std::vector<std::byte> build_bytes(const lngs::idl_strings& defs, const helper::attrs_t& attrs) {
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

		helper::build_strings(output, defs, attrs, true);
		return out;
	}

	static const std::vector<std::byte>& pkg1_builtin() {
		using namespace helper;
		static auto bytes = build_bytes(
			builder{ 123 }.make(
				str(1000, "YES", "builtin:yes"),
				str(1001, "NO", "builtin:no"),
				str(1002, "MAYBE", "builtin:maybe"),
				str(1003, "EX", "builtin:additional"),
				str(1004, "TRA", "builtin:extra"),
				str(1005, "KEYS", "builtin:keys")
			),
			attrs_t{}
		);
		return bytes;
	}

	static const std::vector<std::byte>& pkg2_builtin() {
		using namespace helper;
		static auto bytes = build_bytes(
			builder{ 123 }.make(
				str(1000, "LEFT", "builtin:left"),
				str(1001, "RIGHT", "builtin:right"),
				str(1002, "FRONT", "builtin:front"),
				str(1003, "BACK", "builtin:back"),
				str(1004, "EX", "builtin:additional"),
				str(1005, "TRA", "builtin:extra"),
				str(1006, "KEYS", "builtin:keys")
			),
			attrs_t{}
		);
		return bytes;
	}

	static const std::vector<std::byte>& pkg3_builtin() {
		using namespace helper;
		static auto bytes = build_bytes(
			builder{ 123 }.make(
				str(1000, "ADJ1", "builtin:advanced"),
				str(1001, "ADJ2", "builtin:freeware"),
				str(1002, "ADJ3", "builtin:audio"),
				str(1003, "NOUN", "builtin:player"),
				str(1004, "EX", "builtin:additional"),
				str(1005, "TRA", "builtin:extra"),
				str(1006, "KEYS", "builtin:keys")
			),
			attrs_t{}
		);
		return bytes;
	}

	static const std::initializer_list<std::string> pkg1_keys{ "YES", "NO", "MAYBE" };
	static const std::initializer_list<std::string> pkg2_keys{ "LEFT", "RIGHT", "FRONT", "BACK" };
	static const std::initializer_list<std::string> pkg3_keys{ "ADJ1", "ADJ2", "ADJ3", "NOUN" };

	static const std::initializer_list<std::string> builtin_keys{ "EX", "TRA", "KEYS" };

	static const stg_info packages[] = {
		{ "testset1.ext", "pkg1", { "foo", "bar", "fred-XYZZY" }, pkg1_keys, builtin_keys, pkg1_builtin },
		{ "testset1.ext", "pkg2", { "foo", "bar" }, pkg2_keys, builtin_keys, pkg2_builtin },
		{ "testset1.ext", "pkg3", { "foo", "bar", "baz-QUUX" }, pkg3_keys, builtin_keys, pkg3_builtin },
		{ "testset2.sub", "pkg1", { "foo", "bar", "fred-XYZZY" }, pkg1_keys, builtin_keys, pkg1_builtin },
		{ "testset2.sub", "pkg2", { "foo", "bar" }, pkg2_keys, builtin_keys, pkg2_builtin },
		{ "testset2.sub", "pkg3", { "foo", "bar", "baz-QUUX" }, pkg3_keys, builtin_keys, pkg3_builtin },
	};

	INSTANTIATE_TEST_CASE_P(packages, storage_FileBased, ValuesIn(packages));
	INSTANTIATE_TEST_CASE_P(packages, storage_Builtin, ValuesIn(packages));
	INSTANTIATE_TEST_CASE_P(packages, storage_FileWithBuiltin, ValuesIn(packages));

	static const open_first_of groups[] = {
		{"pkg1", { "fred-XYZZY", "fred", "foo" }},
		{"pkg2", { "fred-XYZZY", "fred", "foo" }},
		{"pkg3", { "no", "recognized", "names" }, false},
	};

	INSTANTIATE_TEST_CASE_P(groups, storage_FileBased_open, ValuesIn(groups));

	static const header headers[] = {
		{ {}, { "en" } },
		{ "da, en-gb;q=0.8, en;q=0.7"sv, { "da", "en-gb", "en" }},
		{ "da, en-gb;q=0.8, pl-PL;q=0.5, en;q=0.7, fr;q=0.8, pl-PL;dun-dun=dunn;q=0.999"sv, { "da", "pl-PL", "pl", "en-gb", "fr", "en" }},
		{ "da, en-gb;q=0.8888, en;q=0.7a8, fr;q=0.77, pl;q=0.7.8"sv, { "da", "en-gb", "fr", "en", "pl" }}
	};

	INSTANTIATE_TEST_CASE_P(headers, storage_AcceptLanguage, ValuesIn(headers));

	TEST(storage, system_locales) {
#ifndef _WIN32
		std::setlocale(LC_MESSAGES, "C");
		setenv("LANGUAGE", "POSIX", 1);
		setenv("LC_MESSAGES", "king", 1);
		setenv("LANG", "king-KONG", 1);
#endif
		auto actual = system_locales();
#ifndef _WIN32
		auto expected = std::vector{ "king", "king-KONG", "en" };
		ASSERT_GE(expected.size(), actual.size());
		EXPECT_EQ(expected.size(), actual.size());
		auto it = begin(expected);
		for (auto const& act : actual) {
			auto const& exp = *it++;
			EXPECT_EQ(exp, act);
		}
#endif
	}
}
