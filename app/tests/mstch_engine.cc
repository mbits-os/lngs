#include <gtest/gtest.h>
#include <lngs/internals/mstch_engine.hpp>

namespace lngs::app::testing {
	class strstream : public outstream {
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
		strstream out{};
		idl_strings defs{"project", "version", {}, 0, -1, false, {}};
		auto result = write_mstch(out, defs, {}, "no_such",
		                          {{"project", "name"}, {"version", "0.1.0"}});
		EXPECT_EQ(0, result);
		EXPECT_TRUE(out.str.empty());
	}
}  // namespace lngs::app::testing