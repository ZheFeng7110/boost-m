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
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <cassert>
#include <functional>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <sstream>
#include <iterator>
#include <list>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
import mcpplibs.mwrap.boost.regex;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// libs/regex/example/snippets/regex_search_example.cpp
namespace ex_regex_search_example {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_search_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_search example: searches a cpp file for class definitions.
  */
  

// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";


boost::regex expression(re);

void IndexClasses(map_type& m, const std::string& file)
{
   std::string::const_iterator start, end;
   start = file.begin();
   end = file.end();   
   boost::match_results<std::string::const_iterator> what;
   boost::match_flag_type flags = boost::match_default;
   while(boost::regex_search(start, end, what, expression, flags))   
   {
      // what[0] contains the whole string
      // what[5] contains the class name.
      // what[6] contains the template specialisation if any.
      // add class name and position to map:
      m[std::string(what[5].first, what[5].second) + std::string(what[6].first, what[6].second)] = 
               what[5].first - file.begin();      
      // update search position:
      start = what[0].second;      
      // update flags:
      flags |= boost::match_prev_avail;
      flags |= boost::match_not_bob;
   }
}



using namespace std;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << endl;
      map_type m;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      IndexClasses(m, text);
      ex_out << m.size() << " matches found" << endl;
      map_type::iterator c, d;
      c = m.begin();
      d = m.end();
      while(c != d)
      {
         ex_out << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_search_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
3 matches found
class "holder" found at index: 80
class "thing" found at index: 140
class "widget" found at index: 24
)ex"));
}
}  // namespace ex_regex_search_example

// libs/regex/example/snippets/regex_iterator_example.cpp
namespace ex_regex_iterator_example {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_iterator_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_iterator example 2: searches a cpp file for class definitions,
  *                using global data.
  */


using namespace std;

// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";


boost::regex expression(re);
map_type class_index;

bool regex_callback(const boost::match_results<std::string::const_iterator>& what)
{
   // what[0] contains the whole string
   // what[5] contains the class name.
   // what[6] contains the template specialisation if any.
   // add class name and position to map:
   class_index[what[5].str() + what[6].str()] = what.position(5);
   return true;
}

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << endl;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      // construct our iterators:
      boost::sregex_iterator m1(text.begin(), text.end(), expression);
      boost::sregex_iterator m2;
      std::for_each(m1, m2, &regex_callback);
      // copy results:
      ex_out << class_index.size() << " matches found" << endl;
      map_type::iterator c, d;
      c = class_index.begin();
      d = class_index.end();
      while(c != d)
      {
         ex_out << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
      class_index.erase(class_index.begin(), class_index.end());
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_iterator_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
3 matches found
class "holder" found at index: 80
class "thing" found at index: 140
class "widget" found at index: 24
)ex"));
}
}  // namespace ex_regex_iterator_example

// libs/regex/example/snippets/regex_grep_example_1.cpp
namespace ex_regex_grep_example_1 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_grep_example_1.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_grep example 1: searches a cpp file for class definitions.
  */


// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";

boost::regex expression(re);

class IndexClassesPred
{
   map_type& m;
   std::string::const_iterator base;
public:
   IndexClassesPred(map_type& a, std::string::const_iterator b) : m(a), base(b) {}
   bool operator()(const boost::match_results<std::string::const_iterator>& what)
   {
      // what[0] contains the whole string
      // what[5] contains the class name.
      // what[6] contains the template specialisation if any.
      // add class name and position to map:
      m[std::string(what[5].first, what[5].second) + std::string(what[6].first, what[6].second)] = 
               what[5].first - base;
      return true;
   }
private:
   IndexClassesPred& operator=(const IndexClassesPred&);
};

void IndexClasses(map_type& m, const std::string& file)
{
   std::string::const_iterator start, end;
   start = file.begin();
   end = file.end();
   boost::regex_grep(IndexClassesPred(m, start), start, end, expression);
}



using namespace std;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << endl;
      map_type m;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      IndexClasses(m, text);
      ex_out << m.size() << " matches found" << endl;
      map_type::iterator c, d;
      c = m.begin();
      d = m.end();
      while(c != d)
      {
         ex_out << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_grep_example_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
3 matches found
class "holder" found at index: 80
class "thing" found at index: 140
class "widget" found at index: 24
)ex"));
}
}  // namespace ex_regex_grep_example_1

