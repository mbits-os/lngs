#include <gtest/gtest.h>
#include "diag_helper.h"
#include <lngs/internals/commands.hpp>
#include <lngs/internals/strings.hpp>

namespace lngs::app::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct frozen_result {
		std::string_view input;
		std::string output;
		bool found_new = true;
	};

	class frozen : public TestWithParam<frozen_result> {};

	TEST_P(frozen, text) {
		auto[input, expected, found_new] = GetParam();

		diagnostics diag;
		diag.set_contents("", input);

		idl_strings strings;
		bool idl_valid = read_strings(diag.source(""), strings, diag);
		EXPECT_TRUE(idl_valid);

		auto frozen = freeze::freeze(strings);
		EXPECT_EQ(found_new, frozen);

		outstrstream output;
		auto src = diag.source("");
		freeze::write(output, strings, src);
		EXPECT_EQ(expected, output.contents);
	}

	const frozen_result sources[] = {
		{
			R"([serial(0)]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(1001), plural("values"), help("help string")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
})",
			R"([serial(1)]
strings {
	[id(1002), help("help string"), plural("values")] ID = "value";
	[id(1001), plural("values"), help("help string")] ID2 = "value2";
	[id(1003), help("help string")] ID3 = "value3";
})"
		},
		{
			R"([serial(0)]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(10), plural("values"), help("help string")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
})",
			R"([serial(1)]
strings {
	[id(1001), help("help string"), plural("values")] ID = "value";
	[id(10), plural("values"), help("help string")] ID2 = "value2";
	[id(1002), help("help string")] ID3 = "value3";
})"
		},
		{
			R"([serial(23)]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(10000), plural("values"), help("help string")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
})",
			R"([serial(24)]
strings {
	[id(10001), help("help string"), plural("values")] ID = "value";
	[id(10000), plural("values"), help("help string")] ID2 = "value2";
	[id(10002), help("help string")] ID3 = "value3";
})"
		},
		{
			R"([serial(0)]
strings {
	[id(1001), help("help string"), plural("values")] ID = "value";
	[id(1002), plural("values"), help("help string")] ID2 = "value2";
	[id(1003), help("help string")] ID3 = "value3";
})",
			R"([serial(0)]
strings {
	[id(1001), help("help string"), plural("values")] ID = "value";
	[id(1002), plural("values"), help("help string")] ID2 = "value2";
	[id(1003), help("help string")] ID3 = "value3";
})",
			false
		},
	};

	INSTANTIATE_TEST_SUITE_P(sources, frozen, ValuesIn(sources));
}
