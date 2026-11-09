// mcpplibs.mwrap.boost.bimap — the official examples that combine Boost.Bimap
// with other libraries through their modules (bimap_and_boost/{assign,
// lambda,property_map,range,serialization}.cpp, population_bimap.cpp,
// mi_to_b_path/mi_hashed_indices.cpp), with the same output oracle as
// examples.cpp. Two programs are edited before porting (and the oracle runs
// the edited program): population_bimap spells boost::optional (the unqualified
// name is ambiguous with std::optional since C++17, upstream too) and writes
// its BOOST_FOREACH loops over an iterator pair as range-for over
// boost::make_iterator_range; mi_hashed_indices writes
// BOOST_MULTI_INDEX_MEMBER(T, type, m) as the member<T, type, &T::m> it
// expands to. The foreach, typeof and xpressive examples are about those
// macro libraries: include_with_boost.cpp.
// range.cpp also writes using namespace boost::placeholders (see there), and
// serialization.cpp archives to the temporary directory instead of ./data.
// The range module exports boost::count_if, which range.cpp's own count_if
// meets through `using namespace boost` (the original includes only the range
// headers it needs); the port calls the example's count_if qualified.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cassert>
import std;
import mcpplibs.mwrap.boost.bimap;
import mcpplibs.mwrap.boost.assign;
import mcpplibs.mwrap.boost.bind;
import mcpplibs.mwrap.boost.lambda;
import mcpplibs.mwrap.boost.property_map;
import mcpplibs.mwrap.boost.range;
import mcpplibs.mwrap.boost.serialization;
import mcpplibs.mwrap.boost.multi_index;
import mcpplibs.mwrap.boost.tokenizer;
import mcpplibs.mwrap.boost.optional;

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

// libs/bimap/example/population_bimap.cpp
namespace ex_population_bimap {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------





using namespace boost::bimaps;
using namespace boost;
using namespace std;

int example_main()
{
    {

    typedef bimap<

        string,
        multiset_of< boost::optional<string> >

    > bm_type;

    bm_type bm;

    assign::insert( bm )

        ( "John" , string("lazarus" ) )
        ( "Peter", string("vinicius") )
        ( "Simon", string("vinicius") )
        ( "Brian", none               )
    ;

    ex_out << "John is working in "
         << bm.left.at( "John" ).get_value_or( "no project" )
         << endl;

    ex_out << "Project vinicius is being developed by " << endl;
    for( bm_type::right_reference rp : boost::make_iterator_range( bm.right.equal_range( std::string("vinicius") ) ) )
    {
        ex_out << rp.second << endl;
    }

    ex_out << "This workers need a project " << endl;
    for( bm_type::right_reference rp : boost::make_iterator_range( bm.right.equal_range(none) ) )
    {
        ex_out << rp.second << endl;
    }

}

    //[ code_population_bimap

    typedef bimap<

        unordered_set_of< std::string >,
        multiset_of< long, std::greater<long> >

    > population_bimap;

    typedef population_bimap::value_type population;

    population_bimap pop;
    pop.insert( population("China",          1321000000) );
    pop.insert( population("India",          1129000000) );
    pop.insert( population("United States",   301950000) );
    pop.insert( population("Indonesia",       234950000) );
    pop.insert( population("Brazil",          186500000) );
    pop.insert( population("Pakistan",        163630000) );

    ex_out << "Countries by their population:" << std::endl;

    // First requirement
    /*<< The right map view works like a
         `std::multimap< long, std::string, std::greater<long> >`,
         We can iterate over it to print the results in the required order. >>*/
    for( population_bimap::right_const_iterator
            i = pop.right.begin(), iend = pop.right.end();
            i != iend ; ++i )
    {
        ex_out << i->second << " with " << i->first << std::endl;
    }

    // Second requirement
    /*<< The left map view works like a `std::unordered_map< std::string, long >`,
         given the name of the country we can use it to search for the population 
         in constant time >>*/
    ex_out << "Population of China: " << pop.left.at("China") << std::endl;
    //]

    return 0;
}

TEST_CASE("example: population_bimap") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(John is working in lazarus
Project vinicius is being developed by 
Peter
Simon
This workers need a project 
Brian
Countries by their population:
China with 1321000000
India with 1129000000
United States with 301950000
Indonesia with 234950000
Brazil with 186500000
Pakistan with 163630000
Population of China: 1321000000
)ex"));
}
}  // namespace ex_population_bimap

// libs/bimap/example/mi_to_b_path/mi_hashed_indices.cpp
namespace ex_mi_hashed_indices {
//[ code_mi_to_b_path_mi_hashed_indices




using namespace boost::multi_index;
namespace bl = boost::lambda;

// word_counter keeps the occurrences of words inserted. A hashed
// index allows for fast checking of preexisting entries.

struct word_counter_entry
{
    std::string  word;
    unsigned int occurrences;

