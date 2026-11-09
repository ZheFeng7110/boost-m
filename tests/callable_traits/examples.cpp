// mcpplibs.mwrap.boost.callable_traits — the 37 official examples
// (libs/callable_traits/example/*.cpp) as test cases, each in its own
// namespace. They are compile-time checks: the test passes by compiling.
// The examples' feature guards are resolved for a C++23 compiler:
// BOOST_CLBL_TRTS_DISABLE_* and _MSVC are off, noexcept function types are on,
// and transaction_safe (GCC's -fgnu-tm) is off: the three *_transaction_safe
// examples compile to an empty main upstream then, and are excluded in
// coverage.toml.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.callable_traits;

// libs/callable_traits/example/add_member_const.cpp
namespace ex_add_member_const {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: add_member_const") {

    {
        using pmf = int(foo::*)();
        using expect = int(foo::*)() const;
        using test = ct::add_member_const_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_const_t doesn't change anything when
        // the function type is already const.
        using pmf = int(foo::*)() const &&;
        using expect = int(foo::*)() const &&;
        using test = ct::add_member_const_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() volatile &;
        using expect = int(foo::*)() const volatile &;
        using test = ct::add_member_const_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_const_t can also be used with "abominable"
        // function types.
        using f = int();
        using expect = int() const;
        using test = ct::add_member_const_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_add_member_const

// libs/callable_traits/example/add_member_cv.cpp
namespace ex_add_member_cv {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: add_member_cv") {

    {
        using pmf = void(foo::*)();
        using expect = void(foo::*)() const volatile;
        using test = ct::add_member_cv_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_cv_t doesn't change anything when
        // the function type is already cv-qualified.
        using pmf = void(foo::*)() const volatile &&;
        using expect = void(foo::*)() const volatile &&;
        using test = ct::add_member_cv_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = void(foo::*)() volatile &;
        using expect = void(foo::*)() const volatile &;
        using test = ct::add_member_cv_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_cv_t can also be used with "abominable"
        // function types.
        using f = void();
        using expect = void() const volatile;
        using test = ct::add_member_cv_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_add_member_cv

// libs/callable_traits/example/add_member_lvalue_reference.cpp
namespace ex_add_member_lvalue_reference {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: add_member_lvalue_reference") {

    {
        using pmf = void(foo::*)();
        using expect = void(foo::*)() &;
        using test = ct::add_member_lvalue_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_lvalue_reference_t doesn't change anything when
        // the function type already has an lvalue qualifier.
        using pmf = void(foo::*)() &;
        using expect = void(foo::*)() &;
        using test = ct::add_member_lvalue_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_lvalue_reference_t models C++11 reference collapsing
        // rules, so that adding an lvalue qualifier to an
        // rvalue-qualified type will force the lvalue.
        using pmf = void(foo::*)() &&;
        using expect = void(foo::*)() &;
        using test = ct::add_member_lvalue_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_lvalue_reference_t can also be used to create "abominable"
        // function types.
        using f = void();
        using expect = void() &;
        using test = ct::add_member_lvalue_reference_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_add_member_lvalue_reference

// libs/callable_traits/example/add_member_rvalue_reference.cpp
namespace ex_add_member_rvalue_reference {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: add_member_rvalue_reference") {

    {
        using pmf = void(foo::*)();
        using expect = void(foo::*)() &&;
        using test = ct::add_member_rvalue_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_rvalue_reference_t doesn't change anything when
        // the function type already has an rvalue qualifier.
        using pmf = void(foo::*)() &&;
        using expect = void(foo::*)() &&;
        using test = ct::add_member_rvalue_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_rvalue_reference_t models C++11 reference collapsing
        // rules, so that adding an rvalue qualifier to an
        // lvalue-qualified type will not change anything.
        using pmf = void(foo::*)() const &;
        using expect = void(foo::*)() const &;
        using test = ct::add_member_rvalue_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_rvalue_reference_t can also be used with "abominable"
        // function types.
        using f = void() const;
        using expect = void() const &&;
        using test = ct::add_member_rvalue_reference_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_add_member_rvalue_reference

// libs/callable_traits/example/add_member_volatile.cpp
namespace ex_add_member_volatile {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: add_member_volatile") {

