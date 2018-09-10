#include <lngs/argparser.hpp>
#include <lngs/streams.hpp>
#include <string_view>

struct test {
	const char* title;
	int(*callback)();
	int expected{ 0 };
};

std::vector<test> g_tests;

template <typename Test>
struct registrar {
	registrar() {
		::g_tests.push_back({ Test::get_name(), Test::run, Test::expected()}); \
	}
};

#define TEST_BASE(name, EXPECTED) \
	struct test_ ## name { \
		static const char* get_name() noexcept { return #name; } \
		static int run(); \
		static int expected() noexcept { return (EXPECTED); } \
	}; \
	registrar<test_ ## name> reg_ ## name; \
	int test_ ## name ::run()

#define TEST(name) TEST_BASE(name, 0)
#define TEST_FAIL(name) TEST_BASE(name, 1)

int main(int argc, char* argv[]) {
	if (argc == 1) {
		auto& out = lngs::get_stdout();
		for (auto const& test : g_tests) {
			out.fmt("{}:{}\n", test.expected, test.title);
		}
		return 0;
	}

	size_t test = atoi(argv[1]);
	return g_tests[test].callback();
}

template <typename ... CString, typename Mod>
int every_test_ever(Mod mod, CString ... args) {
	std::string arg_opt;
	std::string arg_req;
	bool starts_as_false{ false };
	bool starts_as_true{ true };
	std::string positional;

	char* __args[] = { ((char*)"args-help-test"), ((char*)args)..., nullptr };
	args::parser parser{ "program description", (int)std::size(__args) - 1, __args };
	parser.arg(arg_opt, "o", "opt").meta("VAR").help("a help for arg_opt").opt();
	parser.arg(arg_req, "r", "req").help("a help for arg_req");
	parser.set<std::true_type>(starts_as_false, "on", "1").help("a help for on").opt();
	parser.set<std::false_type>(starts_as_true, "off", "0").help("a help for off").opt();
	parser.arg(positional).help("a help for positional").opt();
	mod(parser);
	parser.parse();
	return 0;
}

void noop(const args::parser&) {}
void modify(args::parser& parser) {
	parser.program("another");
	if (parser.program() != "another") {
		fprintf(stderr, "Program not changed: %s\n", parser.program().c_str());
		std::exit(1);
	}
	parser.usage("[OPTIONS]");
	if (parser.usage() != "[OPTIONS]") {
		fprintf(stderr, "Usage not changed: %s\n", parser.program().c_str());
		std::exit(1);
	}
}

TEST(short_help) {
	return every_test_ever(noop, "-h");
}

TEST(long_help) {
	return every_test_ever(noop, "--help");
}

TEST(help_mod) {
	return every_test_ever(modify, "-h");
}

TEST_FAIL(no_req) {
	return every_test_ever(noop);
}

TEST_FAIL(no_req_mod) {
	return every_test_ever(modify, noop);
}

TEST_FAIL(full) {
	return every_test_ever(noop, "-oVALUE", "-r", "SEPARATE", "--req", "ANOTHER ONE", "--on", "-10", "--off", "POSITIONAL");
}

TEST_FAIL(missing_arg_short) {
	return every_test_ever(noop, "-r");
}

TEST_FAIL(missing_arg) {
	return every_test_ever(noop, "--req");
}

TEST_FAIL(unknown) {
	return every_test_ever(noop, "--flag");
}

TEST_FAIL(unknown_short) {
	return every_test_ever(noop, "-f");
}
