// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <algorithm>
#include <ctime>
#include <iterator>

#include <lngs/internals/commands.hpp>
#include <lngs/internals/diagnostics.hpp>
#include <lngs/internals/mstch_engine.hpp>

using namespace std::literals;

namespace lngs::app::mustache {
	namespace {
		std::string escape(const std::string& in) {
			std::string out;
			out.reserve(in.length() * 11 / 10);
			for (auto c : in) {
				switch (c) {
					case '\\':
						out.append("\\\\");
						break;
					case '\a':
						out.append("\\a");
						break;
					case '\b':
						out.append("\\b");
						break;
					case '\f':
						out.append("\\f");
						break;
					case '\n':
						out.append("\\n");
						break;
					case '\r':
						out.append("\\r");
						break;
					case '\t':
						out.append("\\t");
						break;
					case '\v':
						out.append("\\v");
						break;
					case '\"':
						out.append("\\\"");
						break;
					default:
						out.push_back(c);
				}
			}
			return out;
		}

		struct context_printer {
			diags::outstream& out;
			size_t indent{};

			template <typename Auto>
			void operator()(Auto const&) {
				out.write("void"sv);
			}

			// std::nullptr_t, std::string, long long, double, bool

			void operator()(std::string const& s) {
				out.print("\"{}\"", escape(s));
			}
			void operator()(std::nullptr_t) { out.write("null"sv); }
			void operator()(long long value) { out.print("{}", value); }
			void operator()(double value) { out.print("{}", value); }
			void operator()(bool value) { out.print("{}", value); }

			void operator()(mstch::map const& map) {
				print_list(map, '{', '}');
			}

			void operator()(mstch::array const& array) {
				print_list(array, '[', ']');
			}

			template <typename Container>
			void print_list(Container const& items, char pre, char post) {
				if (items.empty()) {
					out.print("{}{}", pre, post);
					return;
				}

				if (items.size() == 1) {
					out.write(pre);
					print_item(*items.begin());
					out.write(post);
					return;
				}

				bool first{true};
				std::string prefix(indent, '\t');
				++indent;

				out.write(pre);
				for (auto const& item : items) {
					if (first)
						first = false;
					else
						out.write(',');
					out.print("\n{}\t", prefix);
					print_item(item);
				}
				out.print("\n{}{}", prefix, post);

				--indent;
			}

			void print_item(mstch::node const& node) {
				std::visit(*this, node.base());
			}

			void print_item(mstch::map::value_type const& pair) {
				auto const& [key, node] = pair;
				(*this)(key);
				out.write(": "sv);
				print_item(node);
			}
		};
	}  // namespace

	int write(mstch_env const& env,
	          std::string const& template_name,
	          std::optional<std::string> const& additional_directory,
	          std::optional<std::filesystem::path> const& debug_out) {
		str_transform const transform{
		    escape,
		    straighten,
		    escape,
		};

		if (debug_out) {
			auto outf = diags::fs::fopen(*debug_out, "w");
			if (outf) {
				diags::foutstream out{std::move(outf)};
				auto const ctx = env.expand_context({}, transform);
				context_printer prn{out};
				prn(ctx);
			}
		}

		return env.write_mstch(template_name, {}, transform,
		                       additional_directory);
	}
}  // namespace lngs::app::mustache
