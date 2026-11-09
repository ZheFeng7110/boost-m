// mcpplibs.mwrap.boost.bimap — the official examples (libs/bimap/example/*.cpp,
// mi_to_b_path/*.cpp) ported mechanically: each program runs as a case and its
// output must equal what the original printed when built from upstream/boost
// alone. Interactive examples read a fixed stdin. population_bimap.cpp,
// mi_hashed_indices.cpp and the bimap_and_boost/ examples are
// examples_with_boost.cpp and include_with_boost.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cassert>
import std;
import mcpplibs.mwrap.boost.bimap;
import mcpplibs.mwrap.boost.tokenizer;

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

// libs/bimap/example/at_function_examples.cpp
namespace ex_at_function_examples {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

void first_bimap()
{
   //[ code_at_function_first

    typedef bimap< set_of< std::string >, list_of< int > > bm_type;
    bm_type bm;

    try
    {
        bm.left.at("one") = 1; // throws std::out_of_range
    }
    catch( std::out_of_range & e ) {}

    assert( bm.empty() );

    bm.left["one"] = 1; // Ok

    assert( bm.left.at("one") == 1 ); // Ok
    //]
}

void second_bimap()
{
    //[ code_at_function_second

    typedef bimap< multiset_of<std::string>, unordered_set_of<int> > bm_type;
    bm_type bm;

    //<-
    /*
    //->
    bm.right[1] = "one"; // compilation error
    //<-
    */
    //->

    bm.right.insert( bm_type::right_value_type(1,"one") );

    assert( bm.right.at(1) == "one" ); // Ok

    try
    {
        ex_out << bm.right.at(2); // throws std::out_of_range
    }
    catch( std::out_of_range & e ) {}

    //<-
    /*
    //->
    bm.right.at(1) = "1"; // compilation error
    //<-
    */
    //->

    //]
}

int example_main()
{
    first_bimap();
    second_bimap();
    return 0;
}

TEST_CASE("example: at_function_examples") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_at_function_examples

// libs/bimap/example/mi_to_b_path/bidirectional_map.cpp
namespace ex_bidirectional_map {
using namespace boost::bimaps;

// A dictionary is a bidirectional map from strings to strings

typedef bimap<std::string,std::string> dictionary;
typedef dictionary::value_type translation;

int example_main()
{
    dictionary d;

    // Fill up our microdictionary.
    // first members Spanish, second members English.

    d.insert( translation("hola" ,"hello"  ));
    d.insert( translation("adios","goodbye"));
    d.insert( translation("rosa" ,"rose"   ));
    d.insert( translation("mesa" ,"table"  ));

    ex_out << "enter a word" << std::endl;
    std::string word;
    std::getline(ex_in,word);

    // search the queried word on the from index (Spanish)

    dictionary::left_const_iterator it = d.left.find(word);

    if( it != d.left.end() )
    {
        // the second part of the element is the equivalent in English

        ex_out << word << " is said "
                  << it->second /*< `it` is an iterator of the left view, so 
                                    `it->second` refers to the right element of
                                     the relation, the word in english >*/
                  << " in English" << std::endl;
    }
    else
    {
        // word not found in Spanish, try our luck in English

        dictionary::right_const_iterator it2 = d.right.find(word);
        if( it2 != d.right.end() )
        {
            ex_out << word << " is said "
                      << it2->second /*< `it2` is an iterator of the right view, so
                                         `it2->second` refers to the left element of
                                          the relation, the word in spanish >*/
                      << " in Spanish" << std::endl;
        }
        else
        {
            ex_out << "No such word in the dictionary" << std::endl;
        }
    }

    return 0;
}
//]

TEST_CASE("example: bidirectional_map") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hola
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(enter a word
hola is said hello in English
)ex"));
}
}  // namespace ex_bidirectional_map

// libs/bimap/example/mi_to_b_path/hashed_indices.cpp
namespace ex_hashed_indices {
using namespace boost::bimaps;

struct word        {};
struct occurrences {};

typedef bimap
<
    
