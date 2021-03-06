// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "expr_parser.hpp"
#include <cctype>
#include "node.hpp"

namespace lngs::plurals::parser {
	namespace {
		unsigned char s2uc(char c) { return static_cast<unsigned char>(c); }
	}  // namespace

	std::vector<token> tokenizer::tokenize(const std::string& value) {
		std::vector<token> out;

		auto c = std::begin(value);
		auto e = std::end(value);

		while (c != e) {
			bool move = true;
			switch (*c) {
				case 'n':
					out.push_back(tok_variable);
					break;
				case '*':
					out.push_back(tok_mul);
					break;
				case '/':
					out.push_back(tok_div);
					break;
				case '%':
					out.push_back(tok_mod);
					break;
				case '+':
					out.push_back(tok_plus);
					break;
				case '-':
					out.push_back(tok_minus);
					break;
				case '(':
					out.push_back(tok_lparen);
					break;
				case ')':
					out.push_back(tok_rparen);
					break;
				case '?':
					out.push_back(tok_question);
					break;
				case ':':
					out.push_back(tok_colon);
					break;
				case '!': {
					auto next = c;
					++next;
					if (next != e && *next == '=') {
						c = next;
						out.push_back(tok_ne);
					} else
						out.push_back(tok_bang);

					break;
				}
				case '<': {
					auto next = c;
					++next;
					if (next != e && *next == '=') {
						c = next;
						out.push_back(tok_le);
					} else
						out.push_back(tok_lt);

					break;
				}
				case '>': {
					auto next = c;
					++next;
					if (next != e && *next == '=') {
						c = next;
						out.push_back(tok_ge);
					} else
						out.push_back(tok_gt);

					break;
				}
				case '&': {
					++c;
					if (c != e && *c == '&')
						out.push_back(tok_ampamp);
					else
						out.push_back(tok_unknown);

					break;
				}
				case '|': {
					++c;
					if (c != e && *c == '|')
						out.push_back(tok_pipepipe);
					else
						out.push_back(tok_unknown);

					break;
				}
				case '=': {
					++c;
					if (c != e && *c == '=')
						out.push_back(tok_eq);
					else
						out.push_back(tok_unknown);

					break;
				}
				case ' ':
				case '\t':
				case '\n':
					break;
				default:
					if (std::isdigit(s2uc(*c))) {
						int val = 0;
						while (c != e && std::isdigit(s2uc(*c))) {
							val *= 10;
							val += *c - '0';
							++c;
						}
						out.push_back(val);
						move = false;
						break;
					}
					out.push_back(tok_unknown);
					break;
			}

			if (move) ++c;
		}

		return out;
	}

	template <typename It>
	std::unique_ptr<expr> trenary(It& cur, It end);

	template <typename It>
	std::unique_ptr<expr> expression(It& cur, It end) {
		return trenary(cur, end);
	}

	template <typename It>
	std::unique_ptr<expr> simple(It& cur, It end) {
		if (cur == end) return {};

		switch (cur->type) {
			case tok_lparen: {
				++cur;
				auto content = expression(cur, end);
				if (cur == end || cur->type != tok_rparen) return {};

				++cur;
				return content;
			}
			case tok_number: {
				auto val = cur->value;
				++cur;
				return std::make_unique<nodes::value>(val);
			}
			case tok_bang: {
				++cur;
				auto content = simple(cur, end);
				if (!content) return {};
				return std::make_unique<nodes::logical_not>(std::move(content));
			}
			case tok_variable:
				++cur;
				return std::make_unique<nodes::var>();
			default:
				break;
		};

		return {};
	}

