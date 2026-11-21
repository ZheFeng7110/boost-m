// boost.convert spirit converter — pure include. boost/convert/spirit.hpp
// pulls Boost.Spirit (header-only, no module), so it is not in the convert
// module (tools/export_policy.toml [convert]); this TU uses headers only.
// Covers the spirit part of libs/convert/example/getting_started.cpp
// (example3) and the stream_converter page's default-constructible note.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <string>
#include <boost/convert.hpp>
#include <boost/convert/spirit.hpp>
#include <boost/convert/stream.hpp>

namespace {
// A type without a default constructor: convert() builds it through
// boost::make_default.
struct direction {
    enum value_type { up, dn };
    direction(value_type value) : value_(value) {}
    bool operator==(direction that) const { return value_ == that.value_; }
    value_type value() const { return value_; }
private:
    value_type value_;
};
std::istream& operator>>(std::istream& stream, direction& dir) {
    std::string str;
    stream >> str;
    if (str == "up") dir = direction::up;
    else if (str == "dn") dir = direction::dn;
    else stream.setstate(std::ios_base::failbit);
    return stream;
}
}  // namespace

namespace boost {
template <>
inline direction make_default<direction>() {
    return direction(direction::up);
}
}  // namespace boost

TEST_CASE("example: getting_started (spirit converter)") {
    auto cnv3 = boost::cnv::spirit();
    CHECK(boost::convert<int>("123", cnv3).value() == 123);
    CHECK_FALSE(boost::convert<int>("12x", cnv3).has_value());
    CHECK(boost::convert<double>("1.25", cnv3).value() == 1.25);
}

TEST_CASE("doc: the-default-constructible-type-requirement") {
    boost::cnv::cstream cnv;
    CHECK(boost::convert<direction>("dn", cnv).value() == direction::dn);
    CHECK(boost::convert<direction>("xx", cnv, direction::up) == direction::up);
}