// libs/regex/example/snippets/regex_grep_example_2.cpp
namespace ex_regex_grep_example_2 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_grep_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_grep example 2: searches a cpp file for class definitions,
  *                using a global callback function.
  */


// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";


boost::regex expression(re);
map_type class_index;
std::string::const_iterator base;

bool grep_callback(const boost::match_results<std::string::const_iterator>& what)
{
   // what[0] contains the whole string
   // what[5] contains the class name.
   // what[6] contains the template specialisation if any.
   // add class name and position to map:
   class_index[std::string(what[5].first, what[5].second) + std::string(what[6].first, what[6].second)] = 
               what[5].first - base;
   return true;
}

void IndexClasses(const std::string& file)
{
   std::string::const_iterator start, end;
   start = file.begin();
   end = file.end();
   base = start;
   boost::regex_grep(grep_callback, start, end, expression);
}


using namespace std;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << endl;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      IndexClasses(text);
      ex_out << class_index.size() << " matches found" << endl;
      map_type::iterator c, d;
      c = class_index.begin();
      d = class_index.end();
      while(c != d)
      {
         ex_out << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
      class_index.erase(class_index.begin(), class_index.end());
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_grep_example_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
3 matches found
class "holder" found at index: 80
class "thing" found at index: 140
class "widget" found at index: 24
)ex"));
}
}  // namespace ex_regex_grep_example_2

// libs/regex/example/snippets/regex_grep_example_3.cpp
namespace ex_regex_grep_example_3 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_grep_example_3.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_grep example 3: searches a cpp file for class definitions,
  *                using a bound member function callback.
  */


// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";


class class_index
{
   boost::regex expression;
   map_type index;
   std::string::const_iterator base;

   bool grep_callback(boost::match_results<std::string::const_iterator> what);
public:
   map_type& get_map() { return index; }
   void IndexClasses(const std::string& file);
   class_index()
      : expression(re) {}
};

bool class_index::grep_callback(boost::match_results<std::string::const_iterator> what)
{
   // what[0] contains the whole string
   // what[5] contains the class name.
   // what[6] contains the template specialisation if any.
   // add class name and position to map:
   index[std::string(what[5].first, what[5].second) + std::string(what[6].first, what[6].second)] =
               what[5].first - base;
   return true;
}

void class_index::IndexClasses(const std::string& file)
{
   std::string::const_iterator start, end;
   start = file.begin();
   end = file.end();
   base = start;
#if BOOST_WORKAROUND(_MSC_VER, < 1300) && !defined(_STLP_VERSION)
   boost::regex_grep(std::bind1st(std::mem_fun1(&class_index::grep_callback), this),
            start,
            end,
            expression);
#elif defined(BOOST_NO_CXX98_BINDERS)
   boost::regex_grep(std::bind(&class_index::grep_callback, this, std::placeholders::_1),
            start,
            end,
            expression);
#else
   boost::regex_grep(std::bind1st(std::mem_fun(&class_index::grep_callback), this),
            start,
            end,
            expression);
#endif
}



using namespace std;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << endl;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      class_index idx;
      idx.IndexClasses(text);
      ex_out << idx.get_map().size() << " matches found" << endl;
      map_type::iterator c, d;
      c = idx.get_map().begin();
      d = idx.get_map().end();
      while(c != d)
      {
         ex_out << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_grep_example_3") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
3 matches found
class "holder" found at index: 80
class "thing" found at index: 140
class "widget" found at index: 24
)ex"));
}
}  // namespace ex_regex_grep_example_3

// libs/regex/example/snippets/regex_merge_example.cpp
namespace ex_regex_merge_example {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_merge_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_merge example:
  *                converts a C++ file to syntax highlighted HTML.
  */


// purpose:
// takes the contents of a file and transform to
// syntax highlighted code in html format

boost::regex e1, e2;
extern const char* expression_text;
extern const char* format_string;
extern const char* pre_expression;
extern const char* pre_format;
extern const char* header_text;
extern const char* footer_text;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   try{
   e1.assign(expression_text);
   e2.assign(pre_expression);
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << std::endl;
      std::ifstream fs(argv[i]);
      std::string in;
      load_file(in, fs);
      fs.close();
      std::string out_name = std::string(argv[i]) + std::string(".htm");
      std::ofstream os(out_name.c_str());
      os << header_text;
      // strip '<' and '>' first by outputting to a
      // temporary string stream
      std::ostringstream t(std::ios::out | std::ios::binary);
      std::ostream_iterator<char> oi(t);
      boost::regex_merge(oi, in.begin(), in.end(), e2, pre_format, boost::match_default | boost::format_all);
      // then output to final output stream
      // adding syntax highlighting:
      std::string s(t.str());
      std::ostream_iterator<char> out(os);
      boost::regex_merge(out, s.begin(), s.end(), e1, format_string, boost::match_default | boost::format_all);
      os << footer_text;
      os.close();
   }
   }
   catch(...)
   { return -1; }
   return 0;
    return 0;  // main's implicit return
}

