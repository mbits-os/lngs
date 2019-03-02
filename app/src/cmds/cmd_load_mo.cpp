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

#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/gettext.hpp>

#include <algorithm>

namespace lngs::app {
	std::string language_name(std::string_view ll_cc);
}

namespace lngs::app::make {
	file load_mo(const idl_strings& defs, bool warp_missing, bool verbose, source_file data, diagnostics& diags) {
		file file;
		file.serial = defs.serial;
		auto map = gtt::open(data, diags);
		file.strings = translations(map, defs.strings, warp_missing, verbose, data, diags);
		file.attrs = attributes(map);
		return file;
	}

	bool fix_attributes(file& file, source_file& mo_file, const std::string& ll_CCs, diagnostics& diags) {
		auto prop = find_if(begin(file.attrs), end(file.attrs), [](auto& item) { return item.key.id == ATTR_CULTURE; });
		if (prop == end(file.attrs) || prop->value.empty()) {
			const auto pos = mo_file.position();
			diags.push_back(pos[severity::warning] << lng::ERR_MSGS_ATTR_LANG_MISSING);
		} else {
			bool lang_set = false;

			if (!ll_CCs.empty()) {
				auto is = diags.source(ll_CCs);
				const auto pos = is.position();

				std::map<std::string, std::string> names;
				if (!ll_CC(std::move(is), diags, names))
					return false;

				auto it = names.find(prop->value);
				if (it == names.end()) {
					diags.push_back(pos[severity::warning] << arg(lng::ERR_LOCALE_MISSING, prop->value));
				} else {
					file.attrs.emplace_back(ATTR_LANGUAGE, it->second);
					lang_set = true;
				}
			}

			if (!lang_set) {
				auto name = language_name(prop->value);
				if (!name.empty())
					file.attrs.emplace_back(ATTR_LANGUAGE, warp(name));
			}
		}

		sort(begin(file.attrs), end(file.attrs), [](auto& lhs, auto& rhs) { return lhs.key.id < rhs.key.id; });
		return true;
	}
}
