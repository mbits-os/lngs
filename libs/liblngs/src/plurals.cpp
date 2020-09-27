// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <cstdlib>
#include <lngs/plurals.hpp>
#include "expr_parser.hpp"
#include "node.hpp"
#include "str.hpp"

namespace lngs::plurals {
	lexical decode(std::string_view entry) {
		lexical out;
		auto values = split_view(entry, ";");
		for (auto& val : values) {
			auto attr = split_view(val, "=", 1);
			if (attr.size() < 2) continue;
			auto name = strip(attr[0]);
			auto value = strip(attr[1]);
			if (name == "nplurals")
				out.nplurals = std::atoi(value.c_str());
			else if (name == "plural")
				out.plural = parser::parse(value);
		}

		return out;
	}

	intmax_t lexical::eval(intmax_t n) const noexcept {
		if (!plural) return 0;
		bool failed = false;
		const auto ret = plural->eval(n, failed);
		if (failed) return 0;
		return ret;
	}
}  // namespace lngs::plurals
