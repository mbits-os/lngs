// Copyright (c) 2013 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <functional>
#include <lngs/lngs_file.hpp>
#include <map>
#include <vector>

namespace lngs {
	using path = std::filesystem::path;
	namespace manager {
		// uses base and filename to generate paths "<base>/<lng>/<filename>"
		class SubdirPath {
			std::filesystem::path m_base;
			std::filesystem::path m_fname;

		public:
			explicit SubdirPath(std::filesystem::path base, std::string fname)
			    : m_base(std::move(base)), m_fname(std::move(fname)) {}

			std::filesystem::path expand(const std::string& lng) const {
				return m_base / lng / m_fname;
			}

			std::vector<std::filesystem::path> known() const {
				std::vector<std::filesystem::path> out;

				for (auto& entry :
				     std::filesystem::directory_iterator{m_base}) {
					std::error_code ec;
					auto stat = entry.status(ec);
					if (ec) continue;

					if (!std::filesystem::is_directory(stat)) continue;

					auto full = entry.path() / m_fname;
					if (std::filesystem::is_regular_file(full))
						out.push_back(full);
				};

				return out;
			}
		};

		// uses base and filename to generate paths "<base>/<filename>.<lng>"
		class ExtensionPath {
			std::filesystem::path m_base;
			std::string m_fname;

		public:
			explicit ExtensionPath(std::filesystem::path base,
			                       std::string fname)
			    : m_base(std::move(base)), m_fname(std::move(fname)) {}

			std::filesystem::path expand(const std::string& lng) const {
				static std::string dot{"."};
				return m_base / (m_fname + dot + lng);
			}

			std::vector<std::filesystem::path> known() const {
				std::vector<std::filesystem::path> out;

				for (auto& entry :
				     std::filesystem::directory_iterator{m_base}) {
					std::error_code ec;
					auto stat = entry.status(ec);
					if (ec) continue;

					if (!std::filesystem::is_regular_file(stat)) continue;

					auto fname = entry.path().filename().replace_extension();
					if (fname == m_fname) out.push_back(entry.path());
				};

				return out;
			}
		};
	}  // namespace manager

	struct memory_block : memory_view {
		std::vector<std::byte> block;
	};

	struct culture {
		std::string lang;
		std::string name;
	};

	enum class SerialNumber : unsigned {
		UseAny = std::numeric_limits<unsigned>::max()
	};

	class translation {
		struct manager_t {
			virtual ~manager_t() {}
			virtual std::filesystem::path expand(
			    const std::string& lng) const = 0;
			virtual std::vector<std::filesystem::path> known() const = 0;
		};

		template <typename T>
		class manager_impl : public manager_t {
			T info;

		public:
			template <typename... Args>
			explicit manager_impl(Args&&... args)
			    : info(std::forward<Args>(args)...) {}
			std::filesystem::path expand(
			    const std::string& lng) const override {
				return info.expand(lng);
			}

			std::vector<std::filesystem::path> known() const override {
				return info.known();
			}
		};

		std::unique_ptr<manager_t> m_path_mgr;
		std::filesystem::path m_path;
		memory_block m_data;
		lang_file m_file;
		std::filesystem::file_time_type m_mtime;

		std::filesystem::file_time_type mtime() const noexcept {
			std::error_code ec;
			auto time = std::filesystem::last_write_time(m_path, ec);
			if (ec) return decltype(time){};
			return time;
		}

		std::map<uint32_t, std::function<void()>> m_updatelisteners;
		uint32_t m_nextupdate = 0xba5e0000;

		friend class translation_tests;

		void onupdate();

	public:
		using identifier = lang_file::identifier;
		using quantity = lang_file::quantity;

		static memory_block open_file(
		    const std::filesystem::path& path) noexcept;
		template <typename T, typename... Args>
		void path_manager(Args&&... args) {
			m_path_mgr =
			    std::make_unique<manager_impl<T>>(std::forward<Args>(args)...);
		}

		bool open(const std::string& lng, SerialNumber serial);
		bool fresh() const noexcept { return mtime() == m_mtime; }
		std::string_view get_string(identifier id) const noexcept;
		std::string_view get_string(identifier id,
		                            quantity count) const noexcept;
		std::string_view get_attr(uint32_t id) const noexcept;
		std::string_view get_key(uint32_t id) const noexcept;
		uint32_t find_key(std::string_view id) const noexcept;
		std::vector<culture> known() const;

		uint32_t add_onupdate(const std::function<void()>&);
		void remove_onupdate(uint32_t token);
	};
}  // namespace lngs