	template <typename It>
	std::unique_ptr<expr> muldiv(It& cur, It end) {
		auto left = simple(cur, end);
		decltype(left) right;

		if (!left) return left;

		bool should_carry_on = true;
		while (cur != end && should_carry_on) {
			switch (cur->type) {
				case tok_mul:
					++cur;
					right = simple(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::multiply>(std::move(left),
					                                         std::move(right));
					break;
				case tok_div:
					++cur;
					right = simple(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::divide>(std::move(left),
					                                       std::move(right));
					break;
				case tok_mod:
					++cur;
					right = simple(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::modulo>(std::move(left),
					                                       std::move(right));
					break;
				default:
					should_carry_on = false;
					break;
			};
		}

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> addsub(It& cur, It end) {
		auto left = muldiv(cur, end);
		decltype(left) right;

		if (!left) return left;

		bool should_carry_on = true;
		while (cur != end && should_carry_on) {
			switch (cur->type) {
				case tok_plus:
					++cur;
					right = muldiv(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::plus>(std::move(left),
					                                     std::move(right));
					break;
				case tok_minus:
					++cur;
					right = muldiv(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::minus>(std::move(left),
					                                      std::move(right));
					break;
				default:
					should_carry_on = false;
					break;
			};
		}

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> relation(It& cur, It end) {
		auto left = addsub(cur, end);
		decltype(left) right;

		if (!left) return left;

		bool should_carry_on = true;
		while (cur != end && should_carry_on) {
			switch (cur->type) {
				case tok_lt:
					++cur;
					right = addsub(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::less_than>(std::move(left),
					                                          std::move(right));
					break;
				case tok_le:
					++cur;
					right = addsub(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::less_than_or_equal>(
					    std::move(left), std::move(right));
					break;
				case tok_gt:
					++cur;
					right = addsub(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::greater_than>(
					    std::move(left), std::move(right));
					break;
				case tok_ge:
					++cur;
					right = addsub(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::greater_than_or_equal>(
					    std::move(left), std::move(right));
					break;
				default:
					should_carry_on = false;
					break;
			};
		}

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> compare(It& cur, It end) {
		auto left = relation(cur, end);
		decltype(left) right;

		if (!left) return left;

		bool should_carry_on = true;
		while (cur != end && should_carry_on) {
			switch (cur->type) {
				case tok_eq:
					++cur;
					right = relation(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::equal>(std::move(left),
					                                      std::move(right));
					break;
				case tok_ne:
					++cur;
					right = relation(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::not_equal>(std::move(left),
					                                          std::move(right));
					break;
				default:
					should_carry_on = false;
					break;
			};
		}

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> logical_and(It& cur, It end) {
		auto left = compare(cur, end);
		decltype(left) right;

		if (!left) return left;

		bool should_carry_on = true;
		while (cur != end && should_carry_on) {
			switch (cur->type) {
				case tok_ampamp:
					++cur;
					right = compare(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::logical_and>(
					    std::move(left), std::move(right));
					break;
				default:
					should_carry_on = false;
					break;
			};
		}

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> logical_or(It& cur, It end) {
		auto left = logical_and(cur, end);
		decltype(left) right;

		if (!left) return left;

		bool should_carry_on = true;
		while (cur != end && should_carry_on) {
			switch (cur->type) {
				case tok_pipepipe:
					++cur;
					right = logical_and(cur, end);
					if (!right) return {};
					left = std::make_unique<nodes::logical_or>(
					    std::move(left), std::move(right));
					break;
				default:
					should_carry_on = false;
					break;
			};
		}

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> trenary(It& cur, It end) {
		auto first = logical_or(cur, end);
		if (!first) return {};
		if (cur == end || cur->type != tok_question) return first;

		++cur;
		auto second = trenary(cur, end);
		if (!second || cur == end || cur->type != tok_colon) return {};

		++cur;
		auto third = trenary(cur, end);
		if (!third) return {};

		return std::make_unique<nodes::ternary>(
		    std::move(first), std::move(second), std::move(third));
	}

	std::unique_ptr<expr> parse(const std::string& value) {
		auto tokens = tokenize(value);

		auto cur = std::begin(tokens);
		auto end = std::end(tokens);
		auto ret = expression(cur, end);
		if (cur != end) return {};

		return ret;
	}
}  // namespace lngs::plurals::parser
