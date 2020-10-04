#pragma once

#include <lngs/lngs_file.hpp>

#include <diags/streams.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/strings.hpp>

namespace lngs::testing::helper {
	struct attrs_t {
		std::vector<std::pair<attr_t, std::string>> vals{};
		intmax_t (*plural_map)(intmax_t) = nullptr;

		attrs_t&& culture(std::string val) && {
			vals.emplace_back(ATTR_CULTURE, std::move(val));
			return std::move(*this);
		}

		attrs_t&& language(std::string val) && {
			vals.emplace_back(ATTR_LANGUAGE, std::move(val));
			return std::move(*this);
		}

		attrs_t&& plurals(std::string val) && {
			vals.emplace_back(ATTR_PLURALS, std::move(val));
			return std::move(*this);
		}

		template <class Lambda>
		attrs_t&& map(Lambda cb) && {
			plural_map = cb;
			return std::move(*this);
		}
	};

	struct builder {
		uint32_t serial{0};
		template <typename... Strings>
		lngs::app::idl_strings make(Strings... strings) {
			return {{},
			        {},
			        {},
			        serial,
			        -1,
			        false,
			        {lngs::app::idl_string{strings}...}};
		}
	};

	inline lngs::app::idl_string str(int id,
	                                 std::string key,
	                                 std::string value) {
		return {std::move(key), std::move(value), {}, {}, id, id};
	}

	inline void build_strings(diags::outstream& dst,
	                          const lngs::app::idl_strings& defs,
	                          const attrs_t& attrs,
	                          bool with_keys) {
		lngs::app::file file;
		file.serial = defs.serial;

		file.strings.reserve(defs.strings.size());
		file.attrs.reserve(attrs.vals.size());
		if (with_keys) file.keys.reserve(defs.strings.size());

		for (auto& string : defs.strings) {
			file.strings.emplace_back(string.id, string.value);
			if (with_keys) file.keys.emplace_back(string.id, string.key);
		}

		for (auto [id, attr] : attrs.vals)
			file.attrs.emplace_back(id, attr);

		file.write(dst);
	}
}  // namespace lngs::testing::helper
