"""Boost.URL: the example programs of libs/url/example/ (examples.cpp)."""
import common as c

HEADER = """\
// mcpplibs.mwrap.boost.url — the official example programs (libs/url/example/
// {mailto,magnet,qrcode,finicky,sanitize,suffix_list}) ported mechanically:
// each runs as a case with the command line its usage text documents, and its
// output must equal what the same code printed when built from upstream/boost
// headers alone (with the url and json TUs). finicky reads the example's own
// config.json, suffix_list a subset of the public suffix list, both written to
// the temporary directory; the examples' own headers are inlined; finicky's CHECK macro is EX_CHECK here (doctest's
// CHECK). router and file_router are beast HTTP servers (excluded in
// coverage.toml).
"""
TMP = "std::filesystem::temp_directory_path()"
MAIL = ('std::string("mailto:someone@example.com,%73omeoneelse@me.com?to=thirdperson@example.com'
        '&subject=Our%20meeting&cc=someone_else@example.com,onemore@ex%61mple.com&body=Hi%21")')
MAGNET = ('std::string("magnet:?xt=urn:btih:d2474e86c95b19b8bcfdb92bc12c9d44667cfa36&dn=Leaves+of+Grass+by+Walt+Whitman.epub'
          '&tr=udp%3A%2F%2Ftracker.example4.com%3A80&tr=udp%3A%2F%2Ftracker.example1.com%3A1337")')
SUF = ('{ std::ofstream f(' + TMP + ' / "boost-m-url-suffixes.dat"); f << "// a subset of the public suffix list\\n'
       'com\\norg\\nuk\\nco.uk\\n*.ck\\n!www.ck\\n"; }')


def run(group):
    D = c.lib_dir("url") / "example"
    cfg = (D / "finicky/config.json").read_text()
    fin = '{ std::ofstream f(' + TMP + ' / "boost-m-url-finicky.json"); f << R"cfg(' + cfg + ')cfg"; }'
    return c.port(["--lib", "url", "--modules", "url,json,core,regex", "--inline-local",
                   "--add-include", "boost/assert.hpp",
                   *c.oracle_link("url_oracle", "json_oracle", "regex_oracle", "system_extras"),
                   "--out", "tests/url/examples.cpp", "--header", HEADER,
                   "--argv", "mailto=" + MAIL, "--argv", "magnet=" + MAGNET,
                   "--argv", 'qrcode=std::string("Hello world"),,std::string("200"),,std::string("200"),,std::string("UTF-8")',
                   "--setup", "finicky=" + fin,
                   "--argv", "finicky=(" + TMP + ' / "boost-m-url-finicky.json").string(),,'
                   'std::string("http://www.example.org/index.html")',
                   "--argv", 'sanitize=std::string("http://www.example.com/a/./b/../c?q=%20x#frag")',
                   "--setup", "suffix_list=" + SUF,
                   "--argv", 'suffix_list=std::string("https://www.bbc.co.uk/news"),,(' + TMP +
                   ' / "boost-m-url-suffixes.dat").string()',
                   "--post-re", r"finicky:\bCHECK\(=>EX_CHECK("]
                  + [str(D / f / (f + ".cpp")) for f in ("mailto", "magnet", "qrcode", "finicky", "sanitize",
                                                        "suffix_list")])
