"""Boost.JSON: libs/json/example/ (examples.cpp)."""
import common as c

TMP = "std::filesystem::temp_directory_path()"
# the JSON file pretty / validate / cbor read, in the temporary directory (the cwd)
FIX = ('{ std::ofstream f(' + TMP + ' / "boost-m-json-sample.json"); '
       'f << R"j({"name":"boost","version":[1,92,0],"modules":true,"ratio":0.25,"tags":["a","b"],"nested":{"k":null}})j"; '
       'std::filesystem::current_path(std::filesystem::temp_directory_path()); }')
ARG = 'std::string("boost-m-json-sample.json")'
HEADER = """\
// mcpplibs.mwrap.boost.json — the official examples (libs/json/example/*.cpp)
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone
// (with the json TUs). pretty, validate and cbor read a JSON file named on
// their command line (a fixture in the temporary directory; validate also a
// malformed one); the example's file.hpp is inlined; cbor's
// BOOST_JSON_UNREACHABLE() (an internal macro) is std::unreachable(). parse_into_canada and
// parse_into_citm_catalog and parse_into are timing benchmarks over data files the release
// does not ship (excluded in coverage.toml).
"""


def run(group):
    D = c.lib_dir("json") / "example"
    return c.port(["--lib", "json", "--modules", "json,container",
                   *c.oracle_link("json_oracle", "container_oracle", "system_extras"),
                   "--inline-local", "--keep-include", "boost/describe", "--out", "tests/json/examples.cpp",
                   "--header", HEADER,
                   "--post-re", r"cbor:BOOST_JSON_UNREACHABLE\(\)=>std::unreachable()",
                   "--setup", "pretty=" + FIX, "--argv", "pretty=" + ARG,
                   "--setup", "validate=" + FIX, "--argv", "validate=" + ARG,
                   "--setup", "cbor=" + FIX, "--argv", "cbor=" + ARG]
                  + [str(D / (s + ".cpp")) for s in ("cbor", "path", "pretty", "proxy", "use_allocator", "validate")])
