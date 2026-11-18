// Experiment: asio's use_future error path alone. The exception_ptr is made in
// code from the asio module; unless this TU checks a throw of the same type
// itself, its CatchableTypes lack the copy constructor (clice-io/xclang#27).
#pragma once
TEST_CASE("repro: use_future error what") {
    boost::asio::io_context ctx;
    boost::asio::steady_timer t(ctx, std::chrono::hours(1));
    auto f = t.async_wait(boost::asio::use_future);
    t.cancel();
    ctx.run();
    try {
        f.get();
        CHECK(false);
    } catch (const std::exception& e) {
        CHECK(std::string(e.what()).size() > 0);
    }
}
