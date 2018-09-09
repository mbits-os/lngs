/*
 * Copyright (C) 2018 midnightBITS
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

#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <variant>
#include <locale/file.hpp>
#include <lngs/streams.hpp>
#include <lngs/strings/lngs.hpp>

namespace lngs {
	struct strings {
		virtual ~strings();
		virtual std::string_view get(lng str) const = 0;
	};

	enum class severity {
		verbose,
		note,
		warning,
		error,
		fatal
	};

	struct location_severity;
	struct argumented_string;
	struct diagnostic;
	class diagnostics;

	struct location {
		unsigned token{ 0 };
		unsigned line{ 0 };
		unsigned column{ 0 };
		constexpr location moved(unsigned ln, unsigned col = 0) const {
			return  { token, ln, col };
		}

		constexpr location_severity operator[](severity sev) const;
	};

	struct location_severity {
		location loc;
		severity sev;
	};

	constexpr location_severity location::operator[](severity sev) const {
		return { *this, sev };
	}

	struct location_range_severity {
		location start;
		location end;
		severity sev;
	};

	struct location_range {
		location start;
		location end;
		constexpr location_range_severity operator[](severity sev) const {
			return { start, end, sev };
		}
	};

	constexpr inline location_range operator/(const location& start, const location& end) {
		return { start, end };
	}

	constexpr inline location_range_severity operator/(const location& start, const location_severity& loc_sev) {
		return { start, loc_sev.loc, loc_sev.sev };
	}

	struct argumented_string {
		using subs = std::vector<argumented_string>;
		std::variant<std::string, lng> value;
		std::vector<argumented_string> args;

		argumented_string() = default;

		argumented_string(std::string value) : value{ std::move(value) } {}

		argumented_string(lng value) : value{ value } {}

		argumented_string(std::string value, std::vector<argumented_string> args)
			: value{ std::move(value) }
			, args{ std::move(args) }
		{
		}

		argumented_string(lng value, std::vector<argumented_string> args)
			: value{ value }
			, args{ std::move(args) }
		{
		}

		template <typename ... Args>
		argumented_string(std::string value, argumented_string first, Args ... args)
			: argumented_string{ std::move(value), subs{ std::move(first), argumented_string(std::move(args))... } }
		{
		}

		template <typename ... Args>
		argumented_string(lng value, argumented_string first, Args ... args)
			: argumented_string{ value, subs{ std::move(first), argumented_string(std::move(args))... } }
		{
		}

		argumented_string& operator<<(argumented_string arg) {
			args.emplace_back(std::move(arg));
			return *this;
		}

		void print(outstream& o, const strings& tr) const;
		std::string str(const strings& tr) const;

		bool operator==(const argumented_string& rhs) const noexcept;
		bool operator!=(const argumented_string& rhs) const noexcept { return !(*this == rhs); }
	};

	template <typename ... Args>
	[[nodiscard]] argumented_string arg(std::string value, Args ... args)
	{
		return { std::move(value), argumented_string(std::move(args))... };
	}

	template <typename ... Args>
	[[nodiscard]] argumented_string arg(lng value, Args ... args)
	{
		return { value, argumented_string(std::move(args))... };
	}

	enum class link_type {
		gcc,
		vc,
#ifdef _WIN32
		native = vc
#else
		native = gcc
#endif
	};
	struct diagnostic {
		location pos;
		location end_pos;
		severity sev{ severity::note };
		argumented_string message;
		std::vector<diagnostic> children;

		static constexpr const size_t tab_size = 3;

		void print(outstream& o, const class diagnostics& host, const strings& tr, link_type links = link_type::native, size_t depth = 0) const;

		static std::tuple<std::string, std::size_t, std::size_t> prepare(std::string_view, std::size_t start_col, std::size_t end_col);

		diagnostic& with(std::initializer_list<diagnostic> subs) {
			children.insert(end(children), begin(subs), end(subs));
			return *this;
		}

		diagnostic& with(diagnostic sub) {
			children.emplace_back(std::move(sub));
			return *this;
		}
	};

	inline diagnostic operator<< (const location_severity& loc_sev, argumented_string msg) {
		return { loc_sev.loc, {}, loc_sev.sev, std::move(msg), {} };
	}

	inline diagnostic operator<< (const location_range_severity& range_sev, argumented_string msg) {
		return { range_sev.start, range_sev.end, range_sev.sev, std::move(msg), {} };
	}

	class source_file : public instream {
		friend class diagnostics;
		class source_view;
		std::shared_ptr<source_view> view_;
		size_t position_{ 0 };
		source_file(std::shared_ptr<source_view> view) : view_{ view } {}
	public:
		source_file() = default;
		~source_file();
		source_file(const source_file&) = delete;
		source_file& operator=(const source_file&) = delete;
		source_file(source_file&&) = default;
		source_file& operator=(source_file&&) = default;

		[[nodiscard]] bool valid() const noexcept;

		std::size_t read(void* buffer, std::size_t length) noexcept final;
		bool eof() const noexcept final;
		std::byte peek() noexcept final;
		const std::vector<std::byte>& data() const noexcept;
		std::string_view line(unsigned no) noexcept;

		location position(unsigned line = 0, unsigned column = 0);
	};

	class diagnostics {
		friend class source_file::source_view;
		struct bucket_item {
			std::string path;
			std::shared_ptr<source_file::source_view> contents;
			unsigned key;
		};

		using hasher = std::hash<std::string>;
		using hashed = decltype(std::declval<hasher>()(std::declval<std::string>()));
		using bucket = std::vector<bucket_item>;

		std::map<hashed, bucket, std::less<>> files_;
		std::map<unsigned, hashed> reverse_;
		unsigned current_value_ = 0;

		std::vector<diagnostic> set_;

		template <typename Key>
		bucket_item* lookup(const Key& path);

		template <typename Key>
		const bucket_item* lookup(const Key& path) const;
	public:
		std::string_view filename(const location&) const;
		source_file open(const std::string& path, const char* mode = "r");
		source_file source(const std::string_view& path);
		source_file source(const std::string_view& path) const;
		void set_contents(const std::string& path, std::vector<std::byte> data);
		void set_contents(const std::string& path, std::string_view data);

		void push_back(diagnostic diag);

		const std::vector<diagnostic>& diagnostic_set() const noexcept { return set_; }

		bool has_errors() const noexcept;
	};

	struct printer_anchor {
		outstream& out;
		diagnostics& diag;
		strings& tr;
		~printer_anchor();
	};
}
