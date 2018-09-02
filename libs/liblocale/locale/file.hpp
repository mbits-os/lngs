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

#include <filesystem>
#include <vector>
#include <cstddef>

namespace fs {
using namespace std::filesystem;
using std::error_code;

struct fcloser { void operator()(FILE* f) { std::fclose(f); } };
class file : private std::unique_ptr<FILE, fcloser> {
	using parent_t = std::unique_ptr<FILE, fcloser>;
	static FILE* fopen(path fname, char const* mode) noexcept;
public:
	file() = default;
	file(const file&) = delete;
	file& operator=(const file&) = delete;
	file(file&&) = default;
	file& operator=(file&&) = default;

	explicit file(const path& fname) noexcept : file(fname, "r") {}
	file(const path& fname, const char* mode) noexcept : parent_t(fopen(fname, mode)) {}

	using parent_t::operator bool;

	FILE* handle() const noexcept { return get(); }

	void close() noexcept { reset(); }
	void open(const path& fname, char const* mode = "r") noexcept { reset(fopen(fname, mode)); }

	std::vector<std::byte> read() const noexcept;
	size_t load(void* buffer, size_t length) const noexcept;
	size_t store(const void* buffer, size_t length) const noexcept;
	bool feof() const noexcept { return std::feof(get()); }
};

inline file fopen(const path& file, char const* mode = "r") noexcept {
	return { file, mode };
};
}
