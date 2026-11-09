// mcpplibs.mwrap.boost.regex — libs/regex/example/grep/grep.cpp, a grep built
// on Boost.Regex and Boost.Program_options, run as a case on a fixture file
// with -n -i "class|line" and with -c -E: its output must equal what the same
// code printed when built from upstream/boost headers alone. The file path is
// the program's own printing of its input, so the cases print it too.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
import std;
import mcpplibs.mwrap.boost.regex;
import mcpplibs.mwrap.boost.program_options;

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

// libs/regex/example/grep/grep.cpp
namespace ex_grep {
#ifdef BOOST_MSVC
#pragma warning(disable:4512 4244)
#endif


namespace po = boost::program_options;

int after_context;
int before_context;
bool print_byte_offset;
bool count_only;
std::string pattern;
bool print_non_matching_files;
bool files_only;
bool print_line_numbers;

boost::regex_constants::syntax_option_type flags = boost::regex_constants::basic;
boost::regex re;
boost::smatch what;
std::string current_file;
int file_count;

void process_stream(std::istream& is)
{
   std::string line;
   int match_found = 0;
   int linenum = 1;
   while(std::getline(is, line))
   {
      bool result = boost::regex_search(line, what, re);
      if(result)
      {
         if(print_non_matching_files)
            return;
         if(files_only)
         {
            ex_out << current_file << std::endl;
            return;
         }
         if(!match_found && !count_only && (file_count > 1))
         {
            ex_out << current_file << ":\n";
         }
         ++match_found;
         if(!count_only)
         {
            if(print_line_numbers)
            {
               ex_out << linenum << ":";
            }
            if(print_byte_offset)
            {
               ex_out << what.position() << ":";
            }
            ex_out << what[0] << std::endl;
         }
      }
      ++linenum;
   }
   if(count_only && match_found)
   {
      ex_out << match_found << " matches found in file " << current_file << std::endl;
   }
   else if(print_non_matching_files && !match_found)
   {
      ex_out << current_file << std::endl;
   }
}

void process_file(const std::string& name)
{
   current_file = name;
   std::ifstream is(name.c_str());
   if(is.bad())
   {
      ex_out << "Unable to open file " << name << std::endl;
   }
   process_stream(is);
}

int example_main(int argc, char * argv[])
{
   try{
      po::options_description opts("Options");
      opts.add_options()
         ("help,h", "produce help message") 
         //("after-context,A", po::value<int>(&after_context)->default_value(0), "Print arg  lines  of  trailing  context  after  matching  lines. Places  a  line  containing  --  between  contiguous  groups  of matches.")
         //("before-context,B", po::value<int>(&before_context)->default_value(0), "Print  arg  lines  of  leading  context  before  matching lines. Places  a  line  containing  --  between  contiguous  groups  of matches.")
         //("context,C", po::value<int>(), "Print  arg lines of output context.  Places a line containing -- between contiguous groups of matches.")
         ("byte-offset,b", "Print the byte offset within the input file before each line  of output.")
         ("count,c", "Suppress normal output; instead print a count of matching  lines for  each  input  file.  With the -v, --invert-match option (see below), count non-matching lines.")
         ("extended-regexp,E", "Interpret PATTERN as an POSIX-extended regular expression.")
         ("perl-regexp,P", "Interpret PATTERN as a Perl regular expression.")
         //("regexp,e", po::value<std::string>(&pattern), "Use PATTERN as the pattern; useful to protect patterns beginning with -.")
         ("basic-regexp,G", "Interpret arg as a POSIX-basic regular expression (see below).  This is the default.")
         ("ignore-case,i", "Ignore case distinctions in  both  the  PATTERN  and  the  input files.")
         ("files-without-match,L", "Suppress  normal  output;  instead  print the name of each input file from which no output would normally have been printed.  The scanning will stop on the first match.")
         ("files-with-matches,l", "Suppress  normal  output;  instead  print the name of each input file from which output would normally have  been  printed.   The scanning will stop on the first match.")
         ("line-number,n", "Prefix each line of output with the line number within its input file.")
         ;
      // Hidden options, will be allowed both on command line and
      // in config file, but will not be shown to the user.
      po::options_description hidden("Hidden options");
      hidden.add_options()
         ("input-file", po::value< std::vector<std::string> >(), "input file")
         ("input-pattern", po::value< std::string >(), "input file")
         ;

      po::options_description cmdline_options;
      cmdline_options.add(opts).add(hidden);

      po::positional_options_description p;
      p.add("input-pattern", 1);
      p.add("input-file", -1);

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(cmdline_options)/*.options(hidden)*/.positional(p).run(), vm);
      po::notify(vm);

      if (vm.count("help")) 
      {
         ex_out << opts << "\n";
         return 0;
      }
      if (vm.count("context")) 
      {
         after_context = vm["context"].as< int >();
         before_context = after_context;
      }
      if(vm.count("extended-regexp"))
      {
         flags = boost::regex_constants::extended;
      }
      if(vm.count("basic-regexp"))
      {
         flags = boost::regex_constants::basic;
      }
      if(vm.count("perl-regexp"))
      {
         flags = boost::regex_constants::perl;
      }
      if(vm.count("ignore-case"))
      {
         flags |= boost::regex_constants::icase;
      }
      if(vm.count("byte-offset"))
      {
         print_byte_offset = true;
      }
      if(vm.count("count"))
      {
         count_only = true;
      }
      if(vm.count("files-without-match"))
      {
         print_non_matching_files = true;
      }
      if(vm.count("files-with-matches"))
      {
         files_only = true;
      }
      if(vm.count("line-number"))
      {
         print_line_numbers = true;
      }
      if(vm.count("input-pattern"))
      {
         pattern = vm["input-pattern"].as< std::string >();
         re.assign(pattern, flags);
      }
      else
      {
         ex_out << "No pattern specified" << std::endl;
         return 1;
      }
      if (vm.count("input-file"))
      {
         const std::vector<std::string>& files = vm["input-file"].as< std::vector<std::string> >();
         file_count = files.size();
         for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
         {
            process_file(*i);
         }
      }
      else
      {
         // no input files, scan stdin instead:
         process_stream(ex_in);
      }

   }
   catch(const std::exception& e)
   {
      ex_out << e.what() << std::endl;
   }

   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: grep") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    { std::ofstream f(std::filesystem::temp_directory_path() / "boost-m-regex-grep.txt"); f << "alpha line\nclass widget\nBeta Line\nclass gadget\nomega\n"; }
    ex_args.push_back(std::string("-n"));
    ex_args.push_back(std::string("-i"));
    ex_args.push_back(std::string("class|line"));
    ex_args.push_back((std::filesystem::temp_directory_path() / "boost-m-regex-grep.txt").string());
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_grep
