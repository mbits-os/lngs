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

#include <lngs/lngs_storage.hpp>

namespace lngs {
	template <unsigned Serial, typename Storage>
	class VersionedBuiltin : public Storage {
	public:
		static constexpr SerialNumber serial_number{Serial};

		template <typename NextStorage>
		using rebind = VersionedBuiltin<Serial, NextStorage>;
	};

	template <unsigned Serial, typename Storage = storage::FileBased>
	class VersionedFile : public Storage {
	public:
		static constexpr SerialNumber serial_number{Serial};

		template <typename NextStorage = storage::FileBased>
		using rebind = VersionedFile<Serial, NextStorage>;

		template <typename T, typename C>
		using is_range_of = std::integral_constant<
		    bool,
		    std::is_reference<decltype(
		        *std::begin(std::declval<C>()))>::value &&
		        std::is_convertible<decltype(*std::begin(std::declval<C>())),
		                            T>::value>;

		using Storage::open;
		using Storage::open_first_of;

		bool open(const std::string& lng) {
			return Storage::open(lng, serial_number);
		}

		template <typename T>
		std::enable_if_t<std::is_convertible<T, std::string>::value, bool>
		open_first_of(std::initializer_list<T> langs) {
			return Storage::open_first_of(langs, serial_number);
		}

		template <typename C>
		std::enable_if_t<is_range_of<std::string, C>::value, bool>
		open_first_of(C&& langs) {
			return Storage::open_first_of(langs, serial_number);
		}
	};

	template <typename Enum, typename Storage = storage::FileBased>
	class SingularStrings : public Storage {
	public:
		template <typename NextStorage = storage::FileBased>
		using rebind =
		    SingularStrings<Enum, typename Storage::template rebind<NextStorage>>;

		std::string operator()(Enum val) const noexcept {
			auto const id = static_cast<lang_file::identifier>(val);
			auto ptr = Storage::get_string(id);
			return !ptr.empty() ? std::string{ptr} : std::string{};
		}

		std::string attr(v1_0::attr_t val) const noexcept {
			auto ptr = Storage::get_attr(val);
			return !ptr.empty() ? std::string{ptr} : std::string{};
		}
	};

	template <typename Enum, typename Storage = storage::FileBased>
	class PluralOnlyStrings : public Storage {
	public:
		template <typename NextStorage = storage::FileBased>
		using rebind =
		    PluralOnlyStrings<Enum, typename Storage::template rebind<NextStorage>>;

		std::string operator()(Enum val, intmax_t count) const noexcept {
			auto const id = static_cast<lang_file::identifier>(val);
			auto const quantity = static_cast<lang_file::quantity>(count);
			auto ptr = Storage::get_string(id, quantity);
			return !ptr.empty() ? std::string{ptr} : std::string{};
		}

		std::string attr(v1_0::attr_t val) const noexcept {
			auto ptr = Storage::get_attr(val);
			return !ptr.empty() ? std::string{ptr} : std::string{};
		}
	};

	template <typename SEnum,
	          typename PEnum,
	          typename Storage = storage::FileBased>
	class StringsWithPlurals : public SingularStrings<SEnum, Storage> {
	public:
		template <typename NextStorage = storage::FileBased>
		using rebind =
		    StringsWithPlurals<SEnum,
		                       PEnum,
		                       typename Storage::template rebind<NextStorage>>;

		using SingularStrings<SEnum, Storage>::operator();  // un-hide
		std::string operator()(PEnum val, intmax_t count) const noexcept {
			auto const id = static_cast<lang_file::identifier>(val);
			auto const quantity = static_cast<lang_file::quantity>(count);
			auto ptr =
			    SingularStrings<SEnum, Storage>::get_string(id, quantity);
			return !ptr.empty() ? std::string{ptr} : std::string{};
		}
	};
}  // namespace lngs
