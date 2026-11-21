// mcpplibs.mwrap.boost.convert — the official examples whose default
// converter is boost::cnv::cstream (libs/convert/example/getting_serious.cpp,
// stream.cpp) and the converter-agnostic algorithms.cpp, plus the design-note
// snippets of libs/convert/doc/design_notes.qbk.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.convert;

using std::string;
using boost::convert;
using boost::lexical_cast;
using boost::optional;

struct boost::cnv::by_default : boost::cnv::cstream {};

namespace cnv = boost::cnv;
namespace arg = boost::cnv::parameter;

namespace {
int fallback_function() { return -1; }
struct fallback_func {
    int operator()() const { return 42; }
};

// libs/convert/test/test.hpp: change, with stream operators.
struct change {
    enum value_type { no, up, dn };
    change(value_type v = no) : value_(v) {}
    bool operator==(change v) const { return value_ == v.value_; }
    value_type value() const { return value_; }
private:
    value_type value_;
};
std::istream& operator>>(std::istream& stream, change& chg) {
    std::string str;
    stream >> str;
    if (str == "up") chg = change::up;
    else if (str == "dn") chg = change::dn;
    else if (str == "no") chg = change::no;
    else stream.setstate(std::ios_base::failbit);
    return stream;
}
std::ostream& operator<<(std::ostream& stream, change const& chg) {
    return stream << (chg == change::up ? "up" : chg == change::dn ? "dn" : "no");
}
}  // namespace

TEST_CASE("example: getting_serious") {
    boost::cnv::cstream cnv;
    std::string const str = "123";
    int const fallback_value = -1;
    // example1
    CHECK(convert<int>("not an int", cnv).value_or(-1) == -1);
    CHECK(lexical_cast<int>(str) == 123);
    CHECK(convert<int>(str, cnv).value() == 123);
    optional<int> r1 = convert<int>(str, cnv);
    optional<int> r2 = convert<int>("x", cnv);
    CHECK(r1.value() == 123);
    CHECK_THROWS_AS(r2.value(), boost::bad_optional_access);
    CHECK((r1 ? r1.value() : fallback_value) == 123);
    CHECK(r2.value_or(fallback_value) == -1);
    CHECK(convert<int>("x", cnv).value_or_eval(fallback_function) == -1);
    // example4
    CHECK(convert<int>("x", cnv).value_or_eval(fallback_func()) == 42);
    CHECK(convert<int>("x", cnv, fallback_func()) == 42);
    // example7
    CHECK(convert<int>(str, cnv, fallback_value) == 123);
    CHECK(convert<int>("x", cnv, fallback_value) == -1);
    CHECK_THROWS_AS(convert<int>("x", cnv, boost::throw_on_failure), boost::bad_optional_access);
    CHECK(convert<int>(str).value_or(fallback_value) == 123);
    CHECK(convert<int>(str).value() == 123);
}

TEST_CASE("example: stream") {
    // example1
    CHECK(convert<int>("123").value() == 123);
    CHECK(convert<int>("uhm").value_or(-1) == -1);
    CHECK(convert<string>(123).value() == "123");
    // example2
    boost::cnv::cstream ccnv;
    boost::cnv::wstream wcnv;
    CHECK(convert<int>("  FF", ccnv(std::hex)(std::skipws)).value_or(0) == 255);
    CHECK(convert<int>(L"  F", wcnv(std::hex)(std::skipws)).value_or(0) == 15);
    CHECK(convert<int>("  FF", ccnv(std::dec)(std::skipws)).value_or(-5) == -5);
    CHECK(convert<int>(L"  F", wcnv(std::dec)(std::skipws)).value_or(-5) == -5);
    ccnv(std::showbase)(std::uppercase)(std::hex);
    CHECK(convert<string>(255, ccnv, "bad") == "0XFF");
    CHECK(convert<string>(15, ccnv, "bad") == "0XF");
    ccnv(arg::base = cnv::base::dec)(arg::uppercase = true)(arg::notation = cnv::notation::scientific);
    CHECK(convert<string>(1234.5, ccnv, "bad").find('E') != string::npos);
    // example6
    boost::cnv::cstream cnv1;
    boost::cnv::lexical_cast cnv2;
    change chg = change::up;
    CHECK(convert<string>(chg, cnv1, "bad") == "up");
    CHECK(convert<string, change>(change::dn, cnv1, "bad") == "dn");
    CHECK(convert<change>("up", cnv1, change::no) == change::up);
    CHECK(convert<change>("up", cnv2, change::no) == change::up);
}

