// mcpplibs.mwrap.boost.regex — user types at Boost.Regex's customization
// points: a user traits class (libs/regex/doc/regex_traits.qbk) layered on
// boost::regex_traits, searches over user iterator types (a deque, a user
// bidirectional container), a match_results with a user allocator, a user
// formatter functor for regex_replace, and a user callback for regex_grep.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.regex;

namespace user {
// Traits that treat '_' as a member of every character class: a user traits
// class layered on the default one.
struct underscore_traits : boost::regex_traits<char> {
    using base = boost::regex_traits<char>;
    bool isctype(char c, char_class_type f) const { return c == '_' ? true : base::isctype(c, f); }
};
using uregex = boost::basic_regex<char, underscore_traits>;

template <class T>
struct counting_allocator {
    using value_type = T;
    static inline int allocations = 0;
    counting_allocator() = default;
    template <class U>
    counting_allocator(const counting_allocator<U>&) noexcept {}
    T* allocate(std::size_t n) {
        ++allocations;
        return std::allocator<T>().allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
    template <class U>
    bool operator==(const counting_allocator<U>&) const noexcept { return true; }
};

struct shout_formatter {
    template <class Match>
    std::string operator()(const Match& m) const {
        std::string s = m.str();
        for (auto& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
        return "<" + s + ">";
    }
};

struct word_counter {
    std::map<std::string, int>* counts;
    template <class Match>
    bool operator()(const Match& m) const {
        ++(*counts)[m.str()];
        return true;
    }
};
}  // namespace user

TEST_CASE("doc: regex_traits") {
    user::uregex word("[[:alpha:]]+");
    CHECK(boost::regex_match("snake_case", word));
    user::uregex folded("snake_case", boost::regex::icase);
    CHECK(boost::regex_match("SNAKE_CASE", folded));
    boost::regex plain("[[:alpha:]]+");
    CHECK_FALSE(boost::regex_match("snake_case", plain));
}

TEST_CASE("user iterator types") {
    std::deque<char> text{'a', 'b', '1', '2'};
    boost::match_results<std::deque<char>::iterator> m;
    REQUIRE(boost::regex_search(text.begin(), text.end(), m, boost::regex("\\d+")));
    CHECK(std::string(m[0].first, m[0].second) == "12");
    std::list<char> lst{'x', 'y', 'z'};
    CHECK(boost::regex_match(lst.begin(), lst.end(), boost::regex("x.z")));
    std::vector<char> v{'a', ' ', 'b'};
    boost::regex_iterator<std::vector<char>::const_iterator> it(v.cbegin(), v.cend(), boost::regex("\\w")), end;
    CHECK(std::distance(it, end) == 2);
}

TEST_CASE("match_results with a user allocator") {
    using alloc = user::counting_allocator<boost::sub_match<std::string::const_iterator>>;
    boost::match_results<std::string::const_iterator, alloc> m;
    const std::string s = "2026-11-09";
    REQUIRE(boost::regex_search(s, m, boost::regex("(\\d+)-(\\d+)-(\\d+)")));
    CHECK(m.str(3) == "09");
    CHECK(alloc::allocations > 0);
}

TEST_CASE("user formatter and grep callback") {
    std::string s = "one two one";
    CHECK(boost::regex_replace(s, boost::regex("\\w+"), user::shout_formatter()) == "<ONE> <TWO> <ONE>");
    std::map<std::string, int> counts;
    boost::regex_grep(user::word_counter{&counts}, s, boost::regex("\\w+"));
    CHECK(counts["one"] == 2);
    CHECK(counts["two"] == 1);
}
