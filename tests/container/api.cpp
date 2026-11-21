// mcpplibs.mwrap.boost.container — the documentation's inline code
// (libs/container/doc/container.qbk) and the container families: vector,
// devector, stable_vector, small/static_vector, deque, list/slist, the flat
// associative containers, string, hub, the pmr aliases and memory resources,
// const-reference parameters into the same container, null iterators.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.container;

namespace bc = boost::container;

TEST_CASE("doc: container_const_reference_parameters") {
    // Arguments referring into the container itself are safe.
    bc::vector<int> v{0, 1, 2, 3};
    v.insert(v.begin(), v[2]);
    CHECK(v == bc::vector<int>{2, 0, 1, 2, 3});
    bc::list<int> l{5, 5, 6};
    l.remove(*l.begin());
    CHECK(l.size() == 1);
    CHECK(l.front() == 6);
}

TEST_CASE("doc: devector") {
    bc::devector<int> d;
    d.reserve_front(10);
    d.reserve_back(10);
    for (int i = 0; i < 5; ++i) {
        d.push_front(-i);
        d.push_back(i);
    }
    CHECK(d.front() == -4);
    CHECK(d.back() == 4);
    CHECK(d.front_free_capacity() + d.back_free_capacity() + d.size() >= 20);
    d.pop_front();
    d.pop_back();
    CHECK(d.size() == 8);
    // capacity() is the minimum number of elements insertable without
    // reallocating; size() may exceed it.
    d.reserve(32);
    CHECK(d.capacity() >= 32);
}

TEST_CASE("doc: stable_vector") {
    bc::stable_vector<std::string> sv{"a", "b", "c"};
    auto it = sv.begin() + 2;
    std::string* p = &*it;
    sv.insert(sv.begin(), "z");  // iterators and references stay valid
    sv.erase(sv.begin() + 1);
    CHECK(*it == "c");
    CHECK(p == &sv.back());
    CHECK(sv.size() == 3);
}

TEST_CASE("doc: hub_introduction") {
    bc::hub<int> h;
    for (int i = 0; i < 100; ++i) h.insert(i);
    auto it = h.insert(100);
    for (int i = 101; i < 200; ++i) h.insert(i);
    erase_if(h, [](int x) { return x % 2 != 0; });
    CHECK(*it == 100);  // the documentation writes `*it = 100` in assert: a typo for ==
    for (int i = 200; i < 10000; ++i) h.insert(i);
    CHECK(*it == 100);
}

TEST_CASE("doc: hub_capacity") {
    bc::hub<int> h;
    h.reserve(1000);
    CHECK(h.capacity() == 1024);  // rounded to a multiple of 64
    for (int i = 0; i < 500; ++i) h.insert(i);
    h.trim_capacity(750);
    CHECK(h.capacity() >= 750);
    CHECK(h.capacity() % 64 == 0);
    h.trim_capacity();
    erase_if(h, [](int x) { return x % 2 != 0; });
    h.shrink_to_fit();
    long sum = 0;
    for (const auto& x : h) sum += x;
    CHECK(sum == 249 * 250);  // 0 + 2 + ... + 498
}

TEST_CASE("doc: hub_unordered_insertion") {
    bc::hub<int> h = {0, 1, 2};
    h.erase(h.begin());
    h.insert({3, 4, 5});
    std::multiset<int> seen(h.begin(), h.end());
    CHECK(seen == std::multiset<int>{1, 2, 3, 4, 5});  // insertion order is not kept
}

TEST_CASE("doc: hub_std_hive_operations") {
    bc::hub<int> h1 = {0, 2, 3, 4, 6}, h2 = {1, 4, 6, 7, 9};
    h1.splice(h2);
    CHECK(h2.empty());
    h1.sort();
    h1.unique();
    CHECK(std::vector<int>(h1.begin(), h1.end()) == std::vector<int>{0, 1, 2, 3, 4, 6, 7, 9});
    bc::hub<int> h;
    int* p = std::addressof(*h.insert(50));
    bc::hub<int>::iterator it = h.get_iterator(p);
    h.erase(it);
    CHECK(h.empty());
}

TEST_CASE("doc: hub_visitation") {
    bc::hub<int> h = {1, 2, 3};
    for (auto& x : h) x *= 2;
    for_each(h, [](auto& x) { x *= 2; });  // the hub's own for_each
    CHECK(std::accumulate(h.begin(), h.end(), 0) == 24);
}

TEST_CASE("doc: null_iterators") {
    auto ni = bc::vector<int>::iterator();
    auto nd = bc::vector<double>::iterator();
    CHECK(ni == ni);
    CHECK_FALSE(nd != nd);
    static_assert(!std::is_same_v<decltype(ni), decltype(nd)>);  // ni == nd does not compile
}

TEST_CASE("doc: polymorphic_memory_resources") {
    char buffer[1024];
    bc::pmr::monotonic_buffer_resource mr(buffer, sizeof buffer);
    bc::pmr::vector<bc::pmr::string> v(&mr);
    v.emplace_back("a string longer than the small buffer optimization");
    CHECK(v.get_allocator().resource() == &mr);
    CHECK(v.back().get_allocator().resource() == &mr);  // uses-allocator propagation
    static_assert(std::is_same_v<bc::pmr::vector_of<int>::type, bc::pmr::vector<int>>);
    bc::pmr::unsynchronized_pool_resource pool;
    bc::pmr::map<int, bc::pmr::string> m(&pool);
    m.emplace(1, "x");
    CHECK(m.at(1) == "x");
    CHECK(bc::pmr::get_default_resource() == bc::pmr::new_delete_resource());
}

TEST_CASE("flat containers, small/static vector, string, deque, slist") {
    bc::flat_map<int, std::string> fm{{3, "c"}, {1, "a"}};
    CHECK(fm.begin()->second == "a");
    CHECK(fm.nth(1)->first == 3);
    bc::flat_multiset<int> fms{2, 1, 2};
    CHECK(fms.count(2) == 2);
    bc::small_vector<int, 4> sv{1, 2, 3, 4};
    sv.push_back(5);  // spills to the heap
    CHECK(sv.size() == 5);
    bc::static_vector<int, 3> stv{1, 2, 3};
    CHECK_THROWS_AS(stv.push_back(4), bc::bad_alloc);  // the library's own bad_alloc
    bc::string s("hello");
    s += " world";
    CHECK(s.find("world") == 6);
    CHECK(bc::string("a") < bc::string("b"));
    bc::deque<int> dq{1, 2};
    dq.push_front(0);
    CHECK(dq[0] == 0);
    bc::slist<int> sl{3, 2, 1};
    sl.sort();
    CHECK(sl.front() == 1);
}
