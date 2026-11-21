// mcpplibs.mwrap.boost.url — the official example programs (libs/url/example/
// {mailto,magnet,qrcode,finicky,sanitize,suffix_list}) ported mechanically:
// each runs as a case with the command line its usage text documents, and its
// output must equal what the same code printed when built from upstream/boost
// headers alone (with the url and json TUs). finicky reads the example's own
// config.json, suffix_list a subset of the public suffix list, both written to
// the temporary directory; the examples' own headers are inlined; finicky's CHECK macro is EX_CHECK here (doctest's
// CHECK). router and file_router are beast HTTP servers (excluded in
// coverage.toml).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
import std;
import mcpplibs.mwrap.boost.url;
import mcpplibs.mwrap.boost.json;
import mcpplibs.mwrap.boost.core;
import mcpplibs.mwrap.boost.regex;

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

// libs/url/example/mailto/mailto.cpp
namespace ex_mailto {
// ---- inlined rfc.hpp ----
#undef BOOST_URL_EXAMPLE_MAILTO_MAILTO_GRAMMAR_HPP
//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef BOOST_URL_EXAMPLE_MAILTO_MAILTO_GRAMMAR_HPP
#define BOOST_URL_EXAMPLE_MAILTO_MAILTO_GRAMMAR_HPP


namespace urls = boost::urls;
namespace grammar = boost::urls::grammar;

/// The set of dtext_no_obs characters
struct dtext_no_obs_chars_t
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        // dtext-no-obs = %d33-90 / %d94-126
        return (c >= '!' && c <= 'Z') ||
               (c >= '^' && c <= '~');
    }
};

/// A character set containing dtext_no_obs characters.
constexpr dtext_no_obs_chars_t dtext_no_obs_chars{};

/// A character set containing crlf characters.
constexpr auto crlf_chars =
    grammar::lut_chars("\n\r");

/// A character set containing atext characters.
constexpr auto atext_chars =
    grammar::lut_chars("!#$%&'*+-/=?^_`{|}~") +
    grammar::alnum_chars +
    grammar::digit_chars;

/// A character set containing wsp characters.
constexpr auto wsp_chars =
    grammar::lut_chars(" \t");

/// The set of obs_ctext characters
struct obs_ctext_chars_t
{
    static constexpr unsigned char c1 = char(1);
    static constexpr unsigned char c2 = char(8);
    static constexpr unsigned char c3 = char(11);
    static constexpr unsigned char c4 = char(12);
    static constexpr unsigned char c5 = char(14);
    static constexpr unsigned char c6 = char(31);
    static constexpr unsigned char c7 = char(127);

    constexpr obs_ctext_chars_t() noexcept = default;

    constexpr
    bool
    operator()(char c) const noexcept
    {
        // obs-ctext = obs-NO-WS-CTL
        // obs-NO-WS-CTL = %d1-8 / %d11 / %d12 / %d14-31 / %d127
        return
            (static_cast<unsigned char>(c) >= c1 &&
             static_cast<unsigned char>(c) <= c2) ||
            (static_cast<unsigned char>(c) >= c3 &&
             static_cast<unsigned char>(c) <= c4) ||
            static_cast<unsigned char>(c) == c5 ||
            static_cast<unsigned char>(c) == c6 ||
            static_cast<unsigned char>(c) == c7;
    }
};

/// A character set containing obs_ctext characters.
constexpr obs_ctext_chars_t obs_ctext_chars{};

/// A character set containing obs_qtext characters.
constexpr auto obs_qtext_chars = obs_ctext_chars;

/// The set of ctext characters
struct ctext_chars_t
{
    constexpr ctext_chars_t() noexcept = default;

    constexpr
    bool
    operator()(char c) const noexcept
    {
        // ctext = %d33-39 / %d42-91 / %d93-126 / obs-ctext
        return
            (c >= '!' && c <= '\'') ||
            (c >= '*' && c <= '[') ||
            (c >= ']' && c <= '~') ||
            obs_ctext_chars(c);
    }
};

/// A character set containing ctext characters.
constexpr ctext_chars_t ctext_chars{};

/// The set of qtext characters
struct qtext_chars_t
{
    constexpr qtext_chars_t() noexcept = default;

    constexpr
    bool
    operator()(char c) const noexcept
    {
        // qtext = %d33 / %d35-91 / %d93-126 / obs-qtext
        return c == '!' ||
               (c >= '#' && c <= '[') ||
               (c >= ']' && c <= '~') ||
               obs_qtext_chars(c);
    }
};

/// A character set containing qtext characters.
constexpr qtext_chars_t qtext_chars{};

/// A character set containing qchars
constexpr auto qchars = urls::unreserved_chars + "!$\\()*+,;:@";

constexpr auto atext_token =
    grammar::token_rule(atext_chars);

/// Rule for dot-atom-text = 1*atext *("." 1*atext)
constexpr auto dot_atom_text_rule =
    grammar::range_rule(
        atext_token,
        grammar::tuple_rule(
            grammar::squelch(
                grammar::delim_rule('.')),
            atext_token));

/// Rule for "[" *dtext-no-obs "]"
constexpr auto quoted_dtext_no_obs =
    grammar::tuple_rule(
        grammar::squelch(
            grammar::delim_rule('[')),
        grammar::optional_rule(
            grammar::token_rule(dtext_no_obs_chars)),
        grammar::squelch(
            grammar::delim_rule(']')));

/// Rule for domain = dot-atom-text / "[" *dtext-no-obs "]"
constexpr auto domain_rule =
    grammar::variant_rule(
        dot_atom_text_rule,
        quoted_dtext_no_obs);

/// Rule for obs-qp = "\" (%d0 / obs-NO-WS-CTL / LF / CR)
constexpr auto obs_qp_rule =
    grammar::tuple_rule(
        grammar::delim_rule('\\'),
        grammar::variant_rule(
            grammar::delim_rule('\0'),
            grammar::delim_rule(ctext_chars),
            grammar::delim_rule(crlf_chars)));

/// Rule for quoted-pair = ("\" (VCHAR / WSP)) / obs-qp
constexpr auto quoted_pair_rule =
    grammar::variant_rule(
        grammar::tuple_rule(
            grammar::delim_rule('\\'),
            grammar::variant_rule(
                grammar::delim_rule(grammar::vchars),
                grammar::delim_rule(' '))),
        obs_qp_rule);

/// Rule for obs-FWS = 1*WSP *(CRLF 1*WSP)
constexpr auto obs_fws_rule =
    grammar::tuple_rule(
        grammar::token_rule(wsp_chars),
        grammar::range_rule(
            grammar::delim_rule(crlf_chars),
            grammar::token_rule(wsp_chars)));

/// Rule for FWS = ([*WSP CRLF] 1*WSP) / obs-FWS
constexpr auto fws_rule =
    grammar::variant_rule(
        grammar::tuple_rule(
            grammar::optional_rule(
                grammar::tuple_rule(
                    grammar::optional_rule(
                        grammar::token_rule(wsp_chars)),
                    grammar::delim_rule(crlf_chars))),
            grammar::token_rule(wsp_chars)),
        obs_fws_rule);

namespace detail
{
    // workaround for value-based recursive rules
    struct ccontent_and_comment_rules {
        struct ccontent_rule_t
        {
            using value_type = boost::core::string_view;

            boost::system::result< value_type >
            parse(
                char const*& it,
                char const* end
            ) const noexcept
            {
                auto it0 = it;
                bool v = ccontent_and_comment_rules::
                    parse_ccontent(it, end);
                if (v)
                    return boost::core::string_view(it0, it);
                return grammar::error::invalid;
            }
        };

        static
        bool
        parse_ccontent(char const*& it, char const* end) noexcept
        {
            // ccontent = ctext / quoted-pair / comment
            return
                grammar::parse(
                    it, end,
                    grammar::variant_rule(
                        grammar::delim_rule(ctext_chars),
                        quoted_pair_rule,
                        ccontent_rule_t{})).has_value();
        };

        struct comment_rule_t
        {
            using value_type = boost::core::string_view;

            boost::system::result< value_type >
            parse(
                char const*& it,
                char const* end
            ) const noexcept
            {
                auto it0 = it;
                bool v = ccontent_and_comment_rules::
                    parse_comment(it, end);
                if (v)
                    return boost::core::string_view(it0, it);
                return grammar::error::invalid;
            }
        };

        static
        bool
        parse_comment(char const*& it, char const* end) noexcept
        {
            // comment = "(" *([FWS] ccontent) [FWS] ")"
            return grammar::parse(
                it, end,
                grammar::tuple_rule(
                    grammar::delim_rule('('),
                    grammar::range_rule(
                        grammar::tuple_rule(
                            grammar::optional_rule(fws_rule),
                            ccontent_rule_t{})),
                    grammar::optional_rule(fws_rule),
                    grammar::delim_rule(')'))).has_value();
        }
    };
}

