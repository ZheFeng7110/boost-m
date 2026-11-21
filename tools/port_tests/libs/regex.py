"""Boost.Regex: libs/regex/example/snippets/*.cpp (examples.cpp) and
grep/grep.cpp (examples_grep.cpp)."""
import common as c

GROUPS = ["snippets", "grep"]

TMP = "std::filesystem::temp_directory_path()"
# the C++ file the indexing examples read, in the temporary directory (the cwd)
FIX = ('{ std::ofstream f(' + TMP + ' / "boost-m-regex-fixture.cpp"); '
       'f << "#include <vector>\\nclass widget\\n{\\npublic:\\n   int x;\\n};\\ntemplate <class T>\\n'
       'struct holder : public widget\\n{\\n   T value;\\n};\\nclass gadget;\\nclass thing { };\\n'
       '// trailing comment\\nint main() { return 0; }\\n"; std::filesystem::current_path(' + TMP + '); }')
ARG = 'std::string("boost-m-regex-fixture.cpp")'
FILE_PROGS = ["regex_search_example", "regex_iterator_example", "regex_grep_example_1", "regex_grep_example_2",
              "regex_grep_example_3", "regex_merge_example", "regex_replace_example",
              "regex_split_example_2", "regex_token_iterator_eg_2", "partial_regex_grep", "partial_regex_iterate"]
OTHERS = ["regex_match_example", "regex_split_example_1", "regex_token_iterator_eg_1", "credit_card_example",
          "partial_regex_match"]
HEADER = """\
// mcpplibs.mwrap.boost.regex — the official examples (libs/regex/example/
// snippets/*.cpp) ported mechanically: each program runs as a case and its
// output must equal what the same code printed when built from upstream/boost
// headers alone (with the regex TUs). The TU includes the standard headers
// each example includes instead of `import std;`: the examples write
// `using namespace std; using namespace boost;`, and std::regex must stay
// unseen as in the originals. The examples that index the classes of
// the C++ files named on their command line read one fixture file written to
// the temporary directory; the interactive ones read a fixed stdin. grep.cpp
// and timer/regex_timer.cpp are command-line tools (examples_grep.cpp /
// excluded); icu_example and mfc_example need ICU and MFC, captures_example a
// regex library built with BOOST_REGEX_MATCH_EXTRA, regex_grep_example_4
// Borland's __closure (all excluded in coverage.toml).
"""

GREP_FIX = ('{ std::ofstream f(' + TMP + ' / "boost-m-regex-grep.txt"); '
            'f << "alpha line\\nclass widget\\nBeta Line\\nclass gadget\\nomega\\n"; '
            'std::filesystem::current_path(' + TMP + '); }')
GREP_HEADER = """\
// mcpplibs.mwrap.boost.regex — libs/regex/example/grep/grep.cpp, a grep built
// on Boost.Regex and Boost.Program_options, run as a case on a fixture file
// with -n -i "class|line" and with -c -E: its output must equal what the same
// code printed when built from upstream/boost headers alone. The file path is
// the program's own printing of its input, so the cases print it too.
"""


def run(group):
    D = c.lib_dir("regex") / "example"
    if group == "grep":
        return c.port(["--lib", "regex", "--modules", "regex,program_options",
                       *c.oracle_link("regex_oracle", "po_oracle", "system_extras"),
                       "--out", "tests/regex/examples_grep.cpp", "--header", GREP_HEADER,
                       "--setup", "grep=" + GREP_FIX,
                       "--argv", 'grep=std::string("-n"),,std::string("-i"),,std::string("class|line"),,'
                       'std::string("boost-m-regex-grep.txt")', str(D / "grep/grep.cpp")])
    cmd = ["--lib", "regex", "--modules", "regex", "--std-includes", *c.oracle_link("regex_oracle"),
           "--out", "tests/regex/examples.cpp", "--header", HEADER]
    for p in FILE_PROGS:
        cmd += ["--setup", p + "=" + FIX, "--argv", p + "=" + ARG]
    cmd += ["--stdin", "regex_match_example=100 this is an ftp message text\\n500 no\\nquit\\n",
            "--stdin", "regex_split_example_1=This is a string of tokens\\nquit\\n",
            "--stdin", "regex_token_iterator_eg_1=quit\\n",
            "--stdin", "partial_regex_match=1234-5678-1234-456\\n1234-5678-12\\nquit\\n"]
    return c.port(cmd + [str(D / "snippets" / (p + ".cpp")) for p in FILE_PROGS + OTHERS])
