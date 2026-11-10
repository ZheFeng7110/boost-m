// mcpplibs.mwrap.boost.container — the official examples
// (libs/container/example/*.cpp) ported mechanically: each program runs as a
// case and its output must equal what the same code printed when built from
// upstream/boost headers alone. The BOOST_CONTAINER_TRY/CATCH and
// BOOST_RV_REF / BOOST_MOVABLE_BUT_NOT_COPYABLE macros the examples use come
// from their upstream headers (container/detail/workaround.hpp, move/core.hpp),
// included before the imports (docs/usage.md, macros). doc_extended_allocators
// needs the dlmalloc-based allocators, which the package does not build
// (tools/gen_features.py: alloc_lib.c is a C TU): coverage.toml, gap.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cassert>
#include <boost/container/detail/workaround.hpp>
#include <boost/move/core.hpp>
#include <boost/config.hpp>
#include <cstddef>
import std;
import mcpplibs.mwrap.boost.container;
import mcpplibs.mwrap.boost.move;

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

// libs/container/example/doc_custom_deque.cpp
namespace ex_doc_custom_deque {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------------------------
//          'stored_size' option
//--------------------------------------------

   //This option specifies that a deque that will use "unsigned char" as
   //the type to store capacity or size internally.
   typedef deque_options< stored_size<unsigned char> >::type size_option_t;

   //Size-optimized deque is smaller than the default one.
   typedef deque<int, new_allocator<int>, size_option_t > size_optimized_deque_t;
   assert(( sizeof(size_optimized_deque_t) < sizeof(deque<int>) ));

   //Requesting capacity for more elements than representable by "unsigned char"
   //is an error in the size optimized deque.
   bool exception_thrown = false;
   /*<-*/ 
   #ifndef BOOST_NO_EXCEPTIONS
   BOOST_CONTAINER_TRY{ size_optimized_deque_t v(256); } BOOST_CONTAINER_CATCH(...){ exception_thrown = true; } BOOST_CONTAINER_CATCH_END
   #else
   exception_thrown = true;
   #endif   //BOOST_NO_EXCEPTIONS
   /*->*/
   //=try       { size_optimized_deque_t v(256); }
   //=catch(...){ exception_thrown = true;       }

   assert(exception_thrown == true);

//--------------------------------------------
//          'block_size/segment_size' option
//--------------------------------------------

   //This option specifies the desired block size for deque
   typedef deque_options< block_size<128u> >::type block_128_option_t;

   //segment_size is an alias for block_size (an alias for block_size)
   typedef deque_options< segment_size<128u> >::type segment_128_option_t;

   //This deque will allocate blocks of 128 elements 
   typedef deque<int, void, block_128_option_t > block_128_deque_t;
   assert(block_128_deque_t::get_block_size() == 128u);

   //This deque will allocate segments of 128 elements (an alias for block_size)
   typedef deque<int, void, segment_128_option_t > segment_128_deque_t;
   assert(segment_128_deque_t::get_block_size() == 128u);

//--------------------------------------------
//          'block_bytes/segment_bytes' option
//--------------------------------------------

   //This option specifies the maximum block size for deque
   //in bytes
   typedef deque_options< block_bytes<1024u> >::type block_1024_bytes_option_t;

   //This option specifies the maximum segment size for deque
   //in bytes (an alias for block_bytes)
   typedef deque_options< segment_bytes<1024u> >::type segment_1024_bytes_option_t;

   //This deque will allocate blocks of 1024 bytes
   typedef deque<int, void, block_1024_bytes_option_t > block_1024_bytes_deque_t;
   assert(block_1024_bytes_deque_t::get_block_size() == 1024u/sizeof(int));

   //This deque will allocate blocks of 1024 bytes (an alias for block_bytes)
   typedef deque<int, void, segment_1024_bytes_option_t > segment_1024_bytes_deque_t;
   assert(segment_1024_bytes_deque_t::get_block_size() == 1024u/sizeof(int));

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_deque") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_deque

// libs/container/example/doc_custom_devector.cpp
namespace ex_doc_custom_devector {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------------------------
//          'stored_size' option
//--------------------------------------------

   //Specify that a devector will use "unsigned char" as the type to store size/capacity
   typedef devector_options< stored_size<unsigned char> >::type size_option_t;

   //Size-optimized devector is smaller than the default one.
   typedef devector<int, new_allocator<int>, size_option_t > size_optimized_devector_t;
   assert(( sizeof(size_optimized_devector_t) < sizeof(devector<int>) ));

