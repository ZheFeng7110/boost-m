// Experiment: the std cases with textual standard headers only (no import).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <exception>
#include <future>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

#include "cases.hpp"

#if defined(_MSC_VER)
#include <ppltasks.h>
TEST_CASE("repro: ambient scheduler") {
    const auto& s = Concurrency::get_ambient_scheduler();
    std::fprintf(stderr, "ambient scheduler at %p, holds %p\n", (const void*)&s, (const void*)s.get());
    CHECK(&s != nullptr);
}
#endif
