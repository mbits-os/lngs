// Copyright (c) 2020 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/gettext.hpp>
#include <string_view>

namespace gtt {
	using namespace lngs::app;

	enum class cmd_type {
		unknown,
		msgctx,
		msgid,
		msgstr,
		msgid_plural,
	};

	bool is_space(unsigned char c) {
		switch (c) {
			case ' ':
			case '\t':
			case '#':
				return true;
			default:
				return false;
		}
	}

	struct parser_ctx {
		diags::source_code& src;
		diags::sources& diags;
		std::map<std::string, std::string>& result;

		unsigned lineno = 0;
		std::string_view line{};
		unsigned char const* cur{};
		unsigned char const* end{};

		struct cmd_info {
			cmd_type type = cmd_type::unknown;
			unsigned counter{};
			std::string contents;

			bool operator<(cmd_info const& rhs) const noexcept {
				if (type != rhs.type) return type < rhs.type;
				return counter < rhs.counter;
			}
		};

		using record_info = std::vector<cmd_info>;

		cmd_info curr_cmd{};
		record_info curr_rec{};

		bool next_line() noexcept {
			if (src.eof()) return false;
			line = src.line(++lineno);
			cur = reinterpret_cast<unsigned char const*>(line.data());
			end = cur + line.size();

			return true;
		}

		diags::location pos() const noexcept {
			auto begin = reinterpret_cast<unsigned char const*>(line.data());
			return src.position(lineno, static_cast<unsigned>(cur - begin));
		}

		void line_changed(cmd_type newtype = cmd_type::unknown) {
			cmd_info prev{};
			prev.type = newtype;

			std::swap(prev, curr_cmd);

			if (prev.type == cmd_type::unknown) return;

			curr_rec.push_back(std::move(prev));

			if (prev.type == cmd_type::msgstr &&
			    curr_cmd.type != cmd_type::msgstr) {
				if (curr_rec.empty()) return;

				record_info rec{};
				std::swap(curr_rec, rec);

				std::stable_sort(rec.begin(), rec.end());
				if (rec.front().type != cmd_type::msgstr) {
					auto ctxid = rec.size();
					auto idid = rec.size();
					std::size_t string_length = 0;
					std::size_t ndx = 0;
					for (auto const& cmd : rec) {
						switch (cmd.type) {
							case cmd_type::msgctx:
								ctxid = ndx;
								break;
							case cmd_type::msgid:
								idid = ndx;
								break;
							case cmd_type::msgstr:
								string_length += cmd.contents.size() + 1;
								break;
							default:
								break;
						}
						++ndx;
					}

					if (ctxid == rec.size()) ctxid = idid;
					if (ctxid == rec.size()) return;
					auto key = std::move(rec[ctxid].contents);

					if (string_length) --string_length;

					std::string message(string_length, '\0');
					auto ptr = message.data();
					for (auto const& cmd : rec) {
						if (cmd.type != cmd_type::msgstr) continue;
						std::memcpy(ptr, cmd.contents.data(),
						            cmd.contents.size());
						ptr += cmd.contents.size() + 1;
					}

					result[std::move(key)] = std::move(message);
				}
			}
		}

		void line_changed(std::string_view command) {
			auto next_type = cmd_type::unknown;
			if (command == "msgctxt")
				next_type = cmd_type::msgctx;
			else if (command == "msgid")
				next_type = cmd_type::msgid;
			else if (command == "msgid_plural")
				next_type = cmd_type::msgid_plural;
			else if (command == "msgstr")
				next_type = cmd_type::msgstr;

			if (next_type == cmd_type::unknown) {
				push_back(pos()[diags::severity::warning]
				          << format(lng::ERR_GETTEXT_UNRECOGNIZED_FIELD,
				                    std::string{command}));
			}
			line_changed(next_type);
		}

		void push_back(diags::diagnostic const& diag) { diags.push_back(diag); }

		bool eol() const noexcept { return cur == end; }

		unsigned char next() noexcept {
			if (eol()) return 0xFF;
			return *cur++;
		}
		unsigned char peek() const noexcept {
			if (eol()) return 0xFF;
			return *cur;
		}

		void skip_ws() {
			while (!eol()) {
				auto const c = peek();
				switch (c) {
					case ' ':
					case '\t':
						++cur;
						break;
					case '#':
						cur = end;
						return;
					default:
						return;
				}
			}
		}

