// mcpplibs.mwrap.boost.macros smoke — the mapped half of the macro surface.
//
// A named module cannot export preprocessing macros; the object-like constants
// keep their value, type and evaluation semantics as `inline constexpr` objects
// in `mcpplibs::mwrap::boost::macros`, under the original macro spelling.
// This translation unit does NOT include <boost/version.hpp>, so the mapped
// names are reachable as ordinary qualified constants.
#include "test_assert.hpp"
import std;
import mcpplibs.mwrap.boost.macros;

int main() {
    namespace macros = mcpplibs::mwrap::boost::macros;

    static_assert(macros::BOOST_VERSION == 109200);
    static_assert(macros::BOOST_VERSION == 100000 + 92 * 100);
    assert(macros::BOOST_VERSION / 100 % 1000 == 92);

    static_assert(sizeof(macros::BOOST_VERSION) == sizeof(int));
    constexpr int version = macros::BOOST_VERSION;
    assert(version == 109200);

    assert(std::string_view(macros::BOOST_LIB_VERSION) == "1_92");
    assert(std::string_view(macros::BOOST_LIB_VERSION).size() == 4);

    // The type is preserved, not stringified into a generic scalar.
    static_assert(std::is_same_v<decltype(macros::BOOST_VERSION), const int>);
    static_assert(std::is_same_v<std::remove_cv_t<decltype(macros::BOOST_LIB_VERSION)>,
                                 const char*>);
    return 0;
}
