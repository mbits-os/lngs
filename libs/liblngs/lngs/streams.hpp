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

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <locale/file.hpp>
#include <fmt/core.h>

namespace lngs {
	struct instream {
		virtual ~instream();
		virtual std::size_t read(void* buffer, std::size_t length) noexcept = 0;
		virtual bool eof() const noexcept = 0;
		virtual std::byte peek() noexcept = 0;
	};

	class buffered_instream : public instream {
	protected:
		enum { buf_size = 1024 };
	private:
		std::byte buffer[buf_size];
		std::byte* cur = buffer;
		std::byte* end = buffer;
		bool seen_eof = false;

		virtual size_t underflow(std::byte (&buffer)[buf_size]) noexcept = 0;
		void underflow() noexcept;
	public:
		std::size_t read(void* data, std::size_t length) noexcept override;
		bool eof() const noexcept override;
		std::byte peek() noexcept override;
	};

	class std_instream final : public buffered_instream {
		std::FILE* ptr;
		size_t underflow(std::byte(&buffer)[buf_size]) noexcept final;
	public:
		std_instream(std::FILE* ptr) noexcept : ptr(ptr) {}
	};

	class finstream final : public buffered_instream {
		fs::file file;
		size_t underflow(std::byte(&buffer)[buf_size]) noexcept final;
	public:
		finstream(fs::file file) noexcept : file(std::move(file)) {}
	};

	class meminstream : public instream {
		const std::byte* ptr;
		size_t length;
		const std::byte* cur = ptr;
		const std::byte* end = ptr + length;
	public:
		meminstream(const std::byte* ptr, std::size_t length) noexcept : ptr(ptr), length(length) {}
		std::size_t read(void* data, std::size_t length) noexcept override;
		bool eof() const noexcept override;
		std::byte peek() noexcept override;
	};

	struct outstream {
		virtual ~outstream();
		virtual std::size_t write(const void* buffer, std::size_t length) noexcept = 0;

		std::size_t write(const std::string& s) noexcept
		{
			return write(s.c_str(), s.length());
		}

		template <typename T>
		std::size_t write(const T& obj) noexcept
		{
			return write(&obj, sizeof(obj)) / sizeof(obj); // will yield 1 on success or 0 on error
		}

		template <typename... Args>
		inline std::size_t print(fmt::string_view format_str, const Args & ... args) {
			fmt::format_arg_store<fmt::format_context, Args...> as(args...);
			return vprint(format_str, as);
		}

		template <typename... Args>
		inline std::size_t fmt(::fmt::string_view format_str, const Args & ... args) {
			::fmt::format_arg_store<::fmt::format_context, Args...> as(args...);
			return vprint(format_str, as);
		}
	private:
		std::size_t vprint(fmt::string_view format_str, fmt::format_args args);
	};

	class std_outstream : public outstream {
		std::FILE* ptr;
	public:
		std_outstream(std::FILE* ptr) noexcept : ptr(ptr) {}
		std::size_t write(const void* data, std::size_t length) noexcept final
		{
			return std::fwrite(data, 1, length, ptr);
		}
	};

	std_outstream& get_stdout();
	std_outstream& get_stderr();

	class foutstream : public outstream {
		fs::file file;
	public:
		foutstream(fs::file file) noexcept : file(std::move(file)) {}
		std::size_t write(const void* data, std::size_t length) noexcept final
		{
			return file.store(data, length);
		}
	};
}
