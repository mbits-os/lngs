#include <gtest/gtest.h>
#include "diag_helper.h"
#include <string_view>
#include <numeric>

extern fs::path TESTING_data_path;

namespace lngs::testing {
	using namespace ::std::literals;
	using ::testing::Test;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	class source_location : public Test {
	protected:
		struct item {
			std::string_view path;
			location loc;
			item(std::string_view path) : path{ path } {}
		};

		std::vector<item> data = {
			// GCC
			{ "/usr/include/c++/8"sv },
			{ "/usr/include/x86_64-linux-gnu/c++/8"sv },
			{ "/usr/include/c++/8/backward"sv },
			{ "/usr/lib/gcc/x86_64-linux-gnu/8/include"sv },
			{ "/usr/local/include"sv },
			{ "/usr/lib/gcc/x86_64-linux-gnu/8/include-fixed"sv },
			{ "/usr/include/x86_64-linux-gnu"sv },
			{ "/usr/include"sv },
			// Clang
			{ "/usr/include/c++/v1"sv },
			{ "/usr/include/clang/7.0.0/include"sv },
			{ "/usr/local/include"sv },
			{ "/usr/lib/gcc/x86_64-linux-gnu/8/include"sv },
			{ "/usr/include/x86_64-linux-gnu"sv },
			{ "/usr/include"sv },
		};
		diagnostics diag;
	public:
		void SetUp() override {
			for (auto& name : data) {
				name.loc = diag.source(name.path).position();
			}
		}
	};

	TEST_F(source_location, unique) {
		for (auto
				begin_iter = begin(data),
				end_iter = end(data),
				lhs_iter = begin_iter;
			lhs_iter != end_iter; ++lhs_iter)
		{
			const auto& lhs = *lhs_iter;
			for (auto rhs_iter = begin_iter; rhs_iter != lhs_iter; ++rhs_iter) {
				const auto& rhs = *rhs_iter;

				if (lhs.path == rhs.path) {
					ASSERT_EQ(lhs.loc.token, rhs.loc.token);
				} else {
					ASSERT_NE(lhs.loc.token, rhs.loc.token);
				}
			}
		}
	}

	TEST_F(source_location, reverse) {
		for (auto const& name : data) {
			// printf("%.*s --> %u\n", (int)name.path.length(), name.path.data(), name.loc.token);
			ASSERT_EQ(name.path, diag.filename(name.loc));
		}
	}

	struct diagnostic_write {
		expected_diagnostic value;
		std::initializer_list<const char*> expected;
	};

	std::ostream& operator<<(std::ostream& o, const diagnostic_write& diag) {
		return o << diag.value;
	}
	class diag_write : public TestWithParam<diagnostic_write> {};

	TEST_F(diag_write, lines) {
		static constexpr const std::string_view raven[] = {
			"Then this ebony bird beguiling my sad fancy into smiling,"sv,
			"By the grave and stern decorum of the countenance it wore,"sv,
			"\"Though thy crest be shorn and shaven, thou,\" I said, \"art sure no craven,"sv,
			"Ghastly grim and ancient Raven wandering from the Nightly shore-"sv,
			"Tell me what thy lordly name is on the Night's Plutonian shore!\""sv,
			"               Quoth the Raven \"Nevermore.\""sv,
		};

		diagnostics diag;
		auto src = diag.open((TESTING_data_path / "the_raven.txt").string());

		EXPECT_TRUE(src.valid());
		EXPECT_EQ(src.line(0), ""sv) << "Line: 0";

		for (size_t ln = 2 * std::size(raven); ln > 0; --ln) {
			if (ln <= std::size(raven)) {
				EXPECT_EQ(src.line(ln), raven[ln - 1]) << "Line: " << ln;
			}
			else {
				EXPECT_EQ(src.line(ln), ""sv) << "Line: " << ln;
			}
		}
	}

	TEST_F(diag_write, lines_not_found) {
		diagnostics diag;
		auto src = diag.open((TESTING_data_path / "no-such.txt").string());

		EXPECT_FALSE(src.valid());
		EXPECT_EQ(src.line(0), ""sv) << "Line: 0";

		for (size_t ln = 12; ln > 0; --ln) {
			EXPECT_EQ(src.line(ln), ""sv) << "Line: " << ln;
		}
		EXPECT_TRUE(src.valid());
	}

