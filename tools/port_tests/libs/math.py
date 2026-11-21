"""Boost.Math: libs/math/example/*.cpp, in four files (examples_1..4.cpp)."""
import re
import sys
import common as c

GROUPS = ["1", "2", "3", "4"]

EXCLUDE = {
    # the default policy configured with BOOST_MATH_*_POLICY before the headers
    "geometric_examples", "inverse_gaussian_example", "negative_binomial_example1", "error_handling_example",
    # data files and Boost.Test programs, no main
    "HSO3SO4", "special_data", "test_cpp_float_close_fraction",
    # need svg_plot
    "lambert_w_graph", "lambert_w_diode_graph",
    # do not compile upstream (1.92.0): names that no longer exist
    "continued_fractions", "lambert_w_example", "policy_eg_8",
    # an optimiser run of minutes
    "jso_example",
    # crashes as published: tanh_sinh<double>(tol, 10) passes tol as max_refinements (0)
    "double_exponential",
    # write into ./.temps/, a directory of the upstream test tree (the oracle aborts without it)
    "nonfinite_serialization_archives", "nonfinite_num_facet_serialization",
    # need GMP, quadmath, Eigen, lodepng or Boost.Timer's library
    "centered_continued_fraction", "luroth", "reciprocal_fibonacci_constant", "lambert_w_precision_example",
    "float128_example", "root_finding_multiprecision_example", "root_finding_n_example", "brent_minimise_example",
    "cma_es_example", "color_maps_example", "color_maps_sf_example", "root_finding_algorithms",
    "root_n_finding_algorithms", "root_elliptic_finding",
}
# random seeds, timings or the platform's float layout in the output
NO_COMPARE = ["random_search_example", "inspect_fp", "naive_monte_carlo_example",
              "reverse_mode_linear_regression_example", "differential_evolution", "root_finding_example",
              "nonfinite_num_facet_trap", "autodiff_reverse_black_scholes"]
HEADER = """\
// mcpplibs.mwrap.boost.math — the official examples (libs/math/example/{part}),
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone. The
// headers of the macros the examples use (BOOST_MATH_DECLARE_*, BOOST_MATH_ASSERT,
// BOOST_FLOAT64_C, BOOST_SERIALIZATION_NVP) are #included before the imports.
// Outputs with random seeds, timings, the float layout or the C++/C library's
// text are not compared (random_search, inspect_fp, naive_monte_carlo,
// reverse_mode_linear_regression, differential_evolution; root_finding_example:
// std::cbrt's last digit; autodiff_reverse_black_scholes: last digits of libm results;
// nonfinite_num_facet_trap: the iostream_category message). The outputs are
// those of the oracle's platform, x86_64 Linux: elsewhere the width of long
// double and the libm's last digits differ, and a case checks the exit status.
#if defined(__x86_64__) && defined(__linux__)
#define EX_OUTPUT_PLATFORM 1
#else
#define EX_OUTPUT_PLATFORM 0
#endif
"""


def parts(files):
    """Four TUs of similar size. An example stays in the file that has it, so
    a release that adds or drops examples changes only those cases; a new one
    goes to the file with the fewest."""
    where = {}
    for n in range(1, 5):
        out = c.TESTS / ("math/examples_%d.cpp" % n)
        if out.is_file():
            for name in re.findall(r'^TEST_CASE\("example: ([^"]+)"\)', out.read_text(), re.M):
                where[name] = n - 1
    result = [[] for _ in range(4)]
    for f in files:
        if f.stem in where:
            result[where[f.stem]].append(f)
    for f in files:
        if f.stem not in where:
            min(result, key=len).append(f)
    return [sorted(p) for p in result]


def run(group):
    files = [f for f in sorted((c.lib_dir("math") / "example").glob("*.cpp")) if f.stem not in EXCLUDE]
    n = int(group)
    part = parts(files)[n - 1]
    out = "tests/math/examples_%d.cpp" % n
    cmd = ["--lib", "math",
           "--modules", "math,multiprecision,lexical_cast,array,random,range,endian,format,serialization,type_traits",
           "--keep-include",
           r"boost/math/special_functions/math_fwd\.hpp|boost/math/distributions/fwd\.hpp|boost/math/policies/policy\.hpp",
           "--add-include", "boost/math/tools/config.hpp", "--add-include", "boost/assert.hpp",
           # the macros the examples use (macros are not exported)
           "--add-include", "boost/math/tools/assert.hpp", "--add-include", "boost/math/cstdfloat/cstdfloat_types.hpp",
           "--add-include", "boost/math/special_functions/math_fwd.hpp", "--add-include", "boost/math/distributions/fwd.hpp",
           "--add-include", "boost/serialization/nvp.hpp",
           *c.oracle_link("boost_serialization"),
           # boost::is_same / is_integral: the examples relied on Boost.TypeTraits being
           # reached through the math headers, which it no longer is (the module imports it)
           "--oracle-arg=-include", "--oracle-arg=boost/type_traits.hpp",
           *sum((["--output-unless", "%s=1" % x] for x in NO_COMPARE), []),
           # lexical_cast_nonfinite_facets installs a global locale with the nonfinite facets
           "--reset-locale", "--output-platform",
           # the plots and data files the examples write go to the temporary directory
           "--extra-helpers", c.helper("cwd_tmp.hpp"),
           "--out", out, "--header", HEADER.format(part="*.cpp, part %d of 4" % n)] + [str(f) for f in part]
    print("==", out, len(part))
    sys.stdout.flush()
    return c.port(cmd)
