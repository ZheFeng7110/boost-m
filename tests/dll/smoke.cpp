// mcpplibs.mwrap.boost.dll smoke — shared_library default state (path conversion on
// the MSVC flavor crashes in program_location — upstream issue, see M9 doc), and the
// boost::dll::fs spelling. Upstream that namespace is `using namespace
// boost::filesystem;`, which an importer cannot see; the module re-exports every name
// the filesystem module exports directly in boost::filesystem through it
// (tools/export_policy.toml [[dll.republish]]). The names below go beyond what the
// tests and examples use, including Boost-only ones (unique_path), so a narrowed
// re-export fails here. Filesystem calls use the error_code overloads: nothing throws.
#include "../test_assert.hpp"
import std;
import mcpplibs.mwrap.boost.dll;

namespace fs = boost::dll::fs;

// The same entities as boost::filesystem, not look-alikes.
static_assert(std::is_same_v<fs::path, boost::filesystem::path>);
static_assert(std::is_same_v<fs::directory_entry, boost::filesystem::directory_entry>);
static_assert(std::is_same_v<fs::recursive_directory_iterator,
                             boost::filesystem::recursive_directory_iterator>);
static_assert(std::is_same_v<fs::file_status, boost::filesystem::file_status>);
static_assert(std::is_same_v<fs::filesystem_error, boost::filesystem::filesystem_error>);
static_assert(std::is_same_v<fs::space_info, boost::filesystem::space_info>);
static_assert(std::is_same_v<fs::ofstream, boost::filesystem::ofstream>);
// error_code / system_error are using-declarations in boost/dll/config.hpp.
static_assert(std::is_same_v<fs::error_code, boost::system::error_code>);
static_assert(std::is_same_v<fs::system_error, boost::system::system_error>);
// The API spells its paths dll::fs::path.
static_assert(std::is_same_v<decltype(boost::dll::program_location()), fs::path>);
// Enumerators of the unscoped file_type / perms reach dll::fs like any other name.
static_assert(fs::regular_file == boost::filesystem::regular_file);
static_assert(fs::owner_read == boost::filesystem::owner_read);
// Functions outside the documented subset, unevaluated (unique_path needs the
// platform's random source; system_complete / portable_name are Boost-only).
static_assert(std::is_same_v<decltype(fs::unique_path()), fs::path>);
static_assert(std::is_same_v<decltype(fs::system_complete(std::declval<const fs::path&>())),
                             fs::path>);
static_assert(std::is_same_v<decltype(fs::portable_name(std::string())), bool>);
static_assert(std::is_same_v<decltype(fs::symlink_status(std::declval<const fs::path&>())),
                             fs::file_status>);

namespace {

void shared_library_default_state() {
    boost::dll::shared_library lib;
    assert(!lib.is_loaded());
    assert(!lib);
}

void filesystem_through_dll_fs() {
    fs::error_code ec;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path dir = fs::temp_directory_path(ec) / ("boost-m-dll-fs-" + std::to_string(stamp));
    assert(!ec);
    assert(fs::create_directories(dir / "sub", ec) && !ec);
    assert(fs::is_directory(dir, ec));

    const fs::path file = dir / "sub" / "a.txt";
    {
        fs::ofstream out(file);
        out << "abcdef";
    }
    assert(fs::exists(file, ec));
    assert(fs::is_regular_file(fs::symlink_status(file, ec)));
    assert(fs::status_known(fs::status(file, ec)));
    assert(fs::file_size(file, ec) == 6 && !ec);
    fs::resize_file(file, 2, ec);
    assert(!ec && fs::file_size(file, ec) == 2);
    assert(fs::hard_link_count(file, ec) == 1 && !ec);
    assert((fs::status(file, ec).permissions() & fs::owner_read) != fs::no_perms);
    assert(fs::space(dir, ec).capacity > 0 && !ec);

    fs::copy_file(file, dir / "b.txt", ec);
    assert(!ec);
    int files = 0;
    for (fs::recursive_directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
        files += fs::is_regular_file(it->status()) ? 1 : 0;
    assert(!ec && files == 2);

    fs::file_size(dir / "missing", ec);
    assert(ec);
    assert(fs::remove_all(dir, ec) == 4 && !ec);
    assert(!fs::exists(dir, ec));
}

}  // namespace

int main() {
    shared_library_default_state();
    filesystem_through_dll_fs();
    return 0;
}