   //Requesting capacity for more elements than representable by "unsigned char" is an error
   bool exception_thrown = false;
   /*<-*/ 
   #ifndef BOOST_NO_EXCEPTIONS
   BOOST_CONTAINER_TRY{ size_optimized_devector_t v(256); } BOOST_CONTAINER_CATCH(...){ exception_thrown = true; } BOOST_CONTAINER_CATCH_END
   #else
   exception_thrown = true;
   #endif   //BOOST_NO_EXCEPTIONS
   /*->*/
   //=try       { size_optimized_devector_t v(256); }
   //=catch(...){ exception_thrown = true;          }
   assert(exception_thrown == true);

//--------------------------------------------
//          'growth_factor' option
//--------------------------------------------

   //Specify that a devector will increase its capacity 50% when reallocating
   typedef devector_options< growth_factor<growth_factor_50> >::type growth_50_option_t;

   //Fill the devector until full capacity is reached
   devector<int, new_allocator<int>, growth_50_option_t > growth_50_dv(5, 0);
   std::size_t old_cap = growth_50_dv.capacity();
   growth_50_dv.resize(old_cap);

   //Now insert an additional item and check the new buffer is 50% bigger
   growth_50_dv.push_back(1);
   assert(growth_50_dv.capacity() == old_cap*3/2);

//--------------------------------------------
//          'relocate_on' option
//--------------------------------------------

   //Specifies that a devector will not reallocate but relocate elements if the free space
   //at one end is exhausted and the total load factor is below the 66% threshold.
   typedef devector_options< relocate_on_66 >::type relocation_66_option_t;

   //Configure devector to have equal free space at both ends
   devector<int, new_allocator<int>, relocation_66_option_t > reloc_66_dv
      (16u, 16u, reserve_only_tag_t());
   old_cap = reloc_66_dv.capacity();
   const std::size_t front_free_cap = reloc_66_dv.front_free_capacity();

   //Fill vector at the back end
   while (reloc_66_dv.back_free_capacity() > 0)
      reloc_66_dv.push_back(0);

   //Front free capacity is intact
   assert(reloc_66_dv.front_free_capacity() == front_free_cap);

   //Now insert new element, values should relocated to the middle as the
   //load factor is near 50%
   reloc_66_dv.push_back(0);
   assert(reloc_66_dv.capacity() == old_cap);
   assert(reloc_66_dv.front_free_capacity() < front_free_cap);

   //Fill the back end again
   while (reloc_66_dv.back_free_capacity() > 0)
      reloc_66_dv.push_back(1);

   //New insertion should reallocate as load factor is higher than 66%
   reloc_66_dv.push_back(-1);
   assert(reloc_66_dv.capacity() > old_cap);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_devector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_devector

// libs/container/example/doc_custom_segtor.cpp
namespace ex_doc_custom_segtor {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------------------------
//          'stored_size' option
//--------------------------------------------

   //This option specifies that a segtor will use "unsigned char" as
   //the type to store capacity or size internally.
   typedef segtor_options< stored_size<unsigned char> >::type size_option_t;

   //Size-optimized segtor is smaller than the default one.
   typedef segtor<int, new_allocator<int>, size_option_t > size_optimized_segtor_t;
   assert(( sizeof(size_optimized_segtor_t) < sizeof(segtor<int>) ));

   //Requesting capacity for more elements than representable by "unsigned char"
   //is an error in the size optimized segtor.
   bool exception_thrown = false;
   /*<-*/ 
   #ifndef BOOST_NO_EXCEPTIONS
   BOOST_CONTAINER_TRY{ size_optimized_segtor_t v(256); } BOOST_CONTAINER_CATCH(...){ exception_thrown = true; } BOOST_CONTAINER_CATCH_END
   #else
   exception_thrown = true;
   #endif   //BOOST_NO_EXCEPTIONS
   /*->*/
   //=try       { size_optimized_segtor_t v(256); }
   //=catch(...){ exception_thrown = true;                  }

   assert(exception_thrown == true);

//--------------------------------------------
//          'block_size/segment_size' option
//--------------------------------------------

   //This option specifies the desired block size for segtor
   typedef segtor_options< block_size<128u> >::type block_128_option_t;

   //segment_size is an alias for block_size (an alias for block_size)
   typedef segtor_options< segment_size<128u> >::type segment_128_option_t;

   //This segtor will allocate blocks of 128 elements 
   typedef segtor<int, void, block_128_option_t > block_128_segtor_t;
   assert(block_128_segtor_t::get_block_size() == 128u);

