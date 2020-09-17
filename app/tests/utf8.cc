#include <gtest/gtest.h>
#include <lngs/internals/utf8.hpp>

namespace utf::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class op {
		none,
		utf8to16,
		utf8to32,
		utf16to8,
		utf16to32,
		utf32to8,
		utf32to16,
	};

	struct string_convert {
		const std::string utf8;
		const std::u16string utf16;
		const std::u32string utf32;
		op oper{op::none};
	};

	template <typename Out>
	struct utf_helper;
	template <>
	struct utf_helper<char> {
		template <typename View>
		static auto conv(View str) {
			return as_u8(str);
		}
	};
	template <>
	struct utf_helper<char16_t> {
		template <typename View>
		static auto conv(View str) {
			return as_u16(str);
		}
	};
	template <>
	struct utf_helper<char32_t> {
		template <typename View>
		static auto conv(View str) {
			return as_u32(str);
		}
	};

	template <typename Out, typename In>
	static std::basic_string<Out> utf_as(std::basic_string_view<In> in) {
		return utf_helper<Out>::conv(in);
	}

	struct utf_conv : TestWithParam<string_convert> {
		template <typename Out, typename In>
		void ExpectUtfEq(const std::basic_string<Out>& expected,
		                 const std::basic_string<In>& tested) {
			auto actual = utf_as<Out, In>(tested);
			EXPECT_EQ(expected, actual);
		}
	};

	struct utf_errors : utf_conv {};

	TEST_P(utf_conv, utf8) {
		auto [u8, u16, u32, ign] = GetParam();
		ExpectUtfEq(u8, u16);
		ExpectUtfEq(u8, u32);
	}

	TEST_P(utf_conv, utf16) {
		auto [u8, u16, u32, ign] = GetParam();
		ExpectUtfEq(u16, u8);
		ExpectUtfEq(u16, u32);
	}

	TEST_P(utf_conv, utf32) {
		auto [u8, u16, u32, ign] = GetParam();
		ExpectUtfEq(u32, u8);
		ExpectUtfEq(u32, u16);
	}

	TEST_P(utf_errors, check) {
		auto [u8, u16, u32, oper] = GetParam();
		switch (oper) {
			case op::utf8to16:
				ExpectUtfEq(u16, u8);
				break;
			case op::utf8to32:
				ExpectUtfEq(u32, u8);
				break;
			case op::utf16to8:
				ExpectUtfEq(u8, u16);
				break;
			case op::utf16to32:
				ExpectUtfEq(u32, u16);
				break;
			case op::utf32to8:
				ExpectUtfEq(u8, u32);
				break;
			case op::utf32to16:
				ExpectUtfEq(u16, u32);
				break;
			default:
				break;
		};
	}

	template <class... Char>
	std::u32string utf32(Char... chars) {
		char32_t data[] = {static_cast<char32_t>(chars)..., 0u};
		return data;
	}

	template <class... Char>
	std::u16string utf16(Char... chars) {
		char16_t data[] = {static_cast<char16_t>(chars)..., 0u};
		return data;
	}

	template <class Char>
	char u2s(Char c) {
		return static_cast<char>(static_cast<unsigned char>(c));
	}
	template <class... Char>
	std::string utf8(Char... chars) {
		char data[] = {u2s(chars)..., 0u};
		return data;
	}

	const string_convert strings[] = {
	    {},
	    {"ascii", u"ascii", U"ascii"},
	    {"\x24", utf16(0x24), utf32(0x24)},
	    {"\xc2\xa2", utf16(0xa2), utf32(0xa2)},
	    {"\xe2\x82\xac", utf16(0x20ac), utf32(0x20ac)},
	    {"\xf0\x90\x8d\x88", utf16(0xd800, 0xdf48), utf32(0x10348u)},
	    {u8"vȧĺũê\0vȧĺũêş"s, u"vȧĺũê\0vȧĺũêş"s, U"vȧĺũê\0vȧĺũêş"s},
	    {u8"ŧĥê qũïçķ Ƌȓôŵñ ƒôx ĵũmpş ôvêȓ ȧ ĺȧȥÿ đôğ",
	     u"ŧĥê qũïçķ Ƌȓôŵñ ƒôx ĵũmpş ôvêȓ ȧ ĺȧȥÿ đôğ",
	     U"ŧĥê qũïçķ Ƌȓôŵñ ƒôx ĵũmpş ôvêȓ ȧ ĺȧȥÿ đôğ"},
	    {u8"ȾĦȄ QÙÍÇĶ ßŔÖŴÑ ƑÖX ĴÙMPŞ ÖVȄŔ Ä ȽÄȤÝ ÐÖĠ",
	     u"ȾĦȄ QÙÍÇĶ ßŔÖŴÑ ƑÖX ĴÙMPŞ ÖVȄŔ Ä ȽÄȤÝ ÐÖĠ",
	     U"ȾĦȄ QÙÍÇĶ ßŔÖŴÑ ƑÖX ĴÙMPŞ ÖVȄŔ Ä ȽÄȤÝ ÐÖĠ"}};

	INSTANTIATE_TEST_SUITE_P(strings, utf_conv, ValuesIn(strings));

	const string_convert bad[] = {
	    {utf8('a', 'b', 0xe0, 0x9f, 0x9f), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xed, 0xa0, 0xa0), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf0, 0x8f, 0x8f, 0x8f), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf4, 0x90, 0x90, 0x90), {}, {}, op::utf8to32},

	    {utf8('a', 'b', 0xef, 0xbf, 0xbd, 'c', 'd'),
	     {},
	     utf32('a', 'b', 0x00110000u, 'c', 'd'),
	     op::utf32to8},
	    {{},
	     utf16('a', 'b', 0xFFFD, 'c', 'd'),
	     utf32('a', 'b', 0x00110000u, 'c', 'd'),
	     op::utf32to16},
	    {utf8('a', 'b', 0xf4, 0x8f, 0xbf, 0xbf, 'c', 'd'),
	     {},
	     utf32('a', 'b', 0x0010FFFFu, 'c', 'd'),
	     op::utf32to8},
	    {{},
	     utf16('a', 'b', 0xDBFF, 0xDFFF, 'c', 'd'),
	     utf32('a', 'b', 0x0010FFFFu, 'c', 'd'),
	     op::utf32to16},
	    {utf8('a', 'b', 0xef, 0xbf, 0xbd, 'c', 'd'),
	     {},
	     utf32('a', 'b', 0xD811, 'c', 'd'),
	     op::utf32to8},
	    {{},
	     utf16('a', 'b', 0xFFFD, 'c', 'd'),
	     utf32('a', 'b', 0xD811, 'c', 'd'),
	     op::utf32to16},
	    {utf8('a', 'b', 0xef, 0xbf, 0xbd, 'c', 'd'),
	     {},
	     utf32('a', 'b', 0x00110000u, 'c', 'd'),
	     op::utf32to8},

	    // timer clock: U+23F2 E2:8F:B2
	    {utf8('a', 'b', 0xe2, 0x8f), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe2, 0x8f, '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xff, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xfe, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xfd, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xfc, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xfb, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xfa, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf0, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf1, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf2, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf3, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf4, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf3, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf2, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf1, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xf0, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xef, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xee, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xed, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xec, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xeb, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xea, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe0, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe9, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe8, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe7, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe6, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe5, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe4, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe3, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe1, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xe0, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xdf, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xde, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xdd, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xdc, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xdb, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xda, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd0, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd9, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd8, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd7, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd6, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd5, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd4, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd3, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd1, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	    {utf8('a', 'b', 0xd0, '-', '-', '-', '-', '-'), {}, {}, op::utf8to32},
	};

	INSTANTIATE_TEST_SUITE_P(bad, utf_errors, ValuesIn(bad));

	const string_convert those_four[] = {
		{utf8(0xe0, 0x9f, 0x80), {}, {}, op::utf8to32},
		{utf8(0xe0, 0xa0, 0x80), {}, utf32(0x0800u), op::utf8to32},
		{utf8(0xe0, 0xa1, 0x80), {}, utf32(0x0840u), op::utf8to32},

		{utf8(0xed, 0x9e, 0x80), {}, utf32(0xD780u), op::utf8to32},
		{utf8(0xed, 0x9f, 0x80), {}, utf32(0xD7C0u), op::utf8to32},
		{utf8(0xed, 0xa0, 0x80), {}, {}, op::utf8to32},

		{utf8(0xf0, 0x8f, 0x80, 0x80), {}, {}, op::utf8to32},
		{utf8(0xf0, 0x90, 0x80, 0x80), {}, utf32(0x10000u), op::utf8to32},
		{utf8(0xf0, 0x91, 0x80, 0x80), {}, utf32(0x11000u), op::utf8to32},

		{utf8(0xf4, 0x8e, 0x80, 0x80), {}, utf32(0x10E000u), op::utf8to32},
		{utf8(0xf4, 0x8f, 0x80, 0x80), {}, utf32(0x10F000u), op::utf8to32},
		{utf8(0xf4, 0x90, 0x80, 0x80), {}, {}, op::utf8to32},

		{utf8(0x80), {}, {}, op::utf8to32},
	};
	INSTANTIATE_TEST_SUITE_P(those_four, utf_errors, ValuesIn(those_four));
}  // namespace utf::testing
