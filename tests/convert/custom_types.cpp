// mcpplibs.mwrap.boost.convert — user types and user converters: a user type
// converted through its stream operators (cstream) and through user
// conversion operators of the converter signature; user converters as a
// class, a plain function and a lambda (both documented signatures); a type
// without a default constructor through boost::make_default; boost::cnv::apply
// with a user converter in an algorithm; a string-to-string transforming
// converter (other.qbk "Beyond Basic Conversions").
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.convert;

namespace user {
struct rgb {
    std::uint8_t r = 0, g = 0, b = 0;
    bool operator==(const rgb&) const = default;
};
std::ostream& operator<<(std::ostream& os, const rgb& c) {
    return os << '#' << std::hex << std::setfill('0') << std::setw(2) << int(c.r) << std::setw(2) << int(c.g)
              << std::setw(2) << int(c.b) << std::dec;
}
std::istream& operator>>(std::istream& is, rgb& c) {
    std::string s;
    is >> s;
    if (s.size() != 7 || s[0] != '#') {
        is.setstate(std::ios_base::failbit);
        return is;
    }
    unsigned v = std::stoul(s.substr(1), nullptr, 16);
    c = {std::uint8_t(v >> 16), std::uint8_t(v >> 8), std::uint8_t(v)};
    return is;
}

// A converter class with the library's signature #2.
struct rgb_converter {
    void operator()(std::string const& in, boost::optional<rgb>& out) const {
        if (in == "red") out = rgb{255, 0, 0};
        else if (in == "green") out = rgb{0, 255, 0};
    }
    void operator()(rgb const& in, boost::optional<std::string>& out) const {
        if (in == rgb{255, 0, 0}) out = std::string("red");
    }
};

// A plain function converter.
void parse_bool(std::string const& in, boost::optional<bool>& out) {
    if (in == "yes" || in == "true") out = true;
    else if (in == "no" || in == "false") out = false;
}

// No default constructor.
struct port {
    explicit port(int n) : n(n) {}
    int n;
};
std::istream& operator>>(std::istream& is, port& p) {
    int n = -1;
    if (is >> n && n >= 0 && n < 65536) p = port(n);
    else is.setstate(std::ios_base::failbit);
    return is;
}
}  // namespace user

namespace boost {
template <>
inline user::port make_default<user::port>() {
    return user::port(0);
}
}  // namespace boost

TEST_CASE("a user type through its stream operators") {
    boost::cnv::cstream cnv;
    CHECK(boost::convert<std::string>(user::rgb{1, 2, 255}, cnv).value() == "#0102ff");
    CHECK(boost::convert<user::rgb>("#0a0b0c", cnv).value() == user::rgb{10, 11, 12});
    CHECK_FALSE(boost::convert<user::rgb>("0a0b0c", cnv).has_value());
    boost::cnv::lexical_cast lc;
    CHECK(boost::convert<user::rgb>("#ffffff", lc).value() == user::rgb{255, 255, 255});
}

TEST_CASE("user converters: class, function, lambda") {
    user::rgb_converter rc;
    CHECK(boost::convert<user::rgb>(std::string("red"), rc).value() == user::rgb{255, 0, 0});
    CHECK_FALSE(boost::convert<user::rgb>(std::string("blue"), rc).has_value());
    CHECK(boost::convert<std::string>(user::rgb{255, 0, 0}, rc).value() == "red");
    CHECK(boost::convert<bool>(std::string("yes"), &user::parse_bool).value());
    CHECK_FALSE(boost::convert<bool>(std::string("maybe"), &user::parse_bool).has_value());
    auto twice = [](int in, boost::optional<int>& out) { out = in * 2; };
    CHECK(boost::convert<int>(21, twice).value() == 42);
}

TEST_CASE("a type without a default constructor") {
    boost::cnv::cstream cnv;
    CHECK(boost::convert<user::port>("8080", cnv).value().n == 8080);
    CHECK(boost::convert<user::port>("70000", cnv, user::port(1)).n == 1);
}

TEST_CASE("apply with a user converter in an algorithm") {
    std::vector<std::string> in{"true", "nope", "no"};
    std::vector<bool> out;
    std::transform(in.begin(), in.end(), std::back_inserter(out),
                   boost::cnv::apply<bool>(&user::parse_bool).value_or(false));
    CHECK(out == std::vector<bool>{true, false, false});
    std::vector<int> n;
    std::vector<std::string> nums{"1", "x", "3"};
    std::transform(nums.begin(), nums.end(), std::back_inserter(n),
                   boost::cnv::apply<int>(boost::cnv::strtol()).value_or(0));
    CHECK(n == std::vector<int>{1, 0, 3});
}

namespace user {
// other.qbk: a converter that transforms instead of converting types —
// "ABC" <-> "123" (the page's test/encryption.cpp is not in the release).
struct cypher {
    void operator()(std::string const& in, boost::optional<std::string>& out) const {
        std::string r;
        for (char c : in) {
            if (c >= 'A' && c <= 'I') r += char('1' + (c - 'A'));
            else if (c >= '1' && c <= '9') r += char('A' + (c - '1'));
            else return;  // not encodable: conversion fails
        }
        out = r;
    }
};
}  // namespace user

TEST_CASE("doc: other_conversions") {
    using std::string;
    user::cypher my_cypher;
    string encrypted = boost::convert<string>("ABC", my_cypher).value();
    string decrypted = boost::convert<string>(encrypted, my_cypher).value();
    CHECK(encrypted == "123");
    CHECK(decrypted == "ABC");
    CHECK(!boost::convert<string>("abc", my_cypher));
}
