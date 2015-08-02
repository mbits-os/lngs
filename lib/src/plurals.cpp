#include "plurals.hpp"
#include "expr_parser.hpp"
#include "node.hpp"
#include "str.hpp"

namespace plurals {
	lexical decode(const std::string& entry) {
		lexical out{ 0,{} };
		auto values = split(entry, ";");
		for (auto& val : values) {
			auto attr = split(val, "=", 1);
			if (attr.size() < 2) continue;
			auto name = strip(attr[0]);
			auto value = strip(attr[1]);
			if (name == "nplurals")
				out.nplurals = std::atoi(value.c_str());
			else if (name == "plural")
				out.plural = parser::parse(value);
		}

		return std::move(out);
	}

	intmax_t lexical::eval(intmax_t n) const
	{
		if (!plural)
			return 0;

		try {
			return plural->eval(n);
		}
		catch (bool) {
			return 0;
		}
	}
}
