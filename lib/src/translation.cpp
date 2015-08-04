/*
 * Copyright (C) 2015 midnightBITS
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

#include <translation.hpp>
#include <assert.h>

namespace locale {
	/* static */
	memory_block translation::open(const fs_ex::path& path) noexcept
	{
		std::error_code ec;
		auto size = fs_ex::file_size(path, ec);
		if (ec)
			return{};

		memory_block block;
		block.size = size;
		block.block.reset(new (std::nothrow) char[(size_t)block.size]);
		block.contents = block.block.get();
		if (!block.block)
			return{};

#ifdef WIN32
		FILE* file = nullptr;
		_wfopen_s(&file, path.wstring().c_str(), L"rb");
		std::unique_ptr<FILE, decltype(&fclose)> f { file, fclose };
#else
		std::unique_ptr<FILE, decltype(&fclose)> f {
			fopen(path.native().c_str(), "rb"), fclose
		};
#endif

		if (!f)
			return{};

		bool read = fread(block.block.get(), (size_t)block.size, 1, f.get()) == 1;
		if (!read)
			return{};

		return block;
	}

	bool translation::open(const std::string& lng)
	{
		assert(m_path_mgr);
		m_path = m_path_mgr->expand(lng);
		m_mtime = mtime();

		m_file.close();
		m_path.make_preferred();
		m_data = open(m_path);

		if (!m_file.open(m_data)) {
			m_file.close();
			m_data = memory_block { };
			m_mtime = decltype(m_mtime){};
			return false;
		}

		return true;
	}

	const char* translation::get_string(uint32_t id) const noexcept
	{
		return m_file.get_string(id);
	}

	const char* translation::get_string(intmax_t count, uint32_t id) const noexcept
	{
		return m_file.get_string(count, id);
	}

	const char* translation::get_attr(uint32_t id) const noexcept
	{
		return m_file.get_attr(id);
	}

	const char* translation::get_key(uint32_t id) const noexcept
	{
		return m_file.get_key(id);
	}

	uint32_t translation::find_key(const char* id) const noexcept
	{
		return m_file.find_key(id);
	}

	std::vector<culture> translation::known() const
	{
		assert(m_path_mgr);
		auto files = m_path_mgr->known();

		std::vector<culture> out;

		for (auto& path : files) {
			lang_file file;
			path.make_preferred();
			auto view = open(path);
			if (!file.open(view))
				continue;

			auto lang = file.get_attr(ATTR_CULTURE);
			auto name = file.get_attr(ATTR_LANGUAGE);

			culture c;
			if (lang)
				c.lang = lang;
			if (name)
				c.name = name;

			auto copy = m_path_mgr->expand(c.lang);
			copy.make_preferred();
			if (copy != path)
				continue;

			out.push_back(c);
		}

		return out;
	}
}
