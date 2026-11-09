// boost.describe with the json module — libs/describe/example/{console,
// from_json,json_rpc,to_json}.cpp: the BOOST_DESCRIBE_* macros come from
// #include (no module can carry them) and Boost.JSON from its module, whose
// describe-based conversions see the described types (design §4.4: the
// includes precede every import). Each program runs as a case; its output
// must equal what the original printed (built from upstream/boost with
// boost/json/src.hpp).
// console.cpp reads a fixed stdin, and its endless loop stops at the end of
// input (an added 'if (!std::cin) break;', also in the oracle's copy).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/describe.hpp>
#include <boost/version.hpp>
import std;
import mcpplibs.mwrap.boost.json;
import mcpplibs.mwrap.boost.mp11;
import mcpplibs.mwrap.boost.variant2;
import mcpplibs.mwrap.boost.utility;
import mcpplibs.mwrap.boost.type_traits;
import mcpplibs.mwrap.boost.core;

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

// libs/describe/example/console.cpp
namespace ex_console {
#if defined(_MSC_VER)
# pragma warning(disable: 4702) // unreachable code
#endif


// get variable

template<class Scope> boost::json::value get( Scope& scope, boost::string_view name )
{
    using Md = boost::describe::describe_members<Scope, boost::describe::mod_public>;

    bool found = false;
    boost::json::value result;

    boost::mp11::mp_for_each<Md>([&](auto D) {

        if( !found && name == D.name )
        {
            result = boost::json::value_from( scope.*D.pointer );
            found = true;
        }

    });

    if( !found )
    {
        throw std::invalid_argument(
            std::string( "'" ) + std::string( name ) + "': no such variable" );
    }

    return result;
}

// set variable

template<class T> void set_impl( T & t, boost::string_view /*name*/, boost::json::value const& value )
{
    t = boost::json::value_to<T>( value );
}

template<class T> void set_impl( T const & /*t*/, boost::string_view name, boost::json::value const& /*value*/ )
{
    throw std::invalid_argument(
        std::string( "'" ) + std::string( name ) + "': variable cannot be modified" );
}

template<class Scope> void set( Scope& scope, boost::string_view name, boost::json::value const& value )
{
    using Md = boost::describe::describe_members<Scope, boost::describe::mod_public>;

    bool found = false;

    boost::mp11::mp_for_each<Md>([&](auto D) {

        if( !found && name == D.name )
        {
            set_impl( scope.*D.pointer, name, value );
            found = true;
        }

    });

    if( !found )
    {
        throw std::invalid_argument(
            std::string( "'" ) + std::string( name ) + "': no such variable" );
    }
}

//

struct globals
{
    std::string const help = "Enter a variable name ('x', 'y', 'v', or 'm') to print its value; enter variable=value to assign a new value to a variable. Values are in JSON format.";

    int x = 1;

    double y = 3.14;

    std::vector<int> v{ 1, 2, 3 };

    std::map<std::string, double> m{ { "BTC", 44898.68 }, { "ETH", 1386.57 } };
};

BOOST_DESCRIBE_STRUCT( globals, (), (help, x, y, v, m) )

int example_main()
{
    globals g_;

    for( ;; )
    {
        ex_out << "\n> ";

        std::string line;
        std::getline( ex_in, line );
        if( !ex_in ) break; // boost-m port: stop at the end of input

        try
        {
            std::size_t i = line.find( '=' );

            if( i != std::string::npos )
            {
                set( g_, line.substr( 0, i ), boost::json::parse( line.substr( i + 1 ) ) );
            }
            else
            {
                ex_out << get( g_, line ) << std::endl;
            }
        }
        catch( std::exception const& x )
        {
            ex_out << "Error: " << x.what() << std::endl;
        }
    }
    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: console") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(x
x=42
x
v=[4,5]
v
m
help=1
zzz
y=2.5
y
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(
> 1

> 
> 42

> 
> [4,5]

> {"BTC":4.489868E4,"ETH":1.38657E3}

> Error: 'help': variable cannot be modified

> Error: 'zzz': no such variable

> 
> 2.5E0

> )ex"));
}
}  // namespace ex_console

// libs/describe/example/from_json.cpp
namespace ex_from_json {
namespace app
{

#if BOOST_VERSION < 108100

template<class T> void extract( boost::json::object const & obj, char const * name, T & value )
{
    value = boost::json::value_to<T>( obj.at( name ) );
}

template<class T,
    class D1 = boost::describe::describe_members<T,
        boost::describe::mod_public | boost::describe::mod_protected>,
    class D2 = boost::describe::describe_members<T, boost::describe::mod_private>,
    class En = std::enable_if_t<boost::mp11::mp_empty<D2>::value && !std::is_union<T>::value> >