    {
        using pmf = void(foo::*)();
        using expect = void(foo::*)() volatile;
        using test = ct::add_member_volatile_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_volatile_t doesn't change anything when
        // the function type is already volatile.
        using pmf = void(foo::*)() volatile &&;
        using expect = void(foo::*)() volatile &&;
        using test = ct::add_member_volatile_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = void(foo::*)() const &;
        using expect = void(foo::*)() const volatile &;
        using test = ct::add_member_volatile_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        // add_member_volatile_t can also be used with "abominable"
        // function types.
        using f = void();
        using expect = void() volatile;
        using test = ct::add_member_volatile_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_add_member_volatile

// libs/callable_traits/example/add_noexcept.cpp
namespace ex_add_noexcept {
using boost::callable_traits::add_noexcept_t;

static_assert(std::is_same<
    add_noexcept_t<int()>,
    int() noexcept
>{}, "");

TEST_CASE("example: add_noexcept") {}
}  // namespace ex_add_noexcept

// libs/callable_traits/example/add_varargs.cpp
namespace ex_add_varargs {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: add_varargs") {

    {
        using f = void(int);
        using expect = void(int, ...);
        using test = ct::add_varargs_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using fp = void(*)();
        using expect = void(*)(...);
        using test = ct::add_varargs_t<fp>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using fr = void(&)(const char*);
        using expect = void(&)(const char*, ...);
        using test = ct::add_varargs_t<fr>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = void(foo::*)() const;
        using expect = void(foo::*)(...) const;
        using test = ct::add_varargs_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");

