"""Boost.Decimal: libs/decimal/example/*.cpp (examples.cpp)."""
import common as c

HEADER = """\
// mcpplibs.mwrap.boost.decimal — the official examples (libs/decimal/example/
// *.cpp) ported mechanically: each program runs as a case and its output must
// equal what the same code printed when built from upstream/boost headers
// alone. The examples branch on the library's configuration macros
// (BOOST_DECIMAL_HAS_FORMAT_SUPPORT, ...); like any consumer of those macros
// the TU includes <version>, boost/decimal/detail/config.hpp and the headers
// that define them (decimal/format.hpp) before the import. statistics.cpp and
// rounding_mode_compile_time.cpp define configuration macros before the
// headers (the module is built with the defaults) and are excluded, as are
// the fmtlib example's fmt parts (fmt is not vendored). print.cpp's
// std::print goes to stdout, so the port formats into the captured stream;
// to_from_file draws its values from std::uniform_int_distribution, whose
// sequence is the standard library's own, so only its round-trip result is
// checked (the program verifies the values it reads back).
"""
# configuration macros defined before the headers (the module has the defaults)
EXCLUDE = {"statistics", "rounding_mode_compile_time"}


def run(group):
    files = [f for f in sorted((c.lib_dir("decimal") / "example").glob("*.cpp")) if f.stem not in EXCLUDE]
    return c.port(["--lib", "decimal", "--modules", "decimal",
                   "--add-include", "version", "--add-include", "boost/decimal/detail/config.hpp",
                   "--add-include", "boost/decimal/format.hpp", "--inline-local",
                   "--post-re", r"print:std::print\(=>ex_out << std::format(",
                   "--output-unless", "to_from_file=1",
                   "--out", "tests/decimal/examples.cpp", "--header", HEADER] + [str(f) for f in files])
