// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/internals/commands.hpp>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app::py {
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          std::optional<std::filesystem::path> const& redirected) {
		return write_mstch(out, defs, redirected, "py");
	}
}  // namespace lngs::app::py
