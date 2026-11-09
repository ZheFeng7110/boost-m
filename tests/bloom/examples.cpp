// mcpplibs.mwrap.boost.bloom — libs/bloom/example/{basic,rolling_filter,
// serialization}.cpp ported mechanically: each program runs as a case and its
// output must equal what the original printed when built from upstream/boost
// alone. serialization writes its filter to the temporary directory instead
// of ./filter.bin, and keeps its #include of boost/core/detail/splitmix64.hpp
// (a detail header no module exports). genome.cpp reads a 140 MB genome
// download: genome.cpp in this directory runs it on a small FASTA fixture.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <boost/core/detail/splitmix64.hpp>
import std;
import mcpplibs.mwrap.boost.bloom;
import mcpplibs.mwrap.boost.uuid;

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

// libs/bloom/example/basic.cpp
namespace ex_basic {
int example_main()
{
  /* Bloom filter of strings with 5 bits set per insertion */

  using filter = boost::bloom::filter<std::string, 5>;

  /* create filter with a capacity of 1,000,000 bits */

  filter f(1000000);

  /* insert elements in the set */

  f.insert("hello");
  f.insert("Boost");

  /* elements inserted are always correctly checked as such */

  assert(f.may_contain("hello") == true);

  /* Elements not inserted may incorrectly be identified as such with a
   * false probability rate (FPR) which is a function of the array capacity,
   * the number of bits set per element and generally how the
   * boost::bloom::filter was configured.
   */

  if(f.may_contain("bye")) { /* likely false */
    ex_out << "false positive\n";
  }
  else {
    ex_out << "everything worked as expected\n";
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: basic") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(everything worked as expected
)ex"));
}
}  // namespace ex_basic

// libs/bloom/example/rolling_filter.cpp
namespace ex_rolling_filter {
/* Regular Bloom filters don't "forget", that is, the number of elements
 * in the filter keeps growing (and the FPR increasing) until the entire bit
 * array is reset. This is how a *rolling filter* can be devised that keeps
 * track of the last elements inserted only:
 * 
 *   - A so-called *window size* w and a number of windows n are specified.
 *   - n regular Bloom filters are kept simultaneously, one for each window.
 *   - We keep an index i to designate the i-th filter as the *active filter*.
 *   - Insertion of a new element is done in the active filter only.
 *   - Lookup queries all the filters.
 *   - Every w insertions, we bump i (mod n) to switch to a new active filter
 *     and clear it before continuing with the insertions.
 * 
 * It's not hard to see that this algorithm implements a filter that remembers
 * only the last s elements inserted, with w * (n-1) < s <= w * n. The
 * resulting FPR oscillates between 1 - (1 - sub_fpr)^(n-1) and 
 * 1 - (1 - sub_fpr)^n, where sub_fpr is the FPR of an individual filter
 * after w insertions.
 */

template<
  typename T, std::size_t K,
  typename Subfilter = boost::bloom::block<unsigned char,1>,
  std::size_t Stride = 0,
  typename Hash = boost::hash<T>,
  typename Allocator=std::allocator<unsigned char>
>
class rolling_filter
{
public:
  rolling_filter(std::size_t w_, std::size_t n, double max_fpr): 
    w(w_), fs(n) 
  {
    assert(w > 0);
    assert(n >= 2);
    assert(max_fpr >= 0.0 && max_fpr <= 1.0);

    /* Adjust the capacity of each individual filter so that
     * this->max_fpr() ~ max_fpr. 
     */

    auto sub_fpr = 1.0 - std::pow(1.0 - max_fpr, 1.0 / n);
    auto m = filter_type::capacity_for(w, sub_fpr);
    for(auto& f: fs) f.reset(m);
  }

  std::size_t min_size() const
  {
    /* Minimum size under *stationary* conditions: the actual size can be
     * smaller than this if we haven't yet inserted that many elements.
     */

    return w * (fs.size() - 1);
  }

  std::size_t max_size() const
  {
    return w * fs.size();
  }

  double min_fpr() const
  {
    double sub_fpr = filter_type::fpr_for(w, fs[0].capacity());
    return 1.0 - std::pow(1.0 - sub_fpr, (double)fs.size() - 1.0);
  }

  double max_fpr() const
  {
    double sub_fpr = filter_type::fpr_for(w, fs[0].capacity());
    return 1.0 - std::pow(1.0 - sub_fpr, (double)fs.size());
  }

  std::size_t capacity() const
  {
    return fs[0].capacity() * fs.size();
  }

