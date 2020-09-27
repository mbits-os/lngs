// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <assert.h>
#include <lngs/translation.hpp>

namespace lngs {
	/* static */
	memory_block translation::open_file(const fs::path& path) noexcept {
		memory_block block;

		auto file = fs::fopen(path, "rb");
		if (!file) return block;

		block.block = file.read();
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
