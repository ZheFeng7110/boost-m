"""Boost.Bloom: libs/bloom/example/{basic,rolling_filter,serialization}.cpp
(examples.cpp). genome.cpp is hand-written (tests/bloom/genome.cpp)."""
import common as c

HEADER = """\
// mcpplibs.mwrap.boost.bloom — libs/bloom/example/{basic,rolling_filter,
// serialization}.cpp ported mechanically: each program runs as a case and its
// output must equal what the original printed when built from upstream/boost
// alone. serialization writes its filter to the temporary directory instead
// of ./filter.bin, and keeps its #include of boost/core/detail/splitmix64.hpp
// (a detail header no module exports). genome.cpp reads a 140 MB genome
// download: genome.cpp in this directory runs it on a small FASTA fixture.
"""


def run(group):
    D = c.lib_dir("bloom") / "example"
    return c.port(["--lib", "bloom", "--modules", "bloom,uuid", "--keep-include", "boost/core/detail/splitmix64",
                   "--out", "tests/bloom/examples.cpp", "--header", HEADER,
                   "--post-sub", 'serialization:  static constexpr const char* filename = "filter.bin";=>'
                   '  static const std::string filename_s = (std::filesystem::temp_directory_path() / '
                   '"boost-m-bloom-filter.bin").string();\n  static const char* filename = filename_s.c_str();']
                  + [str(D / (s + ".cpp")) for s in ("basic", "rolling_filter", "serialization")])
