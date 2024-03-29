// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

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
		    SingularStrings<Enum,
		                    typename Storage::template rebind<NextStorage>>;

		std::string_view operator()(Enum val) const noexcept {
			auto const id = static_cast<lang_file::identifier>(val);
			return Storage::get_string(id);
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
		    PluralOnlyStrings<Enum,
		                      typename Storage::template rebind<NextStorage>>;

		std::string_view operator()(Enum val, intmax_t count) const noexcept {
			auto const id = static_cast<lang_file::identifier>(val);
			auto const quantity = static_cast<lang_file::quantity>(count);
			return Storage::get_string(id, quantity);
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
		std::string_view operator()(PEnum val, intmax_t count) const noexcept {
			auto const id = static_cast<lang_file::identifier>(val);
			auto const quantity = static_cast<lang_file::quantity>(count);
			return SingularStrings<SEnum, Storage>::get_string(id, quantity);
		}
	};
}  // namespace lngs