     multiset_of< tagged<unsigned int,occurrences>, std::greater<unsigned int> >, 
unordered_set_of< tagged< std::string,       word>                             >

> word_counter;

typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

int example_main()
{

    std::string text=
        "Relations between data in the STL are represented with maps."
        "A map is a directed relation, by using it you are representing "
        "a mapping. In this directed relation, the first type is related to "
        "the second type but it is not true that the inverse relationship "
        "holds. This is useful in a lot of situations, but there are some "
        "relationships that are bidirectional by nature.";

    // feed the text into the container

    word_counter   wc;
    text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));
    unsigned int   total_occurrences = 0;

    for( text_tokenizer::const_iterator it = tok.begin(), it_end = tok.end();
         it != it_end ; ++it )
    {
        ++total_occurrences;

        word_counter::map_by<occurrences>::iterator wit =
            wc.by<occurrences>().insert(
                 word_counter::map_by<occurrences>::value_type(0,*it)
            ).first;

        wc.by<occurrences>().modify_key( wit, ++_key);
    }

    // list words by frequency of appearance

    ex_out << std::fixed << std::setprecision(2);

    for( word_counter::map_by<occurrences>::const_iterator
            wit     = wc.by<occurrences>().begin(),
            wit_end = wc.by<occurrences>().end();

         wit != wit_end; ++wit )
    {
        ex_out << std::setw(15) << wit->get<word>() << ": "
                  << std::setw(5)
                  << 100.0 * wit->get<occurrences>() / total_occurrences << "%"
                  << std::endl;
    }

    return 0;
}
//]

TEST_CASE("example: hashed_indices") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(            the:  6.15%
             is:  6.15%
            are:  6.15%
              a:  4.62%
       directed:  3.08%
       relation:  3.08%
           type:  3.08%
             it:  3.08%
             in:  3.08%
            but:  3.08%
           that:  3.08%
             by:  3.08%
      Relations:  1.54%
        between:  1.54%
           data:  1.54%
            STL:  1.54%
    represented:  1.54%
           with:  1.54%
           maps:  1.54%
              A:  1.54%
            map:  1.54%
          using:  1.54%
            you:  1.54%
   representing:  1.54%
        mapping:  1.54%
             In:  1.54%
           this:  1.54%
          first:  1.54%
        related:  1.54%
             to:  1.54%
         second:  1.54%
            not:  1.54%
           true:  1.54%
        inverse:  1.54%
   relationship:  1.54%
          holds:  1.54%
           This:  1.54%
         useful:  1.54%
            lot:  1.54%
             of:  1.54%
     situations:  1.54%
          there:  1.54%
           some:  1.54%
  relationships:  1.54%
  bidirectional:  1.54%
         nature:  1.54%
)ex"));
}
}  // namespace ex_hashed_indices

// libs/bimap/example/mi_to_b_path/mi_bidirectional_map.cpp
namespace ex_mi_bidirectional_map {
//[ code_mi_to_b_path_mi_bidirectional_map



using namespace boost;
using namespace boost::multi_index;

// tags for accessing both sides of a bidirectional map

struct from {};
struct to   {};

// The class template bidirectional_map wraps the specification
// of a bidirectional map based on multi_index_container.

template<typename FromType,typename ToType>
struct bidirectional_map
{
    typedef std::pair<FromType,ToType> value_type;

    typedef multi_index_container<
        value_type,
        indexed_by
        <
            ordered_unique
            <
                tag<from>, member<value_type,FromType,&value_type::first>
            >,
            ordered_unique
            <
                tag<to>, member<value_type,ToType,&value_type::second>
            >
        >

  > type;

};

// A dictionary is a bidirectional map from strings to strings

typedef bidirectional_map<std::string,std::string>::type dictionary;

int example_main()
{
    dictionary d;

    // Fill up our microdictionary.
    // first members Spanish, second members English.

    d.insert(dictionary::value_type("hola","hello"));
    d.insert(dictionary::value_type("adios","goodbye"));
    d.insert(dictionary::value_type("rosa","rose"));
    d.insert(dictionary::value_type("mesa","table"));

    ex_out << "enter a word" << std::endl;
    std::string word;
    std::getline(ex_in,word);

    // search the queried word on the from index (Spanish)

    dictionary::iterator it = d.get<from>().find(word);

    if( it != d.end() )
    {
        // the second part of the element is the equivalent in English

        ex_out << word << " is said "
                  << it->second << " in English" << std::endl;
    }
    else
    {
        // word not found in Spanish, try our luck in English

        dictionary::index_iterator<to>::type it2 = d.get<to>().find(word);
        if( it2 != d.get<to>().end() )
        {
            ex_out << word << " is said "
                      << it2->first << " in Spanish" << std::endl;
        }
        else
        {
            ex_out << "No such word in the dictionary" << std::endl;
        }
    }

    return 0;
}
//]

TEST_CASE("example: mi_bidirectional_map") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(table
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(enter a word
table is said mesa in Spanish
)ex"));
}
}  // namespace ex_mi_bidirectional_map

