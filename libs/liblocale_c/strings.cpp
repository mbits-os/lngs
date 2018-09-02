#include <locale/strings.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <locale/locale_file.hpp>
#include <locale/translation.hpp>

#ifdef _WIN32
#define API(T, name) T __stdcall name
#else
#define API(T, name) T name
#endif

namespace locale {
	class DllTranslation {
		memory_block m_data;
		lang_file m_file;
	public:
		bool open(const char* path)
		{
			m_data = locale::translation::open_file(path);
			if (!m_file.open(m_data)) {
				m_file.close();
				m_data = memory_block { };
				return false;
			}
			return true;
		}

		const char* get_string(uint32_t id) const
		{
			return m_file.get_string(id);
		}

		const char* get_string(intmax_t count, uint32_t id) const
		{
			return m_file.get_string(count, id);
		}
	};

}

API(HSTRINGS, OpenStrings)(const char* path)
{
	auto tr = std::make_unique<locale::DllTranslation>();
	if (!tr->open(path))
		return nullptr;

	return (HSTRINGS)tr.release();
}

API(const char*, ReadString)(HSTRINGS opaque, uint32_t id)
{
	auto tr = (locale::DllTranslation*)opaque;
	if (!tr)
		return nullptr;
	return tr->get_string(id);
}

API(const char*, ReadStringPl)(HSTRINGS opaque, intmax_t count, uint32_t id)
{
	auto tr = (locale::DllTranslation*)opaque;
	if (!tr)
		return nullptr;
	return tr->get_string(count, id);
}

API(void, CloseStrings)(HSTRINGS opaque)
{
	auto tr = (locale::DllTranslation*)opaque;
	delete tr;
}

#ifdef _WIN32
BOOL WINAPI DllMain(
	_In_ HINSTANCE /* hinstDLL */,
	_In_ DWORD     /* fdwReason */,
	_In_ LPVOID    /* lpvReserved */
	)
{
	return TRUE;
}
#endif
