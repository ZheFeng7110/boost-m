// mcpplibs.mwrap.boost.dll — user types across the import boundary: a user
// object and a user function exported by alias (BOOST_DLL_ALIAS) and by a user
// section (BOOST_DLL_ALIAS_SECTIONED), imported back through import_symbol /
// import_alias / shared_library::get_alias; a factory returning a user
// interface whose implementation lives behind the boundary, kept alive by the
// library it came from; and library_info listing the user section. The
// exports are this program's own (-rdynamic, tests/dll/mcpp.toml), with the
// macro header included next to the module import (design §4.4).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/config.hpp>     // BOOST_SYMBOL_VISIBLE
#include <boost/dll/alias.hpp>  // BOOST_DLL_ALIAS, BOOST_DLL_ALIAS_SECTIONED
import std;
import mcpplibs.mwrap.boost.dll;

namespace dll = boost::dll;

namespace app {
struct settings {
    int level = 3;
    std::string label = "user settings";
};
settings exported_settings;

class BOOST_SYMBOL_VISIBLE shape {
public:
    virtual double area() const = 0;
    virtual std::string kind() const = 0;
    virtual ~shape() = default;
};
inline int live_shapes = 0;
class square final : public shape {
    double side_;

public:
    explicit square(double s) : side_(s) { ++live_shapes; }
    ~square() override { --live_shapes; }
    double area() const override { return side_ * side_; }
    std::string kind() const override { return "square"; }
};
std::unique_ptr<shape> make_square(double side) { return std::make_unique<square>(side); }

std::string greet(const std::string& who) { return "hello, " + who; }
int twice(int x) { return 2 * x; }
}  // namespace app

BOOST_DLL_ALIAS(app::exported_settings, user_settings)
BOOST_DLL_ALIAS(app::make_square, user_make_square)
BOOST_DLL_ALIAS_SECTIONED(app::greet, user_greet, UserSect)
BOOST_DLL_ALIAS_SECTIONED(app::twice, user_twice, UserSect)

namespace {
dll::fs::path self() { return dll::program_location(); }
}  // namespace

TEST_CASE("custom: a user object imported by alias") {
    auto s = dll::import_alias<app::settings>(self(), "user_settings");
    CHECK(s->level == 3);
    CHECK(s->label == "user settings");
    s->level = 7;  // the same object: the import is a reference into the program
    CHECK(app::exported_settings.level == 7);
}

TEST_CASE("custom: a factory of a user interface") {
    using factory_t = std::unique_ptr<app::shape>(double);
    std::function<factory_t> make = dll::import_alias<factory_t>(self(), "user_make_square");
    {
        std::unique_ptr<app::shape> sq = make(3);
        CHECK(sq->kind() == "square");
        CHECK(sq->area() == 9);
        CHECK(app::live_shapes == 1);
    }
    CHECK(app::live_shapes == 0);
    // the same through a shared_library the caller owns
    dll::shared_library lib(self());
    auto& f = lib.get_alias<factory_t>("user_make_square");
    CHECK(f(2)->area() == 4);
}

TEST_CASE("custom: user section") {
    dll::library_info info(self());
    std::vector<std::string> names = info.symbols("UserSect");
    std::sort(names.begin(), names.end());
    CHECK(names == std::vector<std::string>{"user_greet", "user_twice"});
    dll::shared_library lib(self());
    CHECK(lib.get_alias<std::string(const std::string&)>("user_greet")("module") == "hello, module");
    CHECK(lib.get_alias<int(int)>("user_twice")(21) == 42);
}

TEST_CASE("custom: an imported user callable keeps its library") {
    std::function<int(int)> f;
    {
        auto imported = dll::import_alias<int(int)>(self(), "user_twice");
        f = imported;  // the boost::dll import holds the library; std::function holds the import
    }
    CHECK(f(5) == 10);
}
