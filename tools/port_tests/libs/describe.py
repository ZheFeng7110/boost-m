"""Boost.Describe: libs/describe/example/ (include_examples.cpp,
examples_json.cpp, examples_serialization.cpp). Edited source:
sources/describe.patch (console.cpp)."""
import common as c

D = c.lib_dir("describe") / "example"

INCLUDE_HEADER = """\
// boost.describe — the official examples (libs/describe/example/*.cpp) that use
// describe with header-only companions (mp11, container_hash, core,
// type_traits, variant2, utility): describe has no module (its API is the
// BOOST_DESCRIBE_* macros, docs/usage.md §2.3), so this TU includes and does
// not import. Each program runs as a case; its output must equal what the
// same code printed when built from upstream/boost headers alone. The examples that
// need a compiled library (json, serialization) are examples_json.cpp and
// examples_serialization.cpp; the fmtlib ones are excluded (fmt is not
// vendored).
// pm_to_string.cpp names its own ::detail namespace; in the port it is the
// case namespace's detail.
"""
JSON_HEADER = """\
// boost.describe with the json module — libs/describe/example/{console,
// from_json,json_rpc,to_json}.cpp: the BOOST_DESCRIBE_* macros come from
// #include (no module can carry them) and Boost.JSON from its module, whose
// describe-based conversions see the described types (design §4.4: the
// includes precede every import). Each program runs as a case; its output
// must equal what the original printed (built from upstream/boost with
// boost/json/src.hpp).
// console.cpp reads a fixed stdin, and its endless loop stops at the end of
// input (an added 'if (!std::cin) break;', also in the oracle's copy).
"""
SERIALIZATION_HEADER = """\
// boost.describe with the serialization module — libs/describe/example/
// serialization.cpp: described classes serialized through a generic
// serialize() written with describe_members, archives from the module.
// Output must equal the original program's.
"""


def run(group):
    include = ["enum_to_string", "equality", "hash_value", "pm_to_string", "print_function", "printing_enums_ct",
               "printing_enums_rt", "string_to_enum", "struct_to_tuple"]
    rc = c.port(["--pure", "--lib", "describe", "--modules", "",
                 "--post-sub", "pm_to_string:::detail::get_member_name=>detail::get_member_name",
                 "--output-unless", "string_to_enum=defined(_MSC_VER)",
                 "--out", "tests/describe/include_examples.cpp", "--header", INCLUDE_HEADER]
                + [str(D / (s + ".cpp")) for s in include])
    rc |= c.port(["--lib", "describe", "--modules", "json,mp11,variant2,utility,type_traits,core",
                  "--keep-include", "boost/describe|boost/version.hpp",
                  *c.oracle_link("json_oracle", "system_extras"),
                  "--stdin", "console=x\\nx=42\\nx\\nv=[4,5]\\nv\\nm\\nhelp=1\\nzzz\\ny=2.5\\ny\\n",
                  "--out", "tests/describe/examples_json.cpp", "--header", JSON_HEADER,
                  str(c.patched("libs/describe/example/console.cpp"))]
                 + [str(D / (s + ".cpp")) for s in ("from_json", "json_rpc", "to_json")])
    rc |= c.port(["--lib", "describe", "--modules", "serialization,mp11,core", "--keep-include", "boost/describe",
                  "--oracle-for", "serialization=-L%s -lboost_serialization" % c.ORACLE_LIBS,
                  "--out", "tests/describe/examples_serialization.cpp", "--header", SERIALIZATION_HEADER,
                  str(D / "serialization.cpp")])
    return rc
