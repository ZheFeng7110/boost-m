// mcpplibs.mwrap.boost.unordered — libs/unordered/examples/
// case_insensitive_test.cpp. The example's helper header
// (examples/case_insensitive.hpp: ihash / iequal_to over
// boost::algorithm::iequals) is copied into the case namespace, and its
// BOOST_TEST checks are doctest CHECKs (the same conditions); both keep the
// TU on imports only.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.unordered;
import mcpplibs.mwrap.boost.algorithm;

namespace hash_examples {
struct iequal_to {
    iequal_to() {}
    explicit iequal_to(std::locale const& l) : locale_(l) {}
    template <typename String1, typename String2>
    bool operator()(String1 const& x1, String2 const& x2) const {
        return boost::algorithm::iequals(x1, x2, locale_);
    }
private:
    std::locale locale_;
};
struct ihash {
    ihash() {}
    explicit ihash(std::locale const& l) : locale_(l) {}
    template <typename String>
    std::size_t operator()(String const& x) const {
        std::size_t seed = 0;
        for (typename String::const_iterator it = x.begin(); it != x.end(); ++it)
            boost::hash_combine(seed, std::toupper(*it, locale_));
        return seed;
    }
private:
    std::locale locale_;
};
}  // namespace hash_examples

namespace {
struct word_info {
    int tag;
    explicit word_info(int t = 0) : tag(t) {}
};

template <class S>
void run(S one, S ONE, S One, S two, S TWO, S Two) {
    boost::unordered_map<S, word_info, hash_examples::ihash, hash_examples::iequal_to> idictionary;
    CHECK(idictionary.empty());
    idictionary[one] = word_info(1);
    CHECK(idictionary.size() == 1);
    CHECK((idictionary.find(ONE) != idictionary.end() && idictionary.find(ONE) == idictionary.find(one)));
    idictionary.insert(std::make_pair(ONE, word_info(2)));
    CHECK(idictionary.size() == 1);
    CHECK((idictionary.find(ONE) != idictionary.end() && idictionary.find(ONE)->first == one &&
           idictionary.find(ONE)->second.tag == 1));
    idictionary[One] = word_info(3);
    CHECK(idictionary.size() == 1);
    CHECK((idictionary.find(ONE) != idictionary.end() && idictionary.find(ONE)->first == one &&
           idictionary.find(ONE)->second.tag == 3));
    idictionary[two] = word_info(4);
    CHECK(idictionary.size() == 2);
    CHECK((idictionary.find(two) != idictionary.end() && idictionary.find(TWO)->first == two &&
           idictionary.find(Two)->second.tag == 4));
}
}  // namespace

TEST_CASE("example: case_insensitive_test") {
    run<std::string>("one", "ONE", "One", "two", "TWO", "Two");                // test1
    run<std::wstring>(L"one", L"ONE", L"One", L"two", L"TWO", L"Two");        // test2
}
