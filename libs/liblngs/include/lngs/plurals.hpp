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

#pragma once

#include <memory>
#include <string_view>

namespace lngs::plurals {
	struct expr {
		virtual ~expr() noexcept {}
		virtual intmax_t eval(intmax_t n, bool& failed) const noexcept = 0;
	};

	struct lexical {
		int nplurals{ 0 };
		std::unique_ptr<expr> plural{};

		lexical();
		~lexical();
		lexical(const lexical&) = delete;
		lexical(lexical&&);
		lexical& operator=(const lexical&) = delete;
		lexical& operator=(lexical&&);

		intmax_t eval(intmax_t n) const noexcept;
		explicit operator bool() const noexcept { return !!plural; }
	};

	lexical decode(std::string_view entry);
}
