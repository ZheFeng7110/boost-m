// mcpplibs.mwrap.boost.json — libs/json/test/doc_forward_conversion.cpp, the code the
// documentation pages include, unchanged except that its includes are imports
// and its test macros come from doc_test_suite.hpp (doctest).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "doc_test_suite.hpp"
#include <cassert>
#include <cmath>
#include <boost/config.hpp>
#include <boost/core/detail/static_assert.hpp>
import std;
import mcpplibs.mwrap.boost.json;
import mcpplibs.mwrap.boost.container;

// tag::doc_forward_conversion_1[]
namespace boost {
namespace json {

class value;

struct value_from_tag;

template< class T >
struct try_value_to_tag;

template< class T1, class T2 >
struct result_for;

template< class T >
void value_from( T&& t, value& jv );

template< class T >
typename result_for< T, value >::type
try_value_to( const value& jv );

}
}
// end::doc_forward_conversion_1[]

// tag::doc_forward_conversion_3[]
namespace boost {
namespace json {

class value;

struct value_from_tag;

template< class T >
struct try_value_to_tag;

template< class T1, class T2 >
struct result_for;

template< class T, class Context >
void value_from( T&& t, value& jv, const Context& ctx );

template< class T, class Context >
typename result_for< T, value >::type
try_value_to( const value& jv, const Context& ctx );

}
}
// end::doc_forward_conversion_3[]

// ---- inlined doc_types.hpp ----
//
// Copyright (c) 2022 Dmitry Arkhipov (grisumbras@yandex.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//



#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

#include <array>

// tag::snippet_conv_spec_trait1[]
namespace user_ns
{

class ip_address
{
public:
    ip_address(
        unsigned char oct1,
        unsigned char oct2,
        unsigned char oct3,
        unsigned char oct4 );

    const unsigned char*
    begin() const;

    const unsigned char*
    end() const;

private:
    std::array<unsigned char, 4> octets_;
};

template< std::size_t N >
unsigned char
get(const ip_address& addr);

} // namespace user_ns

namespace std
{

template<>
struct tuple_size< user_ns::ip_address >
    : std::integral_constant<std::size_t, 4>
{ };

template< std::size_t N >
struct tuple_element< N, user_ns::ip_address >
{
    using type = unsigned char;
};

} // namespace std
// end::snippet_conv_spec_trait1[]

namespace user_ns
{

inline
ip_address::ip_address(
    unsigned char oct1,
    unsigned char oct2,
    unsigned char oct3,
    unsigned char oct4 )
    : octets_{{oct1, oct2, oct3, oct4}}
{ }

inline
const unsigned char*
ip_address::begin() const
{
    return octets_.data();
}

inline
const unsigned char*
ip_address::end() const
{
    return begin() + octets_.size();
}

template< std::size_t N >
unsigned char
get(ip_address const& addr )
{
    static_assert( N >= 0 && N <= 4, "incorrect N" );
    return *(addr.begin() + N);
}

inline
bool
operator==(ip_address const& l, ip_address const& r) noexcept
{
    return
        get<0>(l) == get<0>(r) &&
        get<1>(l) == get<1>(r) &&
        get<2>(l) == get<2>(r) &&
        get<3>(l) == get<3>(r);
}

} // namespace user_ns

// ---- end doc_types.hpp ----


// tag::doc_forward_conversion_2[]
namespace user_ns
{

template< class JsonValue >
void tag_invoke(
    const boost::json::value_from_tag&, JsonValue& jv, const ip_address& addr )
{
    const unsigned char* b = addr.begin();
    jv = { b[0], b[1], b[2], b[3] };
}

template< class JsonValue >
typename boost::json::result_for< ip_address, JsonValue >::type
tag_invoke(
    const boost::json::try_value_to_tag< ip_address >&,
    const JsonValue& jv )
{
    using namespace boost::json;

    if( !jv.is_array() )
        return make_error_code( std::errc::invalid_argument );

    auto const& arr = jv.get_array();
    if( arr.size() != 4 )
        return make_error_code( std::errc::invalid_argument );

    auto oct1 = try_value_to< unsigned char >( arr[0] );
    if( !oct1 )
        return make_error_code( std::errc::invalid_argument );

    auto oct2 = try_value_to< unsigned char >( arr[1] );
    if( !oct2 )
        return make_error_code( std::errc::invalid_argument );

    auto oct3 = try_value_to< unsigned char >( arr[2] );
    if( !oct3 )
        return make_error_code( std::errc::invalid_argument );

    auto oct4 = try_value_to< unsigned char >( arr[3] );
    if( !oct4 )
        return make_error_code( std::errc::invalid_argument );

    return ip_address{ *oct1, *oct2, *oct3, *oct4 };
}

}
// end::doc_forward_conversion_2[]

// tag::doc_forward_conversion_3[]

namespace user_ns
{

struct as_string
{ };

template< class JsonValue >
void
tag_invoke(
    const boost::json::value_from_tag&, JsonValue& jv,
    const ip_address& addr,
    const as_string& )
{
    auto& js = jv.emplace_string();
    js.resize( 4 * 3 + 3 + 1 ); // XXX.XXX.XXX.XXX\0
    auto it = addr.begin();
    auto n = std::snprintf(
        js.data(), js.size(), "%hhu.%hhu.%hhu.%hhu", it[0], it[1], it[2], it[3] );
    js.resize(n);
}

template< class JsonValue >
typename boost::json::result_for< ip_address, JsonValue >::type
tag_invoke(
    const boost::json::try_value_to_tag< ip_address >&,
    const JsonValue& jv,
    const as_string& )
{
    const auto* js = jv.if_string();
    if( ! js )
        return make_error_code( std::errc::invalid_argument );

    unsigned char octets[4];
    int result = std::sscanf(
        js->data(), "%hhu.%hhu.%hhu.%hhu", octets, octets + 1, octets + 2, octets + 3 );
    if( result != 4 )
        return make_error_code( std::errc::invalid_argument );

    return ip_address( octets[0], octets[1], octets[2], octets[3] );
}

}
// end::doc_forward_conversion_3[]




namespace boost {
namespace json {

class doc_forward_conversion
{
public:
    void
    run()
    {
        value jv{ 212, 115, 81, 22 };
        auto const addr = value_to< user_ns::ip_address >( jv );
        BOOST_TEST( get<0>(addr) == 212 );
        BOOST_TEST( get<1>(addr) == 115 );
        BOOST_TEST( get<2>(addr) == 81 );
        BOOST_TEST( get<3>(addr) == 22 );

        value const jv2 = value_from( addr );
        BOOST_TEST( jv == jv2 );

        jv = value_from( addr, user_ns::as_string() );
        BOOST_TEST( jv.is_string() );

        string const& js = jv.get_string();
        BOOST_TEST( js == "212.115.81.22" );

        auto const addr2 = value_to< user_ns::ip_address >(
            jv, user_ns::as_string() );
        BOOST_TEST( get<0>(addr2) == 212 );
        BOOST_TEST( get<1>(addr2) == 115 );
        BOOST_TEST( get<2>(addr2) == 81 );
        BOOST_TEST( get<3>(addr2) == 22 );
    }
};

TEST_SUITE(doc_forward_conversion, "boost.json.doc_forward_conversion");

} // namespace json
} // namespace boost
