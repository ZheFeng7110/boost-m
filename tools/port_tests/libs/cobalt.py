"""Boost.Cobalt: libs/cobalt/example/ (examples.cpp)."""
import common as c
from net_gen import Gen

HEADER = """\
// mcpplibs.mwrap.boost.cobalt — the official examples (libs/cobalt/example/),
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers and the
// cobalt sources alone. A case cannot define main (which
// <boost/cobalt/main.hpp> does), nor a file one co_main per example: each
// co_main becomes a task<int> (ex_co_main) that ex_cobalt_main runs with
// cobalt::run. echo_server listens on a free port instead of 55555 and
// a harness client exchanges one message with it; delay / delay_op get a
// 10 ms delay as argv[1]. The output of thread and thread_pool (worker
// threads) is not compared.
"""
CO = [(r"\bcobalt::main co_main\(", "boost::cobalt::task<int> ex_co_main("),
      (r"\Z", "\nint example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }\n")]
# Its operator co_await (namespace boost::signals2, for ADL) uses the example's
# own signal_awaitable: the case namespace closes around it, in the original order.
SIG_NS = "ex_signals"
SIGNALS = CO + [
    (r"\nnamespace boost::signals2\n\{", "\n}  // namespace " + SIG_NS + " (reopened after the operator)\n namespace boost::signals2\n{"),
    (r"(return \{sig\};\n\}\n\n\})", "\\1\nnamespace " + SIG_NS + " {"),
    (r"-> signal_awaitable<", "-> ::" + SIG_NS + "::signal_awaitable<"),
    (r"operator co_await\(signals::signal<Args\.\.\.> & sig\)", "operator co_await(boost::signals2::signal<Args...> & sig)"),
    (r"signal_awaitable<signals::signal<Args\.\.\.>>", "signal_awaitable<boost::signals2::signal<Args...>>")]


def run(group):
    g = Gen("tests/cobalt/examples.cpp", HEADER, lib="cobalt", modules="asio,cobalt,outcome,lockfree,signals2",
            helpers=("asio.hpp", "cobalt.hpp"), oracle_includes=("boost/asio.hpp", "boost/cobalt.hpp"),
            oracle_args=(c.oracle_lib("cobalt_oracle"),), includes=("boost/cobalt/config.hpp",))
    for s in ["channel", "outcome"]:
        g.add(s, re=CO)
    # (spsc is not ported: its awaitable queue races; coverage.toml has the reason)
    # worker threads: the order of their output varies
    g.add("thread", re=CO, nocheck="1")
    g.add("thread_pool", nocheck="1", re=[(r"\bex_printf\(", "ex_printf_mt(")])  # a plain main
    for s in ["delay", "delay_op"]:
        g.add(s, re=CO, argv=['"10"'])
    g.add("signals", re=SIGNALS)
    g.add("echo_server", re=CO + [(r"\b55555\b", "ex_port_number()")],
          setup="ex_port = ex_free_port();", serve='ex_tcp_exchange(ex_port, "hello", 5)')
    return g.run()
