// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/internals/commands.hpp>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app::py {
	int write(mstch_env const& env) {
		return env.write_mstch("py");
	}
}  // namespace lngs::app::py
