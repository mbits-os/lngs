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

#include "cmakeconfig.h"

#ifdef CMAKE_HAVE_TS_FILESYSTEM
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <locale/locale_file.hpp>

#ifndef CMAKE_HAVE_TS_FILESYSTEM
namespace boost { namespace filesystem {
inline directory_iterator begin(const directory_iterator& iter)
{
	return iter;
}
inline directory_iterator end(const directory_iterator&)
{
	return directory_iterator { };
}
}} // boost::filesystem
#endif

namespace locale {

	namespace fs {
#ifdef CMAKE_HAVE_TS_FILESYSTEM
		using namespace std::experimental::filesystem;
		using std::error_code;
#else // !def CMAKE_HAVE_TS_FILESYSTEM
		using namespace boost::filesystem;
		using boost::system::error_code;
		using file_time_type = std::time_t;
#endif // CMAKE_HAVE_TS_FILESYSTEM
	}

	namespace manager {
		// uses base and filename to generate paths "<base>/<lng>/<filename>"
		class SubdirPath {
			fs::path m_base;
			fs::path m_fname;
		public:
			explicit SubdirPath(std::string base, std::string fname)
				: m_base(std::move(base))
				, m_fname(std::move(fname))
			{}

			fs::path expand(const std::string& lng) const
			{
				return m_base / lng / m_fname;
			}

			std::vector<fs::path> known() const
			{
				std::vector<fs::path> out;

				for (auto& entry : fs::directory_iterator { m_base }) {
					fs::error_code ec;
					auto stat = entry.status(ec);
					if (ec)
						continue;

					if (!fs::is_directory(stat))
						continue;

					auto full = entry.path() / m_fname;
					if (fs::is_regular_file(full))
						out.push_back(full);
				};

				return out;
			}
		};

		// uses base and filename to generate paths "<base>/<filename>.<lng>"
		class ExtensionPath {
			fs::path m_base;
			std::string m_fname;
		public:
			explicit ExtensionPath(std::string base, std::string fname)
				: m_base(std::move(base))
				, m_fname(std::move(fname))
			{
			}

			fs::path expand(const std::string& lng) const
			{
				static std::string dot { "." };
				return m_base / (m_fname + dot + lng);
			}

			std::vector<fs::path> known() const
			{
				std::vector<fs::path> out;

				for (auto& entry : fs::directory_iterator { m_base }) {
					fs::error_code ec;
					auto stat = entry.status(ec);
					if (ec)
						continue;

					if (!fs::is_regular_file(stat))
						continue;

					auto fname = entry.path().filename().replace_extension();
					if (fname == m_fname)
						out.push_back(entry.path());
				};

				return out;
			}
		};
	}

	struct memory_block : memory_view {
		std::unique_ptr<char[]> block;
	};

	struct culture {
		std::string lang;
		std::string name;
	};

	class translation {
		struct manager_t {
			virtual ~manager_t() { }
			virtual fs::path expand(const std::string& lng) const = 0;
			virtual std::vector<fs::path> known() const = 0;
		};

		template <typename T>
		class manager_impl : public manager_t {
			T info;
		public:
			template <typename... Args>
			explicit manager_impl(Args&&... args) : info(std::forward<Args>(args)...) {}
			fs::path expand(const std::string& lng) const override
			{
				return info.expand(lng);
			}

			std::vector<fs::path> known() const override
			{
				return info.known();
			}
		};

		std::unique_ptr<manager_t> m_path_mgr;
		fs::path m_path;
		memory_block m_data;
		lang_file m_file;
		fs::file_time_type m_mtime;

		fs::file_time_type mtime() const noexcept
		{
			fs::error_code ec;
			auto time = fs::last_write_time(m_path, ec);
			if (ec)
				return decltype(time){};
			return time;
		}

		std::map<uint32_t, std::function<void()>> m_updatelisteners;
		uint32_t m_nextupdate = 0xba5e0000;

		void onupdate();
	public:
		static memory_block open_file(const fs::path& path) noexcept;
		template <typename T, typename... Args>
		void path_manager(Args&&... args)
		{
			m_path_mgr = std::make_unique<manager_impl<T>>(std::forward<Args>(args)...);
		}

		bool open(const std::string& lng);
		bool fresh() const noexcept { return mtime() == m_mtime; }
		const char* get_string(uint32_t id) const noexcept;
		const char* get_string(intmax_t count, uint32_t id) const noexcept;
		const char* get_attr(uint32_t id) const noexcept;
		const char* get_key(uint32_t id) const noexcept;
		uint32_t find_key(const char* id) const noexcept;
		std::vector<culture> known() const;

		uint32_t add_onupdate(const std::function<void()>&);
		void remove_onupdate(uint32_t token);
	};
}
