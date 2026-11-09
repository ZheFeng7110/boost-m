// mcpplibs.mwrap.boost.circular_buffer — circular_buffer and
// circular_buffer_space_optimized: construction, capacity control, insertion
// at both ends with overwrite, rotation and linearization, array_one/array_two,
// insert/erase/rinsert/rerase, assign, set_capacity / rset_capacity, resize,
// comparison and swap.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.circular_buffer;

using cb_t = boost::circular_buffer<int>;

namespace {
std::vector<int> v(const cb_t& c) { return {c.begin(), c.end()}; }
}  // namespace

TEST_CASE("construction") {
    CHECK(cb_t().capacity() == 0);
    cb_t a(5, 7);
    CHECK(a.size() == 5);
    CHECK(a.full());
    cb_t b(4, 2, 9);  // capacity 4, two elements of 9
    CHECK(b.size() == 2);
    CHECK(b.reserve() == 2);
    std::vector<int> src{1, 2, 3, 4, 5};
    cb_t c(3, src.begin(), src.end());  // keeps the last three
    CHECK(v(c) == std::vector<int>{3, 4, 5});
    cb_t d(src.begin(), src.end());
    CHECK(d.capacity() == 5);
    cb_t e = {1, 2};
    (void)e;
}

TEST_CASE("push/pop at both ends and overwrite") {
    cb_t c(3);
    c.push_back(1);
    c.push_back(2);
    c.push_front(0);
    CHECK(v(c) == std::vector<int>{0, 1, 2});
    c.push_front(-1);  // full: overwrites the back
    CHECK(v(c) == std::vector<int>{-1, 0, 1});
    c.push_back(5);  // full: overwrites the front
    CHECK(v(c) == std::vector<int>{0, 1, 5});
    c.pop_front();
    c.pop_back();
    CHECK(v(c) == std::vector<int>{1});
    c.push_back(8);
    c.push_front(9);
    CHECK(v(c) == std::vector<int>{9, 1, 8});
    CHECK(c.at(2) == 8);
    CHECK_THROWS_AS(c.at(3), std::out_of_range);
}

TEST_CASE("linearize, rotate, array_one/array_two") {
    cb_t c(4);
    for (int i = 1; i <= 6; ++i) c.push_back(i);  // 3 4 5 6, wrapped
    CHECK_FALSE(c.is_linearized());
    auto one = c.array_one();
    auto two = c.array_two();
    CHECK(one.second + two.second == 4);
    int* p = c.linearize();
    CHECK(c.is_linearized());
    CHECK(std::vector<int>(p, p + 4) == std::vector<int>{3, 4, 5, 6});
    c.rotate(c.begin() + 1);
    CHECK(v(c) == std::vector<int>{4, 5, 6, 3});
}

TEST_CASE("insert / erase / rinsert / rerase") {
    cb_t c;
    c.assign(5, 3, 0);  // capacity 5, three zeros (assign(n, x) would set capacity n)
    CHECK(c.capacity() == 5);
    c.insert(c.begin() + 1, 7);
    CHECK(v(c) == std::vector<int>{0, 7, 0, 0});
    c.insert(c.end(), 2, 9);  // full: elements at the front are overwritten
    CHECK(v(c) == std::vector<int>{7, 0, 0, 9, 9});
    c.rinsert(c.begin(), 1);  // full: inserting toward the front drops the back
    CHECK(v(c) == std::vector<int>{1, 7, 0, 0, 9});
    c.erase(c.begin() + 2, c.begin() + 4);
    CHECK(v(c) == std::vector<int>{1, 7, 9});
    c.rerase(c.begin());
    CHECK(v(c) == std::vector<int>{7, 9});
    c.erase_begin(1);
    c.erase_end(1);
    CHECK(c.empty());
}

TEST_CASE("capacity changes and resize") {
    cb_t c(5);
    for (int i = 0; i < 5; ++i) c.push_back(i);
    c.set_capacity(3);  // keeps the front
    CHECK(v(c) == std::vector<int>{0, 1, 2});
    c.rset_capacity(2);  // keeps the back
    CHECK(v(c) == std::vector<int>{1, 2});
    c.set_capacity(4);
    c.resize(4, 8);
    CHECK(v(c) == std::vector<int>{1, 2, 8, 8});
    c.rresize(2);
    CHECK(v(c) == std::vector<int>{8, 8});
    c.clear();
    CHECK(c.empty());
    CHECK(c.capacity() == 4);
    CHECK(c.max_size() >= 4);
}

TEST_CASE("comparison and swap") {
    cb_t a(3), b(3);
    a.push_back(1);
    b.push_back(2);
    CHECK(a < b);
    CHECK(a != b);
    swap(a, b);
    CHECK(a.front() == 2);
    cb_t c(a);
    CHECK(c == a);
}

TEST_CASE("space optimized") {
    using so_t = boost::circular_buffer_space_optimized<int>;
    so_t s(so_t::capacity_type(100, 10));  // capacity 100, at least 10 allocated
    CHECK(s.capacity().capacity() == 100);
    CHECK(s.capacity().min_capacity() == 10);
    CHECK(s.empty());
    for (int i = 0; i < 150; ++i) s.push_back(i);
    CHECK(s.size() == 100);
    CHECK(s.front() == 50);
    s.set_capacity(so_t::capacity_type(20));
    CHECK(s.size() == 20);
    s.erase(s.begin(), s.begin() + 10);
    CHECK(s.front() == 60);
    CHECK(s.size() == 10);
}
