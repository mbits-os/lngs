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

#include <lngs/diagnostics.hpp>
#include <fmt/format.h>
#ifdef _WIN32
#include <lngs/utf8.hpp>
#include <windows.h>
#undef SEVERITY_ERROR
#undef min
#undef max
#endif

namespace lngs {
	strings::~strings() = default;

#ifdef _WIN32
	inline std::string oem_cp(std::string_view view) {
		auto u16 = utf::as_u16(view);
		auto cp = GetOEMCP();
		size_t str_size = WideCharToMultiByte(cp, 0, (LPCWSTR)u16.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (!str_size)
			return { view.data(), view.length() };

		std::string out(str_size - 1, ' ');
		auto result = WideCharToMultiByte(cp, 0, (LPCWSTR)u16.c_str(), -1, out.data(), str_size, nullptr, nullptr);
		if (!result)
			return { view.data(), view.length() };

		return out;
	}
#else
	inline std::string_view oem_cp(std::string_view view) { return view; }
#endif

	void argumented_string::print(outstream& o, const strings& tr) const {
		auto oem = std::visit([&](const auto& arg) {
			if constexpr (std::is_same_v<decltype(arg), const std::string&>) {
				return oem_cp(arg);
			} else
				return oem_cp(tr.get(arg));
		}, value);
		auto str = std::string_view{ oem };

		if (str.empty())
			return;

		if (args.empty()) {
			o.write(str.data(), str.length());
			return;
		}

		std::vector<std::string> storage;
		storage.reserve(args.size());
		for (auto const& arg : args)
			storage.push_back(arg.str(tr));

		auto args_data = std::make_unique<fmt::format_args::format_arg[]>(args.size());
		auto it = args_data.get();
		for (auto const& arg : storage)
			*it++ = fmt::internal::make_arg<fmt::format_context>(arg);
		fmt::format_args f_args{ args_data.get(), args.size() };

		fmt::memory_buffer buffer;
		vformat_to(buffer, str, f_args);
		o.write(buffer.data(), buffer.size());
	}

	std::string argumented_string::str(const strings& tr) const {
		struct string_builder : outstream {
			std::string s;
			std::size_t write(const void* buffer, std::size_t length) noexcept {
				try {
					s.reserve(s.length() + length + 1);
				} catch (std::bad_alloc&) {
					return 0;
				}
				s.append((const char*)buffer, length);
				return length;
			}
		} out;
		print(out, tr);
		return std::move(out.s);
	}

	bool argumented_string::operator==(const argumented_string& rhs) const noexcept {
		if (value.index() != rhs.value.index())
			return false;

		if (!std::visit([](const auto& lhs, const auto& rhs) {
			if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>)
				return lhs == rhs;
			else
				return false;
		}, value, rhs.value))
			return false;

		if (args.size() != rhs.args.size())
			return false;

		auto it = begin(rhs.args);
		for (const auto& sub : args) {
			if (!(sub == *it++))
				return false;
		}
		return true;
	}

	class source_file::source_view {
		diagnostics::bucket_item* item_;
		fs::file file_;
		std::vector<std::byte> content_;
		std::vector<std::size_t> endlines_;
		bool eof_{ false };
	public:
		source_view(diagnostics::bucket_item* item) : item_{ item } {}
		void open() { file_.open(item_->path); }

		std::string_view name() const noexcept {
			return item_->path;
		}

		unsigned key() const noexcept {
			return item_->key;
		}

		void ensure(size_t size) {
			std::byte buffer[8192];
			while (!eof_ && content_.size() < size) {
				auto read = file_.load(buffer, sizeof(buffer));
				if (read)
					content_.insert(content_.end(), buffer, buffer + read);

				if (file_.feof())
					eof_ = true;
			}
		}

		const std::vector<std::byte>& data() noexcept {
			std::byte buffer[8192];
			while (!eof_) {
				auto read = file_.load(buffer, sizeof(buffer));
				if (read)
					content_.insert(content_.end(), buffer, buffer + read);

				if (file_.feof())
					eof_ = true;
			}

			return content_;
		}

		void data(std::vector<std::byte>&& bytes) noexcept {
			eof_ = true;
			file_.close();
			content_ = std::move(bytes);
			endlines_.clear();
		}

