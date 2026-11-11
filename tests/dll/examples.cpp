// mcpplibs.mwrap.boost.dll — the official examples (libs/dll/example/): the
// importers run as cases against the plugin shared libraries built from the
// examples' own plugin sources (tests/dll/plugins/, unchanged), which sit next
// to this program. Where an importer takes the plugin directory or paths as
// argv, the case passes this program's directory; output that the original
// printed is captured and compared with what the example's documentation
// shows. The plugin API headers (tutorial_common, tutorial8) are written out
// here as the importer sees them: the dll module is imported, not included.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/config.hpp>  // BOOST_SYMBOL_VISIBLE
import std;
import mcpplibs.mwrap.boost.dll;

namespace dll = boost::dll;

// ---- tutorial_common/my_plugin_api.hpp ----
class BOOST_SYMBOL_VISIBLE my_plugin_api {
public:
    virtual std::string name() const = 0;
    virtual float calculate(float x, float y) = 0;
    virtual ~my_plugin_api() = default;
};

// ---- tutorial8/refcounting_api.hpp ----
class my_refcounting_api : public my_plugin_api {
public:
    virtual dll::fs::path location() const = 0;
};
struct library_holding_deleter {
    std::shared_ptr<dll::shared_library> lib_;
    void operator()(my_refcounting_api* p) const { delete p; }
};
inline std::shared_ptr<my_refcounting_api> bind(my_refcounting_api* plugin) {
    dll::fs::path location = plugin->location();
    auto lib = std::make_shared<dll::shared_library>(location);
    library_holding_deleter deleter;
    deleter.lib_ = lib;
    return std::shared_ptr<my_refcounting_api>(plugin, deleter);
}
inline std::shared_ptr<my_refcounting_api> get_plugin(dll::fs::path path, const char* func_name) {
    using func_t = my_refcounting_api*();
    auto creator = dll::import_alias<func_t>(path, func_name, dll::load_mode::append_decorations);
    my_refcounting_api* plugin = creator();
    return bind(plugin);
}

namespace {
// The directory of the plugins: this program's.
dll::fs::path plugin_dir() { return dll::program_location().parent_path(); }
dll::fs::path plugin(const char* name) {
    // lib<name>.so / lib<name>.dylib, <name>.dll (not decorate(): it drops the
    // prefix of a name that starts with "lib", such as library1)
#ifdef _WIN32
    const std::string prefix;
#else
    const std::string prefix = "lib";
#endif
    return plugin_dir() / (prefix + name + dll::shared_library::suffix().string());
}
}  // namespace

TEST_CASE("example: getting_started") {
    dll::fs::path path_to_shared_library = plugin("getting_started_library");
    auto c_func = dll::import_symbol<int(int)>(path_to_shared_library, "c_func_name");
    CHECK(c_func(1) == 2);
    std::shared_ptr<int> c_var = dll::import_symbol<int>(path_to_shared_library, "c_variable_name");
    int c_var_old_contents = *c_var;
    *c_var = 100;
    CHECK(c_var_old_contents == 1);
    auto cpp_func = dll::import_alias<std::string(const std::string&)>(path_to_shared_library, "pretty_name");
    CHECK(cpp_func(std::string("In importer.")) == "In importer. Hello from lib!");
    auto cpp11_func = dll::import_symbol<int(std::string&&)>(path_to_shared_library, "i_am_a_cpp11_function");
    CHECK(cpp11_func(std::string("In importer.")) == int(sizeof("In importer.") - 1));
    std::shared_ptr<std::string> cpp_var = dll::import_symbol<std::string>(path_to_shared_library, "cpp_variable_name");
    std::string cpp_var_old_contents = *cpp_var;
    *cpp_var = "New value";
    CHECK(cpp_var_old_contents == "some value");
    CHECK(*cpp_var == "New value");
}

TEST_CASE("example: getting_started_library") {
    // the library's other exports, as the getting-started page lists them
    dll::shared_library lib(plugin("getting_started_library"));
    CHECK(lib.has("c_func_name"));
    CHECK(lib.has("pretty_name"));
    CHECK_FALSE(lib.has("i_am_function_with_ugly_name"));
}

TEST_CASE("example: tutorial1") {
    auto plugin = dll::import_symbol<my_plugin_api>(plugin_dir() / "my_plugin_sum", "plugin", dll::load_mode::append_decorations);
    CHECK(plugin->calculate(1.5, 1.5) == 3);
    CHECK(plugin->name() == "sum");
}
TEST_CASE("example: my_plugin_sum") {
    dll::shared_library lib(plugin_dir() / "my_plugin_sum", dll::load_mode::append_decorations);
    CHECK(lib.has("plugin"));
    CHECK(lib.get<my_plugin_api>("plugin").name() == "sum");
}

