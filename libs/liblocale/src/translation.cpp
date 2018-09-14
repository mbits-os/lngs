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

#include <locale/translation.hpp>
#include <assert.h>

namespace locale {
	/* static */
	memory_block translation::open_file(const fs::path& path) noexcept
	{
		fs::error_code ec;
		auto size = fs::file_size(path, ec);
		if (ec)
			return{};

		memory_block block;
		block.size = size;
		block.block.reset(new (std::nothrow) std::byte[(size_t)block.size]);
		block.contents = block.block.get();
		if (!block.block)
			return{};

#ifdef WIN32
		FILE* file = nullptr;
		(void)_wfopen_s(&file, path.wstring().c_str(), L"rb");
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
		m_data = open_file(m_path);

		if (!m_file.open(m_data)) {
			m_file.close();
			m_data = memory_block { };
			m_mtime = decltype(m_mtime){};
			return false;
		}

		onupdate();
		return true;
	}

	std::string_view translation::get_string(identifier id) const noexcept
	{
		return m_file.get_string(id);
	}

	std::string_view translation::get_string(identifier id, quantity count) const noexcept
	{
		return m_file.get_string(id, count);
	}

	std::string_view translation::get_attr(uint32_t id) const noexcept
	{
		return m_file.get_attr(id);
	}

	std::string_view translation::get_key(uint32_t id) const noexcept
	{
		return m_file.get_key(id);
	}

	uint32_t translation::find_key(std::string_view id) const noexcept
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
			auto view = open_file(path);
			if (!file.open(view))
				continue;

			auto lang = file.get_attr(ATTR_CULTURE);
			auto name = file.get_attr(ATTR_LANGUAGE);

			culture c;
			if (!lang.empty())
				c.lang = lang;
			if (!name.empty())
				c.name = name;

			auto copy = m_path_mgr->expand(c.lang);
			copy.make_preferred();
			if (copy != path)
				continue;

			out.push_back(c);
		}

		return out;
	}

	uint32_t translation::add_onupdate(const std::function<void()>& fn)
	{
		if (!fn)
			return 0;
		++m_nextupdate;
		if (!m_nextupdate)
			++m_nextupdate;
		m_updatelisteners[m_nextupdate] = fn;
		return m_nextupdate;
	}

	void translation::remove_onupdate(uint32_t token)
	{
		auto it = m_updatelisteners.find(token);
		if (it != m_updatelisteners.end())
			m_updatelisteners.erase(it);
	}

	void translation::onupdate()
	{
		auto copy = m_updatelisteners;
		for (auto& pair : copy)
			pair.second();
	}
}