const char* pre_expression = "(<)|(>)|\\r";
const char* pre_format = "(?1&lt;)(?2&gt;)";


const char* expression_text = // preprocessor directives: index 1
                              "(^[[:blank:]]*#(?:[^\\\\\\n]|\\\\[^\\n[:punct:][:word:]]*[\\n[:punct:][:word:]])*)|"
                              // comment: index 2
                              "(//[^\\n]*|/\\*.*?\\*/)|"
                              // literals: index 3
                              "\\<([+-]?(?:(?:0x[[:xdigit:]]+)|(?:(?:[[:digit:]]*\\.)?[[:digit:]]+(?:[eE][+-]?[[:digit:]]+)?))u?(?:(?:int(?:8|16|32|64))|L)?)\\>|"
                              // string literals: index 4
                              "('(?:[^\\\\']|\\\\.)*'|\"(?:[^\\\\\"]|\\\\.)*\")|"
                              // keywords: index 5
                              "\\<(__asm|__cdecl|__declspec|__export|__far16|__fastcall|__fortran|__import"
                              "|__pascal|__rtti|__stdcall|_asm|_cdecl|__except|_export|_far16|_fastcall"
                              "|__finally|_fortran|_import|_pascal|_stdcall|__thread|__try|asm|auto|bool"
                              "|break|case|catch|cdecl|char|class|const|const_cast|continue|default|delete"
                              "|do|double|dynamic_cast|else|enum|explicit|extern|false|float|for|friend|goto"
                              "|if|inline|int|long|mutable|namespace|new|operator|pascal|private|protected"
                              "|public|register|reinterpret_cast|return|short|signed|sizeof|static|static_cast"
                              "|struct|switch|template|this|throw|true|try|typedef|typeid|typename|union|unsigned"
                              "|using|virtual|void|volatile|wchar_t|while)\\>"
                              ;

const char* format_string = "(?1<font color=\"#008040\">$&</font>)"
                            "(?2<I><font color=\"#000080\">$&</font></I>)"
                            "(?3<font color=\"#0000A0\">$&</font>)"
                            "(?4<font color=\"#0000FF\">$&</font>)"
                            "(?5<B>$&</B>)";

const char* header_text = "<HTML>\n<HEAD>\n"
                          "<TITLE>Auto-generated html formated source</TITLE>\n"
                          "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=windows-1252\">\n"
                          "</HEAD>\n"
                          "<BODY LINK=\"#0000ff\" VLINK=\"#800080\" BGCOLOR=\"#ffffff\">\n"
                          "<P> </P>\n<PRE>";

const char* footer_text = "</PRE>\n</BODY>\n\n";

TEST_CASE("example: regex_merge_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
)ex"));
}
}  // namespace ex_regex_merge_example

// libs/regex/example/snippets/regex_replace_example.cpp
namespace ex_regex_replace_example {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_replace_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_replace example:
  *                converts a C++ file to syntax highlighted HTML.
  */


// purpose:
// takes the contents of a file and transform to
// syntax highlighted code in html format

boost::regex e1, e2;
extern const char* expression_text;
extern const char* format_string;
extern const char* pre_expression;
extern const char* pre_format;
extern const char* header_text;
extern const char* footer_text;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, const char** argv)
{
   try{
   e1.assign(expression_text);
   e2.assign(pre_expression);
   for(int i = 1; i < argc; ++i)
   {
      ex_out << "Processing file " << argv[i] << std::endl;
      std::ifstream fs(argv[i]);
      std::string in;
      load_file(in, fs);
      fs.close();
      std::string out_name = std::string(argv[i]) + std::string(".htm");
      std::ofstream os(out_name.c_str());
      os << header_text;
      // strip '<' and '>' first by outputting to a
      // temporary string stream
      std::ostringstream t(std::ios::out | std::ios::binary);
      std::ostream_iterator<char> oi(t);
      boost::regex_replace(oi, in.begin(), in.end(), e2, pre_format, boost::match_default | boost::format_all);
      // then output to final output stream
      // adding syntax highlighting:
      std::string s(t.str());
      std::ostream_iterator<char> out(os);
      boost::regex_replace(out, s.begin(), s.end(), e1, format_string, boost::match_default | boost::format_all);
      os << footer_text;
      os.close();
   }
   }
   catch(...)
   { return -1; }
   return 0;
    return 0;  // main's implicit return
}

