// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/gettext.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/strings.hpp>

#include <algorithm>

namespace lngs::app {
	std::string language_name(std::string_view ll_cc);
}

namespace lngs::app::make {
	file load_msgs(const idl_strings& defs,
	               bool warp_missing,
	               bool verbose,
	               diags::source_code data,
	               diags::sources& diags) {
		auto const binary_messages = gtt::is_mo(data);

		file file;
		file.serial = defs.serial;
		auto map = binary_messages ? gtt::open_mo(data, diags)
		                           : gtt::open_po(data, diags);
		file.strings =
		    translations(map, defs.strings, warp_missing, verbose, data, diags);
		file.attrs = attributes(map);
		return file;
	}

	bool fix_attributes(file& file,
	                    diags::source_code& mo_file,
	                    const std::string& ll_CCs,
	                    diags::sources& diags) {
		auto prop = find_if(begin(file.attrs), end(file.attrs), [](auto& item) {
			return item.key.id == ATTR_CULTURE;
		});
		if (prop == end(file.attrs) || prop->value.empty()) {
			const auto pos = mo_file.position();
			diags.push_back(pos[diags::severity::warning]
			                << lng::ERR_MSGS_ATTR_LANG_MISSING);
		} else {
			bool lang_set = false;

			if (!ll_CCs.empty()) {
				auto is = diags.source(ll_CCs);
				const auto pos = is.position();

				std::map<std::string, std::string> names;
				if (!ll_CC(std::move(is), diags, names)) return false;

				auto it = names.find(prop->value);
				if (it == names.end()) {
					diags.push_back(pos[diags::severity::warning] << format(
					                    lng::ERR_LOCALE_MISSING, prop->value));
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

		sort(begin(file.attrs), end(file.attrs),
		     [](auto& lhs, auto& rhs) { return lhs.key.id < rhs.key.id; });
		return true;
	}
}  // namespace lngs::app::make
