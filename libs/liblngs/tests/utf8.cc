#include <gtest/gtest.h>
#include <lngs/utf8.hpp>

namespace utf::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct string_convert {
		const std::string utf8;
		const std::u16string utf16;
		const std::u32string utf32;
	};

	template <typename Out> struct utf_helper;
	template <> struct utf_helper<char> {
		template <typename View>
		static auto conv(View str) { return as_u8(str); }
	};
	template <> struct utf_helper<char16_t> {
		template <typename View>
		static auto conv(View str) { return as_u16(str); }
	};
	template <> struct utf_helper<char32_t> {
		template <typename View>
		static auto conv(View str) { return as_u32(str); }
	};

	template <typename Out, typename In>
	static std::basic_string<Out> utf_as(std::basic_string_view<In> in) {
		return utf_helper<Out>::conv(in);
	}

	struct utf_conv : TestWithParam<string_convert> {
		template <typename Out, typename In>
		void ExpectUtfEq(const std::basic_string<Out>& expected, const std::basic_string<In>& tested) {
			auto actual = utf_as<Out, In>(tested);
			EXPECT_EQ(expected, actual);
		}
	};

	TEST_P(utf_conv, utf8) {
		auto[u8, u16, u32] = GetParam();
		ExpectUtfEq(u8, u16);
		ExpectUtfEq(u8, u32);
	}

	TEST_P(utf_conv, utf16) {
		auto[u8, u16, u32] = GetParam();
		ExpectUtfEq(u16, u8);
		ExpectUtfEq(u16, u32);
	}

	TEST_P(utf_conv, utf32) {
		auto[u8, u16, u32] = GetParam();
		ExpectUtfEq(u32, u8);
		ExpectUtfEq(u32, u16);
	}

	template <class ... Char>
	std::u32string utf32(Char ... chars) {
		char32_t data[] = { char32_t(chars)..., 0u };
		return data;
	}

	template <class ... Char>
	std::u16string utf16(Char ... chars) {
		char16_t data[] = { char16_t(chars)..., 0u };
		return data;
	}

	const string_convert strings[] = {
		{},
		{ "ascii", u"ascii", U"ascii" },
		{ "\x24", utf16(0x24), utf32(0x24) },
		{ "\xc2\xa2", utf16(0xa2), utf32(0xa2) },
		{ "\xe2\x82\xac", utf16(0x20ac), utf32(0x20ac) },
		{ "\xf0\x90\x8d\x88", utf16(0xd800, 0xdf48), utf32(0x10348u) },
		{
			u8"vȧĺũê\0vȧĺũêş"s,
			u"vȧĺũê\0vȧĺũêş"s,
			U"vȧĺũê\0vȧĺũêş"s
		},
		{
			u8"ŧĥê qũïçķ Ƌȓôŵñ ƒôx ĵũmpş ôvêȓ ȧ ĺȧȥÿ đôğ",
			u"ŧĥê qũïçķ Ƌȓôŵñ ƒôx ĵũmpş ôvêȓ ȧ ĺȧȥÿ đôğ",
			U"ŧĥê qũïçķ Ƌȓôŵñ ƒôx ĵũmpş ôvêȓ ȧ ĺȧȥÿ đôğ"
		},
		{
			u8"ȾĦȄ QÙÍÇĶ ßŔÖŴÑ ƑÖX ĴÙMPŞ ÖVȄŔ Ä ȽÄȤÝ ÐÖĠ",
			u"ȾĦȄ QÙÍÇĶ ßŔÖŴÑ ƑÖX ĴÙMPŞ ÖVȄŔ Ä ȽÄȤÝ ÐÖĠ",
			U"ȾĦȄ QÙÍÇĶ ßŔÖŴÑ ƑÖX ĴÙMPŞ ÖVȄŔ Ä ȽÄȤÝ ÐÖĠ"
		}
	};

	INSTANTIATE_TEST_CASE_P(strings, utf_conv, ValuesIn(strings));
}
