#include <gtest/gtest.h>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/strings.hpp>
#include <regex>
#include "diag_helper.h"

namespace lngs::app::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct pot_result {
		std::string_view input;
		std::string output;
		app::pot::info info{"HOLDER", "AUTHOR-EMAIL-ADDRESS",
		                    "JUST SO STORIES"};
	};

	struct pot_year_result {
		std::string_view input;
		std::string_view prev;
		std::string output;
		bool file_exists{true};
		app::pot::info info{"HOLDER", "AUTHOR-EMAIL-ADDRESS",
		                    "JUST SO STORIES"};
	};

	class pot : public TestWithParam<pot_result> {};
	class pot_year : public TestWithParam<pot_year_result> {};

	TEST_P(pot, text) {
		auto [input, expected_template, info] = GetParam();

		diagnostics diag;
		diag.set_contents("", input);

		idl_strings strings;
		bool idl_valid = read_strings(diag.source(""), strings, diag);
		EXPECT_TRUE(idl_valid);

		outstrstream output;
		app::pot::write(output, strings, {}, info);

		std::regex pot_creation_date{
		    R"("POT-Creation-Date: ((\d{4})-\d{2}-\d{2} \d{2}:\d{2}[+-]\d{4})\\n")"};
		std::smatch matches;
		ASSERT_TRUE(
		    std::regex_search(output.contents, matches, pot_creation_date));

		auto expected =
		    fmt::format(expected_template, fmt::arg("ThisYear", matches.str(2)),
		                fmt::arg("CurrentDate", matches.str(1)));

		EXPECT_EQ(expected, output.contents);
	}

	TEST_P(pot_year, text) {
		auto [input, prev, expected_template, file_exists, info] = GetParam();

		diagnostics diag;
		diag.set_contents("", input);

		if (file_exists) diag.set_contents("template", prev);

		idl_strings strings;
		bool idl_valid = read_strings(diag.source(""), strings, diag);
		EXPECT_TRUE(idl_valid);

		info.year = app::pot::year_from_template(diag.open("template"));

		outstrstream output;
		app::pot::write(output, strings, {}, info);

		std::regex pot_creation_date{
		    R"("POT-Creation-Date: ((\d{4})-\d{2}-\d{2} \d{2}:\d{2}[+-]\d{4})\\n")"};
		std::smatch matches;
		ASSERT_TRUE(
		    std::regex_search(output.contents, matches, pot_creation_date));

		auto expected =
		    fmt::format(expected_template, fmt::arg("ThisYear", matches.str(2)),
		                fmt::arg("CurrentDate", matches.str(1)));

		EXPECT_EQ(expected, output.contents);
	}

	const pot_result sources[] = {
	    {
	        R"([serial(0), project("name")]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(1001), plural("values")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
	[id(-1)] ID4 = "value4";
})",
	        R"(# JUST SO STORIES.
# Copyright (C) {ThisYear} HOLDER
# This file is distributed under the same license as the name package.
# AUTHOR-EMAIL-ADDRESS, {ThisYear}.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: name \n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {CurrentDate}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n != 1);\n"

#. help string
msgctxt "ID"
msgid "value"
msgid_plural "values"
msgstr[0] ""
msgstr[1] ""

msgctxt "ID2"
msgid "value2"
msgid_plural "values"
msgstr[0] ""
msgstr[1] ""

#. help string
msgctxt "ID3"
msgid "value3"
msgstr ""

msgctxt "ID4"
msgid "value4"
msgstr ""

)"},
	    {
	        R"([serial(0), project("library"), version("1.0")]
strings {
	[id(-1), help("help string")] ID = "value";
	[id(1001), help("second help string")] ID2 = "value2";
	[id(-1), help("third help string")] ID3 = "value3";
	[id(-1)] ID4 = "value4\\special chars:	\"\r\n\"\tand also: \a\b\f\v";
})",
	        R"(# JUST SO STORIES.
