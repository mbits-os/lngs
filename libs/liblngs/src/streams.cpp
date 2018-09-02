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

#include <lngs/streams.hpp>
#include <cctype>
#include <cstring>

namespace locale {
	void finstream::underflow()
	{
		auto really_read = std::fread(buffer, 1, sizeof(buffer), ptr);
		cur = buffer;
		end = cur + really_read;
		seen_eof = !really_read;
	}

	std::size_t finstream::read(void* data, std::size_t length)
	{
		auto dst = reinterpret_cast<char*>(data);
		auto size = length;
		while (size) {
			if (end == cur)
				underflow();

			std::size_t chunk = end - cur;
			if (chunk > size)
				chunk = size;

			if (!chunk)
				break;

			std::memcpy(dst, cur, chunk);
			dst += chunk;
			cur += chunk;
			size -= chunk;
		}

		return length - size;
	}

	bool finstream::eof() const
	{
		return seen_eof;
	}

	char finstream::peek()
	{
		if (end == cur)
			underflow();

		return *cur;
	}

	std::size_t meminstream::read(void* data, std::size_t len)
	{
		auto size = len;
		size_t rest = end - cur;
		if (rest < size)
			size = rest;

		std::memcpy(data, cur, size);
		cur += size;
		return size;
	}

	bool meminstream::eof() const
	{
		return end == cur;
	}

	char meminstream::peek()
	{
		if (end == cur)
			return -1;
		return *cur;
	}

	void memoutstream::print() const
	{
		constexpr size_t line = 16;
		auto ptr = m_data.data();
		auto size = m_data.size();
		uint32_t off = 0;

		while (size > line) {
			printf("%08X ", off);
			for (size_t i = 0; i < line; ++i)
				printf("%02X ", (uint8_t)ptr[i]);

			for (size_t i = 0; i < line; ++i) {
				if (std::isprint((uint8_t)ptr[i]))
					putchar((uint8_t)ptr[i]);
				else
					putchar('.');
			}

			putchar('\n');

			size -= line;
			ptr += line;
			off += line;
		}

		if (size) {
			printf("%08X ", off);
			for (size_t i = 0; i < size; ++i)
				printf("%02X ", (uint8_t)ptr[i]);
			for (size_t i = size; i < line; ++i)
				printf("   ");

			for (size_t i = 0; i < size; ++i) {
				if (std::isprint((uint8_t)ptr[i]))
					putchar((uint8_t)ptr[i]);
				else
					putchar('.');
			}

			putchar('\n');
		}
	}
}
