// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>
#include <lngs/lngs_base.hpp>
#include <lngs/plurals.hpp>
#include <memory>
#include <string_view>

namespace lngs {

	struct memory_view {
		const std::byte* contents = nullptr;
		uintmax_t size = 0;
	};

	struct lang_file {
		enum class identifier : uint32_t {};
		enum class quantity : intmax_t {};
		lang_file() noexcept;
		~lang_file() noexcept { close(); }
		bool open(const memory_view& view) noexcept;
		void close() noexcept;
		unsigned get_serial() const noexcept;
		std::string_view get_string(identifier id) const noexcept;
		std::string_view get_string(identifier id,
		                            quantity count) const noexcept;
		std::string_view get_attr(uint32_t id) const noexcept;
		std::string_view get_key(uint32_t id) const noexcept;
		uint32_t find_key(std::string_view id) const noexcept;
		uint32_t size() const noexcept { return strings.count; }
		intmax_t calc_substring(quantity count) const;

	private:
		struct section {
			uint32_t count = 0;
			const string_key* keys = nullptr;
			const char* strings = nullptr;
			void close() noexcept {
				count = 0;
				keys = nullptr;
				strings = nullptr;
			}
			const string_key* get(identifier id) const noexcept;
			std::string_view string(identifier id) const noexcept;
			std::string_view string(const string_key& key) const noexcept;

			const string_key* begin() const noexcept { return keys; }
			const string_key* end() const noexcept { return keys + count; }

			bool read_strings(const string_header* sec) noexcept;
		};
		unsigned serial;
		section attrs;
		section strings;
		section keys;
		mutable plurals::lexical lex;
	};
}  // namespace lngs
