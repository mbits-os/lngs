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

#include <assert.h>
#include <cctype>

#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>

namespace lngs::app {
	constexpr std::byte operator""_b(char c) { return (std::byte) c; }

	enum tok_t {
		NONE,
		SQBRAKET_O = '[',
		SQBRAKET_C = ']',
		CBRAKET_O = '{',
		CBRAKET_C = '}',
		BRAKET_O = '(',
		BRAKET_C = ')',
		EQ_SIGN = '=',
		SEMI = ';',
		COMMA = ',',
		END_OF_FILE,
		STRING,
		NUMBER,
		ID,
	};

	struct token {
		tok_t type;
		std::string value;
		location start_pos;
		location end_pos;
		int file_offset;

		template <typename ... Args>
		[[nodiscard]] diagnostic error(std::string msg, Args&& ... args) const noexcept
		{
			return message(severity::error, std::move(msg), std::forward<Args>(args)...);
		}
		template <typename ... Args>
		[[nodiscard]] diagnostic warn(std::string msg, Args&& ... args) const noexcept
		{
			return message(severity::warning, std::move(msg), std::forward<Args>(args)...);
		}
		template <typename ... Args>
		[[nodiscard]] diagnostic message(severity sev, std::string msg, Args&& ... args) const noexcept
		{
			return (start_pos/end_pos)[sev] << arg(std::move(msg), std::forward<Args>(args)...);
		}

		template <typename ... Args>
		[[nodiscard]] diagnostic error(lng msg, Args&& ... args) const noexcept
		{
			return message(severity::error, msg, std::forward<Args>(args)...);
		}
		template <typename ... Args>
		[[nodiscard]] diagnostic warn(lng msg, Args&& ... args) const noexcept
		{
			return message(severity::warning, msg, std::forward<Args>(args)...);
		}
		template <typename ... Args>
		[[nodiscard]] diagnostic message(severity sev, lng msg, Args&& ... args) const noexcept
		{
			return (start_pos/end_pos)[sev] << arg(msg, std::forward<Args>(args)...);
		}

		argumented_string info_expected() const
		{
			switch (type) {
			case STRING: return lng::ERR_EXPECTED_STRING;
			case NUMBER: return lng::ERR_EXPECTED_NUMBER;
			case ID: return value.empty() ? arg(lng::ERR_EXPECTED_ID) : "`" + value + "'";
			default: break;
			}
			assert(type != NONE && type != END_OF_FILE);
			char buffer[] = "` '";
			buffer[1] = (char)type;
			return arg(buffer);
		}

		argumented_string info_got() const
		{
			switch (type) {
			case NONE: return lng::ERR_EXPECTED_GOT_UNRECOGNIZED;
			case END_OF_FILE: return lng::ERR_EXPECTED_GOT_EOF;
			case STRING: return lng::ERR_EXPECTED_GOT_STRING;
			case NUMBER: return lng::ERR_EXPECTED_GOT_NUMBER;
			case ID: return value.empty() ? arg(lng::ERR_EXPECTED_GOT_ID) : "`" + value + "'";
			default: break;
			}
			char buffer[] = "` '";
			buffer[1] = (char)type;
			return arg(buffer);
		}
	};

	class tokenizer {
		source_file& in_;
		unsigned line_ = 1;
		unsigned column_ = 1;
		int file_offset_ = 0;

		bool peeked_ = false;
		bool eof_ = false;
		location start_pos_;
		token next_;

		char nextc() {
			char c;
			auto size = in_.read(&c, 1);
			if (!size) {
				eof_ = true;
				return 0;
			}
			++file_offset_;
			++column_;
			return c;
		}
		void set_next(int offset, tok_t tok);
		void set_next(int offset, const std::string& value, tok_t tok);
		void ws_comments();
		void read();
	public:
		tokenizer(source_file& in)
			: in_(in)
		{}

