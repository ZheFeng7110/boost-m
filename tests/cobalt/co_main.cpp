// mcpplibs.mwrap.boost.cobalt — a program written with co_main.
// The module leaves out <boost/cobalt/main.hpp>, which defines the program's
// main (tools/export_policy.toml [cobalt]); a co_main program includes that
// header itself, before the import (docs/usage.md §2.2). Its main runs co_main
// on a fresh io_context and returns co_main's result.
#include <boost/cobalt/main.hpp>
#include "../test_assert.hpp"
import mcpplibs.mwrap.boost.cobalt;

namespace {
boost::cobalt::task<int> answer() { co_return 42; }

boost::cobalt::promise<void> producer(boost::cobalt::channel<int>& chan) {
    for (int i = 1; i <= 3; ++i)
        co_await chan.write(i);
    chan.close();
}
}  // namespace

boost::cobalt::main co_main(int argc, char* argv[]) {
    assert(argc >= 1 && argv[0] != nullptr);
    assert(co_await answer() == 42);

    boost::cobalt::channel<int> chan;
    auto p = producer(chan);
    int sum = 0;
    while (chan.is_open())
        sum += co_await chan.read();
    co_await p;
    assert(sum == 6);
    co_return 0;
}