/// Rule for ccontent = ctext / quoted-pair / comment
constexpr auto ccontent_rule =
    detail::ccontent_and_comment_rules::ccontent_rule_t{};

/// Rule for comment = "(" *([FWS] ccontent) [FWS] ")"
constexpr auto comment_rule =
    detail::ccontent_and_comment_rules::comment_rule_t{};

/// Rule for CFWS = (1*([FWS] comment) [FWS]) / FWS
constexpr auto cfws_rule =
    grammar::variant_rule(
        grammar::tuple_rule(
            grammar::range_rule(
                grammar::tuple_rule(
                    grammar::optional_rule(fws_rule),
                    comment_rule), 1),
            grammar::optional_rule(fws_rule)),
        fws_rule);

/// Rule for qcontent = qtext / quoted-pair
constexpr auto qcontent_rule =
    grammar::variant_rule(
        grammar::delim_rule(qtext_chars),
        quoted_pair_rule);

/// Rule for quoted-string = [CFWS] DQUOTE *([FWS] qcontent) [FWS] DQUOTE [CFWS]
constexpr auto quoted_string_rule =
    grammar::tuple_rule(
        grammar::optional_rule(cfws_rule),
        grammar::delim_rule('"'),
        grammar::range_rule(
            grammar::tuple_rule(
                grammar::optional_rule(fws_rule),
                qcontent_rule)),
        grammar::optional_rule(fws_rule),
        grammar::delim_rule('"'),
        grammar::optional_rule(cfws_rule));

/// Rule for local-part = dot-atom-text / quoted-string
constexpr auto local_part_rule =
    grammar::variant_rule(
        dot_atom_text_rule,
        quoted_string_rule);

/// Rule for addr-spec = local-part "@" domain
constexpr auto addr_spec_rule =
    grammar::tuple_rule(
        local_part_rule,
        grammar::squelch(
            grammar::delim_rule('@')),
        domain_rule);

/// Rule for to = addr-spec *("," addr-spec )
constexpr auto to_rule =
    grammar::range_rule(
        addr_spec_rule,
        grammar::tuple_rule(
            grammar::squelch(
                grammar::delim_rule(',')),
            addr_spec_rule));

/// Rule for hfvalue = *qchar
constexpr auto hfvalue_rule = urls::pct_encoded_rule(qchars);

/// Rule for hfname = *qchar
struct hfname_rule_t
{
    using value_type = boost::core::string_view;

    boost::system::result<value_type>
    parse(
        char const*& it,
        char const* end
    ) const noexcept
    {
        boost::core::string_view s(it, end);
        it += s.size();
        auto r = grammar::parse(s, hfvalue_rule);
        if (!r)
            return r.error();

        // The user agent interpreting a 'mailto' URI SHOULD NOT create a
        // message if any of the header fields are considered dangerous
        static const boost::core::string_view valid_k[] = {
            "to", "subject", "keywords",
            "cc", "body",    "in-reply-to"
        };
        if (std::any_of(
                std::begin(valid_k), std::end(valid_k),
                [s](boost::core::string_view valid_k)
            {
                return grammar::ci_is_equal(s, valid_k);
            }))
            return s;
        return grammar::error::invalid;
    }
};

constexpr auto hfname_rule = hfname_rule_t{};

#endif
// ---- end rfc.hpp ----

namespace urls = boost::urls;

// fwd-declaration for mailto_view
struct mailto_rule_t;

/// A new url type for mailto URLs
/**
    This class represents a URI with the mailto
    scheme.

    Unlike a urls::url_view, which only represents
    the general syntax of urls, a mailto_view
    represents a reference to fields that are
    relevant to mailto URLs, while ignoring
    elements of the general syntax
    that are not relevant to the scheme.

    This allows us to use the general syntax
    parsers to create a representation that
    is more appropriate for the specified scheme
    syntax.

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc6068"
        >The 'mailto' URI Scheme</a>
    @li <a href="https://www.rfc-editor.org/errata/rfc6068"
        >RFC Errata Report</a>

    @par References
    @li <a href="https://en.wikipedia.org/wiki/Mailto"
        >mailto (Wikipedia)</a>

 */
class mailto_view
{
    urls::url_view u_;

public:
    /// Return the specified email address in the URL
    /**
        A mailto URL might contain multiple email
        addresses separated by commas.

        The first addresses are represented in
        the path. Other addresses are in
        any query parameter whose key is "to".

        @param i Address index

        @return The specified address
     */
    std::string
    address(std::size_t i = 0) const;

    /// @copydoc address()
    urls::pct_string_view
    encoded_address(std::size_t i = 0) const noexcept;

    /// Return number of email addresses in the URL
    std::size_t
    size() const noexcept;

    /// Return the specified cc email address in the URL
    /**
        A mailto URL might contain multiple cc
        email addresses separated by commas.

        Addresses can be represented in any query
        parameter whose key is "cc".

        @param i Address index

        @return The specified cc address
     */
    std::string
    cc(std::size_t i) const;

    /// @copydoc cc()
    urls::pct_string_view
    encoded_cc(std::size_t i) const noexcept;

    /// Return number of "cc" email addresses in the URL
    std::size_t
    size_cc() const noexcept;

    /// Return email message subject
    std::string
    subject() const;

    /// @copydoc subject()
    urls::pct_string_view
    encoded_subject() const noexcept;

    /// Return email message body
    std::string
    body() const;

    /// @copydoc body()
    urls::pct_string_view
    encoded_body() const noexcept;

    friend
    std::ostream&
    operator<<(std::ostream& os, mailto_view m)
    {
        return os << m.u_;
    }

private:
    // Count number of addresses in a string
    static
    std::size_t
    addr_in_str(boost::core::string_view s);

    // Get the ith address from a string
    static
    boost::optional<urls::pct_string_view>
    get_nth_address(boost::core::string_view to, std::size_t &i) noexcept;

    // Get param value or empty otherwise
    urls::pct_string_view
    param_or_empty(urls::pct_string_view k) const noexcept;

    friend mailto_rule_t;
};

/** Rule to match a mailto URL
*/
struct mailto_rule_t
{
    /// Value type returned by the rule
    using value_type = mailto_view;

    /// Parse a sequence of characters into a mailto_view
    boost::system::result< value_type >
    parse( char const*& it, char const* end ) const noexcept;
};

constexpr mailto_rule_t mailto_rule{};

/** Return a parsed mailto URL from a string, or error.

    This is a more convenient user-facing function
    to parse mailto URLs.
*/
boost::system::result< mailto_view >
parse_mailto( boost::core::string_view s ) noexcept
{
    return urls::grammar::parse(s, mailto_rule);
}

