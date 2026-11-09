// mcpplibs.mwrap.boost.bloom — the tutorial of Boost.Bloom
// (libs/bloom/doc/bloom/tutorial.adoc) section by section, plus the subfilter
// family and the filter's construction/assignment/comparison members.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.bloom;
import mcpplibs.mwrap.boost.core;  // boost::span

using filter = boost::bloom::filter<std::string, 8>;

TEST_CASE("doc: tutorial#tutorial") {
    // Filter Definition: T, K, Subfilter, Stride, Hash, Allocator.
    using f1 = boost::bloom::filter<int, 3>;
    using f2 = boost::bloom::filter<int, 1, boost::bloom::block<std::uint64_t, 4>>;
    using f3 = boost::bloom::filter<int, 1, boost::bloom::multiblock<std::uint32_t, 8>, 2>;
    static_assert(std::is_same_v<f1::value_type, int>);
    static_assert(f1::k == 3);
    static_assert(std::is_same_v<f2::subfilter, boost::bloom::block<std::uint64_t, 4>>);
    static_assert(f3::stride == 2);
    static_assert(std::is_same_v<f1::hasher, boost::hash<int>>);
}

TEST_CASE("doc: tutorial#capacity") {
    filter f(1'000'000);
    CHECK(f.capacity() >= 1'000'000);
    filter g(100'000, 0.01);
    filter g2(filter::capacity_for(100'000, 0.01));
    CHECK(g.capacity() == g2.capacity());
    // filter(100'000, 1E-50) asks for ~1.4E12 bits; the documented bad_alloc
    // depends on the allocator refusing it (Linux overcommit may not), so the
    // test checks the computed capacity instead of allocating it.
    CHECK(filter::capacity_for(100'000, 1E-50) > std::size_t(1) << 40);
    g.reset(2'000'000);
    CHECK(g.capacity() >= 2'000'000);
    g.reset(100'000, 0.005);
    CHECK(g.capacity() == filter(filter::capacity_for(100'000, 0.005)).capacity());
    g.reset();
    CHECK(g.capacity() == 0);
}

TEST_CASE("doc: tutorial#insertion-and-lookup") {
    filter f(10'000, 0.01);
    std::vector<std::string> data{"a", "b", "c"};
    f.insert("hello");
    f.insert(data.begin(), data.end());
    CHECK(f.may_contain("hello"));
    CHECK(f.may_contain("b"));
    int fp = 0;
    for (int i = 0; i < 1000; ++i) fp += f.may_contain("absent-" + std::to_string(i));
    CHECK(fp < 50);  // FPR configured at 1%
    double fpr = filter::fpr_for(100, f.capacity());
    CHECK(fpr > 0.0);
    CHECK(fpr < 0.01);
    f.clear();
    CHECK_FALSE(f.may_contain("hello"));
}

TEST_CASE("doc: tutorial#bulk-operations") {
    filter f(10'000, 0.01);
    std::vector<std::string> data{"one", "two", "three"};
    f.insert(data.begin(), data.end());
    std::vector<std::string> input{"one", "nope", "three"};
    std::vector<bool> results;
    f.may_contain(input.begin(), input.end(), [&](const std::string&, bool b) { results.push_back(b); });
    CHECK(results.size() == 3);
    CHECK(results[0]);
    CHECK(results[2]);
}

TEST_CASE("doc: tutorial#filter-combination") {
    filter f(10'000, 0.01), f2(f.capacity()), f3(f.capacity());
    f.insert("a");
    f2.insert("b");
    f |= f2;
    CHECK(f.may_contain("a"));
    CHECK(f.may_contain("b"));
    f3.insert("a");
    f &= f3;
    CHECK(f.may_contain("a"));
    filter other(f.capacity() * 2);
    CHECK_THROWS_AS(f |= other, std::invalid_argument);
}

TEST_CASE("doc: tutorial#direct-access-to-the-array") {
    filter f1(5'000, 0.01);
    f1.insert("persisted");
    const auto path = std::filesystem::temp_directory_path() / "boost-m-bloom-array.bin";
    {
        std::ofstream out(path, std::ios::binary);
        std::size_t c1 = f1.capacity();
        out.write(reinterpret_cast<const char*>(&c1), sizeof(c1));
        boost::span<const unsigned char> s1 = f1.array();
        out.write(reinterpret_cast<const char*>(s1.data()), static_cast<std::streamsize>(s1.size()));
    }
    filter f2;
    {
        std::ifstream in(path, std::ios::binary);
        std::size_t c2;
        in.read(reinterpret_cast<char*>(&c2), sizeof(c2));
        f2.reset(c2);
        boost::span<unsigned char> s2 = f2.array();
        in.read(reinterpret_cast<char*>(s2.data()), static_cast<std::streamsize>(s2.size()));
    }
    CHECK(f2 == f1);
    CHECK(f2.may_contain("persisted"));
    std::filesystem::remove(path);
}

TEST_CASE("doc: future_work#future_work") {
    // try_insert is future work; today's idiom:
    filter f(1000, 0.01);
    auto try_insert = [&](const std::string& x) {
        if (f.may_contain(x)) return false;
        f.insert(x);
        return true;
    };
    CHECK(try_insert("x"));
    CHECK_FALSE(try_insert("x"));
}

TEST_CASE_TEMPLATE("subfilters", Sub, boost::bloom::block<unsigned char, 1>, boost::bloom::block<std::uint64_t, 3>,
                   boost::bloom::multiblock<std::uint32_t, 4>, boost::bloom::fast_multiblock32<8>,
                   boost::bloom::fast_multiblock64<4>) {
    boost::bloom::filter<int, 1, Sub> f(4'000, 0.02);
    for (int i = 0; i < 200; ++i) f.insert(i);
    for (int i = 0; i < 200; ++i) REQUIRE(f.may_contain(i));
    auto g = f;
    CHECK(g == f);
    g.clear();
    CHECK(g != f);
    std::swap(f, g);
    CHECK_FALSE(f.may_contain(1));
}
