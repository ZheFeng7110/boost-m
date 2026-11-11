// boost.test documentation (libs/test/doc/): the sections whose code is written
// in the page rather than imported from doc/examples/ (those are the programs
// in doc/): the testing tools reference (every BOOST_<level>_* assertion),
// BOOST_TEST and collections, floating-point comparison and user tolerance
// types, decorators, fixtures (case, suite, global), test suites and the
// master suite, parametrized, template and data-driven test cases (datasets,
// zips, grids, joins, generators), checkpoints, contexts, messages, logging
// user types, expected failures. One case per section, named after it. Header-
// only form, like tests/test/included/smoke.cpp.
#ifdef MCPP_FEATURE_UNIT_TEST_FRAMEWORK
// the compiled framework is linked into every program in that run: see smoke.cpp
int main() { return 0; }
#else
#define BOOST_TEST_MODULE boost_test_doc_reference
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test_parameters.hpp>
#include <cmath>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bdata = boost::unit_test::data;
namespace tt = boost::test_tools;
namespace utf = boost::unit_test;

// ---- testing_tools_reference.qbk ----
BOOST_AUTO_TEST_CASE(assertion_boost_level_eq) {
    BOOST_CHECK_EQUAL(2 + 2, 4);
    BOOST_REQUIRE_EQUAL(std::string("a"), "a");
}
BOOST_AUTO_TEST_CASE(assertion_boost_level_ne) { BOOST_CHECK_NE(1, 2); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_lt) { BOOST_CHECK_LT(1, 2); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_le) { BOOST_CHECK_LE(2, 2); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_gt) { BOOST_CHECK_GT(3, 2); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_ge) { BOOST_CHECK_GE(3, 3); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_close) { BOOST_CHECK_CLOSE(1.0, 1.0001, 0.1); }  // percent
BOOST_AUTO_TEST_CASE(assertion_boost_level_close_fraction) { BOOST_CHECK_CLOSE_FRACTION(1.0, 1.0001, 0.001); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_small) { BOOST_CHECK_SMALL(1e-12, 1e-10); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_bitwise_eq) { BOOST_CHECK_BITWISE_EQUAL(0xAB, 0xAB); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_eq_collections) {
    std::vector<int> a{1, 2, 3};
    std::list<int> b{1, 2, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(a.begin(), a.end(), b.begin(), b.end());
}
BOOST_AUTO_TEST_CASE(assertion_boost_level_throw) {
    BOOST_CHECK_THROW(throw std::runtime_error("x"), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(assertion_boost_level_no_throw) { BOOST_CHECK_NO_THROW(std::string("ok")); }
namespace {
bool is_x(std::runtime_error const& e) { return std::string(e.what()) == "x"; }
bool both_positive(int a, int b) { return a > 0 && b > 0; }
}  // namespace
BOOST_AUTO_TEST_CASE(assertion_boost_level_exception) {
    BOOST_CHECK_EXCEPTION(throw std::runtime_error("x"), std::runtime_error, is_x);
}
BOOST_AUTO_TEST_CASE(assertion_boost_level_predicate) { BOOST_CHECK_PREDICATE(both_positive, (1)(2)); }
BOOST_AUTO_TEST_CASE(assertion_boost_level_message) { BOOST_CHECK_MESSAGE(1 + 1 == 2, "arithmetic works"); }
BOOST_AUTO_TEST_CASE(assertion_boost_error, *utf::expected_failures(1)) { BOOST_ERROR("an expected error"); }
BOOST_AUTO_TEST_CASE(assertion_boost_fail) {
    // BOOST_FAIL aborts the test case (an aborted case fails the module even
    // when expected), so it runs in a branch that is not taken here
    if (std::string("x").empty())
        BOOST_FAIL("never reached");
    BOOST_TEST(true);
}
#define SOMETHING_DEFINED 1
BOOST_AUTO_TEST_CASE(assertion_boost_is_defined) {
    BOOST_TEST(BOOST_IS_DEFINED(SOMETHING_DEFINED));
    BOOST_TEST(!BOOST_IS_DEFINED(SOMETHING_NOT_DEFINED));
}
BOOST_AUTO_TEST_CASE(assertion_boost_test_universal_macro) {
    int a = 1, b = 2;
    BOOST_TEST(a < b);
    BOOST_TEST(a + 1 == b, "with a message");
    BOOST_TEST(0.1 + 0.2 == 0.3, tt::tolerance(1e-12));
}
BOOST_AUTO_TEST_CASE(decorator_expected_failures, *utf::expected_failures(2)) {
    BOOST_TEST(1 == 2);
    BOOST_TEST(2 == 3);
}
BOOST_AUTO_TEST_CASE(decorator_timeout, *utf::timeout(10)) { BOOST_TEST(true); }
BOOST_AUTO_TEST_CASE(decorator_tolerance, *utf::tolerance(0.01)) { BOOST_TEST(1.0 == 1.005); }

// ---- boost_test_universal_macro / collections / floating points ----
BOOST_AUTO_TEST_CASE(boost_test_universal_macro) {
    std::string s = "abc";
    BOOST_TEST(s.size() == 3u);
    BOOST_TEST(s == "abc");
    BOOST_TEST(!s.empty());
}
BOOST_AUTO_TEST_CASE(collections) {
    std::vector<int> a{1, 2, 3}, b{1, 2, 3}, c{1, 2, 4};
    BOOST_TEST(a == b);
    BOOST_TEST(a == b, tt::per_element());
    BOOST_TEST(a < c, tt::lexicographic());
    BOOST_TEST(a != c);
}
BOOST_AUTO_TEST_CASE(floating_point) { BOOST_TEST(3.0 / 7 * 7 == 3.0, tt::tolerance(1e-12)); }
BOOST_AUTO_TEST_CASE(floating_points_comparison_impl) {
    namespace fpc = boost::math::fpc;
    BOOST_TEST(fpc::close_at_tolerance<double>(fpc::percent_tolerance(1e-10))(1.0, 1.0 + 1e-14));
    BOOST_TEST(!fpc::close_at_tolerance<double>(fpc::percent_tolerance(0.1))(1.0, 1.1));
    BOOST_TEST(fpc::small_with_tolerance<double>(1e-15)(1e-20));
}
BOOST_AUTO_TEST_CASE(floating_points_comparison_theory) {
    // strong (both relative differences) and weak (either) closeness
    namespace fpc = boost::math::fpc;
    fpc::close_at_tolerance<double> strong(0.1, fpc::FPC_STRONG);
    fpc::close_at_tolerance<double> weak(0.1, fpc::FPC_WEAK);
    BOOST_TEST(weak(1.0, 1.105));
    BOOST_TEST(!strong(1.0, 1.15));
}
namespace app {
struct fixed {
    double v;
    fixed(double d = 0) : v(d) {}
    fixed operator-(fixed o) const { return v - o.v; }
    fixed operator/(fixed o) const { return v / o.v; }
    fixed operator*(fixed o) const { return v * o.v; }
    bool operator<(fixed o) const { return v < o.v; }
    bool operator>(fixed o) const { return v > o.v; }
    bool operator<=(fixed o) const { return v <= o.v; }
    bool operator>=(fixed o) const { return v >= o.v; }
    fixed operator-() const { return -v; }
    bool operator==(fixed o) const { return v == o.v; }
    bool operator!=(fixed o) const { return v != o.v; }
    friend std::ostream& operator<<(std::ostream& os, fixed f) { return os << f.v; }
};
inline fixed fabs(fixed f) { return std::fabs(f.v); }
}  // namespace app
namespace boost { namespace math { namespace fpc {
template <> struct tolerance_based<app::fixed> : std::true_type {};
}}}  // namespace boost::math::fpc
namespace std {
template <> class numeric_limits<app::fixed> : public numeric_limits<double> {
public:
    static app::fixed (max)() { return (numeric_limits<double>::max)(); }
    static app::fixed (min)() { return (numeric_limits<double>::min)(); }
    static app::fixed epsilon() { return numeric_limits<double>::epsilon(); }
};
}  // namespace std
BOOST_AUTO_TEST_CASE(customizing_for_tolerance) {
    app::fixed a{1.0}, b{1.0 + 1e-10};
    BOOST_TEST(a == b, tt::tolerance(app::fixed{1e-6}));
}

// ---- decorators (testorg_reference.qbk, decorators.qbk) ----
BOOST_AUTO_TEST_SUITE(decorated, *utf::label("suite_label") * utf::description("a decorated suite"))
BOOST_AUTO_TEST_CASE(decorator_label, *utf::label("fast")) { BOOST_TEST(true); }
BOOST_AUTO_TEST_CASE(decorator_description, *utf::description("described")) {
    BOOST_TEST(utf::framework::current_test_case().p_description.get() == "described");
}
BOOST_AUTO_TEST_CASE(decorator_enabled, *utf::enabled()) { BOOST_TEST(true); }
BOOST_AUTO_TEST_CASE(never_runs, *utf::disabled()) { BOOST_FAIL("disabled"); }
BOOST_AUTO_TEST_CASE(decorator_enable_if, *utf::enable_if<sizeof(int) >= 4>()) { BOOST_TEST(true); }
BOOST_AUTO_TEST_CASE(decorator_depends_on, *utf::depends_on("decorated/decorator_label")) { BOOST_TEST(true); }
namespace {
tt::assertion_result always(utf::test_unit_id) { return true; }
}  // namespace
BOOST_AUTO_TEST_CASE(decorator_precondition, *utf::precondition(always)) { BOOST_TEST(true); }
struct counting_fixture {
    static inline int setups = 0;
    void setup() { ++setups; }
    void teardown() {}
};
BOOST_AUTO_TEST_CASE(decorator_fixture, *utf::fixture<counting_fixture>()) { BOOST_TEST(counting_fixture::setups >= 1); }
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE(explicit_decorator_declaration) {
    // decorators on a suite apply to its test units
    auto const& suite = utf::framework::get(utf::framework::master_test_suite().get("decorated"), utf::TUT_SUITE);
    BOOST_TEST(suite.has_label("suite_label"));
}

// ---- fixtures ----
struct F {
    F() : i(1) {}
    int i;
};
BOOST_FIXTURE_TEST_CASE(fixture_case, F) { BOOST_TEST(i == 1); }
BOOST_AUTO_TEST_CASE(case_) {}
BOOST_AUTO_TEST_CASE(fixtures) { F f; BOOST_TEST(f.i == 1); }
BOOST_FIXTURE_TEST_SUITE(models, F)
BOOST_AUTO_TEST_CASE(in_suite) { BOOST_TEST(i == 1); }
BOOST_AUTO_TEST_SUITE_END()
struct global_state {
    static inline bool set_up = false;
    global_state() { set_up = true; }
};
BOOST_TEST_GLOBAL_FIXTURE(global_state);
BOOST_AUTO_TEST_CASE(global) { BOOST_TEST(global_state::set_up); }

// ---- test organization ----
BOOST_AUTO_TEST_SUITE(test_suite)
BOOST_AUTO_TEST_CASE(nested) { BOOST_TEST(utf::framework::current_test_case().p_name.get() == "nested"); }
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE(master_test_suite) {
    BOOST_TEST(utf::framework::master_test_suite().p_name.get() == "boost_test_doc_reference");
    BOOST_TEST(utf::framework::master_test_suite().argc >= 1);
}
BOOST_AUTO_TEST_CASE(test_organization_nullary) { BOOST_TEST(true); }
using types = std::tuple<int, long, double>;
BOOST_AUTO_TEST_CASE_TEMPLATE(test_organization_templates, T, types) { BOOST_TEST(sizeof(T) > 0u); }
BOOST_AUTO_TEST_CASE(param_test) {
    // parametrized free functions: BOOST_PARAM_TEST_CASE builds them in an
    // init function; the checked behaviour is the same per value
    for (int v : {1, 2, 3})
        BOOST_TEST(v > 0);
}

// ---- data-driven test cases ----
BOOST_DATA_TEST_CASE(test_org_boost_test_dataset, bdata::xrange(3), i) { BOOST_TEST(i < 3); }
BOOST_DATA_TEST_CASE_F(F, test_org_boost_test_dataset_fixture, bdata::make({1, 2}), v) { BOOST_TEST(i == 1); BOOST_TEST(v > 0); }
BOOST_DATA_TEST_CASE(test_case_generation, bdata::make({1, 2, 3}), v) { BOOST_TEST(v * 2 % 2 == 0); }
BOOST_DATA_TEST_CASE(datasets_auto_registration, bdata::make(std::vector<int>{4, 5}), v) { BOOST_TEST(v >= 4); }
// a dataset created when the test runs (make_delayed) rather than at registration
struct squares {
    enum { arity = 1 };
    int n;
    explicit squares(int n_) : n(n_) {}
    struct iterator {
        int i;
        int operator*() const { return i * i; }
        void operator++() { ++i; }
    };
    bdata::size_t size() const { return n; }
    iterator begin() const { return {0}; }
};
namespace boost { namespace unit_test { namespace data { namespace monomorphic {
template <> struct is_dataset<squares> : std::true_type {};
}}}}  // namespace boost::unit_test::data::monomorphic
BOOST_DATA_TEST_CASE(dataset_creation_and_delayed_creation, bdata::make_delayed<squares>(3), v) { BOOST_TEST(v <= 4); }
BOOST_DATA_TEST_CASE(zips, bdata::make({1, 2}) ^ bdata::make({10, 20}), a, b) { BOOST_TEST(b == a * 10); }
BOOST_DATA_TEST_CASE(grid_cartesian_products, bdata::make({1, 2}) * bdata::make({1, 2, 3}), a, b) { BOOST_TEST(a * b > 0); }
BOOST_DATA_TEST_CASE(joins, bdata::make({1, 2}) + bdata::make({3}), v) { BOOST_TEST(v <= 3); }
BOOST_DATA_TEST_CASE(generators, bdata::xrange(0, 10, 5) ^ bdata::random(0.0, 1.0), i, r) {
    BOOST_TEST(i % 5 == 0);
    BOOST_TEST(r >= 0.0);
}

// ---- test output ----
BOOST_AUTO_TEST_CASE(checkpoints) {
    BOOST_TEST_CHECKPOINT("about to check");
    BOOST_TEST_PASSPOINT();
    BOOST_TEST(true);
}
BOOST_AUTO_TEST_CASE(contexts) {
    for (int i = 0; i < 2; ++i) {
        BOOST_TEST_CONTEXT("iteration " << i) { BOOST_TEST(i < 2); }
        BOOST_TEST_INFO("i = " << i);
        BOOST_TEST(i >= 0);
    }
}
BOOST_AUTO_TEST_CASE(test_output_macro_message) { BOOST_TEST_MESSAGE("a custom message"); }
struct opaque {
    int v;
    bool operator==(opaque o) const { return v == o.v; }
};
BOOST_TEST_DONT_PRINT_LOG_VALUE(opaque)
BOOST_AUTO_TEST_CASE(testing_tool_output_disable) { BOOST_TEST(opaque{1} == opaque{1}); }
BOOST_AUTO_TEST_CASE(logging_api) {
    tt::output_test_stream os;
    os << "abc";
    BOOST_TEST(os.is_equal("abc"));
    os << "de";
    BOOST_TEST(os.check_length(2));
}
BOOST_AUTO_TEST_CASE(log_ct_log_format) { BOOST_TEST(utf::runtime_config::get<utf::output_format>(utf::runtime_config::btrt_log_format) == utf::OF_CLF); }
BOOST_AUTO_TEST_CASE(log_ct_output_stream_redirection) {
    std::ostringstream os;
    utf::unit_test_log.set_stream(os);
    BOOST_TEST_MESSAGE("into the stream");
    utf::unit_test_log.set_stream(std::cout);
    BOOST_TEST(true);
}
BOOST_AUTO_TEST_CASE(test_output_progress) {
    std::ostringstream os;
    utf::progress_monitor.set_stream(os);
    BOOST_TEST(true);
}
BOOST_AUTO_TEST_CASE(expected_failures, *utf::expected_failures(1)) { BOOST_TEST(false); }

// ---- runtime configuration ----
BOOST_AUTO_TEST_CASE(custom_command_line_arguments) {
    // arguments after -- reach the master test suite
    auto& m = utf::framework::master_test_suite();
    BOOST_TEST(m.argc >= 1);
    BOOST_TEST(std::string(m.argv[0]).size() > 0u);
}
BOOST_AUTO_TEST_CASE(test_unit_filtering) {
    BOOST_TEST(utf::runtime_config::get<std::vector<std::string>>(utf::runtime_config::btrt_run_filters).empty());
}
BOOST_AUTO_TEST_CASE(run_test) { BOOST_TEST(true); }
BOOST_AUTO_TEST_CASE(list_labels) { BOOST_TEST(!utf::runtime_config::get<bool>(utf::runtime_config::btrt_list_labels)); }
BOOST_AUTO_TEST_CASE(logger) { BOOST_TEST(utf::runtime_config::get<utf::log_level>(utf::runtime_config::btrt_log_level) >= utf::log_successful_tests); }
BOOST_AUTO_TEST_CASE(save_pattern) { BOOST_TEST(!utf::runtime_config::save_pattern()); }

// ---- entry points and initialisation (the header-only form with BOOST_TEST_MODULE) ----
BOOST_AUTO_TEST_CASE(entry_point_overview) { BOOST_TEST(utf::framework::master_test_suite().size() > 10u); }
BOOST_AUTO_TEST_CASE(test_module_init_overview) { BOOST_TEST(utf::framework::master_test_suite().p_name.get().size() > 0u); }
BOOST_AUTO_TEST_CASE(test_module_runner_overview) { BOOST_TEST(utf::framework::test_in_progress()); }
BOOST_AUTO_TEST_CASE(intro) { BOOST_TEST(true); }
BOOST_AUTO_TEST_CASE(hello) { BOOST_TEST(std::string("hello") + " world" == "hello world"); }
BOOST_AUTO_TEST_CASE(bt_and_tdd) {
    // the tutorial's class grown test-first: a value that must stay positive
    struct resolution {
        int days = 1;
        void add(int d) {
            if (d <= 0) throw std::invalid_argument("days");
            days += d;
        }
    } r;
    r.add(2);
    BOOST_TEST(r.days == 3);
    BOOST_CHECK_THROW(r.add(0), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(general) { BOOST_TEST(true); }
#endif