int example_main(int argc, char** argv)
{
    // This example shows how to use custom parsing
    // to process alternate URI schemes, in this
    // case "mailto"
    if (argc != 2) {
        ex_out << argv[0] << "\n";
        ex_out << "mailto <URL>\n"
                     "examples:\n"
                     // Single e-mail address
                     "mailto mailto:someone@example.com\n"
                     // Two e-mail addresses
                     "mailto mailto:someone@example.com,someoneelse@example.com\n"
                     // E-mail headers
                     "mailto mailto:someone@example.com?subject=Our%20meeting&cc=someone_else@example.com&body=Hi%21\n"
                     // E-mail headers only
                     "mailto mailto:?to=&subject=mailto%20example&body=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FMailto\n"
                     // All fields
                     "mailto mailto:someone@example.com,%73omeoneelse@me.com?to=thirdperson@example.com&subject=Our%20meeting&cc=someone_else@example.com,onemore@ex%61mple.com&body=Hi%21\n";
        return EXIT_FAILURE;
    }

    boost::system::result<mailto_view> r =
        parse_mailto(argv[1]);
    if (!r)
        return EXIT_FAILURE;

    mailto_view m = *r;
    ex_out << "link: " << m << "\n";

    for (std::size_t i = 0; i < m.size(); ++i)
        ex_out <<
            "to[" << i << "]: " <<
            m.address(i) << "\n";

    for (std::size_t i = 0; i < m.size_cc(); ++i)
        ex_out <<
            "cc[" << i << "]: " <<
            m.address(i) << "\n";

    ex_out << "subject: " << m.subject() << "\n";
    ex_out << "body: " << m.body() << "\n";

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

std::string
mailto_view::address(std::size_t i) const
{
    return encoded_address(i).decode();
}

urls::pct_string_view
mailto_view::encoded_address(std::size_t i) const noexcept
{
    // Look for ith email address in the path string
    auto s = get_nth_address(u_.encoded_path(), i);
    if (s)
        return *s;

    // Look for ith email address in one of the "to" headers
    auto ps = u_.encoded_params();
    auto it = ps.find("to", urls::ignore_case);
    while (it != ps.end())
    {
        s = get_nth_address((*it++).value, i);
        if (s)
            return *s;
        it = ps.find(it, "to", urls::ignore_case);
    }
    return {};
}

std::size_t
mailto_view::size() const noexcept
{
    // Count addresses in path
    std::size_t n = addr_in_str(u_.encoded_path());

    // Count addresses in "to" headers
    auto ps = u_.encoded_params();
    auto it = ps.find("to", urls::ignore_case);
    while (it != ps.end())
    {
        n += addr_in_str((*it++).value);
        it = ps.find(it, "to", urls::ignore_case);
    }
    return n;
}

std::string
mailto_view::cc(std::size_t i) const
{
    return encoded_cc(i).decode();
}

urls::pct_string_view
mailto_view::encoded_cc(std::size_t i) const noexcept
{
    // Look for ith email address in one of the "to" headers
    auto ps = u_.encoded_params();
    auto it = ps.find("cc", urls::ignore_case);
    while (it != ps.end())
    {
        auto s = get_nth_address((*it++).value, i);
        if (s)
            return *s;
        it = ps.find(it, "cc", urls::ignore_case);
    }
    return {};
}

std::size_t
mailto_view::size_cc() const noexcept
{
    // Count addresses in "to" headers
    std::size_t n = 0;
    auto ps = u_.encoded_params();
    auto it = ps.find("cc", urls::ignore_case);
    while (it != ps.end())
    {
        n += addr_in_str((*it++).value);
        it = ps.find(it, "cc", urls::ignore_case);
    }
    return n;
}

std::string
mailto_view::subject() const
{
    return encoded_subject().decode();
}

urls::pct_string_view
mailto_view::encoded_subject() const noexcept
{
    return param_or_empty("subject");
}

std::string
mailto_view::mailto_view::body() const
{
    return encoded_body().decode();
}

urls::pct_string_view
mailto_view::encoded_body() const noexcept
{
    return param_or_empty("body");
}

std::size_t
mailto_view::addr_in_str(boost::core::string_view s)
{
    std::size_t n = 0;
    bool empty = true;
    for (char c : s)
    {
        if (c == ',')
        {
            n += !empty;
            empty = true;
        }
        else
        {
            empty = false;
        }
    }
    n += !empty;
    return n;
}

boost::optional<urls::pct_string_view>
mailto_view::get_nth_address(boost::core::string_view to, std::size_t &i) noexcept
{
    auto p = to.find(',');
    while (p != boost::core::string_view::npos)
    {
        if (i == 0)
            return urls::pct_string_view(
                to.substr(0, p));
        --i;
        to.remove_prefix(p + 1);
        p = to.find(',');
    }
    if (!to.empty())
    {
        if (i == 0)
            return urls::pct_string_view(
                to.substr(0, p));
        --i;
    }
    return boost::none;
}

urls::pct_string_view
mailto_view::param_or_empty(urls::pct_string_view k) const noexcept
{
    auto ps = u_.encoded_params();
    auto it = ps.find(k, urls::ignore_case);
    if (it != ps.end())
        return (*it).value;
    return {};
}

auto
mailto_rule_t::parse( char const*& it, char const* end ) const noexcept
    -> boost::system::result< value_type >
{
    // Syntax-based rules
    boost::system::result<urls::url_view> r =
        urls::grammar::parse(it, end, urls::absolute_uri_rule);
    if (!r)
        return r.error();

    // Scheme-based rules
    mailto_view m;
    m.u_ = *r;
    auto valid_header = [](urls::param_pct_view p) {
        return
            urls::grammar::parse(p.key, hfname_rule) &&
            urls::grammar::parse(p.value, hfvalue_rule) &&
            p.has_value &&
            (!urls::grammar::ci_is_equal(p.key, "to") ||
             urls::grammar::parse(p.value, addr_spec_rule));
    };
    auto ps = m.u_.encoded_params();
    if (m.u_.scheme() == "mailto" &&
        !m.u_.has_authority() &&
        urls::grammar::parse(m.u_.encoded_path(), to_rule) &&
        std::all_of(ps.begin(), ps.end(), valid_header))
        return m;
    return urls::grammar::error::invalid;
}

//]

TEST_CASE("example: mailto") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back(std::string("mailto:someone@example.com,%73omeoneelse@me.com?to=thirdperson@example.com&subject=Our%20meeting&cc=someone_else@example.com,onemore@ex%61mple.com&body=Hi%21"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(link: mailto:someone@example.com,%73omeoneelse@me.com?to=thirdperson@example.com&subject=Our%20meeting&cc=someone_else@example.com,onemore@ex%61mple.com&body=Hi%21
to[0]: someone@example.com
to[1]: someoneelse@me.com
to[2]: thirdperson@example.com
cc[0]: someone@example.com
cc[1]: someoneelse@me.com
subject: Our meeting
body: Hi!
)ex"));
}
}  // namespace ex_mailto

// libs/url/example/magnet/magnet.cpp
namespace ex_magnet {
// ---- inlined filter_view.hpp ----
#undef BOOST_URL_EXAMPLE_GRAMMAR_MAGNER_FILTERED_VIEW_HPP
//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef BOOST_URL_EXAMPLE_GRAMMAR_MAGNER_FILTERED_VIEW_HPP
#define BOOST_URL_EXAMPLE_GRAMMAR_MAGNER_FILTERED_VIEW_HPP

/// A small filter and transform view
/**
   This is a view that represents filtered
   and transformed members of the underlying
   container.

   All elements in the container are filtered
   with a given predicate. Elements that pass
   the predicate are then transformed into an
   element of a specified value type.

   @tparam Container Type of underlying container
   @tparam Value Type of elements after transform
   @tparam Predicate Type of predicate to filter elements
   @tparam Transform Type of transform function on filtered elements
 */
template <class Container, class Value, class Predicate, class Transform>
class filter_view
{
    Container c_;
    Predicate p_;
    Transform t_;

public:
    using value_type = Value;

    class iterator
    {
        friend filter_view;

        using base_iter = typename Container::iterator;

        base_iter b_;
        filter_view& v_;

        void
        skip_filtered()
        {
            // Increment the base iterator
            // skipping all elements that
            // don't pass the filter.
            auto e = v_.c_.end();
            while (b_ != e && !v_.p_(*b_))
                ++b_;
        }

        iterator(
            filter_view& v,
            base_iter it)
            : b_(it)
            , v_(v)
        {
            skip_filtered();
        }

    public:
        iterator&
        operator++() noexcept
        {
            ++b_;
            skip_filtered();
            return *this;
        }

        iterator
        operator++(int) noexcept
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        value_type
        operator*()
        {
            return v_.t_(*b_);
        }

        friend
        bool
        operator==(
            iterator const& a,
            iterator const& b) noexcept
        {
            return a.b_ == b.b_;
        }

        friend
        bool
        operator!=(
            iterator const& a,
            iterator const& b) noexcept
        {
            return a.b_ != b.b_;
        }
    };

    filter_view(
        Container const& base,
        Predicate pred = {},
        Transform transform = {})
        : c_(base)
        , p_(pred)
        , t_(transform) {};

    iterator begin()
    {
        return {*this, c_.begin()};
    }

    iterator end()
    {
        return {*this, c_.end()};
    }
};

/// The identity function
template <class T>
struct identity_fn
{
    T operator()(T const& t) { return t; }
};

/// Return a container view filtered with Predicate
template <class Container, class Value, class Predicate>
filter_view<
    Container,
    typename Container::value_type,
    Predicate,
    identity_fn<typename Container::value_type>>
filter(Container&& c, Predicate&& p) {
    return {
        std::forward<Container>(c),
        std::forward<Predicate>(p),
        identity_fn<typename Container::value_type>{}
    };
}

/// Return a container view filtered with Predicate and Transform applied
template <class Container, class Value, class Predicate, class Transform>
filter_view<
    Container,
    typename Container::value_type,
    Predicate,
    Transform>
filter(Container&& c, Predicate&& p, Transform&& t) {
    return {
        std::forward<Container>(c),
        std::forward<Predicate>(p),
        std::forward<Transform>(t)
    };
}


#endif
// ---- end filter_view.hpp ----

namespace urls = boost::urls;
namespace core = boost::core;

/** Callable to identify a magnet "exact topic"

    This callable evaluates if a query parameter
    represents a magnet "exact topic".

    This callable is used as a filter for
    the topics_view.
 */
struct is_exact_topic
{
    bool
    operator()(urls::param_view p);
};

/** Callable to identify a magnet url parameter

    This callable evaluates if a query parameter
    has a given key and a url as its value.

    These urls are percent-encoded twice,
    which means we need to decode it once
    before attempting to parse it.

    This callable is used as a filter for
    the keys_view.
 */
class is_url_with_key
{
    core::string_view k_;
public:
    is_url_with_key(
        core::string_view key)
        : k_(key) {}

    bool
    operator()(urls::param_view p);
};

/** Callable to convert param values to urls

    This callable converts the value of a
    query parameter into a urls::url_view.

    This callable is used as a transform
    function for the topics_view.
 */
struct param_view_to_url
{
    urls::url
    operator()(urls::param_view p);
};

/** Callable to convert param values to std::string

    This callable converts the value of a
    query parameter into a std::string.

    This callable is used as a transform
    function for the keys_view.
 */
struct to_decoded_value
{
    std::string
    operator()(urls::param_view p)
    {
        return p.value;
    }
};

/** Callable to convert param values to info_hashes

    This callable converts the value of a
    query parameter into a core::string_view with
    its infohash.

    The infohash hash is a parameter of an
    exact topic field in the magnet link.

    This callable is used as a transform
    function for the info_hashes_view.
 */
struct param_view_to_infohash
{
    core::string_view
    operator()(urls::param_view p);
};

/** Callable to convert param values to protocols

    This callable converts the value of a
    query parameter into a core::string_view with
    its protocol.

    The protocol is a parameter of an exact
    topic field in the magnet link.

    This callable is used as a transform
    function for the protocols_view.
 */
struct to_protocol
{
    core::string_view
    operator()(urls::param_view p);
};

struct magnet_link_rule_t;

/** A new url type for magnet links

    This class represents a reference to a
    magnet link.

    Unlike a urls::url_view, which only represents the
    general syntax of urls, a magnet_link_view
    represents a reference to fields that are
    relevant to magnet links, while ignoring
    elements of the general syntax
    that are not relevant to the scheme.

    This allows us to use the general syntax
    parsers to create a representation that
    is more appropriate for the specified scheme
    syntax.

    @par Specification
    @li <a href="https://www.bittorrent.org/beps/bep_0005.html"
        >DHT Protocol</a>
    @li <a href="https://www.bittorrent.org/beps/bep_0009.html"
        >Extension for Peers to Send Metadata Files</a>
    @li <a href="https://www.bittorrent.org/beps/bep_0053.html"
        >Magnet URI extension</a>
    @li <a href="https://en.wikipedia.org/wiki/Magnet_URI_scheme"
        >Magnet URI scheme</a>

    @par References
    @li <a href="https://github.com/webtorrent/magnet-uri"
        >magnet-uri</a>

 */
class magnet_link_view
{
    urls::url_view u_;

public:
    /// A view of all exact topics in the magnet_link
    using topics_view =
        filter_view<
            urls::params_view,
            urls::url,
            is_exact_topic,
            param_view_to_url>;

    /// A view of all info_hashes in the magnet_link
    using info_hashes_view =
        filter_view<
            urls::params_view,
            std::string,
            is_exact_topic,
            param_view_to_infohash>;

    /// A view of all protocols in the magnet_link
    using protocols_view =
        filter_view<
            urls::params_view,
            std::string,
            is_exact_topic,
            to_protocol>;

    /** A view of all urls with the specified key in the magnet_link

        A number of fields in a magnet link refer
        to a list of urls with the same query
        parameter keys.
    */
    using keys_view =
        filter_view<
            urls::params_view,
            std::string,
            is_url_with_key,
            to_decoded_value>;

    /** URNs to the file or files hashes

        An exact topic is the main field of a
        magnet link. A magnet link must contain
        one or more exact topics with the query
        key "xt" or ["xt.1", "xt.2", ...].

        The value of each exact topic is a URN
        representing the file hash and the protocol
        to access the file.

        @return A view of all exact topic URNs in the link
    */
    topics_view
    exact_topics() const noexcept;

    /** Info hash of the file or files

        @return A view of all info hashes in exact topics
    */
    info_hashes_view
    info_hashes() const noexcept;

    /** Protocol of the exact topics

        @return A view of all protocols in exact topics
    */
    protocols_view
    protocols() const noexcept;

    /** Return view of address trackers

        A tracker URL is used to obtain resources
        for BitTorrent downloads.

        @return A view of all address trackers in the link
    */
    keys_view
    address_trackers() const;

    /** Return view of exact sources

        An exact source URL is a direct download
        link to the file.

        @return A view of all exact sources
    */
    keys_view
    exact_sources() const;

    /** Return view of acceptable sources

        An acceptable source URL is a direct
        download link to the file that can be
        used as a fallback for exact sources.

        @return A view of all acceptable sources
    */
    keys_view
    acceptable_sources() const;

    /** Return keyword topic

        The keyword topic is the search keywords
        to use in P2P networks.

        @par Example
        kt=martin+luther+king+mp3

        @return Keyword topic
    */
    boost::optional<std::string>
    keyword_topic() const noexcept;

    /** Return manifest topics

        This function returns a link to the
        metafile that contains a list of magneto.

        @par Specification
        @li <a href="http://rakjar.de/gnuticles/MAGMA-Specsv22.txt"
            >MAGnet MAnifest</a>

        @return A view of manifest topics
    */
    keys_view
    manifest_topics() const;

    /** Return display name

        This function returns a filename to
        display to the user. This field is
        only used for convenience.

        @par Specification
        @li <a href="http://rakjar.de/gnuticles/MAGMA-Specsv22.txt"
            >MAGnet MAnifest</a>

        @return Display name
    */
    boost::optional<urls::pct_string_view>
    display_name() const noexcept;

    /** Return web seed

        The web seed represents the payload data
        served over HTTP(S).

        @return Web seed
    */
    keys_view
    web_seed() const;

    /** Return extra supplement parameter

        This function returns informal options
        and parameters of the magnet link.

        Query parameters whose keys have the
        prefix "x." are used in magnet links
        for extra parameters. These names
        are guaranteed to never be standardized.

        @par Example
        x.parameter_name=parameter_data

        @return Web seed
    */
    boost::optional<urls::pct_string_view>
    param(core::string_view key) const noexcept;

    friend
    std::ostream&
    operator<<(std::ostream& os, magnet_link_view m)
    {
        return os << m.u_;
    }

private:
    // get a query parameter as a urls::pct_string_view
    boost::optional<urls::pct_string_view>
    encoded_param(core::string_view key) const noexcept;

    // get a query parameter as a urls::url_view
    boost::optional<urls::url_view>
    url_param(core::string_view key) const noexcept;

    friend magnet_link_rule_t;
};

bool
is_exact_topic::
operator()(urls::param_view p)
{
    // These comparisons use the lazy
    // operator== for urls::pct_string_view
    // For instance, the comparison also works
    // if the underlying key is "%78%74"/
    if (p.key == "xt")
        return true;
    return
        p.key.size() > 3 &&
        *std::next(p.key.begin(), 0) == 'x' &&
        *std::next(p.key.begin(), 1) == 't' &&
        *std::next(p.key.begin(), 2) == '.' &&
        std::all_of(
            std::next(p.key.begin(), 3),
            p.key.end(),
            urls::grammar::digit_chars);
}

bool
is_url_with_key::
operator()(urls::param_view p)
{
    if (p.key != k_)
        return false;
    boost::system::error_code ec;
    std::string buf(
        p.value.begin(), p.value.end());
    if (ec.failed())
        return false;
    boost::system::result<urls::url_view> r =
        urls::parse_uri(buf);
    return r.has_value();
}

urls::url
param_view_to_url::
operator()(urls::param_view p)
{
    // `param_view_to_url` is used in topics_view,
    // where the URL is not
    // percent-encoded twice.
    // Thus, we can already parse the
    // encoded value.
    auto ur =
        urls::parse_uri(p.value);
    BOOST_ASSERT(ur);
    urls::url u = *ur;
    return u;
}

core::string_view
param_view_to_infohash::
operator()(urls::param_view p)
{
    urls::url_view topic =
        urls::parse_uri(p.value).value();
    core::string_view t = topic.encoded_path();
    std::size_t pos = t.find_last_of(':');
    if (pos != core::string_view::npos)
        return t.substr(pos + 1);
    return t;
}

core::string_view
to_protocol::
operator()(urls::param_view p)
{
    urls::url_view topic =
        urls::parse_uri(p.value).value();
    core::string_view t = topic.encoded_path();
    std::size_t pos = t.find_last_of(':');
    return t.substr(0, pos);
}

auto
magnet_link_view::exact_topics() const noexcept
    -> topics_view
{
    return {u_.params()};
}

auto
magnet_link_view::info_hashes() const noexcept
    -> info_hashes_view
{
    return {u_.params()};
}

auto
magnet_link_view::protocols() const noexcept
    -> protocols_view
{
    return {u_.params()};
}

auto
magnet_link_view::address_trackers() const
    -> keys_view
{
    return {
        u_.params(),
        is_url_with_key{"tr"}};
}

auto
magnet_link_view::exact_sources() const
    -> keys_view
{
    return {
        u_.params(),
        is_url_with_key{"xs"}};
}

auto
magnet_link_view::acceptable_sources() const
    -> keys_view
{
    return {
        u_.params(),
        is_url_with_key{"as"}};
}

boost::optional<std::string>
magnet_link_view::keyword_topic() const noexcept
{
    boost::optional<urls::pct_string_view> o =
        encoded_param("kt");
    if (o)
        return o->decode();
    return boost::none;
}

auto
magnet_link_view::manifest_topics() const
    -> keys_view
{
    return {
        u_.params(),
        is_url_with_key{"mt"}};
}

boost::optional<urls::pct_string_view>
magnet_link_view::display_name() const noexcept
{
    return encoded_param("dn");
}

auto
magnet_link_view::web_seed() const
    -> keys_view
{
    return {
        u_.params(),
        is_url_with_key{"ws"}};
}

boost::optional<urls::pct_string_view>
magnet_link_view::param(core::string_view key) const noexcept
{
    urls::params_view ps = u_.params();
    auto it = ps.begin();
    auto end = ps.end();
    while (it != end)
    {
        urls::param_view p = *it;
        if (p.key.size() < 2)
        {
            ++it;
            continue;
        }
        auto first = p.key.begin();
        auto mid = std::next(p.key.begin(), 2);
        auto last = p.key.end();
        urls::pct_string_view prefix(
            core::string_view(first, mid));
        urls::pct_string_view suffix(
            core::string_view(mid, last));
        if (prefix == "x." &&
            suffix == key &&
            p.has_value)
            return urls::pct_string_view(p.value);
        ++it;
    }
    return boost::none;
}

boost::optional<urls::pct_string_view>
magnet_link_view::encoded_param(core::string_view key) const noexcept
{
    urls::params_encoded_view ps = u_.encoded_params();
    auto it = ps.find(key);
    if (it != ps.end() && (*it).has_value)
        return urls::pct_string_view((*it).value);
    return boost::none;
}

boost::optional<urls::url_view>
magnet_link_view::url_param(core::string_view key) const noexcept
{
    urls::params_encoded_view ps = u_.encoded_params();
    auto it = ps.find(key);
    if (it != ps.end() && (*it).has_value)
    {
        boost::system::result<urls::url_view> r =
            urls::parse_uri((*it).value);
        if (r)
            return *r;
    }
    return boost::none;
}

/** Rule to match a magnet link
*/
struct magnet_link_rule_t
{
    /// Value type returned by the rule
    using value_type = magnet_link_view;

    /// Parse a sequence of characters into a magnet_link_view
    boost::system::result< value_type >
    parse( char const*& it, char const* end ) const noexcept;
};

auto
magnet_link_rule_t::parse(
    char const*& it,
    char const* end ) const noexcept
    -> boost::system::result< value_type >
{
    // 1) Parse url with the general uri syntax
    boost::system::result<urls::url_view> r =
        urls::grammar::parse(it, end, urls::absolute_uri_rule);
    if(!r)
        return urls::grammar::error::invalid;
    magnet_link_view m;
    m.u_ = *r;

    // 2) Check if exact topics are valid urls
    // and that we have at least one. This is the
    // only mandatory field in magnet links.
    auto ps = m.u_.params();
    auto pit = ps.begin();
    auto pend = ps.end();
    pit = std::find_if(pit, pend, is_exact_topic{});
    if (pit == pend)
    {
        // no exact topic in the magnet link
        return urls::grammar::error::invalid;
    }

    // all topics should parse as valid urls
    if (!std::all_of(pit, pend, [](
        urls::param_view p)
    {
        if (!is_exact_topic{}(p))
            return true;
        boost::system::result<urls::url_view> u =
            urls::parse_uri(p.value);
        return u.has_value();
    }))
        return urls::grammar::error::invalid;

    // all other fields are optional
    // magnet link is OK
    return m;
}

constexpr magnet_link_rule_t magnet_link_rule{};

/** Return a parsed magnet link from a string, or error.

    This is a more convenient user-facing function
    to parse magnet links.
*/
boost::system::result< magnet_link_view >
parse_magnet_link( core::string_view s ) noexcept
{
    return urls::grammar::parse(s, magnet_link_rule);
}

int example_main(int argc, char** argv)
{
    // This example shows how to use custom parsing
    // to process alternate URI schemes, in this
    // case "magnet"
    if (argc != 2) {
        ex_out << argv[0] << "\n";
        ex_out << "magnet <link>\n"
                     "example: magnet magnet:?xt=urn:btih:d2474e86c95b19b8bcfdb92bc12c9d44667cfa36"
                                            "&dn=Leaves+of+Grass+by+Walt+Whitman.epub"
                                            "&tr=udp%3A%2F%2Ftracker.example4.com%3A80"
                                            "&tr=udp%3A%2F%2Ftracker.example5.com%3A80"
                                            "&tr=udp%3A%2F%2Ftracker.example3.com%3A6969"
                                            "&tr=udp%3A%2F%2Ftracker.example2.com%3A80"
                                            "&tr=udp%3A%2F%2Ftracker.example1.com%3A1337\n";
        return EXIT_FAILURE;
    }

    boost::system::result<magnet_link_view> r =
        parse_magnet_link(argv[1]);
    if (!r)
        return EXIT_FAILURE;

    magnet_link_view m = *r;
    ex_out << "link: " << m << "\n";

    auto xt = m.exact_topics();
    for (auto h : xt)
        ex_out << "topic: " << h << "\n";

    auto hs = m.info_hashes();
    for (auto h : hs)
        ex_out << "hash: " << h << "\n";

    auto ps = m.protocols();
    for (auto p : ps)
        ex_out << "protocol: " << p << "\n";

    auto tr = m.address_trackers();
    for (auto h : tr)
        ex_out << "tracker: " << h << "\n";

    auto xs = m.exact_sources();
    for (auto x : xs)
        ex_out << "exact source: " << x << "\n";

    auto as = m.acceptable_sources();
    for (auto a : as)
        ex_out << "topic: " << a << "\n";

    auto mt = m.manifest_topics();
    for (auto a : mt)
        ex_out << "manifest topic: " << a << "\n";

    auto ws = m.web_seed();
    for (auto a : ws)
        ex_out << "web seed: " << a << "\n";

    auto kt = m.keyword_topic();
    if (kt)
        ex_out << "keyword topic: " << *kt << "\n";

    auto dn = m.display_name();
    if (dn)
        ex_out << "display name: " << *dn << "\n";

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: magnet") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back(std::string("magnet:?xt=urn:btih:d2474e86c95b19b8bcfdb92bc12c9d44667cfa36&dn=Leaves+of+Grass+by+Walt+Whitman.epub&tr=udp%3A%2F%2Ftracker.example4.com%3A80&tr=udp%3A%2F%2Ftracker.example1.com%3A1337"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(link: magnet:?xt=urn:btih:d2474e86c95b19b8bcfdb92bc12c9d44667cfa36&dn=Leaves+of+Grass+by+Walt+Whitman.epub&tr=udp%3A%2F%2Ftracker.example4.com%3A80&tr=udp%3A%2F%2Ftracker.example1.com%3A1337
topic: urn:btih:d2474e86c95b19b8bcfdb92bc12c9d44667cfa36
hash: d2474e86c95b19b8bcfdb92bc12c9d44667cfa36
protocol: btih
tracker: udp://tracker.example4.com:80
tracker: udp://tracker.example1.com:1337
display name: Leaves+of+Grass+by+Walt+Whitman.epub
)ex"));
}
}  // namespace ex_magnet

