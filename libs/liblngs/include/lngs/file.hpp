// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace fs {
	using namespace std::filesystem;
	using std::error_code;

	struct fcloser {
		void operator()(FILE* f) { std::fclose(f); }
	};
	class file : private std::unique_ptr<FILE, fcloser> {
		using parent_t = std::unique_ptr<FILE, fcloser>;
		static FILE* fopen(path fname, char const* mode) noexcept;

	public:
		file() = default;
		~file() = default;
		file(const file&) = delete;
		file& operator=(const file&) = delete;
		file(file&&) = default;
		file& operator=(file&&) = default;

		explicit file(const path& fname) noexcept : file(fname, "r") {}
		file(const path& fname, const char* mode) noexcept
		    : parent_t(fopen(fname, mode)) {}

		using parent_t::operator bool;

		void close() noexcept { reset(); }
		void open(const path& fname, char const* mode = "r") noexcept {
			reset(fopen(fname, mode));
		}

		std::vector<std::byte> read() const noexcept;
		size_t load(void* buffer, size_t length) const noexcept;
		size_t store(const void* buffer, size_t length) const noexcept;
		bool feof() const noexcept { return std::feof(get()); }
	};

	file fopen(const path& file, char const* mode = "r") noexcept;
}  // namespace fs
