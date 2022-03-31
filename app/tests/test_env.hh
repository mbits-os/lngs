#pragma once

#include <lngs/internals/mstch_engine.hpp>
#include <lngs/internals/strings.hpp>

namespace lngs::app {
	template <typename Output>
	struct test_env {
		Output output{};
		idl_strings strings{};
#ifdef LNGS_LINKED_RESOURCES
		mstch_env env() noexcept { return {output, strings}; }
#else
		std::optional<std::filesystem::path> redir{};
		mstch_env env() noexcept { return {output, strings, redir}; }
#endif
	};
}  // namespace lngs::app