		tokenizer(const tokenizer&) = delete;
		tokenizer& operator=(const tokenizer&) = delete;

		location position() const {
			return in_.position(line_, column_);
		}

		const token& peek();
		token get();

		bool expect(tok_t type, bool fatal, diagnostics& diag);
		bool expect(tok_t first, tok_t second, diagnostics& diag);
		bool expect(tok_t first, tok_t second, tok_t third, diagnostics& diag);
		bool expect(const std::string& tok, bool fatal, diagnostics& diag);
	};

	void tokenizer::set_next(int offset, tok_t tok)
	{
		next_.type = tok;
		next_.start_pos = start_pos_;
		next_.end_pos = position();
		next_.file_offset = offset;
	}

	void tokenizer::set_next(int offset, const std::string& value, tok_t tok)
	{
		next_.type = tok;
		next_.value = value;
		next_.start_pos = start_pos_;
		next_.end_pos = position();
		next_.file_offset = offset;
	}

	void tokenizer::ws_comments()
	{
		bool found_comment = false;

		do {
			found_comment = false;
			while (!eof_ && std::isspace((uint8_t)in_.peek())) {
				auto c = nextc();
				if (c == '\n') {
					++line_;
					column_ = 1;
				}
			}

			if (eof_ || in_.peek() != '/'_b)
				break;
			nextc();

			if (!eof_ && in_.peek() == '/'_b) {
				while (!eof_ && in_.peek() != '\n'_b)
					nextc();

				found_comment = true;
			}

		} while (found_comment);

		start_pos_ = position();
	}

	void tokenizer::read()
	{
		ws_comments();

		auto offset = file_offset_;
		auto c = nextc();
		if (eof_)
			return set_next(offset, END_OF_FILE);

		switch (c) {
		case '[': return set_next(offset, SQBRAKET_O);
		case ']': return set_next(offset, SQBRAKET_C);
		case '{': return set_next(offset, CBRAKET_O);
		case '}': return set_next(offset, CBRAKET_C);
		case '(': return set_next(offset, BRAKET_O);
		case ')': return set_next(offset, BRAKET_C);
		case '=': return set_next(offset, EQ_SIGN);
		case ';': return set_next(offset, SEMI);
		case ',': return set_next(offset, COMMA);
		case '"': {
			bool escaping = false;
			bool instring = true;
			std::string s;
			while (!eof_ && instring) {
				c = nextc();
				if (eof_)
					break;

				if (escaping) {
					switch (c) {
					case 'a': s.push_back('\a'); break;
					case 'b': s.push_back('\b'); break;
					case 'f': s.push_back('\f'); break;
					case 'n': s.push_back('\n'); break;
					case 'r': s.push_back('\r'); break;
					case 't': s.push_back('\t'); break;
					case 'v': s.push_back('\v'); break;
					default: s.push_back(c);
					};
					escaping = false;
				} else {
					switch (c) {
					case '\\': escaping = true; break;
					case '"': instring = false; break;
					default:
						s.push_back(c);
					}
				}
			}
			return set_next(offset, s, STRING);
		}
		default:
			if (std::isdigit((uint8_t)c) || c == '-' || c == '+') {
				if (c == '-' || c == '+') {
					if (eof_ || !std::isdigit((uint8_t)in_.peek()))
						break;
				}

				std::string s;
				s.push_back(c);

				while (!eof_ && std::isdigit((uint8_t)in_.peek())) {
					s.push_back(nextc());
				}

				return set_next(offset, s, NUMBER);
			}

			if (c == '_' || std::isalpha((uint8_t)c)) {
				std::string s;
				s.push_back(c);
				while (!eof_ && (std::isalnum((uint8_t)in_.peek()) || in_.peek() == '_'_b)) {
					s.push_back(nextc());
				}

				return set_next(offset, s, ID);
			}
		}

		set_next(offset, NONE);
	}

