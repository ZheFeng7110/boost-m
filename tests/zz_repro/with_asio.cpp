// Experiment: the std cases with `import std` and the asio module, plus the
// asio pieces composed_4 uses (use_future, an error stored in the promise).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.asio;

namespace asio = boost::asio;
using namespace std::chrono_literals;

#include "cases.hpp"

TEST_CASE("repro: boost system_error exception_ptr") {
    auto p = std::make_exception_ptr(boost::system::system_error(asio::error::invalid_argument));
    CHECK_THROWS_AS(std::rethrow_exception(p), boost::system::system_error);
}
TEST_CASE("repro: throw boost system_error") {
    try {
        throw boost::system::system_error(asio::error::invalid_argument);
    } catch (const std::exception& e) {
        CHECK(std::string(e.what()).size() > 0);
    }
}
TEST_CASE("repro: asio post") {
    asio::io_context ctx;
    int ran = 0;
    asio::post(ctx, [&] { ++ran; });
    ctx.run();
    CHECK(ran == 1);
}
TEST_CASE("repro: asio use_future value") {
    asio::io_context ctx;
    auto f = asio::post(ctx, asio::use_future([] { return 7; }));
    ctx.run();
    CHECK(f.get() == 7);
}
TEST_CASE("repro: asio use_future error") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1h);
    auto f = t.async_wait(asio::use_future);
    t.cancel();
    ctx.run();
    CHECK_THROWS_AS(f.get(), boost::system::system_error);
}
TEST_CASE("repro: asio post bind error") {
    asio::io_context ctx;
    boost::system::error_code got;
    asio::post(ctx, std::bind([&](const boost::system::error_code& ec) { got = ec; },
                              boost::system::error_code(asio::error::invalid_argument)));
    ctx.run();
    CHECK(got == asio::error::invalid_argument);
}