		std::string_view line(unsigned no) noexcept {
			constexpr auto endline = std::byte{ '\n' };

			if (!no)
				return {};
			--no;

			while (no >= endlines_.size()) {
				auto current = endlines_.empty() ? 0u : (endlines_.back() + 1);
				auto data = content_.data() + current;
				auto end = content_.data() + content_.size();
				auto it = std::find(data, end, endline);

				if (it != end) {
					endlines_.push_back(it - content_.data());
					continue;
				}

				if (eof_) {
					endlines_.push_back(it - content_.data());
					break;
				}

				ensure(content_.size() + 1024);
			}

			if (no < endlines_.size()) {
				auto data = reinterpret_cast<const char*>(content_.data());
				if (!no)
					return { data, endlines_[0] };
				const auto prev = endlines_[no - 1];
				return { data + prev + 1, endlines_[no] - prev - 1 };
			}

			return {};
		}

		bool valid() const noexcept { return file_ || eof_; }

		bool eof(size_t at) const noexcept {
			return eof_ && (at >= content_.size());
		}

		std::byte peek(size_t at) noexcept {
			ensure(at + 1);
			if (at >= content_.size())
				return std::byte{ 0 };
			return content_[at];
		}

		size_t read(size_t at, void* buffer, size_t length) {
			ensure(at + length);
			auto rest = content_.size();
			if (rest > at)
				rest -= at;
			else
				rest = 0;

			if (rest > length)
				rest = length;

			if (rest)
				std::memcpy(buffer, content_.data() + at, rest);
			return rest;
		}

		std::vector<std::byte> read(size_t at) {
			{
				size_t buff = at;
				while (!eof_) {
					buff += 8192;
					ensure(buff);
				}
			}
			if (at >= content_.size())
				return {};
			return { next(begin(content_), at), end(content_) };
		}
	};

	source_file::~source_file() = default;

	bool source_file::valid() const noexcept {
		return view_ && view_->valid();
	}

	std::size_t source_file::read(void* buffer, std::size_t length) noexcept {
		auto read = view_->read(position_, buffer, length);
		position_ += read;
		return read;
	}

	std::vector<std::byte> source_file::read() {
		auto ret = view_->read(position_);
		position_ += ret.size();
		return ret;
	}

	bool source_file::eof() const noexcept {
		return view_->eof(position_);
	}

	std::byte source_file::peek() noexcept {
		return view_->peek(position_);
	}

	const std::vector<std::byte>& source_file::data() const noexcept {
		return view_->data();
	}

	std::string_view source_file::line(unsigned no) noexcept {
		return view_->line(no);
	}

	std::string_view source_file::name() noexcept {
		return view_->name();
	}

	location source_file::position(unsigned line, unsigned column) {
		return { view_->key(), line, column };
	}

	void diagnostic::print(outstream& o, const class diagnostics& host, const strings& tr, link_type links, size_t depth) const {
		for (size_t i = 0; i < depth; ++i) {
			static constexpr const char pre[] = "    ";
			o.write(pre, sizeof(pre) - 1);
		}

		if (sev != severity::verbose) {
			auto file = host.filename(pos);
			if (!file.empty()) {
				o.write(file.data(), file.length());
				if (links == link_type::gcc) {
					if (pos.line) {
						o.print(":{}", pos.line);
						if (pos.column) {
							o.print(":{}", pos.column);
						}
					}
				} else {
					if (pos.line) {
						o.print("({}", pos.line);
						if (pos.column) {
							o.print(",{}", pos.column);
						}
						o.print(")");
					}
				}
				o.print(": ");
			}

			const auto id = [](severity sev) {
				switch (sev) {
				case severity::warning: return lngs::lng::SEVERITY_WARNING;
				case severity::error:   return lngs::lng::SEVERITY_ERROR;
				case severity::fatal:   return lngs::lng::SEVERITY_FATAL;
				default: break;
				}
				return lngs::lng::SEVERITY_NOTE;
			}(sev);

			o.print("{}: ", oem_cp(tr.get(id)));
		}
		message.print(o, tr);
		o.write('\n');

		auto file = host.filename(pos);
		if (!file.empty() && pos.line) {
			auto src = host.source(file);
			if (src.valid()) {
				auto [line, start_col, end_col] = prepare(src.line(pos.line), pos.column, end_pos.column);
				o.write(line);
				o.write('\n');
				if (pos.column) {
					for (unsigned i = 0; i < start_col; ++i)
						o.write(' ');
					o.write('^');
					for (unsigned i = start_col + 1; i < end_col; ++i)
						o.write('~');
					o.write('\n');
				}
			}
		}

		for (auto const& child : children)
			child.print(o, host, tr, links, depth + 1);
	}

