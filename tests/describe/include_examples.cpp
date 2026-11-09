// boost.describe — the official examples (libs/describe/example/*.cpp) that use
// describe with header-only companions (mp11, container_hash, core,
// type_traits, variant2, utility): describe has no module (its API is the
// BOOST_DESCRIBE_* macros, docs/usage.md §2.3), so this TU includes and does
// not import. Each program runs as a case; its output must equal what the
// same code printed when built from upstream/boost headers alone. The examples that
// need a compiled library (json, serialization) are examples_json.cpp and
// examples_serialization.cpp; the fmtlib ones are excluded (fmt is not
// vendored).
// pm_to_string.cpp names its own ::detail namespace; in the port it is the
// case namespace's detail.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <iostream>
#include <boost/variant2/variant.hpp>
#include <vector>
#include <boost/container_hash/hash.hpp>
#include <boost/version.hpp>
#include <boost/core/type_name.hpp>
#include <string>
#include <ostream>
#include <array>
#include <stdexcept>
#include <typeinfo>
#include <cstring>
#include <tuple>
#include <sstream>
#include <string>
#include <string_view>

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline char ex_arg0[] = "example";
inline char* ex_argv[] = {ex_arg0, nullptr};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// libs/describe/example/enum_to_string.cpp
namespace ex_enum_to_string {
template<class E> char const * enum_to_string( E e )
{
    char const * r = "(unnamed)";

    boost::mp11::mp_for_each< boost::describe::describe_enumerators<E> >([&](auto D){

        if( e == D.value ) r = D.name;

    });

    return r;
}


enum E
{
    v1 = 3,
    v2,
    v3 = 11
};

BOOST_DESCRIBE_ENUM(E, v1, v2, v3)

int example_main()
{
    ex_out << "E(" << v1 << "): " << enum_to_string( v1 ) << std::endl;
    ex_out << "E(" << 0 << "): " << enum_to_string( E(0) ) << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: enum_to_string") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(E(3): v1
E(0): (unnamed)
)ex"));
}
}  // namespace ex_enum_to_string

// libs/describe/example/equality.cpp
namespace ex_equality {
using namespace boost::describe;

namespace app
{
template<class T,
    class Bd = describe_bases<T, mod_any_access>,
    class Md = describe_members<T, mod_any_access>,
    class En = std::enable_if_t<!std::is_union<T>::value> >
    bool operator==( T const& t1, T const& t2 )
{
    bool r = true;

    boost::mp11::mp_for_each<Bd>([&](auto D){

        using B = typename decltype(D)::type;
        r = r && (B const&)t1 == (B const&)t2;

    });

    boost::mp11::mp_for_each<Md>([&](auto D){

        r = r && t1.*D.pointer == t2.*D.pointer;

    });

    return r;
}

struct A
{
    int x = 1;
};

BOOST_DESCRIBE_STRUCT(A, (), (x))

struct B
{
    int y = 2;
};

BOOST_DESCRIBE_STRUCT(B, (), (y))

struct C
{
    std::vector<boost::variant2::variant<A, B>> v;
};

BOOST_DESCRIBE_STRUCT(C, (), (v))

} // namespace app


int example_main()
{
    app::C c1, c2, c3;

    c1.v.push_back( app::A{} );
    c2.v.push_back( app::A{} );
    c3.v.push_back( app::B{} );

    ex_out << std::boolalpha
        << ( c1 == c2 ) << ' '
        << ( c1 == c3 ) << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: equality") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(true false
)ex"));
}
}  // namespace ex_equality

// libs/describe/example/hash_value.cpp
namespace ex_hash_value {
using namespace boost::describe;

namespace app
{

#if BOOST_VERSION < 108100

template<class T,
    class Bd = describe_bases<T, mod_any_access>,
    class Md = describe_members<T, mod_any_access>,
    class En = std::enable_if_t<!std::is_union<T>::value> >
    std::size_t hash_value( T const & t )
{
    std::size_t r = 0;

    boost::mp11::mp_for_each<Bd>([&](auto D){

        using B = typename decltype(D)::type;
        boost::hash_combine( r, (B const&)t );

    });

    boost::mp11::mp_for_each<Md>([&](auto D){

        boost::hash_combine( r, t.*D.pointer );

    });

    return r;
}

#endif

struct A
{
    int x = 1;
};

BOOST_DESCRIBE_STRUCT(A, (), (x))

struct B
{
    int y = 2;
};

BOOST_DESCRIBE_STRUCT(B, (), (y))

struct C
{
    std::vector<boost::variant2::variant<A, B>> v;
};

BOOST_DESCRIBE_STRUCT(C, (), (v))

} // namespace app


int example_main()
{
    app::C c;

    c.v.push_back( app::A{} );
    c.v.push_back( app::B{} );

    ex_out << boost::hash<app::C>()( c ) << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: hash_value") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(14636833987593068677
)ex"));
}
}  // namespace ex_hash_value

