"""Boost.Asio: libs/asio/example/. The examples that run without a network
peer (examples_cpp11/14/20.cpp) and the ones that need one, over loopback in
one process (network_*.cpp)."""
import re
import sys
from pathlib import Path

import common as c
from net_gen import Gen, serve_main, ns

GROUPS = ["cpp11", "cpp14", "cpp20", "net-cpp11", "net-serialization", "net-http", "net-cpp14-cpp20"]

D = c.lib_dir("asio") / "example"


def classify():
    """{path under example/: counts of lines that mention argv[, acceptor,
    a resolver or connect(, and std::cin or getline}."""
    pats = {"argv": r"argv\[", "acceptor": r"acceptor", "connect": r"resolver|connect\(", "cin": r"std::cin|getline"}
    cls = {}
    for f in sorted(D.rglob("*.cpp")):
        lines = f.read_text(errors="replace").splitlines()
        cls[f.relative_to(D).as_posix()] = {k: sum(1 for l in lines if re.search(p, l)) for k, p in pats.items()}
    return cls


# Self-contained apart from argv / stdin, which the cases supply.
INPUTS = {"executors/fork_join", "executors/pipeline", "executors/async_1", "executors/async_2",
          "local/connect_pair", "files/async_file_copy", "files/blocking_file_copy",
          "invocation/prioritised_handlers", "invocation/completion_executor"}
SKIP = re.compile(r"http/|fork/|services/|daytime|type_erasure/(sleep|stdin_line_reader)")
HEADER = """\
// mcpplibs.mwrap.boost.asio — the official {v} examples (libs/asio/example/{v}/)
// that run without a network peer, ported mechanically: each program runs as a
// case and its output must equal what the same code printed when built from
// upstream/boost headers alone. The operations/ examples accept three
// connections on a fixed port: here the port is a free one and a background
// peer (the loopback harness below) connects and drains, so they run in one
// process; their 1 s delay between repeated messages is 10 ms here (the
// output does not depend on it). Outputs not compared: parallel_sort (completion order), the
// parallel_group examples that write to the process's stdout/stderr
// descriptors (bypassing the captured stream, completion order varies) and
// priority_scheduler (std::priority_queue does not order equal priorities) and
// prioritised_handlers (whether the accept has completed when the queue is
// first run depends on the host's timing) and tutorial/timer5 (two threads,
// both timers due at the same instant), and composed_4 on Windows (the system's
// text for EINVAL).
// boost/asio/yield.hpp / unyield.hpp (the stackless-coroutine macros) stay
// where the examples include them; BOOST_ASIO_COMPLETION_TOKEN_FOR(sig) is
// written as the completion_token_for<sig> concept it expands to; boost/asio/coroutine.hpp is included first
// so those in-place includes add only the macros. The deferred examples print
// ec.message() through ex_message(), which spells no error "Success" (the
// C library's own text for it differs between platforms); programs that need
// a POSIX stream_descriptor or socketpair are compiled only where asio has them.
// The actor benchmark passes its token 500 thousand times, not 50 million.
"""