	template <std::size_t tab> constexpr std::size_t tabbed(std::size_t in) {
		return ((in + tab) / tab) * tab;
	}

	template <std::size_t tab> constexpr std::size_t tabbed_len(std::string_view line, std::size_t col) {
		std::size_t tabbed_col = 0;

		for (auto b = begin(line), e = b + std::min(col, line.length()); b != e; ++b) {
			if (*b == '\t')
				tabbed_col = tabbed<tab>(tabbed_col);
			else
				++tabbed_col;
		}

		return tabbed_col;
	}

	std::tuple<std::string, std::size_t, std::size_t> diagnostic::prepare(std::string_view line, std::size_t start_col, std::size_t end_col) {
		constexpr std::size_t tab_depth = 3;
		auto dec = [](auto val) { return val ? val - 1 : val; };
		std::size_t tabbed_col = dec(tabbed_len<tab_depth>(line, start_col));
		std::size_t tabbed_end = dec(tabbed_len<tab_depth>(line, end_col));
		std::size_t tabbed_length = tabbed_len<tab_depth>(line, line.length());

		std::string s;
		s.reserve(tabbed_length);
		for (auto c : line) {
			if (c == '\t') {
				tabbed_length = tabbed<tab_depth>(s.length());
				while (s.length() < tabbed_length)
					s.push_back(' ');
			} else
				s.push_back(c);
		}

		return { s, tabbed_col, tabbed_end };
	}

	template <typename Key>
	diagnostics::bucket_item* diagnostics::lookup(const Key& path) {
		auto key = std::hash<Key>{}(path);
		auto it = files_.lower_bound(key);
		if (it == end(files_) || it->first != key) {
			it = files_.insert(it, { key, bucket{} });
		}

		for (auto& item : it->second) {
			if (item.path == path)
				return &item;
		}

		auto current = ++current_value_;
		it->second.push_back({ std::string{ path }, {}, current });
		reverse_[current] = key;

		return &it->second.back();
	}

	template <typename Key>
	const diagnostics::bucket_item* diagnostics::lookup(const Key& path) const {
		auto key = std::hash<Key>{}(path);
		auto it = files_.lower_bound(key);
		if (it == end(files_) || it->first != key)
			return nullptr;

		for (auto& item : it->second) {
			if (item.path == path)
				return &item;
		}

		return nullptr;
	}

	std::string_view diagnostics::filename(const location& loc) const {
		auto it_hash = reverse_.find(loc.token);
		if (it_hash == end(reverse_))
			return {};
		auto it = files_.lower_bound(it_hash->second);
		if (it == end(files_))
			return {};

		for (const auto& item : it->second) {
			if (item.key == loc.token)
				return item.path;
		}

		return {};
	}

	source_file diagnostics::open(const std::string& path) {
		auto item = lookup(path);
		if (!item->contents) {
			(item->contents = std::make_shared<source_file::source_view>(item))->open();
		}
		return item->contents;
	}

	source_file diagnostics::source(const std::string_view& path) {
		auto item = lookup(path);
		if (!item->contents)
			item->contents = std::make_shared<source_file::source_view>(item);

		return item->contents;
	}

	source_file diagnostics::source(const std::string_view& path) const {
		auto item = lookup(path);
		return item ? item->contents : source_file{};
	}

	void diagnostics::set_contents(const std::string& path, std::vector<std::byte> data) {
		auto item = lookup(path);
		if (item) {
			(item->contents = std::make_shared<source_file::source_view>(item))->data(std::move(data));
		}
	}

	void diagnostics::set_contents(const std::string& path, std::string_view data) {
		auto item = lookup(path);
		if (item) {
			std::vector<std::byte> bytes(data.length());
			memcpy(bytes.data(), data.data(), data.length());
			(item->contents = std::make_shared<source_file::source_view>(item))->data(std::move(bytes));
		}
	}

	void diagnostics::push_back(diagnostic diag) {
		set_.push_back(std::move(diag));
	}

	bool diagnostics::has_errors() const noexcept {
		for (auto const& diag : set_) {
			if (diag.sev > severity::warning)
				return true;
		}
		return false;
	}

	printer_anchor::~printer_anchor() {
		for (const auto& diagnostic : diag.diagnostic_set())
			diagnostic.print(out, diag, tr);
	}
}
