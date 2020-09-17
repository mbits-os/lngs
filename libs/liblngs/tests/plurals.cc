#include <gtest/gtest.h>
#include <lngs/plurals.hpp>

namespace lngs::plurals::testing {
	using namespace ::std::literals;
	using ::testing::Test;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct lex_cb { intmax_t(*code)(intmax_t) = nullptr; };
	struct lex_range { intmax_t from; intmax_t to; intmax_t output; lex_cb cb{}; };
	struct lex_simple_range {
		intmax_t from;
		intmax_t to;
		constexpr lex_range operator<=(intmax_t expected) const { return { from, to, expected }; }
		template <class Lambda>
		constexpr lex_range map(Lambda cb) const { return { from, to, 0, { cb } }; }
	};
	constexpr inline lex_simple_range R(intmax_t from, intmax_t to) { return { from, to }; }
	constexpr inline lex_simple_range R(intmax_t from) { return { from, from + 1 }; }

	struct lex_program {
		std::string_view code;
		std::initializer_list<lex_range> data;
		bool expect_valid{ true };
		friend std::ostream& operator<<(std::ostream& o, const lex_program& p) {
			return o << '"' << p.code << '"';
		}
	};

	enum class op_type {
		unary,
		binary
	};

	struct oper {
		std::string_view name{};
		op_type kind{ op_type::binary };
		constexpr oper() = default;
		constexpr oper(std::string_view name, op_type kind = op_type::binary) : name{ name }, kind{ kind } {}

		friend std::ostream& operator<<(std::ostream& o, const oper& op) {
			o << '"';
			if (op.kind == op_type::binary)
				o << "X";

			return o << op.name << (op.kind == op_type::binary ? "Y" : "X") << '"';
		}
	};

	constexpr inline oper unary(std::string_view name) { return { name, op_type::unary }; }

	struct plural_ops : TestWithParam<oper> {};
	struct plurals : TestWithParam<lex_program> {};

	TEST_P(plural_ops, alone) {
		auto[name, kind] = GetParam();
		std::string prog = "plural=";
		prog.append(name.data(), name.length());

		auto lex = decode(prog);
		EXPECT_FALSE(!!lex);
		EXPECT_EQ(0, lex.eval(0));
		EXPECT_EQ(0, lex.eval(1));
		EXPECT_EQ(0, lex.eval(2));
		EXPECT_EQ(0, lex.eval(3));
		EXPECT_EQ(0, lex.eval(4));
	}

	TEST_P(plural_ops, missing_right) {
		auto[name, kind] = GetParam();
		if (kind == op_type::unary)
			return;

		std::string prog = "plural=n";
		prog.append(name.data(), name.length());

		auto lex = decode(prog);
		EXPECT_FALSE(!!lex);
		EXPECT_EQ(0, lex.eval(0));
		EXPECT_EQ(0, lex.eval(1));
		EXPECT_EQ(0, lex.eval(2));
		EXPECT_EQ(0, lex.eval(3));
		EXPECT_EQ(0, lex.eval(4));
	}

	TEST_P(plural_ops, missing_left) {
		auto[name, kind] = GetParam();
		if (kind == op_type::unary)
			return;

		std::string prog = "plural=";
		prog.append(name.data(), name.length());
		prog += "n";

		auto lex = decode(prog);
		EXPECT_FALSE(!!lex);
		EXPECT_EQ(0, lex.eval(0));
		EXPECT_EQ(0, lex.eval(1));
		EXPECT_EQ(0, lex.eval(2));
		EXPECT_EQ(0, lex.eval(3));
		EXPECT_EQ(0, lex.eval(4));
	}

	TEST_P(plurals, run) {
		auto[code, ranges, expected_valid] = GetParam();

		auto lex = decode(code);
		EXPECT_EQ(expected_valid, !!lex);

		for (auto[from, to, output, cb] : ranges) {
			if (cb.code) {
				const auto expected = cb.code;
				for (intmax_t n = from; n < to; ++n) {
					auto actual = lex.eval(n);
					EXPECT_EQ(expected(n), actual) << "  Range: " << from << "-" << to << "; Curent: " << n;
				}
			} else {
				const auto expected = output;
				for (intmax_t n = from; n < to; ++n) {
					auto actual = lex.eval(n);
					EXPECT_EQ(expected, actual) << "  Range: " << from << "-" << to << "; Curent: " << n;
				}
			}
		}
	}

	constexpr static const oper ops[] = {
		"*"sv, "/"sv, "%"sv, "+"sv, "-"sv,
		unary("!"sv),
		"=="sv, "!="sv, "<"sv, "<="sv, ">"sv, ">="sv,
		"&&"sv, "||"sv
	};

