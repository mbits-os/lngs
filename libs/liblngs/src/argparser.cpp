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

#include <lngs/argparser.hpp>
#include <locale/version.hpp>

#include <cstdlib>

void args::parser::short_help(FILE* out, bool for_error)
{
	fprintf(out, "Locale File Processor %s", locale::version::full);
	if (locale::version::has_commit)
		fprintf(out, " (%s)", locale::version::commit);
	if (auto const current = locale::get_version();
		current.full != locale::version::full ||
		current.commit != locale::version::commit)
	{
		fprintf(out, "\nUsing liblocale/%.*s", (int)current.full.length(), current.full.data());
		if (!current.commit.empty())
			fprintf(out, " (%.*s)", (int)current.commit.length(), current.commit.data());
	}
	if (!for_error)
		fprintf(out, "\n");
	fprintf(out, "\nusage: %s", prog_.c_str());

	if (!usage_.empty()) {
		fprintf(out, " %s\n", usage_.c_str());
		return;
	}

	fprintf(out, " [-h]");
	for (auto& action : actions_) {
		if (action->names().empty())
			continue;

		fprintf(out, " ");
		if (!action->required())
			fprintf(out, "[");
		auto& name = action->names().front();
		if (name.length() == 1)
			fprintf(out, "-%s", name.c_str());
		else
			fprintf(out, "--%s", name.c_str());
		if (action->needs_arg()) {
			if (action->meta().empty())
				fprintf(out, " ARG");
			else {
				fprintf(out, " %s", action->meta().c_str());
			}
		}
		if (!action->required())
			fprintf(out, "]");
	}

	fprintf(out, "\n");
}

void args::parser::help()
{
	short_help();

	if (!description_.empty())
		printf("\n%s\n", description_.c_str());

	printf("\narguments:\n\n");
	std::vector<std::pair<std::string, std::string>> info;
	info.reserve(1 + actions_.size());
	info.push_back(std::make_pair("-h, --help", "show this help message and exit"));
	for (auto& action : actions_) {
		std::string names;
		bool first = true;
		for (auto& name : action->names()) {
			if (first) first = false;
			else names.append(", ");

			if (name.length() == 1)
				names.append("-");
			else
				names.append("--");
			names.append(name);
		}
		if (names.empty())
			continue;

		if (action->needs_arg()) {
			std::string arg;
			if (action->meta().empty())
				arg = " ARG";
			else
				arg = " " + action->meta();
			names.append(arg);
		}

		info.push_back(std::make_pair(names, action->help()));
	}

	format_list(info);

	std::exit(0);
}

void args::parser::format_list(const std::vector<std::pair<std::string, std::string>>& info)
{
	size_t len = 0;
	for (auto& pair : info) {
		if (len < std::get<0>(pair).length())
			len = std::get<0>(pair).length();
	}

	for (auto& pair : info) {
		printf("%-*s %s\n", (int)len, std::get<0>(pair).c_str(), std::get<1>(pair).c_str());
	}
}

void args::parser::error(const std::string& msg)
{
	short_help(stderr, true);
	fprintf(stderr, "%s: error: %s\n", prog_.c_str(), msg.c_str());
	std::exit(-1);
}

void args::parser::program(const std::string& value)
{
	prog_ = value;
}

const std::string& args::parser::program()
{
	return prog_;
}

void args::parser::usage(const std::string& value)
{
	usage_ = value;
}

const std::string& args::parser::usage()
{
	return usage_;
}

void args::parser::parse()
{
	auto count = args_.size();
	for (decltype(count) i = 0; i < count; ++i) {
		auto& arg = args_[i];

		if (arg.length() > 1 && arg[0] == '-') {
			if (arg.length() > 2 && arg[1] == '-')
				parse_long(arg.substr(2), i);
			else
				parse_short(arg.substr(1), i);
		} else
			error("unrecognized argument: " + arg);
	}

	for (auto& action : actions_) {
		if (action->required() && !action->visited()) {
			auto& name = action->names().front();

			auto arg = name.length() == 1 ? "-" + name : "--" + name;
			error("argument " + arg + " is required");
		}
	}
}

void args::parser::parse_long(const std::string& name, size_t& i) {
	if (name == "help")
		help();

	for (auto& action : actions_) {
		if (!action->is(name))
			continue;

		if (action->needs_arg()) {
			++i;
			if (i >= args_.size())
				error("argument --" + name + ": expected one argument");

			action->visit(*this, args_[i]);
		}
		else
			action->visit(*this);

		return;
	}

	error("unrecognized argument: --" + name);
}

static inline std::string expand(char c) {
	char buff[] = { c, 0 };
	return buff;
}
void args::parser::parse_short(const std::string& name, size_t& arg)
{
	auto length = name.length();
	for (decltype(length) i = 0; i < length; ++i) {
		auto c = name[i];
		if (c == 'h')
			help();

		bool found = false;
		for (auto& action : actions_) {
			if (!action->is(c))
				continue;

			if (action->needs_arg()) {
				std::string param;

				++i;
				if (i < length)
					param = name.substr(i);
				else {
					++arg;
					if (arg >= args_.size())
						error("argument -" + expand(c) + ": expected one argument");

					param = args_[arg];
				}

				i = length;

				action->visit(*this, param);
			}
			else
				action->visit(*this);

			found = true;
			break;
		}

		if (!found)
			error("unrecognized argument: -" + expand(c));
	}
}
