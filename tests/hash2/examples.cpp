// mcpplibs.mwrap.boost.hash2 — the official examples (libs/hash2/example/*.cpp)
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone.
// md5sum and hash2sum (command-line tools) run on two files written to the
// temporary directory; json_value hashes a boost::json::value through the
// json module and adds its hash_append overload to namespace boost.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/config.hpp>
#include <cerrno>
import std;
import mcpplibs.mwrap.boost.hash2;
import mcpplibs.mwrap.boost.json;
import mcpplibs.mwrap.boost.unordered;

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

// libs/hash2/example/compile_time.cpp
namespace ex_compile_time {
#if defined(BOOST_MSVC) && BOOST_MSVC < 1920
# pragma warning(disable: 4307) // integral constant overflow
#endif


// xxd -i resource
constexpr unsigned char resource[] = {
  0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x6d, 0x90,
  0xcf, 0x6e, 0x83, 0x30, 0x0c, 0xc6, 0xcf, 0x45, 0xea, 0x3b, 0x78, 0x9c,
  0x4b, 0x02, 0x3d, 0x6e, 0xd0, 0x43, 0xff, 0x1c, 0x26, 0x55, 0x3b, 0x14,
  0x6d, 0xd7, 0x2a, 0x04, 0x43, 0x22, 0x95, 0x84, 0x25, 0x66, 0x8c, 0x47,
  0xda, 0x5b, 0x2e, 0x91, 0xd6, 0xcb, 0xd4, 0x93, 0x2d, 0xdb, 0xdf, 0xef,
  0xb3, 0x5d, 0x2a, 0x1a, 0x6e, 0xbb, 0x75, 0x52, 0x2a, 0x14, 0x6d, 0x8c,
  0x03, 0x92, 0x00, 0x45, 0x34, 0x66, 0xf8, 0x39, 0xe9, 0xaf, 0x2a, 0x75,
  0xd8, 0x39, 0xf4, 0x2a, 0x05, 0x69, 0x0d, 0xa1, 0xa1, 0x2a, 0xcd, 0x5f,
  0xe0, 0xfd, 0x72, 0xae, 0x5a, 0x2b, 0x79, 0x54, 0x73, 0x25, 0xbc, 0xda,
  0xb2, 0x98, 0xa6, 0x91, 0xc0, 0xef, 0xa8, 0xc6, 0xb6, 0x4b, 0x88, 0x17,
  0x6c, 0xb5, 0x43, 0x49, 0xda, 0xf4, 0x40, 0x16, 0xca, 0x80, 0x0f, 0xcc,
  0x2a, 0x7d, 0xa8, 0x7f, 0x50, 0x2c, 0xb9, 0xd8, 0x31, 0xc6, 0x22, 0xf9,
  0x8f, 0x58, 0xf2, 0xfb, 0xd6, 0x4f, 0x59, 0xb6, 0x4e, 0x56, 0x3f, 0x70,
  0xb0, 0xe3, 0xe2, 0x74, 0xaf, 0x08, 0xf6, 0x38, 0x08, 0x03, 0x47, 0x31,
  0xa3, 0xdf, 0xc0, 0x36, 0xcf, 0x8b, 0xd0, 0x3f, 0x6a, 0x4f, 0x4e, 0x37,
  0x13, 0x61, 0x0b, 0x93, 0x69, 0xd1, 0x01, 0x29, 0x84, 0xbd, 0xb5, 0x9e,
  0xa0, 0xb6, 0x1d, 0xcd, 0xc2, 0x21, 0x9c, 0xb5, 0x44, 0xe3, 0x71, 0x03,
  0x1f, 0xe8, 0xbc, 0xb6, 0x06, 0x0a, 0x96, 0x07, 0xd3, 0x55, 0x8d, 0x08,
  0x42, 0x4a, 0x3b, 0x8c, 0xc2, 0x2c, 0xf1, 0x86, 0x4e, 0xdf, 0xc2, 0xf4,
  0xeb, 0xe1, 0xf4, 0x56, 0x9f, 0xae, 0xc5, 0x35, 0x67, 0xf4, 0x4d, 0x60,
  0x5d, 0xf8, 0xcf, 0xb8, 0x80, 0xa0, 0x20, 0x89, 0xef, 0x7b, 0xe6, 0x7c,
  0x9e, 0x67, 0xd6, 0x44, 0x13, 0x66, 0x5d, 0xcf, 0xff, 0x29, 0xd6, 0x49,
  0x96, 0x85, 0x0b, 0x7e, 0x01, 0x36, 0x66, 0x95, 0x6b, 0x80, 0x01, 0x00,
  0x00
};

template<std::size_t N> constexpr auto md5( unsigned char const(&a)[ N ] )
{
    boost::hash2::md5_128 hash;
    hash.update( a, N );
    return hash.result();
}

constexpr auto resource_digest = md5( resource );

int example_main()
{
    ex_out << "Resource digest: " << resource_digest << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: compile_time") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Resource digest: 07e59d263e2a423ab48a1e2f630e9f15
)ex"));
}
}  // namespace ex_compile_time

