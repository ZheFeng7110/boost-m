// mcpplibs.mwrap.boost.json — user types at Boost.JSON's customization points
// through the module: tag_invoke(value_from_tag / value_to_tag /
// try_value_to_tag) for a user struct, user sequence and map types converted
// by the generic rules (begin/end + value_type, key/mapped), a user optional-
// like and variant-like through the standard ones, conversion contexts
// (a user context selecting a representation), and a user memory_resource
// behind storage_ptr. Described types are mixed_describe.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.json;

namespace json = boost::json;

namespace user {
struct point {
    double x, y;
    bool operator==(const point&) const = default;
};
void tag_invoke(json::value_from_tag, json::value& jv, point const& p) { jv = {{"x", p.x}, {"y", p.y}}; }
point tag_invoke(json::value_to_tag<point>, json::value const& jv) {
    auto& o = jv.as_object();
    return {json::value_to<double>(o.at("x")), json::value_to<double>(o.at("y"))};
}

struct temperature {
    double kelvin;
};
boost::system::result<temperature> tag_invoke(json::try_value_to_tag<temperature>, json::value const& jv) {
    if (!jv.is_number()) return boost::system::error_code(json::error::not_number);
    double k = jv.to_number<double>();
    if (k < 0) return boost::system::error_code(json::error::out_of_range);
    return temperature{k};
}

// A user sequence container: converted element-wise by the generic rules.
class ring {
public:
    using value_type = int;
    using iterator = std::vector<int>::const_iterator;
    using const_iterator = iterator;
    ring() = default;
    iterator begin() const { return v_.begin(); }
    iterator end() const { return v_.end(); }
    void push_back(int x) { v_.push_back(x); }
    std::size_t size() const { return v_.size(); }
private:
    std::vector<int> v_;
};

// A user context: points as two-element arrays instead of objects.
struct as_array_ctx {};
void tag_invoke(json::value_from_tag, json::value& jv, point const& p, as_array_ctx const&) {
    jv = {p.x, p.y};
}

class counting_resource : public boost::container::pmr::memory_resource {
public:
    std::size_t allocated = 0;
private:
    void* do_allocate(std::size_t n, std::size_t align) override {
        allocated += n;
        return ::operator new(n, std::align_val_t(align));
    }
    void do_deallocate(void* p, std::size_t, std::size_t align) override {
        ::operator delete(p, std::align_val_t(align));
    }
    bool do_is_equal(const boost::container::pmr::memory_resource& o) const noexcept override { return this == &o; }
};
}  // namespace user

TEST_CASE("doc: conversion/custom#tag-invoke-overloads (user struct)") {
    user::point p{1.5, -2};
    json::value jv = json::value_from(p);
    CHECK(json::serialize(jv) == R"({"x":1.5E0,"y":-2E0})");
    CHECK(json::value_to<user::point>(jv) == p);
    std::vector<user::point> pts{{1, 2}, {3, 4}};
    json::value arr = json::value_from(pts);  // containers of user types
    CHECK(arr.as_array().size() == 2);
    CHECK(json::value_to<std::vector<user::point>>(arr) == pts);
    std::map<std::string, user::point> named{{"o", {0, 0}}};
    CHECK(json::value_from(named).at("o").at("x") == 0.0);
}

TEST_CASE("try_value_to with a user error") {
    auto ok = json::try_value_to<user::temperature>(json::value(300.0));
    REQUIRE(ok.has_value());
    CHECK(ok->kelvin == 300.0);
    auto neg = json::try_value_to<user::temperature>(json::value(-1));
    CHECK(neg.error() == json::error::out_of_range);
    auto str = json::try_value_to<user::temperature>(json::value("hot"));
    CHECK(str.has_error());
    CHECK_THROWS_AS(json::value_to<user::temperature>(json::value("hot")), boost::system::system_error);
}

TEST_CASE("user sequence containers through the generic rules") {
    user::ring r;
    r.push_back(1);
    r.push_back(2);
    static_assert(json::is_sequence_like<user::ring>::value);
    CHECK(json::serialize(json::value_from(r)) == "[1,2]");
    auto back = json::value_to<user::ring>(json::parse("[3,4,5]"));
    CHECK(back.size() == 3);
}

TEST_CASE("standard vocabulary types holding user types") {
    std::optional<user::point> none, some = user::point{1, 1};
    CHECK(json::value_from(none).is_null());
    CHECK(json::value_from(some).is_object());
    std::variant<int, user::point> v = user::point{2, 3};
    json::value jv = json::value_from(v);
    CHECK(jv.at("y") == 3.0);
    CHECK(std::get<user::point>(json::value_to<std::variant<int, user::point>>(jv)) == user::point{2, 3});
}

TEST_CASE("doc: conversion/context#passing-data-to-conversion-functions (user context)") {
    user::point p{7, 8};
    CHECK(json::serialize(json::value_from(p, user::as_array_ctx{})) == "[7E0,8E0]");
    CHECK(json::value_from(p).is_object());  // without the context
}

TEST_CASE("a user memory_resource behind storage_ptr") {
    user::counting_resource mr;
    {
        json::storage_ptr sp(&mr);
        json::value jv = json::parse(R"({"a":[1,2,3],"b":"a string long enough to allocate"})", sp);
        CHECK(jv.storage().get() == &mr);
        CHECK(jv.at("a").storage().get() == &mr);  // propagated to every element
    }
    CHECK(mr.allocated > 0);
}
