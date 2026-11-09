// mcpplibs.mwrap.boost.charconv — the documentation examples of Boost.Charconv
// (libs/charconv/doc/charconv/*.adoc) as cases "doc: <page>#<section>", plus
// the overloads the definitions list (string_view input, precision,
// from_chars_erange, every base, every chars_format). The library has no
// user-type customization point. to_chars does not null-terminate, so results
// are compared as [first, ptr) instead of with strcmp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.charconv;
import mcpplibs.mwrap.boost.core;  // boost::core::string_view

namespace cc = boost::charconv;

namespace {
template <class T, class... A>
std::string to_s(T v, A... a) {
    char buf[128];
    auto r = cc::to_chars(buf, buf + sizeof buf, v, a...);
    REQUIRE(r.ec == std::errc());
    return std::string(buf, r.ptr);
}
}  // namespace

TEST_CASE("doc: basic_usage#basic_usage") {
    const char* buffer = "42";
    int v = 0;
    cc::from_chars_result r = cc::from_chars(buffer, buffer + std::strlen(buffer), v);
    CHECK(r.ec == std::errc());
    CHECK(v == 42);
    char out[64];
    cc::to_chars_result t = cc::to_chars(out, out + sizeof(out), 123456);
    CHECK(t.ec == std::errc());
    CHECK(std::string_view(out, t.ptr) == "123456");
}

TEST_CASE("doc: chars_format#definition") {
    static_assert(static_cast<unsigned>(cc::chars_format::general) ==
                  (static_cast<unsigned>(cc::chars_format::fixed) | static_cast<unsigned>(cc::chars_format::scientific)));
    CHECK(to_s(1234.5, cc::chars_format::fixed) == "1234.5");
    CHECK(to_s(1234.5, cc::chars_format::scientific) == "1.2345e+03");
    CHECK(to_s(1234.5, cc::chars_format::general) == "1234.5");
    CHECK(to_s(1.0, cc::chars_format::hex) == "1p+0");
}

TEST_CASE("doc: from_chars#integral") {
    const char* buffer = "42";
    int v = 0;
    cc::from_chars_result r = cc::from_chars(buffer, buffer + std::strlen(buffer), v);
    CHECK(r.ec == std::errc());
    CHECK(static_cast<bool>(r));
    CHECK(v == 42);
    std::string str_buffer(buffer);
    boost::core::string_view sv(str_buffer);
    int v2;
    auto r2 = cc::from_chars(sv, v2);
    CHECK(static_cast<bool>(r2));
    CHECK(v2 == v);
}

TEST_CASE("doc: from_chars#floating-point") {
    const char* buffer = "1.2345";
    double v = 0;
    auto r = cc::from_chars(buffer, buffer + std::strlen(buffer), v);
    CHECK(static_cast<bool>(r));
    CHECK(v == 1.2345);
    std::string str_buffer(buffer);
    boost::core::string_view sv(str_buffer);
    double v2;
    CHECK(static_cast<bool>(cc::from_chars(sv, v2)));
    CHECK(v == v2);
}

TEST_CASE("doc: from_chars#integral-2") {
    const char* buffer = "2a";
    unsigned v = 0;
    auto r = cc::from_chars(buffer, buffer + std::strlen(buffer), v, 16);
    CHECK(static_cast<bool>(r));
    CHECK(v == 42);
    unsigned b36 = 0;
    std::string_view z = "zz";
    CHECK(static_cast<bool>(cc::from_chars(z.data(), z.data() + z.size(), b36, 36)));
    CHECK(b36 == 35 * 36 + 35);
}

