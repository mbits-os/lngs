/*
 * Copyright (C) 2018 midnightBITS
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

#pragma once

#include <vector>
#include <cstddef>
#include <tuple>
#include <locale/file.hpp>

namespace lngs {
	struct file;
	struct outstream;
	struct idl_strings;

	static inline std::string straighten(std::string str) {
		for (auto& c : str)
			if (c == '\n') c = ' ';
		return str;
	}
}

namespace lngs::pot {
	struct info {
		std::string copy{ "THE PACKAGE'S COPYRIGHT HOLDER" };
		std::string first_author{ "FIRST AUTHOR <EMAIL@ADDRESS>" };
		std::string lang_team{ "LANGUAGE <LL@li.org>" };
	};

	int write(outstream& out, const idl_strings& defs, const info& nfo);
}

namespace lngs::enums {
	int write(outstream& out, const idl_strings& defs, bool with_resource);
}

namespace lngs::py {
	int write(outstream& out, const idl_strings& defs);
}

namespace lngs::make {
	file load_mo(const idl_strings& defs, bool warp_missing, bool verbose, const fs::path& path);
	bool fix_attributes(file& file, const fs::path& ll_CCs);
}

namespace lngs::res {
	file make_resource(const idl_strings& defs, bool warp_strings, bool with_keys);
	int update_and_write(outstream& out, file& data, std::string_view include, std::string_view project);
}

namespace lngs::freeze {
	bool freeze(idl_strings& defs);
	int write(outstream& out, const idl_strings& defs, const std::vector<std::byte>& data);
}
