// mcpplibs.mwrap.boost.convert — the official examples whose default
// converter is boost::cnv::lexical_cast (libs/convert/example/
// getting_started.cpp, lexical_cast.cpp). A program defines
// boost::cnv::by_default once, so the examples are grouped by the default
// they define: examples_stream.cpp and examples_default_converter.cpp hold
// the others. The spirit converter of getting_started is include_spirit.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.convert;

using std::string;
using boost::convert;
using boost::lexical_cast;

struct boost::cnv::by_default : boost::cnv::lexical_cast {};

namespace {
void log(char const*) {}
int fallback_fun(char const* msg, int fallback_value) {
    log(msg);
    return fallback_value;
}
}  // namespace

TEST_CASE("example: getting_started") {
    // example1
    auto cnv = boost::cnv::lexical_cast();
    CHECK(lexical_cast<int>("123") == 123);
    CHECK(convert<int>("123").value() == 123);
    CHECK(convert<int>("123", cnv).value() == 123);
    CHECK(lexical_cast<string>(123) == "123");
    CHECK(convert<string>(123).value() == "123");
    CHECK(convert<string>(123, cnv).value() == "123");
    // example2
    CHECK(convert<int>("uhm", boost::cnv::lexical_cast()).value_or(-1) == -1);
    // example3 (lexical_cast and strtol; spirit is include_spirit.cpp)
    CHECK(convert<int>("123", boost::cnv::lexical_cast()).value() == 123);
    CHECK(convert<int>("123", boost::cnv::strtol()).value() == 123);
    // example4
    CHECK_THROWS(lexical_cast<int>("   123"));
    auto ccnv = boost::cnv::cstream();
    CHECK(convert<int>("   123", ccnv(std::skipws)).value() == 123);
    CHECK(lexical_cast<string>(12.34567) == "12.34567");
    CHECK(convert<string>(12.34567, ccnv(std::fixed)(std::setprecision(3))).value() == "12.346");
    CHECK(convert<string>(12.34567, ccnv(std::scientific)(std::setprecision(3))).value() == "1.235e+01");
    // example5
    boost::cnv::cstream cs;
    CHECK(convert<int>("123", cs).value() == 123);
    CHECK(convert<int>("uhm", cs).value_or(-1) == -1);
    // example6 / example7
    auto hex = boost::cnv::cstream();
    CHECK(convert<int>(std::string("123"), hex(std::hex)).value_or(-1) == 291);
    CHECK(convert<int>(std::string("456"), hex(std::dec)).value_or(-1) == 456);
    // example8
    int i1 = 12;
    try {
        i1 = lexical_cast<int>(std::string("123"));
    } catch (...) {
        log("bad i1");
    }
    CHECK(i1 == 123);
    // example9
    std::string const s1 = "123", s2 = "nan";
    CHECK(convert<int>(s1).value_or_eval(std::bind(fallback_fun, "bad i1", 11)) == 123);
    CHECK(convert<int>(s2).value_or_eval(std::bind(fallback_fun, "bad i2", 12)) == 12);
}

TEST_CASE("example: lexical_cast") {
    CHECK(lexical_cast<int>("123") == 123);
    CHECK(convert<int>("123").value() == 123);
    CHECK(convert<int>("uhm").value_or(-1) == -1);
    CHECK(lexical_cast<string>(123) == "123");
    CHECK(convert<string>(123).value() == "123");
    CHECK_THROWS_AS(convert<int>("uhm").value(), boost::bad_optional_access);
}

TEST_CASE("doc: introduction") {
    // The doc's opening example: a lexical_cast-based default converter.
    boost::optional<int> r = convert<int>("12");
    REQUIRE(r);
    CHECK(*r == 12);
    CHECK(convert<double>("1.5").value() == 1.5);
}
