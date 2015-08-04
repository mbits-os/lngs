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

#pragma once

#include <translation.hpp>
#include <assert.h>
#include <type_traits>
#include <string>
#include <vector>

namespace locale {
	std::vector<std::string> system_locales(bool init_setlocale = true);
	namespace storage {
		class FileBased {
			std::shared_ptr<translation> m_impl;

		protected:
			const char* get_string(uint32_t val) const noexcept
			{
				assert(m_impl);
				return m_impl->get_string(val);
			}

			const char* get_string(uint32_t val, intmax_t count) const noexcept
			{
				assert(m_impl);
				return m_impl->get_string(count, val);
			}

			const char* get_attr(uint32_t val) const noexcept
			{
				assert(m_impl);
				return m_impl->get_attr(val);
			}

			const char* get_key(uint32_t val) const noexcept
			{
				assert(m_impl);
				return m_impl->get_key(val);
			}

			uint32_t find_key(const char* val) const noexcept
			{
				assert(m_impl);
				return m_impl->find_key(val);
			}


			template <typename C>
			bool open_range(C&& langs)
			{
				for (auto& lang : langs) {
					if (open(lang))
						return true;
				}

				return false;
			}
		public:
			template <typename Manager, typename... Args>
			void path_manager(Args&&... args)
			{
				m_impl = std::make_shared<translation>();
				m_impl->path_manager(Manager { std::forward<Args>(args)... });
			}

			bool open(const std::string& lng)
			{
				assert(m_impl);
				return m_impl->open(lng);
			}

			template <typename T, typename C>
			using is_range_of = std::integral_constant<bool,
				std::is_reference<decltype(*std::begin(std::declval<C>()))>::value &&
				std::is_convertible<decltype(*std::begin(std::declval<C>())), T>::value>;

			template <typename T>
			std::enable_if_t<std::is_convertible<T, std::string>::value, bool>
				open_first_of(std::initializer_list<T>&& langs)
			{
				return open_range(std::move(langs));
			}

			template <typename C>
			std::enable_if_t<is_range_of<std::string, C>::value, bool>
				open_first_of(C&& langs)
			{
				return open_range(std::forward<C>(langs));
			}
		};

		template <typename ResourceT>
		class Builtin {
			std::shared_ptr<lang_file> m_file;
		protected:
			const char* get_string(uint32_t val) const noexcept
			{
				assert(m_file);
				return m_file->get_string(val);
			}

			const char* get_string(uint32_t val, intmax_t count) const noexcept
			{
				assert(m_file);
				return m_file->get_string(count, val);
			}

			const char* get_attr(uint32_t val) const noexcept
			{
				assert(m_file);
				return m_file->get_attr(val);
			}

			const char* get_key(uint32_t val) const noexcept
			{
				assert(m_file);
				return m_file->get_key(val);
			}

			uint32_t find_key(const char* val) const noexcept
			{
				assert(m_file);
				return m_file->find_key(val);
			}

		public:
			bool init()
			{
				m_file = std::make_shared<lang_file>();
				memory_view view;
				view.contents = ResourceT::data();
				view.size = ResourceT::size();
				return m_file->open(view);
			}
		};

		template <typename ResourceT>
		class FileWithBuiltin : private FileBased, private Builtin<ResourceT> {
			using B1 = FileBased;
			using B2 = Builtin<ResourceT>;
		protected:
			const char* get_string(uint32_t val) const noexcept
			{
				auto ret = B1::get_string(val);
				if (ret)
					return ret;
				return B2::get_string(val);
			}

			const char* get_string(uint32_t val, intmax_t count) const noexcept
			{
				auto ret = B1::get_string(val, count);
				if (ret)
					return ret;
				return B2::get_string(val, count);
			}

			const char* get_attr(uint32_t val) const noexcept
			{
				auto ret = B1::get_attr(val);
				if (ret)
					return ret;
				return B2::get_attr(val);
			}

			const char* get_key(uint32_t val) const noexcept
			{
				auto ret = B1::get_key(val);
				if (ret)
					return ret;
				return B2::get_key(val);
			}

			uint32_t find_key(const char* val) const noexcept
			{
				auto ret = B1::find_key(val);
				if (ret != (uint32_t)-1)
					return ret;
				return B2::find_key(val);
			}
		public:
			using FileBased::path_manager;
			using FileBased::open;
			using FileBased::open_first_of;
			using Builtin<ResourceT>::init;
		};
	}
}
