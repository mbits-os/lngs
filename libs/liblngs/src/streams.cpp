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
#include <fmt/format.h>

namespace lngs {
	instream::~instream() = default;
	void buffered_instream::underflow() noexcept
	{
		auto really_read = underflow(buffer_);
		cur_ = buffer_;
		end_ = cur_ + really_read;
		seen_eof_ = !really_read;
	}

	std::size_t buffered_instream::read(void* data, std::size_t length) noexcept
	{
		auto dst = reinterpret_cast<char*>(data);
		auto size = length;
		while (size) {
			if (end_ == cur_)
				underflow();

			std::size_t chunk = end_ - cur_;
			if (chunk > size)
				chunk = size;

			if (!chunk)
				break;

			std::memcpy(dst, cur_, chunk);
			dst += chunk;
			cur_ += chunk;
			size -= chunk;
		}

		return length - size;
	}

	bool buffered_instream::eof() const noexcept
	{
		return seen_eof_;
	}

	std::byte buffered_instream::peek() noexcept
	{
		if (end_ == cur_)
			underflow();

		return *cur_;
	}

	size_t std_instream::underflow(std::byte(&buffer)[buf_size]) noexcept
	{
		return std::fread(buffer, 1, sizeof(buffer), ptr);
	}

	size_t finstream::underflow(std::byte(&buffer)[buf_size]) noexcept
	{
		return file.load(buffer, buf_size);
	}

	std::size_t meminstream::read(void* data, std::size_t len) noexcept
	{
		auto size = len;
		size_t rest = end - cur;
		if (rest < size)
			size = rest;

		std::memcpy(data, cur, size);
		cur += size;
		return size;
	}

	bool meminstream::eof() const noexcept
	{
		return end == cur;
	}

	std::byte meminstream::peek() noexcept
	{
		if (end == cur)
			return {};
		return *cur;
	}

	outstream::~outstream() = default;

	std::size_t outstream::vprint(fmt::string_view format_str, fmt::format_args args) {
		fmt::memory_buffer buffer;
		vformat_to(buffer, format_str, args);
		return write(buffer.data(), buffer.size());
	}

	std_outstream& get_stdout() {
		static std_outstream stdout__{ stdout };
		return stdout__;
	}

	std_outstream& get_stderr() {
		static std_outstream stderr__{ stderr };
		return stderr__;
	}

}
