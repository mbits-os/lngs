#include <gtest/gtest.h>
#include "diag_helper.h"
#include <lngs/internals/strings/lngs.hpp>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/strings.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/utf8.hpp>
#include "strings_helper.h"
#include "ostrstream.h"

namespace lngs {
	void PrintTo(const string_key& key, std::ostream* o) {
		*o << '(' << key.id << ',' << key.length << '@' << key.offset << ')';
	}
	bool operator==(const string_key& lhs, const string_key& rhs) {
		return lhs.id == rhs.id && lhs.length == rhs.length && lhs.offset == rhs.offset;
	}
}

namespace lngs::app {
	void PrintTo(const tr_string& s, std::ostream* o) {
		PrintTo(s.key, o);
		*o << "=\"" << straighten(s.value) << '"';
	}

	bool operator==(const tr_string& lhs, const tr_string& rhs) {
		return lhs.key == rhs.key && lhs.value == rhs.value;
	}
}

namespace lngs::app::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class Keys {
		Exclude = false,
		Include = true
	};

	enum class Result {
		Unmodifed = false,
		Warped = true
	};

	struct make_result {
		idl_strings input;
		file output;
		Keys with_keys{ Keys::Exclude };
		Result wrapped_string{ Result::Unmodifed };
	};

	struct res_make : TestWithParam<make_result> {
		struct UnexpectedStrings {
			const std::vector<tr_string>& expected;
			const std::vector<tr_string>& actual;

			static void print(std::ostream& o, char prefix, const std::vector<tr_string>& list, size_t skip) {
				for (size_t i = skip; i < list.size(); ++i) {
					const auto& str = list[i];
					o << "  " << prefix;
					PrintTo(str.key, &o);
					o << "=\"" << straighten(str.value) << "\"\n";
				}
			}

			friend std::ostream& operator<<(std::ostream& o, const UnexpectedStrings& data) {
				if (data.actual.size() > data.expected.size())
					UnexpectedStrings::print(o, '+', data.actual, data.expected.size());
				else
					UnexpectedStrings::print(o, '-', data.expected, data.actual.size());
				return o;
			}
		};

		struct Bytes {
			const std::string& expected;
			const std::string& actual;

			static void print(std::ostream& o, const std::string& s) {
				o << std::hex;
				for (auto c : utf::as_u32(s)) {
					if (c <= 127) {
						o << (char)c;
						continue;
					}

					o << '\\';
					if (c <= 0xFFFF)
						o << 'u' << std::setw(4);
					else
						o << 'U' << std::setw(8);

					o << std::setfill('0') << (int)c;
				}
				o << '\n' << std::dec;
			}
		};

		friend std::ostream& operator<<(std::ostream& o, const Bytes b) {
			Bytes::print(o, b.actual);
			return o;
		}
	};

	TEST_P(res_make, convert) {
		auto[input, expected, with_keys, wrapped_strings] = GetParam();

		auto file = res::make_resource(input, wrapped_strings == Result::Warped, with_keys == Keys::Include);

		EXPECT_EQ(expected.serial, file.serial);
#define EXPECT_STRINGS(name) \
		EXPECT_EQ(expected.name.size(), file.name.size()) << UnexpectedStrings{ expected.name, file.name }; \
		{ \
			size_t max = std::min(expected.name.size(), file.name.size()); \
			for (size_t i = 0; i < max; ++i) { \
				EXPECT_EQ(expected.name[i], file.name[i]) << Bytes{expected.name[i].value, file.name[i].value}; \
				EXPECT_EQ(utf::as_u32(expected.name[i].value), utf::as_u32(file.name[i].value)); \
			} \
		}
		EXPECT_STRINGS(strings);
		EXPECT_STRINGS(keys);
		EXPECT_STRINGS(attrs);
	}

	inline tr_string make_str(uint32_t id, std::string val) {
		return { id, std::move(val) };
	}

	const make_result make_strings[] = {
		{
		},
		{
			test_strings(1234567).make(
				test_string(1001, "ID", "value")
			),
			{
				1234567,
				{},
				{
					make_str(1001, "value")
				}
			}
		},
		{
			test_strings(1234567).make(
				test_string(1001, "ID", "value")
			),
			{
				1234567,
				{},
				{
					make_str(1001, "value")
				},
				{
					make_str(1001, "ID")
				}
			},
			Keys::Include
		},
		{
			test_strings(1234567).make(
				test_string(1001, "ID", "value", PluralStr{"values"})
			),
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, "value\0values"s)
				}
			}
		},
		{
			test_strings(1234567).make(
				test_string(1001, "ID", "value", PluralStr{"values"})
			),
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, "value\0values"s)
				},
				{
					make_str(1001, "ID")
				}
			},
			Keys::Include
		},
		{
			test_strings(1234567).make(
				test_string(1001, "ID", "value", PluralStr{"values"}),
				test_string(1002, "ID_PANGRAM_LOWER", "the quick brown fox jumps over a lazy dog"),
				test_string(1003, "ID_PANGRAM_UPPER", "THE QUICK BROWN FOX JUMPS OVER A LAZY DOG")
			),
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, u8"v\u0227\u013a\u0169\u00ea\0v\u0227\u013a\u0169\u00ea\u015f"s),
					make_str(1002, u8"\u0167\u0125\u00ea q\u0169\u00ef\u00e7\u0137 \u018b\u0213\u00f4\u0175\u00f1 "
								   u8"\u0192\u00f4x \u0135\u0169mp\u015f \u00f4v\u00ea\u0213 \u0227 "
								   u8"\u013a\u0227\u0225\u00ff \u0111\u00f4\u011f"),
					make_str(1003, u8"\u023e\u0126\u0204 Q\u00d9\u00cd\u00c7\u0136 \u00df\u0154\u00d6\u0174\u00d1 "
								   u8"\u0191\u00d6X \u0134\u00d9MP\u015e \u00d6V\u0204\u0154 \u00c4 "
								   u8"\u023d\u00c4\u0224\u00dd \u00d0\u00d6\u0120")
				}
			},
			Keys::Exclude,
			Result::Warped
		},
		{
			test_strings(1234567).make(
				test_string(1001, "ID", "value", PluralStr{"values"}),
				test_string(1002, "ID_PANGRAM_LOWER", "the quick brown fox jumps over a lazy dog \"01234569\""),
				test_string(1003, "ID_PANGRAM_UPPER", "THE QUICK BROWN FOX JUMPS OVER A LAZY DOG \"01234569\"")
			),
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, u8"v\u0227\u013a\u0169\u00ea\0v\u0227\u013a\u0169\u00ea\u015f"s),
					make_str(1002, u8"\u0167\u0125\u00ea q\u0169\u00ef\u00e7\u0137 \u018b\u0213\u00f4\u0175\u00f1 "
								   u8"\u0192\u00f4x \u0135\u0169mp\u015f \u00f4v\u00ea\u0213 \u0227 "
								   u8"\u013a\u0227\u0225\u00ff \u0111\u00f4\u011f ?01234569?"),
					make_str(1003, u8"\u023e\u0126\u0204 Q\u00d9\u00cd\u00c7\u0136 \u00df\u0154\u00d6\u0174\u00d1 "
								   u8"\u0191\u00d6X \u0134\u00d9MP\u015e \u00d6V\u0204\u0154 \u00c4 "
								   u8"\u023d\u00c4\u0224\u00dd \u00d0\u00d6\u0120 ?01234569?")
				},
				{
					make_str(1001, "ID"),
					make_str(1002, "ID_PANGRAM_LOWER"),
					make_str(1003, "ID_PANGRAM_UPPER")
				}
			},
			Keys::Include,
			Result::Warped
		}
	};

	INSTANTIATE_TEST_SUITE_P(strings, res_make, ValuesIn(make_strings));

	struct write_result {
		file input;
		std::string_view include, project;
		std::string output;
	};

	struct partial_ostrstream : outstream {
		const size_t chars;
		std::string contents;

		partial_ostrstream(size_t chars) : chars{ chars } {}

		std::size_t write(const void* data, std::size_t length) noexcept final
		{
			auto rest = chars > contents.size() ? chars - contents.size() : 0;
			if (length > rest)
				length = rest;

			auto b = static_cast<const char*>(data);
			auto e = b + length;
			auto size = contents.size();
			contents.insert(end(contents), b, e);
			return contents.size() - size;
		}
	};

	struct res_write : TestWithParam<write_result> {};

	TEST_P(res_write, compile) {
		auto[input, include, project, expected] = GetParam();

		outstrstream actual;
		res::update_and_write(actual, input, include, project);
		EXPECT_EQ(expected, actual.contents);
	}

	TEST_P(res_write, partial_middle) {
		auto[input, include, project, expected] = GetParam();

		const auto pos1 = expected.find("const char __resource[] = {") + 27;
		const auto pos2 = expected.find("}; // __resource", pos1);

		partial_ostrstream actual{ pos1 + (pos2 - pos1) / 2 };
		res::update_and_write(actual, input, include, project);
		EXPECT_EQ(expected.substr(0, actual.chars), actual.contents);
	}

	TEST_P(res_write, partial_start) {
		auto[input, include, project, expected] = GetParam();

		const auto pos = expected.find("const char __resource[] = {") + 30;

		partial_ostrstream actual{ pos };
		res::update_and_write(actual, input, include, project);
		EXPECT_EQ(expected.substr(0, actual.chars), actual.contents);
	}

	TEST_P(res_write, partial_endline) {
		auto[input, include, project, expected] = GetParam();

		const auto pos1 = expected.find("const char __resource[] = {") + 27;
		const auto pos2 = expected.find("}; // __resource", pos1);
		const auto pos = expected.rfind("\"\n", pos2);

		partial_ostrstream actual{ pos };
		res::update_and_write(actual, input, include, project);
		EXPECT_EQ(expected.substr(0, actual.chars), actual.contents);
	}

	TEST_P(res_write, partial_finalize) {
		auto[input, include, project, expected] = GetParam();

		const auto pos1 = expected.find("const char __resource[] = {") + 27;
		const auto pos = expected.find("\"\n", pos1) + 1;

		partial_ostrstream actual{ pos };
		res::update_and_write(actual, input, include, project);
		EXPECT_EQ(expected.substr(0, actual.chars), actual.contents);
	}

	const write_result write_resources[] = {
		{
			{}, {}, {},
			R"(// THIS FILE IS AUTOGENERATED
#include ""

namespace  {
    namespace {
        const char __resource[] = {
            "\x4c\x41\x4e\x47\x20\x68\x64\x72\x02\x00\x00\x00\x00\x01\x00\x00"
            "\x00\x00\x00\x00\x6c\x61\x73\x74\x00\x00\x00\x00"
        }; // __resource
    } // namespace

    /*static*/ const char* Resource::data() { return __resource; }
    /*static*/ std::size_t Resource::size() { return sizeof(__resource); }
} // namespace 
)"
		},
		{
			{}, "enums.h", "project",
			R"(// THIS FILE IS AUTOGENERATED
#include "enums.h"

namespace project {
    namespace {
        const char __resource[] = {
            "\x4c\x41\x4e\x47\x20\x68\x64\x72\x02\x00\x00\x00\x00\x01\x00\x00"
            "\x00\x00\x00\x00\x6c\x61\x73\x74\x00\x00\x00\x00"
        }; // __resource
    } // namespace

    /*static*/ const char* Resource::data() { return __resource; }
    /*static*/ std::size_t Resource::size() { return sizeof(__resource); }
} // namespace project
)"
		},
		{
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, u8"value\0values"s),
					make_str(1002, u8"the quick brown fox jumps over a lazy dog"),
					make_str(1003, u8"THE QUICK BROWN FOX JUMPS OVER A LAZY DOG")
				}
			}, "enums.h", "project",
			R"(// THIS FILE IS AUTOGENERATED
