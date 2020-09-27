// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <memory>
#include <string_view>

namespace lngs::plurals {
	struct expr {
		virtual ~expr() noexcept {}
		virtual intmax_t eval(intmax_t n, bool& failed) const noexcept = 0;
	};

	struct lexical {
		int nplurals{0};
		std::unique_ptr<expr> plural{};

		lexical() = default;
		~lexical() = default;
		lexical(const lexical&) = delete;
		lexical(lexical&&) = default;
		lexical& operator=(const lexical&) = delete;
		lexical& operator=(lexical&&) = default;

		intmax_t eval(intmax_t n) const noexcept;
		explicit operator bool() const noexcept { return !!plural; }
	};

	lexical decode(std::string_view entry);
}  // namespace lngs::plurals