    word_counter_entry( std::string word_ ) : word(word_), occurrences(0) {}
};

typedef multi_index_container
<
    word_counter_entry,
    indexed_by
    <
        ordered_non_unique
        <
            boost::multi_index::member< word_counter_entry, unsigned int, &word_counter_entry::occurrences >,
            std::greater<unsigned int>
        >,
        hashed_unique
        <
            boost::multi_index::member< word_counter_entry, std::string, &word_counter_entry::word >
        >
  >

> word_counter;

typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

int example_main()
{
    std::string text=
        "En un lugar de la Mancha, de cuyo nombre no quiero acordarme... "
        "...snip..."
        "...no se salga un punto de la verdad.";

    // feed the text into the container

    word_counter   wc;
    text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));
    unsigned int   total_occurrences = 0;

    for( text_tokenizer::iterator it = tok.begin(), it_end = tok.end();
         it != it_end ; ++it )
    {
        ++total_occurrences;
        word_counter::iterator wit = wc.insert(*it).first;
        wc.modify_key( wit, ++ bl::_1 );
    }

    // list words by frequency of appearance

    ex_out << std::fixed << std::setprecision(2);

    for( word_counter::iterator wit = wc.begin(), wit_end=wc.end();
         wit != wit_end; ++wit )
    {
        ex_out << std::setw(11) << wit->word << ": "
                  << std::setw(5)
                  << 100.0 * wit->occurrences / total_occurrences << "%"
                  << std::endl;
    }

    return 0;
}
//]

TEST_CASE("example: mi_hashed_indices") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(         de: 14.29%
         no:  9.52%
         un:  9.52%
         la:  9.52%
         En:  4.76%
      lugar:  4.76%
     Mancha:  4.76%
       cuyo:  4.76%
     nombre:  4.76%
     quiero:  4.76%
  acordarme:  4.76%
       snip:  4.76%
         se:  4.76%
      salga:  4.76%
      punto:  4.76%
     verdad:  4.76%
)ex"));
}
}  // namespace ex_mi_hashed_indices

// libs/bimap/example/bimap_and_boost/assign.cpp
namespace ex_assign {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------





using namespace boost::bimaps;
using namespace boost;


int example_main()
{
    //[ code_bimap_and_boost_assign

    typedef bimap< multiset_of< int >, list_of< std::string > > bm_type;

    // We can use assign::list_of to initialize the container.

    bm_type bm = assign::list_of< bm_type::relation > /*<
        Note that `bm_type::relation` has to be used instead of `bm_type::value_type`.
        Contrary to `value_type`, `relation` type stores the elements as non const, a
        requirement of `assign::list_of` >*/
        ( 1, "one"   )
        ( 2, "two"   )
        ( 3, "three" );

    // The left map view is a multiset, again we use insert

    assign::insert( bm.left )
        ( 4, "four" )
        ( 5, "five" )
        ( 6, "six"  );

    // The right map view is a list so we use push_back here
    // Note the order of the elements in the list!

    assign::push_back( bm.right )
        ( "seven" , 7 )
        ( "eight" , 8 );

    assign::push_front( bm.right )
        ( "nine"  ,  9 )
        ( "ten"   , 10 )
        ( "eleven", 11 );

   // Since it is left_based the main view is a multiset, so we use insert

    assign::insert( bm )
        ( 12, "twelve"   )
        ( 13, "thirteen" );
    //]

    return 0;
}

TEST_CASE("example: assign") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_assign

// libs/bimap/example/bimap_and_boost/lambda.cpp
namespace ex_lambda {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

int example_main()
{
    //[ code_bimap_and_boost_lambda

    typedef bimap< std::string, int > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type("one",1) );
    bm.insert( bm_type::value_type("two",2) );

    bm.right.range( 5 < _key, _key < 10 );

    bm.left.modify_key( bm.left.find("one"), _key = "1" );

    bm.left.modify_data( bm.left.begin(), _data *= 10 );
    //]
    return 0;
}

TEST_CASE("example: lambda") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_lambda

// libs/bimap/example/bimap_and_boost/property_map.cpp
namespace ex_property_map {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

//[ code_bimap_and_boost_property_map

template <typename AddressMap>
void foo(AddressMap & address_map)
{
    typedef typename boost::property_traits<AddressMap>::value_type value_type;
    typedef typename boost::property_traits<AddressMap>::key_type key_type;

    value_type address;
    key_type fred = "Fred";
    ex_out << boost::get(address_map, fred);
}

int example_main()
{
    typedef bimap<std::string, multiset_of<std::string> > Name2Address;
    typedef Name2Address::value_type location;

    Name2Address name2address;
    name2address.insert( location("Fred", "710 West 13th Street") );
    name2address.insert( location( "Joe", "710 West 13th Street") );

    foo( name2address.left );

    return 0;
}
//]

TEST_CASE("example: property_map") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(710 West 13th Street)ex"));
}
}  // namespace ex_property_map

