#include <gtest/gtest.h>
#include "diag_helper.h"
#include <string_view>
#include <numeric>

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
		const char* expected;
	};

	std::ostream& operator<<(std::ostream& o, const diagnostic_write& diag) {
		return o << diag.value;
	}
	class diag_write : public TestWithParam<diagnostic_write> {};

	TEST_P(diag_write, print) {
		auto [value, expected] = GetParam();
		diagnostics diag;
		const auto loc = diag.source(value.filename).position(value.line, value.column);

		outstrstream actual;
		strings_mock tr;
		alt_strings_mock tr_alt;
		auto use_tr = [use_alt = value.use_alt_tr](strings& normal, strings& alt) -> strings& {
			return use_alt ? alt : normal;
		};
		(loc[value.sev] << value.message.arg()).print(actual, diag, use_tr(tr, tr_alt), value.link);

		ASSERT_EQ(expected, actual.contents);
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
		};

		auto[value, expected_base] = GetParam();
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
		const auto loc = diag.source(value.filename).position(value.line, value.column);

		outstrstream actual;
		strings_mock tr;
		alt_strings_mock tr_alt;
		auto use_tr = [use_alt = value.use_alt_tr](strings& normal, strings& alt)->strings& {
			return use_alt ? alt : normal;
		};

		(loc[value.sev] << value.message.arg()).print(actual, diag, use_tr(tr, tr_alt), value.link);

		std::string expected;
		expected.reserve([&](auto expected_base, const auto& value) {
			size_t length = std::strlen(expected_base);
			if (value.line) {
				length += text[value.line - 1].length() + 1;
				if (value.column) {
					length += value.column + 1;
				}
			}
			return length;
		}(expected_base, value));

		expected.append(expected_base);
		if (value.line) {
			expected.append(text[value.line - 1]);
			expected.push_back('\n');
			if (value.column) {
				for (auto i = 0u; i < (value.column - 1); ++i)
					expected.push_back(' ');
				expected.push_back('^');
				expected.push_back('\n');
			}
		}

		ASSERT_EQ(expected, actual.contents);
	}

	static const diagnostic_write severities[] = {
		{ { "", 0, 0, severity::verbose, "", link_type::gcc, false }, "\n" },
		{ { "", 0, 0, severity::note, "", link_type::gcc, false }, "note: \n" },
		{ { "", 0, 0, severity::warning, "", link_type::gcc, false }, "warning: \n" },
		{ { "", 0, 0, severity::error, "", link_type::gcc, false }, "error: \n" },
		{ { "", 0, 0, severity::fatal, "", link_type::gcc, false }, "fatal: \n" },
		{ { "" }, "\n" },
		{ { "", 0, 0, severity::note }, "nnn: \n" },
		{ { "", 0, 0, severity::warning }, "www: \n" },
		{ { "", 0, 0, severity::error }, "eee: \n" },
		{ { "", 0, 0, severity::fatal }, "fff: \n" },
	};
	static const diagnostic_write path[] = {
		{ { "path" }, "\n" },
		{ { "path", 0, 0, severity::note }, "path: nnn: \n" },
		{ { "path", 0, 0, severity::warning }, "path: www: \n" },
		{ { "path", 0, 0, severity::error }, "path: eee: \n" },
		{ { "path", 0, 0, severity::fatal }, "path: fff: \n" },
		{ { "path", 4 }, "\n" },
		{ { "path", 4, 0, severity::note }, "path:4: nnn: \n" },
		{ { "path", 4, 0, severity::warning }, "path:4: www: \n" },
		{ { "path", 4, 0, severity::error }, "path:4: eee: \n" },
		{ { "path", 4, 0, severity::fatal }, "path:4: fff: \n" },
		{ { "path", 0, 2 }, "\n" },
		{ { "path", 0, 2, severity::note }, "path: nnn: \n" },
		{ { "path", 0, 2, severity::warning }, "path: www: \n" },
		{ { "path", 0, 2, severity::error }, "path: eee: \n" },
		{ { "path", 0, 2, severity::fatal }, "path: fff: \n" },
		{ { "path", 4, 2 }, "\n" },
		{ { "path", 4, 2, severity::note }, "path:4:2: nnn: \n" },
		{ { "path", 4, 2, severity::warning }, "path:4:2: www: \n" },
		{ { "path", 4, 2, severity::error }, "path:4:2: eee: \n" },
		{ { "path", 4, 2, severity::fatal }, "path:4:2: fff: \n" },
		{ { "path", 0, 0, severity::verbose, {}, link_type::vc }, "\n" },
		{ { "path", 0, 0, severity::note, {}, link_type::vc }, "path: nnn: \n" },
		{ { "path", 0, 0, severity::warning, {}, link_type::vc }, "path: www: \n" },
		{ { "path", 0, 0, severity::error, {}, link_type::vc }, "path: eee: \n" },
		{ { "path", 0, 0, severity::fatal, {}, link_type::vc }, "path: fff: \n" },
		{ { "path", 4, 0, severity::verbose, {}, link_type::vc }, "\n" },
		{ { "path", 4, 0, severity::note, {}, link_type::vc }, "path(4): nnn: \n" },
		{ { "path", 4, 0, severity::warning, {}, link_type::vc }, "path(4): www: \n" },
		{ { "path", 4, 0, severity::error, {}, link_type::vc }, "path(4): eee: \n" },
		{ { "path", 4, 0, severity::fatal, {}, link_type::vc }, "path(4): fff: \n" },
		{ { "path", 0, 2, severity::verbose, {}, link_type::vc }, "\n" },
		{ { "path", 0, 2, severity::note, {}, link_type::vc }, "path: nnn: \n" },
		{ { "path", 0, 2, severity::warning, {}, link_type::vc }, "path: www: \n" },
		{ { "path", 0, 2, severity::error, {}, link_type::vc }, "path: eee: \n" },
		{ { "path", 0, 2, severity::fatal, {}, link_type::vc }, "path: fff: \n" },
		{ { "path", 4, 2, severity::verbose, {}, link_type::vc }, "\n" },
		{ { "path", 4, 2, severity::note, {}, link_type::vc }, "path(4,2): nnn: \n" },
		{ { "path", 4, 2, severity::warning, {}, link_type::vc }, "path(4,2): www: \n" },
		{ { "path", 4, 2, severity::error, {}, link_type::vc }, "path(4,2): eee: \n" },
		{ { "path", 4, 2, severity::fatal, {}, link_type::vc }, "path(4,2): fff: \n" },
	};
	static const diagnostic_write message[] = {
		{ { "", 0, 0, severity::verbose, "a message" }, "a message\n" },
		{ { "", 0, 0, severity::note, "a message" }, "nnn: a message\n" },
		{ { "", 0, 0, severity::warning, "a message" }, "www: a message\n" },
		{ { "", 0, 0, severity::error, "a message" }, "eee: a message\n" },
		{ { "", 0, 0, severity::fatal, "a message" }, "fff: a message\n" },
	};
	static const diagnostic_write path_message[] = {
		{ { "path", 4, 1, severity::verbose, "a message" }, "a message\n" },
		{ { "path", 5, 2, severity::note, "a message" }, "path:5:2: nnn: a message\n" },
		{ { "path", 6, 3, severity::warning, "a message" }, "path:6:3: www: a message\n" },
		{ { "path", 7, 4, severity::error, "a message" }, "path:7:4: eee: a message\n" },
		{ { "path", 3, 5, severity::fatal, "a message" }, "path:3:5: fff: a message\n" },
		{ { "path", 2, 6, severity::verbose, "a message", link_type::gcc, false }, "a message\n" },
		{ { "path", 1, 7, severity::note, "a message", link_type::gcc, false }, "path:1:7: note: a message\n" },
		{ { "path", 4, 8, severity::warning, "a message", link_type::gcc, false }, "path:4:8: warning: a message\n" },
		{ { "path", 5, 9, severity::error, "a message", link_type::gcc, false }, "path:5:9: error: a message\n" },
		{ { "path", 6, 10, severity::fatal, "a message", link_type::gcc, false }, "path:6:10: fatal: a message\n" },
	};

	INSTANTIATE_TEST_CASE_P(severities, diag_write, ValuesIn(severities));
	INSTANTIATE_TEST_CASE_P(path, diag_write, ValuesIn(path));
	INSTANTIATE_TEST_CASE_P(message, diag_write, ValuesIn(message));
	INSTANTIATE_TEST_CASE_P(path_message, diag_write, ValuesIn(path_message));
}