#include "enums.h"

namespace project {
    namespace {
        const char __resource[] = {
            "\x4c\x41\x4e\x47\x20\x68\x64\x72\x02\x00\x00\x00\x00\x01\x00\x00"
            "\x87\xd6\x12\x00\x61\x74\x74\x72\x0d\x00\x00\x00\x01\x00\x00\x00"
            "\x07\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x1c\x00\x00\x00"
            "\x6e\x70\x6c\x75\x72\x61\x6c\x73\x3d\x32\x3b\x20\x70\x6c\x75\x72"
            "\x61\x6c\x3d\x28\x6e\x20\x21\x3d\x20\x31\x29\x3b\x00\x00\x00\x00"
            "\x73\x74\x72\x73\x24\x00\x00\x00\x03\x00\x00\x00\x0d\x00\x00\x00"
            "\xe9\x03\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\xea\x03\x00\x00"
            "\x0d\x00\x00\x00\x29\x00\x00\x00\xeb\x03\x00\x00\x37\x00\x00\x00"
            "\x29\x00\x00\x00\x76\x61\x6c\x75\x65\x00\x76\x61\x6c\x75\x65\x73"
            "\x00\x74\x68\x65\x20\x71\x75\x69\x63\x6b\x20\x62\x72\x6f\x77\x6e"
            "\x20\x66\x6f\x78\x20\x6a\x75\x6d\x70\x73\x20\x6f\x76\x65\x72\x20"
            "\x61\x20\x6c\x61\x7a\x79\x20\x64\x6f\x67\x00\x54\x48\x45\x20\x51"
            "\x55\x49\x43\x4b\x20\x42\x52\x4f\x57\x4e\x20\x46\x4f\x58\x20\x4a"
            "\x55\x4d\x50\x53\x20\x4f\x56\x45\x52\x20\x41\x20\x4c\x41\x5a\x59"
            "\x20\x44\x4f\x47\x00\x00\x00\x00\x6c\x61\x73\x74\x00\x00\x00\x00"
        }; // __resource
    } // namespace

