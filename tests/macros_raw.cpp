// The raw header + mapped constant interaction.
//
// <mwrap/boost/macros.hpp> is the documented include entry: it re-includes the
// upstream headers byte-for-byte, so the raw macros stay defined in this
// translation unit. Two consequences are pinned here, because both surprise:
//
//   1. `BOOST_VERSION` expands normally (the supported raw-header use);
//   2. an active macro expands even inside a qualified name, so
//      `mcpplibs::mwrap::boost::macros::BOOST_VERSION` becomes `...::109200`
//      in THIS translation unit. The mapped constant is for a translation unit
//      that does not include the raw macro, or one that #undef's it first —
//      which is the second half of this test.
//
// The .macros module itself #undef's both names in its own unit, so the library
// never depends on this hazard.
#include "test_assert.hpp"

#include <mwrap/boost/macros.hpp>

#include <string_view>

import mcpplibs.mwrap.boost.macros;

#define MWRAP_STRINGIZE_IMPL(x) #x
#define MWRAP_STRINGIZE(x) MWRAP_STRINGIZE_IMPL(x)

int main() {
    // 1. Raw macro expansion, multiple evaluation of an argument.
    static_assert(BOOST_VERSION == 109200);
    int evaluations = 0;
    const int recorded = (++evaluations, BOOST_VERSION);
    assert(recorded == 109200);
    assert(evaluations == 1);

    // BOOST_ASSERT is an assertion macro, not a function: with NDEBUG it is a
    // no-op, so only the checked build asserts its side effect count.
#ifndef NDEBUG
    int asserted = 0;
    BOOST_ASSERT(++asserted == 1);
    assert(asserted == 1);
#endif

    // 2. The macro wins inside the qualified name — stringified proof. The
    //    token sequence after expansion is `...::109200`, so a qualified use in
    //    THIS translation unit would not name the mapped constant at all.
    static_assert(std::string_view(MWRAP_STRINGIZE(
                      mcpplibs::mwrap::boost::macros::BOOST_VERSION)) ==
                  "mcpplibs::mwrap::boost::macros::109200");

    // 3. After recording the value, #undef the raw names and use the mapped
    //    constants from the module as intended.
    constexpr int raw_version = BOOST_VERSION;
    constexpr std::string_view raw_lib_version = BOOST_LIB_VERSION;
#undef BOOST_VERSION
#undef BOOST_LIB_VERSION
    static_assert(raw_version == 109200);
    static_assert(raw_lib_version == "1_92");
    static_assert(mcpplibs::mwrap::boost::macros::BOOST_VERSION == 109200);
    assert(std::string_view(mcpplibs::mwrap::boost::macros::BOOST_LIB_VERSION) ==
           "1_92");
    return 0;
}