const char* pre_expression = "(<)|(>)|(&)|\\r";
const char* pre_format = "(?1&lt;)(?2&gt;)(?3&amp;)";


const char* expression_text = // preprocessor directives: index 1
                              "(^[[:blank:]]*#(?:[^\\\\\\n]|\\\\[^\\n[:punct:][:word:]]*[\\n[:punct:][:word:]])*)|"
                              // comment: index 2
                              "(//[^\\n]*|/\\*.*?\\*/)|"
                              // literals: index 3
                              "\\<([+-]?(?:(?:0x[[:xdigit:]]+)|(?:(?:[[:digit:]]*\\.)?[[:digit:]]+(?:[eE][+-]?[[:digit:]]+)?))u?(?:(?:int(?:8|16|32|64))|L)?)\\>|"
                              // string literals: index 4
                              "('(?:[^\\\\']|\\\\.)*'|\"(?:[^\\\\\"]|\\\\.)*\")|"
                              // keywords: index 5
                              "\\<(__asm|__cdecl|__declspec|__export|__far16|__fastcall|__fortran|__import"
                              "|__pascal|__rtti|__stdcall|_asm|_cdecl|__except|_export|_far16|_fastcall"
                              "|__finally|_fortran|_import|_pascal|_stdcall|__thread|__try|asm|auto|bool"
                              "|break|case|catch|cdecl|char|class|const|const_cast|continue|default|delete"
                              "|do|double|dynamic_cast|else|enum|explicit|extern|false|float|for|friend|goto"
                              "|if|inline|int|long|mutable|namespace|new|operator|pascal|private|protected"
                              "|public|register|reinterpret_cast|return|short|signed|sizeof|static|static_cast"
                              "|struct|switch|template|this|throw|true|try|typedef|typeid|typename|union|unsigned"
                              "|using|virtual|void|volatile|wchar_t|while)\\>"
                              ;

const char* format_string = "(?1<font color=\"#008040\">$&</font>)"
                            "(?2<I><font color=\"#000080\">$&</font></I>)"
                            "(?3<font color=\"#0000A0\">$&</font>)"
                            "(?4<font color=\"#0000FF\">$&</font>)"
                            "(?5<B>$&</B>)";

const char* header_text = "<HTML>\n<HEAD>\n"
                          "<TITLE>Auto-generated html formated source</TITLE>\n"
                          "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=windows-1252\">\n"
                          "</HEAD>\n"
                          "<BODY LINK=\"#0000ff\" VLINK=\"#800080\" BGCOLOR=\"#ffffff\">\n"
                          "<P> </P>\n<PRE>";

const char* footer_text = "</PRE>\n</BODY>\n\n";

TEST_CASE("example: regex_replace_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Processing file boost-m-regex-fixture.cpp
)ex"));
}
}  // namespace ex_regex_replace_example

// libs/regex/example/snippets/regex_split_example_2.cpp
namespace ex_regex_split_example_2 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_split_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_split example: spit out linked URL's.
  */



