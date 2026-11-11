// boost.test: user types at the framework's customization points — a type
// printed through boost_test_print_type (and one through operator<<), a user
// dataset for BOOST_DATA_TEST_CASE, user fixtures with setup/teardown (case,
// suite and global), a user precondition, a user log formatter installed for a
// run of assertions, and floating-point tolerance for a user type. Header-only
// form, like tests/test/included/smoke.cpp.
#ifdef MCPP_FEATURE_UNIT_TEST_FRAMEWORK
// the compiled framework is linked into every program in that run: see
// tests/test/included/smoke.cpp
int main() { return 0; }
#else
#define BOOST_TEST_MODULE boost_test_custom_types
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <cmath>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace bdata = boost::unit_test::data;
namespace tt = boost::test_tools;
namespace utf = boost::unit_test;

namespace app {
struct point {
    int x, y;
    bool operator==(point const& o) const { return x == o.x && y == o.y; }
    bool operator!=(point const& o) const { return !(*this == o); }
};
// the framework's printing customization point, found by ADL
inline std::ostream& boost_test_print_type(std::ostream& os, point const& p) { return os << "point(" << p.x << ", " << p.y << ")"; }

struct tag {
    std::string name;
    bool operator==(tag const& o) const { return name == o.name; }
    friend std::ostream& operator<<(std::ostream& os, tag const& t) { return os << "#" << t.name; }
};

// a dataset of the first n odd numbers
struct odd_numbers {
    enum { arity = 1 };
    int n;
    explicit odd_numbers(int count) : n(count) {}
    struct iterator {
        int i;
        int operator*() const { return 2 * i + 1; }
        void operator++() { ++i; }
    };
    bdata::size_t size() const { return n; }
    iterator begin() const { return {0}; }
};
}  // namespace app
namespace boost { namespace unit_test { namespace data { namespace monomorphic {
template <> struct is_dataset<app::odd_numbers> : std::true_type {};
}}}}  // namespace boost::unit_test::data::monomorphic

BOOST_AUTO_TEST_CASE(print_type) {
    tt::output_test_stream os;
    tt::tt_detail::print_log_value<app::point>()(os, app::point{1, 2});
    BOOST_TEST(os.is_equal("point(1, 2)"));
    tt::tt_detail::print_log_value<app::tag>()(os, app::tag{"t"});
    BOOST_TEST(os.is_equal("#t"));
    BOOST_TEST((app::point{1, 2} == app::point{1, 2}));
}

BOOST_DATA_TEST_CASE(user_dataset, app::odd_numbers(4), v) { BOOST_TEST(v % 2 == 1); }
BOOST_DATA_TEST_CASE(user_dataset_zipped, app::odd_numbers(3) ^ bdata::xrange(3), odd, i) { BOOST_TEST(odd == 2 * i + 1); }

namespace app {
struct counted {
    static inline int setups = 0, teardowns = 0;
    counted() = default;
    void setup() { ++setups; }
    void teardown() { ++teardowns; }
    int value = 7;
};
struct suite_state {
    static inline int constructed = 0;
    suite_state() { ++constructed; }
};
struct global_state {
    static inline bool up = false;
    global_state() { up = true; }
    ~global_state() { up = false; }
};
}  // namespace app
using user_global_state = app::global_state;
BOOST_TEST_GLOBAL_FIXTURE(user_global_state);

BOOST_FIXTURE_TEST_CASE(case_fixture, app::counted) {
    BOOST_TEST(value == 7);
    BOOST_TEST(app::counted::setups >= 1);
}
BOOST_AUTO_TEST_SUITE(with_suite_fixture, *utf::fixture<app::suite_state>())
BOOST_AUTO_TEST_CASE(first) { BOOST_TEST(app::suite_state::constructed == 1); }
BOOST_AUTO_TEST_CASE(second) { BOOST_TEST(app::suite_state::constructed == 1); }  // once per suite
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE(global_fixture) { BOOST_TEST(app::global_state::up); }

namespace app {
tt::assertion_result global_is_up(utf::test_unit_id) {
    tt::assertion_result r(global_state::up);
    r.message() << "the global fixture is not up";
    return r;
}
}  // namespace app
BOOST_AUTO_TEST_CASE(user_precondition, *utf::precondition(app::global_is_up)) { BOOST_TEST(true); }

namespace app {
// a log formatter that counts what it is told
struct counting_formatter : utf::output::compiler_log_formatter {
    static inline int assertions = 0;
    void log_entry_start(std::ostream& os, utf::log_entry_data const& d, log_entry_types t) override {
        ++assertions;
        compiler_log_formatter::log_entry_start(os, d, t);
    }
};
}  // namespace app
BOOST_AUTO_TEST_CASE(user_log_formatter) {
    std::ostringstream os;
    utf::unit_test_log.set_stream(os);
    utf::unit_test_log.set_threshold_level(utf::log_successful_tests);
    utf::unit_test_log.set_formatter(new app::counting_formatter);
    BOOST_TEST(1 + 1 == 2);
    BOOST_TEST_MESSAGE("a message");
    utf::unit_test_log.set_formatter(new utf::output::compiler_log_formatter);
    utf::unit_test_log.set_threshold_level(utf::log_all_errors);
    utf::unit_test_log.set_stream(std::cout);
    BOOST_TEST(app::counting_formatter::assertions >= 2);
    BOOST_TEST(os.str().find("a message") != std::string::npos);
}

namespace app {
struct meters {
    double v;
    meters(double d = 0) : v(d) {}
    meters operator-(meters o) const { return v - o.v; }
    meters operator-() const { return -v; }
    meters operator/(meters o) const { return v / o.v; }
    meters operator*(meters o) const { return v * o.v; }
    bool operator<(meters o) const { return v < o.v; }
    bool operator>(meters o) const { return v > o.v; }
    bool operator<=(meters o) const { return v <= o.v; }
    bool operator>=(meters o) const { return v >= o.v; }
    bool operator==(meters o) const { return v == o.v; }
    bool operator!=(meters o) const { return v != o.v; }
    friend std::ostream& operator<<(std::ostream& os, meters m) { return os << m.v << " m"; }
};
inline meters fabs(meters m) { return std::fabs(m.v); }
}  // namespace app
namespace boost { namespace math { namespace fpc {
template <> struct tolerance_based<app::meters> : std::true_type {};
}}}  // namespace boost::math::fpc
namespace std {
template <> class numeric_limits<app::meters> : public numeric_limits<double> {
public:
    static app::meters (max)() { return (numeric_limits<double>::max)(); }
    static app::meters (min)() { return (numeric_limits<double>::min)(); }
    static app::meters epsilon() { return numeric_limits<double>::epsilon(); }
};
}  // namespace std
BOOST_AUTO_TEST_CASE(user_tolerance_type, *utf::tolerance(app::meters(1e-6))) {
    BOOST_TEST(app::meters(10.0) == app::meters(10.0 + 1e-9));
}
#endif
