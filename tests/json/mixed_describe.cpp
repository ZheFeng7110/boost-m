// boost.json with described types — libs/json/doc/pages/conversion
// (described classes and enums are converted automatically) and parse_into of
// a described struct. The BOOST_DESCRIBE_* macros come from #include (no
// module carries them) and Boost.JSON from its module (design §4.4: the
// includes precede the imports).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/describe.hpp>
import std;
import mcpplibs.mwrap.boost.json;

namespace user {
enum class level { low, high };
BOOST_DESCRIBE_ENUM(level, low, high)
struct settings {
    std::string name;
    int retries = 0;
    level lvl = level::low;
    std::vector<double> weights;
    std::optional<std::string> note;
};
BOOST_DESCRIBE_STRUCT(settings, (), (name, retries, lvl, weights, note))
struct base {
    int id = 0;
};
BOOST_DESCRIBE_STRUCT(base, (), (id))
struct derived : base {
    bool on = false;
};
BOOST_DESCRIBE_STRUCT(derived, (base), (on))
}  // namespace user

TEST_CASE("described struct and enum: value_from / value_to") {
    user::settings s{"svc", 3, user::level::high, {0.5}, std::nullopt};
    boost::json::value jv = boost::json::value_from(s);
    CHECK(jv.at("name") == "svc");
    CHECK(jv.at("lvl") == "high");
    CHECK(jv.at("note").is_null());
    auto back = boost::json::value_to<user::settings>(jv);
    CHECK(back.retries == 3);
    CHECK(back.lvl == user::level::high);
    user::derived d;
    d.id = 9;
    d.on = true;
    CHECK(boost::json::serialize(boost::json::value_from(d)) == R"({"id":9,"on":true})");
}

TEST_CASE("parse_into a described struct") {
    user::settings s;
    boost::system::error_code ec;
    boost::json::parse_into(s, R"({"name":"x","retries":2,"lvl":"low","weights":[1,2],"note":"n"})", ec);
    REQUIRE_FALSE(ec.failed());
    CHECK(s.weights.size() == 2);
    CHECK(*s.note == "n");
    boost::json::parse_into(s, R"({"name":1})", ec);
    CHECK(ec.failed());
}