// libs/url/example/qrcode/qrcode.cpp
namespace ex_qrcode {
namespace urls = boost::urls;
namespace core = boost::core;

int example_main(int argc, char** argv)
{
    if (argc < 2) {
        ex_out << argv[0] << "\n";
        ex_out << "Usage: qrcode <data> <width> <height> <output encoding> <error correction> <border>\n"
                     "options:\n"
                     "    <data>:             The data to encode (required)\n"
                     "    <width>:            Image width (default: 100)\n"
                     "    <height>:           Image height (default: width)\n"
                     "    <output encoding>:  UTF-8, Shift_JIS, ISO-8859-1 (default: utf8)\n"
                     "    <error correction>: percentage of error correction (default: 7)\n"
                     "    <margin>:           border width (default: 4)\n"
                     "examples:\n"
                     "qrcode \"Hello world\"\n";
        return EXIT_FAILURE;
    }

    urls::url u =
        urls::parse_uri(
            "https://chart.googleapis.com/chart?cht=qr").value();
    auto ps = u.params();

    // Data
    ps.append({"chl", argv[1]});

    // Size
    std::size_t width = argc < 3 ? 100 : std::stoll(argv[2]);
    std::size_t height = argc < 4 ? width : std::stoll(argv[3]);
    ps.append({"chs", std::to_string(width) + "x" + std::to_string(height)});

    // Encoding
    if (argc >= 5)
    {
         core::string_view output_encoding =
            core::string_view(argv[3]) == "Shift_JIS" ||
            core::string_view(argv[3]) == "ISO-8859-1" ?
                argv[4] : "UTF-8";
         ps.append({"choe", output_encoding});
    }

    // Error
    if (argc >= 6)
    {
        std::size_t err = std::stoll(argv[5]);
        std::string chld;
        if (err < 11)
            chld = "L";
        else if (err < 20)
            chld = "M";
        else if (err < 27)
            chld = "Q";
        else
            chld = "H";
        std::size_t margin = argc < 7 ? 4 : std::stoll(argv[6]);
        chld += "|";
        chld += std::to_string(margin);
        ps.append({"chld", chld});
    }

    ex_out << u << '\n';

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: qrcode") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back(std::string("Hello world"));
    ex_args.push_back(std::string("200"));
    ex_args.push_back(std::string("200"));
    ex_args.push_back(std::string("UTF-8"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(https://chart.googleapis.com/chart?cht=qr&chl=Hello+world&chs=200x200&choe=UTF-8
)ex"));
}
}  // namespace ex_qrcode

// libs/url/example/finicky/finicky.cpp
namespace ex_finicky {
namespace urls = boost::urls;
namespace json = boost::json;
namespace core = boost::core;

json::value
read_json( std::istream& is, boost::system::error_code& ec )
{
    json::parse_options opt;
    opt.allow_comments = true;
    json::stream_parser p(json::storage_ptr(), opt);
    std::string line;
    while( std::getline( is, line ) )
    {
        p.write( line, ec );
        if( ec )
            return nullptr;
    }
    p.finish( ec );
    if( ec )
        return nullptr;
    return p.release();
}

bool
glob_match(
    core::string_view pattern,
    core::string_view str)
{
    // regex
    if (str.starts_with("/") &&
        str.ends_with("/"))
    {
        const boost::regex pr(pattern.begin() + 1, pattern.end() - 1);
        return boost::regex_match(std::string(str), pr);
    }

    // literal
    if (!pattern.contains('*'))
    {
        return pattern == str;
    }

    // glob
    std::string p = pattern;
    std::size_t i = p.find('*');
    while (i != std::string::npos)
    {
        auto e = std::min(p.find_first_not_of('*', i), p.size());
        std::size_t n = e - i;
        if (n == 1)
        {
            p.replace(i, e, "[^/]*");
            i += 5;
        }
        else
        {
            p.replace(i, e, ".*");
            i += 2;
        }
        i = p.find('*', i);
    }
    const boost::regex pr(p);
    return boost::regex_match(std::string(str), pr);
}

bool
url_match(
    json::value& mv,
    urls::url const& u)
{
    if (mv.is_string())
    {
        json::string& p = mv.as_string();
        return glob_match(u.buffer(), p);
    }
    else if (mv.is_array())
    {
        json::array& m = mv.as_array();
        for (auto& mi: m)
        {
            if (!mi.is_string())
                throw std::invalid_argument(
                    "handle match is not a string");
            if (glob_match(mi.as_string(), u.buffer()))
                return true;
        }
    }
    else if (mv.is_object())
    {
        json::object& m = mv.as_object();
        std::pair<core::string_view, core::string_view>
            field_values[] = {
                {"protocol",  u.scheme()},
                {"authority", u.encoded_authority()},
                {"username",  u.encoded_user()},
                {"user",      u.encoded_user()},
                {"password",  u.encoded_password()},
                {"userinfo",  u.encoded_userinfo()},
                {"host",      u.encoded_host()},
                {"port",      u.port()},
                {"path",      u.encoded_path()},
                {"pathname",  u.encoded_path()},
                {"query",     u.encoded_query()},
                {"search",    u.encoded_query()},
                {"fragment",  u.encoded_fragment()},
                {"hash",      u.encoded_fragment()},
            };
        for (auto& p: field_values)
        {
            auto it = m.find(p.first);
            if (it != m.end())
            {
                if (!it->value().is_string())
                    throw std::invalid_argument(
                        "match fields should be a strings");
                if (glob_match(p.second, p.first))
                    return true;
            }
        }
    }
    return false;
}

#define EX_CHECK(c, msg)             \
    if (!(c))                     \
    {                             \
        ex_out << msg << "\n"; \
        return EXIT_FAILURE;      \
    }

int example_main(int argc, char** argv)
{
    if (argc < 3) {
        ex_out << argv[0] << "\n";
        ex_out << "Usage: finicky <config> <url>\n"
                     "options:\n"
                     "    <config>: Configuration file\n"
                     "    <url>:    The url to open\n"
                     "examples:\n"
                     "    finicky config.json \"http://www.example.com\"\n";
        return EXIT_FAILURE;
    }

    // Parse url
    boost::system::result<urls::url> ru = urls::parse_uri(argv[2]);
    EX_CHECK(ru, "Invalid URL");
    urls::url u = *ru;

    // Open config file
    std::fstream fin(argv[1]);
    EX_CHECK(fin.good(), "Cannot open configuration file");
    boost::system::error_code ec;
    json::value c = read_json(fin, ec);
    EX_CHECK(!ec.failed(), "Cannot parse configuration file");
    EX_CHECK(c.is_object(), "Configuration file is not an object");
    json::object& o = c.as_object();

    // Set initial browser
    auto bit = o.find("defaultBrowser");
    EX_CHECK(
        bit != o.end(),
        "Configuration file has no defaultBrowser");
    EX_CHECK(
        bit->value().is_string(),
        "defaultBrowser should be a string");
    json::string& browser = bit->value().as_string();

    // Apply rewrites to the input string
    auto rsit = o.find("rewrite");
    if (rsit != o.end())
    {
        EX_CHECK(
            rsit->value().is_array(),
            "rewrite rules should be an array");
        auto& rs = rsit->value().as_array();
        for (auto& rv: rs)
        {
            EX_CHECK(
                rv.is_object(),
                "individual rewrite rule should be an object");
            json::object& r = rv.as_object();

            // Look for match
            auto mit = r.find("match");
            EX_CHECK(
                mit != r.end(),
                "rewrite rule should have a match field");
            EX_CHECK(
                mit->value().is_object() || mit->value().is_string(),
                "rewrite match field is not an object");
            if (!url_match(mit->value(), u))
                continue;

            // Apply replacement rule
            auto uit = r.find("url");
            EX_CHECK(
                uit != r.end(),
                "rewrite rule should have a url field");
            EX_CHECK(
                uit->value().is_object() ||
                uit->value().is_string(),
                "url field must be an object or string");

            if (uit->value().is_string())
            {
                json::string& uo = uit->value().as_string();
                auto ru1 = urls::parse_uri(uo);
                EX_CHECK(ru1, "url " << uo.c_str() << " is invalid");
                u = *ru;
            }
            else
            {
                json::object& uo = uit->value().as_object();
                auto it = uo.find("protocol");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "protocol field should be a string");
                    u.set_scheme(it->value().as_string());
                }

                it = uo.find("authority");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "authority field should be a string");
                    u.set_encoded_authority(
                        it->value().as_string().subview());
                }

                it = uo.find("username");
                if (it == uo.end())
                    it = uo.find("user");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "username field should be a string");
                    u.set_encoded_user(
                        it->value().as_string().subview());
                }