        // add_varargs_t doesn't change anything when
        // the type already has varargs.
        using twice = ct::add_varargs_t<test>;
        static_assert(std::is_same<test, twice>::value, "");
    }
}
}  // namespace ex_add_varargs

// libs/callable_traits/example/apply_member_pointer.cpp
namespace ex_apply_member_pointer {
namespace ct = boost::callable_traits;

struct foo;
struct bar;

TEST_CASE("example: apply_member_pointer") {

    {
        // function type -> member function pointer type
        using f = int(int);
        using g = ct::apply_member_pointer_t<f, foo>;
        using expect = int(foo::*)(int);
        static_assert(std::is_same<g, expect>::value, "");
    }

    {
        // function pointer type (unqualified) -> member function pointer type
        using f = int(*)();
        using g = ct::apply_member_pointer_t<f, foo>;
        using expect = int(foo::*)();
        static_assert(std::is_same<g, expect>::value, "");
    }

    {
        // function pointer type (qualified) -> member data pointer type

        // Look out for cases like these two. If the input type to apply_member_pointer
        // is a ``[*qualified]`` function pointer type, then the  aliased type is a member data
        // pointer to a ``[*qualified function pointer]``.

        {
            using f = int(*&)();
            using g = ct::apply_member_pointer_t<f, foo>;
            using expect = int (* foo::*)();
            static_assert(std::is_same<g, expect>::value, "");
        }

        {
            using f = int(* const)();
            using g = ct::apply_member_pointer_t<f, foo>;
            using expect = int (* const foo::*)();
            static_assert(std::is_same<g, expect>::value, "");
        }
    }

    {
        // function reference type -> member function pointer type
        using f = void(&)();
        using g = ct::apply_member_pointer_t<f, foo>;
        using expect = void(foo::*)();
        static_assert(std::is_same<g, expect>::value, "");
    }

    {
        // member function pointer type -> member function pointer type
        // (note the different parent class)
        using f = int(bar::*)() const;
        using g = ct::apply_member_pointer_t<f, foo>;
        using expect = int(foo::*)() const;
        static_assert(std::is_same<g, expect>::value, "");
    }

    {
        // non-callable type -> member data pointer type
        using g = ct::apply_member_pointer_t<int, foo>;
        using expect = int foo::*;
        static_assert(std::is_same<g, expect>::value, "");
    }

    {
        // function object type -> member data pointer type
        // the same is true for lambdas and generic lambdas
        auto lambda = [](){};
        using f = decltype(lambda);
        using g = ct::apply_member_pointer_t<f, foo>;
        using expect = f foo::*;
        static_assert(std::is_same<g, expect>::value, "");
    }
}
}  // namespace ex_apply_member_pointer

// libs/callable_traits/example/args.cpp
namespace ex_args {
namespace ct = boost::callable_traits;

template<typename T, typename Expect>
void test(){
    using args_t = ct::args_t<T>;
    static_assert(std::is_same<args_t, Expect>::value, "");
}

TEST_CASE("example: args") {

    {
        auto lamda = [](int, float&, const char*){};
        using lam = decltype(lamda);
        using expect = std::tuple<int, float&, const char*>;

        test<lam, expect>();
    }

    {
        struct foo;
        using pmf = void(foo::*)(int, float&, const char*);
        using expect = std::tuple<foo&, int, float&, const char*>;

        test<pmf, expect>();
    }

    {
        using function_ptr = void(*)(int, float&, const char*);
        using expect = std::tuple<int, float&, const char*>;
        test<function_ptr, expect>();
    }

    {
        using function_ref = void(&)(int, float&, const char*);
        using expect = std::tuple<int, float&, const char*>;
        test<function_ref, expect>();
    }

    {
        using function = void(int, float&, const char*);
        using expect = std::tuple<int, float&, const char*>;
        test<function, expect>();
    }

    {
        using abominable = void(int, float&, const char*) const;
        using expect = std::tuple<int, float&, const char*>;
        test<abominable, expect>();
    }
}
}  // namespace ex_args

// libs/callable_traits/example/class_of.cpp
namespace ex_class_of {
namespace ct = boost::callable_traits;

struct foo;

static_assert(std::is_same<foo, ct::class_of_t<int(foo::*)()>>::value, "");
static_assert(std::is_same<foo, ct::class_of_t<int foo::*>>::value, "");

TEST_CASE("example: class_of") {}
}  // namespace ex_class_of

// libs/callable_traits/example/function_type.cpp
namespace ex_function_type {
namespace ct = boost::callable_traits;

template<typename T>
void test(){

    // this example shows how boost::callable_traits::function_type_t
    // bevaves consistently for many different types
    using type = ct::function_type_t<T>;
    using expect = void(int, float&, const char*);
    static_assert(std::is_same<expect, type>{}, "");
}

TEST_CASE("example: function_type") {

    auto lamda = [](int, float&, const char*){};
    using lam = decltype(lamda);
    test<lam>();

    using function_ptr = void(*)(int, float&, const char*);
    test<function_ptr>();

    using function_ref = void(&)(int, float&, const char*);
    test<function_ref>();

    using function = void(int, float&, const char*);
    test<function>();

    using abominable = void(int, float&, const char*) const;
    test<abominable>();
}
}  // namespace ex_function_type

// libs/callable_traits/example/function_types_remove_const_comparison.cpp
namespace ex_function_types_remove_const_comparison {
struct foo {
        void bar() const {}
};

using const_removed = boost::callable_traits::remove_member_const_t<decltype(&foo::bar)>;

static_assert(std::is_same<const_removed, void(foo::*)()>::value, "");

TEST_CASE("example: function_types_remove_const_comparison") {}
}  // namespace ex_function_types_remove_const_comparison

// libs/callable_traits/example/has_member_qualifiers.cpp
namespace ex_has_member_qualifiers {
namespace ct = boost::callable_traits;

struct foo;

static_assert(ct::has_member_qualifiers<int(foo::*)() const>::value, "");
static_assert(ct::has_member_qualifiers<int(foo::*)() volatile>::value, "");
static_assert(!ct::has_member_qualifiers<int(foo::*)()>::value, "");

TEST_CASE("example: has_member_qualifiers") {}
}  // namespace ex_has_member_qualifiers

// libs/callable_traits/example/has_varargs.cpp
namespace ex_has_varargs {
namespace ct = boost::callable_traits;

static_assert(ct::has_varargs<int(...)>::value, "");
static_assert(!ct::has_varargs<int()>::value, "");

TEST_CASE("example: has_varargs") {}
}  // namespace ex_has_varargs

// libs/callable_traits/example/has_void_return.cpp
namespace ex_has_void_return {
namespace ct = boost::callable_traits;

static_assert(ct::has_void_return<void()>::value, "");
static_assert(!ct::has_void_return<int()>::value, "");

TEST_CASE("example: has_void_return") {}
}  // namespace ex_has_void_return

// libs/callable_traits/example/intro.cpp
namespace ex_intro {
namespace ct = boost::callable_traits;

// This function template helps keep our example code neat
template<typename A, typename B>
void assert_same(){ static_assert(std::is_same<A, B>::value, ""); }

// foo is a function object
struct foo {
    void operator()(int, char, float) const {}
};

TEST_CASE("example: intro") {

    // Use args_t to retrieve a parameter list as a std::tuple:
    assert_same<
        ct::args_t<foo>,
        std::tuple<int, char, float>
    >();

    // has_void_return lets us perform a quick check for a void return type
    static_assert(ct::has_void_return<foo>::value, "");

    // Detect C-style variadics (ellipses) in a signature (e.g. printf)
    static_assert(!ct::has_varargs<foo>::value, "");

    // pmf is a pointer-to-member function: void (foo::*)(int, char, float) const
    using pmf = decltype(&foo::operator());

    // remove_member_const_t lets you remove the const member qualifier
    assert_same<
        ct::remove_member_const_t<pmf>,
        void (foo::*)(int, char, float) /*no const!*/
    >();

    // Conversely, add_member_const_t adds a const member qualifier
    assert_same<
        pmf,
        ct::add_member_const_t<void (foo::*)(int, char, float)>
    >();

    // is_const_member_v checks for the presence of member const
    static_assert(ct::is_const_member<pmf>::value, "");
}
}  // namespace ex_intro

// libs/callable_traits/example/is_const_member.cpp
namespace ex_is_const_member {
namespace ct = boost::callable_traits;

struct foo;

static_assert(ct::is_const_member<int(foo::*)() const>::value, "");
static_assert(!ct::is_const_member<int(foo::*)()>::value, "");

TEST_CASE("example: is_const_member") {}
}  // namespace ex_is_const_member

// libs/callable_traits/example/is_cv_member.cpp
namespace ex_is_cv_member {
namespace ct = boost::callable_traits;

struct foo;

static_assert(ct::is_cv_member<int(foo::*)() const volatile>::value, "");
static_assert(!ct::is_cv_member<int(foo::*)()>::value, "");
static_assert(!ct::is_cv_member<int(foo::*)() const>::value, "");
static_assert(!ct::is_cv_member<int(foo::*)() volatile>::value, "");

TEST_CASE("example: is_cv_member") {}
}  // namespace ex_is_cv_member

// libs/callable_traits/example/is_invocable.cpp
namespace ex_is_invocable {
namespace ct = boost::callable_traits;

struct foo {
    template<typename T>
    typename std::enable_if<std::is_integral<T>::value>::type
    operator()(T){}
};

static_assert(ct::is_invocable<foo, int>::value, "");
static_assert(!ct::is_invocable<foo, double>::value, "");

TEST_CASE("example: is_invocable") {}
}  // namespace ex_is_invocable

// libs/callable_traits/example/is_lvalue_reference_member.cpp
namespace ex_is_lvalue_reference_member {
namespace ct = boost::callable_traits;

static_assert(ct::is_lvalue_reference_member<int()&>::value, "");
static_assert(!ct::is_lvalue_reference_member<int()&&>::value, "");
static_assert(!ct::is_lvalue_reference_member<int()>::value, "");

struct foo;

static_assert(ct::is_lvalue_reference_member<int(foo::*)()&>::value, "");
static_assert(!ct::is_lvalue_reference_member<int(foo::*)()&&>::value, "");
static_assert(!ct::is_lvalue_reference_member<int(foo::*)()>::value, "");

TEST_CASE("example: is_lvalue_reference_member") {}
}  // namespace ex_is_lvalue_reference_member

// libs/callable_traits/example/is_noexcept.cpp
namespace ex_is_noexcept {
namespace ct = boost::callable_traits;

struct foo;

static_assert(ct::is_noexcept<int() noexcept>::value, "");
static_assert(ct::is_noexcept<int(*)() noexcept>::value, "");
static_assert(ct::is_noexcept<int(&)() noexcept>::value, "");
static_assert(ct::is_noexcept<int(foo::*)() const noexcept>::value, "");

static_assert(!ct::is_noexcept<int()>::value, "");
static_assert(!ct::is_noexcept<int(*)()>::value, "");
static_assert(!ct::is_noexcept<int(&)()>::value, "");
static_assert(!ct::is_noexcept<int(foo::*)() const>::value, "");

TEST_CASE("example: is_noexcept") {}
}  // namespace ex_is_noexcept

// libs/callable_traits/example/is_reference_member.cpp
namespace ex_is_reference_member {
namespace ct = boost::callable_traits;

static_assert(ct::is_reference_member<int()&>::value, "");
static_assert(ct::is_reference_member<int()&&>::value, "");
static_assert(!ct::is_reference_member<int()>::value, "");

struct foo;

static_assert(ct::is_reference_member<int(foo::*)()&>::value, "");
static_assert(ct::is_reference_member<int(foo::*)()&&>::value, "");
static_assert(!ct::is_reference_member<int(foo::*)()>::value, "");

TEST_CASE("example: is_reference_member") {}
}  // namespace ex_is_reference_member

// libs/callable_traits/example/is_rvalue_reference_member.cpp
namespace ex_is_rvalue_reference_member {
namespace ct = boost::callable_traits;

static_assert(ct::is_rvalue_reference_member<int()&&>::value, "");
static_assert(!ct::is_rvalue_reference_member<int()&>::value, "");
static_assert(!ct::is_rvalue_reference_member<int()>::value, "");

struct foo;

static_assert(ct::is_rvalue_reference_member<int(foo::*)()&&>::value, "");
static_assert(!ct::is_rvalue_reference_member<int(foo::*)()&>::value, "");
static_assert(!ct::is_rvalue_reference_member<int(foo::*)()>::value, "");

TEST_CASE("example: is_rvalue_reference_member") {}
}  // namespace ex_is_rvalue_reference_member

// libs/callable_traits/example/is_volatile_member.cpp
namespace ex_is_volatile_member {
namespace ct = boost::callable_traits;

static_assert(ct::is_volatile_member<int() volatile>::value, "");
static_assert(ct::is_volatile_member<int() const volatile>::value, "");
static_assert(!ct::is_volatile_member<int()>::value, "");

struct foo;

static_assert(ct::is_volatile_member<int(foo::*)() volatile>::value, "");
static_assert(!ct::is_volatile_member<int(foo::*)() const>::value, "");
static_assert(!ct::is_volatile_member<int(foo::*)()>::value, "");

TEST_CASE("example: is_volatile_member") {}
}  // namespace ex_is_volatile_member

// libs/callable_traits/example/overview.cpp
namespace ex_overview {
using std::is_same;
using std::tuple;

using namespace boost::callable_traits;

struct number {
    int value;
    int add(int n) const { return value + n; }
};

using pmf = decltype(&number::add);

//` Manipulate member functions pointers with ease:
static_assert(is_same<
    remove_member_const_t<pmf>,
    int(number::*)(int)
>{}, "");

static_assert(is_same<
    add_member_volatile_t<pmf>,
    int(number::*)(int) const volatile
>{}, "");

static_assert(is_same<
    class_of_t<pmf>,
    number
>{}, "");

//` INVOKE-aware metafunctions:

static_assert(is_same<
    args_t<pmf>,
    tuple<const number&, int>
>{}, "");

static_assert(is_same<
    return_type_t<pmf>,
    int
>{}, "");

static_assert(is_same<
    function_type_t<pmf>,
    int(const number&, int)
>{}, "");

static_assert(is_same<
    qualified_class_of_t<pmf>,
    const number&
>{}, "");

//` Here are a few other trait examples:
static_assert(is_const_member<pmf>{}, "");
static_assert(!is_volatile_member<pmf>{}, "");
static_assert(!has_void_return<pmf>{}, "");
static_assert(!has_varargs<pmf>{}, "");

TEST_CASE("example: overview") {}
}  // namespace ex_overview

// libs/callable_traits/example/qualified_class_of.cpp
namespace ex_qualified_class_of {
namespace ct = boost::callable_traits;

struct foo;

static_assert(std::is_same<foo &,
    ct::qualified_class_of_t<int(foo::*)()>>::value, "");
    
static_assert(std::is_same<foo const &,
    ct::qualified_class_of_t<int(foo::*)() const>>::value, "");

static_assert(std::is_same<foo volatile &&,
    ct::qualified_class_of_t<int(foo::*)() volatile &&>>::value, "");

TEST_CASE("example: qualified_class_of") {}
}  // namespace ex_qualified_class_of

// libs/callable_traits/example/remove_member_const.cpp
namespace ex_remove_member_const {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: remove_member_const") {