// libs/bimap/example/mi_to_b_path/tagged_bidirectional_map.cpp
namespace ex_tagged_bidirectional_map {
using namespace boost::bimaps;

// tags

struct spanish {};
struct english {};

// A dictionary is a bidirectional map from strings to strings

typedef bimap
<
    tagged< std::string,spanish >, tagged< std::string,english >

> dictionary;

typedef dictionary::value_type translation;

int example_main()
{
    dictionary d;

    // Fill up our microdictionary. 
    // first members Spanish, second members English.

    d.insert( translation("hola" ,"hello"  ));
    d.insert( translation("adios","goodbye"));
    d.insert( translation("rosa" ,"rose"   ));
    d.insert( translation("mesa" ,"table"  ));

    ex_out << "enter a word" << std::endl;
    std::string word;
    std::getline(ex_in,word);

    // search the queried word on the from index (Spanish) */

    dictionary::map_by<spanish>::const_iterator it =
        d.by<spanish>().find(word);

    if( it != d.by<spanish>().end() )
    {
        ex_out << word << " is said " 
                  << it->get<english>() << " in English" << std::endl;
    }
    else
    {
        // word not found in Spanish, try our luck in English

        dictionary::map_by<english>::const_iterator it2 =
            d.by<english>().find(word);

        if( it2 != d.by<english>().end() )
        {
            ex_out << word << " is said "
                      << it2->get<spanish>() << " in Spanish" << std::endl;
        }
        else
        {
            ex_out << "No such word in the dictionary" << std::endl;
        }
    }

    return 0;
}
//]

TEST_CASE("example: tagged_bidirectional_map") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(goodbye
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(enter a word
goodbye is said adios in Spanish
)ex"));
}
}  // namespace ex_tagged_bidirectional_map

// libs/bimap/example/mighty_bimap.cpp
namespace ex_mighty_bimap {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------
// This is the translator example from the tutorial.
// In this example the set type of relation is changed to allow the iteration
// of the container.


//[ code_mighty_bimap


struct english {};
struct spanish {};

int example_main()
{
    using namespace boost::bimaps;

    typedef bimap
    <
        unordered_set_of< tagged< std::string, spanish > >,
        unordered_set_of< tagged< std::string, english > >,
        list_of_relation

    > translator;

    translator trans;

    // We have to use `push_back` because the collection of relations is
    // a `list_of_relation`

    trans.push_back( translator::value_type("hola"  ,"hello"   ) );
    trans.push_back( translator::value_type("adios" ,"goodbye" ) );
    trans.push_back( translator::value_type("rosa"  ,"rose"    ) );
    trans.push_back( translator::value_type("mesa"  ,"table"   ) );

    ex_out << "enter a word" << std::endl;
    std::string word;
    std::getline(ex_in,word);

    // Search the queried word on the from index (Spanish)

    translator::map_by<spanish>::const_iterator is
        = trans.by<spanish>().find(word);

    if( is != trans.by<spanish>().end() )
    {
        ex_out << word << " is said "
                  << is->get<english>()
                  << " in English" << std::endl;
    }
    else
    {
        // Word not found in Spanish, try our luck in English

        translator::map_by<english>::const_iterator ie
            = trans.by<english>().find(word);

        if( ie != trans.by<english>().end() )
        {
            ex_out << word << " is said "
                      << ie->get<spanish>()
                      << " in Spanish" << std::endl;
        }
        else
        {
            // Word not found, show the possible translations

            ex_out << "No such word in the dictionary"      << std::endl;
            ex_out << "These are the possible translations" << std::endl;

            for( translator::const_iterator
                    i = trans.begin(),
                    i_end = trans.end();

                    i != i_end ; ++i )
            {
                ex_out << i->get<spanish>()
                          << " <---> "
                          << i->get<english>()
                          << std::endl;
            }
        }
    }
    return 0;
}
//]

TEST_CASE("example: mighty_bimap") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hola
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(enter a word
hola is said hello in English
)ex"));
}
}  // namespace ex_mighty_bimap

// libs/bimap/example/projection.cpp
namespace ex_projection {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------



using namespace boost::bimaps;

void years_example()
{
    //[ code_projection_years

    typedef bimap<std::string,multiset_of<int,std::greater<int> > > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type("John" ,34) );
    bm.insert( bm_type::value_type("Peter",24) );
    bm.insert( bm_type::value_type("Mary" ,12) );

    // Find the name of the next younger person after Peter

    bm_type::left_const_iterator name_iter = bm.left.find("Peter");

    bm_type::right_const_iterator years_iter = bm.project_right(name_iter);

    ++years_iter;

    ex_out << "The next younger person after Peter is " << years_iter->second;
    //]
}

