"""Boost.Hash2: libs/hash2/example/*.cpp (examples.cpp)."""
import common as c

HEADER = """\
// mcpplibs.mwrap.boost.hash2 — the official examples (libs/hash2/example/*.cpp)
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone.
// md5sum and hash2sum (command-line tools) run on two files written to the
// temporary directory, named relative to it (the output prints them); json_value hashes a boost::json::value through the
// json module and adds its hash_append overload to namespace boost.
"""
# two input files in the temporary directory, which becomes the cwd
FILES = ('{ std::ofstream(std::filesystem::temp_directory_path() / "boost-m-hash2-a.txt", std::ios::binary) '
         '<< "The quick brown fox jumps over the lazy dog"; '
         'std::ofstream(std::filesystem::temp_directory_path() / "boost-m-hash2-b.txt", std::ios::binary) << ""; '
         'std::filesystem::current_path(std::filesystem::temp_directory_path()); }')
A, B = 'std::string("boost-m-hash2-a.txt")', 'std::string("boost-m-hash2-b.txt")'


def run(group):
    return c.port(["--lib", "hash2", "--modules", "hash2,json,unordered",
                   "--oracle-for", "json_value=-L%s -ljson_oracle -lsystem_extras" % c.ORACLE_LIBS,
                   "--setup", "md5sum=" + FILES, "--setup", "hash2sum=" + FILES,
                   "--argv", "md5sum=%s,,%s" % (A, B), "--argv", 'hash2sum=std::string("md5_128"),,%s,,%s' % (A, B),
                   "--out", "tests/hash2/examples.cpp", "--header", HEADER]
                  + [str(f) for f in sorted((c.lib_dir("hash2") / "example").glob("*.cpp"))])
