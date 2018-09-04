#include "diag_helper.h"

namespace lngs::testing {
	std::string_view strings_mock::get(lng id) const {
		switch (id) {
		case lng::SEVERITY_NOTE: return "note";
		case lng::SEVERITY_WARNING: return "warning";
		case lng::SEVERITY_ERROR: return "error";
		case lng::SEVERITY_FATAL: return "fatal";
		case lng::ERR_FILE_MISSING: return "could not open `{0}'";
		case lng::ERR_FILE_NOT_FOUND: return "could not open the file";
		case lng::ERR_NOT_STRINGS_FILE: return "`{0}' is not strings file";
		case lng::ERR_NO_NEW_STRINGS: return "no new strings";
		case lng::ERR_ATTR_EMPTY: return "attribute `{0}' should not be empty";
		case lng::ERR_ATTR_MISSING: return "attribute `{0}' is missing";
		case lng::ERR_REQ_ATTR_MISSING: return "required attribute `{0}' is missing";
		case lng::ERR_ID_MISSING_HINT: return "before finalizing a value, use `id(-1)'";
		case lng::ERR_EXPECTED: return "expected {0}, got {1}";
		case lng::ERR_EXPECTED_UNRECOGNIZED: return "unrecognized text";
		case lng::ERR_EXPECTED_EOF: return "EOF";
		case lng::ERR_EXPECTED_STRING: return "string";
		case lng::ERR_EXPECTED_NUMBER: return "number";
		case lng::ERR_EXPECTED_ID: return "identifier";
		case lng::ERR_EXPECTED_GOT_EOF: return "EOF";
		case lng::ERR_EXPECTED_GOT_STRING: return "string";
		case lng::ERR_EXPECTED_GOT_NUMBER: return "number";
		case lng::ERR_EXPECTED_GOT_ID: return "identifier";
		}
		return "";
	};

	std::string_view alt_strings_mock::get(lng id) const {
		switch (id) {
		case lng::SEVERITY_NOTE: return "nnn";
		case lng::SEVERITY_WARNING: return "www";
		case lng::SEVERITY_ERROR: return "eee";
		case lng::SEVERITY_FATAL: return "fff";
		case lng::ERR_FILE_MISSING: return "ERR_FILE_MISSING({0})";
		case lng::ERR_FILE_NOT_FOUND: return "ERR_FILE_NOT_FOUND";
		case lng::ERR_NOT_STRINGS_FILE: return "ERR_NOT_STRINGS_FILE({0})";
		case lng::ERR_NO_NEW_STRINGS: return "ERR_NO_NEW_STRINGS";
		case lng::ERR_ATTR_EMPTY: return "ERR_ATTR_EMPTY({0})";
		case lng::ERR_ATTR_MISSING: return "ERR_ATTR_MISSING({0})";
		case lng::ERR_REQ_ATTR_MISSING: return "ERR_REQ_ATTR_MISSING({0})";
		case lng::ERR_ID_MISSING_HINT: return "ERR_ID_MISSING_HINT";
		case lng::ERR_EXPECTED: return "ERR_EXPECTED({0}, {1})";
		case lng::ERR_EXPECTED_UNRECOGNIZED: return "ERR_EXPECTED_UNRECOGNIZED";
		case lng::ERR_EXPECTED_EOF: return "ERR_EXPECTED_EOF";
		case lng::ERR_EXPECTED_STRING: return "ERR_EXPECTED_STRING";
		case lng::ERR_EXPECTED_NUMBER: return "ERR_EXPECTED_NUMBER";
		case lng::ERR_EXPECTED_ID: return "ERR_EXPECTED_ID";
		case lng::ERR_EXPECTED_GOT_EOF: return "ERR_EXPECTED_GOT_EOF";
		case lng::ERR_EXPECTED_GOT_STRING: return "ERR_EXPECTED_GOT_STRING";
		case lng::ERR_EXPECTED_GOT_NUMBER: return "ERR_EXPECTED_GOT_NUMBER";
		case lng::ERR_EXPECTED_GOT_ID: return "ERR_EXPECTED_GOT_ID";
		}
		return "";
	};

	const char* UnexpectedDiags::name(lng val) {
		switch (val) {
#define NAME(x) case lng::x: return #x;
			NAME(SEVERITY_NOTE);
			NAME(SEVERITY_WARNING);
			NAME(SEVERITY_ERROR);
			NAME(SEVERITY_FATAL);
			NAME(ERR_FILE_MISSING);
			NAME(ERR_FILE_NOT_FOUND);
			NAME(ERR_NOT_STRINGS_FILE);
			NAME(ERR_NO_NEW_STRINGS);
			NAME(ERR_ATTR_EMPTY);
			NAME(ERR_ATTR_MISSING);
			NAME(ERR_REQ_ATTR_MISSING);
			NAME(ERR_ID_MISSING_HINT);
			NAME(ERR_EXPECTED);
			NAME(ERR_EXPECTED_UNRECOGNIZED);
			NAME(ERR_EXPECTED_EOF);
			NAME(ERR_EXPECTED_STRING);
			NAME(ERR_EXPECTED_NUMBER);
			NAME(ERR_EXPECTED_ID);
			NAME(ERR_EXPECTED_GOT_EOF);
			NAME(ERR_EXPECTED_GOT_STRING);
			NAME(ERR_EXPECTED_GOT_NUMBER);
			NAME(ERR_EXPECTED_GOT_ID);
#undef NAME
		};
		return "???";
	}

	const char* UnexpectedDiags::name(severity sev) {
		switch (sev) {
#define NAME(x) case severity::x: return "severity::" #x;
			NAME(verbose);
			NAME(note);
			NAME(warning);
			NAME(error);
			NAME(fatal);
#undef NAME
		};
		return "!!!";
	}

	void UnexpectedDiags::argumented(std::ostream& o, const argumented_string& arg) {
		if (std::holds_alternative< std::string>(arg.value))
			o << "\"" << std::get<std::string>(arg.value) << "\"";
		else
			o << "lng::" << name(std::get<lng>(arg.value));

		if (arg.args.empty())
			return;

		o << "(";
		auto first = true;
		for (auto const& sub : arg.args) {
			if (first) first = false;
			else o << ",";
			argumented(o, sub);
		}
		o << ")";
	}
}

namespace lngs {
	void PrintTo(lng id, std::ostream* o) {
		*o << testing::UnexpectedDiags::name(id);
	}

	void PrintTo(severity sev, std::ostream* o) {
		*o << testing::UnexpectedDiags::name(sev);
	}

	void PrintTo(argumented_string arg, std::ostream* o) {
		testing::UnexpectedDiags::argumented(*o, arg);
	}
}
