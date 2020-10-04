// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <assert.h>
#include <cstdio>
#include <lngs/translation.hpp>
#include <memory>

namespace lngs {
	namespace {
		struct fcloser {
			void operator()(FILE* f) { std::fclose(f); }
		};
		using file = std::unique_ptr<FILE, fcloser>;

		file fopen(std::filesystem::path path, char const* mode) noexcept {
			path.make_preferred();
#if defined WIN32 || defined _WIN32
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
			std::unique_ptr<wchar_t[]> heap;
			wchar_t buff[20];
			wchar_t* ptr = buff;
			auto len = mode ? strlen(mode) : 0;
			if (len >= sizeof(buff)) {
				heap.reset(new (std::nothrow) wchar_t[len + 1]);
				if (!heap) return nullptr;
				ptr = heap.get();
			}

			auto dst = ptr;
			while (*dst++ = *mode++)
				;

			return file{::_wfopen(path.native().c_str(), ptr)};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else  // WIN32 || _WIN32
			return file{std::fopen(path.string().c_str(), mode)};
#endif
		}

		std::vector<std::byte> read(file const& ptr) noexcept {
			std::vector<std::byte> out;
			std::byte buffer[1024];

			while (true) {
				auto ret = std::fread(buffer, 1, sizeof(buffer), ptr.get());
				if (!ret) {
					if (!std::feof(ptr.get())) out.clear();
					break;
				}
				out.insert(end(out), buffer, buffer + ret);
			}

			return out;
		}
	}  // namespace

	/* static */
	memory_block translation::open_file(
	    const std::filesystem::path& path) noexcept {
		memory_block block;

		auto file = fopen(path, "rb");
		if (!file) return block;

		block.block = read(file);
		block.contents = block.block.data();
		block.size = block.block.size();

		return block;
	}

	bool translation::open(const std::string& lng, SerialNumber serial) {
		assert(m_path_mgr);
		m_path = m_path_mgr->expand(lng);
		m_mtime = mtime();

		m_file.close();
		m_path.make_preferred();
		m_data = open_file(m_path);

		auto const check_serial = serial != SerialNumber::UseAny;
		auto const serial_to_check = static_cast<unsigned>(serial);
		if (!m_file.open(m_data) ||
		    (check_serial && m_file.get_serial() != serial_to_check)) {
			m_file.close();
			m_data = memory_block{};
			m_mtime = decltype(m_mtime){};

			onupdate();
			return false;
		}

		onupdate();
		return true;
	}

	std::string_view translation::get_string(identifier id) const noexcept {
		return m_file.get_string(id);
	}

	std::string_view translation::get_string(identifier id,
	                                         quantity count) const noexcept {
		return m_file.get_string(id, count);
	}

	std::string_view translation::get_attr(uint32_t id) const noexcept {
		return m_file.get_attr(id);
	}

	std::string_view translation::get_key(uint32_t id) const noexcept {
		return m_file.get_key(id);
	}

	uint32_t translation::find_key(std::string_view id) const noexcept {
		return m_file.find_key(id);
	}

	std::vector<culture> translation::known() const {
		assert(m_path_mgr);
		auto files = m_path_mgr->known();

		std::vector<culture> out;

		for (auto& path : files) {
			lang_file file;
			path.make_preferred();
			auto view = open_file(path);
			if (!file.open(view)) continue;

			auto lang = file.get_attr(ATTR_CULTURE);
			auto name = file.get_attr(ATTR_LANGUAGE);

			culture c;
			if (!lang.empty()) c.lang = lang;
			if (!name.empty()) c.name = name;

			auto copy = m_path_mgr->expand(c.lang);
			copy.make_preferred();
			if (copy != path) continue;

			out.push_back(c);
		}

		return out;
	}

	uint32_t translation::add_onupdate(const std::function<void()>& fn) {
		if (!fn) return 0;
		++m_nextupdate;
		if (!m_nextupdate) ++m_nextupdate;
		m_updatelisteners[m_nextupdate] = fn;
		return m_nextupdate;
	}

	void translation::remove_onupdate(uint32_t token) {
		auto it = m_updatelisteners.find(token);
		if (it != m_updatelisteners.end()) m_updatelisteners.erase(it);
	}

	void translation::onupdate() {
		auto copy = m_updatelisteners;
		for (auto& pair : copy)
			pair.second();
	}
}  // namespace lngs