	TEST_F(diag_write, const_lookup) {
		diagnostics diag;
		const diagnostics& ref = diag;

		auto src = ref.source("");
		EXPECT_EQ(0u, src.position().token);
		EXPECT_EQ(""sv, ref.filename({ 2 }));
	}

	diagnostic conv(diagnostics& diag, const expected_diagnostic& exp) {
		const auto loc = diag.source(exp.filename).position(exp.line, exp.column);
		auto end = exp.column_end ? loc.moved(loc.line, exp.column_end) : location{};

		auto d = loc/end[exp.sev] << exp.message.arg();
		d.children.reserve(exp.subs.size());
		for (auto const& child : exp.subs)
			d.children.push_back(conv(diag, child));
		return d;
	}

	TEST_P(diag_write, print) {
		auto [value, exp_str] = GetParam();
		diagnostics diag;

		outstrstream actual;
		strings_mock tr;
		alt_strings_mock tr_alt;
		auto use_tr = [use_alt = value.use_alt_tr](strings& normal, strings& alt) -> strings& {
			return use_alt ? alt : normal;
		};
		conv(diag, value).print(actual, diag, use_tr(tr, tr_alt), value.link);

		std::string expected;
		expected.reserve(std::accumulate(begin(exp_str), end(exp_str), 0u,
			[](size_t acc, const auto& view) { return acc + strlen(view); }
		));

		for (auto const& line : exp_str)
			expected.append(line);

		ASSERT_EQ(expected, actual.contents);
	}

	template <typename It, size_t length>
	void output(std::string& expected, It& curr, It end, const expected_diagnostic& value, const std::string_view (&text)[length]) {
		if (curr!=end)
			expected.append(*curr++);

		if (value.line) {
			struct index { size_t raw, mapped{ 0 }; };

			auto line = text[value.line - 1];
			index col{ value.column };
			index col_end{ value.column_end };

			size_t len = 0;
			size_t pos = 0;

			for (auto c : line) {
				++len;
				++pos;
				if (col.raw == pos)
					col.mapped = len;
				if (col_end.raw == pos)
					col_end.mapped = len;
				if (c == '\t') {
					expected.push_back(' ');
					while (len % diagnostic::tab_size) {
						++len;
						expected.push_back(' ');
					}
				}
				else {
					expected.push_back(c);
				}
			}
			if (col.raw && !col.mapped)
				col.mapped = len + (col.raw - pos);
			if (col_end.raw && !col_end.mapped)
				col_end.mapped = len + (col_end.raw - pos);

			expected.push_back('\n');
			if (value.column) {
				for (auto i = 0u; i < (col.mapped - 1); ++i)
					expected.push_back(' ');
				expected.push_back('^');
				if (value.column_end) {
					for (auto i = col.mapped; i < (col_end.mapped - 1); ++i)
						expected.push_back('~');
				}
				expected.push_back('\n');
			}
		}

		for (auto const& sub : value.subs)
			output(expected, curr, end, sub, text);
	}

	TEST_P(diag_write, print_with_source) {
		static constexpr const std::string_view text[] = {
			"one012345678901234567890123456789012345678901234567890123456789"sv,
			"two012345678901234567890123456789012345678901234567890123456789"sv,
			"three012345678901234567890123456789012345678901234567890123456789"sv,
			"four012345678901234567890123456789012345678901234567890123456789"sv,
			"five012345678901234567890123456789012345678901234567890123456789"sv,
			"six012345678901234567890123456789012345678901234567890123456789"sv,
			"seven012345678901234567890123456789012345678901234567890123456789"sv,
			"\twords\t= value1, value2, value3"sv
		};

		auto[value, exp_str] = GetParam();
		diagnostics diag;
		{
			std::vector<std::byte> contents;
			contents.reserve(std::accumulate(std::begin(text), std::end(text), 0u,
				[](size_t acc, const auto& view) { return acc + view.length() + 1; }
			));
			for (auto const& line : text) {
				auto b = reinterpret_cast<const std::byte*>(line.data());
				auto e = b + line.length();
				contents.insert(end(contents), b, e);
				contents.push_back(std::byte('\n'));
			}
			diag.set_contents(value.filename, contents);
		}

		outstrstream actual;
		strings_mock tr;
		alt_strings_mock tr_alt;
		auto use_tr = [use_alt = value.use_alt_tr](strings& normal, strings& alt)->strings& {
			return use_alt ? alt : normal;
		};

		conv(diag, value).print(actual, diag, use_tr(tr, tr_alt), value.link);

		std::string expected;
		{
			auto b = begin(exp_str);
			output(expected, b, end(exp_str), value, text);
		}

		ASSERT_EQ(expected, actual.contents);
	}

