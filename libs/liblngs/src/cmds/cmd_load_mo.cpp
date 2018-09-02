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

#include <lngs/commands.hpp>
#include <lngs/streams.hpp>
#include <lngs/strings.hpp>
#include <lngs/languages.hpp>
#include <lngs/gettext.hpp>

#include <algorithm>

namespace lngs {
	std::string language_name(std::string_view ll_cc);
}

namespace lngs::make {
	file load_mo(const idl_strings& defs, bool warp_missing, bool verbose, const fs::path& path) {
		file file;
		file.serial = defs.serial;
		auto map = gtt::open(path);
		file.strings = translations(map, defs.strings, warp_missing, verbose);
		file.attrs = attributes(map);
		return file;
	}

	bool fix_attributes(file& file, const fs::path& ll_CCs) {
		auto prop = find_if(begin(file.attrs), end(file.attrs), [](auto& item) { return item.key.id == locale::ATTR_CULTURE; });
		if (prop == end(file.attrs) || prop->value.empty()) {
			printf("warning: message file does not contain Language attribute.\n");
		} else {
			bool lang_set = false;

			if (!ll_CCs.empty()) {
				std::map<std::string, std::string> names;
				if (!ll_CC(ll_CCs, names))
					return false;

				auto it = names.find(prop->value);
				if (it == names.end())
					printf("warning: no %s in %s\n", prop->value.c_str(), ll_CCs.string().c_str());
				else {
					file.attrs.emplace_back(locale::ATTR_LANGUAGE, it->second);
					lang_set = true;
				}
			}

			if (!lang_set) {
				auto name = language_name(prop->value);
				if (!name.empty())
					file.attrs.emplace_back(locale::ATTR_LANGUAGE, warp(name));
			}
		}

		sort(begin(file.attrs), end(file.attrs), [](auto& lhs, auto& rhs) { return lhs.key.id < rhs.key.id; });
		return true;
	}
}