// libs/hash2/example/compile_time_2.cpp
namespace ex_compile_time_2 {
#if defined(BOOST_MSVC) && BOOST_MSVC < 1920
# pragma warning(disable: 4307) // integral constant overflow
#endif


extern constexpr char const license[] =

"Boost Software License - Version 1.0 - August 17th, 2003\n"
"\n"
"Permission is hereby granted, free of charge, to any person or organization\n"
"obtaining a copy of the software and accompanying documentation covered by\n"
"this license (the \"Software\") to use, reproduce, display, distribute,\n"
"execute, and transmit the Software, and to prepare derivative works of the\n"
"Software, and to permit third-parties to whom the Software is furnished to\n"
"do so, all subject to the following:\n"
"\n"
"The copyright notices in the Software and this entire statement, including\n"
"the above license grant, this restriction and the following disclaimer,\n"
"must be included in all copies of the Software, in whole or in part, and\n"
"all derivative works of the Software, unless such copies or derivative\n"
"works are solely in the form of machine-executable object code generated by\n"
"a source language processor.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT\n"
"SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE\n"
"FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,\n"
"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER\n"
"DEALINGS IN THE SOFTWARE.\n"

;

constexpr unsigned char secret[] = {
    0xA4, 0x80, 0x0E, 0xE8, 0x20, 0x0B, 0x7C, 0x9A,
    0xF1, 0x3E, 0x3D, 0xEC, 0x64, 0x4F, 0x64, 0xCA,
    0x33, 0xCC, 0x84, 0xC8, 0x34, 0xE3, 0x08, 0xAE,
    0x92, 0x89, 0xEB, 0xD0, 0x47, 0x39, 0x87, 0xD8,
};

template<std::size_t N> constexpr auto hmac_sha2_256( char const(&s)[ N ] )
{
    boost::hash2::hmac_sha2_256 hmac( secret, sizeof(secret) );

    // N-1, in order to not include the null terminator
    boost::hash2::hash_append_range( hmac, {}, s, s + N - 1 );

    return hmac.result();
}

constexpr auto license_mac = hmac_sha2_256( license );

int example_main()
{
    ex_out << "License authentication code: " << license_mac << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: compile_time_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(License authentication code: 86795556f991dff7df6dbcdb7fa417670a27ad512b2927d4dc5abdc6c71b1559
)ex"));
}
}  // namespace ex_compile_time_2

// libs/hash2/example/hash2sum.cpp
namespace ex_hash2sum {
#define _CRT_SECURE_NO_WARNINGS


template<class Hash> void hash2sum( std::FILE* f, char const* fn )
{
    Hash hash;

    int const N = 4096;
    unsigned char buffer[ N ];

    for( ;; )
    {
        std::size_t n = std::fread( buffer, 1, N, f );

        if( std::ferror( f ) )
        {
            std::fprintf( stderr, "'%s': read error: %s\n", fn, std::strerror( errno ) );
            return;
        }

        if( n == 0 ) break;

        hash.update( buffer, n );
    }

    std::string digest = to_string( hash.result() );

    ex_printf( "%s *%s\n", digest.c_str(), fn );
}

template<class Hash> void hash2sum( char const* fn )
{
    std::FILE* f = std::fopen( fn, "rb" );

    if( f == 0 )
    {
        std::fprintf( stderr, "'%s': open error: %s\n", fn, std::strerror( errno ) );
    }
    else
    {
        hash2sum<Hash>( f, fn );
        std::fclose( f );
    }
}

using namespace boost::mp11;
using namespace boost::hash2;

using hashes = mp_list<