int example_main()
{
    years_example();

    return 0;
}

TEST_CASE("example: projection") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(The next younger person after Peter is Mary)ex"));
}
}  // namespace ex_projection

// libs/bimap/example/repetitions_counter.cpp
namespace ex_repetitions_counter {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

struct counter {
    counter() : c(0) {}
    counter& operator++() { ++c; return *this; }
    unsigned int operator++(int) { return c++; }
    operator const unsigned int() const { return c; }
    private:
    unsigned int c;
};

int example_main()
{
    //[ code_repetitions_counter

    typedef bimap
    <
        unordered_set_of< std::string >,
        list_of< counter > /*< `counter` is an integer that is initialized 
                                in zero in the constructor >*/

    > word_counter;

    typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

    std::string text=
        "Relations between data in the STL are represented with maps."
        "A map is a directed relation, by using it you are representing "
        "a mapping. In this directed relation, the first type is related to "
        "the second type but it is not true that the inverse relationship "
        "holds. This is useful in a lot of situations, but there are some "
        "relationships that are bidirectional by nature.";

    // feed the text into the container
    word_counter   wc;
    text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));

    for( text_tokenizer::const_iterator it = tok.begin(), it_end = tok.end();
         it != it_end ; ++it )
    {
        /*<< Because the right collection type is `list_of`, the right data 
             is not used a key and can be modified in the same way as with
             standard maps. >>*/
        ++ wc.left[*it];
    }

    // list words with counters by order of appearance
    /*<< When we insert the elements using the left map view, the element 
         is inserted at the end of the list. >>*/
    for( word_counter::right_const_iterator
            wit = wc.right.begin(), wit_end = wc.right.end();

         wit != wit_end; ++wit )
    {
        ex_out << wit->second << ": " << wit->first;
    }
    //]

    return 0;
}

TEST_CASE("example: repetitions_counter") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Relations: 1between: 1data: 1in: 2the: 4STL: 1are: 4represented: 1with: 1maps: 1A: 1map: 1is: 4a: 3directed: 2relation: 2by: 2using: 1it: 2you: 1representing: 1mapping: 1In: 1this: 1first: 1type: 2related: 1to: 1second: 1but: 2not: 1true: 1that: 2inverse: 1relationship: 1holds: 1This: 1useful: 1lot: 1of: 1situations: 1there: 1some: 1relationships: 1bidirectional: 1nature: 1)ex"));
}
}  // namespace ex_repetitions_counter

// libs/bimap/example/simple_bimap.cpp
namespace ex_simple_bimap {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------


//[ code_simple_bimap



template< class MapType >
void print_map(const MapType & map,
               const std::string & separator,
               std::ostream & os )
{
    typedef typename MapType::const_iterator const_iterator;

    for( const_iterator i = map.begin(), iend = map.end(); i != iend; ++i )
    {
        os << i->first << separator << i->second << std::endl;
    }
}

int example_main()
{
    // Soccer World cup

    typedef boost::bimap< std::string, int > results_bimap;
    typedef results_bimap::value_type position;

    results_bimap results;
    results.insert( position("Argentina"    ,1) );
    results.insert( position("Spain"        ,2) );
    results.insert( position("Germany"      ,3) );
    results.insert( position("France"       ,4) );

    ex_out << "The number of countries is " << results.size()
              << std::endl;

    ex_out << "The winner is " << results.right.at(1)
              << std::endl
              << std::endl;

    ex_out << "Countries names ordered by their final position:"
              << std::endl;

    // results.right works like a std::map< int, std::string >

    print_map( results.right, ") ", ex_out );

    ex_out << std::endl
              << "Countries names ordered alphabetically along with"
                    "their final position:"
              << std::endl;

    // results.left works like a std::map< std::string, int >

    print_map( results.left, " ends in position ", ex_out );

    return 0;
}
//]

TEST_CASE("example: simple_bimap") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(The number of countries is 4
The winner is Argentina

Countries names ordered by their final position:
1) Argentina
2) Spain
3) Germany
4) France

Countries names ordered alphabetically along withtheir final position:
Argentina ends in position 1
France ends in position 4
Germany ends in position 3
Spain ends in position 2
)ex"));
}
}  // namespace ex_simple_bimap

