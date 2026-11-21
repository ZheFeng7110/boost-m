// mcpplibs.mwrap.boost.variant2 — the overview examples of Boost.Variant2
// (libs/variant2/doc/variant2/overview.adoc) as test cases.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.variant2;

using namespace boost::variant2;

namespace shapes_virtual {
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
};
class Rectangle : public Shape {
    double width_, height_;
public:
    Rectangle(double width, double height) : width_(width), height_(height) {}
    double area() const override { return width_ * height_; }
};
class Circle : public Shape {
    double radius_;
public:
    explicit Circle(double radius) : radius_(radius) {}
    double area() const override { return std::numbers::pi * radius_ * radius_; }
};
double total_area(std::vector<std::unique_ptr<Shape>> const& v) {
    double s = 0.0;
    for (auto const& p : v) s += p->area();
    return s;
}
}  // namespace shapes_virtual

namespace shapes_variant {
struct Rectangle {
    double width_, height_;
    double area() const { return width_ * height_; }
};
struct Circle {
    double radius_;
    double area() const { return std::numbers::pi * radius_ * radius_; }
};
double total_area(std::vector<variant<Rectangle, Circle>> const& v) {
    double s = 0.0;
    for (auto const& x : v) s += visit([](auto const& y) { return y.area(); }, x);
    return s;
}
}  // namespace shapes_variant

TEST_CASE("doc: description") {
    // A configuration file of key=value lines as map<string, variant<...>>.
    std::map<std::string, variant<std::int64_t, double, std::string>> config;
    std::istringstream file("server.host=test.example.com\nserver.port=9174\ncache.max_load=0.7\n");
    for (std::string line; std::getline(file, line);) {
        auto eq = line.find('=');
        std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        if (value.find_first_not_of("0123456789") == std::string::npos)
            config[key] = std::int64_t(std::stoll(value));
        else if (value.find_first_not_of("0123456789.") == std::string::npos)
            config[key] = std::stod(value);
        else
            config[key] = value;
    }
    CHECK(get<std::string>(config["server.host"]) == "test.example.com");
    CHECK(get<std::int64_t>(config["server.port"]) == 9174);
    CHECK(get<double>(config["cache.max_load"]) == doctest::Approx(0.7));
}

TEST_CASE("doc: usage-examples") {
    std::vector<std::unique_ptr<shapes_virtual::Shape>> v1;
    v1.push_back(std::make_unique<shapes_virtual::Circle>(1.0));
    v1.push_back(std::make_unique<shapes_virtual::Rectangle>(2.0, 3.0));
    std::vector<variant<shapes_variant::Rectangle, shapes_variant::Circle>> v2;
    v2.push_back(shapes_variant::Circle{1.0});
    v2.push_back(shapes_variant::Rectangle{2.0, 3.0});
    CHECK(shapes_variant::total_area(v2) == doctest::Approx(std::numbers::pi + 6.0));
    CHECK(shapes_variant::total_area(v2) == doctest::Approx(shapes_virtual::total_area(v1)));
}

TEST_CASE("doc: construction-and-assignment") {
    variant<shapes_variant::Rectangle, shapes_variant::Circle> v = shapes_variant::Circle{1.0};
    CHECK(v.index() == 1);
    v = shapes_variant::Rectangle{2.0, 3.0};
    CHECK(v.index() == 0);
    variant<int, float> w((short)1);  // overload resolution picks int
    CHECK(holds_alternative<int>(w));
}

namespace {
std::string f_index(variant<int, float> const& v) {
    switch (v.index()) {
    case 0: return "int " + std::to_string(get<0>(v));
    case 1: return "float " + std::to_string(int(get<1>(v)));
    default: return "never";
    }
}
std::string f_get_if(variant<int, float> const& v) {
    if (int const* p = get_if<int>(&v)) return "int " + std::to_string(*p);
    if (float const* p = get_if<float>(&v)) return "float " + std::to_string(int(*p));
    return "never";
}
}  // namespace

TEST_CASE("doc: inspecting-the-value") {
    variant<int, float> a = 3, b = 2.0f;
    CHECK(f_index(a) == "int 3");
    CHECK(f_index(b) == "float 2");
    CHECK(f_get_if(a) == "int 3");
    CHECK(f_get_if(b) == "float 2");
    CHECK_THROWS_AS(get<0>(b), bad_variant_access);
    CHECK_THROWS_AS(get<float>(a), bad_variant_access);
}

namespace {
double f(double x) { return x; }
double g1(variant<int, float> const& v) { return visit(f, v); }

struct F {
    std::string* out;
    void operator()(int x) const { *out = "int " + std::to_string(x); }
    void operator()(float) const { *out = "float"; }
};
}  // namespace

TEST_CASE("doc: visitation") {
    CHECK(g1(variant<int, float>(4)) == 4.0);
    CHECK(g1(variant<int, float>(1.5f)) == 1.5);
    std::string out;
    visit(F{&out}, variant<int, float>(7));
    CHECK(out == "int 7");
    std::ostringstream os;
    visit([&](auto const& x) { os << x; }, variant<int, float>(9));
    CHECK(os.str() == "9");
    variant<int, float> v1 = 2, v2 = 0.5f;
    CHECK(visit([](auto x1, auto x2) { return double(x1) * double(x2); }, v1, v2) == 1.0);
    // Default construction value-initializes the first alternative; monostate
    // avoids constructing anything.
    CHECK(get<int>(variant<int, float>{}) == 0);
    CHECK(get<float>(variant<float, int>{}) == 0.0f);
    variant<monostate, std::mutex, std::recursive_mutex> m;
    CHECK(m.index() == 0);
    m.emplace<std::mutex>();
    CHECK(holds_alternative<std::mutex>(m));
}