    md5_128,
    sha1_160,
    sha2_256,
    sha2_224,
    sha2_512,
    sha2_384,
    sha2_512_256,
    sha2_512_224,
    sha3_256,
    sha3_224,
    sha3_512,
    sha3_384,
    ripemd_160,
    ripemd_128,
    blake2b_512,
    blake2s_256

>;

constexpr char const* names[] = {

    "md5_128",
    "sha1_160",
    "sha2_256",
    "sha2_224",
    "sha2_512",
    "sha2_384",
    "sha2_512_256",
    "sha2_512_224",
    "sha3_256",
    "sha3_224",
    "sha3_512",
    "sha3_384",
    "ripemd_160",
    "ripemd_128",
    "blake2b_512",
    "blake2s_256"
};

int example_main( int argc, char const* argv[] )
{
    if( argc < 2 )
    {
        std::fputs( "usage: hash2sum <hash> <files...>\n", stderr );
        return 2;
    }

    std::string hash( argv[1] );
    bool found = false;

    mp_for_each< mp_iota<mp_size<hashes>> >([&](auto I){

        if( hash == names[I] )
        {
            using Hash = mp_at_c<hashes, I>;

            for( int i = 2; i < argc; ++i )
            {
                hash2sum<Hash>( argv[i] );
            }

            found = true;
        }

    });

    if( !found )
    {
        std::fprintf( stderr, "hash2sum: unknown hash algorithm name '%s'; use one of the following:\n\n", hash.c_str() );

        for( char const* name: names )
        {
            std::fprintf( stderr, "   %s\n", name );
        }

        return 1;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: hash2sum") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream(std::filesystem::temp_directory_path() / "boost-m-hash2-a.txt", std::ios::binary) << "The quick brown fox jumps over the lazy dog"; std::ofstream(std::filesystem::temp_directory_path() / "boost-m-hash2-b.txt", std::ios::binary) << ""; }
    ex_args.push_back(std::string("md5_128"));
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-hash2-a.txt").string());
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-hash2-b.txt").string());
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(9e107d9d372bb6826bd81d3542a419d6 */tmp/boost-m-hash2-a.txt
d41d8cd98f00b204e9800998ecf8427e */tmp/boost-m-hash2-b.txt
)ex"));
}
}  // namespace ex_hash2sum

// libs/hash2/example/hash_32_64.cpp
namespace ex_hash_32_64 {
template<class T, class H1, class H2 = H1> class hash
{
public:

    using hash_type = typename std::conditional<
        sizeof(typename H1::result_type) == sizeof(std::size_t), H1, H2
    >::type;

private:

    hash_type h_;

public:

    hash(): h_()
    {
    }

    explicit hash( std::uint64_t seed ): h_( seed )
    {
    }

    hash( unsigned char const* p, std::size_t n ): h_( p, n )
    {
    }

    std::size_t operator()( T const& v ) const
    {
        hash_type h( h_ );
        boost::hash2::hash_append( h, {}, v );
        return boost::hash2::get_integral_result<std::size_t>( h );
    }
};

int example_main()
{
    {
        using hasher = hash<std::string, boost::hash2::siphash_32, boost::hash2::siphash_64>;

        ex_out << boost::core::type_name<hasher>() << " uses "
            << boost::core::type_name<hasher::hash_type>() << std::endl;

        boost::unordered_flat_map<std::string, int, hasher> map;

        map[ "foo" ] = 1;
        map[ "bar" ] = 2;
    }

    {
        using hasher = hash<std::string, boost::hash2::md5_128>;

        ex_out << boost::core::type_name<hasher>() << " uses "
            << boost::core::type_name<hasher::hash_type>() << std::endl;

        boost::unordered_flat_map<std::string, int, hasher> map;

        map[ "foo" ] = 1;
        map[ "bar" ] = 2;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: hash_32_64") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(ex_hash_32_64::hash<std::string, boost::hash2::siphash_32, boost::hash2::siphash_64> uses boost::hash2::siphash_64
ex_hash_32_64::hash<std::string, boost::hash2::md5_128, boost::hash2::md5_128> uses boost::hash2::md5_128
)ex"));
}
}  // namespace ex_hash_32_64

// libs/hash2/example/hash_with_any_seed.cpp
namespace ex_hash_with_any_seed {
template<class T, class H> class hash
{
private:

    H h_;

public:

    hash(): h_()
    {
    }

    explicit hash( std::uint64_t seed ): h_( seed )
    {
    }

    hash( unsigned char const* p, std::size_t n ): h_( p, n )
    {
    }

    std::size_t operator()( T const& v ) const
    {
        H h( h_ );
        boost::hash2::hash_append( h, {}, v );
        return boost::hash2::get_integral_result<std::size_t>( h );
    }
};

int example_main()
{
    using hasher = hash<std::string, boost::hash2::siphash_64>;

    {
        boost::unordered_flat_map<std::string, int, hasher> map;

        map[ "foo" ] = 1;
        map[ "bar" ] = 2;
    }

    {
        std::uint64_t seed = 0x0102030405060708ull;

        boost::unordered_flat_map<std::string, int, hasher> map( 0, hasher( seed ) );

        map[ "foo" ] = 1;
        map[ "bar" ] = 2;
    }

    {
        unsigned char const seed[ 16 ] =
        {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
        };

        boost::unordered_flat_map<std::string, int, hasher> map( 0, hasher(seed, sizeof(seed)) );

        map[ "foo" ] = 1;
        map[ "bar" ] = 2;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: hash_with_any_seed") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_hash_with_any_seed

// libs/hash2/example/hash_with_byte_seed.cpp
namespace ex_hash_with_byte_seed {
template<class T, class H> class hash
{
private:

    H h_;

public:

    hash( unsigned char const* p, std::size_t n ): h_( p, n )
    {
    }

    std::size_t operator()( T const& v ) const
    {
        H h( h_ );
        boost::hash2::hash_append( h, {}, v );
        return boost::hash2::get_integral_result<std::size_t>( h );
    }
};

int example_main()
{
    unsigned char const seed[ 16 ] =
    {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };

    using hasher = hash<std::string, boost::hash2::siphash_64>;

    boost::unordered_flat_map<std::string, int, hasher> map( 0, hasher( seed, sizeof(seed) ) );

    map[ "foo" ] = 1;
    map[ "bar" ] = 2;
    return 0;  // main's implicit return
}

TEST_CASE("example: hash_with_byte_seed") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_hash_with_byte_seed

// libs/hash2/example/hash_with_uint64_seed.cpp
namespace ex_hash_with_uint64_seed {
template<class T, class H> class hash
{
private:

    std::uint64_t seed_;

public:

    explicit hash( std::uint64_t seed ): seed_( seed )
    {
    }

    std::size_t operator()( T const& v ) const
    {
        H h( seed_ );
        boost::hash2::hash_append( h, {}, v );
        return boost::hash2::get_integral_result<std::size_t>( h );
    }
};

int example_main()
{
    std::uint64_t seed = 0x0102030405060708ull;

    using hasher = hash<std::string, boost::hash2::siphash_64>;

    boost::unordered_flat_map<std::string, int, hasher> map( 0, hasher( seed ) );

    map[ "foo" ] = 1;
    map[ "bar" ] = 2;
    return 0;  // main's implicit return
}

TEST_CASE("example: hash_with_uint64_seed") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_hash_with_uint64_seed

// libs/hash2/example/hash_without_seed.cpp
namespace ex_hash_without_seed {
template<class T, class H> class hash
{
public:

    std::size_t operator()( T const& v ) const
    {
        H h;
        boost::hash2::hash_append( h, {}, v );
        return boost::hash2::get_integral_result<std::size_t>( h );
    }
};

int example_main()
{
    using hasher = hash<std::string, boost::hash2::fnv1a_64>;

    boost::unordered_flat_map<std::string, int, hasher> map;

    map[ "foo" ] = 1;
    map[ "bar" ] = 2;
    return 0;  // main's implicit return
}

TEST_CASE("example: hash_without_seed") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_hash_without_seed

// libs/hash2/example/json_value.cpp: its additions to namespace boost/std, outside the case namespace
namespace boost
{
namespace json
{

template<class Provider, class Hash, class Flavor>
void tag_invoke( boost::hash2::hash_append_tag const&, Provider const&,
    Hash& h, Flavor const& f, boost::json::value const* v )
{
    boost::hash2::hash_append( h, f, v->kind() ); // TODO: int64 vs uint64
    boost::json::visit( [&](auto const& w){
        boost::hash2::hash_append( h, f, w ); }, *v );
}

} // namespace json
}
// libs/hash2/example/json_value.cpp
namespace ex_json_value {
 // namespace boost

template<class T> auto md5( T const& v )
{
    boost::hash2::md5_128 hash;
    boost::hash2::hash_append( hash, {}, v );
    return hash.result();
}

static constexpr char const* json = R"(
{
  "x1": -5,
  "x2": 3.14,
  "x3": "str",
  "x4": [ null, true, false ]
}
)";

int example_main()
{
    auto jv = boost::json::parse( json );
    ex_out << md5( jv ) << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: json_value") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(e33268fc9e1dc055ee3d5e2783d2a0a2
)ex"));
}
}  // namespace ex_json_value

// libs/hash2/example/md5sum.cpp
namespace ex_md5sum {
#define _CRT_SECURE_NO_WARNINGS


static void md5sum( std::FILE* f, char const* fn )
{
    boost::hash2::md5_128 hash;

    int const N = 4096;
    unsigned char buffer[ N ];

    for( ;; )
    {
        std::size_t n = std::fread( buffer, 1, N, f );

        if( std::ferror( f ) )
        {
            std::fprintf( stderr, "'%s': read error: %s\n", fn, std::strerror( errno ) );
            return;
        }

        if( n == 0 ) break;

        hash.update( buffer, n );
    }

    std::string digest = to_string( hash.result() );

    ex_printf( "%s *%s\n", digest.c_str(), fn );
}

int example_main( int argc, char const* argv[] )
{
    for( int i = 1; i < argc; ++i )
    {
        std::FILE* f = std::fopen( argv[i], "rb" );

        if( f == 0 )
        {
            std::fprintf( stderr, "'%s': open error: %s\n", argv[i], std::strerror( errno ) );
            continue;
        }

        md5sum( f, argv[i] );

        std::fclose( f );
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: md5sum") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream(std::filesystem::temp_directory_path() / "boost-m-hash2-a.txt", std::ios::binary) << "The quick brown fox jumps over the lazy dog"; std::ofstream(std::filesystem::temp_directory_path() / "boost-m-hash2-b.txt", std::ios::binary) << ""; }
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-hash2-a.txt").string());
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-hash2-b.txt").string());
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(9e107d9d372bb6826bd81d3542a419d6 */tmp/boost-m-hash2-a.txt
d41d8cd98f00b204e9800998ecf8427e */tmp/boost-m-hash2-b.txt
)ex"));
}
}  // namespace ex_md5sum

// libs/hash2/example/xxh128_from_xxh64.cpp
namespace ex_xxh128_from_xxh64 {
class xxhash_128: private boost::hash2::xxhash_64
{
public:

    using result_type = boost::hash2::digest<16>;

    using xxhash_64::xxhash_64;
    using xxhash_64::update;

    result_type result()
    {
        std::uint64_t r1 = xxhash_64::result();
        std::uint64_t r2 = xxhash_64::result();

        result_type r = {};

        boost::endian::store_little_u64( r.data() + 0, r1 );
        boost::endian::store_little_u64( r.data() + 8, r2 );

        return r;
    }
};


int example_main()
{
    std::string tv( "The quick brown fox jumps over the lazy dog" );

    xxhash_128 hash( 43 );
    hash.update( tv.data(), tv.size() );

    ex_out << hash.result() << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: xxh128_from_xxh64") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(25748d46e9f5119ab7c2a4551965441c
)ex"));
}
}  // namespace ex_xxh128_from_xxh64