TEST_CASE("example: algorithms") {
    std::array<char const*, 3> strs = {{" 5", "0XF", "not an int"}};
    // introduction
    {
        std::vector<int> ints;
        boost::cnv::cstream cnv;
        cnv(std::hex)(std::skipws);
        std::transform(strs.begin(), strs.end(), std::back_inserter(ints),
                       boost::cnv::apply<int>(std::cref(cnv)).value_or(-1));
        CHECK(ints == std::vector<int>{5, 15, -1});
    }
    // example1: lexical_cast throws on the first failure
    {
        std::vector<int> ints;
        CHECK_THROWS(std::transform(strs.begin(), strs.end(), std::back_inserter(ints),
                                    [](char const* s) { return boost::lexical_cast<int>(string(s)); }));
        CHECK(ints.empty());
    }
    // example2
    {
        std::vector<int> ints;
        std::transform(strs.begin(), strs.end(), std::back_inserter(ints),
                       boost::cnv::apply<int>(boost::cnv::lexical_cast()).value_or(-1));
        CHECK(ints == std::vector<int>{-1, -1, -1});
    }
    // example3
    {
        std::vector<int> ints;
        boost::cnv::cstream cnv;
        CHECK_THROWS_AS(std::transform(strs.begin(), strs.end(), std::back_inserter(ints),
                                       boost::cnv::apply<int>(std::cref(cnv(std::hex)(std::skipws)))),
                        boost::bad_optional_access);
        CHECK(ints == std::vector<int>{5, 15});
    }
    // example4
    {
        std::vector<int> ints;
        boost::cnv::cstream cnv;
        std::transform(strs.begin(), strs.end(), std::back_inserter(ints),
                       boost::cnv::apply<int>(std::cref(cnv(std::hex)(std::skipws))).value_or(-1));
        CHECK(ints == std::vector<int>{5, 15, -1});
    }
    // example5
    {
        std::array<int, 3> ints = {{15, 16, 17}};
        std::vector<std::string> out;
        boost::cnv::cstream cnv;
        cnv(std::hex)(std::uppercase)(std::showbase);
        std::transform(ints.begin(), ints.end(), std::back_inserter(out),
                       boost::cnv::apply<string>(std::cref(cnv)));
        CHECK(out == std::vector<std::string>{"0XF", "0X10", "0X11"});
    }
}

namespace {
// design_notes.qbk#converter_signature: the strtol-based converter shape #2.
struct strtol_like {
    int base_ = 10;
    void operator()(char const* str_in, boost::optional<int>& result_out) const {
        char const* str_end = str_in + std::strlen(str_in);
        char* cnv_end = nullptr;
        long int result = std::strtol(str_in, &cnv_end, base_);
        if (INT_MIN <= result && result <= INT_MAX && cnv_end == str_end) result_out = int(result);
    }
};
}  // namespace

TEST_CASE("doc: converter_signature") {
    CHECK(convert<int>("42", strtol_like()).value() == 42);
    CHECK_FALSE(convert<int>("42x", strtol_like()).has_value());
}

TEST_CASE("doc: user-interface-signature") {
    boost::optional<int> out = boost::convert<int>("17");
    bool out_success = out ? true : false;
    int out_value = out.value_or(-1);
    CHECK(out_success);
    CHECK(out_value == 17);
    CHECK(boost::convert<int>("z", boost::cnv::cstream(), -1) == -1);  // #2 with a fallback
}

TEST_CASE("doc: requirements") {
    // R3a/R3b: throwing and non-throwing; R5c: fallback; R5a: success flag.
    CHECK_THROWS(convert<int>("z").value());
    CHECK_NOTHROW(convert<int>("z").value_or(0));
    CHECK(convert<int>("z", boost::cnv::cstream(), 7) == 7);
    CHECK_FALSE(convert<int>("z").has_value());
}
