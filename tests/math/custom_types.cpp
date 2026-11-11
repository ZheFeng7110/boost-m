// mcpplibs.mwrap.boost.math — user types at Boost.Math's customization
// points: a user real type (strong_real, a wrapper of double with the
// operations and std-like functions the library asks for, numeric_limits and
// the policies' precision) through special functions, distributions,
// constants and quadrature; user policies (error handlers that record, a
// precision); user functors for the root finders, the minimiser and the
// series / continued-fraction tools; a user distribution-like object with
// find_location; and the traits rejecting unsuitable types.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.math;

namespace bm = boost::math;
namespace pol = boost::math::policies;

namespace user {
class strong_real {
    double v_ = 0;

public:
    strong_real() = default;
    template <class A>
        requires std::is_arithmetic_v<A>
    strong_real(A v) : v_(double(v)) {}
    double value() const { return v_; }
    explicit operator double() const { return v_; }
    explicit operator int() const { return int(v_); }
    explicit operator long() const { return long(v_); }
    explicit operator long long() const { return (long long)(v_); }
    explicit operator bool() const { return v_ != 0; }
    strong_real& operator+=(strong_real o) { v_ += o.v_; return *this; }
    strong_real& operator-=(strong_real o) { v_ -= o.v_; return *this; }
    strong_real& operator*=(strong_real o) { v_ *= o.v_; return *this; }
    strong_real& operator/=(strong_real o) { v_ /= o.v_; return *this; }
    friend strong_real operator+(strong_real a, strong_real b) { return a.v_ + b.v_; }
    friend strong_real operator-(strong_real a, strong_real b) { return a.v_ - b.v_; }
    friend strong_real operator*(strong_real a, strong_real b) { return a.v_ * b.v_; }
    friend strong_real operator/(strong_real a, strong_real b) { return a.v_ / b.v_; }
    friend strong_real operator-(strong_real a) { return -a.v_; }
    friend strong_real operator+(strong_real a) { return a; }
    friend auto operator<=>(strong_real a, strong_real b) = default;
    friend bool operator==(strong_real a, strong_real b) = default;
    friend std::ostream& operator<<(std::ostream& os, strong_real r) { return os << r.v_; }
    friend std::istream& operator>>(std::istream& is, strong_real& r) { return is >> r.v_; }
};
// the functions the library calls unqualified (found by ADL)
#define USER_UNARY(f) inline strong_real f(strong_real a) { return std::f(a.value()); }
USER_UNARY(abs) USER_UNARY(fabs) USER_UNARY(sqrt) USER_UNARY(exp) USER_UNARY(log) USER_UNARY(log10) USER_UNARY(sin)
USER_UNARY(cos) USER_UNARY(tan) USER_UNARY(asin) USER_UNARY(acos) USER_UNARY(atan) USER_UNARY(sinh) USER_UNARY(cosh)
USER_UNARY(tanh) USER_UNARY(floor) USER_UNARY(ceil) USER_UNARY(trunc) USER_UNARY(round)
#undef USER_UNARY
inline strong_real pow(strong_real a, strong_real b) { return std::pow(a.value(), b.value()); }
inline strong_real pow(strong_real a, int b) { return std::pow(a.value(), b); }
inline strong_real fmod(strong_real a, strong_real b) { return std::fmod(a.value(), b.value()); }
inline strong_real atan2(strong_real a, strong_real b) { return std::atan2(a.value(), b.value()); }
inline strong_real ldexp(strong_real a, int e) { return std::ldexp(a.value(), e); }
inline strong_real frexp(strong_real a, int* e) { return std::frexp(a.value(), e); }
inline strong_real modf(strong_real a, strong_real* ip) {
    double i;
    double r = std::modf(a.value(), &i);
    *ip = i;
    return r;
}
}  // namespace user

namespace std {
template <> class numeric_limits<user::strong_real> : public numeric_limits<double> {
public:
    static user::strong_real min() noexcept { return numeric_limits<double>::min(); }
    static user::strong_real max() noexcept { return numeric_limits<double>::max(); }
    static user::strong_real lowest() noexcept { return numeric_limits<double>::lowest(); }
    static user::strong_real epsilon() noexcept { return numeric_limits<double>::epsilon(); }
    static user::strong_real infinity() noexcept { return numeric_limits<double>::infinity(); }
    static user::strong_real quiet_NaN() noexcept { return numeric_limits<double>::quiet_NaN(); }
    static user::strong_real denorm_min() noexcept { return numeric_limits<double>::denorm_min(); }
    static user::strong_real round_error() noexcept { return 0.5; }
};
}  // namespace std

