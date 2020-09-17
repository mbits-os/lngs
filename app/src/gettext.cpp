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

#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/gettext.hpp>
#include <string_view>

namespace gtt {
	using namespace lngs::app;

	class gtt_stream {
		const std::vector<std::byte>& m_ref;
		size_t m_offset = 0;
		bool m_reverse = false;

	public:
		gtt_stream(const std::vector<std::byte>& ref) : m_ref{ref} {}

		uint32_t next() {
			auto tmp = m_offset;
			m_offset += sizeof(4);
			return intFromOffset(tmp);
		}

		void reverse() { m_reverse = true; }

		size_t tell() const { return m_offset; }

		uint32_t intFromOffset(size_t off) const noexcept {
			const auto length = m_ref.size();
			const auto rest = length >= off ? length - off : 0;

			if (rest < sizeof(uint32_t)) return 0;

			uint32_t out = 0;
			auto ptr = m_ref.data() + off;

			if (m_reverse) {
				for (size_t b = 0; b < sizeof(uint32_t); ++b, ++ptr) {
					out <<= 8;
					out |= std::to_integer<uint32_t>(*ptr);
				}
			} else {
				out = *reinterpret_cast<const uint32_t*>(ptr);
			}

			return out;
		}

		static bool gtt_error(lng code, source_file& src, diagnostics& diags) {
			auto diag = src.position()[severity::error]
			            << lng::ERR_GETTEXT_FORMAT;
			diag.children.push_back(src.position()[severity::note] << code);
			diags.push_back(std::move(diag));
			return false;
		}

		bool offsetsValid(size_t off,
		                  size_t count,
		                  uint32_t hashtop,
		                  source_file& src,
		                  diagnostics& diags) const noexcept {
			const auto size = m_ref.size();

			for (size_t i = 0; i < count; ++i) {
				auto chunk = off + i * 8;
				auto length = intFromOffset(chunk);
				auto offset = intFromOffset(chunk + 4);
				if (offset < hashtop)
					return gtt_error(lng::ERR_GETTEXT_STRING_OUTSIDE, src,
					                 diags);

				if (offset > size)
					return gtt_error(lng::ERR_GETTEXT_FILE_TRUNCATED, src,
					                 diags);

				const auto rest = size >= offset ? size - offset : 0;
				if (rest < length)
					return gtt_error(lng::ERR_GETTEXT_FILE_TRUNCATED, src,
					                 diags);
				if (length > 0) {
					if (rest == length)
						return gtt_error(lng::ERR_GETTEXT_NOT_ASCIIZ, src,
						                 diags);
					auto c = std::to_integer<unsigned>(m_ref[offset + length]);
					if (c != 0)
						return gtt_error(lng::ERR_GETTEXT_NOT_ASCIIZ, src,
						                 diags);
				}
			}

			return true;
		}

		std::string_view getString(size_t off, size_t i) const noexcept {
			auto chunk = off + i * 8;
			auto length = intFromOffset(chunk);
			auto offset = intFromOffset(chunk + 4);
			return {reinterpret_cast<const char*>(m_ref.data()) + offset,
			        length};
		}
	};

	static std::string gtt_key(const std::string_view& val) noexcept {
		auto pos = val.find(0x04);
		return {val.data(), pos == std::string_view::npos ? val.length() : pos};
	}

	std::map<std::string, std::string> open(source_file& src,
	                                        diagnostics& diags) {
		std::map<std::string, std::string> out;

		if (!src.valid()) {
			diags.push_back(src.position()[severity::error]
			                << lng::ERR_FILE_NOT_FOUND);
			return {};
		}

		auto& contents = src.data();
		gtt_stream in{contents};
		auto magic = in.next();
		if (magic == 0xde120495) {
			in.reverse();
			if (0 != in.next()) return out;
		} else {
			if ((0x950412de != magic) || (0 != in.next())) {
				return out;
			}
		}

		auto count = in.next();
		auto originals = in.next();
		auto translation = in.next();
		auto hashsize = in.next();
		auto hashpos = in.next();

		if ((originals < in.tell()) || (translation < originals) ||
		    (hashpos < translation)) {
			gtt_stream::gtt_error(lng::ERR_GETTEXT_BLOCKS_OVERLAP, src, diags);
			return {};
		}
		if (((translation - originals) < (count * 2 * sizeof(uint32_t))) ||
		    ((hashpos - translation) < (count * 2 * sizeof(uint32_t)))) {
			gtt_stream::gtt_error(lng::ERR_GETTEXT_FILE_TRUNCATED, src, diags);
			return {};
		}

		if (!in.offsetsValid(originals, count, hashpos + hashsize, src,
		                     diags) ||
		    !in.offsetsValid(translation, count, hashpos + hashsize, src,
		                     diags)) {
			return out;
		}

		for (size_t i = 0; i < count; ++i) {
			auto orig = in.getString(originals, i);
			auto trans = in.getString(translation, i);
			out[gtt_key(orig)] = {trans.begin(), trans.end()};
		}
		return out;
	}
}  // namespace gtt
