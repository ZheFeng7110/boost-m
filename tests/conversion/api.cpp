// mcpplibs.mwrap.boost.conversion — polymorphic_cast (pointer and reference,
// throwing std::bad_cast), polymorphic_downcast, polymorphic_pointer_cast /
// polymorphic_pointer_downcast over raw, std and boost smart pointers, and
// implicit_cast (constexpr, no explicit conversions), after
// libs/conversion/test/{cast_test,polymorphic_cast_test,implicit_cast}.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.conversion;
import mcpplibs.mwrap.boost.smart_ptr;

namespace {
struct Base {
    virtual ~Base() = default;
    virtual char kind() const { return 'B'; }
};
struct Base2 {
    virtual ~Base2() = default;
    virtual char kind2() const { return '2'; }
};
struct Derived : Base, Base2 {
    char kind() const override { return 'D'; }
};
struct Other : Base {};
}  // namespace

TEST_CASE("polymorphic_cast: downcast and crosscast") {
    Derived d;
    Base* b = &d;
    CHECK(boost::polymorphic_cast<Derived*>(b)->kind() == 'D');
    Base2* cross = boost::polymorphic_cast<Base2*>(b);  // crosscast
    CHECK(cross->kind2() == '2');
    Other o;
    Base* ob = &o;
    CHECK_THROWS_AS(boost::polymorphic_cast<Derived*>(ob), std::bad_cast);
}

TEST_CASE("polymorphic_downcast: pointer and reference") {
    Derived d;
    Base* b = &d;
    CHECK(boost::polymorphic_downcast<Derived*>(b) == &d);
    const Base& cb = d;
    CHECK(&boost::polymorphic_downcast<const Derived&>(cb) == &d);
}

TEST_CASE("polymorphic_pointer_cast / downcast") {
    auto sp = std::make_shared<Derived>();
    std::shared_ptr<Base> base = sp;
    std::shared_ptr<Derived> back = boost::polymorphic_pointer_cast<Derived>(base);
    CHECK(back == sp);
    CHECK(boost::polymorphic_pointer_downcast<Derived>(base) == sp);
    CHECK(boost::polymorphic_pointer_cast<Base2>(base)->kind2() == '2');
    std::shared_ptr<Base> other = std::make_shared<Other>();
    CHECK_THROWS_AS(boost::polymorphic_pointer_cast<Derived>(other), std::bad_cast);
    boost::shared_ptr<Base> bsp(new Derived);
    CHECK(boost::polymorphic_pointer_cast<Derived>(bsp)->kind() == 'D');
    Derived raw;
    Base* rb = &raw;
    CHECK(boost::polymorphic_pointer_cast<Derived>(rb) == &raw);
    CHECK(boost::polymorphic_pointer_downcast<Derived>(rb) == &raw);
    std::shared_ptr<Base> empty;
    CHECK_THROWS_AS(boost::polymorphic_pointer_cast<Derived>(empty), std::bad_cast);
}

namespace {
struct from_int {
    from_int(int v) : v(v) {}  // implicit
    int v;
};
struct explicit_only {
    explicit explicit_only(int) {}
};

template <class T, class U, class = void>
inline constexpr bool implicit_castable = false;
template <class T, class U>
inline constexpr bool implicit_castable<T, U, std::void_t<decltype(boost::implicit_cast<T>(std::declval<U>()))>> = true;
}  // namespace

TEST_CASE("implicit_cast") {
    constexpr long l = boost::implicit_cast<long>(7);
    static_assert(l == 7);
    CHECK(boost::implicit_cast<from_int>(5).v == 5);
    Derived d;
    Base* up = boost::implicit_cast<Base*>(&d);  // upcast is implicit
    CHECK(up->kind() == 'D');
    CHECK(boost::implicit_cast<const char*>("x")[0] == 'x');
    static_assert(implicit_castable<from_int, int>);
    static_assert(!implicit_castable<explicit_only, int>);  // explicit constructor
    static_assert(!implicit_castable<Derived*, Base*>);     // no downcast
    static_assert(!implicit_castable<int*, void*>);         // no void* -> T*
}