                it = uo.find("password");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "password field should be a string");
                    u.set_encoded_password(
                        it->value().as_string().subview());
                }

                it = uo.find("userinfo");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "userinfo field should be a string");
                    u.set_encoded_userinfo(
                        it->value().as_string().subview());
                }

                it = uo.find("host");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "host field should be a string");
                    u.set_encoded_host(
                        it->value().as_string().subview());
                }

                it = uo.find("port");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "port field should be a string");
                    u.set_port(
                        it->value().as_string().subview());
                }

                it = uo.find("path");
                if (it == uo.end())
                    it = uo.find("pathname");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "path field should be a string");
                    u.set_encoded_path(
                        it->value().as_string().subview());
                }

                it = uo.find("query");
                if (it == uo.end())
                    it = uo.find("search");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "query field should be a string");
                    u.set_encoded_query(
                        it->value().as_string().subview());
                }

                it = uo.find("fragment");
                if (it == uo.end())
                    it = uo.find("hash");
                if (it != uo.end())
                {
                    EX_CHECK(
                        it->value().is_string(),
                        "fragment field should be a string");
                    u.set_encoded_fragment(
                        it->value().as_string().subview());
                }
            }
        }
    }

    // Determine which browser should handle the url
    auto hsit = o.find("handlers");
    if (hsit != o.end())
    {
        EX_CHECK(
            hsit->value().is_array(),
            "handler rules should be an array");
        auto& hs = hsit->value().as_array();
        for (auto& hv: hs)
        {
            EX_CHECK(
                hv.is_object(),
                "individual handlers should be an object");
            json::object& h = hv.as_object();

            auto mit = h.find("match");
            EX_CHECK(
                mit != h.end(),
                "handle rule should have a match field");
            EX_CHECK(
                mit->value().is_string() || mit->value().is_array(),
                "handle match field must be an array or a string");

            auto hbit = h.find("browser");
            EX_CHECK(
                hbit != h.end(),
                "handle rule should have a browser field");
            EX_CHECK(
                hbit->value().is_string(),
                "browser field is not a string");

            // Look for match and change browser
            if (url_match(mit->value(), u))
            {
                browser = hbit->value().as_string().subview();
                break;
            }
        }
    }

    // Print command finicky would run
    ex_out << "\"" << browser.c_str() << "\" " << u << '\n';

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: finicky") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-url-finicky.json"); f << R"cfg({
  "defaultBrowser": "Google Chrome",
  "rewrite": [
    {
      "match": {
        "protocol": "http"
      },
      "url": {
        "protocol": "https"
      }
    },
    {
      "match": "**/*example.org/*",
      "url": "http://example.com"
    }
  ],
  "handlers": [
    {
      "match": [
        "**/*apple.com/*",
        "**/*example.org/*",
        "**/*example.com**"
      ],
      "browser": "Safari"
    },
    {
      "match": "/workplace/",
      "browser": "Firefox"
    },
    {
      "match": [
        "**/*google.com/*",
        "**/*.google.com/*"
      ],
      "browser": "Google Chrome"
    }
  ]
})cfg"; }
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-url-finicky.json").string());
    ex_args.push_back(std::string("http://www.example.org/index.html"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex("Safari" http://www.example.org/index.html
)ex"));
}
}  // namespace ex_finicky

