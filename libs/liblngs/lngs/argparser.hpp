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

#include <string>
#include <memory>
#include <vector>
#include <lngs/utf8.hpp>
#include <locale/file.hpp>

namespace args {

	class parser;

	struct action {
		virtual ~action() {}
		virtual bool required() const = 0;
		virtual void required(bool value) = 0;
		virtual bool needs_arg() const = 0;
		virtual void visit(parser&) = 0;
		virtual void visit(parser&, const std::string& /*arg*/) = 0;
		virtual bool visited() const = 0;
		virtual void meta(const std::string& s) = 0;
		virtual const std::string& meta() const = 0;
		virtual void help(const std::string& s) = 0;
		virtual const std::string& help() const = 0;
		virtual bool is(const std::string& name) const = 0;
		virtual bool is(char name) const = 0;
		virtual const std::vector<std::string>& names() const = 0;
	};

	class action_base : public action {
		bool visited_ = false;
		bool required_ = true;
		std::vector<std::string> names_;
		std::string meta_;
		std::string help_;

		static void pack(std::vector<std::string>&) {}
		template <typename Name, typename... Names>
		static void pack(std::vector<std::string>& dst, Name&& name, Names&&... names)
		{
			dst.emplace_back(std::forward<Name>(name));
			pack(dst, std::forward<Names>(names)...);
		}

	protected:
		template <typename... Names>
		action_base(Names&&... argnames)
		{
			pack(names_, std::forward<Names>(argnames)...);
		}

		void visited(bool val) { visited_ = val; }
	public:
		void required(bool value) override { required_ = value; }
		bool required() const override { return required_; }

		void visit(parser&) override { visited_ = true; }
		void visit(parser&, const std::string& /*arg*/) override { visited_ = true; }
		bool visited() const override { return visited_; }
		void meta(const std::string& s) override { meta_ = s; }
		const std::string& meta() const override { return meta_; }
		void help(const std::string& s) override { help_ = s; }
		const std::string& help() const override { return help_; }

		bool is(const std::string& name) const override
		{
			for (auto& argname : names_) {
				if (argname.length() > 1 && argname == name)
					return true;
			}

			return false;
		}

		bool is(char name) const override
		{
			for (auto& argname : names_) {
				if (argname.length() == 1 && argname[0] == name)
					return true;
			}

			return false;
		}

		const std::vector<std::string>& names() const override
		{
			return names_;
		}
	};

	template <typename T>
	class store_action : public action_base {
		T* ptr;
	public:
		template <typename... Names>
		explicit store_action(T* dst, Names&&... names) : action_base(std::forward<Names>(names)...), ptr(dst) {}

		bool needs_arg() const override { return true; }
		void visit(parser&, const std::string& arg) override
		{
			*ptr = arg;
			visited(true);
		}
	};

	template <typename T, typename Value>
	class set_value : public action_base {
		T* ptr;
	public:
		template <typename... Names>
		explicit set_value(T* dst, Names&&... names) : action_base(std::forward<Names>(names)...), ptr(dst) {}

		bool needs_arg() const override { return false; }
		void visit(parser&) override
		{
			*ptr = Value::value;
			visited(true);
		}
	};

	class action_builder {
		friend class parser;

		action* ptr;
		action_builder(action* ptr) : ptr(ptr) {}
		action_builder(const action_builder&);
	public:
		action_builder& meta(const std::string& name) {
			ptr->meta(name);
			return *this;
		}
		action_builder& help(const std::string& dscr) {
			ptr->help(dscr);
			return *this;
		}
		action_builder& req(bool value = true) {
			ptr->required(value);
			return *this;
		}
		action_builder& opt(bool value = true) {
			ptr->required(!value);
			return *this;
		}
	};

	class parser {
		std::vector<std::unique_ptr<action>> actions_;
		std::string description_;
		std::vector<std::string> args_;
		std::string prog_;
		std::string usage_;

		template <typename C>
		std::string program_name(const C* arg0) {
			fs::path name{ arg0 };
#if defined WIN32 || defined _WIN32
			return name.stem().string();
#else
			return name.filename().string();
#endif
		}
		const char* cstr(const char* ptr) { return ptr; }
		std::string cstr(const char32_t* ptr) { return utf::as_u8(ptr); }

		void parse_long(const std::string& name, size_t& i);
		void parse_short(const std::string& name, size_t& i);

	public:
		template <typename C>
		parser(const std::string& description, int argc, C* argv[]) : description_(description), prog_{ program_name(argv[0]) }
		{
			args_.reserve(argc - 1);
			for (int i = 1; i < argc; ++i)
				args_.emplace_back(cstr(argv[i]));
		}

		template <typename T, typename... Names>
		action_builder arg(T& dst, Names&&... names) {
			actions_.push_back(std::make_unique<store_action<T>>(&dst, std::forward<Names>(names)...));
			return action_builder(actions_.back().get());
		}

		template <typename Value, typename T, typename... Names>
		action_builder set(T& dst, Names&&... names) {
			actions_.push_back(std::make_unique<set_value<T, Value>>(&dst, std::forward<Names>(names)...));
			return action_builder(actions_.back().get());
		}

		void program(const std::string& value);
		const std::string& program();

		void usage(const std::string& value);
		const std::string& usage();

		const std::vector<std::string>& args() const { return args_; }

		void parse();

		void short_help(FILE* out = stdout, bool for_error = false);
		[[noreturn]] void help();
		void format_list(const std::vector<std::pair<std::string, std::string>>& info);
		[[noreturn]] void error(const std::string& msg);
	};
}