// libs/bimap/example/standard_map_comparison.cpp
namespace ex_standard_map_comparison {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

//[ code_standard_map_comparison

template< class Map, class CompatibleKey, class CompatibleData >
void use_it( Map & m,
             const CompatibleKey  & key,
             const CompatibleData & data )
{
    typedef typename Map::value_type value_type;
    typedef typename Map::const_iterator const_iterator;

    m.insert( value_type(key,data) );
    const_iterator iter = m.find(key);
    if( iter != m.end() )
    {
        assert( iter->first  == key  );
        assert( iter->second == data );

        ex_out << iter->first << " --> " << iter->second;
    }
    m.erase(key);
}

int example_main()
{
    typedef bimap< set_of<std::string>, set_of<int> > bimap_type;
    bimap_type bm;

    // Standard map
    {
        typedef std::map< std::string, int > map_type;
        map_type m;

        use_it( m, "one", 1 );
    }

    // Left map view
    {
        typedef bimap_type::left_map map_type;
        map_type & m = bm.left;

        use_it( m, "one", 1 );
    }

    // Reverse standard map
    {
        typedef std::map< int, std::string > reverse_map_type;
        reverse_map_type rm;

        use_it( rm, 1, "one" );
    }

    // Right map view
    {
        typedef bimap_type::right_map reverse_map_type;
        reverse_map_type & rm = bm.right;

        use_it( rm, 1, "one" );
    }

    return 0;
}
//]

TEST_CASE("example: standard_map_comparison") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(one --> 1one --> 11 --> one1 --> one)ex"));
}
}  // namespace ex_standard_map_comparison

// libs/bimap/example/step_by_step.cpp
namespace ex_step_by_step {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------



// A convenience header is available in the boost directory:

int example_main()
{
    //[ code_step_by_step_definition

    typedef boost::bimap< int, std::string > bm_type;
    bm_type bm;
    //]

    //[ code_step_by_step_set_of_relations_view

    bm.insert( bm_type::value_type(1, "one" ) );
    bm.insert( bm_type::value_type(2, "two" ) );

    ex_out << "There are " << bm.size() << "relations" << std::endl;

    for( bm_type::const_iterator iter = bm.begin(), iend = bm.end(); 
        iter != iend; ++iter )
    {
        // iter->left  : data : int
        // iter->right : data : std::string

        ex_out << iter->left << " <--> " << iter->right << std::endl;
    }
    //]

    //[ code_step_by_step_left_map_view

    /*<< The type of `bm.left` is `bm_type::left_map` and the type
         of `bm.right` is `bm_type::right_map` >>*/
    typedef bm_type::left_map::const_iterator left_const_iterator;

    for( left_const_iterator left_iter = bm.left.begin(), iend = bm.left.end();
         left_iter != iend; ++left_iter )
    {
        // left_iter->first  : key  : int
        // left_iter->second : data : std::string

        ex_out << left_iter->first << " --> " << left_iter->second << std::endl;
    }

    /*<< `bm_type::left_`\ -type- can be used as a shortcut for the more verbose
    `bm_type::left_map::`\ -type- >>*/ 
    bm_type::left_const_iterator left_iter = bm.left.find(2);
    assert( left_iter->second ==  "two" );

    /*<< This line produces the same effect of
         `bm.insert( bm_type::value_type(3,"three") );` >>*/
    bm.left.insert( bm_type::left_value_type( 3, "three" ) );
    //]



    //[ code_step_by_step_right_map_view

    bm_type::right_const_iterator right_iter = bm.right.find("two");

    // right_iter->first  : key  : std::string
    // right_iter->second : data : int

    assert( right_iter->second ==  2 );

    assert( bm.right.at("one") == 1 );

    bm.right.erase("two");

    /*<< This line produces the same effect of
         `bm.insert( bm_type::value_type(4,"four") );` >>*/
    bm.right.insert( bm_type::right_value_type( "four", 4 ) );
    //]

    return 0;
}

TEST_CASE("example: step_by_step") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(There are 2relations
1 <--> one
2 <--> two
1 --> one
2 --> two
)ex"));
}
}  // namespace ex_step_by_step

