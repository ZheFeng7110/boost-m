// mcpplibs.mwrap.boost.conversion — the documentation examples of
// Boost.Conversion (libs/conversion/doc/conversion.qbk) through the module
// that the 2026-11-09 hardening added (design §5.3).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.conversion;
import mcpplibs.mwrap.boost.smart_ptr;

namespace {
class Fruit {
public:
    virtual ~Fruit() = default;
    // intrusive_ptr support
    friend void intrusive_ptr_add_ref(Fruit* f) { ++f->refs; }
    friend void intrusive_ptr_release(Fruit* f) {
        if (--f->refs == 0) delete f;
    }
    int refs = 0;
};
class Banana : public Fruit {
public:
    int curve = 3;
};

template <class FruitPtr>
int f(FruitPtr fruit) {
    // ... logic which leads us to believe it is a banana
    auto banana = boost::polymorphic_pointer_downcast<Banana>(fruit);
    return banana->curve;
}
}  // namespace

TEST_CASE("doc: polymorphic-downcast") {
    Banana b;
    Fruit* fruit = &b;
    Banana* banana = boost::polymorphic_downcast<Banana*>(fruit);
    CHECK(banana == &b);
    Banana& ref = boost::polymorphic_downcast<Banana&>(*fruit);
    CHECK(&ref == &b);
}

TEST_CASE("doc: polymorphic-pointer-cast") {
    Banana raw;
    CHECK(f(static_cast<Fruit*>(&raw)) == 3);
    CHECK(f(std::shared_ptr<Fruit>(std::make_shared<Banana>())) == 3);
    CHECK(f(boost::shared_ptr<Fruit>(boost::make_shared<Banana>())) == 3);
    CHECK(f(boost::intrusive_ptr<Fruit>(new Banana)) == 3);
}