def examples(v):
    cls = classify()
    selfcontained = lambda f: cls[f]["argv"] == 0 and cls[f]["acceptor"] == 0 and cls[f]["cin"] == 0
    files = sorted(f for f in cls if f.startswith(v + "/") and not SKIP.search(f) and
                   (selfcontained(f) or "/operations/" in f or f.split("/", 1)[1][:-4] in INPUTS))
    cmd = ["--lib", "asio", "--modules", "asio,thread" if v == "cpp11" else "asio",
           "--case-from-rel", "--inline-local", "--extra-helpers", c.helper("asio.hpp"),
           "--add-include", "boost/asio/detail/config.hpp", "--add-include", "boost/asio/coroutine.hpp",
           "--oracle-arg=-lpthread",
           "--oracle-arg=-include", "--oracle-arg=thread", "--oracle-arg=-include", "--oracle-arg=functional",
           "--oracle-arg=-include", "--oracle-arg=random", "--oracle-arg=-include", "--oracle-arg=boost/asio.hpp",
           "--out", "tests/asio/examples_%s.cpp" % v, "--header", HEADER.format(v=v),
           "--output-unless", "parallel_sort=1", "--output-unless", "wait_for_all=1",
           "--output-unless", "ranged_wait_for_all=1", "--output-unless", "priority_scheduler=1",
           "--output-unless", "prioritised_handlers=1",
           "--output-unless", "cpp11/tutorial/timer5/timer=1",
           # composed_4 prints the system's text for EINVAL (Windows: WSA's
           # "An invalid argument was supplied", code 10022)
           "--output-unless", "composed_4=defined(BOOST_ASIO_WINDOWS)",
           "--post-re", r"parallel_sort:BOOST_ASIO_COMPLETION_TOKEN_FOR\((void\([^)]*\))\)=>boost::asio::completion_token_for<\1>",
           "--merge", "main=sleep.cpp,stdin_line_reader.cpp",
           "--stdin", "main=line one\\nline two\\n",
           # type_erasure reads the process's stdin descriptor: a pipe holding the same text here
           "--post-sub", "main:::dup(STDIN_FILENO)=>ex_stdin_fd(ex_in.str())",
           # type_erasure reads a POSIX stream_descriptor; connect_pair: Windows has
           # AF_UNIX sockets but no socketpair (asio's connect_pair fails at run time)
           "--only-if", "main=defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)",
           "--only-if", "connect_pair=!defined(BOOST_ASIO_WINDOWS)",
           # the parallel_group examples are POSIX-only (an empty main elsewhere)
           *sum((["--output-unless", "%s=!defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)" % x] for x in [
               "ranged_wait_for_all", "wait_for_all", "wait_for_one", "wait_for_one_error", "wait_for_one_success"]), []),
           # actor is a benchmark: 50M hops through a ring of 503 actors (35 s on one
           # loaded CPU); 500k hops exercise the same code
           "--post-re", r"actor:\b50000000\b=>500000",
           "--argv", "fork_join=\"1000\"",
           "--stdin", "pipeline=the quick brown fox\\njumps over the lazy dog\\n",
           "--stdin", "async_1=first line\\n", "--stdin", "async_2=one\\ntwo\\nthree\\n",
           "--stdin", "connect_pair=hello\\nworld\\n",
           "--stdin", "callback_wrapper=a line of input\\n", "--stdin", "c_callback_wrapper=a line of input\\n",
           "--oracle-for", "connect_pair=" + c.oracle_lib("thread_oracle"),
           "--output-unless", "fork_join=1",
           # without BOOST_ASIO_HAS_FILE their main() takes no arguments
           "--post-re", r"async_file_copy:int example_main\(\)=>int example_main(int, char*[])",
           "--post-re", r"blocking_file_copy:int example_main\(\)=>int example_main(int, char*[])",
           # connect_pair loops for ever; at the end of the input the port stops the
           # filter thread and leaves the loop
           "--post-sub", "connect_pair:std::getline(ex_in, request);=>if (!std::getline(ex_in, request)) { io_context.stop(); thread.join(); break; }",
           "--setup", "async_file_copy=static const std::string ex_src = ex_temp_path(\"copy-src\"); std::ofstream(ex_src) << \"file contents\\n\";",
           "--argv", "async_file_copy=ex_src,,ex_temp_path(\"copy-dst\")",
           "--setup", "blocking_file_copy=static const std::string ex_src = ex_temp_path(\"copy-src\"); std::ofstream(ex_src) << \"file contents\\n\";",
           "--argv", "blocking_file_copy=ex_src,,ex_temp_path(\"copy-dst\")"]
    for f in files:
        stem = Path(f).stem
        if "/deferred/" in f:  # the text of "no error" is the C library's
            cmd += ["--post-re", stem + r":\bec\.message\(\)=>ex_message(ec)"]
        if "/operations/" in f and "composed" in stem:
            cmd += ["--setup", stem + "=ex_port = ex_free_port(); ex_peer_async(ex_port, 3);",
                    "--post-re", stem + r":\b55555\b=>ex_port_number()",
                    "--post-re", stem + r":std::chrono::seconds\(1\)=>std::chrono::milliseconds(10)"]
    cmd += [str(D / f) for f in files]
    print("==", v, len(files))
    sys.stdout.flush()
    return c.port(cmd)


