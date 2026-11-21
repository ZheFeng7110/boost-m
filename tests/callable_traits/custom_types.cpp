// mcpplibs.mwrap.boost.callable_traits — user callables: member functions of a
// user class with every qualifier combination, user function objects
// (including generic and overloaded ones), user function pointers, and the
// FAQ's point (libs/callable_traits/doc/callable_traits.qbk#faq): one trait
// replaces the hand-written partial specializations per callable shape.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.callable_traits;

namespace ct = boost::callable_traits;

namespace user {
struct widget {
    int get() const { return v; }
    void set(int x) & { v = x; }
    std::string name() && { return "temp"; }
    double scale(double f) const volatile noexcept { return f; }
    int printf_like(const char*, ...) { return 0; }
    int v = 0;
};

struct adder {
    long operator()(int a, int b) const { return a + b; }
};

struct stateful {
    int calls = 0;
    void operator()(std::string_view) { ++calls; }
};

inline int free_fn(float, char&) noexcept { return 1; }
}  // namespace user

TEST_CASE("member functions of a user class") {
    using get_t = decltype(&user::widget::get);
    static_assert(std::is_same_v<ct::return_type_t<get_t>, int>);
    static_assert(std::is_same_v<ct::class_of_t<get_t>, user::widget>);
    static_assert(std::is_same_v<ct::qualified_class_of_t<get_t>, const user::widget&>);
    static_assert(ct::is_const_member<get_t>::value);
    static_assert(!ct::is_volatile_member<get_t>::value);
    static_assert(std::is_same_v<ct::args_t<get_t>, std::tuple<const user::widget&>>);

    using set_t = decltype(&user::widget::set);
    static_assert(ct::is_lvalue_reference_member<set_t>::value);
    static_assert(ct::has_void_return<set_t>::value);
    static_assert(std::is_same_v<ct::remove_member_reference_t<set_t>, void (user::widget::*)(int)>);

    using name_t = decltype(&user::widget::name);
    static_assert(ct::is_rvalue_reference_member<name_t>::value);
    static_assert(std::is_same_v<ct::qualified_class_of_t<name_t>, user::widget&&>);

    using scale_t = decltype(&user::widget::scale);
    static_assert(ct::is_cv_member<scale_t>::value);
    static_assert(ct::is_noexcept<scale_t>::value);
    static_assert(std::is_same_v<ct::remove_noexcept_t<scale_t>,
                                 double (user::widget::*)(double) const volatile>);
    static_assert(std::is_same_v<ct::remove_member_cv_t<ct::remove_noexcept_t<scale_t>>,
                                 double (user::widget::*)(double)>);

    using var_t = decltype(&user::widget::printf_like);
    static_assert(ct::has_varargs<var_t>::value);
    static_assert(std::is_same_v<ct::remove_varargs_t<var_t>, int (user::widget::*)(const char*)>);

    // Rebuild a member pointer of a user class from a plain function type.
    static_assert(std::is_same_v<ct::apply_member_pointer_t<int(), user::widget>, int (user::widget::*)()>);
    static_assert(std::is_same_v<ct::add_member_const_t<int (user::widget::*)()>,
                                 int (user::widget::*)() const>);
    // And the traits agree with a call through the pointer.
    user::widget w;
    std::invoke(&user::widget::set, w, 4);
    CHECK(std::invoke(&user::widget::get, w) == 4);
    static_assert(ct::is_invocable_r<int, get_t, const user::widget&>::value);
}

TEST_CASE("user function objects and function pointers") {
    static_assert(std::is_same_v<ct::args_t<user::adder>, std::tuple<int, int>>);
    static_assert(std::is_same_v<ct::return_type_t<user::adder>, long>);
    static_assert(std::is_same_v<ct::function_type_t<user::adder>, long(int, int)>);
    static_assert(!ct::is_const_member<user::stateful>::value);
    static_assert(std::is_same_v<ct::args_t<user::stateful>, std::tuple<std::string_view>>);
    static_assert(ct::is_noexcept<decltype(&user::free_fn)>::value);
    static_assert(std::is_same_v<ct::args_t<decltype(&user::free_fn)>, std::tuple<float, char&>>);
    static_assert(std::is_same_v<ct::add_noexcept_t<int (*)(int)>, int (*)(int) noexcept>);
    auto lambda = [n = 2](double d) mutable { return d * n++; };
    static_assert(std::is_same_v<ct::args_t<decltype(lambda)>, std::tuple<double>>);
    static_assert(!ct::is_const_member<decltype(lambda)>::value);
    CHECK(lambda(1.5) == 3.0);
}

namespace faq {
// What the FAQ calls the alternative: one specialization per shape.
template <class T>
struct arity_by_hand;
template <class R, class... A>
struct arity_by_hand<R(A...)> : std::integral_constant<std::size_t, sizeof...(A)> {};
template <class R, class... A>
struct arity_by_hand<R (*)(A...)> : arity_by_hand<R(A...)> {};
template <class R, class C, class... A>
struct arity_by_hand<R (C::*)(A...) const> : arity_by_hand<R(A...)> {};

// With callable_traits, one definition covers every callable shape.
template <class T>
inline constexpr std::size_t arity = std::tuple_size_v<ct::args_t<T>>;
}  // namespace faq

TEST_CASE("doc: faq") {
    static_assert(faq::arity_by_hand<int(int, int)>::value == 2);
    static_assert(faq::arity<int(int, int)> == 2);
    static_assert(faq::arity_by_hand<int (*)(char)>::value == faq::arity<int (*)(char)>);
    // The member pointer form counts the object parameter, as args_t does.
    static_assert(faq::arity<decltype(&user::widget::get)> == 1);
    static_assert(faq::arity<user::adder> == 2);
    static_assert(faq::arity<decltype(&user::widget::scale)> == 2);
    CHECK(faq::arity<void(int, int, int)> == 3);
}