// libs/describe/example/pm_to_string.cpp
namespace ex_pm_to_string {
#if defined(__GNUC__) && __GNUC__ == 12
# pragma GCC diagnostic ignored "-Wrestrict" // false positive
#endif


namespace detail
{

template<class T1, class T2>
constexpr auto equals( T1 t1, T2 t2, int ) -> decltype( t1 == t2 )
{
    return t1 == t2;
}

template<class T1, class T2>
constexpr bool equals( T1 /*t1*/, T2 /*t2*/, long )
{
    return false;
}

using namespace boost::describe;

template<class T, class C,
    class L = describe_members<C, mod_any_access | mod_any_member>>
char const* get_member_name( T C::* pm, int )
{
    char const * name = nullptr;

    boost::mp11::mp_for_each<L>([&](auto D){

        if( equals( D.pointer, pm, 0 ) ) name = D.name;

    });

    return name;
}

template<class T, class C>
char const* get_member_name( T C::* /*pm*/, long )
{
    return nullptr;
}

} // namespace detail

template<class T, class C>
std::string pm_to_string( T C::* pm )
{
    char const * name = detail::get_member_name( pm, 0 );

    if( name == nullptr ) name = "(unknown)";

    return "&" + boost::core::type_name<C>() + "::" + name
        + " [" + boost::core::type_name<T>() + "]";
}

template<class T, class C>
std::ostream& operator<<( std::ostream& os, T C::* pm )
{
    os << pm_to_string( pm );
    return os;
}

struct X
{
    int m;
    int f() const { return m; }
};

BOOST_DESCRIBE_STRUCT(X, (), (m, f))

struct Y: public X
{
    int m;
    int g() const { return -m; }
};

BOOST_DESCRIBE_STRUCT(Y, (X), (m, g))

struct Z
{
    void h() {}
};

#if !defined(_MSC_VER) || defined(__clang__)

// MSVC doesn't support BOOST_DESCRIBE_CLASS inside
// templates until 2022 in C++20 mode

template<class T1, class T2> struct pair
{
    T1 first;
    T2 second;

    BOOST_DESCRIBE_CLASS(pair, (), (first, second), (), ())
};

#endif


int example_main()
{
    ex_out << &X::m << std::endl;
    ex_out << &X::f << std::endl;

    ex_out << &Y::m << std::endl;
    ex_out << &Y::f << std::endl;
    ex_out << &Y::g << std::endl;

    ex_out << &Z::h << std::endl;

#if !defined(_MSC_VER) || defined(__clang__)
    ex_out << &pair<int, float>::second << std::endl;
#endif
    return 0;  // main's implicit return
}

TEST_CASE("example: pm_to_string") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(&ex_pm_to_string::X::m [int]
&ex_pm_to_string::X::f [int() const]
&ex_pm_to_string::Y::m [int]
&ex_pm_to_string::X::f [int() const]
&ex_pm_to_string::Y::g [int() const]
&ex_pm_to_string::Z::(unknown) [void()]
&ex_pm_to_string::pair<int, float>::second [float]
)ex"));
}
}  // namespace ex_pm_to_string

// libs/describe/example/print_function.cpp
namespace ex_print_function {
using namespace boost::describe;

template<class T,
    class Bd = describe_bases<T, mod_any_access>,
    class Md = describe_members<T, mod_any_access>,
    class En = std::enable_if_t<!std::is_union<T>::value> >
    std::ostream& operator<<( std::ostream & os, T const & t )
{
    os << "{";

    bool first = true;

    boost::mp11::mp_for_each<Bd>([&](auto D){

        if( !first ) { os << ", "; } first = false;

        using B = typename decltype(D)::type;
        os << (B const&)t;

    });

    boost::mp11::mp_for_each<Md>([&](auto D){

        if( !first ) { os << ", "; } first = false;

        os << "." << D.name << " = " << t.*D.pointer;

    });

    os << "}";
    return os;
}

struct X
{
    int m1 = 1;
};

BOOST_DESCRIBE_STRUCT(X, (), (m1))

struct Y
{
    int m2 = 2;
};

BOOST_DESCRIBE_STRUCT(Y, (), (m2))

class Z: public X, private Y
{
    int m1 = 3;
    int m2 = 4;

    BOOST_DESCRIBE_CLASS(Z, (X, Y), (), (), (m1, m2))
};


int example_main()
{
    ex_out << Z() << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: print_function") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex({{.m1 = 1}, {.m2 = 2}, .m1 = 3, .m2 = 4}
)ex"));
}
}  // namespace ex_print_function