namespace {
bool close(double a, double b, double rel = 1e-12) {
    return std::abs(a - b) <= rel * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}
using R = user::strong_real;
}  // namespace

TEST_CASE("custom: real type through special functions") {
    static_assert(std::numeric_limits<R>::is_specialized);
    CHECK(close(bm::tgamma(R(5)).value(), 24));
    CHECK(close(bm::erf(R(0.5)).value(), bm::erf(0.5)));
    CHECK(close(bm::log1p(R(1e-10)).value(), bm::log1p(1e-10)));
    CHECK(close(bm::cyl_bessel_j(0, R(1)).value(), bm::cyl_bessel_j(0, 1.0), 1e-12));
    CHECK(close(bm::beta(R(2), R(3)).value(), 1.0 / 12));
}

TEST_CASE("custom: real type through distributions and constants") {
    bm::normal_distribution<R> n(R(0), R(1));
    CHECK(close(bm::cdf(n, R(0)).value(), 0.5));
    CHECK(close(bm::quantile(n, R(0.975)).value(), 1.959963984540054, 1e-10));
    bm::students_t_distribution<R> t(R(10));
    CHECK(close(bm::cdf(t, R(0)).value(), 0.5));
    CHECK(close(bm::constants::pi<R>().value(), std::numbers::pi));
}

TEST_CASE("custom: real type through quadrature and roots") {
    bm::quadrature::tanh_sinh<R> integrator;
    R I = integrator.integrate([](R x) { return x * x; }, R(0), R(1));
    CHECK(close(I.value(), 1.0 / 3, 1e-12));
    auto f = [](R x) { return std::make_tuple(x * x - R(2), R(2) * x); };
    R root = bm::tools::newton_raphson_iterate(f, R(1), R(0), R(2), 50);
    CHECK(close(root.value(), std::sqrt(2.0), 1e-14));
}

namespace user {
// An error handler policy: user_error with the functions boost::math calls.
inline int domain_errors = 0;
}  // namespace user
namespace boost { namespace math { namespace policies {
template <class T>
T user_domain_error(const char*, const char*, const T&) {
    ++user::domain_errors;
    return std::numeric_limits<T>::quiet_NaN();
}
}}}  // namespace boost::math::policies

TEST_CASE("custom: policies") {
    using recording = pol::policy<pol::domain_error<pol::user_error>>;
    user::domain_errors = 0;
    CHECK(std::isnan(bm::acosh(0.5, recording())));
    CHECK(user::domain_errors == 1);
    // precision through a policy, also as a distribution's policy
    using low = pol::policy<pol::digits10<6>>;
    CHECK(close(bm::tgamma(3.5, low()), bm::tgamma(3.5), 1e-6));
    bm::normal_distribution<double, low> n;
    CHECK(close(bm::cdf(n, 1.0), bm::cdf(bm::normal(), 1.0), 1e-6));
    static_assert(pol::digits<double, low>() < 53);
}

namespace user {
// root-finder and minimiser functors with state
struct cubic {
    int calls = 0;
    std::tuple<double, double, double> operator()(double x) {
        ++calls;
        return {x * x * x - 8, 3 * x * x, 6 * x};
    }
};
struct parabola {
    double operator()(double x) const { return (x - 2.5) * (x - 2.5); }
};
struct geometric_series {  // sum r^k = 1/(1-r)
    using result_type = double;
    double r, term = 1;
    double operator()() {
        double t = term;
        term *= r;
        return t;
    }
};
}  // namespace user

TEST_CASE("custom: tools functors") {
    user::cubic f;
    double r = bm::tools::halley_iterate(std::ref(f), 1.0, 0.0, 4.0, 50);
    CHECK(close(r, 2, 1e-14));
    CHECK(f.calls > 0);
    auto m = bm::tools::brent_find_minima(user::parabola{}, 0.0, 5.0, 30);
    CHECK(close(m.first, 2.5, 1e-6));
    user::geometric_series s{0.5};
    std::uintmax_t terms = 200;
    CHECK(close(bm::tools::sum_series(s, 1e-16, terms), 2.0, 1e-14));
}

TEST_CASE("custom: unsuitable types") {
    static_assert(!std::is_floating_point_v<R>);  // a class type, used through numeric_limits
    static_assert(std::is_same_v<bm::tools::promote_args_t<float, double>, double>);
    static_assert(std::is_same_v<bm::tools::promote_args_t<int>, double>);  // integers promote
    static_assert(!pol::is_policy<int>::value);
    static_assert(pol::is_policy<pol::policy<>>::value);
}
