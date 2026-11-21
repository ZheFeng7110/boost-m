// boost.describe — user types described with the BOOST_DESCRIBE_* macros, the
// library's whole purpose: the class and enum pages of the documentation
// (libs/describe/doc/describe/{classes,enums}.adoc) plus the queries over the
// descriptors (members, bases, modifiers, static and function members,
// overloads, nested enums). Pure include: describe has no module.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <boost/describe.hpp>
#include <boost/mp11.hpp>

using namespace boost::describe;

namespace user {
struct X {
    int m1;
    int m2;
};
BOOST_DESCRIBE_STRUCT(X, (), (m1, m2))

class Y : private X {
public:
    int m3 = 3;
protected:
    int m4 = 4;
private:
    int m5 = 5;
public:
    int f() const { return m5; }
    static int s;
private:
    BOOST_DESCRIBE_CLASS(Y, (X), (m3, f, s), (m4), (m5))
};
inline int Y::s = 9;

struct O {
    int f() { return 1; }
    int f() const { return 2; }
    void f(int) {}
};
BOOST_DESCRIBE_STRUCT(O, (), ((int()) f, (int() const) f, (void(int)) f))

class S {
public:
    static void f(int) {}
    static void f(int, int) {}
    BOOST_DESCRIBE_CLASS(S, (), ((void(int)) f, (void(int, int)) f), (), ())
};

enum E { v1 = 11, v2, v3 = 5 };
BOOST_DESCRIBE_ENUM(E, v1, v2, v3)
enum class F : char { a = 'a', b = 'b' };
BOOST_DESCRIBE_ENUM(F, a, b)
BOOST_DEFINE_ENUM_CLASS(G, red, green, blue)

struct Outer {
    enum class Inner { p, q };
    BOOST_DESCRIBE_NESTED_ENUM(Inner, p, q)
};

template <class T, unsigned M = mod_any_access>
std::vector<std::string> member_names() {
    std::vector<std::string> out;
    boost::mp11::mp_for_each<describe_members<T, M>>([&](auto D) { out.push_back(D.name); });
    return out;
}
}  // namespace user

TEST_CASE("doc: classes#class-types-with-public-members") {
    using user::X;
    static_assert(has_describe_members<X>::value);
    CHECK(user::member_names<X, mod_public>() == std::vector<std::string>{"m1", "m2"});
    X x{1, 2};
    int sum = 0;
    boost::mp11::mp_for_each<describe_members<X, mod_public>>([&](auto D) { sum += x.*D.pointer; });
    CHECK(sum == 3);
}

TEST_CASE("doc: classes#class-types-with-protected-or-private-members") {
    using user::Y;
    CHECK(user::member_names<Y, mod_public>() == std::vector<std::string>{"m3"});
    CHECK(user::member_names<Y, mod_protected>() == std::vector<std::string>{"m4"});
    CHECK(user::member_names<Y, mod_private>() == std::vector<std::string>{"m5"});
    CHECK(user::member_names<Y, mod_any_access | mod_inherited>().size() == 5);  // + X's m1, m2
    using B = describe_bases<Y, mod_any_access>;
    static_assert(boost::mp11::mp_size<B>::value == 1);
    using D1 = boost::mp11::mp_front<B>;
    static_assert(std::is_same_v<D1::type, user::X>);
    static_assert(D1::modifiers == mod_private);
}

TEST_CASE("doc: classes#retrieving-class-properties") {
    using user::Y;
    Y y;
    std::vector<std::string> fns;
    boost::mp11::mp_for_each<describe_members<Y, mod_public | mod_function>>([&](auto D) {
        fns.push_back(D.name);
        CHECK((y.*D.pointer)() == 5);
    });
    CHECK(fns == std::vector<std::string>{"f"});
    boost::mp11::mp_for_each<describe_members<Y, mod_public | mod_static>>([&](auto D) {
        CHECK(std::string(D.name) == "s");
        CHECK(*D.pointer == 9);
    });
    static_assert(!has_describe_members<int>::value);
    static_assert(has_describe_bases<Y>::value);
}

TEST_CASE("doc: classes#overloaded-member-functions") {
    user::O o;
    const user::O& co = o;
    int calls = 0;
    boost::mp11::mp_for_each<describe_members<user::O, mod_public | mod_function>>([&](auto D) {
        using P = std::remove_cv_t<decltype(D.pointer)>;
        if constexpr (std::is_same_v<P, int (user::O::*)()>) calls += (o.*D.pointer)();
        if constexpr (std::is_same_v<P, int (user::O::*)() const>) calls += (co.*D.pointer)() * 10;
        if constexpr (std::is_same_v<P, void (user::O::*)(int)>) (o.*D.pointer)(0);
    });
    CHECK(calls == 21);
    int statics = 0;
    boost::mp11::mp_for_each<describe_members<user::S, mod_public | mod_static | mod_function>>(
        [&](auto) { ++statics; });
    CHECK(statics == 2);
}

TEST_CASE("doc: enums#enums") {
    std::vector<std::string> names;
    std::vector<int> values;
    boost::mp11::mp_for_each<describe_enumerators<user::E>>([&](auto D) {
        names.push_back(D.name);
        values.push_back(D.value);
    });
    CHECK(names == std::vector<std::string>{"v1", "v2", "v3"});
    CHECK(values == std::vector<int>{11, 12, 5});
    CHECK(std::string_view(enum_to_string(user::F::b, "?")) == "b");
    user::F f{};
    CHECK(enum_from_string("a", f));
    CHECK(f == user::F::a);
    CHECK_FALSE(enum_from_string("z", f));
    CHECK(std::string_view(enum_to_string(user::G::green, "?")) == "green");
    CHECK(std::string_view(enum_to_string(user::Outer::Inner::q, "?")) == "q");
    static_assert(has_describe_enumerators<user::G>::value);
}