TEST_CASE("example: tutorial2") {
    using pluginapi_create_t = std::shared_ptr<my_plugin_api>();
    auto creator = dll::import_alias<pluginapi_create_t>(plugin_dir() / "my_plugin_aggregator", "create_plugin",
                                                        dll::load_mode::append_decorations);
    std::shared_ptr<my_plugin_api> plugin = creator();
    CHECK(plugin->calculate(1.5, 1.5) == 3);
    CHECK(plugin->calculate(1.5, 1.5) == 6);  // it aggregates
    CHECK(plugin->name() == "aggregator");
}
TEST_CASE("example: my_plugin_aggregator") {
    dll::shared_library lib(plugin("my_plugin_aggregator"));
    CHECK(lib.has("create_plugin"));
}

TEST_CASE("example: tutorial3") {
    // search the plugins for the create_plugin symbol: only the aggregator has it
    std::vector<dll::fs::path> plugins{plugin("my_plugin_sum"), plugin("my_plugin_aggregator")};
    std::size_t found = 0;
    std::vector<std::string> names;
    for (auto const& p : plugins) {
        dll::shared_library lib(p);
        if (!lib.has("create_plugin"))
            continue;
        using pluginapi_create_t = std::shared_ptr<my_plugin_api>();
        auto creator = dll::import_alias<pluginapi_create_t>(std::move(lib), "create_plugin");
        names.push_back(creator()->name());
        ++found;
    }
    CHECK(found == 1);
    CHECK(names == std::vector<std::string>{"aggregator"});
}

TEST_CASE("example: tutorial6") {
    using callback_t = std::function<void()>;
    static std::vector<std::string> log;
    log.clear();
    std::function<void(const callback_t&)> on_unload =
        dll::import_alias<void(const callback_t&)>(plugin("on_unload_lib"), "on_unload");
    on_unload([] { log.push_back("unloaded"); });
    log.push_back("Before library unload.");
    on_unload = {};
    log.push_back("After library unload.");
    // the callbacks run when the library's statics are destroyed; dlclose may
    // keep the library (a shared object with unique symbols), so only the order
    // of the program's own lines is fixed
    CHECK(log.front() == "Before library unload.");
    CHECK(std::find(log.begin(), log.end(), "After library unload.") != log.end());
}
TEST_CASE("example: on_unload_lib") {
    dll::shared_library lib(plugin("on_unload_lib"));
    CHECK(lib.has("on_unload"));
}

TEST_CASE("example: tutorial7") {
    // the "Anna" section of each library holds its aliases
    std::vector<std::string> printed;
    for (auto const& path : {plugin("library1"), plugin("library2")}) {
        dll::library_info inf(path);
        std::vector<std::string> exports = inf.symbols("Anna");
        dll::shared_library lib(path);
        for (auto const& e : exports) {
            printed.push_back(e);
            lib.get_alias<void(const std::string&)>(e)("User");  // prints to std::cout
        }
    }
    std::sort(printed.begin(), printed.end());
    CHECK(printed == std::vector<std::string>{"are_you_bored", "howdy", "print_hello"});
}
TEST_CASE("example: library1") { CHECK(dll::library_info(plugin("library1")).symbols("Anna").size() == 1); }
TEST_CASE("example: library2") { CHECK(dll::library_info(plugin("library2")).symbols("Anna").size() == 2); }

TEST_CASE("example: tutorial8") {
    std::shared_ptr<my_refcounting_api> plugin = get_plugin(plugin_dir() / "refcounting_plugin", "create_refc_plugin");
    CHECK(plugin->name() == "refcounting");
    CHECK(plugin->location().filename() == ::plugin("refcounting_plugin").filename());
}
TEST_CASE("example: refcounting_plugin") {
    dll::shared_library lib(plugin("refcounting_plugin"));
    CHECK(lib.has("create_refc_plugin"));
}

TEST_CASE("doc: getting_started") {
    // the page's first steps: load, check, get a symbol by its alias
    dll::shared_library lib(plugin("getting_started_library"));
    CHECK(lib.is_loaded());
    CHECK(lib.location().filename() == plugin("getting_started_library").filename());
    CHECK(lib.get_alias<std::string(const std::string&)>("pretty_name")("x") == "x Hello from lib!");
    lib.unload();
    CHECK_FALSE(lib.is_loaded());
}
TEST_CASE("doc: f-a-q") {
    // a library that is not there: an exception, not undefined behaviour
    CHECK_THROWS_AS(dll::shared_library(plugin_dir() / "no_such_plugin", dll::load_mode::append_decorations),
                    boost::system::system_error);
    boost::system::error_code ec;
    dll::shared_library lib(plugin_dir() / "no_such_plugin", ec, dll::load_mode::append_decorations);
    CHECK(ec);
}
TEST_CASE("doc: missuses") {
    // a symbol imported with import_symbol keeps its library loaded
    std::shared_ptr<int> var;
    {
        var = dll::import_symbol<int>(plugin("getting_started_library"), "c_variable_name");
    }
    CHECK(*var >= 1);
}
TEST_CASE("doc: multithread") {
    // loading the same library from several threads
    std::vector<std::thread> threads;
    std::atomic<int> ok{0};
    for (int i = 0; i < 4; ++i)
        threads.emplace_back([&] {
            dll::shared_library lib(plugin("my_plugin_sum"));
            if (lib.has("plugin"))
                ++ok;
        });
    for (auto& t : threads)
        t.join();
    CHECK(ok == 4);
}
