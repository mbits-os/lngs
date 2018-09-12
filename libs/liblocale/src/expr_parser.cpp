/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <locale/plurals.hpp>
#include "expr_parser.hpp"
#include "node.hpp"
#include <cctype>

namespace plurals { namespace parser {
	std::vector<token> tokenizer::tokenize(const std::string& value) {
		std::vector<token> out;

		auto c = std::begin(value);
		auto e = std::end(value);

		while (c != e) {
			bool move = true;
			switch (*c) {
			case 'n': out.push_back(tok_variable); break;
			case '*': out.push_back(tok_mul); break;
			case '/': out.push_back(tok_div); break;
			case '%': out.push_back(tok_mod); break;
			case '+': out.push_back(tok_plus); break;
			case '-': out.push_back(tok_minus); break;
			case '(': out.push_back(tok_lparen); break;
			case ')': out.push_back(tok_rparen); break;
			case '?': out.push_back(tok_question); break;
			case ':': out.push_back(tok_colon); break;
			case '!':
			{
				auto next = c;
				++next;
				if (next != e && *next == '=') {
					c = next;
					out.push_back(tok_ne);
				}
				else
					out.push_back(tok_bang);

				break;
			}
			case '<':
			{
				auto next = c;
				++next;
				if (next != e && *next == '=') {
					c = next;
					out.push_back(tok_le);
				}
				else
					out.push_back(tok_lt);

				break;
			}
			case '>':
			{
				auto next = c;
				++next;
				if (next != e && *next == '=') {
					c = next;
					out.push_back(tok_ge);
				}
				else
					out.push_back(tok_gt);

				break;
			}
			case '&':
			{
				++c;
				if (c != e && *c == '&')
					out.push_back(tok_ampamp);
				else
					out.push_back(tok_unknown);

				break;
			}
			case '|':
			{
				++c;
				if (c != e && *c == '|')
					out.push_back(tok_pipepipe);
				else
					out.push_back(tok_unknown);

				break;
			}
			case '=':
			{
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
				if (std::isdigit((uint8_t)*c)) {
					int val = 0;
					while (c != e && std::isdigit((uint8_t)*c)) {
						val *= 10;
						val += *c - '0';
						++c;
					}
					out.push_back(val);
					move = false;
					break;
				}
				out.push_back(tok_unknown); break;
			}

			if (move)
				++c;
		}

		return out;
	}

	template <typename It>
	std::unique_ptr<expr> trenary(It& cur, It end);

	template <typename It>
	std::unique_ptr<expr> expression(It& cur, It end)
	{
		return trenary(cur, end);
	}

	template <typename It>
	std::unique_ptr<expr> simple(It& cur, It end)
	{
		if (cur == end)
			return{};

		switch (cur->type) {
		case tok_lparen: {
			++cur;
			auto content = expression(cur, end);
			if (cur == end || cur->type != tok_rparen)
				return{};

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
			if (!content)
				return {};
			return std::make_unique<nodes::logical_not>(std::move(content));
		}
		case tok_variable:
			++cur;
			return std::make_unique<nodes::var>();
		default:
			break;
		};

		return{};
	}

	template <typename It>
	std::unique_ptr<expr> muldiv(It& cur, It end)
	{
		auto left = simple(cur, end);
		decltype(left) right;

		if (!left || cur == end)
			return left;

		switch (cur->type) {
		case tok_mul:
			++cur;
			right = muldiv(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::multiply>(std::move(left), std::move(right));
		case tok_div:
			++cur;
			right = muldiv(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::divide>(std::move(left), std::move(right));
		case tok_mod:
			++cur;
			right = muldiv(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::modulo>(std::move(left), std::move(right));
		default:
			break;
		};

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> addsub(It& cur, It end)
	{
		auto left = muldiv(cur, end);
		decltype(left) right;

		if (!left || cur == end)
			return left;

		switch (cur->type) {
		case tok_plus:
			++cur;
			right = addsub(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::plus>(std::move(left), std::move(right));
		case tok_minus:
			++cur;
			right = addsub(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::minus>(std::move(left), std::move(right));
		default:
			break;
		};

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> relation(It& cur, It end)
	{
		auto left = addsub(cur, end);
		decltype(left) right;

		if (!left || cur == end)
			return left;

		switch (cur->type) {
		case tok_lt:
			++cur;
			right = relation(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::less_than>(std::move(left), std::move(right));
		case tok_le:
			++cur;
			right = relation(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::less_than_or_equal>(std::move(left), std::move(right));
		case tok_gt:
			++cur;
			right = relation(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::greater_than>(std::move(left), std::move(right));
		case tok_ge:
			++cur;
			right = relation(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::greater_than_or_equal>(std::move(left), std::move(right));
		default:
			break;
		};

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> compare(It& cur, It end)
	{
		auto left = relation(cur, end);
		decltype(left) right;

		if (!left || cur == end)
			return left;

		switch (cur->type) {
		case tok_eq:
			++cur;
			right = compare(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::equal>(std::move(left), std::move(right));
		case tok_ne:
			++cur;
			right = compare(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::not_equal>(std::move(left), std::move(right));
		default:
			break;
		};

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> andor(It& cur, It end)
	{
		auto left = compare(cur, end);
		decltype(left) right;

		if (!left || cur == end)
			return left;

		switch (cur->type) {
		case tok_ampamp:
			++cur;
			right = andor(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::logical_and>(std::move(left), std::move(right));
		case tok_pipepipe:
			++cur;
			right = andor(cur, end);
			if (!right)
				return{};
			return std::make_unique<nodes::logical_or>(std::move(left), std::move(right));
		default:
			break;
		};

		return left;
	}

	template <typename It>
	std::unique_ptr<expr> trenary(It& cur, It end)
	{
		auto first = andor(cur, end);
		if (!first)
			return{};
		if (cur == end || cur->type != tok_question)
			return first;

		++cur;
		auto second = trenary(cur, end);
		if (!second || cur == end || cur->type != tok_colon)
			return{};

		++cur;
		auto third = trenary(cur, end);
		if (!third)
			return{};

		return std::make_unique<nodes::ternary>(std::move(first), std::move(second), std::move(third));
	}

	std::unique_ptr<expr> parse(const std::string& value) {
		auto tokens = tokenize(value);

		auto cur = std::begin(tokens);
		auto end = std::end(tokens);
		auto ret = expression(cur, end);
		if (cur != end)
			return{};

		return ret;
	}
}}
