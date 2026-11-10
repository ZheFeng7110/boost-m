// mcpplibs.mwrap.boost.url — the library's customization points through the
// module: a user grammar rule (the Rule concept: value_type + parse(it, end))
// composed with the provided combinators (tuple_rule, optional_rule,
// delim_rule, token_rule over user and provided charsets), a user charset
// (CharSet concept), a user StringToken for decoding into a user buffer, and
// url_view / url used as members of user types. The grammar index page's
// `using namespace boost::urls::grammar;` is how this file names the grammar.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.url;
import mcpplibs.mwrap.boost.core;

namespace urls = boost::urls;
using namespace ::boost::urls::grammar;

namespace user {
// CharSet: a user predicate set.
struct hex_upper_chars_t {
    constexpr bool operator()(char c) const noexcept { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'); }
};
constexpr hex_upper_chars_t hex_upper_chars{};

// Rule: a 24-bit colour "#RRGGBB".
struct colour {
    std::uint8_t r, g, b;
};
struct colour_rule_t {
    using value_type = colour;
    boost::system::result<value_type> parse(char const*& it, char const* end) const noexcept {
        if (end - it < 7 || *it != '#') return boost::urls::grammar::error::mismatch;
        ++it;
        unsigned v[3];
        for (unsigned& c : v) {
            unsigned hi = 0, lo = 0;
            auto hexval = [](char ch, unsigned& out) {
                if (ch >= '0' && ch <= '9') out = unsigned(ch - '0');
                else if (ch >= 'A' && ch <= 'F') out = unsigned(ch - 'A' + 10);
                else return false;
                return true;
            };
            if (!hexval(it[0], hi) || !hexval(it[1], lo)) return boost::urls::grammar::error::invalid;
            c = hi * 16 + lo;
            it += 2;
        }
        return colour{std::uint8_t(v[0]), std::uint8_t(v[1]), std::uint8_t(v[2])};
    }
};
constexpr colour_rule_t colour_rule{};

// StringToken: decodes into a buffer the caller owns (a token is a
// temporary deriving from string_token::arg, used once).
struct buffer_token : boost::urls::string_token::arg {
    using result_type = std::string_view;
    explicit buffer_token(std::array<char, 64>& b) : buf(&b) {}
    char* prepare(std::size_t size) override {
        if (size > buf->size()) throw std::length_error("buffer_token");
        n = size;
        return buf->data();
    }
    result_type result() { return {buf->data(), n}; }
    std::array<char, 64>* buf;
    std::size_t n = 0;
};

struct bookmark {
    std::string title;
    urls::url link;
};
}  // namespace user

TEST_CASE("doc: grammar") {
    // A user rule, alone and inside the provided combinators.
    auto r = parse("#FF8000", user::colour_rule);
    REQUIRE(r.has_value());
    CHECK(r->r == 255);
    CHECK(r->g == 128);
    CHECK_FALSE(parse("FF8000", user::colour_rule).has_value());
    auto pair = parse("#000000,#FFFFFF", tuple_rule(user::colour_rule, squelch(delim_rule(',')), user::colour_rule));
    REQUIRE(pair.has_value());
    CHECK(std::get<1>(*pair).b == 255);
    auto maybe = parse("", optional_rule(user::colour_rule));
    REQUIRE(maybe.has_value());
    CHECK_FALSE(maybe->has_value());
}

TEST_CASE("a user charset with token_rule and find_if") {
    auto t = parse("1A2B", token_rule(user::hex_upper_chars));
    REQUIRE(t.has_value());
    CHECK(*t == "1A2B");
    CHECK_FALSE(parse("1a2b", token_rule(user::hex_upper_chars)).has_value());
    std::string_view s = "xxAB";
    CHECK(find_if(s.data(), s.data() + s.size(), user::hex_upper_chars) == s.data() + 2);
    // A provided charset combined with a user one (lut_chars).
    constexpr lut_chars lc = lut_chars("ABCDEF") + digit_chars;
    CHECK(lc('7'));
    CHECK_FALSE(lc('g'));
}

TEST_CASE("decoding into a user StringToken") {
    urls::url_view u("https://example.com/caf%C3%A9?q=a%20b");
    std::array<char, 64> storage;
    std::string_view path = u.encoded_path().decode({}, user::buffer_token(storage));
    CHECK(path == "/caf\xC3\xA9");
    std::string q;
    u.encoded_query().decode({}, urls::string_token::assign_to(q));
    CHECK(q == "q=a b");
}

TEST_CASE("urls in user types") {
    std::vector<user::bookmark> marks;
    marks.push_back({"boost", urls::url("https://www.boost.org/libs/url")});
    marks[0].link.set_scheme("http");
    marks[0].link.params().append({"ref", "tests"});
    CHECK(marks[0].link.buffer() == "http://www.boost.org/libs/url?ref=tests");
    std::map<urls::url, std::string> by_url;
    by_url[marks[0].link] = marks[0].title;
    CHECK(by_url.begin()->second == "boost");
}
