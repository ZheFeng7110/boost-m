// boost.bimap with the macro libraries — pure include: bimap_and_boost/
// {foreach,typeof,xpressive}.cpp exercise BOOST_FOREACH, BOOST_TYPEOF /
// BOOST_AUTO and xpressive actions, which no module can carry, so this TU
// includes everything and imports nothing. Same output oracle as examples.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/config.hpp>
#include <string>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/support/lambda.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

// libs/bimap/example/bimap_and_boost/foreach.cpp
namespace ex_foreach {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------





using namespace boost::bimaps;


int example_main()
{
    //[ code_bimap_and_boost_foreach

    typedef bimap< std::string, list_of<int> > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type("1", 1) );
    bm.insert( bm_type::value_type("2", 2) );
    bm.insert( bm_type::value_type("3", 4) );
    bm.insert( bm_type::value_type("4", 2) );

    BOOST_FOREACH( bm_type::left_reference p, bm.left )
    {
        ++p.second; /*< We can modify the right element because we have
                        use a mutable collection type in the right side. >*/
    }

    BOOST_FOREACH( bm_type::right_const_reference p, bm.right )
    {
        ex_out << p.first << "-->" << p.second << std::endl;
    }

    //]

    // More examples

    BOOST_FOREACH( bm_type::right_reference p, bm.right )
    {
        ++p.first;
    }

    BOOST_FOREACH( bm_type::left_const_reference p, bm.left )
    {
        ex_out << p.first << "-->" << p.second << std::endl;
    }

    BOOST_FOREACH( bm_type::reference p, bm )
    {
        ++p.right;
    }

    const bm_type & cbm = bm;
    BOOST_FOREACH( bm_type::const_reference p, cbm )
    {
        ex_out << p.left << "-->" << p.right << std::endl;
    }

    BOOST_FOREACH( bm_type::const_reference p, bm )
    {
        ex_out << p.left << "-->" << p.right << std::endl;
    }

    //[ code_bimap_and_boost_foreach_using_range

    BOOST_FOREACH( bm_type::left_reference p,
                 ( bm.left.range( std::string("1") <= _key, _key < std::string("3") ) ))
    {
        ++p.second;
    }

    BOOST_FOREACH( bm_type::left_const_reference p,
                 ( bm.left.range( std::string("1") <= _key, _key < std::string("3") ) ))
    {
        ex_out << p.first << "-->" << p.second << std::endl;
    }
    //]

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: foreach") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(2-->1
3-->2
5-->3
3-->4
1-->3
2-->4
3-->6
4-->4
1-->4
2-->5
3-->7
4-->5
1-->4
2-->5
3-->7
4-->5
1-->5
2-->6
)ex"));
}
}  // namespace ex_foreach

// libs/bimap/example/bimap_and_boost/typeof.cpp
namespace ex_typeof {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

struct name   {};
struct number {};

void using_auto()
{
    //[ code_bimap_and_boost_typeof_first

    typedef bimap< tagged<std::string,name>, tagged<int,number> > bm_type;
    bm_type bm;
    bm.insert( bm_type::value_type("one"  ,1) );
    bm.insert( bm_type::value_type("two"  ,2) );
    //]

    //[ code_bimap_and_boost_typeof_using_auto

    for( BOOST_AUTO(iter, bm.by<name>().begin()); iter!=bm.by<name>().end(); ++iter)
    {
        ex_out << iter->first << " --> " << iter->second << std::endl;
    }

    BOOST_AUTO( iter, bm.by<number>().find(2) );
    ex_out << "2: " << iter->get<name>();
    //]
}

void not_using_auto()
{
    typedef bimap< tagged<std::string,name>, tagged<int,number> > bm_type;
    bm_type bm;
    bm.insert( bm_type::value_type("one"  ,1) );
    bm.insert( bm_type::value_type("two"  ,2) );

    //[ code_bimap_and_boost_typeof_not_using_auto

    for( bm_type::map_by<name>::iterator iter = bm.by<name>().begin();
         iter!=bm.by<name>().end(); ++iter)
    {
        ex_out << iter->first << " --> " << iter->second << std::endl;
    }

    bm_type::map_by<number>::iterator iter = bm.by<number>().find(2);
    ex_out << "2: " << iter->get<name>();
    //]
}

int example_main()
{
    using_auto();
    not_using_auto();

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: typeof") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(one --> 1
two --> 2
2: twoone --> 1
two --> 2
2: two)ex"));
}
}  // namespace ex_typeof

// libs/bimap/example/bimap_and_boost/xpressive.cpp
namespace ex_xpressive {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------





using namespace boost::bimaps;
using namespace boost::xpressive;
namespace xp = boost::xpressive;

int example_main()
{
    //[ code_bimap_and_boost_xpressive

    typedef bimap< std::string, int > bm_type;
    bm_type bm;

    std::string rel_str("one <--> 1     two <--> 2      three <--> 3");

    sregex rel = ( (s1= +_w) >> " <--> " >> (s2= +_d) )
    [
        xp::ref(bm)->*insert( xp::construct<bm_type::value_type>(s1, as<int>(s2)) )
    ];

    sregex relations = rel >> *(+_s >> rel);

    regex_match(rel_str, relations);

    assert( bm.size() == 3 );
    //]

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: xpressive") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_xpressive