    /*static*/ const char* Resource::data() { return __resource; }
    /*static*/ std::size_t Resource::size() { return sizeof(__resource); }
} // namespace project
)"
		},
		{
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, u8"v\u0227\u013a\u0169\u00ea\0v\u0227\u013a\u0169\u00ea\u015f"s),
					make_str(1002, u8"\u0167\u0125\u00ea q\u0169\u00ef\u00e7\u0137 \u018b\u0213\u00f4\u0175\u00f1 "
								   u8"\u0192\u00f4x \u0135\u0169mp\u015f \u00f4v\u00ea\u0213 \u0227 "
								   u8"\u013a\u0227\u0225\u00ff \u0111\u00f4\u011f"),
					make_str(1003, u8"\u023e\u0126\u0204 Q\u00d9\u00cd\u00c7\u0136 \u00df\u0154\u00d6\u0174\u00d1 "
								   u8"\u0191\u00d6X \u0134\u00d9MP\u015e \u00d6V\u0204\u0154 \u00c4 "
								   u8"\u023d\u00c4\u0224\u00dd \u00d0\u00d6\u0120")
				},
				{
					make_str(1001, "ID"),
					make_str(1002, "ID_PANGRAM_LOWER"),
					make_str(1003, "ID_PANGRAM_UPPER")
				}
			}, "enums.h", "project",
			R"(// THIS FILE IS AUTOGENERATED