// libs/url/example/sanitize/sanitize.cpp
namespace ex_sanitize {
namespace urls = boost::urls;
namespace core = boost::core;

struct url_components
{
    core::string_view scheme;
    core::string_view user;
    core::string_view password;
    core::string_view hostname;
    core::string_view port;
    core::string_view path;
    core::string_view query;
    core::string_view fragment;
};

core::string_view
port_of_scheme(core::string_view scheme_str) {
    static std::array<std::pair<core::string_view, core::string_view>, 21> scheme_ports =
    {{
         {"http", "80"},
         {"ftp", "21"},
         {"https", "443"},
         {"gopher", "70"},
         {"ldap", "389"},
         {"nntp", "119"},
         {"snews", "563"},
         {"imap", "143"},
         {"pop", "110"},
         {"sip", "5060"},
         {"rtsp", "554"},
         {"wais", "210"},
         {"z39.50r", "210"},
         {"z39.50s", "210"},
         {"prospero", "191"},
         {"nfs", "2049"},
         {"tip", "3372"},
         {"acap", "674"},
         {"telnet", "23"},
         {"ssh", "22"},
         {"", "65535"}
    }};

    auto iequals = [](core::string_view a, core::string_view b)
    {
        if (b.size() != a.size()) {
            return false;
        }
        for (unsigned int i = 0; i < a.size(); ++i) {
            if (std::tolower(a[i]) != std::tolower(b[i])) {
                return false;
            }
        }
        return true;
    };

    auto const& it = std::find_if(
        scheme_ports.begin(),
        scheme_ports.end(),
        [&](std::pair<core::string_view, core::string_view> const& s) {
        return iequals(s.first, scheme_str);
    });

    if (it != scheme_ports.end()) {
        return it->second;
    } else {
        return {};
    }
}

void
extract_relative_ref(
    core::string_view hostinfo_relative,
    url_components &out)
{
    // split path and query#fragment
    constexpr urls::grammar::lut_chars path_end_chars("?#\0");
    auto it = urls::grammar::find_if(
        hostinfo_relative.begin(),
        hostinfo_relative.end(), path_end_chars);
    core::string_view query_and_frag = hostinfo_relative.substr(it - hostinfo_relative.begin());
    if (query_and_frag != hostinfo_relative)
        out.path = hostinfo_relative.substr(
            0, query_and_frag.data() - hostinfo_relative.data());
    if (query_and_frag.empty())
        return;

    // ?query#fragment
    if (query_and_frag.front() == '?') {
        query_and_frag = query_and_frag.substr(1);
        core::string_view::size_type hash_pos = query_and_frag.find('#');
        if (hash_pos != core::string_view::npos) {
            core::string_view fragment_part = query_and_frag.substr(hash_pos);
            out.fragment = fragment_part.substr(1);
            out.query = query_and_frag.substr(
                0, fragment_part.data() - query_and_frag.data());
        } else {
            out.query = query_and_frag;
        }
        return;
    }

    // fragment
    out.fragment = query_and_frag.substr(1);
}

void
extract_userinfo_relative(
    core::string_view relative_ref,
    core::string_view userinfo_relative,
    core::string_view host_info,
    url_components& out) {
    // We expect userinfo_relative to point to the first character of
    // the hostname.  If there's a port it is the first colon,
    // except with IPv6.
    auto host_end_pos = host_info.find(':');
    if (host_end_pos == core::string_view::npos)
    {
        // definitely no port
        out.hostname = userinfo_relative.substr(
            0, relative_ref.data() - userinfo_relative.data());
        return extract_relative_ref(relative_ref, out);
    }

    // extract hostname and port
    out.hostname = userinfo_relative.substr(0, host_end_pos);
    core::string_view host_relative = userinfo_relative.substr(host_end_pos + 1);
    out.port = host_relative.substr(0, relative_ref.data() - host_relative.data());

    // validate port
    bool const valid_port =
        urls::grammar::find_if_not(
            out.port.begin(),
            out.port.end(),
            urls::grammar::digit_chars)
        == out.port.end();
    if (!valid_port)
    {
        // move port to hostname where it can be encoded
        out.hostname = {out.hostname.begin(), out.port.end()};
        out.port = {};
    }

    extract_relative_ref(relative_ref, out);
    if (out.port.empty() && !out.scheme.empty())
        out.port = port_of_scheme(out.scheme);
}

void
extract_scheme_relative(
    core::string_view scheme_relative,
    url_components &out)
{
    // hostinfo
    constexpr urls::grammar::lut_chars hostinfo_end_chars("/?#\0");
    auto it = urls::grammar::find_if(
        scheme_relative.begin(),
        scheme_relative.end(),
        hostinfo_end_chars);
    auto path_offset = (std::min)(
        scheme_relative.size(),
        static_cast<std::size_t>(it - scheme_relative.begin()));
    core::string_view host_info = scheme_relative.substr(0, path_offset);

    // userinfo
    core::string_view relative_ref = scheme_relative.substr(path_offset);
    auto host_offset = host_info.find_last_of('@');
    if (host_offset == core::string_view::npos)
        return extract_userinfo_relative(
            relative_ref,
            scheme_relative,
            host_info,
            out);

    // password
    core::string_view userinfo_at_relative = scheme_relative.substr(host_offset);
    core::string_view userinfo(host_info.data(), userinfo_at_relative.data() - host_info.data());
    auto password_offset = std::min(userinfo.size(), userinfo.find(':'));
    if (password_offset != userinfo.size()) {
        out.user = scheme_relative.substr(0, password_offset);
        core::string_view password = scheme_relative.substr(password_offset + 1);
        out.password = password.substr(0, userinfo_at_relative.data() - password.data());
    } else {
        out.user = scheme_relative.substr(0, userinfo_at_relative.data() - scheme_relative.data());
    }

    // userinfo-relative
    core::string_view userinfo_relative = userinfo_at_relative.substr(1);
    it = urls::grammar::find_if(
        userinfo_relative.begin(),
        userinfo_relative.end(),
        hostinfo_end_chars);
    path_offset = (std::min)(
        userinfo_relative.size(),
        static_cast<std::size_t>(it - userinfo_relative.begin()));
    host_info = userinfo_relative.substr(0, path_offset);
    extract_userinfo_relative(
        relative_ref,
        userinfo_relative,
        host_info,
        out);
}

void
extract_uri_components(
   core::string_view s,
   url_components &out)
{
    if (s.starts_with("//") && !s.starts_with("///"))
    {
        return extract_scheme_relative(s.substr(2), out);
    }

    if (s.starts_with('/'))
    {
        return extract_relative_ref(s, out);
    }

    // extract scheme
    // first char in a scheme must be letter (we accept uppercase here)
    bool has_scheme = false;
    if (!s.empty() && urls::grammar::alpha_chars(s.front())) {
        constexpr
            urls::grammar::lut_chars scheme_chars(
                "0123456789+-.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        char const* it = urls::grammar::find_if_not(
            s.begin() + 1, s.end(), scheme_chars);
        size_t scheme_size = (std::min)(
            s.size(), static_cast<std::size_t>(it - s.begin()));
        // scheme must be non-empty and followed by ':'
        if (s.size() > scheme_size && s[scheme_size] == ':') {
            out.scheme = s.substr(0, scheme_size);
            has_scheme = true;
        }
    }

    // The usual route, parse scheme first
    core::string_view scheme_relative = s;
    if (has_scheme)
    {
        scheme_relative = s.substr(out.scheme.size() + 1);
    }

    const bool has_authority = scheme_relative.starts_with("//");
    if (has_authority)
    {
        scheme_relative = scheme_relative.substr(2);
        return extract_scheme_relative(scheme_relative, out);
    }

    const bool is_relative_ref = !has_scheme && !has_authority;
    if (is_relative_ref)
    {
        // this is the trick browsers usually apply when 1) there's no
        // authority because the "//" is missing, 2) the scheme is also missing,
        // and 3) the first path segment looks like an authority
        //
        // This behavior is widespread, although it's ambiguous because valid
        // host characters are also valid path characters.
        //
        // It's this rule that allows for things like "www.boost.org" in the
        // browser. This is an invalid URL because it has no "//" to indicate
        // this is the authority and "www.boost.org" is a perfectly valid
        // path segment.
        auto first_seg_offset = (std::min)(s.size(), s.find_first_of('/'));
        core::string_view first_seg = s.substr(0, first_seg_offset);
        auto host_delimiter_pos = first_seg.find_first_of(".:");
        bool const looks_like_authority =
            urls::parse_authority(first_seg) &&
            host_delimiter_pos != core::string_view::npos &&
            host_delimiter_pos != first_seg.size() - 1;
        if (looks_like_authority)
            return extract_scheme_relative(s, out);

        // if the first_seg is really a seg, parse as relative ref
        return extract_relative_ref(s, out);
    }

    // all that's left is a relative path
    return extract_relative_ref(scheme_relative, out);
}

void
sanitize_uri(core::string_view s, urls::url_base& dest) {
    url_components o;
    dest.clear();
    extract_uri_components(s, o);
    if (o.scheme.data())
        dest.set_scheme(o.scheme);
    if (o.user.data())
        dest.set_encoded_user(o.user);
    if (o.password.data())
        dest.set_encoded_password(o.password);
    if (o.hostname.data())
        dest.set_encoded_host(o.hostname);
    if (o.port.data())
        dest.set_port(o.port);
    if (o.path.data())
        dest.set_encoded_path(o.path);
    if (o.query.data())
        dest.set_encoded_query(o.query);
    if (o.fragment.data())
        dest.set_encoded_fragment(o.fragment);
}

urls::url
sanitize_uri(core::string_view s) {
    urls::url u;
    sanitize_uri(s, u);
    return u;
}

void
print_url_components(urls::url_view u)
{
    ex_out << "url: " << u.buffer() << '\n';
    if (u.has_scheme())
        ex_out << "scheme: " << u.scheme() << '\n';
    if (u.has_userinfo())
        ex_out << "user: " << u.encoded_user() << '\n';
    if (u.has_password())
        ex_out << "password: " << u.encoded_password() << '\n';
    if (u.has_authority())
        ex_out << "hostname: " << u.encoded_host() << '\n';
    if (u.has_port())
        ex_out << "port: " << u.port() << '\n';
    ex_out << "path: " << u.encoded_path() << '\n';
    ex_out << "segments:\n";
    for (auto seg: u.encoded_segments())
        ex_out << "- " << seg << '\n';
    if (u.has_query())
        ex_out << "query: " << u.encoded_query() << '\n';
    ex_out << "params:\n";
    for (auto param: u.encoded_params())
    {
        if (param.has_value)
            ex_out << "- " << param.key << ": " << param.value << '\n';
        else
            ex_out << "- " << param.key << '\n';
    }
    if (u.has_fragment())
        ex_out << "fragment: " << u.encoded_fragment() << '\n';
}

int example_main(int argc, char **argv)
{
    if (argc != 2)
    {
        core::string_view exec = argv[0];
        auto p = exec.find_last_of("/\\");
        if (p != core::string_view::npos)
            exec = exec.substr(p);
        ex_out
            << "Usage: " << exec
            << " <url>\n"
               "target: a non-strict url\n";
        return EXIT_FAILURE;
    }

    core::string_view uri_str = argv[1];

    boost::system::result<urls::url_view> ru = urls::parse_uri_reference(uri_str);
    if (ru)
    {
        urls::url_view u = *ru;
        if (u.has_scheme() && u.has_fragment())
            ex_out << "Input is a valid URL\n";
        else if (u.has_scheme())
            ex_out << "Input is a valid absolute URL\n";
        else
            ex_out << "Input is a valid relative URL\n";
        print_url_components(u);
        return EXIT_SUCCESS;
    }

    ex_out << "Sanitizing URL:\n";
    ex_out << "input: " << uri_str << '\n';
    urls::url u = sanitize_uri(uri_str);
    print_url_components(u);
    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: sanitize") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back(std::string("http://www.example.com/a/./b/../c?q=%20x#frag"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Input is a valid URL
url: http://www.example.com/a/./b/../c?q=%20x#frag
scheme: http
hostname: www.example.com
path: /a/./b/../c
segments:
- a
- .
- b
- ..
- c
query: q=%20x
params:
- q: %20x
fragment: frag
)ex"));
}
}  // namespace ex_sanitize

