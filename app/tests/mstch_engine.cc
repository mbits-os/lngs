#include <gtest/gtest.h>
#include <diags/streams.hpp>
#include <lngs/internals/mstch_engine.hpp>
#include "test_env.hh"

namespace lngs::app::testing {
	class strstream : public diags::outstream {
	public:
		std::string str;
		std::size_t write(const void* mem, std::size_t length) noexcept final {
			auto then = str.size();
			auto bytes = reinterpret_cast<char const*>(mem);
			str.insert(str.end(), bytes, bytes + length);
			return str.size() - then;
		}
	};

	TEST(mstch, context_conflict) {
		test_env<strstream> data{
		    {},
		    {"project", "version", {}, 0, -1, false, {}},
		};
		auto result = data.env().write_mstch(
		    "no_such", {{"project", "name"}, {"version", "0.1.0"}});
		EXPECT_EQ(0, result);
		EXPECT_TRUE(data.output.str.empty());
	}
}  // namespace lngs::app::testing