#include "enums.h"

namespace project {
    namespace {
        const char __resource[] = {
            "\x4c\x41\x4e\x47\x20\x68\x64\x72\x02\x00\x00\x00\x00\x01\x00\x00"
            "\x87\xd6\x12\x00\x61\x74\x74\x72\x0d\x00\x00\x00\x01\x00\x00\x00"
            "\x07\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x1c\x00\x00\x00"
            "\x6e\x70\x6c\x75\x72\x61\x6c\x73\x3d\x32\x3b\x20\x70\x6c\x75\x72"
            "\x61\x6c\x3d\x28\x6e\x20\x21\x3d\x20\x31\x29\x3b\x00\x00\x00\x00"
            "\x73\x74\x72\x73\x34\x00\x00\x00\x03\x00\x00\x00\x0d\x00\x00\x00"
            "\xe9\x03\x00\x00\x00\x00\x00\x00\x15\x00\x00\x00\xea\x03\x00\x00"
            "\x16\x00\x00\x00\x45\x00\x00\x00\xeb\x03\x00\x00\x5c\x00\x00\x00"
            "\x45\x00\x00\x00\x76\xc8\xa7\xc4\xba\xc5\xa9\xc3\xaa\x00\x76\xc8"
            "\xa7\xc4\xba\xc5\xa9\xc3\xaa\xc5\x9f\x00\xc5\xa7\xc4\xa5\xc3\xaa"
            "\x20\x71\xc5\xa9\xc3\xaf\xc3\xa7\xc4\xb7\x20\xc6\x8b\xc8\x93\xc3"
            "\xb4\xc5\xb5\xc3\xb1\x20\xc6\x92\xc3\xb4\x78\x20\xc4\xb5\xc5\xa9"
            "\x6d\x70\xc5\x9f\x20\xc3\xb4\x76\xc3\xaa\xc8\x93\x20\xc8\xa7\x20"
            "\xc4\xba\xc8\xa7\xc8\xa5\xc3\xbf\x20\xc4\x91\xc3\xb4\xc4\x9f\x00"
            "\xc8\xbe\xc4\xa6\xc8\x84\x20\x51\xc3\x99\xc3\x8d\xc3\x87\xc4\xb6"
            "\x20\xc3\x9f\xc5\x94\xc3\x96\xc5\xb4\xc3\x91\x20\xc6\x91\xc3\x96"
            "\x58\x20\xc4\xb4\xc3\x99\x4d\x50\xc5\x9e\x20\xc3\x96\x56\xc8\x84"
            "\xc5\x94\x20\xc3\x84\x20\xc8\xbd\xc3\x84\xc8\xa4\xc3\x9d\x20\xc3"
            "\x90\xc3\x96\xc4\xa0\x00\x00\x00\x6b\x65\x79\x73\x15\x00\x00\x00"
            "\x03\x00\x00\x00\x0d\x00\x00\x00\xe9\x03\x00\x00\x00\x00\x00\x00"
            "\x02\x00\x00\x00\xea\x03\x00\x00\x03\x00\x00\x00\x10\x00\x00\x00"
            "\xeb\x03\x00\x00\x14\x00\x00\x00\x10\x00\x00\x00\x49\x44\x00\x49"
            "\x44\x5f\x50\x41\x4e\x47\x52\x41\x4d\x5f\x4c\x4f\x57\x45\x52\x00"
            "\x49\x44\x5f\x50\x41\x4e\x47\x52\x41\x4d\x5f\x55\x50\x50\x45\x52"
            "\x00\x00\x00\x00\x6c\x61\x73\x74\x00\x00\x00\x00"
        }; // __resource
    } // namespace

