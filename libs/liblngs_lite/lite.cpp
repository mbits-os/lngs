// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/lite.h>

#ifdef _WIN32
#include <windows.h>
#undef min
#undef max
#endif

#include <lngs/lngs_file.hpp>
#include <lngs/translation.hpp>

#ifdef _WIN32
#define API(T, name) T __stdcall name
#else
#define API(T, name) T name
#endif

namespace lngs {
	class DllTranslation {
		memory_block m_data;
		lang_file m_file;

	public:
		using identifier = lang_file::identifier;
		using quantity = lang_file::quantity;

		bool open(const char* path) {
			m_data = translation::open_file(path);
			if (!m_file.open(m_data)) {
				m_file.close();
				m_data = memory_block{};
				return false;
			}
			return true;
		}

		const char* get_string(identifier id) const {
			return m_file.get_string(id).data();
		}

		const char* get_string(identifier id, quantity count) const {
			return m_file.get_string(id, count).data();
		}
	};

}  // namespace lngs

namespace {
	inline auto from(HSTRINGS opaque) {
		return reinterpret_cast<lngs::DllTranslation*>(opaque);
	}
}  // namespace

API(HSTRINGS, OpenStrings)(const char* path) {
	auto tr = std::make_unique<lngs::DllTranslation>();
	if (!tr->open(path)) return nullptr;

	return reinterpret_cast<HSTRINGS>(tr.release());
}

API(const char*, ReadString)(HSTRINGS opaque, uint32_t id) {
	auto tr = from(opaque);
	if (!tr) return nullptr;
	auto const ident = static_cast<lngs::lang_file::identifier>(id);
	return tr->get_string(ident);
}

API(const char*, ReadStringPl)(HSTRINGS opaque, uint32_t id, intmax_t count) {
	auto tr = from(opaque);
	if (!tr) return nullptr;
	auto const ident = static_cast<lngs::lang_file::identifier>(id);
	auto const quantity = static_cast<lngs::lang_file::quantity>(count);
	return tr->get_string(ident, quantity);
}

API(void, CloseStrings)(HSTRINGS opaque) {
	auto tr = from(opaque);
	delete tr;
}

#ifdef _WIN32
BOOL WINAPI DllMain(_In_ HINSTANCE /* hinstDLL */,
                    _In_ DWORD /* fdwReason */,
                    _In_ LPVOID /* lpvReserved */
) {
	return TRUE;
}
#endif
