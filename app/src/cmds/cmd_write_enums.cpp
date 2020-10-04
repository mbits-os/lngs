// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/internals/commands.hpp>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app::enums {
	int write(diags::outstream& out,
	          const idl_strings& defs,
	          std::optional<fs::path> const& redirected,
	          bool with_resource) {
		return write_mstch(out, defs, redirected, "enums",
		                   {{"with_resource", with_resource}});
	}
}  // namespace lngs::app::enums