   //This segtor will allocate segments of 128 elements (an alias for block_size)
   typedef segtor<int, void, segment_128_option_t > segment_128_segtor_t;
   assert(segment_128_segtor_t::get_block_size() == 128u);

//--------------------------------------------
//          'block_bytes/segment_bytes' option
//--------------------------------------------

   //This option specifies the maximum block size for segtor
   //in bytes
   typedef segtor_options< block_bytes<1024u> >::type block_1024_bytes_option_t;

   //This option specifies the maximum segment size for segtor
   //in bytes (an alias for block_bytes)
   typedef segtor_options< segment_bytes<1024u> >::type segment_1024_bytes_option_t;

   //This segtor will allocate blocks of 1024 bytes
   typedef segtor<int, void, block_1024_bytes_option_t > block_1024_bytes_segtor_t;
   assert(block_1024_bytes_segtor_t::get_block_size() == 1024u/sizeof(int));

   //This segtor will allocate blocks of 1024 bytes (an alias for block_bytes)
   typedef segtor<int, void, segment_1024_bytes_option_t > segment_1024_bytes_segtor_t;
   assert(segment_1024_bytes_segtor_t::get_block_size() == 1024u/sizeof(int));

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_segtor") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_segtor

// libs/container/example/doc_custom_small_vector.cpp
namespace ex_doc_custom_small_vector {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------------------------
//          'inplace_alignment' option
//--------------------------------------------

   //This option specifies the desired alignment for the internal value_type
   typedef small_vector_options< inplace_alignment<16u> >::type alignment_16_option_t;

   //Check 16 byte alignment option
   small_vector<int, 10, void, alignment_16_option_t > sv;
   assert(((std::size_t)sv.data() % 16u) == 0);

//--------------------------------------------
//          'growth_factor' option
//--------------------------------------------

   //This option specifies that a vector will increase its capacity 50%
   //each time the previous capacity was exhausted.
   typedef small_vector_options< growth_factor<growth_factor_50> >::type growth_50_option_t;

   //Fill the vector until full capacity is reached
   small_vector<int, 10, void, growth_50_option_t > growth_50_vector(10, 0);
   const std::size_t old_cap = growth_50_vector.capacity();
   growth_50_vector.resize(old_cap);

   //Now insert an additional item and check the new buffer is 50% bigger
   growth_50_vector.push_back(1);
   assert(growth_50_vector.capacity() == old_cap*3/2);

//--------------------------------------------
//          'stored_size' option
//--------------------------------------------

   //This option specifies that a vector that will use "unsigned char" as
   //the type to store capacity or size internally.
   typedef small_vector_options< stored_size<unsigned char> >::type size_option_t;

   //Size-optimized vector is smaller than the default one.
   typedef small_vector<int, 10, new_allocator<int>, size_option_t > size_optimized_vector_t;
   assert((sizeof(size_optimized_vector_t) < sizeof(small_vector<int, 10>)));

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_small_vector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_small_vector

// libs/container/example/doc_custom_static_vector.cpp
namespace ex_doc_custom_static_vector {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------------------------
//          'inplace_alignment' option
//--------------------------------------------

   //This option specifies the desired alignment for value_type
   typedef static_vector_options< inplace_alignment<16u> >::type alignment_16_option_t;

   //Check 16 byte alignment option
   static_vector<int, 10, alignment_16_option_t > sv;
   assert(((std::size_t)sv.data() % 16u) == 0);

//--------------------------------------------
//          'throw_on_overflow' option
//--------------------------------------------

   //This static_vector won't throw on overflow, for maximum performance
   typedef static_vector_options< throw_on_overflow<false> >::type no_throw_options_t;

   //Create static_vector with no throw on overflow
   static_vector<int, 10, no_throw_options_t > sv2;

//--------------------------------------------
//          'stored_size' option
//--------------------------------------------

   //This options specifies that internal `size()` can be represented by an unsigned char
   //instead of the default `std::size_t`.
   typedef static_vector_options< stored_size<unsigned char> >::type stored_size_uchar_t;

   //Create static_vector that internally stores `size()` in an unsigned char
   //because `capacity()` will be less thatn 256.
   static_vector<unsigned char, 10, stored_size_uchar_t> sv3;

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_static_vector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_static_vector

// libs/container/example/doc_custom_string.cpp
namespace ex_doc_custom_string {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------
//    Option "stored_size"
//--------------------------

   //This option specifies that a string that will use "unsigned char" as
   //the type to store capacity or size internally.
   typedef string_options< stored_size<unsigned char> >::type size_option_t;

   //Size-optimized string is smaller than the default one.
   typedef basic_string<char, std::char_traits<char>, void, size_option_t > size_optimized_string_t;
   assert(( sizeof(size_optimized_string_t) < sizeof(basic_string<char>) ));