# ---- the network examples ----
NET_HEADER = """\
// mcpplibs.mwrap.boost.asio — the official {what} examples that need a network
// peer, run over the loopback interface in one process. A server example
// runs in a background thread on a free port (ex_serve_async) and a harness
// client of the loopback harness below talks to it; the case output is what
// that client received. A client example runs against the ported server
// example it belongs with, started the same way. The expected output is what
// the same code printed when built from upstream/boost headers alone. Fixed
// ports (13, 55555, 54321) are replaced by the free one, "daytime"/"http"
// service names by its number, and output that carries the time of day is
// not compared.
"""
ECHO = 'ex_tcp_exchange(ex_port, "hello", 5)'
UDP_ECHO = 'ex_udp_exchange(ex_port, "hello")'
TCP_TIME = 'ex_tcp_exchange(ex_port, "", 0, false)'
UDP_TIME = 'ex_udp_exchange(ex_port, "x", false)'
P = ["ex_port"]
FREE = "ex_port = ex_free_port();"
FREE_UDP = "ex_port = ex_free_udp_port();"
LOCAL = 'ex_port = ex_local_path("stream.sock");'
# The fd-passing pair calls sendmsg/recvmsg; Windows has AF_UNIX stream sockets
# but binding one fails there (WSAEOPNOTSUPP on the CI hosts).
POSIX = "!defined(BOOST_ASIO_WINDOWS)"


def echo_family(g, v):
    for s in ["echo/async_tcp_echo_server", "echo/blocking_tcp_echo_server"]:
        g.add(v + "/" + s, setup=FREE, argv=P, serve=ECHO)
    for s in ["echo/async_udp_echo_server", "echo/blocking_udp_echo_server"]:
        g.add(v + "/" + s, setup=FREE_UDP, argv=P, serve=UDP_ECHO)
    srv = v + "/echo/blocking_tcp_echo_server"
    g.add(v + "/echo/blocking_tcp_echo_client", with_=[srv],
          setup=FREE + " " + serve_main(ns(srv), ["p"]), argv=['"127.0.0.1"', "ex_port"], stdin="hello world\\n")
    srv = v + "/echo/blocking_udp_echo_server"
    g.add(v + "/echo/blocking_udp_echo_client", with_=[srv],
          setup=FREE_UDP + " " + serve_main(ns(srv), ["p"]), argv=['"127.0.0.1"', "ex_port"], stdin="hello world\\n")


