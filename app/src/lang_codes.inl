// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <string>

namespace {
	const char* key_of(const token* tok) { return tok->key; }
	const char* key_of(const char** key) { return *key; }

	template <typename T, size_t len>
	const T* search(T (&coll)[len], std::string_view key) {
		auto lo = coll;
		auto hi = coll + len - 1;

		while (hi >= lo) {
			auto mid = lo + (hi - lo) / 2;
			auto test = key.compare(key_of(mid));
			if (!test) return mid;

			if (test < 0)
				hi = mid - 1;
			else
				lo = mid + 1;
		}

		return nullptr;
	}

}  // namespace

namespace lngs::app {
	std::string language_name(std::string_view lang) {
		const auto ll_cc = lang.substr(0, lang.find('.'));
		auto pos = ll_cc.find('-');
		if (pos == std::string_view::npos) {
			auto tok = search(languages, ll_cc);
			if (!tok) return {};
			return tok->value;
		}

		auto tok = search(languages, ll_cc.substr(0, pos));
		if (!tok) return {};

		std::string out{tok->value};

		auto len = ll_cc.find('-', pos + 1);
		if (len != std::string_view::npos) len -= pos + 1;
		auto script_region = ll_cc.substr(pos + 1, len);

		if (search(scripts, script_region)) {
			if (len == std::string_view::npos) return out;

			pos += 1 + len;
			len = ll_cc.find('-', pos + 1);
			if (len != std::string_view::npos) len -= pos + 1;
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
}  // namespace lngs::app