   //The internal small string optimization buffer is also smaller
   assert(( size_optimized_string_t().capacity() < basic_string<char>().capacity() ));

   //Requesting capacity for more elements than representable by half of the 
   //"unsigned char" max (due to the need to a bit to indicate short/long representation)
   //minus 1 (due to the terminating null) is an error in the size optimized string.
   bool exception_thrown = false;
   /*<-*/ 
   #ifndef BOOST_NO_EXCEPTIONS
   BOOST_CONTAINER_TRY{ size_optimized_string_t v(127, 'a');} BOOST_CONTAINER_CATCH(...) { exception_thrown = true; } BOOST_CONTAINER_CATCH_END
   #else
   exception_thrown = true;
   #endif   //BOOST_NO_EXCEPTIONS
   /*->*/
   //=try       { size_optimized_string_t v(127, 'a'); }
   //=catch(...){ exception_thrown = true;             }

   assert(exception_thrown == true);

//--------------------------
//    Option "inline_chars"
//--------------------------

   //This option specifies the capacity of the internally stored buffer
   //The maximum value due to internal data organization is 127 chars
   typedef string_options< inline_chars<100> >::type inline_chars_option_t;
   typedef basic_string<char, std::char_traits<char>, void, inline_chars_option_t > inline100_string_t;

   //The size of the object will grow accordingly
   assert(( sizeof(inline100_string_t) > sizeof(basic_string<char>) ));
   assert(( sizeof(inline100_string_t) > 100 ));

   //Internal capacity will be at least the specified one
   assert((inline100_string_t().capacity() >= 100));

//--------------------------
//    Option "growth_factor"
//--------------------------

   //This option specifies that a string will increase its capacity 50%
   //each time the previous capacity was exhausted.
   typedef string_options< growth_factor<growth_factor_50> >::type growth_50_option_t;

   //Fill the string until full capacity is reached
   basic_string<char, std::char_traits<char>, void, growth_50_option_t > growth_50_string(5, 0);
   const std::size_t old_cap = growth_50_string.capacity();
   growth_50_string.resize(old_cap);

   //Now insert an additional item and check the new buffer is aproximately
   //50% bigger (rounding due to the null terminator might round down)
   growth_50_string.push_back(1);
   assert(growth_50_string.capacity() == old_cap*3/2);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_string") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_string

// libs/container/example/doc_custom_tree.cpp
namespace ex_doc_custom_tree {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

   //First define several options

//--------------------------------------------
//          'tree_type' option
//--------------------------------------------

   //This option specifies an AVL tree based associative container
   typedef tree_assoc_options< tree_type<avl_tree> >::type AVLTree;

   //This option specifies an Splay tree based associative container
   typedef tree_assoc_options< tree_type<splay_tree> >::type SplayTree;

   //Splay tree based multiset container
   typedef multiset<int, std::less<int>, new_allocator<int>, SplayTree> SplayMultiset;

   //AVLTree based set container
   typedef set<int, std::less<int>, new_allocator<int>, AVLTree> AvlSet;

   AvlSet avl_set;
   avl_set.insert(0);
   assert(avl_set.find(0) != avl_set.end());

   SplayMultiset splay_mset;
   splay_mset.insert(2);
   splay_mset.insert(2);
   assert(splay_mset.count(2) == 2);

//--------------------------------------------
//          'optimize_size' option
//--------------------------------------------

   //This option specifies an AVL tree based associative container
   //disabling node size optimization.
   typedef tree_assoc_options< tree_type<avl_tree>
                             , optimize_size<false> >::type AVLTreeNoSizeOpt;

   //AVLTree based set container without size optimization
   typedef set<int, std::less<int>, new_allocator<int>, AVLTreeNoSizeOpt> AvlSetNoSizeOpt;

   AvlSetNoSizeOpt avl_set_no_szopt;
   avl_set_no_szopt.insert(1);
   avl_set_no_szopt.insert(1);
   assert(avl_set_no_szopt.count(1) == 1);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_tree") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_tree

// libs/container/example/doc_custom_vector.cpp
namespace ex_doc_custom_vector {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

//--------------------------------------------
//          'stored_size' option
//--------------------------------------------

   //This option specifies that a vector that will use "unsigned char" as
   //the type to store capacity or size internally.
   typedef vector_options< stored_size<unsigned char> >::type size_option_t;

   //Size-optimized vector is smaller than the default one.
   typedef vector<int, new_allocator<int>, size_option_t > size_optimized_vector_t;
   assert(( sizeof(size_optimized_vector_t) < sizeof(vector<int>) ));

