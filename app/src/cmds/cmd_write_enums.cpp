// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/internals/commands.hpp>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app::enums {
	int write(mstch_env const& env, bool with_resource) {
		return env.write_mstch("enums", {{"with_resource", with_resource}});
	}
}  // namespace lngs::app::enums
