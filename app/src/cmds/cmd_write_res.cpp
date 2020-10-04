// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <diags/streams.hpp>
#include <lngs/internals/commands.hpp>
#include <lngs/internals/languages.hpp>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app::res {
	class table_outstream : public diags::outstream {
		static constexpr size_t row_width = 16;
		std::string content;
		size_t offset = 0;

	public:
		table_outstream() = default;
		std::size_t write(const void* data,
		                  std::size_t length) noexcept override;
		std::string finalize();
	};

	std::size_t table_outstream::write(const void* data,
	                                   std::size_t length) noexcept {
		auto chars = reinterpret_cast<const uint8_t*>(data);
		auto save = offset;

		for (size_t i = 0; i < length; ++i, ++offset, ++chars) {
			if ((offset % row_width) == 0) {
				content.append("            \"");
			}
			content.append(fmt::format("\\x{:02x}", *chars));
			if (((offset + 1) % row_width) == 0) {
				content.append("\"\n");
			}
		}

		return offset - save;
	}

	std::string table_outstream::finalize() {
		if ((offset % row_width) != 0) content.append("\"\n");
		return std::move(content);
	}

	file make_resource(const idl_strings& defs,
	                   bool warp_strings,
	                   bool with_keys) {
		file file;
		file.serial = defs.serial;

		file.strings.reserve(defs.strings.size());
		if (with_keys) file.keys.reserve(defs.strings.size());

		bool has_plurals = false;
		for (auto& string : defs.strings) {
			if (!string.plural.empty()) has_plurals = true;
			auto value = string.value;
			if (warp_strings) value = warp(value);

			if (!string.plural.empty()) {
				value.push_back(0);
				if (warp_strings)
					value.append(warp(string.plural));
				else
					value.append(string.plural);
			}

			if (with_keys) file.keys.emplace_back(string.id, string.key);
			file.strings.emplace_back(string.id, value);
		}
		if (has_plurals)
			file.attrs.emplace_back(ATTR_PLURALS,
			                        "nplurals=2; plural=(n != 1);");
		return file;
	}

	int update_and_write(
	    diags::outstream& out,
	    file& data,
	    const idl_strings& defs,
	    std::string_view include,
	    std::optional<std::filesystem::path> const& redirected) {
		auto resource = mstch::lambda{[&]() -> mstch::node {
			table_outstream os{};
			data.write(os);
			return os.finalize();
		}};
		return write_mstch(
		    out, defs, redirected, "res",
		    {{"include", std::string{include}}, {"resource", resource}});
	}
}  // namespace lngs::app::res
