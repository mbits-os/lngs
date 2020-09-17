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

#include <cstdio>
#include <lngs/file.hpp>

namespace fs {
	FILE* file::fopen(path file, char const* mode) noexcept {
		file.make_preferred();
#if defined WIN32 || defined _WIN32
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
		std::unique_ptr<wchar_t[]> heap;
		wchar_t buff[20];
		wchar_t* ptr = buff;
		auto len = mode ? strlen(mode) : 0;
		if (len >= sizeof(buff)) {
			heap.reset(new (std::nothrow) wchar_t[len + 1]);
			if (!heap) return nullptr;
			ptr = heap.get();
		}

		auto dst = ptr;
		while (*dst++ = *mode++)
			;

		return ::_wfopen(file.native().c_str(), ptr);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else  // WIN32 || _WIN32
		return std::fopen(file.string().c_str(), mode);
#endif
	}

	std::vector<std::byte> file::read() const noexcept {
		std::vector<std::byte> out;
		if (!*this) return out;
		std::byte buffer[1024];

		while (true) {
			auto ret = std::fread(buffer, 1, sizeof(buffer), get());
			if (!ret) {
				if (!std::feof(get())) out.clear();
				break;
			}
			out.insert(end(out), buffer, buffer + ret);
		}

		return out;
	}

	size_t file::load(void* buffer, size_t length) const noexcept {
		return std::fread(buffer, 1, length, get());
	}

	size_t file::store(const void* buffer, size_t length) const noexcept {
		return std::fwrite(buffer, 1, length, get());
	}

	file fopen(const path& file, char const* mode) noexcept {
		return {file, mode};
	}

}  // namespace fs
