"""Boost.Bimap: libs/bimap/example/ (examples.cpp, examples_with_boost.cpp,
include_with_boost.cpp). Edited sources: sources/bimap.patch."""
import common as c

D = c.lib_dir("bimap") / "example"
P = "libs/bimap/example/"

EXAMPLES_HEADER = """\
// mcpplibs.mwrap.boost.bimap — the official examples (libs/bimap/example/*.cpp,
// mi_to_b_path/*.cpp) ported mechanically: each program runs as a case and its
// output must equal what the original printed when built from upstream/boost
// alone. Interactive examples read a fixed stdin. population_bimap.cpp,
// mi_hashed_indices.cpp and the bimap_and_boost/ examples are
// examples_with_boost.cpp and include_with_boost.cpp.
"""
WITH_BOOST_HEADER = """\
// mcpplibs.mwrap.boost.bimap — the official examples that combine Boost.Bimap
// with other libraries through their modules (bimap_and_boost/{assign,
// lambda,property_map,range,serialization}.cpp, population_bimap.cpp,
// mi_to_b_path/mi_hashed_indices.cpp), with the same output oracle as
// examples.cpp. Two programs are edited before porting (and the oracle runs
// the edited program): population_bimap spells boost::optional (the unqualified
// name is ambiguous with std::optional since C++17, upstream too) and writes
// its BOOST_FOREACH loops over an iterator pair as range-for over
// boost::make_iterator_range; mi_hashed_indices writes
// BOOST_MULTI_INDEX_MEMBER(T, type, m) as the member<T, type, &T::m> it
// expands to. The foreach, typeof and xpressive examples are about those
// macro libraries: include_with_boost.cpp.
// range.cpp also writes using namespace boost::placeholders (see there), and
// serialization.cpp archives to the temporary directory instead of ./data.
// The range module exports boost::count_if, which range.cpp's own count_if
// meets through `using namespace boost` (the original includes only the range
// headers it needs); the port calls the example's count_if qualified.
"""
INCLUDE_HEADER = """\
// boost.bimap with the macro libraries — pure include: bimap_and_boost/
// {foreach,typeof,xpressive}.cpp exercise BOOST_FOREACH, BOOST_TYPEOF /
// BOOST_AUTO and xpressive actions, which no module can carry, so this TU
// includes everything and imports nothing. Same output oracle as examples.cpp.
"""


def run(group):
    # one list in the byte order of the paths (mi_to_b_path/ before mighty_bimap.cpp)
    files = sorted(list(D.glob("*.cpp")) + list(D.glob("mi_to_b_path/*.cpp")), key=str)
    files = [f for f in files if f.stem not in ("population_bimap", "mi_hashed_indices")]
    rc = c.port(["--lib", "bimap", "--modules", "bimap,tokenizer", "--out", "tests/bimap/examples.cpp",
                 "--header", EXAMPLES_HEADER,
                 "--stdin", "mighty_bimap=hola\\n", "--stdin", "bidirectional_map=hola\\n",
                 "--stdin", "tagged_bidirectional_map=goodbye\\n", "--stdin", "mi_bidirectional_map=table\\n",
                 "--stdin", "user_defined_names=1\\n2\\n"] + [str(f) for f in files])
    with_boost = [c.patched(P + "population_bimap.cpp"), c.patched(P + "mi_to_b_path/mi_hashed_indices.cpp"),
                  D / "bimap_and_boost/assign.cpp", D / "bimap_and_boost/lambda.cpp",
                  D / "bimap_and_boost/property_map.cpp", c.patched(P + "bimap_and_boost/range.cpp"),
                  c.patched(P + "bimap_and_boost/serialization.cpp")]
    rc |= c.port(["--lib", "bimap", "--modules",
                  "bimap,assign,bind,lambda,property_map,range,serialization,multi_index,tokenizer,optional",
                  "--out", "tests/bimap/examples_with_boost.cpp", "--header", WITH_BOOST_HEADER,
                  "--post-sub", "range:    count_if( bm.left.range=>    ex_range::count_if( bm.left.range",
                  "--oracle-for", "serialization=-L%s -lboost_serialization" % c.ORACLE_LIBS]
                 + [str(f) for f in with_boost])
    rc |= c.port(["--pure", "--lib", "bimap", "--modules", "", "--out", "tests/bimap/include_with_boost.cpp",
                  "--header", INCLUDE_HEADER]
                 + [str(D / "bimap_and_boost" / (s + ".cpp")) for s in ("foreach", "typeof", "xpressive")])
    return rc
