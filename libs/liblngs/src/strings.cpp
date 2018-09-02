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

#include <lngs/streams.hpp>
#include <lngs/strings.hpp>

namespace locale {
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
		std::string source;
		size_t line;
		int file_offset;

		void error(const std::string& msg) const { message("error", msg); }
		void warn(const std::string& msg) const { message("warning", msg); }
		void message(const std::string& klass, const std::string& msg) const
		{
			printf("%s(%zu): %s: %s\n", source.c_str(), line, klass.c_str(), msg.c_str());
		}

		std::string info() const
		{
			switch (type) {
			case NONE: return "unrecognized text";
			case END_OF_FILE: return "EOF";
			case STRING: return "string";
			case NUMBER: return "number";
			case ID: return value.empty() ? "identifier" : "`" + value + "'";
			default: break;
			}
			char buffer[] = "` '";
			buffer[1] = (char)type;
			return buffer;
		}
	};

	class tokenizer {
		instream& in;
		std::string name;
		size_t line = 1;
		int file_offset = 0;

		bool peeked = false;
		token next;

		char nextc() {
			char c;
			in.read(&c, 1);
			++file_offset;
			return c;
		}
		void set_next(int offset, tok_t tok);
		void set_next(int offset, const std::string& value, tok_t tok);
		void ws_comments();
		void read();
	public:
		tokenizer(instream& in, const std::string& name)
			: in(in)
			, name(name)
		{}

		tokenizer(const tokenizer&) = delete;
		tokenizer& operator=(const tokenizer&) = delete;

		std::pair<std::string, size_t> position() const {
			return{ name, line };
		}

		const token& peek();
		token get();

		bool expect(tok_t type, bool fatal);
		bool expect(tok_t first, tok_t second);
		bool expect(tok_t first, tok_t second, tok_t third);
		bool expect(const std::string& tok, bool fatal);
	};

	void tokenizer::set_next(int offset, tok_t tok)
	{
		next.source = name;
		next.line = line;
		next.file_offset = offset;
		next.type = tok;
	}

	void tokenizer::set_next(int offset, const std::string& value, tok_t tok)
	{
		next.source = name;
		next.line = line;
		next.file_offset = offset;
		next.value = value;
		next.type = tok;
	}

	void tokenizer::ws_comments()
	{
		bool found_comment = false;

		do {
			found_comment = false;
			while (!in.eof() && std::isspace((uint8_t)in.peek())) {
				auto c = nextc();
				if (c == '\n')
					++line;
			}

			if (in.eof() || in.peek() != '/')
				break;
			nextc();

			if (!in.eof() && in.peek() == '/') {
				while (!in.eof() && in.peek() != '\n')
					nextc();

				found_comment = true;
			}

		} while (found_comment);
	}

	void tokenizer::read()
	{
		ws_comments();

		auto offset = file_offset;
		auto c = nextc();
		if (in.eof())
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
			while (!in.eof() && instring) {
				c = nextc();

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
				}
				else {
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
				std::string s;
				s.push_back(c);
				while (!in.eof() && std::isdigit((uint8_t)in.peek()))
					s.push_back(nextc());

				return set_next(offset, s, NUMBER);
			}

			if (c == '_' || std::isalpha((uint8_t)c)) {
				std::string s;
				s.push_back(c);
				while (!in.eof() && (std::isalnum((uint8_t)in.peek()) || in.peek() == '_'))
					s.push_back(nextc());

				return set_next(offset, s, ID);
			}
		}

		set_next(offset, NONE);
	}

	const token& tokenizer::peek()
	{
		if (!peeked)
			read();

		peeked = true;
		return next;
	}

	token tokenizer::get()
	{
		if (!peeked)
			read();

		peeked = false;
		return std::move(next);
	}

	bool tokenizer::expect(tok_t type, bool fatal)
	{
		auto& t = peek();
		if (type == t.type)
			return true;

		if (fatal) {
			token dummy;
			dummy.type = type;
			t.error("expected " + dummy.info() + ", got " + t.info());
		}

		return false;
	}

	bool tokenizer::expect(tok_t one, tok_t another)
	{
		auto& t = peek();
		if (t.type == one || t.type == another)
			return true;

		token dummy;
		dummy.type = one;
		t.error("expected " + dummy.info() + ", got " + t.info());

		return false;
	}

	bool tokenizer::expect(tok_t first, tok_t second, tok_t third)
	{
		if (peek().type == third)
			return true;

		return expect(first, second);
	}

	bool tokenizer::expect(const std::string& tok, bool fatal)
	{
		auto& t = peek();
		if (ID == t.type && tok == t.value)
			return true;

		if (fatal)
			t.error("expected `" + tok + "', got " + t.info());

		return false;
	}


	struct attr {
		virtual ~attr() {}
		virtual bool required() const = 0;
		virtual void required(bool value) = 0;
		virtual bool needs_arg() const = 0;
		virtual void visit(const token& tok) = 0;
		virtual void visit(const token& tok, const std::string& /*arg*/) = 0;
		virtual bool visited() const = 0;
		virtual const token& visit_pos() const = 0;
		virtual bool is(const std::string& name) const = 0;
		virtual const std::string& name() const = 0;
	};

	class attr_base : public attr {
		bool visited_ = false;
		bool required_ = true;
		std::string name_;
		token tok_;

	protected:
		template <typename Name>
		attr_base(Name&& attrname, bool required)
			: name_(std::forward<Name>(attrname))
			, required_(required)
		{
		}

		void visited(const token& tok, bool val) { visited_ = val; tok_ = tok; }
	public:
		void required(bool value) override { required_ = value; }
		bool required() const override { return required_; }

		void visit(const token& tok) override { visited(tok, true); }
		void visit(const token& tok, const std::string& /*arg*/) override { visited(tok, true); }
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
	class store_attr : public attr_base {
		T* ptr;
	public:
		template <typename Name>
		explicit store_attr(T* dst, Name&& name, bool required) : attr_base(std::forward<Name>(name), required), ptr(dst) {}

		bool needs_arg() const override { return true; }
		void visit(const token& tok, const std::string& arg) override
		{
			store(*ptr, arg);
			visited(tok, true);
		}
	};

	template <typename T, typename Value>
	class set_attr : public attr_base {
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

	class attr_def {
		std::vector<std::unique_ptr<attr>> attrs_;
	public:
		template <typename T, typename Name>
		attr_def& arg(T& dst, Name&& name, bool required = true) {
			attrs_.push_back(std::make_unique<store_attr<T>>(&dst, std::forward<Name>(name), required));
			return *this;
		}

		template <typename Value, typename T, typename Name>
		attr_def& set(T& dst, Name&& name, bool required = true) {
			attrs_.push_back(std::make_unique<set_attr<T, Value>>(&dst, std::forward<Name>(name), required));
			return *this;
		}

		const std::vector<std::unique_ptr<attr>>& attrs() const { return attrs_; }
	};

	bool read_attribute(tokenizer& tok, const attr_def& attrs)
	{
		if (tok.expect(SQBRAKET_C, false))
			return true;

		if (!tok.expect(ID, true))
			return false;
		auto name = tok.get();

		for (auto& att : attrs.attrs()) {
			if (att->name() != name.value)
				continue;

			if (att->needs_arg()) {
				if (!tok.expect(BRAKET_O, true))
					return false;

				tok.get();

				auto& val = tok.peek();
				switch (val.type) {
				case STRING:
				case NUMBER:
					att->visit(name, val.value);
					tok.get();
					if (!tok.expect(BRAKET_C, true))
						return false;
					tok.get();
					break;
				default:
					tok.expect(STRING, true);
					return false;
				}
			}
			else
				att->visit(name);

			if (tok.expect(SQBRAKET_C, false))
				return true;

			if (!tok.expect(COMMA, true))
				return false;

			tok.get();
			return true;
		}

		tok.get();
		if (tok.expect(BRAKET_O, false)) {
			tok.get();
			auto& val = tok.peek();
			switch (val.type) {
			case STRING:
			case NUMBER:
				tok.get();
				if (!tok.expect(BRAKET_C, true))
					return false;
				tok.get();
				break;
			default:
				tok.expect(STRING, true);
				return false;
			}

			if (tok.expect(SQBRAKET_C, false))
				return true;

			if (!tok.expect(COMMA, true))
				return false;

			tok.get();
		}
		return true;
	}

	bool read_attributes(tokenizer& tok, const attr_def& attrs)
	{
		if (!tok.expect(SQBRAKET_O, false)) {
			for (auto& att : attrs.attrs()) {
				if (att->required()) {
					tok.peek().error("required attribute `" + att->name() + "' is missing");
					return false;
				}
			}
			return true; // no required attributes, no section required
		}

		tok.get();

		while (!tok.expect(SQBRAKET_C, false)) {
			if (!read_attribute(tok, attrs))
				return false;
		}

		auto closing = tok.get();
		for (auto& att : attrs.attrs()) {
			if (att->required() && !att->visited()) {
				closing.error("required attribute `" + att->name() + "' is missing");
				return false;
			}
		}

		return true;
	}

	bool read_string(tokenizer& tok, String& str)
	{
		if (!tok.expect(CBRAKET_C, SQBRAKET_O, ID)) // `}', '[' or LNG_ID
			return false;

		{
			attr_def defs;

			defs.arg(str.help, "help", false)
				.arg(str.plural, "plural", false)
				.arg(str.id, "id", false); // id - optional here, test below

			if (!read_attributes(tok, defs))
				return false;

			auto here = tok.peek();

			auto help = defs.attrs()[0].get();
			auto id = defs.attrs()[2].get();

			assert(help && help->is("help"));
			assert(id && id->is("id"));

			if (!help->visited()) {
				here.warn("attribute `help' is missing");
			}
			else if (str.help.empty()) {
				help->visit_pos().warn("attribute `help' should not be empty");
			}

			if (!id->visited()) {
				here.error("required attribute `id' is missing; before assigning a value, use `id(-1)'");
				return false;
			}

			str.id_offset = id->visit_pos().file_offset;
			str.original_id = str.id;
		}

		if (!tok.expect(ID, true))
			return false;
		auto name = tok.get();
		str.key = name.value;

		if (!tok.expect(EQ_SIGN, true))
			return false;
		tok.get();

		if (!tok.expect(STRING, true))
			return false;
		auto value = tok.get();
		str.value = value.value;

		if (!tok.expect(SEMI, true))
			return false;
		tok.get();

		return true;
	}

	bool read_strings(instream& in, const std::string& inname, Strings& def)
	{
		tokenizer tok{ in, inname };

		if (!tok.expect(SQBRAKET_O, ID)) // '[' or `strings'
			return false;

		{
			attr_def defs;

			defs.arg(def.project, "project", false)
				.arg(def.version, "version", false)
				.arg(def.serial, "serial");

			if (!read_attributes(tok, defs))
				return false;

			auto serial = defs.attrs()[2].get();
			assert(serial && serial->is("serial"));
			def.serial_offset = serial->visit_pos().file_offset;
		}

		if (!tok.expect("strings", true))
			return false;
		tok.get();

		if (!tok.expect(CBRAKET_O, true))
			return false;
		tok.get();

		while (!tok.expect(CBRAKET_C, false)) {
			String str;
			if (!read_string(tok, str))
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

	bool read_strings(const fs::path& in, Strings& def, bool verbose)
	{
		auto inname = in;
		inname.make_preferred();

		if (verbose)
			printf("%s\n", inname.string().c_str());

		auto inf = fs::fopen(in, "rb");

		if (!inf) {
			fprintf(stderr, "could not open `%s'", inname.string().c_str());
			return false;
		}

		finstream is{ inf.handle() };
		if (!read_strings(is, inname.string(), def)) {
			if (verbose)
				fprintf(stderr, "`%s' is not strings file.\n", inname.string().c_str());
			return false;
		}

		return true;
	}
}
