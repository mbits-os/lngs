// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

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

	bool is_mo(lngs::app::source_file& src) {
		if (!src.valid()) return false;
		auto pos = src.tell();
		uint32_t magic;
		if (src.read(&magic, sizeof magic) != sizeof magic) {
			src.seek(pos);
			return false;
		}
		if (magic != 0xde120495 && magic != 0x950412de) {
			src.seek(pos);
			return false;
		}
		if (src.read(&magic, sizeof magic) != sizeof magic) {
			src.seek(pos);
			return false;
		}
		if (magic != 0) {
			src.seek(pos);
			return false;
		}

		src.seek(pos);
		return true;
	}

	std::map<std::string, std::string> open_mo(source_file& src,
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
