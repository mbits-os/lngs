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