// libs/bimap/example/bimap_and_boost/range.cpp
namespace ex_range {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




//[ code_bimap_and_boost_range_functions

template< class ForwardReadableRange, class UnaryFunctor >
UnaryFunctor for_each(const ForwardReadableRange & r, UnaryFunctor func)
{
    typedef typename 
    boost::range_const_iterator<ForwardReadableRange>::type const_iterator;

    for(const_iterator i= boost::begin(r), iend= boost::end(r); i!=iend; ++i )
    {
        func(*i);
    }

    return func;
}

template< class ForwardReadableRange, class Predicate >
typename boost::range_difference<ForwardReadableRange>::type
    count_if(const ForwardReadableRange & r, Predicate pred)
{
    typedef typename
    boost::range_const_iterator<ForwardReadableRange>::type const_iterator;

    typename boost::range_difference<ForwardReadableRange>::type c = 0;

    for( const_iterator i = boost::begin(r), iend = boost::end(r); i != iend; ++i )
    {
        if( pred(*i) ) ++c;
    }

    return c;
}
//]


using namespace boost::bimaps;
using namespace boost;
// boost/bind.hpp also makes boost::placeholders visible globally, which a
// module cannot do; the module consumer writes the directive (the
// upstream-recommended spelling).
using namespace boost::placeholders;

//[ code_bimap_and_boost_range

struct pair_printer
{
    pair_printer(std::ostream & o) : os(o) {}
    template< class Pair >
    void operator()(const Pair & p)
    {
        os << "(" << p.first << "," << p.second << ")";
    }
    private:
    std::ostream & os;
};

struct second_extractor
{
    template< class Pair >
    const typename Pair::second_type & operator()(const Pair & p)
    {
        return p.second;
    }
};

int example_main()
{
    typedef bimap< double, multiset_of<int> > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type(2.5 , 1) );
    bm.insert( bm_type::value_type(3.1 , 2) );
    //...
    bm.insert( bm_type::value_type(6.4 , 4) );
    bm.insert( bm_type::value_type(1.7 , 2) );

    // Print all the elements of the left map view

    for_each( bm.left, pair_printer(ex_out) );

    // Print a range of elements of the right map view

    for_each( bm.right.range( 2 <= _key, _key < 6 ), pair_printer(ex_out) );

    // Count the number of elements where the data is equal to 2 from a
    // range of elements of the left map view

    ex_range::count_if( bm.left.range( 2.3 < _key, _key < 5.4 ),
              bind<int>( second_extractor(), _1 ) == 2 );

    return 0;
}
//]

TEST_CASE("example: range") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((1.7,2)(2.5,1)(3.1,2)(6.4,4)(2,3.1)(2,1.7)(4,6.4))ex"));
}
}  // namespace ex_range

// libs/bimap/example/bimap_and_boost/serialization.cpp
namespace ex_serialization {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------





using namespace boost::bimaps;

int example_main()
{
    //[ code_bimap_and_boost_serialization

    typedef bimap< std::string, int > bm_type;

    // Create a bimap and serialize it to a file
    {
        bm_type bm;
        bm.insert( bm_type::value_type("one",1) );
        bm.insert( bm_type::value_type("two",2) );

        std::ofstream ofs((std::filesystem::temp_directory_path() / "boost-m-bimap-serialization.data").string());
        boost::archive::text_oarchive oa(ofs);

        oa << const_cast<const bm_type&>(bm); /*<
            We must do a const cast because Boost.Serialization archives
            only save const objects. Read Boost.Serializartion docs for the
            rationale behind this decision >*/

        /*<< We can only serialize iterators if the bimap was serialized first.
             Note that the const cast is not required here because we create
             our iterators as const. >>*/
        const bm_type::left_iterator left_iter = bm.left.find("two");
        oa << left_iter;

        const bm_type::right_iterator right_iter = bm.right.find(1);
        oa << right_iter;
    }

    // Load the bimap back
    {
        bm_type bm;

        std::ifstream ifs((std::filesystem::temp_directory_path() / "boost-m-bimap-serialization.data").string(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);

        ia >> bm;

        assert( bm.size() == 2 );

        bm_type::left_iterator left_iter;
        ia >> left_iter;

        assert( left_iter->first == "two" );

        bm_type::right_iterator right_iter;
        ia >> right_iter;

        assert( right_iter->first == 1 );
    }
    //]

    return 0;
}

TEST_CASE("example: serialization") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_serialization
