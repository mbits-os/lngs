#include "diag_helper.h"

namespace lngs::testing {
	std::string_view strings_mock::get(lng id) const {
		switch (id) {
		case lng::SEVERITY_NOTE: return "note";
		case lng::SEVERITY_WARNING: return "warning";
		case lng::SEVERITY_ERROR: return "error";
		case lng::SEVERITY_FATAL: return "fatal";
		}
		return "";
	};

	std::string_view alt_strings_mock::get(lng id) const {
		switch (id) {
		case lng::SEVERITY_NOTE: return "nnn";
		case lng::SEVERITY_WARNING: return "www";
		case lng::SEVERITY_ERROR: return "eee";
		case lng::SEVERITY_FATAL: return "fff";
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