// libs/url/example/suffix_list/suffix_list.cpp
namespace ex_suffix_list {
namespace urls = boost::urls;

int example_main(int argc, char** argv)
{
    if (argc < 2) {
        ex_out << argv[0] << "\n";
        ex_out << "Usage: suffix_list <url> <suffix_list>\n"
                     "options:\n"
                     "    <url>:              A valid url (required)\n"
                     "    <suffix_list>:      File with the public suffix list (default: public_suffix_list.dat)\n"
                     "examples:\n"
                     "suffix_list \"www.example.com\" \"public_suffix_list.dat\"\n";
        return EXIT_FAILURE;
    }

    urls::url u = urls::parse_uri(argv[1]).value();

    std::string filename =
        argc < 3 ?
            "public_suffix_list.dat" :
            argv[2];
    std::ifstream fin(filename);
    if (fin.bad())
    {
        ex_out << "Cannot open " << filename << "\n";
        return EXIT_FAILURE;
    }

    std::string comment;
    std::string suffix;
    std::string line;
    while (std::getline(fin, line))
    {
        boost::core::string_view sv(line);
        if (sv.starts_with("//") && suffix.empty())
        {
            comment.append(sv.substr(3));
            comment.append("\n");
        }
        else if (line.empty())
        {
            if (suffix.empty())
                comment.clear();
            else
                break;
        }
        else if (u.encoded_host().ends_with(line))
        {
            suffix = line;
        }
    }

    ex_out <<
        "url:    \n" << u                << "\n\n"
        "host:   \n" << u.encoded_host() << "\n\n"
        "suffix: \n" << suffix           << "\n\n"
        "detail: \n" << comment          << "\n\n";

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: suffix_list") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-url-suffixes.dat"); f << "// a subset of the public suffix list\ncom\norg\nuk\nco.uk\n*.ck\n!www.ck\n"; }
    ex_args.push_back(std::string("https://www.bbc.co.uk/news"));
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-url-suffixes.dat").string());
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(url:    
https://www.bbc.co.uk/news

host:   
www.bbc.co.uk

suffix: 
co.uk

detail: 
a subset of the public suffix list


)ex"));
}
}  // namespace ex_suffix_list
