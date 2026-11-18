// Experiment: std::async alone (the ambient scheduler of MSVC's ppltasks).
#pragma once
TEST_CASE("repro: async") {
    auto f = std::async(std::launch::async, [] { return 42; });
    CHECK(f.get() == 42);
}