   //Requesting capacity for more elements than representable by "unsigned char"
   //is an error in the size optimized vector.
   bool exception_thrown = false;
   /*<-*/ 
   #ifndef BOOST_NO_EXCEPTIONS
   BOOST_CONTAINER_TRY{ size_optimized_vector_t v(256); } BOOST_CONTAINER_CATCH(...){ exception_thrown = true; } BOOST_CONTAINER_CATCH_END
   #else
   exception_thrown = true;
   #endif   //BOOST_NO_EXCEPTIONS
   /*->*/
   //=try       { size_optimized_vector_t v(256); }
   //=catch(...){ exception_thrown = true;        }

   assert(exception_thrown == true);

//--------------------------------------------
//          'growth_factor' option
//--------------------------------------------

   //This option specifies that a vector will increase its capacity 50%
   //each time the previous capacity was exhausted.
   typedef vector_options< growth_factor<growth_factor_50> >::type growth_50_option_t;

   //Fill the vector until full capacity is reached
   vector<int, new_allocator<int>, growth_50_option_t > growth_50_vector(5, 0);
   const std::size_t old_cap = growth_50_vector.capacity();
   growth_50_vector.resize(old_cap);

   //Now insert an additional item and check the new buffer is 50% bigger
   growth_50_vector.push_back(1);
   assert(growth_50_vector.capacity() == old_cap*3/2);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_custom_vector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_custom_vector

// libs/container/example/doc_devector.cpp
namespace ex_doc_devector {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

   devector<int> dv;

   //Unlike vector, a devector can grow cheaply (amortized constant time)
   //at *both* ends, while keeping all elements in contiguous memory.
   dv.push_back(2);              // {2}
   dv.push_back(3);              // {2, 3}
   dv.push_front(1);            // {1, 2, 3}
   dv.push_front(0);            // {0, 1, 2, 3}

   assert(dv.size() == 4);
   assert(dv.front() == 0);
   assert(dv.back()  == 3);

   //Random access and contiguous storage, just like vector.
   assert(dv[0] == 0 && dv[3] == 3);
   const int *p = dv.data();
   for(std::size_t i = 0, n = dv.size(); i != n; ++i)
      assert(p[i] == (int)i);

   //We can ask for free capacity at each end independently, so that the
   //following insertions at that end do not trigger a reallocation.
   dv.reserve_front(dv.size() + 4);
   dv.reserve_back(dv.size() + 4);

   //emplace variants construct the element in place at either end.
   dv.emplace_front(-1);        // {-1, 0, 1, 2, 3}
   dv.emplace_back(4);          // {-1, 0, 1, 2, 3, 4}
   assert(dv.front() == -1);
   assert(dv.back()  == 4);

   //Removal at both ends is constant time and never reallocates.
   dv.pop_front();              // {0, 1, 2, 3, 4}
   dv.pop_back();               // {0, 1, 2, 3}
   assert(dv.size() == 4);
   assert(dv.front() == 0);
   assert(dv.back()  == 3);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_devector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_devector

// libs/container/example/doc_emplace.cpp
namespace ex_doc_emplace {
class non_copy_movable
{
   non_copy_movable(const non_copy_movable &);
   non_copy_movable& operator=(const non_copy_movable &);

