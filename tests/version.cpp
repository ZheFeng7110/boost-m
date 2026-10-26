// mcpplibs.mwrap.boost.version smoke.
//
// The version constants are mapped once, in mcpplibs.mwrap.boost.macros; the
// `version` module is the thin entry that re-exports them (one export owner per
// entity). This test imports only `version`, which proves the re-export, and
// checks the values seeded from the vendored <boost/version.hpp>
// (109200 / "1_92" for Boost 1.92.0).
#include "test_assert.hpp"
import mcpplibs.mwrap.boost.version;

int main() {
    using namespace mcpplibs::mwrap::boost::macros;
    static_assert(BOOST_VERSION == 109200);
    static_assert(BOOST_LIB_VERSION[0] == '1');
    assert(BOOST_VERSION / 100000 == 1);   // major
    assert(BOOST_VERSION / 100 % 1000 == 92);  // minor
    assert(BOOST_VERSION % 100 == 0);      // patch
    assert(BOOST_LIB_VERSION[2] == '9');
    assert(BOOST_LIB_VERSION[3] == '2');
    return 0;
}
