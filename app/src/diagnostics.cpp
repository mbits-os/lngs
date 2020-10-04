// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <lngs/internals/diagnostics.hpp>
#ifdef _WIN32
#include <windows.h>
#include <lngs/internals/utf8.hpp>
#undef SEVERITY_ERROR
#undef min
#undef max
#endif

namespace diags {
#ifdef _WIN32
	inline std::string oem_cp(std::string_view view) {
		auto u16 = utf::as_u16(view);
		auto cp = GetOEMCP();
		size_t str_size = WideCharToMultiByte(cp, 0, (LPCWSTR)u16.c_str(), -1,
		                                      nullptr, 0, nullptr, nullptr);
		if (!str_size) return {view.data(), view.length()};

		std::string out(str_size - 1, ' ');
		auto result =
		    WideCharToMultiByte(cp, 0, (LPCWSTR)u16.c_str(), -1, out.data(),
		                        str_size, nullptr, nullptr);
		if (!result) return {view.data(), view.length()};

		return out;
	}
#else
	inline std::string_view oem_cp(std::string_view view) { return view; }
#endif
}  // namespace diags
