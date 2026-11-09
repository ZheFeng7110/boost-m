// mcpplibs.mwrap.boost.convert — libs/convert/example/default_converter.cpp:
// a user-defined default converter (a cstream configured for uppercase hex).
// default_converter_fail.cpp is a compile-failure demonstration (no default
// converter defined) and is excluded in coverage.toml.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.convert;

struct boost::cnv::by_default : boost::cnv::cstream {
    by_default() { (*this)(std::uppercase)(std::hex); }
};

TEST_CASE("example: default_converter") {
    int i = boost::convert<int>("F").value_or(-1);
    std::string s = boost::convert<std::string>(255).value_or("bad");
    CHECK(i == 15);
    CHECK(s == "FF");
}
