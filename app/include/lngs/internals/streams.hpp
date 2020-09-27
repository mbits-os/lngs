// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <fmt/core.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <lngs/file.hpp>

namespace lngs::app {
	struct instream {
		instream() = default;
		instream(const instream&) = delete;
		instream(instream&&) = default;
		instream& operator=(const instream&) = delete;
		instream& operator=(instream&&) = default;

		virtual ~instream();
		virtual std::size_t read(void* buffer, std::size_t length) noexcept = 0;
		virtual bool eof() const noexcept = 0;
		virtual std::byte peek() noexcept = 0;
		virtual std::size_t tell() const noexcept = 0;
		virtual std::size_t seek(std::size_t) noexcept = 0;
	};

	struct outstream {
		outstream() = default;
		outstream(const outstream&) = delete;
		outstream(outstream&&) = default;
		outstream& operator=(const outstream&) = delete;
		outstream& operator=(outstream&&) = default;

		virtual ~outstream();
		virtual std::size_t write(const void* buffer,
		                          std::size_t length) noexcept = 0;

		std::size_t write(const std::string& s) noexcept {
			return write(s.c_str(), s.length());
		}

		template <typename T>
		std::size_t write(const T& obj) noexcept {
			return write(&obj, sizeof(obj)) /
			       sizeof(obj);  // will yield 1 on success or 0 on error
		}

		template <typename... Args>
		inline std::size_t print(fmt::string_view format_str,
		                         const Args&... args) {
			fmt::format_arg_store<fmt::format_context, Args...> as(args...);
			return vprint(format_str, as);
		}

		template <typename... Args>
		inline std::size_t fmt(::fmt::string_view format_str,
		                       const Args&... args) {
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
		std::size_t write(const void* data, std::size_t length) noexcept final {
			return file.store(data, length);
		}
	};
}  // namespace lngs::app
