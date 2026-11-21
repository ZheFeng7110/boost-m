// mcpplibs.mwrap.boost.any — the documentation examples of Boost.Any
// (libs/any/doc/any.qbk, sections "Examples" and "C++20 module") as test cases.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.any;

using boost::any_cast;
using many = std::list<boost::any>;

namespace {
void append_int(many& values, int value) {
    boost::any to_append = value;
    values.push_back(to_append);
}
void append_string(many& values, const std::string& value) { values.push_back(value); }
void append_char_ptr(many& values, const char* value) { values.push_back(value); }
void append_any(many& values, const boost::any& value) { values.push_back(value); }
void append_nothing(many& values) { values.push_back(boost::any()); }

bool is_empty(const boost::any& operand) { return operand.empty(); }
bool is_int(const boost::any& operand) { return operand.type() == typeid(int); }
bool is_char_ptr(const boost::any& operand) {
    try {
        any_cast<const char*>(operand);
        return true;
    } catch (const boost::bad_any_cast&) {
        return false;
    }
}
bool is_string(const boost::any& operand) { return any_cast<std::string>(&operand); }

void count_all(many& values, std::ostream& out) {
    out << "#empty == " << std::count_if(values.begin(), values.end(), is_empty) << '\n';
    out << "#int == " << std::count_if(values.begin(), values.end(), is_int) << '\n';
    out << "#const char * == " << std::count_if(values.begin(), values.end(), is_char_ptr) << '\n';
    out << "#string == " << std::count_if(values.begin(), values.end(), is_string) << '\n';
}

struct property {
    property() = default;
    property(const std::string& n, const boost::any& v) : name(n), value(v) {}
    std::string name;
    boost::any value;
};
using properties = std::list<property>;

class consumer {
public:
    virtual ~consumer() = default;
    virtual void notify(const boost::any&) = 0;
};
class recorder : public consumer {
public:
    void notify(const boost::any& a) override { seen.push_back(a.type().name()); }
    std::vector<std::string> seen;
};
}  // namespace

TEST_CASE("doc: examples") {
    many values;
    append_int(values, 1);
    append_string(values, "two");
    append_char_ptr(values, "three");
    append_any(values, boost::any(4.0));
    append_nothing(values);
    std::ostringstream out;
    count_all(values, out);
    CHECK(out.str() == "#empty == 1\n#int == 1\n#const char * == 1\n#string == 1\n");

    properties props{{"width", 10}, {"title", std::string("box")}};
    CHECK(any_cast<int>(props.front().value) == 10);
    CHECK(any_cast<std::string>(props.back().value) == "box");

    recorder r;
    consumer& c = r;
    c.notify(42);
    c.notify(std::string("s"));
    CHECK(r.seen.size() == 2);
    CHECK(r.seen[0] == typeid(int).name());

    // unique_any carrying optional user data from an authorizer to a handler.
    struct user_info { std::string login; };
    auto authorizer = [](const std::map<std::string, std::string>& headers,
                         boost::anys::unique_any& user_data) {
        auto it = headers.find("user");
        if (it == headers.end()) return false;
        user_data = user_info{it->second};
        return true;
    };
    boost::anys::unique_any user_data;
    REQUIRE(authorizer({{"user", "alice"}}, user_data));
    CHECK(any_cast<user_info&>(user_data).login == "alice");
    boost::anys::unique_any none;
    CHECK_FALSE(authorizer({}, none));
    CHECK_FALSE(none.has_value());
}

TEST_CASE("doc: c-20-module") {
    // libs/any/modules/usage_sample.cpp, against this package's module.
    boost::any a = 42;
    CHECK(any_cast<int>(a) == 42);
}
