// boost.algorithm string/regex — pure include. The *_regex family is not in
// the module: gcc 16 rejects the boost::regex v5 abi-tag surface inside a
// module interface (tools/export_policy.toml [algorithm]), so it is consumed
// as headers. No import in this TU (gcc include+import ODR rule). Covers
// libs/algorithm/string/example/regex_example.cpp; regex's compiled TUs come
// from the package's base source list.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

TEST_CASE("example: regex_example") {
    std::string str1("abc__(456)__123__(123)__cde");
    CHECK(boost::replace_all_regex_copy(str1, boost::regex("\\(([0-9]+)\\)"),
                                        std::string("#$1#")) == "abc__#456#__123__#123#__cde");
    CHECK(boost::erase_all_regex_copy(str1, boost::regex("[[:alpha:]]+")) ==
          "__(456)__123__(123)__");
    boost::replace_all_regex(str1, boost::regex("_(\\([^\\)]*\\))_"), std::string("-$1-"));
    CHECK(str1 == "abc_-(456)-_123_-(123)-_cde");
}

TEST_CASE("regex finder / find_regex / split_regex") {
    std::string s("a1b22c333");
    auto r = boost::find_regex(s, boost::regex("[0-9]+"));
    CHECK(std::string(r.begin(), r.end()) == "1");
    std::vector<std::string> parts;
    boost::split_regex(parts, s, boost::regex("[0-9]+"));
    CHECK(parts == std::vector<std::string>{"a", "b", "c", ""});
    std::vector<std::string> nums;
    boost::find_all_regex(nums, s, boost::regex("[0-9]+"));
    CHECK(nums == std::vector<std::string>{"1", "22", "333"});
    CHECK(boost::erase_regex_copy(s, boost::regex("[a-z]")) == "1b22c333");
}
