#pragma once
#include <vector>
#include <string>
#include <memory>

namespace plurals { namespace parser {
	inline namespace tokenizer {
		enum tok_t {
			tok_unknown,
			tok_variable,
			tok_number,
			tok_bang,
			tok_mul,
			tok_div,
			tok_mod,
			tok_plus,
			tok_minus,
			tok_lt,
			tok_gt,
			tok_le,
			tok_ge,
			tok_eq,
			tok_ne,
			tok_ampamp,
			tok_pipepipe,
			tok_lparen,
			tok_rparen,
			tok_question,
			tok_colon
		};

		struct token {
			tok_t type;
			int value;
			token() : type(tok_unknown), value(0) {}
			token(tok_t type) : type(type), value(0) {}
			token(int value) : type(tok_number), value(value) {}
		};

		std::vector<token> tokenize(const std::string& value);
	}

	std::unique_ptr<plurals::expr> parse(const std::string& value);
}}
