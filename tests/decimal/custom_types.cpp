// mcpplibs.mwrap.boost.decimal — user code over the decimal types: generic
// user templates constrained on the library's concept/traits
// (is_decimal_floating_point), a user money type built on decimal64_t with its
// own rounding and streaming, decimals as keys of user containers (std::hash,
// ordering), user conversion between the user type and strings via
// <charconv>, and the numeric_limits a user algorithm relies on.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.decimal;

namespace dec = boost::decimal;

namespace user {
template <class T>
    requires dec::detail::is_decimal_floating_point_v<T>
T mean(const std::vector<T>& v) {
    T sum{0};
    for (const auto& x : v) sum += x;
    return sum / T(static_cast<int>(v.size()));
}

class money {
public:
    money() = default;
    explicit money(std::string_view s) : v_(s) {}
    money operator+(money o) const { return money(v_ + o.v_); }
    money operator*(int n) const { return money(v_ * n); }
    money rounded() const { return money(dec::quantize(v_, dec::decimal64_t(1u, -2))); }
    bool operator==(const money&) const = default;
    friend std::ostream& operator<<(std::ostream& os, const money& m) { return os << m.v_; }
    std::string str() const {
        char buf[64];
        auto r = dec::to_chars(buf, buf + sizeof buf, v_, dec::chars_format::fixed, 2);
        return std::string(buf, r.ptr);
    }
private:
    explicit money(dec::decimal64_t v) : v_(v) {}
    dec::decimal64_t v_{0};
};

template <class T>
T epsilon_scaled(T x) {
    return x * std::numeric_limits<T>::epsilon();
}
}  // namespace user

TEST_CASE_TEMPLATE("generic user code over every decimal type", T, dec::decimal32_t, dec::decimal64_t,
                   dec::decimal128_t, dec::decimal_fast32_t, dec::decimal_fast64_t, dec::decimal_fast128_t) {
    std::vector<T> v{T(1), T(2), T(4)};
    CHECK(user::mean(v) == T(7) / T(3));
    static_assert(dec::detail::is_decimal_floating_point_v<T>);
    static_assert(!dec::detail::is_decimal_floating_point_v<double>);
    CHECK(user::epsilon_scaled(T(1)) == std::numeric_limits<T>::epsilon());
    CHECK(std::numeric_limits<T>::digits10 >= 7);
}

TEST_CASE("a user money type: exact decimal cents") {
    user::money a("0.10"), b("0.20");
    CHECK((a + b) == user::money("0.30"));  // exact, unlike binary floating point
    CHECK((a * 3).rounded().str() == "0.30");
    user::money price("19.999");
    CHECK(price.rounded().str() == "20.00");
    std::ostringstream os;
    os << user::money("1.5");
    CHECK(os.str() == "1.5");
}

TEST_CASE("decimals as keys of user containers") {
    std::unordered_map<dec::decimal64_t, std::string> by_price;
    by_price[dec::decimal64_t("1.10")] = "a";
    by_price[dec::decimal64_t("1.1")] = "b";  // same value, same key
    CHECK(by_price.size() == 1);
    std::map<dec::decimal32_t, int> ordered{{dec::decimal32_t(3), 3}, {dec::decimal32_t(1), 1}};
    CHECK(ordered.begin()->second == 1);
}