TEST_CASE("doc: from_chars#floating-point-2") {
    // Boost.Charconv 1.92 reads the hex digits as one integer (0x13a2b =
    // 80427) and applies the p-exponent and the fraction digits as powers of
    // ten: upstream's libs/charconv/test/from_chars_float.cpp
    // (simple_hex_scientific_test) asserts 80427e-14 for this input. The
    // documentation prints 8.0427e-18, and std::from_chars reads the
    // C hexfloat 0x1.3a2bp-10 (about 1.1985e-3). The test pins upstream's
    // tested behavior, so a change of it shows up here.
    const char* buffer = "1.3a2bp-10";
    double v = 0, std_value = 0;
    auto r = cc::from_chars(buffer, buffer + std::strlen(buffer), v, cc::chars_format::hex);
    CHECK(static_cast<bool>(r));
    CHECK(v == 80427e-14);
    std::from_chars(buffer, buffer + std::strlen(buffer), std_value, std::chars_format::hex);
    CHECK(std_value == doctest::Approx(1.19846e-3).epsilon(1e-4));
    const char* b2 = "1.234p-10";  // upstream's second case
    double v2 = 0;
    cc::from_chars(b2, b2 + std::strlen(b2), v2, cc::chars_format::hex);
    CHECK(v2 == 4660e-13);
}

TEST_CASE("doc: from_chars#std-errc-invalid-argument") {
    const char* buffer = "-123";
    unsigned v = 0;
    auto r = cc::from_chars(buffer, buffer + std::strlen(buffer), v);
    CHECK(r.ec == std::errc::invalid_argument);
    CHECK_FALSE(static_cast<bool>(r));
    // The documentation shows "-1.573e-3" with chars_format::fixed failing;
    // like std::from_chars it parses the fixed prefix and stops at 'e'.
    const char* b2 = "-1.573e-3";
    double d = 0;
    auto r2 = cc::from_chars(b2, b2 + std::strlen(b2), d, cc::chars_format::fixed);
    CHECK(r2.ec == std::errc());
    CHECK(*r2.ptr == 'e');
    CHECK(d == -1.573);
    const char* b3 = "e5";
    CHECK(cc::from_chars(b3, b3 + 2, d, cc::chars_format::fixed).ec == std::errc::invalid_argument);
}

TEST_CASE("doc: from_chars#std-errc-result-out-of-range") {
    const char* buffer = "1234";
    unsigned char v = 0;
    auto r = cc::from_chars(buffer, buffer + std::strlen(buffer), v);
    CHECK(r.ec == std::errc::result_out_of_range);
    CHECK(v == 0);
}

TEST_CASE("doc: from_chars#definitions") {
    // from_chars on an out-of-range double leaves the value; from_chars_erange
    // sets it to +-HUGE_VAL / 0 as strtod does.
    const char* big = "1e9999";
    double v = 1.0;
    auto r = cc::from_chars(big, big + std::strlen(big), v);
    CHECK(r.ec == std::errc::result_out_of_range);
    CHECK(v == 1.0);
    double e = 1.0;
    auto re = cc::from_chars_erange(big, big + std::strlen(big), e);
    CHECK(re.ec == std::errc::result_out_of_range);
    CHECK(std::isinf(e));
    float f = 0;
    std::string_view sv = "2.5e1";
    CHECK(static_cast<bool>(cc::from_chars(sv.data(), sv.data() + sv.size(), f, cc::chars_format::scientific)));
    CHECK(f == 25.0f);
    long long ll = 0;
    std::string_view neg = "-9223372036854775808";
    CHECK(static_cast<bool>(cc::from_chars(neg.data(), neg.data() + neg.size(), ll)));
    CHECK(ll == std::numeric_limits<long long>::min());
    auto r3 = cc::from_chars(sv.data(), sv.data() + 3, f);  // stops at the first unparsed char
    CHECK(r3.ptr == sv.data() + 3);
}