	TEST_P(diag_write, has_errors) {
		auto[value, ignore] = GetParam();
		diagnostics diag;
		diag.push_back(conv(diag, value));

		auto expected = value.sev > severity::warning;
		ASSERT_EQ(expected, diag.has_errors()) << UnexpectedDiags::name(value.sev);
	}

	static const diagnostic_write severities[] = {
		{ { "", 0, 0, severity::verbose, "", link_type::gcc, false }, { "\n" } },
		{ { "", 0, 0, severity::note, "", link_type::gcc, false }, { "note: \n" } },
		{ { "", 0, 0, severity::warning, "", link_type::gcc, false }, { "warning: \n" } },
		{ { "", 0, 0, severity::error, "", link_type::gcc, false }, { "error: \n" } },
		{ { "", 0, 0, severity::fatal, "", link_type::gcc, false }, { "fatal: \n" } },
		{ { "" }, { "\n" } },
		{ { "", 0, 0, severity::note }, { "nnn: \n" } },
		{ { "", 0, 0, severity::warning }, { "www: \n" } },
		{ { "", 0, 0, severity::error }, { "eee: \n" } },
		{ { "", 0, 0, severity::fatal }, { "fff: \n" } },
	};
	static const diagnostic_write path[] = {
		{ { "path" }, { "\n" } },
		{ { "path", 0, 0, severity::note }, { "path: nnn: \n" } },
		{ { "path", 0, 0, severity::warning }, { "path: www: \n" } },
		{ { "path", 0, 0, severity::error }, { "path: eee: \n" } },
		{ { "path", 0, 0, severity::fatal }, { "path: fff: \n" } },
		{ { "path", 4 }, { "\n" } },
		{ { "path", 4, 0, severity::note }, { "path:4: nnn: \n" } },
		{ { "path", 4, 0, severity::warning }, { "path:4: www: \n" } },
		{ { "path", 4, 0, severity::error }, { "path:4: eee: \n" } },
		{ { "path", 4, 0, severity::fatal }, { "path:4: fff: \n" } },
		{ { "path", 0, 2 }, { "\n" } },
		{ { "path", 0, 2, severity::note }, { "path: nnn: \n" } },
		{ { "path", 0, 2, severity::warning }, { "path: www: \n" } },
		{ { "path", 0, 2, severity::error }, { "path: eee: \n" } },
		{ { "path", 0, 2, severity::fatal }, { "path: fff: \n" } },
		{ { "path", 4, 2 }, { "\n" } },
		{ { "path", 4, 2, severity::note }, { "path:4:2: nnn: \n" } },
		{ { "path", 4, 2, severity::warning }, { "path:4:2: www: \n" } },
		{ { "path", 4, 2, severity::error }, { "path:4:2: eee: \n" } },
		{ { "path", 4, 2, severity::fatal }, { "path:4:2: fff: \n" } },
		{ { "path", 0, 0, severity::verbose, {}, link_type::vc }, { "\n" } },
		{ { "path", 0, 0, severity::note, {}, link_type::vc }, { "path: nnn: \n" } },
		{ { "path", 0, 0, severity::warning, {}, link_type::vc }, { "path: www: \n" } },
		{ { "path", 0, 0, severity::error, {}, link_type::vc }, { "path: eee: \n" } },
		{ { "path", 0, 0, severity::fatal, {}, link_type::vc }, { "path: fff: \n" } },
		{ { "path", 4, 0, severity::verbose, {}, link_type::vc }, { "\n" } },
		{ { "path", 4, 0, severity::note, {}, link_type::vc }, { "path(4): nnn: \n" } },
		{ { "path", 4, 0, severity::warning, {}, link_type::vc }, { "path(4): www: \n" } },
		{ { "path", 4, 0, severity::error, {}, link_type::vc }, { "path(4): eee: \n" } },
		{ { "path", 4, 0, severity::fatal, {}, link_type::vc }, { "path(4): fff: \n" } },
		{ { "path", 0, 2, severity::verbose, {}, link_type::vc }, { "\n" } },
		{ { "path", 0, 2, severity::note, {}, link_type::vc }, { "path: nnn: \n" } },
		{ { "path", 0, 2, severity::warning, {}, link_type::vc }, { "path: www: \n" } },
		{ { "path", 0, 2, severity::error, {}, link_type::vc }, { "path: eee: \n" } },
		{ { "path", 0, 2, severity::fatal, {}, link_type::vc }, { "path: fff: \n" } },
		{ { "path", 4, 2, severity::verbose, {}, link_type::vc }, { "\n" } },
		{ { "path", 4, 2, severity::note, {}, link_type::vc }, { "path(4,2): nnn: \n" } },
		{ { "path", 4, 2, severity::warning, {}, link_type::vc }, { "path(4,2): www: \n" } },
		{ { "path", 4, 2, severity::error, {}, link_type::vc }, { "path(4,2): eee: \n" } },
		{ { "path", 4, 2, severity::fatal, {}, link_type::vc }, { "path(4,2): fff: \n" } },
	};
	static const diagnostic_write message[] = {
		{ { "", 0, 0, severity::verbose, "a message" }, { "a message\n" } },
		{ { "", 0, 0, severity::note, "a message" }, { "nnn: a message\n" } },
		{ { "", 0, 0, severity::warning, "a message" }, { "www: a message\n" } },
		{ { "", 0, 0, severity::error, "a message" }, { "eee: a message\n" } },
		{ { "", 0, 0, severity::fatal, "a message" }, { "fff: a message\n" } },
	};
	static const diagnostic_write path_message[] = {
		{ { "path", 4, 1, severity::verbose, "a message" }, { "a message\n" } },
		{ { "path", 5, 2, severity::note, "a message" }, { "path:5:2: nnn: a message\n" } },
		{ { "path", 6, 3, severity::warning, "a message" }, { "path:6:3: www: a message\n" } },
		{ { "path", 7, 4, severity::error, "a message" }, { "path:7:4: eee: a message\n" } },
		{ { "path", 3, 5, severity::fatal, "a message" }, { "path:3:5: fff: a message\n" } },
		{ { "path", 2, 6, severity::verbose, "a message", link_type::gcc, false }, { "a message\n" } },
		{ { "path", 1, 7, severity::note, "a message", link_type::gcc, false }, { "path:1:7: note: a message\n" } },
		{ { "path", 4, 8, severity::warning, "a message", link_type::gcc, false }, { "path:4:8: warning: a message\n" } },
		{ { "path", 5, 9, severity::error, "a message", link_type::gcc, false }, { "path:5:9: error: a message\n" } },
		{ { "path", 6, 10, severity::fatal, "a message", link_type::gcc, false }, { "path:6:10: fatal: a message\n" } },

		{ { "path", 1, 11, severity::note, lng::ERR_ID_MISSING_HINT, link_type::gcc, false}, { "path:1:11: note: before finalizing a value, use `id(-1)'\n" } },
		{ { "path", 2, 12, severity::note, lng::ERR_ID_MISSING_HINT}, { "path:2:12: nnn: ERR_ID_MISSING_HINT\n" } },
		{ { "path", 3, 13, severity::error, str(lng::ERR_EXPECTED, "A", "B"), link_type::gcc, false}, { "path:3:13: error: expected A, got B\n" } },
		{ { "path", 4, 14, severity::error, str(lng::ERR_EXPECTED, "A", "B")}, { "path:4:14: eee: ERR_EXPECTED(A, B)\n" } },
	};
	//  1	23457	89.123456789.123456789.1
	// "	words	= value1, value2, value3"sv
	//  123456789.123456789.123456789.123456
	static const diagnostic_write kv[] = {
		{ { "path", 8, 1, severity::note, "key/value", link_type::gcc, false, 2 }, { "path:8:1: note: key/value\n" } },
		{ { "path", 8, 2, severity::note, "key/value", link_type::gcc, false, 3 }, { "path:8:2: note: key/value\n" } },
		{ { "path", 8, 2, severity::note, "key/value", link_type::gcc, false, 7 }, { "path:8:2: note: key/value\n" } },
		{ { "path", 8, 7, severity::note, "key/value", link_type::gcc, false, 8 }, { "path:8:7: note: key/value\n" } },
		{ { "path", 8, 8, severity::note, "key/value", link_type::gcc, false, 9 }, { "path:8:8: note: key/value\n" } },
		{ { "path", 8, 10, severity::note, "key/value", link_type::gcc, false, 16 }, { "path:8:10: note: key/value\n" } },
		{ { "path", 8, 18, severity::note, "key/value", link_type::gcc, false, 24 }, { "path:8:18: note: key/value\n" } },
		{ { "path", 8, 26, severity::note, "key/value", link_type::gcc, false, 32 }, { "path:8:26: note: key/value\n" } },
		{ { "path", 8, 35, severity::note, "key/value", link_type::gcc, false, 36 }, { "path:8:35: note: key/value\n" } },
	};

