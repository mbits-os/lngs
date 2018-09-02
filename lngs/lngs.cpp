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

#include <locale/file.hpp>
#include <lngs/argparser.hpp>

#if defined(_WIN32) && defined(_UNICODE)
using XChar = wchar_t;
#define main wmain
#else
using XChar = char;
#endif

namespace pot { int call(args::parser&); }
namespace enums { int call(args::parser&); }
namespace py { int call(args::parser&); }
namespace make { int call(args::parser&); }
namespace res { int call(args::parser&); }
namespace freeze { int call(args::parser&); }

struct command {
	const char* name;
	const char* description;
	int(*call)(args::parser&);
};

command commands[] = {
	{ "make",  "Translates MO file to LNG file.", make::call },
	{ "pot",   "Creates POT file from message file.", pot::call },
	{ "enums", "Creates header file from message file.", enums::call },
	{ "py",    "Creates Python module with string keys.", py::call },
	{ "res",   "Creates C++ file with fallback resource for the message file.", res::call },
	{ "freeze","Reads the language description file and assigns values to new strings.", freeze::call },
};

int main(int argc, XChar* argv[])
{
	args::parser base{ {}, argc > 1 ? 2 : 1, argv };
	base.usage("[-h] <command> [<args>]");

	if (base.args().empty())
		base.error("command missing");

	if (base.args().front() == "-h") {
		base.short_help();
		printf("\nknown commands:\n\n");

		std::vector<std::pair<std::string, std::string>> info;

		for (auto& cmd : commands)
			info.push_back(std::make_pair(cmd.name, cmd.description));

		base.format_list(info);
		printf(R"(
The flow for string management and creation:

1. String Manager:
   > lngs freeze + git 
2. Developer (compile existing list):
   > lngs enums
   > lngs res
   > git commit .hpp .cpp
3. Developer (add new string):
   [edit .lngs file]
   > git commit .lngs
4. Translator:
   > lngs pot
   > msgmerge (or msginit)
   > git commit .po
5. Developer (release build):
   > msgfmt
   > lngs make
)");
		return 0;
	}

	auto& name = base.args().front();
	for (auto& cmd : commands) {
		if (cmd.name != name)
			continue;

		args::parser sub{ cmd.description, argc - 1, argv + 1 };
		sub.program(base.program() + " " + sub.program());

		return cmd.call(sub);
	}

	base.error("unknown command: " + name);
}
