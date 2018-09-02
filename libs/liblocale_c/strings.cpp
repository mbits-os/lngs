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
		using identifier = lang_file::identifier;
		using quantity = lang_file::quantity;

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

		const char* get_string(identifier id) const
		{
			return m_file.get_string(id).data();
		}

		const char* get_string(identifier id, quantity count) const
		{
			return m_file.get_string(id, count).data();
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
	return tr->get_string((locale::lang_file::identifier)id);
}

API(const char*, ReadStringPl)(HSTRINGS opaque, uint32_t id, intmax_t count)
{
	auto tr = (locale::DllTranslation*)opaque;
	if (!tr)
		return nullptr;
	return tr->get_string((locale::lang_file::identifier)id, (locale::lang_file::quantity)count);
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