def net_cpp11():
    # cpp11: echo, daytime, chat, local, timeouts, misc
    g = Gen("tests/asio/network_cpp11.cpp", NET_HEADER.format(what="C++11 (libs/asio/example/cpp11/)"),
            # handler_tracking/ uses the BOOST_ASIO_HANDLER_LOCATION macro; the
            # fd-passing pair calls sendmsg/recvmsg, which boost/asio.hpp reaches
            includes=["boost/asio/detail/handler_tracking.hpp"],
            pre="// The local/ examples (UNIX domain sockets; the fd-passing pair also calls\n"
                "// sendmsg/recvmsg) are compiled on POSIX systems only.\n"
                "#if !defined(_WIN32)\n#include <sys/socket.h>\n#include <sys/uio.h>\n#endif\n")
    echo_family(g, "cpp11")
    for s in ["handler_tracking/async_tcp_echo_server", "allocation/server", "nonblocking/third_party_lib"]:
        g.add("cpp11/" + s, setup=FREE, argv=P, serve=ECHO)
    g.add("cpp11/buffers/reference_counted", setup=FREE, argv=P, serve=TCP_TIME)
    g.add("cpp11/chat/chat_server", setup=FREE, argv=P, serve='ex_tcp_exchange(ex_port, "   5hello", 9)')
    srv = "cpp11/chat/chat_server"
    g.add("cpp11/chat/chat_client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), ["p"]),
          argv=['"127.0.0.1"', "ex_port"], stdin="hello chat\\n", nocheck="1")
    T13 = [(r"tcp::v4\(\), 13\)", "tcp::v4(), ex_port_number())")]
    U13 = [(r"udp::v4\(\), 13\)", "udp::v4(), ex_port_number())")]
    DAYTIME = [(r'"daytime"', "ex_port")]
    for s in ["tutorial/daytime2/server", "tutorial/daytime3/server"]:
        g.add("cpp11/" + s, setup=FREE, serve=TCP_TIME, re=T13)
    for s in ["tutorial/daytime5/server", "tutorial/daytime6/server"]:
        g.add("cpp11/" + s, setup=FREE_UDP, serve=UDP_TIME, re=U13)
    g.add("cpp11/tutorial/daytime7/server", setup=FREE, serve=TCP_TIME + " + " + UDP_TIME, re=T13 + U13)
    g.add("cpp11/iostreams/daytime_server", setup=FREE, serve=TCP_TIME, re=T13)
    srv = "cpp11/tutorial/daytime2/server"
    g.add("cpp11/tutorial/daytime1/client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), []),
          argv=['"127.0.0.1"'], re=DAYTIME, nocheck="1")
    g.add("cpp11/services/daytime_client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), []),
          argv=['"127.0.0.1"'], re=DAYTIME, merge=["logger_service.cpp"],
          sub=[('logger.use_file("log.txt");', 'logger.use_file(ex_temp_path("log.txt"));')], nocheck="1")
    srv = "cpp11/tutorial/daytime5/server"
    g.add("cpp11/tutorial/daytime4/client", with_=[srv], setup=FREE_UDP + " " + serve_main(ns(srv), []),
          argv=['"127.0.0.1"'], re=DAYTIME, nocheck="1")
    g.add("cpp11/futures/daytime_client", with_=[srv], setup=FREE_UDP + " " + serve_main(ns(srv), []),
          argv=['"127.0.0.1"'], re=DAYTIME, nocheck="1")
    srv = "cpp11/iostreams/daytime_server"
    g.add("cpp11/iostreams/daytime_client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), []),
          argv=['"127.0.0.1"'], re=DAYTIME, nocheck="1")
    srv = "cpp11/echo/blocking_tcp_echo_server"
    g.add("cpp11/timeouts/blocking_tcp_client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), ["p"]),
          argv=['"127.0.0.1"', "ex_port", '"hello"'], nocheck="1")  # prints the round-trip time
    # Its async_result specialization (namespace boost::asio) uses the example's
    # own token type: the case namespace closes around it, in the original order.
    TOKEN_NS = "ex_cpp11_timeouts_blocking_token_tcp_client"
    g.add("cpp11/timeouts/blocking_token_tcp_client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), ["p"]),
          argv=['"127.0.0.1"', "ex_port", '"hello"'],
          re=[(r"\nnamespace boost \{\nnamespace asio \{",
               "\n}  // namespace " + TOKEN_NS + " (reopened after the specialization)\n namespace boost {\nnamespace asio {"),
              (r"\} // namespace asio\n\} // namespace boost",
               "} // namespace asio\n} // namespace boost\nnamespace " + TOKEN_NS + " {"),
              (r"async_result<close_after,", "async_result<::" + TOKEN_NS + "::close_after,"),
              (r"Init init, close_after token", "Init init, ::" + TOKEN_NS + "::close_after token")], nocheck="1")
    g.add("cpp11/local/stream_server", setup=LOCAL, argv=P, serve='ex_local_exchange(ex_port, "hello", 5)',
          nocheck="!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)", only_if=POSIX)
    srv = "cpp11/local/stream_server"
    for s in ["local/stream_client", "local/iostream_client"]:
        g.add("cpp11/" + s, with_=[srv], setup=LOCAL + " " + serve_main(ns(srv), ["p"]), argv=P,
              stdin="hello local\\n", nocheck="!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)", only_if=POSIX)
    g.add("cpp11/local/fd_passing_stream_server", setup=LOCAL, argv=P,
          serve='ex_local_exchange(ex_port, ex_temp_path("fd-target.txt"), 1, false)',
          nocheck="!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)", only_if=POSIX)
    srv = "cpp11/local/fd_passing_stream_server"
    g.add("cpp11/local/fd_passing_stream_client", with_=[srv], setup=LOCAL + " " + serve_main(ns(srv), ["p"]),
          argv=P, stdin="\\n", nocheck="1", only_if=POSIX,  # prints the descriptor number
          # the path to write to, typed on stdin upstream: a temporary file here
          sub=[("ex_in.getline(request, max_length);",
                'std::strcpy(request, ex_temp_path("fd-passing.txt").c_str());')])
    return g.run()


def net_serialization():
    # serialization: needs the serialization module
    g = Gen("tests/asio/network_serialization.cpp", NET_HEADER.format(what="cpp11/serialization (libs/asio/example/cpp11/serialization/)"),
            modules="asio,serialization")
    g.add("cpp11/serialization/server", setup=FREE, argv=P, serve=TCP_TIME, oracle=c.oracle_lib("boost_serialization"))
    srv = "cpp11/serialization/server"
    g.add("cpp11/serialization/client", with_=[srv], setup=FREE + " " + serve_main(ns(srv), ["p"]),
          argv=['"127.0.0.1"', "ex_port"], oracle=c.oracle_lib("boost_serialization"))
    return g.run()