    {
        using pmf = int(foo::*)() const;
        using expect = int(foo::*)();
        using test = ct::remove_member_const_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() const &&;
        using expect = int(foo::*)() &&;
        using test = ct::remove_member_const_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() const volatile &;
        using expect = int(foo::*)() volatile &;
        using test = ct::remove_member_const_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using f = int() const;
        using expect = int();
        using test = ct::remove_member_const_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_remove_member_const

// libs/callable_traits/example/remove_member_cv.cpp
namespace ex_remove_member_cv {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: remove_member_cv") {

    {
        using pmf = int(foo::*)() const volatile;
        using expect = int(foo::*)();
        using test = ct::remove_member_cv_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() const &&;
        using expect = int(foo::*)() &&;
        using test = ct::remove_member_cv_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() const volatile &;
        using expect = int(foo::*)() &;
        using test = ct::remove_member_cv_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using f = int() const volatile;
        using expect = int();
        using test = ct::remove_member_cv_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_remove_member_cv

// libs/callable_traits/example/remove_member_reference.cpp
namespace ex_remove_member_reference {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: remove_member_reference") {

    {
        using pmf = int(foo::*)() &;
        using expect = int(foo::*)();
        using test = ct::remove_member_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() const &&;
        using expect = int(foo::*)() const;
        using test = ct::remove_member_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() const volatile &;
        using expect = int(foo::*)() const volatile;
        using test = ct::remove_member_reference_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using f = int() &&;
        using expect = int();
        using test = ct::remove_member_reference_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_remove_member_reference

// libs/callable_traits/example/remove_member_volatile.cpp
namespace ex_remove_member_volatile {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: remove_member_volatile") {

