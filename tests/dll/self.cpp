// mcpplibs.mwrap.boost.dll — the official examples that import from the
// program itself: tutorial4 (load_self with static_plugin linked in),
// tutorial5 (load_all: the plugins next to this program and the program) and
// tutorial9 (a Windows system library; the original does nothing elsewhere).
// tutorial8_static links the refcounting plugin into its importer, whose
// location() calls this_line_location(): a static (TU-local) function by
// design, so not exported (coverage.toml, gap). BOOST_DLL_ALIAS
// is a macro, so its header is included next to the module import (design
// §4.4); the program exports its symbols (-rdynamic, tests/dll/mcpp.toml) for
// the dynamic loader to find them.
#define BOOST_DLL_FORCE_ALIAS_INSTANTIATION
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/config.hpp>     // BOOST_SYMBOL_VISIBLE
#include <boost/dll/alias.hpp>  // BOOST_DLL_ALIAS
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

// ---- tutorial4/static_plugin.hpp, static_plugin.cpp ----
namespace my_namespace {
std::shared_ptr<my_plugin_api> create_plugin();
}  // namespace my_namespace
BOOST_DLL_ALIAS(my_namespace::create_plugin, create_plugin)

namespace my_namespace {
class my_plugin_static final : public my_plugin_api {
public:
    std::string name() const override { return "static"; }
    float calculate(float x, float y) override { return x - y; }
};
std::shared_ptr<my_plugin_api> create_plugin() { return std::make_shared<my_plugin_static>(); }
}  // namespace my_namespace

TEST_CASE("example: load_self") {
    dll::shared_library self(dll::program_location());
    auto creator = self.get_alias<std::shared_ptr<my_plugin_api>()>("create_plugin");
    CHECK(creator()->calculate(2, 2) == 0);
    float res = creator()->calculate(10, 10);
    CHECK((res > -0.0001 && res < 0.00001));
}
TEST_CASE("example: static_plugin") {
    CHECK(dll::shared_library(dll::program_location()).has("create_plugin"));
    CHECK(my_namespace::create_plugin()->name() == "static");
}

namespace {
// tutorial5/load_all.cpp: plugins_collector, unchanged but for the filter
// that stands in for b2_workarounds::is_shared_library
class plugins_collector {
    using plugins_t = std::map<std::string, dll::shared_library>;
    dll::fs::path plugins_directory_;
    plugins_t plugins_;
    void load_all();
    void insert_plugin(dll::shared_library&& lib);

public:
    plugins_collector(const dll::fs::path& plugins_directory) : plugins_directory_(plugins_directory) { load_all(); }
    std::vector<std::string> names() const {
        std::vector<std::string> r;
        for (auto const& p : plugins_)
            r.push_back(p.first);
        return r;
    }
    std::size_t count() const { return plugins_.size(); }
};
void plugins_collector::load_all() {
    namespace fs = ::boost::dll::fs;
    const auto extension = dll::shared_library::suffix().native();
    fs::recursive_directory_iterator endit;
    for (fs::recursive_directory_iterator it(plugins_directory_); it != endit; ++it) {
        if (!fs::is_regular_file(*it))
            continue;
        if (it->path().extension().native() != extension)
            continue;
        std::error_code error;
        dll::shared_library plugin(it->path(), error);
        if (error)
            continue;
        insert_plugin(std::move(plugin));
    }
    dll::shared_library plugin(dll::program_location());
    insert_plugin(std::move(plugin));
}
void plugins_collector::insert_plugin(dll::shared_library&& lib) {
    std::string plugin_name;
    if (lib.has("create_plugin")) {
        plugin_name = lib.get_alias<std::shared_ptr<my_plugin_api>()>("create_plugin")()->name();
    } else if (lib.has("plugin")) {
        plugin_name = lib.get<my_plugin_api>("plugin").name();
    } else {
        return;
    }
    if (plugins_.find(plugin_name) == plugins_.cend())
        plugins_[plugin_name] = std::move(lib);
}
}  // namespace

TEST_CASE("example: load_all") {
    plugins_collector plugins(dll::program_location().parent_path());
    CHECK(plugins.count() >= 3);
    // my_plugin_sum ("plugin"), my_plugin_aggregator ("create_plugin") and this program
    std::vector<std::string> names = plugins.names();
    for (const char* n : {"aggregator", "static", "sum"})
        CHECK(std::find(names.begin(), names.end(), n) != names.end());
}

TEST_CASE("example: tutorial9") {
#ifdef _WIN32
    // GetStdHandle through its signature with the calling convention, without
    // <windows.h>: HANDLE is void*, DWORD unsigned long, STD_OUTPUT_HANDLE -11
    using GetStdHandle_t = void*(__stdcall)(unsigned long);
    auto get_std_handle =
        dll::import_symbol<GetStdHandle_t>("Kernel32.dll", "GetStdHandle", dll::load_mode::search_system_folders);
    void* h0 = get_std_handle(static_cast<unsigned long>(-11));
    std::function<void*(unsigned long)> get_std_handle2 = get_std_handle;
    CHECK(get_std_handle2(static_cast<unsigned long>(-11)) == h0);
    dll::shared_library lib("Kernel32.dll", dll::load_mode::search_system_folders);
    GetStdHandle_t& func = lib.get<GetStdHandle_t>("GetStdHandle");
    CHECK(func(static_cast<unsigned long>(-11)) == h0);
#else
    // the original's main returns 0 outside Windows
    CHECK(true);
#endif
}