	const token& tokenizer::peek()
	{
		if (!peeked_)
			read();

		peeked_ = true;
		return next_;
	}

	token tokenizer::get()
	{
		if (!peeked_)
			read();

		peeked_ = false;
		return std::move(next_);
	}

	bool tokenizer::expect(tok_t type, bool fatal, diagnostics& diag)
	{
		auto& t = peek();
		if (type == t.type)
			return true;

		if (fatal) {
			token dummy;
			dummy.type = type;
			diag.push_back(t.error(lng::ERR_EXPECTED, dummy.info_expected(), t.info_got()));
		}

		return false;
	}

	bool tokenizer::expect(tok_t one, tok_t another, diagnostics& diag)
	{
		auto& t = peek();
		if (t.type == one || t.type == another)
			return true;

		token dummy;
		dummy.type = one;
		diag.push_back(t.error(lng::ERR_EXPECTED, dummy.info_expected(), t.info_got()));

		return false;
	}

	bool tokenizer::expect(tok_t first, tok_t second, tok_t third, diagnostics& diag)
	{
		if (peek().type == third)
			return true;

		return expect(first, second, diag);
	}

	bool tokenizer::expect(const std::string& tok, bool fatal, diagnostics& diag)
	{
		auto& t = peek();
		if (ID == t.type && tok == t.value)
			return true;

		if (fatal) {
			token dummy;
			dummy.type = ID;
			dummy.value = tok;
			diag.push_back(t.error(lng::ERR_EXPECTED, dummy.info_expected(), t.info_got()));
		}

		return false;
	}

#define USE_SET_ATTR 0 // no set-attributes now, untestable
	struct attr {
		virtual ~attr() {}
		virtual bool required() const = 0;
#if USE_SET_ATTR
		virtual bool needs_arg() const = 0;
#endif
		virtual bool visited() const = 0;
		virtual const token& visit_pos() const = 0;
		virtual bool is(const std::string& name) const = 0;
		virtual const std::string& name() const = 0;
	};

	struct store_attr_base : attr {
		virtual tok_t primary() const noexcept = 0;
		virtual void visit(const token& tok, const std::string& /*arg*/) = 0;
	};

#if USE_SET_ATTR
	struct set_attr_base : attr {
		virtual void visit(const token& tok) = 0;
	};
#endif

	template <typename Base>
	class attr_base : public Base {
		bool visited_ = false;
		bool required_ = true;
		std::string name_;
		token tok_;

	protected:
		template <typename Name>
		attr_base(Name&& attrname, bool required)
			: required_(required)
			, name_(std::forward<Name>(attrname))
		{
		}

		void visited(const token& tok, bool val) { visited_ = val; tok_ = tok; }
	public:
		bool required() const override { return required_; }

		bool visited() const override { return visited_; }
		const token& visit_pos() const override { return tok_; }

		bool is(const std::string& name) const override
		{
			return name == name_;
		}

		const std::string& name() const override
		{
			return name_;
		}
	};

	template <typename T>
	void store(T& dst, const std::string& src) {
		dst = src;
	}

	void store(int& dst, const std::string& src) {
		dst = atoi(src.c_str());
	}

	void store(uint32_t& dst, const std::string& src) {
		dst = atoi(src.c_str());
	}

	template <typename T>
	class store_attr : public attr_base<store_attr_base> {
		T* ptr;
	public:
		template <typename Name>
		explicit store_attr(T* dst, Name&& name, bool required) : attr_base(std::forward<Name>(name), required), ptr(dst) {}

#if USE_SET_ATTR
		bool needs_arg() const override { return true; }
#endif
		tok_t primary() const noexcept override
		{
			if constexpr (std::is_integral_v<T>)
				return NUMBER;
			else
				return STRING;
		}

