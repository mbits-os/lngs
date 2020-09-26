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

#include <lngs/internals/commands.hpp>
#include <lngs/internals/streams.hpp>
#include <lngs/internals/strings.hpp>

namespace lngs::app::enums {
	namespace {
		void print_string(outstream& out,
		                  idl_string const& str,
		                  bool use_plural) {
			auto message = straighten(str.value);
			auto help = straighten(str.help);

			out.fmt("        /// {}", message);
			if (use_plural) {
				auto plural = straighten(str.plural);
				if (!plural.empty()) out.fmt(" | {}", plural);
			}
			if (!help.empty()) out.fmt(" ({})", help);
			out.fmt("\n");
			out.fmt("        {} = {},\n", str.key, str.id);
		}
	}  // namespace
	int write(outstream& out, const idl_strings& defs, bool with_resource) {
		out.fmt(R"(// THIS FILE IS AUTOGENERATED
#pragma once

#include <lngs/lngs.hpp>

// clang-format off
)");

		out.fmt("namespace {0} {{\n",
		        defs.ns_name.empty() ? defs.project : defs.ns_name);
		bool has_singular = false;
		bool has_plural = false;

		bool first = true;
		for (auto& str : defs.strings) {
			if (!str.plural.empty()) {
				has_plural = true;
				continue;
			}

			has_singular = true;

			if (first) {
				out.fmt("    enum class lng {{\n");
				first = false;
			}

			print_string(out, str, false);
		}
		if (!first) {
			out.fmt("    }}; // enum class lng\n\n");
			first = true;
		}

		for (auto& str : defs.strings) {
			if (str.plural.empty()) continue;

			if (first) {
				out.fmt("    enum class counted {{\n");
				first = false;
			}

			print_string(out, str, true);
		}
		if (!first) out.fmt("    }}; // enum class counted\n\n");

		if (!has_singular && !has_plural) {
			out.fmt(R"(    enum class faulty {{
        UNDEFINED = 0 // File did not define any strings
    }}; // enum class faulty

)");
		}

		const char* storage = "";

		if (with_resource) {
			out.fmt(R"(    struct Resource {{
        static const char* data();
        static std::size_t size();
    }};

)");
			storage = ",\n        lngs::storage::FileWithBuiltin<Resource>";
		}

		const char* string_classes[] = {"SingularStrings", "PluralOnlyStrings",
		                                "SingularStrings",
		                                "StringsWithPlurals"};
		const char* string_args[] = {"faulty", "counted", "lng",
		                             "lng, counted"};

		auto const index = 1 * has_plural + 2 * has_singular;

		out.fmt(
		    R"(    using Strings = lngs::{0}<{1}, lngs::VersionedFile<{2}{3}>>;
}} // namespace {4}
// clang-format on
)",
		    string_classes[index], string_args[index], defs.serial, storage,
		    defs.ns_name.empty() ? defs.project : defs.ns_name);

		return 0;
	}
}  // namespace lngs::app::enums
