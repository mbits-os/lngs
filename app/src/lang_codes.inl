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

#include <string>

namespace {
	const char* key_of(const token* tok) { return tok->key; }
	const char* key_of(const char** key) { return *key; }

	template <typename T, size_t len>
	const T* search(T(&coll)[len], std::string_view key)
	{
		auto lo = coll;
		auto hi = coll + len - 1;

		while (hi >= lo) {
			auto mid = lo + (hi - lo) / 2;
			auto test = key.compare(key_of(mid));
			if (!test)
				return mid;

			if (test < 0)
				hi = mid - 1;
			else
				lo = mid + 1;
		}

		return nullptr;
	}

};

namespace lngs::app {
	std::string language_name(std::string_view lang)
	{
		const auto ll_cc = lang.substr(0, lang.find('.'));
		auto pos = ll_cc.find('-');
		if (pos == std::string_view::npos) {
			auto tok = search(languages, ll_cc);
			if (!tok)
				return { };
			return tok->value;
		}

		auto tok = search(languages, ll_cc.substr(0, pos));
		if (!tok)
			return { };

		std::string out { tok->value };

		auto len = ll_cc.find('-', pos + 1);
		if (len != std::string_view::npos)
			len -= pos + 1;
		auto script_region = ll_cc.substr(pos + 1, len);

		if (search(scripts, script_region)) {
			if (len == std::string_view::npos)
				return out;

			pos += 1 + len;
			len = ll_cc.find('-', pos + 1);
			if (len != std::string_view::npos)
				len -= pos + 1;
			script_region = ll_cc.substr(pos + 1, len);
		}

		auto reg = search(regions, script_region);
		if (reg) {
			out.append(" (");
			out.append(reg->value);
			out.append(")");
		} else {
			out.append(" (Unknown - ");
			out.append(script_region);
			out.append(")");
		}

		return out;
	}
}