// libs/bimap/example/tagged_simple_bimap.cpp
namespace ex_tagged_simple_bimap {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------


//[ code_tagged_simple_bimap



struct country  {};
struct place    {};

int example_main()
{
    using namespace boost::bimaps;

    // Soccer World cup.

    typedef bimap
    <
        tagged< std::string, country >,
        tagged< int        , place   >

    > results_bimap;

    typedef results_bimap::value_type position;

    results_bimap results;
    results.insert( position("Argentina"    ,1) );
    results.insert( position("Spain"        ,2) );
    results.insert( position("Germany"      ,3) );
    results.insert( position("France"       ,4) );

    ex_out << "Countries names ordered by their final position:"
                << std::endl;

    /*<< `results.by<place>()` is equivalent to `results.right` >>*/
    for( results_bimap::map_by<place>::const_iterator
            i    = results.by<place>().begin(),
            iend = results.by<place>().end() ;
            i != iend; ++i )
    {
        /*<< `get<Tag>` works for each view of the bimap >>*/
        ex_out << i->get<place  >() << ") "
                  << i->get<country>() << std::endl;
    }

    ex_out << std::endl
              << "Countries names ordered alfabetically along with"
                 "their final position:"
              << std::endl;

    /*<< `results.by<country>()` is equivalent to `results.left` >>*/
    for( results_bimap::map_by<country>::const_iterator
            i    = results.by<country>().begin(),
            iend = results.by<country>().end() ;
            i != iend; ++i )
    {
        ex_out << i->get<country>() << " ends "
                  << i->get<place  >() << "º"
                  << std::endl;
    }

    return 0;
}
//]

TEST_CASE("example: tagged_simple_bimap") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Countries names ordered by their final position:
1) Argentina
2) Spain
3) Germany
4) France

Countries names ordered alfabetically along withtheir final position:
Argentina ends 1º
France ends 4º
Germany ends 3º
Spain ends 2º
)ex"));
}
}  // namespace ex_tagged_simple_bimap

// libs/bimap/example/tutorial_info_hook.cpp
namespace ex_tutorial_info_hook {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;


void tutorial_about_info_hook()
{
    //[ code_tutorial_info_hook_first

    typedef bimap<

        multiset_of< std::string >, // author
             set_of< std::string >, // title

          with_info< std::string >  // abstract

    > bm_type;
    typedef bm_type::value_type book;

    bm_type bm;

    bm.insert(

        book( "Bjarne Stroustrup"   , "The C++ Programming Language",

              "For C++ old-timers, the first edition of this book is"
              "the one that started it all—the font of our knowledge." )
    );


    // Print the author of the bible
    ex_out << bm.right.at("The C++ Programming Language");

    // Print the abstract of this book
    bm_type::left_iterator i = bm.left.find("Bjarne Stroustrup");
    ex_out << i->info;
    //]

    // Contrary to the two key types, the information will be mutable
    // using iterators.

    //[ code_tutorial_info_hook_mutable

    i->info += "More details about this book";
    //]

    // A new function is included in unique map views: info_at(key), that
    // mimics the standard at(key) function but returned the associated
    // information instead of the data.

    //[ code_tutorial_info_hook_info_at

    // Print the new abstract
    ex_out << bm.right.info_at("The C++ Programming Language");
    //]
}

struct author {};
struct title {};
struct abstract {};

void tutorial_about_tagged_info_hook()
{
    //[ code_tutorial_info_hook_tagged_info

    typedef bimap<

        multiset_of< tagged< std::string, author   > >,
             set_of< tagged< std::string, title    > >,

          with_info< tagged< std::string, abstract > >

    > bm_type;
    typedef bm_type::value_type book;

    bm_type bm;

    bm.insert(

        book( "Bjarne Stroustrup"   , "The C++ Programming Language",

              "For C++ old-timers, the first edition of this book is"
              "the one that started it all—the font of our knowledge." )
    );

    // Print the author of the bible
    ex_out << bm.by<title>().at("The C++ Programming Language");

    // Print the abstract of this book
    bm_type::map_by<author>::iterator i = bm.by<author>().find("Bjarne Stroustrup");
    ex_out << i->get<abstract>();

    // Contrary to the two key types, the information will be mutable
    // using iterators.

    i->get<abstract>() += "More details about this book";

    // Print the new abstract
    ex_out << bm.by<title>().info_at("The C++ Programming Language");
    //]
}


void bimap_without_an_info_hook()
{
    //[ code_tutorial_info_hook_nothing

    typedef bimap<

        multiset_of< std::string >, // author
             set_of< std::string >  // title

    > bm_type;
    typedef bm_type::value_type book;

    bm_type bm;

    bm.insert( book( "Bjarne Stroustrup"   , "The C++ Programming Language" ) );
    bm.insert( book( "Scott Meyers"        , "Effective C++"                ) );
    bm.insert( book( "Andrei Alexandrescu" , "Modern C++ Design"            ) );

    // Print the author of Modern C++
    ex_out << bm.right.at( "Modern C++ Design" );
    //]
}


int example_main()
{
    tutorial_about_info_hook();
    tutorial_about_tagged_info_hook();
    bimap_without_an_info_hook();

    return 0;
}

TEST_CASE("example: tutorial_info_hook") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Bjarne StroustrupFor C++ old-timers, the first edition of this book isthe one that started it all—the font of our knowledge.For C++ old-timers, the first edition of this book isthe one that started it all—the font of our knowledge.More details about this bookBjarne StroustrupFor C++ old-timers, the first edition of this book isthe one that started it all—the font of our knowledge.For C++ old-timers, the first edition of this book isthe one that started it all—the font of our knowledge.More details about this bookAndrei Alexandrescu)ex"));
}
}  // namespace ex_tutorial_info_hook

