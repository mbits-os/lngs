#pragma once

#include <memory>
#include <string>

namespace plurals {
	struct expr {
		virtual ~expr() {}
		virtual intmax_t eval(intmax_t n) const = 0;
	};

	struct lexical {
		int nplurals;
		std::unique_ptr<expr> plural;
		intmax_t eval(intmax_t n) const;
		explicit operator bool() const { return !!plural; }
	};

	lexical decode(const std::string& entry);
}
