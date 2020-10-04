#pragma once

#include <gtest/gtest.h>
#include <diags/diagnostic.hpp>
#include <diags/string.hpp>
#include <diags/translator.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <numeric>
#include <string_view>
#include "ostrstream.h"

namespace diags::testing {
	using lngs::app::argument;
	using lngs::app::formatable;
	using lngs::app::lng;
	using lngs::app::translator_type;

	struct lng_strings : translator_type {
		using translator_type::get;
		std::string_view get(severity) const noexcept final;
	};
	struct strings_mock : lng_strings {
		using lng_strings::get;
		std::string_view get(lng id) const noexcept final;
	};

	struct alt_strings_mock : lng_strings {
		using lng_strings::get;
		std::string_view get(lng id) const noexcept final;
	};

	[[deprecated]] inline std::string as_string(std::string_view view) {
		return {view.data(), view.length()};
	}

	struct diagnostic_str {
		union {
			const char* str;
			lng id;
		};
		bool use_string{true};
		std::vector<diagnostic_str> args;

		diagnostic_str() : str{""} {}
		template <typename... Args>
		diagnostic_str(const char* msg, Args&&... args)
		    : str{msg}, args{std::forward<Args>(args)...} {}
		template <typename... Args>
		diagnostic_str(lng id, Args&&... args)
		    : id{id}, use_string{false}, args{std::forward<Args>(args)...} {}

		argument arg() const {
			if (args.empty()) {
				if (use_string) return str;
				return id;
			}

			std::vector<noenum::string::argument> offspring;
			offspring.reserve(args.size());
			for (auto const& a : args)
				offspring.push_back(a.arg());

			if (use_string) return formatable{str, std::move(offspring)};
			return formatable{id, std::move(offspring)};
		}

		friend std::ostream& operator<<(std::ostream& o,
		                                const diagnostic_str& rhs) {
			if (rhs.use_string)
				o << '"' << rhs.str << '"';
			else
				o << "lng:" << static_cast<unsigned>(rhs.id);
			if (!rhs.args.empty()) {
				o << '(';
				bool first = true;
				for (const auto& arg : rhs.args) {
					if (first)
						first = false;
					else
						o << ", ";
					o << arg;
				}
				o << ')';
			}
			return o;
		}
	};

	template <typename... Args>
	inline diagnostic_str str(const char* msg, Args&&... args) {
		return {msg, std::forward<Args>(args)...};
	}

	template <typename... Args>
	inline diagnostic_str str(lng id, Args&&... args) {
		return {id, std::forward<Args>(args)...};
	}

	struct expected_diagnostic {
		const char* filename{""};
		unsigned line{0};
		unsigned column{0};
		severity sev{severity::verbose};
		diagnostic_str message{};
		link_type link{link_type::gcc};
		bool use_alt_tr{true};
		unsigned column_end{0};
		std::vector<expected_diagnostic> subs{};
	};

	inline std::ostream& operator<<(std::ostream& o,
	                                const expected_diagnostic& diag) {
		o << "{\"" << diag.filename << "\"," << diag.line << ',' << diag.column;
		if (diag.column_end) o << ':' << diag.column_end;
		return o << ',' << static_cast<int>(diag.sev) << "," << diag.message
		         << "," << (diag.link == link_type::gcc ? "gcc" : "vc") << ","
		         << (diag.use_alt_tr ? "alt" : "tr") << '}';
	}

	struct UnexpectedDiags {
		const std::vector<diagnostic>& diags;
		size_t skip;
		UnexpectedDiags(const std::vector<diagnostic>& diags, size_t skip)
		    : diags{diags}, skip{skip} {}

		static const char* name(lng val);
		static const char* name(severity sev);
		static void info(std::ostream& o, noenum::string::argument const& arg);
		static void info(std::ostream& o,
		                 noenum::string::formatable const& arg);

		friend std::ostream& operator<<(std::ostream& o,
		                                const UnexpectedDiags& data) {
			o << "Missing diagnostics:\n";
			for (size_t i = data.skip; i < data.diags.size(); ++i) {
				const auto& diag = data.diags[i];
				o << "  " << diag.start().line << ':' << diag.start().column
				  << '/' << diag.stop().line << ':' << diag.stop().column << '['
				  << name(diag.severity()) << "] << ";
				UnexpectedDiags::info(o, diag.message());
				o << "\n";
			}
			return o;
		}
	};

	template <typename ActualTest>
	struct TestWithDiagnostics : ActualTest {
	protected:
		void ExpectDiagsEq(const std::vector<diagnostic>& expected,
		                   const std::vector<diagnostic>& actual,
		                   unsigned src) {
			EXPECT_GE(expected.size(), actual.size())
			    << UnexpectedDiags{actual, expected.size()};
			EXPECT_GE(actual.size(), expected.size())
			    << UnexpectedDiags{expected, actual.size()};

			auto exp_it = begin(expected);
			auto act_it = begin(actual);
			auto size = std::min(expected.size(), actual.size());
			for (decltype(size) i = 0; i < size; ++i, ++exp_it, ++act_it) {
				const auto& exp_msg = *exp_it;
				const auto& act_msg = *act_it;
				if (exp_msg.start().token)
					EXPECT_EQ(exp_msg.start().token, act_msg.start().token);
				else
					EXPECT_EQ(src, act_msg.start().token);
				EXPECT_EQ(exp_msg.severity(), act_msg.severity());
				EXPECT_EQ(exp_msg.message(), act_msg.message());
				ExpectDiagsEq(exp_msg.children(), act_msg.children(), src);
			}
		}
	};
}  // namespace diags::testing

namespace lngs::app {
	void PrintTo(lng id, std::ostream* o);
}

namespace diags {
	void PrintTo(severity sev, std::ostream* o);
	namespace noenum::string {
		void PrintTo(argument const& arg, std::ostream* o);
		void PrintTo(message const& arg, std::ostream* o);
	}  // namespace noenum::string
}  // namespace diags