    /*static*/ const char* Resource::data() { return __resource; }
    /*static*/ std::size_t Resource::size() { return sizeof(__resource); }
} // namespace project
)"
		},
		{
			{
				1234567,
				{
					make_str(ATTR_PLURALS, "nplurals=2; plural=(n != 1);")
				},
				{
					make_str(1001, u8"v\u0227\u013a\u0169\u00ea\0v\u0227\u013a\u0169\u00ea\u015f"s),
					make_str(1002, u8"\u0167\u0125\u00ea q\u0169\u00ef\u00e7\u0137 \u018b\u0213\u00f4\u0175\u00f1 "
								   u8"\u0192\u00f4x \u0135\u0169mp\u015f \u00f4v\u00ea\u0213 \u0227 "
								   u8"\u013a\u0227\u0225\u00ff \u0111\u00f4\u011f"),
					make_str(1003, u8"\u023e\u0126\u0204 Q\u00d9\u00cd\u00c7\u0136 \u00df\u0154\u00d6\u0174\u00d1 "
								   u8"\u0191\u00d6X \u0134\u00d9MP\u015e \u00d6V\u0204\u0154 \u00c4 "
								   u8"\u023d\u00c4\u0224\u00dd \u00d0\u00d6\u0120")
				}
			}, "enums.h", "project",
			R"(// THIS FILE IS AUTOGENERATED
#include "enums.h"

namespace project {
    namespace {
        const char __resource[] = {
            "\x4c\x41\x4e\x47\x20\x68\x64\x72\x02\x00\x00\x00\x00\x01\x00\x00"
            "\x87\xd6\x12\x00\x61\x74\x74\x72\x0d\x00\x00\x00\x01\x00\x00\x00"
            "\x07\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x1c\x00\x00\x00"
            "\x6e\x70\x6c\x75\x72\x61\x6c\x73\x3d\x32\x3b\x20\x70\x6c\x75\x72"
            "\x61\x6c\x3d\x28\x6e\x20\x21\x3d\x20\x31\x29\x3b\x00\x00\x00\x00"
            "\x73\x74\x72\x73\x34\x00\x00\x00\x03\x00\x00\x00\x0d\x00\x00\x00"
            "\xe9\x03\x00\x00\x00\x00\x00\x00\x15\x00\x00\x00\xea\x03\x00\x00"
            "\x16\x00\x00\x00\x45\x00\x00\x00\xeb\x03\x00\x00\x5c\x00\x00\x00"
            "\x45\x00\x00\x00\x76\xc8\xa7\xc4\xba\xc5\xa9\xc3\xaa\x00\x76\xc8"
            "\xa7\xc4\xba\xc5\xa9\xc3\xaa\xc5\x9f\x00\xc5\xa7\xc4\xa5\xc3\xaa"
            "\x20\x71\xc5\xa9\xc3\xaf\xc3\xa7\xc4\xb7\x20\xc6\x8b\xc8\x93\xc3"
            "\xb4\xc5\xb5\xc3\xb1\x20\xc6\x92\xc3\xb4\x78\x20\xc4\xb5\xc5\xa9"
            "\x6d\x70\xc5\x9f\x20\xc3\xb4\x76\xc3\xaa\xc8\x93\x20\xc8\xa7\x20"
            "\xc4\xba\xc8\xa7\xc8\xa5\xc3\xbf\x20\xc4\x91\xc3\xb4\xc4\x9f\x00"
            "\xc8\xbe\xc4\xa6\xc8\x84\x20\x51\xc3\x99\xc3\x8d\xc3\x87\xc4\xb6"
            "\x20\xc3\x9f\xc5\x94\xc3\x96\xc5\xb4\xc3\x91\x20\xc6\x91\xc3\x96"
            "\x58\x20\xc4\xb4\xc3\x99\x4d\x50\xc5\x9e\x20\xc3\x96\x56\xc8\x84"
            "\xc5\x94\x20\xc3\x84\x20\xc8\xbd\xc3\x84\xc8\xa4\xc3\x9d\x20\xc3"
            "\x90\xc3\x96\xc4\xa0\x00\x00\x00\x6c\x61\x73\x74\x00\x00\x00\x00"
        }; // __resource
    } // namespace

    /*static*/ const char* Resource::data() { return __resource; }
    /*static*/ std::size_t Resource::size() { return sizeof(__resource); }
} // namespace project
)"
		},
	};

	INSTANTIATE_TEST_SUITE_P(resources, res_write, ValuesIn(write_resources));

	TEST(res_read, builtin) {
		SingularStrings<lngs::app::lng, storage::Builtin<Resource>> res;
		ASSERT_TRUE(res.init());
		ASSERT_FALSE(res(lng::ERR_EXPECTED).empty());
	}
}