boost::regex e("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"",
               boost::regex::normal | boost::regbase::icase);

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   //
   // attempt to grow string buffer to match file size,
   // this doesn't always work...
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      // use logarithmic growth stategy, in case
      // in_avail (above) returned zero:
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, char** argv)
{
   std::string s;
   std::list<std::string> l;
   int i;
   for(i = 1; i < argc; ++i)
   {
      ex_out << "Findings URL's in " << argv[i] << ":" << std::endl;
      s.erase();
      std::ifstream is(argv[i]);
      load_file(s, is);
      is.close();
      boost::regex_split(std::back_inserter(l), s, e);
      while(l.size())
      {
         s = *(l.begin());
         l.pop_front();
         ex_out << s << std::endl;
      }
   }
   //
   // alternative method:
   // split one match at a time and output direct to
   // ex_out via ostream_iterator<std::string>....
   //
   for(i = 1; i < argc; ++i)
   {
      ex_out << "Findings URL's in " << argv[i] << ":" << std::endl;
      s.erase();
      std::ifstream is(argv[i]);
      load_file(s, is);
      is.close();
      while(boost::regex_split(std::ostream_iterator<std::string>(ex_out), s, e, boost::match_default, 1)) ex_out << std::endl;
   }

   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_split_example_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Findings URL's in boost-m-regex-fixture.cpp:
Findings URL's in boost-m-regex-fixture.cpp:
)ex"));
}
}  // namespace ex_regex_split_example_2

// libs/regex/example/snippets/regex_token_iterator_eg_2.cpp
namespace ex_regex_token_iterator_eg_2 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_token_iterator_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_token_iterator example: spit out linked URL's.
  */



boost::regex e("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"",
               boost::regex::normal | boost::regbase::icase);

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   //
   // attempt to grow string buffer to match file size,
   // this doesn't always work...
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      // use logarithmic growth stategy, in case
      // in_avail (above) returned zero:
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int example_main(int argc, char** argv)
{
   std::string s;
   int i;
   for(i = 1; i < argc; ++i)
   {
      ex_out << "Findings URL's in " << argv[i] << ":" << std::endl;
      s.erase();
      std::ifstream is(argv[i]);
      load_file(s, is);
      is.close();
      boost::sregex_token_iterator i(s.begin(), s.end(), e, 1);
      boost::sregex_token_iterator j;
      while(i != j)
      {
         ex_out << *i++ << std::endl;
      }
   }
   //
   // alternative method:
   // test the array-literal constructor, and split out the whole
   // match as well as $1....
   //
   for(i = 1; i < argc; ++i)
   {
      ex_out << "Findings URL's in " << argv[i] << ":" << std::endl;
      s.erase();
      std::ifstream is(argv[i]);
      load_file(s, is);
      is.close();
      const int subs[] = {1, 0,};
      boost::sregex_token_iterator i(s.begin(), s.end(), e, subs);
      boost::sregex_token_iterator j;
      while(i != j)
      {
         ex_out << *i++ << std::endl;
      }
   }

   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_token_iterator_eg_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Findings URL's in boost-m-regex-fixture.cpp:
Findings URL's in boost-m-regex-fixture.cpp:
)ex"));
}
}  // namespace ex_regex_token_iterator_eg_2

// libs/regex/example/snippets/partial_regex_grep.cpp: its additions to namespace boost/std, outside the case namespace
namespace std{ using ::memmove; }
// libs/regex/example/snippets/partial_regex_grep.cpp
namespace ex_partial_regex_grep {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         partial_regex_grep.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Search example using partial matches.
  */


#ifdef BOOST_NO_STDC_NAMESPACE

#endif

// match some kind of html tag:
boost::regex e("<[^>]*>");
// count how many:
unsigned int tags = 0;
// saved position of partial match:
const char* next_pos = 0;

bool grep_callback(const boost::match_results<const char*>& m)
{
   if(m[0].matched == false)
   {
      // save position and return:
      next_pos = m[0].first;
   }
   else
      ++tags;
   return true;
}

void search(std::istream& is)
{
   char buf[4096];
   next_pos = buf + sizeof(buf);
   bool have_more = true;
   while(have_more)
   {
      // how much do we copy forward from last try:
      std::ptrdiff_t leftover = (buf + sizeof(buf)) - next_pos;
      // and how much is left to fill:
      std::ptrdiff_t size = next_pos - buf;
      // copy forward whatever we have left:
      std::memmove(buf, next_pos, leftover);
      // fill the rest from the stream:
      is.read(buf + leftover, size);
      std::streamsize read = is.gcount();
      // check to see if we've run out of text:
      have_more = read == size;
      // reset next_pos:
      next_pos = buf + sizeof(buf);
      // and then grep:
      boost::regex_grep<bool(*)(const boost::cmatch&), const char*>(grep_callback,
                        static_cast<const char*>(buf),
                        static_cast<const char*>(buf + read + leftover),
                        e,
                        boost::match_default | boost::match_partial);
   }
}

int example_main(int argc, char* argv[])
{
   if(argc > 1)
   {
      for(int i = 1; i < argc; ++i)
      {
         std::ifstream fs(argv[i]);
         if(fs.bad()) continue;
         search(fs);
         fs.close();
      }
   }
   else
   {
      std::string one("<META NAME=\"keywords\" CONTENT=\"regex++, regular expressions, regular expression library, C++\">");
      std::string what;
      while(what.size() < 10000)
      {
         what.append(one);
         what.append(13, ' ');
      }
      std::stringstream ss;
      ss.str(what);
      search(ss);
   }
   ex_out << "total tag count was " << tags << std::endl;
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: partial_regex_grep") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(total tag count was 2
)ex"));
}
}  // namespace ex_partial_regex_grep

