#pragma once

#include <lngs/streams.hpp>
#include <string>
#include <numeric>

namespace lngs::testing {
	struct outstrstream : outstream {
		std::string contents;

		std::size_t write(const void* data, std::size_t length) noexcept final
		{
			auto b = static_cast<const char*>(data);
			auto e = b + length;
			auto size = contents.size();
			contents.insert(end(contents), b, e);
			return contents.size() - size;
		}
	};
}