		bool find_command() {
			using namespace lngs::app;

			auto it = cur;
			while (!eol()) {
				auto const c = peek();
				if (is_space(c)) break;
				if (c == '[') break;
				++cur;
			}
			line_changed({reinterpret_cast<char const*>(it),
			              static_cast<size_t>(cur - it)});

			curr_cmd.counter = 0;
			if (peek() == '[') {
				next();
				if (peek() == ']') {
					push_back(pos()[diags::severity::error]
					          << format(lng::ERR_EXPECTED,
					                    lng::ERR_EXPECTED_NUMBER, "`]'"));
				}
				while (!eol() && peek() != ']') {
					auto const c = next();
					if (!std::isdigit(c)) {
						push_back(pos()[diags::severity::error] << format(
						              lng::ERR_EXPECTED,
						              lng::ERR_EXPECTED_NUMBER,
						              lng::ERR_EXPECTED_GOT_UNRECOGNIZED));
						return false;
					}
					curr_cmd.counter *= 10;
					curr_cmd.counter += static_cast<unsigned>(c) - '0';
				}
				next();
			}
			return true;
		}

		bool parse_string_chunk() {
			using namespace lngs::app;

			while (!eol()) {
				auto const c = next();
				switch (c) {
					case '"':
						return true;

					case '\\': {
						if (eol()) {
							push_back(pos()[diags::severity::error]
							          << format(lng::ERR_EXPECTED,
							                    lng::ERR_EXPECTED_STRING,
							                    lng::ERR_EXPECTED_GOT_EOL));
							return false;
						}
						auto const cont = next();
						switch (cont) {
							case 'a':
								curr_cmd.contents.push_back('\a');
								break;
							case 'b':
								curr_cmd.contents.push_back('\b');
								break;
							case 'f':
								curr_cmd.contents.push_back('\f');
								break;
							case 'n':
								curr_cmd.contents.push_back('\n');
								break;
							case 'r':
								curr_cmd.contents.push_back('\r');
								break;
							case 't':
								curr_cmd.contents.push_back('\t');
								break;
							case 'v':
								curr_cmd.contents.push_back('\v');
								break;
							default:
								push_back(
								    pos()[diags::severity::warning] << format(
								        lng::ERR_GETTEXT_UNRECOGNIZED_ESCAPE,
								        std::string{static_cast<char>(cont)}));
								[[fallthrough]];
							case '\\':
							case '"':
							case '\'':
							case '?':
								curr_cmd.contents.push_back(
								    static_cast<char>(cont));
								break;
						}
						break;
					}
					default:
						curr_cmd.contents.push_back(static_cast<char>(c));
						break;
				}
			}
			push_back(pos()[diags::severity::error]
			          << format(lng::ERR_EXPECTED, "`\"'",
			                    lng::ERR_EXPECTED_GOT_EOL));
			return false;
		}

		bool parse_line() {
			skip_ws();
			if (eol()) {
				line_changed();
				return true;
			}

			if (peek() != '"') {
				find_command();

				skip_ws();

				if (peek() != '"') {
					push_back(pos()[diags::severity::error] << format(
					              lng::ERR_EXPECTED, lng::ERR_EXPECTED_STRING,
					              lng::ERR_EXPECTED_GOT_UNRECOGNIZED));
					return false;
				}
			}

			next();
			if (!parse_string_chunk()) return false;

			skip_ws();
			if (!eol()) {
				push_back(pos()[diags::severity::error]
				          << format(lng::ERR_EXPECTED, lng::ERR_EXPECTED_EOL,
				                    lng::ERR_EXPECTED_GOT_UNRECOGNIZED));
				return false;
			}

			return true;
		}

		bool parse_file() {
			while (next_line()) {
				if (!parse_line()) return false;
			}
			line_changed();
			return true;
		}
	};

	std::map<std::string, std::string> open_po(diags::source_code& src,
	                                           diags::sources& diags) {
		using namespace lngs::app;

		std::map<std::string, std::string> out;

		if (!src.valid()) {
			diags.push_back(src.position()[diags::severity::error]
			                << lng::ERR_FILE_NOT_FOUND);
			return {};
		}

		parser_ctx ctx{src, diags, out};
		if (!ctx.parse_file()) return {};
		return out;
	}
}  // namespace gtt
