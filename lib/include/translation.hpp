/*
 * Copyright (C) 2013 midnightBITS
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

#include <experimental/filesystem>
#include <functional>
#include <memory>
#include <locale_file.hpp>

namespace locale {

	namespace fs_ex = std::experimental::filesystem;

	namespace manager {
		// uses base and filename to generate paths "<base>/<lng>/<filename>"
		class SubdirPath {
			fs_ex::path m_base;
			fs_ex::path m_fname;
		public:
			explicit SubdirPath(std::string base, std::string fname)
				: m_base(std::move(base))
				, m_fname(std::move(fname))
			{}

			fs_ex::path operator()(const std::string& lng) {
				return m_base / lng / m_fname;
			}
		};

		// uses base and filename to generate paths "<base>/<filename>.<lng>"
		class ExtensionPath {
			std::string m_base;
		public:
			explicit ExtensionPath(std::string base, std::string fname)
				: m_base((fs_ex::path{ std::move(base) } / (fname += ".")).string())
			{
			}

			fs_ex::path operator()(const std::string& lng) {
				return m_base + lng;
			}
		};
	}

	struct memory_block : memory_view {
		std::unique_ptr<char[]> block;
	};

	class translation {
		std::function<fs_ex::path(const std::string&)> m_file_path;
		fs_ex::path m_path;
		memory_block m_data;
		lang_file m_file;
		fs_ex::file_time_type m_mtime;

		fs_ex::file_time_type mtime() const noexcept
		{
			std::error_code ec;
			auto time = fs_ex::last_write_time(m_path, ec);
			if (ec)
				return decltype(time){};
			return time;
		}
		memory_block open() noexcept;
	public:
		template <typename T>
		void path_manager(T&& manager)
		{
			m_file_path = manager;
		}

		bool open(const std::string& lng);
		bool fresh() const noexcept { return mtime() == m_mtime; }
		const char* get_string(uint32_t id) const noexcept;
		const char* get_string(intmax_t count, uint32_t id) const noexcept;
		const char* get_attr(uint32_t id) const noexcept;
		const char* get_key(uint32_t id) const noexcept;
		uint32_t find_key(const char* id) const noexcept;
	};
}