	static const diagnostic_write multiline[] = {
		{
			{
				"path", 6, 3, severity::warning, "a message", link_type::gcc, false, 0,
				{
					{ "path", 5, 2, severity::note, "an explanation" }
				}
			},
			{
				"path:6:3: warning: a message\n",
				"path:5:2: note: an explanation\n"
			}
		},
		{
			{
				"path", 6, 3, severity::warning, "a message", link_type::vc, false, 0,
				{
					{ "path", 5, 2, severity::note, "an explanation" }
				}
			},
			{
				"path(6,3): warning: a message\n",
				"    path(5,2): note: an explanation\n"
			}
		},
		{
			{
				"path", 6, 3, severity::warning, "a message", link_type::vc, false, 0,
				{
					{ "path", 5, 2, severity::error, "hidden error" }
				}
			},
			{
				"path(6,3): warning: a message\n",
				"    path(5,2): error: hidden error\n"
			}
		},
	};

	INSTANTIATE_TEST_CASE_P(severities, diag_write, ValuesIn(severities));
	INSTANTIATE_TEST_CASE_P(path, diag_write, ValuesIn(path));
	INSTANTIATE_TEST_CASE_P(message, diag_write, ValuesIn(message));
	INSTANTIATE_TEST_CASE_P(path_message, diag_write, ValuesIn(path_message));
	INSTANTIATE_TEST_CASE_P(kv, diag_write, ValuesIn(kv));
	INSTANTIATE_TEST_CASE_P(multiline, diag_write, ValuesIn(multiline));

	struct diag_ne : TestWithParam<std::pair<argumented_string, argumented_string>> {};

	TEST_P(diag_ne, comp) {
		auto[lhs, rhs] = GetParam();
		EXPECT_NE(lhs, rhs);
	}

	static const std::pair<argumented_string, argumented_string> neq[] = {
		{ lng::ERR_FILE_MISSING, lng::ERR_EXPECTED },
		{ ""s, lng::ERR_FILE_MISSING },
		{ lng::ERR_EXPECTED, ""s },
		{ arg(lng::ERR_FILE_MISSING, "path"s), lng::ERR_FILE_MISSING },
		{ arg(lng::ERR_FILE_MISSING, lng::ERR_EXPECTED_GOT_EOF), arg(lng::ERR_FILE_MISSING, "EOF"s) }
	};

	INSTANTIATE_TEST_CASE_P(neq, diag_ne, ValuesIn(neq));
}