   public:
   non_copy_movable(int = 0) {}
};

int example_main()
{
   using namespace boost::container;

   //Store non-copyable and non-movable objects in a list
   list<non_copy_movable> l;
   non_copy_movable ncm;

   //A new element will be built calling non_copy_movable(int) constructor
   l.emplace(l.begin(), 0);
   assert(l.size() == 1);

   //A new element will be value initialized
   l.emplace(l.begin());
   assert(l.size() == 2);
   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_emplace") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_emplace

// libs/container/example/doc_flat_map.cpp
namespace ex_doc_flat_map {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

   typedef flat_map<int, int> map_t;

   map_t m;

   //Like vector, we can reserve storage to avoid reallocations while filling.
   m.reserve(8);

   //Insertions keep the underlying vector sorted by key. They are O(N)
   //because the elements after the insertion point must be shifted.
   m[30] = 3;
   m[10] = 1;
   m[20] = 2;
   assert(m.size() == 3);

   //Iteration is in key order and over contiguous memory, using random-access
   //iterators, so it is much faster than a node-based std::map.
   {
      map_t::const_iterator it = m.begin();
      assert(it->first == 10 && it->second == 1); ++it;
      assert(it->first == 20 && it->second == 2); ++it;
      assert(it->first == 30 && it->second == 3);
   }

   //Lookup uses binary search: O(log N).
   map_t::iterator f = m.find(20);
   assert(f != m.end() && f->second == 2);

   //All values live in a single underlying sequence of value_type, which is
   //std::pair<Key, T> (an array of structs). This is a key difference with the
   //C++23 std::flat_map, a container *adaptor* that keeps keys and mapped
   //values in two separate, parallel containers (a structure of arrays).
   const map_t::value_type *raw = &*m.begin();
   assert(raw[0].first == 10 && raw[2].first == 30);

   //The underlying sorted vector can be moved out with extract_sequence() and
   //moved back in with adopt_sequence(). Because the extracted sequence is
   //already ordered and free of duplicates, we can re-adopt it in O(1) using
   //the ordered_unique_range_t overload.
   map_t::sequence_type seq = m.extract_sequence();
   assert(m.empty());
   m.adopt_sequence(ordered_unique_range_t(), boost::move(seq));
   assert(m.size() == 3 && m.find(30) != m.end());

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_flat_map") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_flat_map

// libs/container/example/doc_hub_basic.cpp
namespace ex_doc_hub_basic {
#if BOOST_CXX_VERSION < 201103L

int example_main() { return 0;     return 0;  // main's implicit return
}

#else

#ifdef NDEBUG
#	undef NDEBUG
#endif

int example_main()
{
  boost::container::hub<int> h;

  // Insert some elements and keep an iterator to one of them
  for(int i = 0; i < 100; ++i) h.insert(i);
  auto it = h.insert(100);
  for(int i = 101; i < 200; ++i) h.insert(i);

  // Erase some of the elements
  erase_if(h, [](int x) { return x % 2 != 0;});
  assert(*it == 100); // iterator still valid

  // Insert many more elements
  for(int i = 200; i < 10000; ++i) h.insert(i);
  assert(*it == 100); // iterator still valid
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: doc_hub_basic") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_hub_basic

// libs/container/example/doc_move_containers.cpp
namespace ex_doc_move_containers {
class non_copyable
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(non_copyable)

   public:
   non_copyable(){}
   non_copyable(BOOST_RV_REF(non_copyable)) {}
   non_copyable& operator=(BOOST_RV_REF(non_copyable)) { return *this; }
};

int example_main()
{
   using namespace boost::container;

   //Store non-copyable objects in a vector
   vector<non_copyable> v;
   non_copyable nc;
   v.push_back(boost::move(nc));
   assert(v.size() == 1);

   //Reserve no longer needs copy-constructible
   v.reserve(100);
   assert(v.capacity() >= 100);

   //This resize overload only needs movable and default constructible
   v.resize(200);
   assert(v.size() == 200);

   //Containers are also movable
   vector<non_copyable> v_other(boost::move(v));
   assert(v_other.size() == 200);
   assert(v.empty());

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_move_containers") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_move_containers

// libs/container/example/doc_pmr.cpp
namespace ex_doc_pmr {
class ShoppingList
{
   // A vector of strings using polymorphic allocators. Every element
   // of the vector will use the same allocator as the vector itself.
   boost::container::pmr::vector_of
      <boost::container::pmr::string>::type m_strvec;
   //Alternatively in compilers that support template aliases:
   //    boost::container::pmr::vector<boost::container::pmr::string> m_strvec;
   public:

   // This makes uses_allocator<ShoppingList, memory_resource*>::value true
   typedef boost::container::pmr::memory_resource* allocator_type;

   // If the allocator is not specified, "m_strvec" uses pmr::get_default_resource().
   explicit ShoppingList(allocator_type alloc = 0)
      : m_strvec(alloc) {}

   // Copy constructor. As allocator is not specified,
   // "m_strvec" uses pmr::get_default_resource().
   ShoppingList(const ShoppingList& other)
      : m_strvec(other.m_strvec) {}

   // Copy construct using the given memory_resource.
   ShoppingList(const ShoppingList& other, allocator_type a)
      : m_strvec(other.m_strvec, a) {}

   allocator_type get_allocator() const
   { return m_strvec.get_allocator().resource(); }

   void add_item(const char *item)
   { m_strvec.emplace_back(item); }

   //...
};

//]]

//[doc_pmr_main_cpp

//=#include "ShoppingList.hpp"

void processShoppingList(const ShoppingList&)
{  /**/   }

int example_main()
{
   using namespace boost::container;
   //All memory needed by folder and its contained objects will
   //be allocated from the default memory resource (usually new/delete) 
   pmr::list_of<ShoppingList>::type folder; // Default allocator resource
   //Alternatively in compilers that support template aliases:
   //    boost::container::pmr::list<ShoppingList> folder;
   {
      char buffer[1024];
      pmr::monotonic_buffer_resource buf_rsrc(&buffer, 1024);

      //All memory needed by temporaryShoppingList will be allocated
      //from the local buffer (speeds up "processShoppingList")
      ShoppingList temporaryShoppingList(&buf_rsrc);
      assert(&buf_rsrc == temporaryShoppingList.get_allocator());

      //list nodes, and strings "salt" and "pepper" will be allocated
      //in the stack thanks to "monotonic_buffer_resource".
      temporaryShoppingList.add_item("salt");
      temporaryShoppingList.add_item("pepper");
      //...

      //All modifications and additions to "temporaryShoppingList"
      //will use memory from "buffer" until it's exhausted.
      processShoppingList(temporaryShoppingList);

      //Processing done, now insert it in "folder",
      //which uses the default memory resource
      folder.push_back(temporaryShoppingList);
      assert(pmr::get_default_resource() == folder.back().get_allocator());
      //temporaryShoppingList, buf_rsrc, and buffer go out of scope
   }
   return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: doc_pmr") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_pmr

// libs/container/example/doc_recursive_containers.cpp
namespace ex_doc_recursive_containers {
using namespace boost::container;

struct data
{
   int               i_;
   //A vector holding still undefined class 'data'
   vector<data>      v_;
   vector<data>::iterator vi_;
   //A stable_vector holding still undefined class 'data'
   stable_vector<data> sv_;
   stable_vector<data>::iterator svi_;
   //A stable_vector holding still undefined class 'data'
   deque<data> d_;
   deque<data>::iterator di_;
   //A list holding still undefined 'data'
   list<data>        l_;
   list<data>::iterator li_;
   //A map holding still undefined 'data'
   map<data, data>   m_;
   map<data, data>::iterator   mi_;

   friend bool operator <(const data &l, const data &r)
   { return l.i_ < r.i_; }
};

struct tree_node
{
   string name;
   string value;

   //children nodes of this node
   list<tree_node>            children_;
   list<tree_node>::iterator  selected_child_;
};



int example_main()
{
   //a container holding a recursive data type
   stable_vector<data> sv;
   sv.resize(100);

   //Let's build a tree based in
   //a recursive data type
   tree_node root;
   root.name  = "root";
   root.value = "root_value";
   root.children_.resize(7);
   root.selected_child_ = root.children_.begin();
   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_recursive_containers") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_recursive_containers

// libs/container/example/doc_slist.cpp
namespace ex_doc_slist {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

   typedef slist<int> slist_t;

   slist_t l;

   //A singly linked list offers constant-time insertion/removal at the front.
   l.push_front(3);
   l.push_front(2);
   l.push_front(1);                       // {1, 2, 3}

   //Unlike std::forward_list, Boost.Container slist keeps track of its length,
   //so size() is a constant-time operation.
   assert(l.size() == 3);
   assert(l.front() == 1);

   //The efficient way to insert into a singly linked list is *after* a known
   //position (constant time). insert_after/erase_after avoid the linear scan
   //that plain insert/erase would need to locate the previous node.
   slist_t::iterator it = l.begin();      // -> 1
   ++it;                                   // -> 2
   l.insert_after(it, 20);                // {1, 2, 20, 3}
   assert(l.size() == 4);

   //erase_after removes the element following the iterator in constant time.
   l.erase_after(it);                     // removes 20 -> {1, 2, 3}
   assert(l.size() == 3);

   //before_begin() denotes the position just before the first element, so
   //inserting after it is equivalent to push_front.
   l.insert_after(l.before_begin(), 0);   // {0, 1, 2, 3}
   assert(l.front() == 0);

   //slist provides a size-aware splice_after overload that merges a range from
   //another list in constant time when the number of elements is known.
   slist_t other;
   other.push_front(6);
   other.push_front(5);                   // {5, 6}

   slist_t::iterator before_last = other.begin();
   ++before_last;                          // points to the last element (6)
   //Move the whole 'other' list to the front of 'l' in O(1).
   l.splice_after(l.before_begin(), other,
                  other.before_begin(), before_last, other.size());
   assert(other.empty());                  // {5, 6, 0, 1, 2, 3}

   //Forward, in-order traversal.
   const int expected[] = { 5, 6, 0, 1, 2, 3 };
   int i = 0;
   for(slist_t::const_iterator b = l.begin(), e = l.end(); b != e; ++b, ++i)
      assert(*b == expected[i]);
   assert(l.size() == 6);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_slist") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_slist

// libs/container/example/doc_small_vector_base.cpp
namespace ex_doc_small_vector_base {
#ifdef NDEBUG
#undef NDEBUG
#endif

using namespace boost::container;

//small_vector_base<T> erases the preallocated element count "N" from the type,
//so the following non-template functions accept any small_vector<int, N>.

//Reads any small_vector<int, N> (taken by const reference to the base).
int sum(const small_vector_base<int> &sv)
{
   int res = 0;
   for(std::size_t i = 0, n = sv.size(); i != n; ++i)
      res += sv[i];
   return res;
}

//Modifies any small_vector<int, N> (taken by reference to the base).
void push_range(small_vector_base<int> &sv, int first, int last)
{
   for(int i = first; i < last; ++i)
      sv.push_back(i);
}

int example_main()
{
   //Two small_vectors with different in-place capacities (N = 4 and N = 64)...
   small_vector<int, 4>  sv_small;
   small_vector<int, 64> sv_big;

   //...both implicitly convert to small_vector_base<int>, so the very same
   //non-template functions can operate on either of them.
   push_range(sv_small, 0, 5);   //grows beyond its 4 preallocated elements
   push_range(sv_big,   0, 5);   //stays within its 64 preallocated elements

   //The behaviour is identical regardless of the preallocated capacity N.
   assert(sum(sv_small) == (0+1+2+3+4));
   assert(sum(sv_small) == sum(sv_big));

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_small_vector_base") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_small_vector_base

// libs/container/example/doc_static_vector.cpp
namespace ex_doc_static_vector {
#ifdef NDEBUG
#undef NDEBUG
#endif

int example_main()
{
   using namespace boost::container;

   //A static_vector<int> that can hold at most 5 elements. All the storage
   //lives inside the object itself, so creating it performs no dynamic
   //allocation and the capacity is fixed at compile time.
   static_vector<int, 5> sv;

   assert(sv.empty());
   assert(sv.capacity() == 5);   //fixed capacity, known at compile time
   assert(sv.max_size() == 5);

   //Elements are inserted just like with a vector (constant time at the end).
   //Unlike std::array, elements are constructed only as they are inserted.
   for(int i = 0; i < 5; ++i)
      sv.push_back(i);

   assert(sv.size() == 5);
   assert(sv.size() == sv.capacity());   //the container is now full

   //Contiguous storage and random access, just like vector/array.
   assert(sv[0] == 0 && sv[4] == 4);
   assert(sv.data()[2] == 2);

   #ifndef BOOST_NO_EXCEPTIONS
   //By default (throw_on_overflow<true>) inserting beyond the fixed capacity
   //throws instead of allocating more memory.
   bool overflowed = false;
   try {
      sv.push_back(5);   //capacity already reached
   }
   catch(const std::exception &) {
      overflowed = true;
   }
   assert(overflowed);
   #endif

   //Removal at the end is constant time and never reallocates.
   sv.pop_back();
   assert(sv.size() == 4);

   return 0;
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: doc_static_vector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_static_vector

// libs/container/example/doc_type_erasure.cpp
namespace ex_doc_type_erasure {
class MyClass;

class MyClassHolder
{
   public:

   void AddNewObject(const MyClass &o);
   const MyClass & GetLastObject() const;

   private:
   ::boost::container::vector<MyClass> vector_;
};

//]

//[doc_type_erasure_MyClass_h

//MyClass.h

class MyClass
{
   private:
   int value_;

   public:
   MyClass(int val = 0) : value_(val){}

   friend bool operator==(const MyClass &l, const MyClass &r)
   {  return l.value_ == r.value_;  }
   //...
};

//]



//[doc_type_erasure_main_cpp

//Main.cpp

//=#include "MyClassHolder.h"
//=#include "MyClass.h"


int example_main()
{
   MyClass mc(7);
   MyClassHolder myclassholder;
   myclassholder.AddNewObject(mc);
   return myclassholder.GetLastObject() == mc ? 0 : 1;
    return 0;  // main's implicit return
}
//]

//[doc_type_erasure_MyClassHolder_cpp

//MyClassHolder.cpp

//=#include "MyClassHolder.h"

//In the implementation MyClass must be a complete
//type so we include the appropriate header
//=#include "MyClass.h"

void MyClassHolder::AddNewObject(const MyClass &o)
{  vector_.push_back(o);  }

const MyClass & MyClassHolder::GetLastObject() const
{  return vector_.back();  }

//]

TEST_CASE("example: doc_type_erasure") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_doc_type_erasure