// libs/bimap/example/tutorial_modify_and_replace.cpp
namespace ex_tutorial_modify_and_replace {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

void test_replace()
{
    //[ code_tutorial_replace

    typedef bimap< int, std::string > bm_type;
    bm_type bm;

    bm.insert( bm_type::value_type(1,"one") );

    // Replace (1,"one") with (1,"1") using the right map view
    {
        bm_type::right_iterator it = bm.right.find("one");

        bool successful_replace = bm.right.replace_key( it, "1" );

        assert( successful_replace );
    }

    bm.insert( bm_type::value_type(2,"two") );

    // Fail to replace (1,"1") with (1,"two") using the left map view
    {
        assert( bm.size() == 2 );

        bm_type::left_iterator it = bm.left.find(1);

        bool successful_replace = bm.left.replace_data( it, "two" );

        /*<< `it` is still valid here, and the bimap was left unchanged >>*/
        assert( ! successful_replace );
        assert( bm.size() == 2 );
    }
    //]
}

void test_modify()
{
    //[ code_tutorial_modify

    typedef bimap< int, std::string > bm_type;
    bm_type bm;
    bm.insert( bm_type::value_type(1,"one") );

    // Modify (1,"one") to (1,"1") using the right map view
    {
        bm_type::right_iterator it = bm.right.find("one");

        bool successful_modify = bm.right.modify_key( it , _key = "1" );

        assert( successful_modify );
    }

    bm.insert( bm_type::value_type(2,"two") );

    // Fail to modify (1,"1") to (1,"two") using the left map view
    {
        assert( bm.size() == 2 );

        bm_type::left_iterator it = bm.left.find(1);

        bool successful_modify = bm.left.modify_data( it, _data = "two" );

        /*<< `it` is not longer valid and `(1,"1")` is removed from the bimap >>*/
        assert( ! successful_modify );
        assert( bm.size() == 1 );
    }
    //]

    /*
    // Modify (2,"two") to (3,"two") using the set of relations view
    {
        bm_type::iterator it = bm.begin();

        bool successful_modify = bm.modify( it, ++_left );

        assert( successful_modify );
    }
    */
}

int example_main()
{
    test_replace();
    test_modify();
    return 0;
}

TEST_CASE("example: tutorial_modify_and_replace") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_tutorial_modify_and_replace

// libs/bimap/example/tutorial_range.cpp
namespace ex_tutorial_range {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

void using_upper_and_lower_bound()
{
    //[ code_tutorial_range_standard_way

    typedef bimap<int,std::string> bm_type;
    bm_type bm;

    // ...

    bm_type::left_iterator iter_first  = bm.left.lower_bound(20);
    bm_type::left_iterator iter_second = bm.left.upper_bound(50);

    // range [iter_first,iter_second) contains the elements in [20,50]
    //]

    // Subtle changes
    {
        //[ code_tutorial_range_standard_way_subtle_changes

        bm_type::left_iterator iter_first  = bm.left.upper_bound(20);
        bm_type::left_iterator iter_second = bm.left.lower_bound(50);

        // range [iter_first,iter_second) contains the elements in (20,50)
        //]
    }
}

void using_range()
{
    //[ code_tutorial_range

    typedef bimap<int,std::string> bm_type;
    bm_type bm;

    // ...

    /*<< `range_type` is a handy typedef equal to `std::pair<iterator,iterator>`.
         `const_range_type` is provided too, and it is equal to
         `std::pair<const_iterator,const_iterator>`  >>*/
    bm_type::left_range_type r;

    /*<< _key is a Boost.Lambda placeholder. To use it you have to include
        `<boost/bimap/support/lambda.hpp>` >>*/
    r = bm.left.range( 20 <= _key, _key <= 50 ); // [20,50]

    r = bm.left.range( 20 <  _key, _key <  50 ); // (20,50)

    r = bm.left.range( 20 <= _key, _key <  50 ); // [20,50)
    //]

    //[ code_tutorial_range_unbounded

    r = bm.left.range( 20 <= _key, unbounded ); // [20,inf)

    r = bm.left.range( unbounded , _key < 50 ); // (-inf,50)

    /*<< This is equivalent to std::make_pair(s.begin(),s.end()) >>*/
    r = bm.left.range( unbounded , unbounded ); // (-inf,inf)
    //]
}

int example_main()
{
    using_upper_and_lower_bound();
    using_range();

    return 0;
}

TEST_CASE("example: tutorial_range") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_tutorial_range

// libs/bimap/example/unconstrained_collection.cpp
namespace ex_unconstrained_collection {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

int example_main()
{
    // Boost.Bimap
    {
    //[ code_unconstrained_collection_bimap

    typedef bimap< std::string, unconstrained_set_of<int> > bm_type;
    typedef bm_type::left_map map_type;

    bm_type bm;
    map_type & m = bm.left;
    //]

    //[ code_unconstrained_collection_common

    m["one"] = 1;

    assert( m.find("one") != m.end() );

    for( map_type::iterator i = m.begin(), iend = m.end(); i != iend; ++i )
    {
        /*<< The right collection of the bimap is mutable so its elements 
             can be modified using iterators. >>*/
        ++(i->second);
    }

    m.erase("one");
    //]

    m["one"] = 1;
    m["two"] = 2;

    //[ code_unconstrained_collection_only_for_bimap
    typedef map_type::const_iterator const_iterator;
    typedef std::pair<const_iterator,const_iterator> const_range;

    /*<< This range is a model of BidirectionalRange, read the docs of 
         Boost.Range for more information. >>*/
    const_range r = m.range( "one" <= _key, _key <= "two" );
    for( const_iterator i = r.first; i != r.second; ++i )
    {
        ex_out << i->first << "-->" << i->second << std::endl;
    }

    m.modify_key( m.begin(), _key = "1" );
    //]
    }

    // Standard map
    {
    //[ code_unconstrained_collection_map

    typedef std::map< std::string, int > map_type;

    map_type m;
    //]
    }

    return 0;
}

TEST_CASE("example: unconstrained_collection") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(one-->1
two-->2
)ex"));
}
}  // namespace ex_unconstrained_collection

// libs/bimap/example/user_defined_names.cpp
namespace ex_user_defined_names {
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------




using namespace boost::bimaps;

void untagged_version()
{
    //[ code_user_defined_names_untagged_version

    typedef bimap
    <
        multiset_of<std::string>,
        int

    > People;

    People people;

    // ...

    int user_id;
    ex_in >> user_id;

    // people.right : map<id,name>

    People::right_const_iterator id_iter = people.right.find(user_id);
    if( id_iter != people.right.end() )
    {
        // first  : id
        // second : name

        ex_out << "name: " << id_iter->second << std::endl
                  << "id:   " << id_iter->first  << std::endl;
    }
    else
    {
        ex_out << "Unknown id, users are:" << std::endl;

        // people.left : map<name,id>

        for( People::left_const_iterator
                name_iter = people.left.begin(),
                iend      = people.left.end();

             name_iter != iend; ++name_iter )
        {
            // first  : name
            // second : id

            ex_out << "name: " << name_iter->first  << std::endl
                      << "id:   " << name_iter->second << std::endl;
        }
    }
    //]
}

struct id   {};
struct name {};

void tagged_version()
{
    //[ code_user_defined_names_tagged_version

    //<-
    /*
    //->
    struct id   {}; // Tag for the identification number
    struct name {}; // Tag for the name of the person
    //<-
    */
    //->

    typedef bimap
    <
                     tagged< int        , id   >  ,
        multiset_of< tagged< std::string, name > >

    > People;

    People people;

    // ...

    int user_id;
    ex_in >> user_id;

    People::map_by<id>::const_iterator id_iter = people.by<id>().find(user_id);
    if( id_iter != people.by<id>().end() )
    {
        ex_out << "name: " << id_iter->get<name>() << std::endl
                  << "id:   " << id_iter->get<id>()   << std::endl;
    }
    else
    {
        ex_out << "Unknown id, users are:" << std::endl;

        for( People::map_by<name>::const_iterator
                name_iter = people.by<name>().begin(),
                iend      = people.by<name>().end();

            name_iter != iend; ++name_iter )
        {
            ex_out << "name: " << name_iter->get<name>() << std::endl
                      << "id:   " << name_iter->get<id>()   << std::endl;
        }
    }
    //]
}

int example_main()
{
    untagged_version();
    tagged_version();

    return 0;
}

TEST_CASE("example: user_defined_names") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(1
2
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Unknown id, users are:
Unknown id, users are:
)ex"));
}
}  // namespace ex_user_defined_names
