// mcpplibs.mwrap.boost.algorithm — user-defined types through the module:
// user value types with their own ordering and equality, user predicates and
// comparators (function objects, function pointers, lambdas), user iterators
// (a bidirectional list-like container), and string algorithms over a user
// character sequence.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.algorithm;

namespace ba = boost::algorithm;

namespace user {
struct version {
    int major = 0, minor = 0;
    // is_increasing / is_decreasing compare with <= and >=.
    friend auto operator<=>(const version&, const version&) = default;
    friend std::ostream& operator<<(std::ostream& os, const version& v) {
        return os << v.major << '.' << v.minor;
    }
};

struct by_minor {
    bool operator()(const version& a, const version& b) const { return a.minor < b.minor; }
};

bool same_major(const version& a, const version& b) { return a.major == b.major; }

// A minimal bidirectional container with its own iterator type.
class ring {
public:
    explicit ring(std::initializer_list<int> v) : data_(v) {}
    class iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = const int*;
        using reference = const int&;
        iterator() = default;
        iterator(const std::vector<int>* d, std::size_t i) : d_(d), i_(i) {}
        reference operator*() const { return (*d_)[i_]; }
        iterator& operator++() { ++i_; return *this; }
        iterator operator++(int) { auto t = *this; ++i_; return t; }
        iterator& operator--() { --i_; return *this; }
        iterator operator--(int) { auto t = *this; --i_; return t; }
        friend bool operator==(const iterator& a, const iterator& b) { return a.i_ == b.i_; }
    private:
        const std::vector<int>* d_ = nullptr;
        std::size_t i_ = 0;
    };
    using const_iterator = iterator;
    iterator begin() const { return {&data_, 0}; }
    iterator end() const { return {&data_, data_.size()}; }
private:
    std::vector<int> data_;
};
}  // namespace user

TEST_CASE("clamp: user type, operator< and a user comparator") {
    const user::version lo{1, 0}, hi{2, 5};
    CHECK(ba::clamp(user::version{0, 9}, lo, hi) == lo);
    CHECK(ba::clamp(user::version{3, 0}, lo, hi) == hi);
    CHECK(ba::clamp(user::version{1, 7}, lo, hi) == user::version{1, 7});
    // by_minor orders by the minor number only: 9 is above hi.minor (5).
    CHECK(ba::clamp(user::version{0, 9}, lo, hi, user::by_minor()) == hi);
    CHECK(ba::is_clamped(user::version{1, 3}, lo, hi));
}

TEST_CASE("ordering and minmax: user type") {
    std::vector<user::version> v{{1, 0}, {1, 2}, {2, 0}};
    CHECK(ba::is_sorted(v));
    CHECK(ba::is_strictly_increasing(v));
    CHECK_FALSE(ba::is_sorted(v, user::by_minor()));
    auto mm = boost::minmax_element(v.begin(), v.end());
    CHECK(*mm.first == user::version{1, 0});
    CHECK(*mm.second == user::version{2, 0});
    auto mm2 = boost::minmax_element(v.begin(), v.end(), user::by_minor());
    CHECK(mm2.second->minor == 2);
}

TEST_CASE("predicates: function pointer, functor, lambda over user types") {
    std::vector<user::version> v{{1, 0}, {1, 2}, {1, 9}};
    CHECK(ba::all_of(v, [](const user::version& x) { return x.major == 1; }));
    CHECK(ba::one_of_equal(v, user::version{1, 2}));
    CHECK(ba::none_of_equal(v, user::version{3, 0}));
    CHECK(ba::is_palindrome(v, user::same_major));
    CHECK(ba::find_not(v, user::version{1, 0}) == v.begin() + 1);
    std::vector<user::version> w{{1, 9}, {1, 0}, {1, 2}};
    CHECK(ba::is_permutation(v.begin(), v.end(), w.begin(), w.end()));
    CHECK(ba::equal(v.begin(), v.end(), w.begin(), w.end(), user::same_major));
}

TEST_CASE("user iterators: bidirectional only") {
    user::ring r{1, 2, 3, 2, 1};
    CHECK(ba::is_palindrome(r.begin(), r.end()));
    CHECK(ba::is_palindrome(r));
    auto it = ba::find_backward(r.begin(), r.end(), 2);
    CHECK(*it == 2);
    CHECK(std::distance(r.begin(), it) == 3);
    CHECK(ba::is_partitioned_until(r, [](int i) { return i < 3; }) != r.end());
    std::vector<int> out;
    ba::copy_if(r, std::back_inserter(out), [](int i) { return i > 1; });
    CHECK(out == std::vector<int>{2, 3, 2});
}

TEST_CASE("searchers: user element type with operator== and std::hash") {
    struct sym {
        char c;
        bool operator==(const sym&) const = default;
    };
    std::vector<sym> corpus{{'a'}, {'b'}, {'c'}, {'a'}, {'b'}};
    std::vector<sym> pat{{'c'}, {'a'}};
    auto r = ba::knuth_morris_pratt_search(corpus.begin(), corpus.end(), pat.begin(), pat.end());
    CHECK(r.first - corpus.begin() == 2);
}

TEST_CASE("string algorithms over a user predicate and std::vector<char>") {
    struct is_vowel {
        bool operator()(char c) const { return std::string_view("aeiou").find(c) != std::string_view::npos; }
    };
    std::string s("programming");
    CHECK(boost::erase_all_copy(s, "m") == "prograing");
    CHECK(boost::trim_copy_if(std::string("aloha"), is_vowel()) == "loh");
    CHECK(boost::all("aei", is_vowel()));
    std::vector<char> chars{'x', 'y', 'z'};
    boost::to_upper(chars);
    CHECK(chars == std::vector<char>{'X', 'Y', 'Z'});
    std::vector<std::string> parts;
    boost::split(parts, s, is_vowel());
    CHECK(parts.front() == "pr");
}
