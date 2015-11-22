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

#pragma once

#include <cstdint>
#include <cstdio>

namespace locale {
	struct instream {
		virtual ~instream() {}
		virtual std::size_t read(void* buffer, std::size_t length) = 0;
		virtual bool eof() const = 0;
		virtual char peek() = 0;
	};

	class finstream : public instream {
		enum { buf_size = 1024 };
		std::FILE* ptr;
		char buffer[buf_size];
		char* cur = buffer;
		char* end = buffer;
		bool seen_eof = false;

		void underflow();
	public:
		finstream(std::FILE* ptr) : ptr(ptr) {}
		std::size_t read(void* data, std::size_t length) override;
		bool eof() const override;
		char peek() override;
	};

	class meminstream : public instream {
		enum { buf_size = 1024 };
		const char* ptr;
		size_t length;
		const char* cur = ptr;
		const char* end = ptr + length;
	public:
		meminstream(const char* ptr, std::size_t length) : ptr(ptr), length(length) {}
		std::size_t read(void* data, std::size_t length) override;
		bool eof() const override;
		char peek() override;
	};

	struct outstream {
		virtual ~outstream() {}
		virtual std::size_t write(const void* buffer, std::size_t length) = 0;

		std::size_t write(const std::string& s)
		{
			return write(s.c_str(), s.length());
		}

		template <typename T>
		std::size_t write(const T& obj)
		{
			return write(&obj, sizeof(obj)) / sizeof(obj); // will yield 1 on success or 0 on error
		}
	};

	class foutstream : public outstream {
		std::FILE* ptr;
	public:
		foutstream(std::FILE* ptr) : ptr(ptr) {}
		std::size_t write(const void* data, std::size_t length) override
		{
			return std::fwrite(data, 1, length, ptr);
		}
	};

	class memoutstream : public outstream {
		std::vector<char> m_data;
	public:
		~memoutstream() {
			print();
		}

		std::size_t write(const void* ptr, std::size_t size) override
		{
			m_data.insert(m_data.end(), (const char*)ptr, (const char*)ptr + size);
			return size;
		}

		void print() const;
	};

}
