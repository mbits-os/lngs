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

#include <lngs/gettext.hpp>

namespace gtt {
	class GetTextFile {
		std::unique_ptr<char[]> m_content;
		size_t m_size;
	public:
		~GetTextFile()
		{
			close();
		}
		bool open(const fs::path& path)
		{
			close();
			fs::error_code ec;
			auto st = fs::status(path, ec);
			if (ec || !fs::is_regular_file(st))
				return false;

			m_size = (size_t)fs::file_size(path, ec);
			if (ec)
				return false;

			m_content.reset(new char[m_size]);

			auto inf = fs::fopen(path, "rb");
			size_t read = inf.load(m_content.get(), m_size);
			return read == m_size;
		}

		void close()
		{
			m_size = 0;
			m_content.reset();
		}

		uint32_t intFromOffset(size_t off)
		{
			if (off + sizeof(uint32_t) > m_size)
				return 0;

			return *(uint32_t*)(m_content.get() + off);
		}

		uint32_t offsetsValid(size_t off, size_t count, uint32_t hashtop)
		{
			for (size_t i = 0; i < count; ++i) {
				auto chunk = off + i * 8;
				auto length = intFromOffset(chunk);
				auto offset = intFromOffset(chunk + 4);
				char c = 0;
				if (length > 0)
					c = m_content[offset + length];
				if (offset < hashtop || offset > m_size)
					return false;
				if (m_size - offset < length || c != 0)
					return false;
			}

			return true;
		}

		std::vector<char> getString(size_t off, size_t i) {
			auto chunk = off + i * 8;
			auto length = intFromOffset(chunk);
			auto offset = intFromOffset(chunk + 4);
			return{ m_content.get() + offset, m_content.get() + offset + length };
		}
	};

	std::string gtt_key(const std::vector<char>& val)
	{
		auto b = std::begin(val);
		auto e = std::end(val);
		auto c = b;

		while (c != e && *c != 0x04) ++c;

		return{ b, c };
	}

	class gtt_stream {
		GetTextFile& m_ref;
		size_t m_offset = 0;
	public:
		gtt_stream(GetTextFile& ref) : m_ref { ref }
		{
		}

		uint32_t next()
		{
			auto tmp = m_offset;
			m_offset += sizeof(4);
			return m_ref.intFromOffset(m_offset);
		}

		size_t tell() const { return m_offset; }
	};

	std::map<std::string, std::string> open(const fs::path& path)
	{
		using s2s = std::map<std::string, std::string>;

		GetTextFile gtt;
		if (!gtt.open(path))
			return s2s { };

		gtt_stream in { gtt };
		if (0x950412de != in.next()) return s2s { };
		if (0 != in.next()) return s2s { };

		auto count = in.next();
		auto originals = in.next();
		auto translation = in.next();
		auto hashsize = in.next();
		auto hashpos = in.next();

		if (originals < in.tell()) return s2s { };
		if (translation < originals) return s2s { };
		if (hashpos < translation) return s2s { };
		if (translation - originals < count * 4) return s2s { };
		if (hashpos - translation < count * 4) return s2s { };

		if (!gtt.offsetsValid(originals, count, hashpos + hashsize)) return s2s { };
		if (!gtt.offsetsValid(translation, count, hashpos + hashsize)) return s2s { };

		s2s out;
		for (size_t i = 0; i < count; ++i) {
			auto orig = gtt.getString(originals, i);
			auto trans = gtt.getString(translation, i);
			out[gtt_key(orig)] = { trans.begin(), trans.end() };
		}
		return out;
	}
}