// libs/regex/example/snippets/partial_regex_iterate.cpp: its additions to namespace boost/std, outside the case namespace
namespace std{ using ::memmove; }
// libs/regex/example/snippets/partial_regex_iterate.cpp
namespace ex_partial_regex_iterate {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         partial_regex_iterate.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Search example using partial matches.
  */


#ifdef BOOST_NO_STDC_NAMESPACE

#endif

// match some kind of html tag:
boost::regex e("<[^>]*>");
// count how many:
unsigned int tags = 0;

void search(std::istream& is)
{
   // buffer we'll be searching in:
   char buf[4096];
   // saved position of end of partial match:
   const char* next_pos = buf + sizeof(buf);
   // flag to indicate whether there is more input to come:
   bool have_more = true;

   while(have_more)
   {
      // how much do we copy forward from last try:
      std::ptrdiff_t leftover = (buf + sizeof(buf)) - next_pos;
      // and how much is left to fill:
      std::ptrdiff_t size = next_pos - buf;
      // copy forward whatever we have left:
      std::memmove(buf, next_pos, leftover);
      // fill the rest from the stream:
      is.read(buf + leftover, size);
      std::streamsize read = is.gcount();
      // check to see if we've run out of text:
      have_more = read == size;
      // reset next_pos:
      next_pos = buf + sizeof(buf);
      // and then iterate:
      boost::cregex_iterator a(
         buf, 
         buf + read + leftover, 
         e, 
         boost::match_default | boost::match_partial); 
      boost::cregex_iterator b;

      while(a != b)
      {
         if((*a)[0].matched == false)
         {
            // Partial match, save position and break:
            next_pos = (*a)[0].first;
            break;
         }
         else
         {
            // full match:
            ++tags;
         }
         
         // move to next match:
         ++a;
      }
   }
}

int example_main(int argc, char* argv[])
{
   if(argc > 1)
   {
      for(int i = 1; i < argc; ++i)
      {
         std::ifstream fs(argv[i]);
         if(fs.bad()) continue;
         search(fs);
         fs.close();
      }
   }
   else
   {
      std::string one("<META NAME=\"keywords\" CONTENT=\"regex++, regular expressions, regular expression library, C++\">");
      std::string what;
      while(what.size() < 10000)
      {
         what.append(one);
         what.append(13, ' ');
      }
      std::stringstream ss;
      ss.str(what);
      search(ss);
   }
   ex_out << "total tag count was " << tags << std::endl;
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: partial_regex_iterate") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-fixture.cpp"); f << "#include <vector>\nclass widget\n{\npublic:\n   int x;\n};\ntemplate <class T>\nstruct holder : public widget\n{\n   T value;\n};\nclass gadget;\nclass thing { };\n// trailing comment\nint main() { return 0; }\n"; std::filesystem::current_path(std::filesystem::temp_directory_path()); }
    ex_args.push_back(std::string("boost-m-regex-fixture.cpp"));
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(total tag count was 2
)ex"));
}
}  // namespace ex_partial_regex_iterate

