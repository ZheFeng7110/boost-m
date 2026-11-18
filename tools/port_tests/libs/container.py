"""Boost.Container: libs/container/example/*.cpp (examples.cpp)."""
import common as c

HEADER = """\
// mcpplibs.mwrap.boost.container — the official examples
// (libs/container/example/*.cpp) ported mechanically: each program runs as a
// case and its output must equal what the same code printed when built from
// upstream/boost headers alone. The BOOST_CONTAINER_TRY/CATCH and
// BOOST_RV_REF / BOOST_MOVABLE_BUT_NOT_COPYABLE macros the examples use come
// from their upstream headers (container/detail/workaround.hpp, move/core.hpp),
// included before the imports (docs/usage.md, macros). doc_extended_allocators
// needs the dlmalloc-based allocators, which the package does not build
// (tools/gen_features.py: alloc_lib.c is a C TU): coverage.toml, gap.
"""


def run(group):
    files = [f for f in sorted((c.lib_dir("container") / "example").glob("*.cpp")) if "extended_allocators" not in f.name]
    return c.port(["--lib", "container", "--modules", "container,move",
                   "--add-include", "boost/container/detail/workaround.hpp", "--add-include", "boost/move/core.hpp",
                   *c.oracle_link("container_oracle"), "--out", "tests/container/examples.cpp", "--header", HEADER]
                  + [str(f) for f in files])