def net_http():
    # http: the four servers and the clients
    g = Gen("tests/asio/network_http.cpp", NET_HEADER.format(what="HTTP (libs/asio/example/cpp11/http/, iostreams/http_client)"))
    HTTP_GET = 'ex_tcp_exchange(ex_port, "GET /index.html HTTP/1.0\\r\\n\\r\\n", 0)'
    SRC = ["connection.cpp", "connection_manager.cpp", "mime_types.cpp", "reply.cpp", "request_handler.cpp",
           "request_parser.cpp", "server.cpp"]
    DOC = "static const std::string ex_root = ex_doc_root();"
    g.add("cpp11/http/server/main", setup=FREE + " " + DOC, argv=['"127.0.0.1"', "ex_port", "ex_root"],
          serve=HTTP_GET, merge=SRC)
    g.add("cpp11/http/server2/main", setup=FREE + " " + DOC, argv=['"127.0.0.1"', "ex_port", '"2"', "ex_root"],
          serve=HTTP_GET, merge=[s for s in SRC if s != "connection_manager.cpp"] + ["io_context_pool.cpp"])
    g.add("cpp11/http/server3/main", setup=FREE + " " + DOC, argv=['"127.0.0.1"', "ex_port", '"2"', "ex_root"],
          serve=HTTP_GET, merge=[s for s in SRC if s != "connection_manager.cpp"])
    g.add("cpp11/http/server4/main", setup=FREE + " " + DOC, argv=['"127.0.0.1"', "ex_port", "ex_root"],
          serve=HTTP_GET, merge=["file_handler.cpp", "mime_types.cpp", "reply.cpp", "request_parser.cpp", "server.cpp"])
    srv = "cpp11/http/server/main"
    HTTP = [(r'"http"', "ex_port")]
    for client in ["cpp11/http/client/sync_client", "cpp11/http/client/async_client", "cpp11/iostreams/http_client",
              "cpp14/iostreams/http_client"]:
        g.add(client, with_=[srv], setup=FREE + " " + DOC + " " + serve_main(ns(srv), ['"127.0.0.1"', "p", "ex_root"]),
              argv=['"127.0.0.1"', '"/index.html"'], re=HTTP)
    return g.run()


def net_cpp14_cpp20():
    # cpp14 echo; cpp17 / cpp20 coroutine servers and channels
    g = Gen("tests/asio/network_cpp14_cpp20.cpp", NET_HEADER.format(what="C++14 echo, C++17 coroutines_ts and C++20 coroutines/channels"))
    echo_family(g, "cpp14")
    P55 = [(r"\b55555\b", "ex_port_number()")]
    for v in ["cpp17/coroutines_ts", "cpp20/coroutines"]:
        for s in ["echo_server", "echo_server_with_as_single_default", "echo_server_with_as_tuple_default",
                  "echo_server_with_default", "refactored_echo_server"] + \
                 (["echo_server_with_deferred", "echo_server_with_deferred_default"] if v.startswith("cpp20") else []):
            g.add(v + "/" + s, setup=FREE, serve=ECHO, re=P55)
        g.add(v + "/chat_server", setup=FREE, argv=P, serve='ex_tcp_exchange(ex_port, "hello\\n", 6)')
    for s in ["timeout_cancel_after", "timeout_watchdog"]:
        g.add("cpp20/coroutines/" + s, setup=FREE, serve=ECHO, re=[(r"\b54321\b", "ex_port_number()")])
    for s in ["mutual_exclusion_1", "mutual_exclusion_2"]:
        g.add("cpp20/channels/" + s, setup=FREE, argv=['"127.0.0.1"', "ex_port"],
              serve='ex_tcp_exchange(ex_port, "hi\\n", 9)')
    g.add("cpp20/channels/throttling_proxy", setup=FREE + " static const std::string ex_up = ex_harness_echo();",
          argv=['"127.0.0.1"', "ex_port", '"127.0.0.1"', "ex_up"], serve=ECHO)
    return g.run()


def run(group):
    if group in ("cpp11", "cpp14", "cpp20"):
        return examples(group)
    return {"net-cpp11": net_cpp11, "net-serialization": net_serialization, "net-http": net_http,
            "net-cpp14-cpp20": net_cpp14_cpp20}[group]()
