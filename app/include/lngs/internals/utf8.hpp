// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <string>
#include <string_view>

namespace utf {
	std::u16string as_u16(std::string_view src);
	std::u32string as_u32(std::string_view src);
	std::string as_u8(std::u16string_view src);
	std::u32string as_u32(std::u16string_view src);
	std::u16string as_u16(std::u32string_view src);
	std::string as_u8(std::u32string_view src);
}  // namespace utf
