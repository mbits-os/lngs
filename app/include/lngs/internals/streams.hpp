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
#include <lngs/file.hpp>
#include <fmt/core.h>

namespace lngs::app {
	struct instream {
		instream();
		instream(const instream&) = delete;
		instream(instream&&);
		instream& operator=(const instream&) = delete;
		instream& operator=(instream&&);

		virtual ~instream();
		virtual std::size_t read(void* buffer, std::size_t length) noexcept = 0;
		virtual bool eof() const noexcept = 0;
		virtual std::byte peek() noexcept = 0;
	};

	struct outstream {
		outstream();
		outstream(const outstream&) = delete;
		outstream(outstream&&);
		outstream& operator=(const outstream&) = delete;
		outstream& operator=(outstream&&);

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
		std_outstream(std::FILE* ptr) noexcept;
		std::size_t write(const void* data, std::size_t length) noexcept final;
	};

	std_outstream& get_stdout();

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