# Copyright (C) {ThisYear} HOLDER
# This file is distributed under the same license as the library package.
# AUTHOR-EMAIL-ADDRESS, {ThisYear}.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: library 1.0\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {CurrentDate}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

#. help string
msgctxt "ID"
msgid "value"
msgstr ""

#. second help string
msgctxt "ID2"
msgid "value2"
msgstr ""

#. third help string
msgctxt "ID3"
msgid "value3"
msgstr ""

msgctxt "ID4"
msgid "value4\\special chars:\t\"\r\n\"\tand also: \a\b\f\v"
msgstr ""

)"}};

	INSTANTIATE_TEST_SUITE_P(sources, pot, ValuesIn(sources));

	const pot_year_result year_sources[] = {
	    {
	        R"([serial(0), project("name")]
strings {
	[id(-1), help("help string"), plural("values")] ID = "value";
	[id(1001), plural("values")] ID2 = "value2";
	[id(-1), help("help string")] ID3 = "value3";
	[id(-1)] ID4 = "value4";
})",
	        R"(# JUST SO STORIES.
# Copyright (C) 2012 HOLDER
# This file is distributed under the same license as the name package.)",
	        R"(# JUST SO STORIES.
# Copyright (C) 2012 HOLDER
# This file is distributed under the same license as the name package.
# AUTHOR-EMAIL-ADDRESS, 2012.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: name \n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {CurrentDate}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n != 1);\n"

#. help string
msgctxt "ID"
msgid "value"
msgid_plural "values"
msgstr[0] ""
msgstr[1] ""

msgctxt "ID2"
msgid "value2"
msgid_plural "values"
msgstr[0] ""
msgstr[1] ""

#. help string
msgctxt "ID3"
msgid "value3"
msgstr ""

msgctxt "ID4"
msgid "value4"
msgstr ""

)"},
	    {
	        R"([serial(0), project("library"), version("1.0")]
strings {
	[id(-1), help("help string")] ID = "value";
	[id(1001), help("second help string")] ID2 = "value2";
	[id(-1), help("third help string")] ID3 = "value3";
	[id(-1)] ID4 = "value4\\special chars:	\"\r\n\"\tand also: \a\b\f\v";
})",
	        {},
	        R"(# JUST SO STORIES.
# Copyright (C) {ThisYear} HOLDER
# This file is distributed under the same license as the library package.
# AUTHOR-EMAIL-ADDRESS, {ThisYear}.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: library 1.0\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {CurrentDate}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

#. help string
msgctxt "ID"
msgid "value"
msgstr ""

#. second help string
msgctxt "ID2"
msgid "value2"
msgstr ""

#. third help string
msgctxt "ID3"
msgid "value3"
msgstr ""

msgctxt "ID4"
msgid "value4\\special chars:\t\"\r\n\"\tand also: \a\b\f\v"
msgstr ""

)"},
	    {
	        R"([serial(0), project("library"), version("1.0")]
strings {
	[id(-1), help("help string")] ID = "value";
	[id(1001), help("second help string")] ID2 = "value2";
	[id(-1), help("third help string")] ID3 = "value3";
	[id(-1)] ID4 = "value4\\special chars:	\"\r\n\"\tand also: \a\b\f\v";
})",
	        {},
	        R"(# JUST SO STORIES.
# Copyright (C) {ThisYear} HOLDER
# This file is distributed under the same license as the library package.
# AUTHOR-EMAIL-ADDRESS, {ThisYear}.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: library 1.0\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {CurrentDate}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

#. help string
msgctxt "ID"
msgid "value"
msgstr ""

#. second help string
msgctxt "ID2"
msgid "value2"
msgstr ""

#. third help string
msgctxt "ID3"
msgid "value3"
msgstr ""

msgctxt "ID4"
msgid "value4\\special chars:\t\"\r\n\"\tand also: \a\b\f\v"
msgstr ""

)",
	        false}};

	INSTANTIATE_TEST_SUITE_P(sources, pot_year, ValuesIn(year_sources));
}  // namespace lngs::app::testing
