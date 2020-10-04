// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <assert.h>
#include <limits>
#include <lngs/translation.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace lngs {
	std::vector<std::string> system_locales(bool init_setlocale = true);
	std::vector<std::string> http_accept_language(std::string_view header);
	namespace storage {
		class FileBased {
			std::shared_ptr<translation> m_impl;

		protected:
			using identifier = lang_file::identifier;
			using quantity = lang_file::quantity;

			template <typename NextStorage = storage::FileBased>
			using rebind = NextStorage;

			std::string_view get_string(identifier val) const noexcept {
				assert(m_impl);
				return m_impl->get_string(val);
			}

			std::string_view get_string(identifier val,
			                            quantity count) const noexcept {
				assert(m_impl);
				return m_impl->get_string(val, count);
			}

			std::string_view get_attr(uint32_t val) const noexcept {
				assert(m_impl);
				return m_impl->get_attr(val);
			}

			std::string_view get_key(uint32_t val) const noexcept {
				assert(m_impl);
				return m_impl->get_key(val);
			}

			uint32_t find_key(std::string_view val) const noexcept {
				assert(m_impl);
				return m_impl->find_key(val);
			}

			template <typename C>
			bool open_range(C&& langs, SerialNumber serial) {
				for (auto& lang : langs) {
					if (open(lang, serial)) return true;
				}

				return false;
			}

		public:
			template <typename Manager, typename... Args>
			void path_manager(Args&&... args) {
				m_impl = std::make_shared<translation>();
				m_impl->path_manager<Manager>(std::forward<Args>(args)...);
			}

			bool open(const std::string& lng, SerialNumber serial) {
				assert(m_impl);
				return m_impl->open(lng, serial);
			}

			template <typename T, typename C>
			using is_range_of = std::integral_constant<
			    bool,
			    std::is_reference<decltype(
			        *std::begin(std::declval<C>()))>::value &&
			        std::is_convertible<decltype(
			                                *std::begin(std::declval<C>())),
			                            T>::value>;

			template <typename T>
			std::enable_if_t<std::is_convertible<T, std::string>::value, bool>
			open_first_of(std::initializer_list<T> langs, SerialNumber serial) {
				return open_range(std::move(langs), serial);
			}

			template <typename C>
			std::enable_if_t<is_range_of<std::string, C>::value, bool>
			open_first_of(C&& langs, SerialNumber serial) {
				return open_range(std::forward<C>(langs), serial);
			}

			std::vector<culture> known() const {
				assert(m_impl);
				return m_impl->known();
			}

			uint32_t add_onupdate(const std::function<void()>& fn) {
				assert(m_impl);
				return m_impl->add_onupdate(fn);
			}

			void remove_onupdate(uint32_t token) {
				assert(m_impl);
				return m_impl->remove_onupdate(token);
			}
		};

		template <typename ResourceT>
		class Builtin {
			std::shared_ptr<lang_file> m_file;

		protected:
			using identifier = lang_file::identifier;
			using quantity = lang_file::quantity;

			template <typename NextStorage>
			using rebind = NextStorage;

			std::string_view get_string(identifier val) const noexcept {
				assert(m_file);
				return m_file->get_string(val);
			}

			std::string_view get_string(identifier val,
			                            quantity count) const noexcept {
				assert(m_file);
				return m_file->get_string(val, count);
			}

			std::string_view get_attr(uint32_t val) const noexcept {
				assert(m_file);
				return m_file->get_attr(val);
			}

			std::string_view get_key(uint32_t val) const noexcept {
				assert(m_file);
				return m_file->get_key(val);
			}

			uint32_t find_key(std::string_view val) const noexcept {
				assert(m_file);
				return m_file->find_key(val);
			}

		public:
			bool init_builtin() {
				m_file = std::make_shared<lang_file>();
				memory_view view;
				view.contents =
				    reinterpret_cast<std::byte const*>(ResourceT::data());
				view.size = ResourceT::size();
				return m_file->open(view);
			}
		};

		template <typename ResourceT>
		class FileWithBuiltin : private FileBased, private Builtin<ResourceT> {
			using B1 = FileBased;
			using B2 = Builtin<ResourceT>;

		protected:
			using identifier = lang_file::identifier;
			using quantity = lang_file::quantity;

			std::string_view get_string(identifier val) const noexcept {
				auto ret = B1::get_string(val);
				if (!ret.empty()) return ret;
				return B2::get_string(val);
			}

			std::string_view get_string(identifier val,
			                            quantity count) const noexcept {
				auto ret = B1::get_string(val, count);
				if (!ret.empty()) return ret;
				return B2::get_string(val, count);
			}

			std::string_view get_attr(uint32_t val) const noexcept {
				auto ret = B1::get_attr(val);
				if (!ret.empty()) return ret;
				return B2::get_attr(val);
			}

			std::string_view get_key(uint32_t val) const noexcept {
				auto ret = B1::get_key(val);
				if (!ret.empty()) return ret;
				return B2::get_key(val);
			}

			uint32_t find_key(std::string_view val) const noexcept {
				auto ret = B1::find_key(val);
				if (ret != std::numeric_limits<uint32_t>::max()) return ret;
				return B2::find_key(val);
			}

		public:
			using FileBased::add_onupdate;
			using FileBased::known;
			using FileBased::open;
			using FileBased::open_first_of;
			using FileBased::path_manager;
			using FileBased::remove_onupdate;
			using Builtin<ResourceT>::init_builtin;
		};
	}  // namespace storage
}  // namespace lngs
