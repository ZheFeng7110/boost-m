"""Boost.Geometry: the documentation's examples (doc/src/examples/, doc/index/
src/examples/), libs/geometry/example/ and index/example/ (examples*.cpp,
split into _2, _3, ... where registrations disagree)."""
import re
import sys

import common as c

GROUPS = ["algorithms", "geometries", "misc", "example", "index"]

G = c.lib_dir("geometry")
E = G / "doc/src/examples"
KEEP = r"boost/geometry/geometries/(register|adapted)/|boost/foreach|boost/fusion/|boost/tuple/"
def port_split(out, header, files, modules="geometry,algorithm,core", extra=()):
    parts = partition(files)
    rc = 0
    for n, part in enumerate(parts, 1):
        rc |= run1(out if n == 1 else out.replace(".cpp", "_%d.cpp" % n), header, part, modules, extra)
    return rc

def run1(out, header, files, modules, extra):
    cmd = ["--lib", "geometry", "--modules", modules,
           "--keep-include", KEEP, "--inline-local", "--namespace-macro", r"BOOST_GEOMETRY_REGISTER_\w+|BOOST_FUSION_ADAPT_\w+", "--reopen-namespaces",
           "--macros-of", r"boost/geometry/geometries/adapted/(?!boost_fusion)",
           "--add-include", "boost/assert.hpp",
           # typeid(...).name() (the C++ library's mangling) and polygons
           # from std::cos/std::sin at rounding boundaries (the libm's)
           *sum((["--output-unless", "%s=1" % x] for x in [
               "make_2d_point", "coordinate_type", "coordinate_system", "tag_cast", "ring_type", "point_type",
               "interior_type", "polygons_shared_ptr", "polygons_vector", "variants_map", "mapped_file"]), []),
           "--extra-helpers", c.helper("cwd_tmp.hpp"),
           "--case-from-rel", "--out", out, "--header", header, *extra] + [str(f) for f in files]
    print("==", out, len(files))
    sys.stdout.flush()
    return c.port(cmd)

def registrations(f):
    """The registrations of an example that are global to a TU: the *_CS
    macros (one coordinate system per adapted type) and registrations of std::
    containers. Two examples whose keys disagree cannot share a file."""
    text = f.read_text(errors="replace")
    keys = {}
    for m in re.finditer(r"^(BOOST_GEOMETRY_REGISTER_\w+)\s*\(((?:[^()]|\([^()]*\))*)\)", text, re.M):
        macro, args = m.group(1), re.sub(r"\s+", "", m.group(2))
        if macro.endswith("_CS"):
            keys[macro] = args
        elif "std::" in args:
            keys["std:" + re.sub(r"<.*", "", args.split(",")[0])] = macro + "(" + args + ")"
    if "boost_fusion.hpp" in text:
        keys["fusion"] = "own file"  # the textual fusion adaptation meets the module's tuple one
    return keys

def partition(files):
    bins = []
    for f in files:
        k = registrations(f)
        for b in bins:
            if all(b["keys"].get(x, v) == v for x, v in k.items()) and ("fusion" in k) == ("fusion" in b["keys"]):
                b["files"].append(f)
                b["keys"].update(k)
                break
        else:
            bins.append({"files": [f], "keys": dict(k)})
    return [b["files"] for b in bins]

HEAD = """// mcpplibs.mwrap.boost.geometry — the documentation's {what} examples
// ({where}) ported mechanically: each program runs as a case
// and its output must equal what the same code printed when built from
// upstream/boost headers alone. The registration macros
// (BOOST_GEOMETRY_REGISTER_*) and the adaptation headers that define them
// (geometries/register) stay #included before the imports; of the adapted
// headers (geometries/adapted), whose specializations the module has, only the
// macro definitions are copied (gcc takes the module's and a textual copy of
// the same partial specializations as ambiguous);
// and the case namespace closes around each registration (the macros open
// namespace boost::geometry::traits) and each specialization the examples
// write in namespace boost (in place: they use the examples' own types).
// Examples whose registrations of std:: containers or coordinate systems
// disagree are in different files (_2, _3, ...). Not compared: output of
// typeid(...).name() (the C++ library's mangling) and of the rtree examples
// whose polygons come from std::cos/std::sin values on rounding boundaries
// (and mapped_file, whose file outlives a run).
// boost_range/uniqued.cpp does not compile upstream (coverage.toml). The examples' SVG helpers are inlined;
// they write SVG only under HAVE_SVG, which the cases do not define.
"""