// libs/regex/example/snippets/regex_match_example.cpp
namespace ex_regex_match_example {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_match_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: ftp based regex_match example.
  */
  

using namespace std;
using namespace boost;

regex expression("^([0-9]+)(\\-| |$)(.*)$");

// process_ftp:
// on success returns the ftp response code, and fills
// msg with the ftp response message.
int process_ftp(const char* response, std::string* msg)
{
   cmatch what;
   if(regex_match(response, what, expression))
   {
      // what[0] contains the whole string
      // what[1] contains the response code
      // what[2] contains the separator character
      // what[3] contains the text message.
      if(msg)
         msg->assign(what[3].first, what[3].second);
      return ::atoi(what[1].first);
   }
   // failure did not match
   if(msg)
      msg->erase();
   return -1;
}

#if defined(BOOST_MSVC) || (defined(BOOST_BORLANDC) && (BOOST_BORLANDC == 0x550))
//
// problem with std::getline under MSVC6sp3
istream& getline(istream& is, std::string& s)
{
   s.erase();
   char c = static_cast<char>(is.get());
   while(c != '\n')
   {
      s.append(1, c);
      c = static_cast<char>(is.get());
   }
   return is;
}
#endif

int example_main(int argc, const char*[])
{
   std::string in, out;
   do
   {
      if(argc == 1)
      {
         ex_out << "enter test string" << endl;
         getline(ex_in, in);
         if(in == "quit")
            break;
      }
      else
         in = "100 this is an ftp message text";
      int result;
      result = process_ftp(in.c_str(), &out);
      if(result != -1)
      {
         ex_out << "Match found:" << endl;
         ex_out << "Response code: " << result << endl;
         ex_out << "Message text: " << out << endl;
      }
      else
      {
         ex_out << "Match not found" << endl;
      }
      ex_out << endl;
   } while(argc == 1);
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_match_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(100 this is an ftp message text
500 no
quit
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(enter test string
Match found:
Response code: 100
Message text: this is an ftp message text

enter test string
Match found:
Response code: 500
Message text: no

enter test string
)ex"));
}
}  // namespace ex_regex_match_example

// libs/regex/example/snippets/regex_split_example_1.cpp
namespace ex_regex_split_example_1 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_split_example_1.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_split example: split a string into tokens.
  */




unsigned tokenise(std::list<std::string>& l, std::string& s)
{
   return boost::regex_split(std::back_inserter(l), s);
}

using namespace std;


#if defined(BOOST_MSVC) || (defined(BOOST_BORLANDC) && (BOOST_BORLANDC == 0x550))
//
// problem with std::getline under MSVC6sp3
istream& getline(istream& is, std::string& s)
{
   s.erase();
   char c = static_cast<char>(is.get());
   while(c != '\n')
   {
      s.append(1, c);
      c = static_cast<char>(is.get());
   }
   return is;
}
#endif


int example_main(int argc, const char*[])
{
   string s;
   list<string> l;
   do{
      if(argc == 1)
      {
         ex_out << "Enter text to split (or \"quit\" to exit): ";
         getline(ex_in, s);
         if(s == "quit") break;
      }
      else
         s = "This is a string of tokens";
      unsigned result = tokenise(l, s);
      ex_out << result << " tokens found" << endl;
      ex_out << "The remaining text is: \"" << s << "\"" << endl;
      while(l.size())
      {
         s = *(l.begin());
         l.pop_front();
         ex_out << s << endl;
      }
   }while(argc == 1);
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_split_example_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(This is a string of tokens
quit
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter text to split (or "quit" to exit): 6 tokens found
The remaining text is: ""
This
is
a
string
of
tokens
Enter text to split (or "quit" to exit): )ex"));
}
}  // namespace ex_regex_split_example_1

// libs/regex/example/snippets/regex_token_iterator_eg_1.cpp
namespace ex_regex_token_iterator_eg_1 {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_token_iterator_example_1.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_token_iterator example: split a string into tokens.
  */



using namespace std;


#if defined(BOOST_MSVC) || (defined(BOOST_BORLANDC) && (BOOST_BORLANDC == 0x550))
//
// problem with std::getline under MSVC6sp3
istream& getline(istream& is, std::string& s)
{
   s.erase();
   char c = static_cast<char>(is.get());
   while(c != '\n')
   {
      s.append(1, c);
      c = static_cast<char>(is.get());
   }
   return is;
}
#endif


int example_main(int argc, const char*[])
{
   string s;
   do{
      if(argc == 1)
      {
         ex_out << "Enter text to split (or \"quit\" to exit): ";
         getline(ex_in, s);
         if(s == "quit") break;
      }
      else
         s = "This is a string of tokens";

      boost::regex re("\\s+");
      boost::sregex_token_iterator i(s.begin(), s.end(), re, -1);
      boost::sregex_token_iterator j;

      unsigned count = 0;
      while(i != j)
      {
         ex_out << *i++ << endl;
         count++;
      }
      ex_out << "There were " << count << " tokens found." << endl;

   }while(argc == 1);
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: regex_token_iterator_eg_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(quit
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter text to split (or "quit" to exit): )ex"));
}
}  // namespace ex_regex_token_iterator_eg_1