    {
        using pmf = int(foo::*)() const volatile;
        using expect = int(foo::*)() const;
        using test = ct::remove_member_volatile_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() volatile &&;
        using expect = int(foo::*)() &&;
        using test = ct::remove_member_volatile_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = int(foo::*)() volatile &;
        using expect = int(foo::*)() &;
        using test = ct::remove_member_volatile_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using f = int() const volatile;
        using expect = int() const;
        using test = ct::remove_member_volatile_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_remove_member_volatile

// libs/callable_traits/example/remove_noexcept.cpp
namespace ex_remove_noexcept {
using boost::callable_traits::remove_noexcept_t;

static_assert(std::is_same<
    remove_noexcept_t<int() noexcept>,
    int()
>{}, "");

TEST_CASE("example: remove_noexcept") {}
}  // namespace ex_remove_noexcept

// libs/callable_traits/example/remove_varargs.cpp
namespace ex_remove_varargs {
namespace ct = boost::callable_traits;

struct foo {};

TEST_CASE("example: remove_varargs") {

    {
        using f = void(int, ...);
        using expect = void(int);
        using test = ct::remove_varargs_t<f>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using fp = void(*)(...);
        using expect = void(*)();
        using test = ct::remove_varargs_t<fp>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using fr = void(&)(const char*, ...);
        using expect = void(&)(const char*);
        using test = ct::remove_varargs_t<fr>;
        static_assert(std::is_same<test, expect>::value, "");
    } {
        using pmf = void(foo::*)(...) const;
        using expect = void(foo::*)() const;
        using test = ct::remove_varargs_t<pmf>;
        static_assert(std::is_same<test, expect>::value, "");
    }
}
}  // namespace ex_remove_varargs

// libs/callable_traits/example/return_type.cpp
namespace ex_return_type {
namespace ct = boost::callable_traits;

using expect = int;

struct foo;

template<typename T>
void test() {
    using result = ct::return_type_t<T>;
    static_assert(std::is_same<expect, result>{}, "");
}

TEST_CASE("example: return_type") {

    test<int()>();
    test<int(*)()>();
    test<int(&)()>();
    test<int() const>();
    test<int(foo::*)() const>();

    auto x = []() -> int { return 0; };

    test<decltype(x)>();
}
}  // namespace ex_return_type

// libs/callable_traits/example/void.cpp
namespace ex_void {
struct foo {
    void operator()() const {}
};

namespace ct = boost::callable_traits;

TEST_CASE("example: void") {

    using args = ct::args_t<foo>;
    using expected_args = std::tuple<>;
    static_assert(std::is_same<args, expected_args>{}, "");

    using signature = ct::function_type_t<foo>;
    using expected_signature = void();
    static_assert(std::is_same<signature, expected_signature>{}, "");

}
}  // namespace ex_void