def group_algorithms():
    fs = sorted((G / "doc/src/examples/algorithms").glob("*.cpp"))
    return port_split("tests/geometry/examples_algorithms.cpp", HEAD.format(what="algorithm", where="libs/geometry/doc/src/examples/algorithms/"), fs)

def group_geometries():
    fs = sorted(E.glob("core/*.cpp")) + sorted(E.glob("geometries/*.cpp")) + sorted(E.glob("geometries/register/*.cpp")) + \
        sorted(E.glob("geometries/adapted/*.cpp")) + [f for f in sorted(E.glob("geometries/adapted/boost_range/*.cpp")) if f.stem != "uniqued"]
    return port_split("tests/geometry/examples_core_geometries.cpp",
        HEAD.format(what="core, geometry and registration", where="libs/geometry/doc/src/examples/{core,geometries}/"), fs,
        modules="geometry,algorithm,core,polygon,tuple,range")

def group_misc():
    fs = sorted(E.glob("strategies/*.cpp")) + sorted(E.glob("io/*.cpp")) + sorted(E.glob("views/*.cpp")) + \
        sorted(E.glob("arithmetic/*.cpp")) + sorted(E.glob("*.cpp"))
    return port_split("tests/geometry/examples_strategies_io.cpp",
        HEAD.format(what="strategy, I/O, view and arithmetic", where="libs/geometry/doc/src/examples/{strategies,io,views,arithmetic}/, quick_start.cpp"), fs)

def group_example():
    # c03 uses the extensions (not in the release)
    fs = [f for f in sorted((G / "example").glob("*.cpp")) if f.stem != "c03_custom_linestring_example"]
    # BOOST_INSTALL_PROPERTY pastes the example's enum name: the expansion is
    # written out with the name qualified (the block is outside the case namespace)
    ext = []
    for ns in ["07_a_graph_route_example"]:
        for kind in ["vertex", "edge"]:
            ext += ["--post-re", "%s:BOOST_INSTALL_PROPERTY\\(%s, bg_property\\);=>"
                    "template <> struct property_kind< ::ex_%s::%s_bg_property_t > { typedef %s_property_tag type; };"
                    "  // BOOST_INSTALL_PROPERTY(%s, bg_property)" % (ns, kind, ns, kind, kind, kind)]
    return port_split("tests/geometry/examples.cpp",
        HEAD.format(what="tutorial", where="libs/geometry/example/") .replace("the documentation's tutorial", "the official"), fs,
        modules="geometry,algorithm,core,tuple,range,graph,polygon,assign", extra=ext)

def group_index():
    # index/example: benchmarks (timings), a GLUT visualizer, and two that use
    # rtree internals no longer accessible (they do not compile upstream)
    SKIP = {"3d_benchmark", "benchmark", "benchmark2", "benchmark3", "benchmark_experimental", "benchmark_insert",
            "glut_vis", "random_test", "serialize"}
    fs = sorted((G / "doc/index/src/examples/rtree").glob("*.cpp")) + \
        [f for f in sorted((G / "index/example").glob("*.cpp")) if f.stem not in SKIP]
    return port_split("tests/geometry/examples_index.cpp",
        HEAD.format(what="rtree", where="libs/geometry/doc/index/src/examples/rtree/, libs/geometry/index/example/"), fs,
        modules="geometry,algorithm,core,interprocess,timer,range,container")

def run(group):
    return globals()["group_" + group]()