	static const lex_program exception[] = {
		{"plural=3/n"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return 3 / n; }) } },
		{"plural=3%n"sv, { R(0,1) <= 0, R(2,100).map([](intmax_t n) { return 3 % n; }) } },
		{"plural=!(3/n)"sv, { R(0) <= 0, R(1,4) <= 0, R(4,100) <= 1 } },
		{"plural=(3/n)*5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return (3 / n) * 5; }) } },
		{"plural=5*(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return 5 * (3 / n); }) } },
		{"plural=(3/n)+5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return (3 / n) + 5; }) } },
		{"plural=5+(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return 5 + (3 / n); }) } },
		{"plural=(3/n)-5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return (3 / n) - 5; }) } },
		{"plural=5-(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) { return 5 - (3 / n); }) } },
		{"plural=(3/n)<5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) < 5; }) } },
		{"plural=5<(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 < (3 / n); }) } },
		{"plural=(3/n)<=5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) <= 5; }) } },
		{"plural=5<=(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 <= (3 / n); }) } },
		{"plural=(3/n)>5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) > 5; }) } },
		{"plural=5>(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 > (3 / n); }) } },
		{"plural=(3/n)>=5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) >= 5; }) } },
		{"plural=5>=(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 >= (3 / n); }) } },
		{"plural=(3/n)==5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) == 5; }) } },
		{"plural=5==(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 == (3 / n); }) } },
		{"plural=(3/n)!=5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) != 5; }) } },
		{"plural=5!=(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 != (3 / n); }) } },
		{"plural=(3/n)&&5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) && 5; }) } },
		{"plural=5&&(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 5 && (3 / n); }) } },
		{"plural=(3/n)||5"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) || 5; }) } },
		{"plural=0||(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 0 || (3 / n); }) } },
		{"plural=(3/n)?1:0"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return (3 / n) ? 1 : 0; }) } },
		{"plural=1?(3/n):0"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 1 ? (3 / n) : 0; }) } },
		{"plural=0?0:(3/n)"sv, { R(0) <= 0, R(1,100).map([](intmax_t n) -> intmax_t { return 0 ? 0 : (3 / n); }) } },
	};

	static const lex_program bad[] = {
		{""sv, { R(0) <= 0 }, false },
		{"plural="sv, { R(0) <= 0 }, false },
		{"plural=n|3"sv, { R(0) <= 0 }, false },
		{"plural=n&3"sv, { R(0) <= 0 }, false },
		{"plural=n^3"sv, { R(0) <= 0 }, false },
		{"plural=n=3"sv, { R(0) <= 0 }, false },
		{"plural=(n"sv, { R(0) <= 0 }, false },
		{"plural=n?"sv, { R(0) <= 0 }, false },
		{"plural=n?!n:"sv, { R(0) <= 0 }, false },
	};

	static const lex_program good[] = {
		{"plural=n*3"sv, { R(0, 100).map([](intmax_t n) { return n * 3; }) } },
		{"plural=n*3/5*4"sv, { R(0, 3).map([](intmax_t n) { return n * 3 / 5 * 4; }) } },
		{"plural=n+3"sv, { R(0, 100).map([](intmax_t n) { return n + 3; }) } },
		{"plural=n-3"sv, { R(0, 100).map([](intmax_t n) { return n - 3; }) } },
		{"plural=n+3-5+10*3/4"sv, { R(0, 3).map([](intmax_t n) { return n + 3 -5 + 10 * 3 / 4; }) } },
		// The parens around 'n > 5' and later on around &&s are placed due to -Wparentheses warnings
		// Those tests are here to test just that: left-to-rightness of operators and precedence of
		// logical AND over logical OR.
		{"plural=n<5<2*n"sv, { R(0, 3).map([](intmax_t n) -> intmax_t { return (n < 5) < 2 * n; }) } },
		{"plural=n<5<2*n<n*n"sv, { R(0, 3).map([](intmax_t n) -> intmax_t { return ((n < 5) < 2 * n) < n * n; }) } },
		{"plural=n==5!=0*n"sv, { R(0, 6).map([](intmax_t n) -> intmax_t { return (n == 5) != 0; }) } },
		{"plural=n&&n<2||n>5&&10>n"sv, { R(0, 10).map([](intmax_t n) -> intmax_t { return (n && n < 2) || (n > 5 && 10 > n); }) } },
		{"plural=n||n>2&&n<5||10>n"sv, { R(0, 10).map([](intmax_t n) -> intmax_t { return n || (n > 2 && n < 5) || 10 > n; }) } },
		{"plural=n!=1?!n:n"sv, { R(0,1) <= 1, R(2,100) <= 0 } },
	};

	static const lex_program msginit[] = {
		{
			"nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<12 || n%100>14) ? 1 : 2)"sv,
			{
				R(0) <= 2,
				R(1) <= 0,
				R(2,5) <= 1,
				R(5,22) <= 2,
				R(22,25) <= 1,
			}
		}
	};

	INSTANTIATE_TEST_SUITE_P(ops, plural_ops, ValuesIn(ops));
	INSTANTIATE_TEST_SUITE_P(exception, plurals, ValuesIn(exception));
	INSTANTIATE_TEST_SUITE_P(bad, plurals, ValuesIn(bad));
	INSTANTIATE_TEST_SUITE_P(good, plurals, ValuesIn(good));
	INSTANTIATE_TEST_SUITE_P(msginit, plurals, ValuesIn(msginit));
}