  void insert(const T& x)
  {
    if(++count > w) {
      count = 1;
      if(++i >= fs.size()) i = 0;
      fs[i].clear();
    }
    fs[i].insert(x);
  }

  bool may_contain(const T& x) const
  {
    for(const auto& f: fs) {
      if(f.may_contain(x)) return true;
    }
    return false;
  }

private:
  using filter_type = boost::bloom::filter<
    T, K, Subfilter, Stride, Hash, Allocator
  >;
  using vector_type = std::vector<
    filter_type, 
    typename std::allocator_traits<Allocator>::
      template rebind_alloc<filter_type>
  >;

  std::size_t w;
  vector_type fs;
  std::size_t count = 0,
              i = 0;
};

int example_main()
{
  /* Construct a rolling filter with a size between
   * 9,000 and 10,000 elements.
   */

  const std::size_t window_size = 1000;
  const std::size_t num_windows = 10;
  const double      max_fpr = 0.01;

  rolling_filter<std::size_t, 5> rf(window_size, num_windows, max_fpr);
  ex_out << "rolling filter capacity: " << rf.capacity() << " bits\n";

  /* Run the filter through more than 10x the elements it can hold. */

  const std::size_t num_elements = rf.max_size() * 10 + window_size / 2;
  for(std::size_t i = 0 ; i < num_elements; ++i) rf.insert(i);

  /* Check the filter has actually forgotten the first 
   * num_elements - rf.max_size() elements.
   */

  std::size_t count = 0;
  for(std::size_t i = 0 ; i < num_elements - rf.max_size(); ++i) {
    count += rf.may_contain(i);
  }
  ex_out << "measured fpr: " 
            << (double)count / (num_elements - rf.max_size())
            << " (should be between " << rf.min_fpr() 
            << " and " << rf.max_fpr() << ")\n";

  /* The remaining elements must be mostly in the filter. */

  count = 0;
  for(std::size_t i = num_elements - rf.max_size() ; i < num_elements; ++i) {
    count += rf.may_contain(i);
  }
  ex_out << "elements found: " << count
            << " (must be between " << rf.min_size()
            << " and " << rf.max_size() << ")\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: rolling_filter") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(rolling filter capacity: 172720 bits
measured fpr: 0.00909392 (should be between 0.00899309 and 0.00998731)
elements found: 9505 (must be between 9000 and 10000)
)ex"));
}
}  // namespace ex_rolling_filter

// libs/bloom/example/serialization.cpp
namespace ex_serialization {
/* emits a deterministic pseudorandom sequence of UUIDs */

struct uuid_generator
{
  boost::uuids::uuid operator()()
  {
    std::uint8_t    data[16];
    std::uint64_t x = rng();
    std::memcpy(&data[0], &x, sizeof(x));
    x = rng();
    std::memcpy(&data[8], &x, sizeof(x));

    return {data};
  }

  boost::detail::splitmix64 rng;
};

using filter = boost::bloom::filter<
  boost::uuids::uuid, 1, boost::bloom::multiblock<std::uint64_t, 8> >;

static constexpr std::size_t num_elements = 10000;

/* creates a filter with num_elements UUIDs */

filter create_filter()
{
  uuid_generator gen;
  filter         f(num_elements, 0.005);
  for(std::size_t i = 0; i < num_elements; ++i) f.insert(gen());
  return f;
}

void save_filter(const filter& f, const char* filename)
{
  std::ofstream out(filename, std::ios::binary | std::ios::trunc);
  std::size_t c=f.capacity();
  out.write(reinterpret_cast<const char*>(&c), sizeof(c)); /* save capacity (bits) */
  auto s = f.array();
  out.write(reinterpret_cast<const char*>(s.data()), s.size()); /* save array */
}

filter load_filter(const char* filename)
{
  std::ifstream in(filename, std::ios::binary);
  std::size_t c;
  in.read(reinterpret_cast<char*>(&c), sizeof(c));
  filter f(c);
  auto s = f.array();
  in.read(reinterpret_cast<char*>(s.data()), s.size()); /* load array */
  return f;
}

int example_main()
{
  static const std::string filename_s = (std::filesystem::temp_directory_path() / "boost-m-bloom-filter.bin").string();
  static const char* filename = filename_s.c_str();

  auto f1 = create_filter();
  save_filter(f1, filename);
  auto f2 = load_filter(filename);

  if (f1 == f2) ex_out << "serialization correct\n";
  else          ex_out << "something went wrong\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: serialization") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(serialization correct
)ex"));
}
}  // namespace ex_serialization
