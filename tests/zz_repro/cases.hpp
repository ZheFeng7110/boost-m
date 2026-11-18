// Experiment (windows-llvm-msvc crashes): std-only cases shared by the repro
// programs; each program provides the standard library its own way (import
// std, or textual includes) before including this. Not part of the suite.
#pragma once

TEST_CASE("repro: async") {
    auto f = std::async(std::launch::async, [] { return 42; });
    CHECK(f.get() == 42);
}
TEST_CASE("repro: throw system_error") {
    try {
        throw std::system_error(std::make_error_code(std::errc::invalid_argument), std::string(64, 'x'));
    } catch (const std::exception& e) {
        CHECK(std::string(e.what()).size() > 64);
    }
}
TEST_CASE("repro: exception_ptr rethrow") {
    auto p = std::make_exception_ptr(std::runtime_error(std::string(100, 'x')));
    try {
        std::rethrow_exception(p);
    } catch (const std::exception& e) {
        CHECK(std::string(e.what()).size() == 100);
    }
}
TEST_CASE("repro: exception_ptr copy") {
    std::exception_ptr q;
    {
        auto p = std::make_exception_ptr(std::system_error(std::make_error_code(std::errc::invalid_argument)));
        q = p;
    }
    CHECK_THROWS_AS(std::rethrow_exception(q), std::system_error);
}
TEST_CASE("repro: promise set_exception") {
    std::promise<void> pr;
    auto f = pr.get_future();
    pr.set_exception(std::make_exception_ptr(std::system_error(std::make_error_code(std::errc::invalid_argument))));
    try {
        f.get();
        CHECK(false);
    } catch (const std::exception& e) {
        CHECK(std::string(e.what()).size() > 0);
    }
}
TEST_CASE("repro: promise value") {
    std::promise<std::string> pr;
    auto f = pr.get_future();
    pr.set_value(std::string(100, 'y'));
    CHECK(f.get().size() == 100);
}
TEST_CASE("repro: thread") {
    std::string s;
    std::thread t([&] { s.assign(100, 'z'); });
    t.join();
    CHECK(s.size() == 100);
}

// make_exception_ptr copies the object into its storage through the ThrowInfo's
// copy function; a missing copy function means a bitwise copy whose what()
// buffer the by-value parameter frees on return (use-after-free / double free).
template <class E>
void repro_stored_copy(E orig) {
    auto p = std::make_exception_ptr(orig);
    const char* w = nullptr;
    std::string text;
    try {
        std::rethrow_exception(p);
    } catch (const std::exception& e) {
        w = e.what();
        text = w;
    }
    std::fprintf(stderr, "orig.what() %p, rethrown what() %p, %zu chars\n", (const void*)orig.what(), (const void*)w,
                 text.size());
    CHECK(text.find(std::string(100, 'x')) != std::string::npos);
}
TEST_CASE("repro: stored copy runtime_error") { repro_stored_copy(std::runtime_error(std::string(100, 'x'))); }
TEST_CASE("repro: stored copy system_error") {
    repro_stored_copy(std::system_error(std::make_error_code(std::errc::invalid_argument), std::string(100, 'x')));
}
