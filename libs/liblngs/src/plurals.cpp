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

#include <lngs/plurals.hpp>
#include "expr_parser.hpp"
#include "node.hpp"
#include "str.hpp"
#include <cstdlib>

namespace lngs::plurals {
	lexical::lexical() = default;
	lexical::~lexical() = default;
	lexical::lexical(lexical&&) = default;
	lexical& lexical::operator=(lexical&&) = default;

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

	intmax_t lexical::eval(intmax_t n) const noexcept
	{
		if (!plural)
			return 0;
		bool failed = false;
		const auto ret = plural->eval(n, failed);
		if (failed)
			return 0;
		return ret;
	}
}
