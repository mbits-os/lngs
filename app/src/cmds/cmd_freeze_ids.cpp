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

#include <lngs/internals/commands.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>

#include <lngs/internals/diagnostics.hpp>

#include <algorithm>

namespace lngs::app::freeze {
	bool freeze(idl_strings& defs) {
		if (!defs.has_new) return false;

		++defs.serial;

		for (auto& s : defs.strings) {
			s.value.clear();
			s.help.clear();
			s.plural.clear();
		}

		sort(begin(defs.strings), end(defs.strings),
		     [](auto&& left, auto&& right) {
			     return left.id_offset < right.id_offset;
		     });

		return true;
	}

	constexpr std::byte operator""_b(char c) {
		return static_cast<std::byte>(c);
	}
	std::pair<size_t, size_t> value_pos(const std::byte* start) {
		size_t first = 0;
		while (*start != '('_b)
			++start, ++first;
		size_t second = first;
		while (*start != ')'_b)
			++start, ++second;

		return {first, second};
	}

	int write(outstream& out, const idl_strings& defs, source_file& source) {
		const auto& data = source.data();
		const std::byte* bytes = data.data();
		size_t offset = 0;
		auto const serial_offset = static_cast<size_t>(defs.serial_offset);
		auto [from, to] = value_pos(bytes + serial_offset);
		out.write(bytes, serial_offset + from + 1);
		out.fmt("{}", defs.serial);
		offset = serial_offset + to;

		for (auto& str : defs.strings) {
			auto const id_offset = static_cast<size_t>(str.id_offset);
			auto [str_from, str_to] = value_pos(bytes + id_offset);
			out.write(bytes + offset, id_offset + str_from + 1 - offset);
			out.fmt("{}", str.id);
			offset = id_offset + str_to;
		}

		out.write(bytes + offset, data.size() - offset);
		return 0;
	}
}  // namespace lngs::app::freeze