// libs/regex/example/snippets/credit_card_example.cpp
namespace ex_credit_card_example {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         credit_card_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Credit card number formatting code.
  */


bool validate_card_format(const std::string& s)
{
   static const boost::regex e("(\\d{4}[- ]){3}\\d{4}");
   return boost::regex_match(s, e);
}

const boost::regex e("\\A(\\d{3,4})[- ]?(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})\\z");
const std::string machine_format("\\1\\2\\3\\4");
const std::string human_format("\\1-\\2-\\3-\\4");

std::string machine_readable_card_number(const std::string& s)
{
   return boost::regex_replace(s, e, machine_format, boost::match_default | boost::format_sed);
}

std::string human_readable_card_number(const std::string& s)
{
   return boost::regex_replace(s, e, human_format, boost::match_default | boost::format_sed);
}

using namespace std;

int example_main()
{
   string s[4] = { "0000111122223333", "0000 1111 2222 3333",
                   "0000-1111-2222-3333", "000-1111-2222-3333", };
   int i;
   for(i = 0; i < 4; ++i)
   {
      ex_out << "validate_card_format(\"" << s[i] << "\") returned " << validate_card_format(s[i]) << endl;
   }
   for(i = 0; i < 4; ++i)
   {
      ex_out << "machine_readable_card_number(\"" << s[i] << "\") returned " << machine_readable_card_number(s[i]) << endl;
   }
   for(i = 0; i < 4; ++i)
   {
      ex_out << "human_readable_card_number(\"" << s[i] << "\") returned " << human_readable_card_number(s[i]) << endl;
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: credit_card_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(validate_card_format("0000111122223333") returned 0
validate_card_format("0000 1111 2222 3333") returned 1
validate_card_format("0000-1111-2222-3333") returned 1
validate_card_format("000-1111-2222-3333") returned 0
machine_readable_card_number("0000111122223333") returned 0000111122223333
machine_readable_card_number("0000 1111 2222 3333") returned 0000111122223333
machine_readable_card_number("0000-1111-2222-3333") returned 0000111122223333
machine_readable_card_number("000-1111-2222-3333") returned 000111122223333
human_readable_card_number("0000111122223333") returned 0000-1111-2222-3333
human_readable_card_number("0000 1111 2222 3333") returned 0000-1111-2222-3333
human_readable_card_number("0000-1111-2222-3333") returned 0000-1111-2222-3333
human_readable_card_number("000-1111-2222-3333") returned 000-1111-2222-3333
)ex"));
}
}  // namespace ex_credit_card_example

// libs/regex/example/snippets/partial_regex_match.cpp
namespace ex_partial_regex_match {
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         partial_regex_match.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_match example using partial matches.
  */


boost::regex e("(\\d{3,4})[- ]?(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})");

bool is_possible_card_number(const std::string& input)
{
   //
   // return false for partial match, true for full match, or throw for
   // impossible match based on what we have so far...
   boost::match_results<std::string::const_iterator> what;
   if(0 == boost::regex_match(input, what, e, boost::match_default | boost::match_partial))
   {
      // the input so far could not possibly be valid so reject it:
      throw std::runtime_error("Invalid data entered - this could not possibly be a valid card number");
   }
   // OK so far so good, but have we finished?
   if(what[0].matched)
   {
      // excellent, we have a result:
      return true;
   }
   // what we have so far is only a partial match...
   return false;
}

int example_main(int argc, char* argv[])
{
   try{
      std::string input;
      if(argc > 1)
         input = argv[1];
      else
         ex_in >> input;
      if(is_possible_card_number(input))
      {
         ex_out << "Matched OK..." << std::endl;
      }
      else
         ex_out << "Got a partial match..." << std::endl;
   }
   catch(const std::exception& e)
   {
      ex_out << e.what() << std::endl;
   }
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: partial_regex_match") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(1234-5678-1234-456
1234-5678-12
quit
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Got a partial match...
)ex"));
}
}  // namespace ex_partial_regex_match
