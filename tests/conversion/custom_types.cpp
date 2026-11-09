// mcpplibs.mwrap.boost.conversion — user types: a user class hierarchy, a user
// smart pointer that opts into polymorphic_pointer_cast through ADL overloads
// of static_pointer_cast / dynamic_pointer_cast (the documented customization
// point), and implicit_cast through user conversion operators.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.conversion;

namespace zoo {
struct animal {
    virtual ~animal() = default;
    virtual std::string sound() const = 0;
};
struct dog : animal {
    std::string sound() const override { return "woof"; }
};
struct cat : animal {
    std::string sound() const override { return "meow"; }
};

// A user handle: a pointer plus a name; convertible to bool, castable by ADL.
template <class T>
class handle {
public:
    handle() = default;
    handle(T* p, std::string name) : p_(p), name_(std::move(name)) {}
    T* operator->() const { return p_; }
    T& operator*() const { return *p_; }
    explicit operator bool() const { return p_ != nullptr; }
    bool operator!() const { return p_ == nullptr; }
    T* get() const { return p_; }
    const std::string& name() const { return name_; }
private:
    T* p_ = nullptr;
    std::string name_;
};

// polymorphic_pointer_downcast asserts dynamic_pointer_cast<Target>(x) == x.
template <class T, class U>
bool operator==(const handle<T>& a, const handle<U>& b) {
    return a.get() == b.get();
}

inline int static_casts = 0, dynamic_casts = 0;

template <class U, class T>
handle<U> static_pointer_cast(const handle<T>& h) {
    ++static_casts;
    return handle<U>(static_cast<U*>(h.get()), h.name());
}
template <class U, class T>
handle<U> dynamic_pointer_cast(const handle<T>& h) {
    ++dynamic_casts;
    return handle<U>(dynamic_cast<U*>(h.get()), h.name());
}

struct celsius {
    double v;
};
struct fahrenheit {
    double v;
    operator celsius() const { return {(v - 32) * 5 / 9}; }  // implicit conversion operator
};
}  // namespace zoo

TEST_CASE("user hierarchy: polymorphic_cast / downcast") {
    zoo::dog d;
    zoo::animal* a = &d;
    CHECK(boost::polymorphic_cast<zoo::dog*>(a)->sound() == "woof");
    CHECK(boost::polymorphic_downcast<zoo::dog*>(a) == &d);
    CHECK_THROWS_AS(boost::polymorphic_cast<zoo::cat*>(a), std::bad_cast);
}

TEST_CASE("user smart pointer through the ADL customization point") {
    zoo::dog d;
    zoo::handle<zoo::animal> h(&d, "rex");
    zoo::static_casts = zoo::dynamic_casts = 0;
    zoo::handle<zoo::dog> dh = boost::polymorphic_pointer_cast<zoo::dog>(h);
    CHECK(dh->sound() == "woof");
    CHECK(dh.name() == "rex");
    CHECK(zoo::dynamic_casts == 1);
    zoo::handle<zoo::dog> dd = boost::polymorphic_pointer_downcast<zoo::dog>(h);
    CHECK(dd.get() == &d);
    CHECK(zoo::static_casts == 1);  // downcast uses static_pointer_cast (+ a debug check)
    CHECK_THROWS_AS(boost::polymorphic_pointer_cast<zoo::cat>(h), std::bad_cast);
}

TEST_CASE("implicit_cast through a user conversion operator") {
    zoo::celsius c = boost::implicit_cast<zoo::celsius>(zoo::fahrenheit{212});
    CHECK(c.v == doctest::Approx(100));
    zoo::dog d;
    const zoo::animal& a = boost::implicit_cast<const zoo::animal&>(d);
    CHECK(a.sound() == "woof");
}