    T tag_invoke( boost::json::value_to_tag<T> const&, boost::json::value const& v )
{
    auto const& obj = v.as_object();

    T t{};

    boost::mp11::mp_for_each<D1>([&](auto D){

        extract( obj, D.name, t.*D.pointer );

    });

    return t;
}

#endif

struct A
{
    int x;
    int y;
};

BOOST_DESCRIBE_STRUCT(A, (), (x, y))

} // namespace app


int example_main()
{
    boost::json::value jv{ { "x", 1 }, { "y", 2 } };

    ex_out << "jv: " << jv << std::endl;

    auto a = boost::json::value_to<app::A>( jv );

    ex_out << "a: { " << a.x << ", " << a.y << " }" << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: from_json") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(jv: {"x":1,"y":2}
a: { 1, 2 }
)ex"));
}
}  // namespace ex_from_json

// libs/describe/example/json_rpc.cpp
namespace ex_json_rpc {
template<class C1, class C2, class R, class... A, std::size_t... I>
  boost::json::value
    call_impl_( C1 & c1, R (C2::*pmf)(A...), boost::json::array const & args,
      std::index_sequence<I...> )
{
    return boost::json::value_from(
      (c1.*pmf)(
        boost::json::value_to< boost::remove_cv_ref_t<A> >( args[ I ] )... ) );
}

template<class C1, class C2, class R, class... A>
  boost::json::value
    call_impl( C1 & c1, R (C2::*pmf)(A...), boost::json::array const & args )
{
    if( args.size() != sizeof...(A) )
    {
        throw std::invalid_argument( "Invalid number of arguments" );
    }

    return call_impl_( c1, pmf, args, std::index_sequence_for<A...>() );
}

template<class C>
  boost::json::value
    call( C & c, boost::string_view method, boost::json::array const & args )
{
    using Fd = boost::describe::describe_members<C,
        boost::describe::mod_public | boost::describe::mod_function>;

    bool found = false;
    boost::json::value result;

    boost::mp11::mp_for_each<Fd>([&](auto D){

        if( !found && method == D.name)
        {
            result = call_impl( c, D.pointer, args );
            found = true;
        }

    });

    if( !found )
    {
        throw std::invalid_argument( "Invalid method name" );
    }

    return result;
}

struct Object
{
    std::string greet( std::string const & who )
    {
        return "Hello, " + who + "!";
    }

    int add( int x, int y )
    {
        return x + y;
    }
};

BOOST_DESCRIBE_STRUCT(Object, (), (greet, add))


int example_main()
{
    Object obj;
    ex_out << call( obj, "greet", { "world" } ) << std::endl;
    ex_out << call( obj, "add", { 1, 2 } ) << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: json_rpc") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex("Hello, world!"
3
)ex"));
}
}  // namespace ex_json_rpc

// libs/describe/example/to_json.cpp
namespace ex_to_json {
namespace app
{

#if BOOST_VERSION < 108100

template<class T,
    class D1 = boost::describe::describe_members<T,
        boost::describe::mod_public | boost::describe::mod_protected>,
    class D2 = boost::describe::describe_members<T, boost::describe::mod_private>,
    class En = std::enable_if_t<boost::mp11::mp_empty<D2>::value && !std::is_union<T>::value> >

    void tag_invoke( boost::json::value_from_tag const&, boost::json::value& v, T const & t )
{
    auto& obj = v.emplace_object();

    boost::mp11::mp_for_each<D1>([&](auto D){

        obj[ D.name ] = boost::json::value_from( t.*D.pointer );

    });
}

#endif

struct A
{
    int x;
    int y;
};

BOOST_DESCRIBE_STRUCT(A, (), (x, y))

struct B
{
    std::vector<A> v;
    std::map<std::string, A> m;
};

BOOST_DESCRIBE_STRUCT(B, (), (v, m))

} // namespace app


int example_main()
{
    app::B b{ { { 1, 2 }, { 3, 4 } }, { { "k1", { 5, 6 } }, { "k2", { 7, 8 } } } };

    ex_out << boost::json::value_from( b ) << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: to_json") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex({"v":[{"x":1,"y":2},{"x":3,"y":4}],"m":{"k1":{"x":5,"y":6},"k2":{"x":7,"y":8}}}
)ex"));
}
}  // namespace ex_to_json