		void visit(const token& tok, const std::string& arg) override
		{
			store(*ptr, arg);
			visited(tok, true);
		}
	};

#if USE_SET_ATTR
	template <typename T, typename Value>
	class set_attr : public attr_base<set_attr_base> {
		T* ptr;
	public:
		template <typename Name>
		explicit set_attr(T* dst, Name&& name, bool required) : attr_base(std::forward<Name>(name), required), ptr(dst) {}

		bool needs_arg() const override { return false; }
		void visit(const token& tok) override
		{
			*ptr = Value::value;
			visited(tok, true);
		}
	};
#endif

	class attr_def {
		std::vector<std::unique_ptr<attr>> attrs_;
	public:
		template <typename T, typename Name>
		attr_def& arg(T& dst, Name&& name, bool required = true) {
			attrs_.push_back(std::make_unique<store_attr<T>>(&dst, std::forward<Name>(name), required));
			return *this;
		}

		template <typename Value, typename T, typename Name>
		attr_def& set([[maybe_unused]] T& dst, [[maybe_unused]] Name&& name, [[maybe_unused]] bool required = true) {
#if USE_SET_ATTR
			attrs_.push_back(std::make_unique<set_attr<T, Value>>(&dst, std::forward<Name>(name), required));
#else
			static_assert(std::is_same_v<T, void>, "attr_def::set needs support from USE_SET_ATTR");
#endif
			return *this;
		}

		const std::vector<std::unique_ptr<attr>>& attrs() const { return attrs_; }
	};

	bool read_attribute(tokenizer& tok, const attr_def& attrs, diagnostics& diag)
	{
		if (!tok.expect(ID, true, diag))
			return false;
		auto name = tok.get();

		for (auto& att : attrs.attrs()) {
			if (att->name() != name.value)
				continue;

#if USE_SET_ATTR
			if (att->needs_arg())
#endif
			{
				if (!tok.expect(BRAKET_O, true, diag))
					return false;

				auto& store = ((store_attr_base&)*att);

				tok.get();

				auto& val = tok.peek();
				switch (val.type) {
				case STRING:
				case NUMBER:
					store.visit(name, val.value);
					tok.get();
					if (!tok.expect(BRAKET_C, true, diag))
						return false;
					tok.get();
					break;
				default:
					tok.expect(store.primary(), true, diag);
					return false;
				}
			}
#if USE_SET_ATTR
			else
				((set_attr_base&)*att).visit(name);
#endif

			if (tok.expect(SQBRAKET_C, false, diag))
				return true;

			if (!tok.expect(COMMA, true, diag))
				return false;

			tok.get();
			return true;
		}

		if (tok.expect(BRAKET_O, false, diag)) {
			tok.get();
			auto& val = tok.peek();
			switch (val.type) {
			case STRING:
			case NUMBER:
				tok.get();
				if (!tok.expect(BRAKET_C, true, diag))
					return false;
				tok.get();
				break;
			default:
				tok.expect(STRING, true, diag);
				return false;
			}

			if (tok.expect(SQBRAKET_C, false, diag))
				return true;

			if (!tok.expect(COMMA, true, diag))
				return false;

			tok.get();
		}
		return true;
	}

	bool read_attributes(tokenizer& tok, const attr_def& attrs, diagnostics& diag)
	{
		if (!tok.expect(SQBRAKET_O, false, diag)) {
			for (auto& att : attrs.attrs()) {
				if (att->required()) {
					diag.push_back(
						tok.peek().error(lng::ERR_REQ_ATTR_MISSING, att->name())
					);
					return false;
				}
			}
			return true; // no required attributes, no section required
		}

		tok.get();

		while (!tok.expect(SQBRAKET_C, false, diag)) {
			if (!read_attribute(tok, attrs, diag))
				return false;
		}

		auto closing = tok.get();
		for (auto& att : attrs.attrs()) {
			if (att->required() && !att->visited()) {
				diag.push_back(
					closing.error(lng::ERR_REQ_ATTR_MISSING, att->name())
				);
				return false;
			}
		}

		return true;
	}

