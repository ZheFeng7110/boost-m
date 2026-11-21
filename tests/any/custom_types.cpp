// mcpplibs.mwrap.boost.any — user-defined value types in boost::any,
// basic_any and unique_any: the ValueType requirements (copyable for any,
// move-only allowed for unique_any), small-object storage vs heap storage,
// a throwing copy constructor leaving the target unchanged, and destructors
// running exactly once.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.any;

namespace user {
inline int live = 0;
struct tracked {
    int id;
    explicit tracked(int i) : id(i) { ++live; }
    tracked(const tracked& o) : id(o.id) { ++live; }
    tracked(tracked&& o) noexcept : id(o.id) { ++live; }
    tracked& operator=(const tracked&) = default;
    ~tracked() { --live; }
};

struct move_only {
    std::unique_ptr<int> p;
    explicit move_only(int v) : p(std::make_unique<int>(v)) {}
    move_only(move_only&&) noexcept = default;
    move_only& operator=(move_only&&) noexcept = default;
};

struct throwing_copy {
    int v = 0;
    throwing_copy() = default;
    throwing_copy(const throwing_copy&) { throw std::runtime_error("copy"); }
    throwing_copy(throwing_copy&&) noexcept = default;
};

struct big {
    std::array<double, 32> data{};
    std::string label;
};
}  // namespace user

TEST_CASE("any: user type copy/move and lifetime") {
    user::live = 0;
    {
        boost::any a = user::tracked(1);
        CHECK(user::live == 1);
        boost::any b = a;
        CHECK(user::live == 2);
        CHECK(boost::any_cast<user::tracked&>(b).id == 1);
        a = user::tracked(2);
        CHECK(boost::any_cast<const user::tracked&>(a).id == 2);
        CHECK(user::live == 2);
    }
    CHECK(user::live == 0);
}

TEST_CASE("any: throwing copy keeps the target (strong guarantee)") {
    boost::any target = 5;
    user::throwing_copy src;
    CHECK_THROWS_AS(target = src, std::runtime_error);
    CHECK(boost::any_cast<int>(target) == 5);
    target = user::throwing_copy{};  // move: does not throw
    CHECK(target.type() == typeid(user::throwing_copy));
}

TEST_CASE("basic_any: user types below and above the buffer size") {
    using any16 = boost::anys::basic_any<16, 8>;
    user::live = 0;
    {
        any16 small = user::tracked(3);
        any16 large = user::big{{}, "big"};
        CHECK(boost::anys::any_cast<user::tracked&>(small).id == 3);
        CHECK(boost::anys::any_cast<user::big&>(large).label == "big");
        any16 copy = small;
        CHECK(user::live == 2);
        boost::anys::swap(small, large);
        CHECK(boost::anys::any_cast<user::big&>(small).label == "big");
        CHECK(boost::anys::any_cast<user::tracked&>(large).id == 3);
    }
    CHECK(user::live == 0);
}

TEST_CASE("unique_any: move-only user type") {
    boost::anys::unique_any u = user::move_only(9);
    CHECK(*boost::anys::any_cast<user::move_only&>(u).p == 9);
    boost::anys::unique_any v = std::move(u);
    CHECK_FALSE(u.has_value());
    user::move_only out = boost::anys::any_cast<user::move_only>(std::move(v));
    CHECK(*out.p == 9);
    auto& e = v.emplace<user::move_only>(4);
    CHECK(*e.p == 4);
    // unique_any accepts what boost::any (CopyConstructible values only) cannot.
    static_assert(std::is_constructible_v<boost::anys::unique_any, user::move_only>);
}

TEST_CASE("containers of any holding user types") {
    std::vector<boost::any> bag{user::big{{}, "a"}, std::string("b"), 3};
    std::size_t bigs = std::count_if(bag.begin(), bag.end(), [](const boost::any& x) {
        return boost::any_cast<user::big>(&x) != nullptr;
    });
    CHECK(bigs == 1);
}