TEST_CASE("doc: to_chars#integral") {
    char buffer[64]{};
    auto r = cc::to_chars(buffer, buffer + sizeof(buffer) - 1, 42);
    CHECK(r.ec == std::errc());
    CHECK(std::strcmp(buffer, "42") == 0);
}
TEST_CASE("doc: to_chars#floating-point") {
    char buffer[64]{};
    auto r = cc::to_chars(buffer, buffer + sizeof(buffer) - 1, 1e300);
    CHECK(static_cast<bool>(r));
    CHECK(std::strcmp(buffer, "1e+300") == 0);
}
TEST_CASE("doc: to_chars#integral-2") {
    char buffer[64]{};
    auto r = cc::to_chars(buffer, buffer + sizeof(buffer) - 1, 42, 16);
    CHECK(static_cast<bool>(r));
    CHECK(std::strcmp(buffer, "2a") == 0);
    CHECK(to_s(-255, 2) == "-11111111");
}
TEST_CASE("doc: to_chars#floating-point-2") {
    char buffer_u[64]{}, buffer_v[64]{};
    double u = -1.08260383390082950e+20;
    double v = -1.08260383390082946e+20;
    cc::to_chars(buffer_u, buffer_u + sizeof(buffer_u) - 1, u, cc::chars_format::hex);
    cc::to_chars(buffer_v, buffer_v + sizeof(buffer_v) - 1, v, cc::chars_format::hex);
    CHECK(std::string(buffer_u) == "-1.779a8946bb5fap+66");
    CHECK(std::string(buffer_v) == "-1.779a8946bb5f9p+66");  // one ULP apart
}
TEST_CASE("doc: to_chars#integral-3") {
    char buffer[3]{};
    auto r = cc::to_chars(buffer, buffer + sizeof(buffer) - 1, -1234);
    CHECK(r.ec == std::errc::value_too_large);
    CHECK_FALSE(static_cast<bool>(r));
}
TEST_CASE("doc: to_chars#floating-point-3") {
    char buffer[3]{};
    auto r = cc::to_chars(buffer, buffer + sizeof(buffer) - 1, 1.2345);
    CHECK(r.ec == std::errc::value_too_large);
}
TEST_CASE("doc: to_chars#definitions") {
    CHECK(to_s(3.14159, cc::chars_format::fixed, 2) == "3.14");
    CHECK(to_s(3.14159, cc::chars_format::scientific, 3) == "3.142e+00");
    CHECK(to_s(0.1f) == "0.1");  // shortest round trip
    CHECK(to_s(std::numeric_limits<double>::infinity()) == "inf");
    CHECK(to_s(-std::numeric_limits<double>::quiet_NaN()).find("nan") != std::string::npos);
    CHECK(to_s(static_cast<unsigned long long>(-1)) == "18446744073709551615");
}

TEST_CASE("doc: limits#definitions") {
    static_assert(cc::limits<std::int32_t>::max_chars10 >= 11);
    static_assert(cc::limits<std::uint16_t>::max_chars >= 16);
    static_assert(cc::limits<double>::max_chars10 >= 24);
}
TEST_CASE("doc: limits#examples") {
    char buffer[cc::limits<std::int32_t>::max_chars10];
    auto r = cc::to_chars(buffer, buffer + sizeof(buffer), std::numeric_limits<std::int32_t>::max());
    CHECK(static_cast<bool>(r));
    CHECK(std::string_view(buffer, r.ptr) == "2147483647");
    char fb[cc::limits<float>::max_chars10];
    auto rf = cc::to_chars(fb, fb + sizeof(fb), std::numeric_limits<float>::max());
    // The documentation shows 3.40282347e+38; to_chars writes the shortest
    // representation that round-trips.
    CHECK(std::string_view(fb, rf.ptr) == "3.4028235e+38");
    char bb[cc::limits<std::uint16_t>::max_chars];
    auto rb = cc::to_chars(bb, bb + sizeof(bb), std::numeric_limits<std::uint16_t>::max(), 2);
    CHECK(std::string_view(bb, rb.ptr) == "1111111111111111");
}

TEST_CASE("round trips over many values") {
    std::mt19937_64 gen(7);
    for (int i = 0; i < 2000; ++i) {
        double d = std::bit_cast<double>(gen());
        if (!std::isfinite(d)) continue;
        char buf[64];
        auto r = cc::to_chars(buf, buf + sizeof buf, d);
        double back = 0;
        cc::from_chars(buf, r.ptr, back);
        REQUIRE(back == d);
    }
}