	bool read_string(tokenizer& tok, idl_string& str, diagnostics& diag)
	{
		if (!tok.expect(CBRAKET_C, SQBRAKET_O, ID, diag)) // `}', '[' or LNG_ID
			return false;

		{
			attr_def defs;

			defs.arg(str.help, "help", false)
				.arg(str.plural, "plural", false)
				.arg(str.id, "id", false); // id - optional here, test below

			if (!read_attributes(tok, defs, diag))
				return false;

			auto here = tok.peek();

			auto help = defs.attrs()[0].get();
			auto id = defs.attrs()[2].get();

			assert(help && help->is("help"));
			assert(id && id->is("id"));

			if (!help->visited()) {
				diag.push_back(here.warn(lng::ERR_ATTR_MISSING, "help"));
			} else if (str.help.empty()) {
				diag.push_back(
					help->visit_pos().warn(lng::ERR_ATTR_EMPTY, "help")
				);
			}

			if (!id->visited()) {
				auto err = here.error(lng::ERR_REQ_ATTR_MISSING, "id");
				err.children.push_back(
					here.message(severity::note, lng::ERR_ID_MISSING_HINT)
				);
				diag.push_back(std::move(err));
				return false;
			}

			str.id_offset = id->visit_pos().file_offset;
			str.original_id = str.id;
		}

		if (!tok.expect(ID, true, diag))
			return false;
		auto name = tok.get();
		str.key = name.value;

		if (!tok.expect(EQ_SIGN, true, diag))
			return false;
		tok.get();

		if (!tok.expect(STRING, true, diag))
			return false;
		auto value = tok.get();
		str.value = value.value;

		if (!tok.expect(SEMI, true, diag))
			return false;
		tok.get();

		return true;
	}

	bool read_strings(source_file in, idl_strings& def, diagnostics& diag)
	{
		tokenizer tok{ in };

		if (!tok.expect(SQBRAKET_O, ID, diag)) { // '[' or `strings'
			return false;
		}

		{
			attr_def defs;

			defs.arg(def.project, "project", false)
				.arg(def.ns_name, "namespace", false)
				.arg(def.version, "version", false)
				.arg(def.serial, "serial");

			if (!read_attributes(tok, defs, diag))
				return false;

			auto serial = defs.attrs()[3].get();
			assert(serial && serial->is("serial"));
			def.serial_offset = serial->visit_pos().file_offset;
		}

		if (!tok.expect("strings", true, diag))
			return false;
		tok.get();

		if (!tok.expect(CBRAKET_O, true, diag))
			return false;
		tok.get();

		while (!tok.expect(CBRAKET_C, false, diag)) {
			idl_string str;
			if (!read_string(tok, str, diag))
				return false;

			def.strings.push_back(std::move(str));
		}

		tok.get();

		auto max_id = 1000;
		for (auto& str : def.strings) {
			if (str.id <= 0)
				continue;

			if (max_id < str.id)
				max_id = str.id;
		}

		for (auto& str : def.strings) {
			if (str.id > 0)
				continue;

			def.has_new = true;
			str.id = ++max_id;
		}

		return true;
	}

	bool read_strings(const std::string& progname, const fs::path& inname, idl_strings& str, bool verbose, diagnostics& diag)
	{
		auto in = inname;
		in.make_preferred();

		auto src = diag.source(progname).position();
		if (verbose)
			diag.push_back(src[severity::verbose] << inname.string());

		auto inf = diag.open(inname.string());

		if (!inf.valid()) {
			diag.push_back(src[severity::error] << arg(lng::ERR_FILE_MISSING, inname.string()));
			return false;
		}

		if (!read_strings(std::move(inf), str, diag)) {
			if (verbose)
				diag.push_back(src[severity::error] << arg(lng::ERR_NOT_STRINGS_FILE, inname.string()));
			return false;
		}

		return true;
	}
}
