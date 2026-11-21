// mcpplibs.mwrap.boost.json — the official examples (libs/json/example/*.cpp)
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone
// (with the json TUs). pretty, validate and cbor read a JSON file named on
// their command line (a fixture in the temporary directory; validate also a
// malformed one); the example's file.hpp is inlined; cbor's
// BOOST_JSON_UNREACHABLE() (an internal macro) is std::unreachable(). parse_into_canada and
// parse_into_citm_catalog and parse_into are timing benchmarks over data files the release
// does not ship (excluded in coverage.toml).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cassert>
#include <boost/config.hpp>
import std;
import mcpplibs.mwrap.boost.json;
import mcpplibs.mwrap.boost.container;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
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

// libs/json/example/cbor.cpp
namespace ex_cbor {
using namespace boost::json;

void serialize_cbor_number(
    unsigned char mt, std::uint64_t n, std::vector<unsigned char> & out )
{
    mt <<= 5;

    if( n < 24 )
    {
        out.push_back( static_cast<unsigned char>( mt + n ) );
    }
    else if( n < 256 )
    {
        unsigned char data[] = { static_cast<unsigned char>( mt + 24 ), static_cast<unsigned char>( n ) };
        out.insert( out.end(), std::begin( data ), std::end( data ) );
    }
    else if( n < 65536 )
    {
        unsigned char data[] = { static_cast<unsigned char>( mt + 25 ), static_cast<unsigned char>( n >> 8 ), static_cast<unsigned char>( n ) };
        out.insert( out.end(), std::begin( data ), std::end( data ) );
    }
    else if( n < 0x1000000ull )
    {
        unsigned char data[ 5 ];

        data[ 0 ] = static_cast<unsigned char>( mt + 26 );
        boost::endian::endian_store<std::uint32_t, 4, boost::endian::order::big>( data + 1, static_cast<std::uint32_t>( n ) );

        out.insert( out.end(), std::begin( data ), std::end( data ) );
    }
    else
    {
        unsigned char data[ 9 ];

        data[ 0 ] = static_cast<unsigned char>( mt + 27 );
        boost::endian::endian_store<std::uint64_t, 8, boost::endian::order::big>( data + 1, n );

        out.insert( out.end(), std::begin( data ), std::end( data ) );
    }
}

void
serialize_cbor_string( string_view sv, std::vector<unsigned char>& out )
{
    std::size_t n = sv.size();
    serialize_cbor_number( 3, n, out );

    out.insert( out.end(), sv.data(), sv.data() + n );
}

void
serialize_cbor_value( const value& jv, std::vector<unsigned char>& out )
{
    switch( jv.kind() )
    {
    case kind::null:
        out.push_back( 224 + 22 );
        break;

    case kind::bool_:
        out.push_back( 224 + 20 + jv.get_bool() );
        break;

    case kind::int64:
        {
            std::int64_t n = jv.get_int64();
            if( n >= 0 )
                serialize_cbor_number( 0, n, out );
            else
                serialize_cbor_number( 1, ~n, out );
        }
        break;

    case kind::uint64:
        serialize_cbor_number( 0, jv.get_uint64(), out );
        break;

    case kind::double_:
        {
            unsigned char data[ 9 ];
            data[ 0 ] = 224 + 27;
            boost::endian::endian_store<double, 8, boost::endian::order::big>( data + 1, jv.get_double() );

            out.insert( out.end(), std::begin(data), std::end(data) );
        }
        break;

    case kind::string:
        serialize_cbor_string( jv.get_string(), out );
        break;

    case kind::array:
        {
            const array& ja = jv.get_array();
            std::size_t n = ja.size();

            out.reserve( out.size() + n + 1 );

            serialize_cbor_number( 4, n, out );

            for( std::size_t i = 0; i < n; ++i )
                serialize_cbor_value( ja[i], out );
        }
        break;

    case kind::object:
        {
            const object& jo = jv.get_object();
            std::size_t n = jo.size();

            out.reserve( out.size() + 3 * n + 1 );

            serialize_cbor_number( 5, n, out );

            for( const key_value_pair& kv: jo )
            {
                serialize_cbor_string( kv.key(), out );
                serialize_cbor_value( kv.value(), out );
            }
        }
        break;
    }
}

BOOST_NORETURN
void
throw_eof_error()
{
    throw std::runtime_error( "Unexpected end of input" );
}

BOOST_NORETURN
void
throw_format_error( char const * err )
{
    throw std::runtime_error( err );
}

void
ensure( std::size_t n, const unsigned char* first, const unsigned char* last )
{
    if( static_cast<std::size_t>(last - first) < n )
        throw_eof_error();
}

const unsigned char*
parse_cbor_value(
    const unsigned char* first, const unsigned char* last, value& v );

const unsigned char*
parse_cbor_number(
    const unsigned char* first, const unsigned char* last, unsigned char ch, std::uint64_t& n )
{
    unsigned char cv = ch & 31;

    if( cv < 24 )
    {
        n = cv;
    }
    else if( cv == 24 )
    {
        ensure( 1, first, last );
        n = *first++;
    }
    else if( cv == 25 )
    {
        ensure( 2, first, last );
        n = boost::endian::load_big_u16( first );
        first += 2;
    }
    else if( cv == 26 )
    {
        ensure( 4, first, last );
        n = boost::endian::load_big_u32( first );
        first += 4;
    }
    else if( cv == 27 )
    {
        ensure( 8, first, last );
        n = boost::endian::load_big_u64( first );
        first += 8;
    }
    else if( cv == 31 )
    {
        // infinite array/object
        throw_format_error( "Infinite sequences aren't supported" );
    }
    else
    {
        throw_format_error( "Invalid minor type" );
    }

    return first;
}

const unsigned char*
parse_cbor_string(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    std::uint64_t n;
    first = parse_cbor_number( first, last, ch, n );

    ensure( n, first, last );

    string_view sv( reinterpret_cast<char const*>( first ), n );
    first += n;

    v = sv;
    return first;
}

const unsigned char*
parse_cbor_array(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    std::uint64_t n;
    first = parse_cbor_number( first, last, ch, n );

    array & a = v.emplace_array();

    a.resize( n );

    std::size_t i = 0;

    for( ; i < n; ++i ) // double[] fast path
    {
        ensure( 1, first, last );
        unsigned char ch2 = *first;

        if( ch2 != 0xFB ) break;

        ++first;
        ensure( 8, first, last );

        double w = boost::endian::endian_load<double, 8, boost::endian::order::big>( first );
        first += 8;

        a[ i ] = w;
    }

    for( ; i < n; ++i ) // int[] fast path
    {
        ensure( 1, first, last );
        unsigned char ch2 = *first;

        if( ch2 >= 0x40 ) break;

        ++first;

        std::uint64_t m;
        first = parse_cbor_number( first, last, ch2, m );

        if( ch2 < 0x20 )
        {
            a[ i ] = m;
        }
        else
        {
            a[ i ] = static_cast<std::int64_t>( ~m );
        }
    }

    for( ; i < n; ++i )
        first = parse_cbor_value( first, last, a[ i ] );

    return first;
}

const unsigned char*
parse_cbor_object(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    std::uint64_t n;
    first = parse_cbor_number( first, last, ch, n );

    object & o = v.emplace_object();

    o.reserve( n );

    for( std::size_t i = 0; i < n; ++i )
    {
        // key string

        ensure( 1, first, last );
        unsigned char ch2 = *first++;

        if( ( ch2 >> 5 ) != 3 )
            throw_format_error( "Object keys must be strings" );

        std::uint64_t m;
        first = parse_cbor_number( first, last, ch2, m );

        ensure( m, first, last );

        string_view sv( reinterpret_cast<char const*>( first ), m );
        first += m;

        // value

        first = parse_cbor_value( first, last, o[ sv ] );
    }

    return first;
}

const unsigned char*
parse_cbor_unsigned(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    std::uint64_t n;
    first = parse_cbor_number( first, last, ch, n );

    v = n;
    return first;
}

const unsigned char*
parse_cbor_signed(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    std::uint64_t n;
    first = parse_cbor_number( first, last, ch, n );

    v = static_cast<std::int64_t>( ~n );
    return first;
}

const unsigned char*
parse_cbor_semantic_tag(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    std::uint64_t n;
    first = parse_cbor_number( first, last, ch, n );

    // ignore semantic tags

    return parse_cbor_value( first, last, v );
}

const unsigned char*
parse_cbor_type7(
    const unsigned char* first, const unsigned char* last, unsigned char ch, value& v )
{
    switch( ch & 31 )
    {
    case 20:
        v = false;
        return first;

    case 21:
        v = true;
        return first;

    case 22:
        v = nullptr;
        return first;

    case 26: // float
    {
        ensure( 4, first, last );

        float w = boost::endian::endian_load<float, 4, boost::endian::order::big>( first );
        first += 4;

        v = w;
        return first;
    }

    case 27: // double
    {
        ensure( 8, first, last );

        double w = boost::endian::endian_load<double, 8, boost::endian::order::big>( first );
        first += 8;

        v = w;
        return first;
    }

    default:
        throw_format_error( "Invalid minor type for major type 7" );
    }
}

const unsigned char*
parse_cbor_value( const unsigned char* first, const unsigned char* last, value& v )
{
    ensure( 1, first, last );
    const unsigned char ch = *first++;

    switch( ch >> 5 )
    {
    case 0:
        return parse_cbor_unsigned( first, last, ch, v );

    case 1:
        return parse_cbor_signed( first, last, ch, v );

    case 2:
        throw_format_error( "Binary strings aren't supported" );

    case 3:
        return parse_cbor_string( first, last, ch, v );

    case 4:
        return parse_cbor_array( first, last, ch, v );

    case 5:
        return parse_cbor_object( first, last, ch, v );

    case 6:
        return parse_cbor_semantic_tag( first, last, ch, v );

    case 7:
        return parse_cbor_type7( first, last, ch, v );

    default:
        std::unreachable();
    }
}

int example_main(int argc, const char** argv)
{
    if( argc != 2 )
    {
        ex_out << "Usage: cbor FILE_NAME\n";
        return EXIT_FAILURE;
    }

    std::ifstream is(argv[1]);
    is.exceptions(std::ios::badbit);

    const value jv = parse(is);

    std::vector<unsigned char> out;
    serialize_cbor_value( jv, out );

    value jv2;
    parse_cbor_value( out.data(), out.data() + out.size(), jv2 );

    if( jv != jv2 )
    {
        ex_out << "Roundtrip check failed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

// end::example_cbor[]

TEST_CASE("example: cbor") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-json-sample.json"); f << R"j({"name":"boost","version":[1,92,0],"modules":true,"ratio":0.25,"tags":["a","b"],"nested":{"k":null}})j"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-json-sample.json"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_cbor

// libs/json/example/path.cpp
namespace ex_path {
using namespace boost::json;

value&
path( value& jv, std::size_t index)
{
    array *arr;
    if(jv.is_null())
        arr = &jv.emplace_array();
    else
        arr = &jv.as_array();
    if(arr->size() <= index)
        arr->resize(index + 1);
    return (*arr)[index];
}

value&
path( value& jv, string_view key )
{
    object* obj;
    if(jv.is_null())
        obj = &jv.emplace_object();
    else
        obj = &jv.as_object();
    return (*obj)[key];
}

template<class Arg0, class Arg1, class... Args>
value&
path( value& jv, Arg0 const& arg0, Arg1 const& arg1, Args const&... args )
{
    return path( path( jv, arg0 ), arg1, args... );
}

int example_main(int, char**)
{
    value jv;
    path( jv, "user:config", "authority", "router", 0, "users" ) = 42;
    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: path") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_path

// libs/json/example/pretty.cpp
namespace ex_pretty {
// ---- inlined file.hpp ----
#undef BOOST_JSON_EXAMPLE_FILE_HPP
//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_EXAMPLE_FILE_HPP
#define BOOST_JSON_EXAMPLE_FILE_HPP


class file
{
    FILE* f_ = nullptr;
    long size_ = 0;

    void
    fail(boost::system::error_code& ec)
    {
        ec.assign( errno, boost::system::generic_category() );
    }

public:
    ~file()
    {
        if(f_)
            std::fclose(f_);
    }

    file() = default;

    file( file&& other ) noexcept
        : f_(other.f_)
    {
        other.f_ = nullptr;
    }

    file( char const* path, char const* mode )
    {
        open( path, mode );
    }

    file&
    operator=(file&& other) noexcept
    {
        close();
        f_ = other.f_;
        other.f_ = nullptr;
        return *this;
    }

    void
    close()
    {
        if(f_)
        {
            std::fclose(f_);
            f_ = nullptr;
            size_ = 0;
        }
    }

    void
    open(
        char const* path,
        char const* mode,
        boost::system::error_code& ec)
    {
        close();
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4996 )
#endif
        f_ = std::fopen( path, mode );
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
        if( ! f_ )
            return fail(ec);
        if( std::fseek( f_, 0, SEEK_END ) != 0)
            return fail(ec);
        size_ = std::ftell( f_ );
        if( size_ == -1 )
        {
            size_ = 0;
            return fail(ec);
        }
        if( std::fseek( f_, 0, SEEK_SET ) != 0)
            return fail(ec);
    }

    void
    open(
        char const* path,
        char const* mode)
    {
        boost::system::error_code ec;
        open(path, mode, ec);
        if(ec)
            throw boost::system::system_error(ec);
    }

    long
    size() const noexcept
    {
        return size_;
    }

    bool
    eof() const noexcept
    {
        return std::feof( f_ ) != 0;
    }

    std::size_t
    read( char* data, std::size_t size, boost::system::error_code& ec)
    {
        auto const nread = std::fread( data, 1, size, f_ );
        if( std::ferror(f_) )
            ec.assign( errno, boost::system::generic_category() );
        return nread;
    }

    std::size_t
    read( char* data, std::size_t size )
    {
        boost::system::error_code ec;
        auto const nread = read( data, size, ec );
        if(ec)
            throw boost::system::system_error(ec);
        return nread;
    }
};

inline
std::string
read_file( char const* path, boost::system::error_code& ec )
{
    file f;
    f.open( path, "r", ec );
    if(ec)
        return {};
    std::string s;
    s.resize( f.size() );
    s.resize( f.read( &s[0], s.size(), ec) );
    if(ec)
        return {};
    return s;
}

inline
std::string
read_file( char const* path )
{
    boost::system::error_code ec;
    auto s = read_file( path, ec);
    if(ec)
        throw boost::system::system_error(ec);
    return s;
}

#endif
// ---- end file.hpp ----

namespace json = boost::json;

json::value
parse_file( char const* filename )
{
    file f( filename, "r" );
    json::stream_parser p;
    boost::system::error_code ec;
    do
    {
        char buf[4096];
        auto const nread = f.read( buf, sizeof(buf) );
        p.write( buf, nread, ec );
    }
    while( ! f.eof() );
    if( ec )
        return nullptr;
    p.finish( ec );
    if( ec )
        return nullptr;
    return p.release();
}

void
pretty_print( std::ostream& os, json::value const& jv, std::string* indent = nullptr )
{
    std::string indent_;
    if(! indent)
        indent = &indent_;
    switch(jv.kind())
    {
    case json::kind::object:
    {
        os << "{\n";
        indent->append(4, ' ');
        auto const& obj = jv.get_object();
        if(! obj.empty())
        {
            auto it = obj.begin();
            for(;;)
            {
                os << *indent << json::serialize(it->key()) << " : ";
                pretty_print(os, it->value(), indent);
                if(++it == obj.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "}";
        break;
    }

    case json::kind::array:
    {
        os << "[\n";
        indent->append(4, ' ');
        auto const& arr = jv.get_array();
        if(! arr.empty())
        {
            auto it = arr.begin();
            for(;;)
            {
                os << *indent;
                pretty_print( os, *it, indent);
                if(++it == arr.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "]";
        break;
    }

    case json::kind::string:
    {
        os << json::serialize(jv.get_string());
        break;
    }

    case json::kind::uint64:
    case json::kind::int64:
    case json::kind::double_:
        os << jv;
        break;

    case json::kind::bool_:
        if(jv.get_bool())
            os << "true";
        else
            os << "false";
        break;

    case json::kind::null:
        os << "null";
        break;
    }

    if(indent->empty())
        os << "\n";
}

int example_main(int argc, char** argv)
{
    if(argc != 2)
    {
        ex_out <<
            "Usage: pretty <filename>"
            << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        // Parse the file as JSON
        auto const jv = parse_file( argv[1] );

        // Now pretty-print the value
        pretty_print(ex_out, jv);
    }
    catch(std::exception const& e)
    {
        ex_out <<
            "Caught exception: "
            << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

// end::example_pretty[]

TEST_CASE("example: pretty") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-json-sample.json"); f << R"j({"name":"boost","version":[1,92,0],"modules":true,"ratio":0.25,"tags":["a","b"],"nested":{"k":null}})j"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-json-sample.json"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex({
    "name" : "boost",
    "version" : [
        1,
        92,
        0
    ],
    "modules" : true,
    "ratio" : 2.5E-1,
    "tags" : [
        "a",
        "b"
    ],
    "nested" : {
        "k" : null
    }
}
)ex"));
}
}  // namespace ex_pretty

// libs/json/example/proxy.cpp
namespace ex_proxy {
using namespace boost::json;

class proxy
{
    value& jv_;

public:
    explicit
    proxy(value& jv) noexcept
        : jv_(jv)
    {
    }

    proxy
    operator[](string_view key)
    {
        object* obj;
        if(jv_.is_null())
            obj = &jv_.emplace_object();
        else
            obj = &jv_.as_object();
        return proxy((*obj)[key]);
    }

    proxy
    operator[](std::size_t index)
    {
        array *arr;
        if(jv_.is_null())
            arr = &jv_.emplace_array();
        else
            arr = &jv_.as_array();
        if(arr->size() <= index)
            arr->resize(index + 1);
        return proxy((*arr)[index]);
    }

    value&
    operator*() noexcept
    {
        return jv_;
    }
};


int example_main(int, char**)
{
    value jv;
    *proxy(jv)["user:config"]["authority"]["router"][0]["users"] = 42;
    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: proxy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_proxy

// libs/json/example/use_allocator.cpp
namespace ex_use_allocator {
using namespace boost::json;
using namespace boost::container;

template< class Alloc >
struct use_allocator_t
{
    Alloc allocator;
};

template< class Alloc >
use_allocator_t< Alloc >
use_allocator( Alloc alloc ) noexcept
{
    return { alloc };
}

template<
    class T,
    class Alloc,
    class FullContext,
    class = typename std::enable_if<
        is_sequence_like<T>::value &&
        std::uses_allocator<T, Alloc>::value
        >::type >
boost::system::result<T>
tag_invoke( try_value_to_tag<T>, const value& jv, const use_allocator_t<Alloc>& ctx, const FullContext& full_ctx )
{

    array const* arr = jv.if_array();
    if( !arr )
        return {
            boost::system::in_place_error,
            make_error_code(boost::system::errc::invalid_argument) };

    T result(ctx.allocator);
    auto ins = std::inserter(result, result.end());
    for( value const& val: *arr )
    {
        using ValueType = typename T::value_type;
        auto elem_res = try_value_to<ValueType>( val, full_ctx );
        if( elem_res.has_error() )
            return {boost::system::in_place_error, elem_res.error()};
        *ins++ = std::move(elem_res.unsafe_value());
    }
    return result;
}

int example_main(int, char**)
{

    value const jv = { 1, 2, 3, 4, 5, 6, 7, 8 };

    unsigned char buf[1024];
    pmr::monotonic_buffer_resource mr( buf, sizeof(buf) );

    auto v = value_to< pmr::vector<int> >(
        jv,
        use_allocator( pmr::polymorphic_allocator<int>(&mr) ) );

    assert( v.size() == jv.as_array().size() );

    for( auto i = 0u; i < v.size(); ++i )
        assert( v[i] == jv.at(i).to_number<int>() );

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

// end::example_use_allocator[]

TEST_CASE("example: use_allocator") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_use_allocator

// libs/json/example/validate.cpp
namespace ex_validate {
// This file must be manually included when
// using basic_parser to implement a parser.


// ---- inlined file.hpp ----
#undef BOOST_JSON_EXAMPLE_FILE_HPP
//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_EXAMPLE_FILE_HPP
#define BOOST_JSON_EXAMPLE_FILE_HPP


class file
{
    FILE* f_ = nullptr;
    long size_ = 0;

    void
    fail(boost::system::error_code& ec)
    {
        ec.assign( errno, boost::system::generic_category() );
    }

public:
    ~file()
    {
        if(f_)
            std::fclose(f_);
    }

    file() = default;

    file( file&& other ) noexcept
        : f_(other.f_)
    {
        other.f_ = nullptr;
    }

    file( char const* path, char const* mode )
    {
        open( path, mode );
    }

    file&
    operator=(file&& other) noexcept
    {
        close();
        f_ = other.f_;
        other.f_ = nullptr;
        return *this;
    }

    void
    close()
    {
        if(f_)
        {
            std::fclose(f_);
            f_ = nullptr;
            size_ = 0;
        }
    }

    void
    open(
        char const* path,
        char const* mode,
        boost::system::error_code& ec)
    {
        close();
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4996 )
#endif
        f_ = std::fopen( path, mode );
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
        if( ! f_ )
            return fail(ec);
        if( std::fseek( f_, 0, SEEK_END ) != 0)
            return fail(ec);
        size_ = std::ftell( f_ );
        if( size_ == -1 )
        {
            size_ = 0;
            return fail(ec);
        }
        if( std::fseek( f_, 0, SEEK_SET ) != 0)
            return fail(ec);
    }

    void
    open(
        char const* path,
        char const* mode)
    {
        boost::system::error_code ec;
        open(path, mode, ec);
        if(ec)
            throw boost::system::system_error(ec);
    }

    long
    size() const noexcept
    {
        return size_;
    }

    bool
    eof() const noexcept
    {
        return std::feof( f_ ) != 0;
    }

    std::size_t
    read( char* data, std::size_t size, boost::system::error_code& ec)
    {
        auto const nread = std::fread( data, 1, size, f_ );
        if( std::ferror(f_) )
            ec.assign( errno, boost::system::generic_category() );
        return nread;
    }

    std::size_t
    read( char* data, std::size_t size )
    {
        boost::system::error_code ec;
        auto const nread = read( data, size, ec );
        if(ec)
            throw boost::system::system_error(ec);
        return nread;
    }
};

inline
std::string
read_file( char const* path, boost::system::error_code& ec )
{
    file f;
    f.open( path, "r", ec );
    if(ec)
        return {};
    std::string s;
    s.resize( f.size() );
    s.resize( f.read( &s[0], s.size(), ec) );
    if(ec)
        return {};
    return s;
}

inline
std::string
read_file( char const* path )
{
    boost::system::error_code ec;
    auto s = read_file( path, ec);
    if(ec)
        throw boost::system::system_error(ec);
    return s;
}

#endif
// ---- end file.hpp ----

using namespace boost::json;

// The null parser discards all the data
class null_parser
{
    struct handler
    {
        constexpr static std::size_t max_object_size = std::size_t(-1);
        constexpr static std::size_t max_array_size = std::size_t(-1);
        constexpr static std::size_t max_key_size = std::size_t(-1);
        constexpr static std::size_t max_string_size = std::size_t(-1);

        bool on_document_begin( boost::system::error_code& ) { return true; }
        bool on_document_end( boost::system::error_code& ) { return true; }
        bool on_object_begin( boost::system::error_code& ) { return true; }
        bool on_object_end( std::size_t, boost::system::error_code& ) { return true; }
        bool on_array_begin( boost::system::error_code& ) { return true; }
        bool on_array_end( std::size_t, boost::system::error_code& ) { return true; }
        bool on_key_part( string_view, std::size_t, boost::system::error_code& ) { return true; }
        bool on_key( string_view, std::size_t, boost::system::error_code& ) { return true; }
        bool on_string_part( string_view, std::size_t, boost::system::error_code& ) { return true; }
        bool on_string( string_view, std::size_t, boost::system::error_code& ) { return true; }
        bool on_number_part( string_view, boost::system::error_code& ) { return true; }
        bool on_int64( std::int64_t, string_view, boost::system::error_code& ) { return true; }
        bool on_uint64( std::uint64_t, string_view, boost::system::error_code& ) { return true; }
        bool on_double( double, string_view, boost::system::error_code& ) { return true; }
        bool on_bool( bool, boost::system::error_code& ) { return true; }
        bool on_null( boost::system::error_code& ) { return true; }
        bool on_comment_part(string_view, boost::system::error_code&) { return true; }
        bool on_comment(string_view, boost::system::error_code&) { return true; }
    };

    basic_parser<handler> p_;

public:
    null_parser()
        : p_(parse_options())
    {
    }

    ~null_parser()
    {
    }

    std::size_t
    write(
        char const* data,
        std::size_t size,
        boost::system::error_code& ec)
    {
        auto const n = p_.write_some( false, data, size, ec );
        if(! ec && n < size)
            ec = error::extra_data;
        return n;
    }
};

bool
validate( string_view s )
{
    // Parse with the null parser and return false on error
    null_parser p;
    boost::system::error_code ec;
    p.write( s.data(), s.size(), ec );
    if( ec )
        return false;

    // The string is valid JSON.
    return true;
}

int example_main(int argc, char** argv)
{
    if(argc != 2)
    {
        ex_out <<
            "Usage: validate <filename>"
            << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        // Read the file into a string
        auto const s = read_file( argv[1] );

        // See if the string is valid JSON
        auto const valid = validate( s );

        // Print the result
        if( valid )
            ex_out << argv[1] << " contains a valid JSON\n";
        else
            ex_out << argv[1] << " does not contain a valid JSON\n";
    }
    catch(std::exception const& e)
    {
        ex_out <<
            "Caught exception: "
            << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

// end::example_validate[]

TEST_CASE("example: validate") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-json-sample.json"); f << R"j({"name":"boost","version":[1,92,0],"modules":true,"ratio":0.25,"tags":["a","b"],"nested":{"k":null}})j"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-json-sample.json"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(boost-m-json-sample.json contains a valid JSON
)ex"));
}
}  // namespace ex_validate