// libs/describe/example/printing_enums_ct.cpp
namespace ex_printing_enums_ct {
enum E
{
    v1 = 11,
    v2,
    v3 = 5
};

BOOST_DESCRIBE_ENUM(E, v1, v2, v3)

int example_main()
{
    boost::mp11::mp_for_each< boost::describe::describe_enumerators<E> >([](auto D){

        ex_printf( "%s: %d\n", D.name, D.value );

    });
    return 0;  // main's implicit return
}

TEST_CASE("example: printing_enums_ct") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(v1: 11
v2: 12
v3: 5
)ex"));
}
}  // namespace ex_printing_enums_ct

// libs/describe/example/printing_enums_rt.cpp
namespace ex_printing_enums_rt {
template<class E> struct enum_descriptor
{
    E value;
    char const * name;
};

template<class E, template<class... T> class L, class... T>
  constexpr std::array<enum_descriptor<E>, sizeof...(T)>
    describe_enumerators_as_array_impl( L<T...> )
{
    return { { { T::value, T::name }... } };
}

template<class E> constexpr auto describe_enumerators_as_array()
{
    return describe_enumerators_as_array_impl<E>( boost::describe::describe_enumerators<E>() );
}

BOOST_DEFINE_ENUM(E, v1, v2, v3, v4, v5, v6)

int example_main()
{
    constexpr auto D = describe_enumerators_as_array<E>();

    for( auto const& x: D )
    {
        ex_printf( "%s: %d\n", x.name, x.value );
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: printing_enums_rt") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(v1: 0
v2: 1
v3: 2
v4: 3
v5: 4
v6: 5
)ex"));
}
}  // namespace ex_printing_enums_rt

// libs/describe/example/string_to_enum.cpp
namespace ex_string_to_enum {
[[noreturn]] void throw_invalid_name( char const * name, char const * type )
{
    throw std::runtime_error(
        std::string( "Invalid enumerator name '" ) + name
        + "' for enum type '" + type + "'" );
}

template<class E> E string_to_enum( char const * name )
{
    bool found = false;
    E r = {};

    boost::mp11::mp_for_each< boost::describe::describe_enumerators<E> >([&](auto D){

        if( !found && std::strcmp( D.name, name ) == 0 )
        {
            found = true;
            r = D.value;
        }

    });

    if( found )
    {
        return r;
    }
    else
    {
        throw_invalid_name( name, typeid( E ).name() );
    }
}


BOOST_DEFINE_ENUM(E, v1, v2, v3)

int example_main()
{
    try
    {
        ex_out << "v1: " << string_to_enum<E>( "v1" ) << std::endl;
        ex_out << "v2: " << string_to_enum<E>( "v2" ) << std::endl;
        ex_out << "v3: " << string_to_enum<E>( "v3" ) << std::endl;
        ex_out << "v4: " << string_to_enum<E>( "v4" ) << std::endl;
    }
    catch( std::exception const & x )
    {
        ex_out << x.what() << std::endl;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: string_to_enum") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(v1: 0
v2: 1
v3: 2
v4: Invalid enumerator name 'v4' for enum type 'N17ex_string_to_enum1EE'
)ex"));
}
}  // namespace ex_string_to_enum

// libs/describe/example/struct_to_tuple.cpp
namespace ex_struct_to_tuple {
namespace desc = boost::describe;

template<class T, template<class...> class L, class... D>
auto struct_to_tuple_impl( T const& t, L<D...> )
{
    return std::make_tuple( t.*D::pointer... );
}

template<class T,
    class Dm = desc::describe_members<T,
        desc::mod_public | desc::mod_inherited>,
    class En = std::enable_if_t<!std::is_union<T>::value> >
auto struct_to_tuple( T const& t )
{
    return struct_to_tuple_impl( t, Dm() );
}


struct X
{
    int a = 1;
};

BOOST_DESCRIBE_STRUCT(X, (), (a))

struct Y
{
    float b = 3.14f;
};

BOOST_DESCRIBE_STRUCT(Y, (), (b))

struct Z: X, Y
{
};

BOOST_DESCRIBE_STRUCT(Z, (X, Y), ())

int example_main()
{
    Z z;

    auto tp = struct_to_tuple( z );

    ex_out <<
        boost::core::type_name<decltype(tp)>() << ": "
        << std::get<0>(tp) << ", " << std::get<1>(tp);
    return 0;  // main's implicit return
}

TEST_CASE("example: struct_to_tuple") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(std::tuple<int, float>: 1, 3.14)ex"));
}
}  // namespace ex_struct_to_tuple
